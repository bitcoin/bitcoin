// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <core_io.h>
#include <hash.h>
#include <key.h>
#include <key_io.h>
#include <pubkey.h>
#include <script/standard.h>
#include <uint256.h>
#include <util/message.h>
#include <util/strencodings.h>

#include <cassert>
#include <optional>
#include <string>
#include <variant>
#include <vector>

/**
 * Text used to signify that a signed message follows and to prevent
 * inadvertently signing a transaction.
 */
const std::string MESSAGE_MAGIC = "Bitcoin Signed Message:\n";

/**
 * BIP-322 tagged hash
 */
static const HashWriter HASHER_BIP322{TaggedHash("BIP0322-signed-message")};

static constexpr unsigned int BIP322_REQUIRED_FLAGS =
    SCRIPT_VERIFY_CONST_SCRIPTCODE // disallows OP_CODESEPARATOR and FindAndDelete
|   SCRIPT_VERIFY_LOW_S
|   SCRIPT_VERIFY_STRICTENC
|   SCRIPT_VERIFY_NULLFAIL
|   SCRIPT_VERIFY_MINIMALDATA
|   SCRIPT_VERIFY_CLEANSTACK
|   SCRIPT_VERIFY_P2SH
|   SCRIPT_VERIFY_WITNESS
|   SCRIPT_VERIFY_TAPROOT
|   SCRIPT_VERIFY_MINIMALIF;

static constexpr unsigned int BIP322_INCONCLUSIVE_FLAGS =
    SCRIPT_VERIFY_DISCOURAGE_OP_SUCCESS
|   SCRIPT_VERIFY_DISCOURAGE_UPGRADABLE_NOPS
|   SCRIPT_VERIFY_DISCOURAGE_UPGRADABLE_PUBKEYTYPE
|   SCRIPT_VERIFY_DISCOURAGE_UPGRADABLE_TAPROOT_VERSION
|   SCRIPT_VERIFY_DISCOURAGE_UPGRADABLE_WITNESS_PROGRAM;

MessageVerificationResult MessageVerifyBIP322(
    CTxDestination& destination,
    std::vector<unsigned char>& signature,
    const std::string& message,
    MessageVerificationResult legacyError)
{
    auto txs = BIP322Txs::Create(destination, message, legacyError, signature);
    if (!txs) return legacyError;

    const CTransaction& to_sign = txs->m_to_sign;
    const CTransaction& to_spend = txs->m_to_spend;

    const CScript scriptSig = to_sign.vin[0].scriptSig;
    const CScriptWitness& witness = to_sign.vin[0].scriptWitness;

    PrecomputedTransactionData txdata;
    txdata.Init(to_sign, {to_spend.vout[0]});
    TransactionSignatureChecker sigcheck(&to_sign, /* nInIn= */ 0, /* amountIn= */ to_spend.vout[0].nValue, txdata, MissingDataBehavior::ASSERT_FAIL, true);

    if (!VerifyScript(scriptSig, to_spend.vout[0].scriptPubKey, &witness, BIP322_REQUIRED_FLAGS, sigcheck)) {
        return MessageVerificationResult::ERR_INVALID;
    }

    // inconclusive checks

    if (to_sign.nVersion != 0 && to_sign.nVersion != 2) {
        return MessageVerificationResult::INCONCLUSIVE;
    }

    if (!VerifyScript(scriptSig, to_spend.vout[0].scriptPubKey, &witness, BIP322_INCONCLUSIVE_FLAGS, sigcheck)) {
        return MessageVerificationResult::INCONCLUSIVE;
    }

    // timelock check
    if (to_sign.nLockTime > 0 || to_sign.vin[0].nSequence > 0) {
        return MessageVerificationResult::OK_TIMELOCKED;
    }

    return MessageVerificationResult::OK;
}

