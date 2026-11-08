// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <common/signmessage.h>

#include <addresstype.h>
#include <consensus/consensus.h>
#include <consensus/validation.h>
#include <hash.h>
#include <key.h>
#include <key_io.h>
#include <primitives/transaction.h>
#include <pubkey.h>
#include <script/interpreter.h>
#include <script/script.h>
#include <script/script_error.h>
#include <serialize.h>
#include <span.h>
#include <streams.h>
#include <uint256.h>
#include <util/check.h>
#include <util/strencodings.h>

#include <cstdint>
#include <ios>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

/**
 * Text used to signify that a signed message follows and to prevent
 * inadvertently signing a transaction.
 */
const std::string MESSAGE_MAGIC = "Bitcoin Signed Message:\n";

MessageVerificationResult MessageVerify(
    const std::string& address,
    const std::string& signature,
    const std::string& message)
{
    CTxDestination destination = DecodeDestination(address);
    if (!IsValidDestination(destination)) {
        return MessageVerificationResult::ERR_INVALID_ADDRESS;
    }

    if (std::get_if<PKHash>(&destination) == nullptr) {
        return MessageVerificationResult::ERR_ADDRESS_NO_KEY;
    }

    auto signature_bytes = DecodeBase64(signature);
    if (!signature_bytes) {
        return MessageVerificationResult::ERR_MALFORMED_SIGNATURE;
    }

    CPubKey pubkey;
    if (!pubkey.RecoverCompact(MessageHash(message), *signature_bytes)) {
        return MessageVerificationResult::ERR_PUBKEY_NOT_RECOVERED;
    }

    if (!(PKHash(pubkey) == *std::get_if<PKHash>(&destination))) {
        return MessageVerificationResult::ERR_NOT_SIGNED;
    }

    return MessageVerificationResult::OK;
}

bool MessageSign(
    const CKey& privkey,
    const std::string& message,
    std::string& signature)
{
    std::vector<unsigned char> signature_bytes;

    if (!privkey.SignCompact(MessageHash(message), signature_bytes)) {
        return false;
    }

    signature = EncodeBase64(signature_bytes);

    return true;
}

uint256 MessageHash(const std::string& message)
{
    HashWriter hasher{};
    hasher << MESSAGE_MAGIC << message;

    return hasher.GetHash();
}

uint256 BIP322MessageHash(std::string_view message)
{
    HashWriter hasher{TaggedHash("BIP0322-signed-message")};
    hasher.write(MakeByteSpan(message));
    return hasher.GetSHA256();
}

BIP322Transactions CreateBIP322Transactions(const CScript& message_challenge, std::string_view message)
{
    BIP322Transactions txs;
    txs.to_spend.version = 0;
    txs.to_spend.nLockTime = 0;
    COutPoint null_prevout;
    null_prevout.SetNull();
    txs.to_spend.vin.emplace_back(null_prevout, CScript() << OP_0 << ToByteVector(BIP322MessageHash(message)), /*nSequenceIn=*/0);
    txs.to_spend.vout.emplace_back(/*nValueIn=*/0, message_challenge);

    txs.to_sign.version = 0;
    txs.to_sign.nLockTime = 0;
    txs.to_sign.vin.emplace_back(COutPoint{txs.to_spend.GetHash(), 0}, CScript{}, /*nSequenceIn=*/0);
    txs.to_sign.vout.emplace_back(/*nValueIn=*/0, CScript() << OP_RETURN);
    return txs;
}