MessageVerificationResult MessageVerify(
    const std::string& address,
    const std::string& signature,
    const std::string& message)
{
    auto signature_bytes = DecodeBase64(signature);
    if (!signature_bytes) {
        return MessageVerificationResult::ERR_MALFORMED_SIGNATURE;
    }

    CTxDestination destination = DecodeDestination(address);
    if (!IsValidDestination(destination)) {
        return MessageVerificationResult::ERR_INVALID_ADDRESS;
    }

    if (std::get_if<PKHash>(&destination) == nullptr) {
        return MessageVerifyBIP322(destination, *signature_bytes, message, MessageVerificationResult::ERR_ADDRESS_NO_KEY);
    }

    CPubKey pubkey;
    if (!pubkey.RecoverCompact(MessageHash(message, MessageSignatureFormat::LEGACY), *signature_bytes)) {
        return MessageVerifyBIP322(destination, *signature_bytes, message, MessageVerificationResult::ERR_PUBKEY_NOT_RECOVERED);
    }

    if (!(CTxDestination(PKHash(pubkey)) == destination)) {
        /*
        By the time we reach this line, the destination must be `PKHash` and
        we must have been able to recover a pubkey from the signature, which
        means that the signature must have been 65 bytes, otherwise
        `RecoverCompact` will have failed. However, a BIP322 signature for
        `PKHash` must be greater than 65 bytes because it will include a full
        transaction for the p2pkh address. So if we do reach this line, the
        signature is just invalid and there's no need to attempt a
        `MessageVerifyBIP322`.
        */
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

    if (!privkey.SignCompact(MessageHash(message, MessageSignatureFormat::LEGACY), signature_bytes)) {
        return false;
    }

    signature = EncodeBase64(signature_bytes);

    return true;
}

uint256 MessageHash(const std::string& message, MessageSignatureFormat format)
{
    switch (format) {
    case MessageSignatureFormat::LEGACY:
        {
            HashWriter hasher{};
            hasher << MESSAGE_MAGIC << message;
            return hasher.GetHash();
        }

    case MessageSignatureFormat::SIMPLE:
    case MessageSignatureFormat::FULL:
        {
            HashWriter hasher{HASHER_BIP322};
            if (!message.empty()) {
                hasher.write(AsBytes(Span{message.data(), message.size() * sizeof(char)}));
            }
            return hasher.GetSHA256();
        }
    }
    assert(false);
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
        // no default case, so the compiler can warn about missing cases
    }
    assert(false);
}

std::optional<BIP322Txs> BIP322Txs::Create(const CTxDestination& destination, const std::string& message, MessageVerificationResult& result, std::optional<const std::vector<unsigned char>> signature)
{
    // attempt to get script pub key for destination
    CScript message_challenge = GetScriptForDestination(destination);
    if (message_challenge.size() == 0) {
        // NoDestination; failure
        // (use legacy result)
        return std::nullopt;
    }

    // prepare message hash
    uint256 message_hash = MessageHash(message, MessageSignatureFormat::SIMPLE);
    std::vector<unsigned char> message_hash_vec(message_hash.begin(), message_hash.end());

    // generate to_spend transaction
    CMutableTransaction to_spend;
    to_spend.nVersion = 0;
    to_spend.nLockTime = 0;
    to_spend.vin.emplace_back(COutPoint(uint256::ZERO, 0xFFFFFFFF), (CScript() << OP_0 << message_hash_vec), 0);
    to_spend.vout.emplace_back(0, message_challenge);

    CMutableTransaction to_sign;
    if (signature.has_value() && DecodeTx(to_sign, signature.value(), /* try_no_witness= */ true, /* try_witness= */ true)) {
        // validate decoded transaction
        // multiple inputs (proof of funds) are not supported as we do not have UTXO set access
        if (to_sign.vin.size() > 1) {
            result = MessageVerificationResult::ERR_POF;
            return std::nullopt;
        }
        if ((to_sign.vin.size() == 0 || to_sign.vin[0].prevout.hash != to_spend.GetHash()) ||
            (to_sign.vin[0].prevout.n != 0) ||
            (to_sign.vout.size() != 1) ||
            (to_sign.vout[0].nValue != 0) ||
            (to_sign.vout[0].scriptPubKey != (CScript() << OP_RETURN))) {
            result = MessageVerificationResult::ERR_INVALID;
            return std::nullopt;
        }
    } else {
        // signature is missing, or a witness stack only
        to_sign.nVersion = 0;
        to_sign.nLockTime = 0;
        to_sign.vin.emplace_back(COutPoint(to_spend.GetHash(), 0), CScript(), 0);
        if (signature.has_value()) {
            try {
                CDataStream ds(signature.value(), SER_NETWORK, PROTOCOL_VERSION);
                ds >> to_sign.vin[0].scriptWitness.stack;
                if (!ds.empty()) {
                    result = MessageVerificationResult::ERR_INVALID;
                    return std::nullopt;
                }
            } catch (...) {
                // not a script witness either; fall back to legacy error
                // (use legacy result)
                return std::nullopt;
            }
        }
        to_sign.vout.emplace_back(0, CScript() << OP_RETURN);
    }

    return BIP322Txs{to_spend, to_sign};
}