namespace {

// Consensus activation rules and BIP322's required rules, independent of mempool policy.
constexpr script_verify_flags BIP322_REQUIRED_FLAGS{
    SCRIPT_VERIFY_P2SH | SCRIPT_VERIFY_DERSIG | SCRIPT_VERIFY_NULLDUMMY |
    SCRIPT_VERIFY_CHECKLOCKTIMEVERIFY | SCRIPT_VERIFY_CHECKSEQUENCEVERIFY |
    SCRIPT_VERIFY_WITNESS | SCRIPT_VERIFY_TAPROOT | SCRIPT_VERIFY_LOW_S |
    SCRIPT_VERIFY_STRICTENC | SCRIPT_VERIFY_NULLFAIL | SCRIPT_VERIFY_MINIMALDATA |
    SCRIPT_VERIFY_CLEANSTACK | SCRIPT_VERIFY_MINIMALIF | SCRIPT_VERIFY_CONST_SCRIPTCODE};

constexpr script_verify_flags BIP322_UPGRADE_FLAGS{
    SCRIPT_VERIFY_DISCOURAGE_UPGRADABLE_NOPS | SCRIPT_VERIFY_DISCOURAGE_UPGRADABLE_WITNESS_PROGRAM |
    SCRIPT_VERIFY_DISCOURAGE_UPGRADABLE_TAPROOT_VERSION | SCRIPT_VERIFY_DISCOURAGE_OP_SUCCESS |
    SCRIPT_VERIFY_DISCOURAGE_UPGRADABLE_PUBKEYTYPE};

class BIP322SignatureChecker : public DeferringSignatureChecker
{
public:
    using DeferringSignatureChecker::DeferringSignatureChecker;

    bool CheckECDSASignature(const std::vector<unsigned char>& sig, const std::vector<unsigned char>& pubkey,
                             const CScript& script, SigVersion sigversion) const override
    {
        if (!sig.empty() && sig.back() != SIGHASH_ALL) return false;
        return m_checker.CheckECDSASignature(sig, pubkey, script, sigversion);
    }

    bool CheckSchnorrSignature(std::span<const unsigned char> sig, std::span<const unsigned char> pubkey,
                               SigVersion sigversion, ScriptExecutionData& data, ScriptError* error) const override
    {
        if (sig.size() == 65 && sig.back() != SIGHASH_ALL) {
            if (error) *error = SCRIPT_ERR_SCHNORR_SIG_HASHTYPE;
            return false;
        }
        return m_checker.CheckSchnorrSignature(sig, pubkey, sigversion, data, error);
    }
};

std::optional<CScriptWitness> DecodeBIP322Witness(std::span<const unsigned char> bytes)
{
    SpanReader reader{bytes};
    CScriptWitness witness;
    try {
        const auto count = ReadCompactSize(reader);
        // Every item requires at least a CompactSize length, even if empty.
        if (count > reader.size()) return std::nullopt;
        witness.stack.reserve(count);
        for (uint64_t i{0}; i < count; ++i) {
            const auto size = ReadCompactSize(reader);
            if (size > reader.size()) return std::nullopt;
            auto& item = witness.stack.emplace_back(size);
            reader.read(MakeWritableByteSpan(item));
        }
        if (!reader.empty()) return std::nullopt;
    } catch (const std::ios_base::failure&) {
        return std::nullopt;
    }
    return witness;
}

bool HasCodeSeparator(const CScript& script)
{
    auto pc = script.begin();
    opcodetype opcode;
    while (script.GetOp(pc, opcode)) {
        if (opcode == OP_CODESEPARATOR) return true;
    }
    return false;
}

} // namespace

BIP322VerificationResult BIP322Verify(const CTxDestination& destination, std::string_view signature,
                                    std::string_view message, ScriptError* script_error)
{
    if (script_error) *script_error = SCRIPT_ERR_UNKNOWN_ERROR;
    const bool known = std::holds_alternative<WitnessV0KeyHash>(destination) ||
        std::holds_alternative<WitnessV0ScriptHash>(destination) ||
        std::holds_alternative<WitnessV1Taproot>(destination);
    if (!known) {
        const auto unknown = std::get_if<WitnessUnknown>(&destination);
        if (!unknown || unknown->GetWitnessVersion() < 1 || unknown->GetWitnessVersion() > 16 ||
            unknown->GetWitnessProgram().size() < 2 || unknown->GetWitnessProgram().size() > 40) {
            return BIP322VerificationResult::UNSUPPORTED_ADDRESS;
        }
    }
    // Canonical base64 has length 0 mod 4; a three-character variant makes it
    // 3 mod 4. An old unprefixed witness may itself start with smp, ful or pof.
    if (signature.size() % 4 == 3) {
        if (signature.starts_with("ful") || signature.starts_with("pof")) return BIP322VerificationResult::UNSUPPORTED_FORMAT;
        if (signature.starts_with("smp")) signature.remove_prefix(3);
    }
    // No consensus-valid transaction can carry more witness bytes than the block weight limit.
    if (signature.size() > 4 * ((MAX_BLOCK_WEIGHT + 2) / 3)) return BIP322VerificationResult::MALFORMED_WITNESS;
    const auto bytes = DecodeBase64(signature);
    if (!bytes) return BIP322VerificationResult::MALFORMED_BASE64;
    if (bytes->size() > MAX_BLOCK_WEIGHT) return BIP322VerificationResult::MALFORMED_WITNESS;
    auto witness = DecodeBIP322Witness(*bytes);
    if (!witness) return BIP322VerificationResult::MALFORMED_WITNESS;

    const auto challenge = GetScriptForDestination(destination);
    auto txs = CreateBIP322Transactions(challenge, message);
    txs.to_sign.vin[0].scriptWitness = std::move(*witness);
    const CTransaction tx{std::move(txs.to_sign)};
    if (GetTransactionWeight(tx) > int64_t{MAX_BLOCK_WEIGHT}) return BIP322VerificationResult::INVALID;
    PrecomputedTransactionData txdata;
    txdata.Init(tx, {txs.to_spend.vout[0]}, /*force=*/true);
    const TransactionSignatureChecker txchecker{&tx, 0, 0, txdata, MissingDataBehavior::FAIL};
    const BIP322SignatureChecker checker{txchecker};
    const auto& proof = tx.vin[0].scriptWitness;
    if (!VerifyScript({}, challenge, &proof, BIP322_REQUIRED_FLAGS, checker, script_error)) {
        return BIP322VerificationResult::INVALID;
    }

    // Only inspect scripts after Core has verified their commitment to the address.
    // A separator is forbidden even in an inactive branch; bytes inside pushes are data.
    std::span stack{proof.stack};
    bool annex{false};
    if (std::holds_alternative<WitnessV0ScriptHash>(destination)) {
        const auto& script = stack.back();
        if (HasCodeSeparator(CScript{script.begin(), script.end()})) {
            if (script_error) *script_error = SCRIPT_ERR_OP_CODESEPARATOR;
            return BIP322VerificationResult::INVALID;
        }
    } else if (std::holds_alternative<WitnessV1Taproot>(destination)) {
        annex = stack.size() >= 2 && !stack.back().empty() && stack.back()[0] == ANNEX_TAG;
        if (annex) stack = stack.first(stack.size() - 1);
        if (stack.size() >= 2 && (stack.back()[0] & TAPROOT_LEAF_MASK) == TAPROOT_LEAF_TAPSCRIPT) {
            const auto& script = stack[stack.size() - 2];
            if (HasCodeSeparator(CScript{script.begin(), script.end()})) {
                if (script_error) *script_error = SCRIPT_ERR_OP_CODESEPARATOR;
                return BIP322VerificationResult::INVALID;
            }
        }
    }
    // Required failure takes precedence over upgrades, including annexes and unknown keys.
    if (!VerifyScript({}, challenge, &proof, BIP322_REQUIRED_FLAGS | BIP322_UPGRADE_FLAGS, checker, script_error) || annex || !known) {
        return BIP322VerificationResult::INCONCLUSIVE;
    }
    return BIP322VerificationResult::OK;
}

std::string SigningResultString(const SigningResult res)
{
    switch (res) {
        case SigningResult::OK:
            return "No error";
        case SigningResult::PRIVATE_KEY_NOT_AVAILABLE:
            return "Private key not available";
        case SigningResult::SIGNING_FAILED:
            return "Sign failed";
    } // no default case, so the compiler can warn about missing cases
    assert(false);
}
