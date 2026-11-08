// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <addresstype.h>
#include <arith_uint256.h>
#include <common/signmessage.h>
#include <consensus/consensus.h>
#include <core_io.h>
#include <hash.h>
#include <key.h>
#include <key_io.h>
#include <primitives/transaction.h>
#include <pubkey.h>
#include <script/interpreter.h>
#include <script/script.h>
#include <script/script_error.h>
#include <span.h>
#include <streams.h>
#include <test/data/bip322_vectors.json.h>
#include <test/util/setup_common.h>
#include <univalue.h>
#include <util/strencodings.h>

#include <boost/test/unit_test.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <string>
#include <vector>

namespace {
using Witness = std::vector<std::vector<unsigned char>>;
using Result = BIP322VerificationResult;
constexpr script_verify_flags CONSENSUS_FLAGS{SCRIPT_VERIFY_P2SH | SCRIPT_VERIFY_WITNESS |
    SCRIPT_VERIFY_TAPROOT | SCRIPT_VERIFY_DERSIG | SCRIPT_VERIFY_NULLDUMMY |
    SCRIPT_VERIFY_CHECKLOCKTIMEVERIFY | SCRIPT_VERIFY_CHECKSEQUENCEVERIFY};
const std::string RULE_MESSAGE{"BIP322 rule tests"};

CKey TestKey(unsigned char value, bool compressed = true)
{
    std::array<unsigned char, 32> bytes{};
    bytes.back() = value;
    CKey key;
    key.Set(bytes.begin(), bytes.end(), compressed);
    return key;
}

std::string EncodeProof(const Witness& witness)
{
    DataStream bytes;
    bytes << witness;
    return "smp" + EncodeBase64(bytes);
}

void CheckProof(const CTxDestination& destination, const Witness& witness, Result expected)
{
    const auto actual = BIP322Verify(destination, EncodeProof(witness), RULE_MESSAGE);
    BOOST_CHECK_EQUAL(static_cast<int>(actual), static_cast<int>(expected));
}

bool OrdinaryVerify(const CTxDestination& destination, const Witness& witness, script_verify_flags flags = CONSENSUS_FLAGS)
{
    auto txs = CreateBIP322Transactions(GetScriptForDestination(destination), RULE_MESSAGE);
    txs.to_sign.vin[0].scriptWitness.stack = witness;
    const CTransaction tx{txs.to_sign};
    PrecomputedTransactionData data;
    data.Init(tx, {txs.to_spend.vout[0]}, true);
    const TransactionSignatureChecker checker{&tx, 0, 0, data, MissingDataBehavior::FAIL};
    return VerifyScript({}, txs.to_spend.vout[0].scriptPubKey, &tx.vin[0].scriptWitness, flags, checker);
}

std::vector<unsigned char> SignECDSA(const CKey& key, const CTxDestination& destination, const CScript& script, int hash_type)
{
    const auto txs = CreateBIP322Transactions(GetScriptForDestination(destination), RULE_MESSAGE);
    const auto hash = SignatureHash(script, txs.to_sign, 0, hash_type, 0, SigVersion::WITNESS_V0);
    std::vector<unsigned char> sig;
    BOOST_REQUIRE(key.Sign(hash, sig));
    sig.push_back(hash_type);
    return sig;
}

struct TapProof {
    WitnessV1Taproot destination;
    Witness witness;
    uint256 leaf;
};

TapProof MakeTapProof(const CScript& script, Witness stack = {}, int version = TAPROOT_LEAF_TAPSCRIPT, int depth = 0)
{
    const XOnlyPubKey internal{TestKey(99).GetPubKey()};
    const auto leaf = ComputeTapleafHash(version, script);
    auto root = leaf;
    std::vector<unsigned char> control{static_cast<unsigned char>(version)};
    const auto internal_bytes = ToByteVector(internal);
    control.insert(control.end(), internal_bytes.begin(), internal_bytes.end());
    for (int i{0}; i < depth; ++i) {
        const auto sibling = (HashWriter{} << i).GetSHA256();
        root = ComputeTapbranchHash(root, sibling);
        control.insert(control.end(), sibling.begin(), sibling.end());
    }
    const auto tweaked = internal.CreateTapTweak(&root);
    BOOST_REQUIRE(tweaked.has_value());
    control[0] |= tweaked->second;
    stack.emplace_back(script.begin(), script.end());
    stack.push_back(control);
    return {WitnessV1Taproot{tweaked->first}, stack, leaf};
}

std::vector<unsigned char> SignSchnorr(const CKey& key, const CTxDestination& destination, int hash_type,
                                     const uint256* leaf = nullptr, const std::vector<unsigned char>* annex = nullptr,
                                     const uint256* merkle_root = nullptr)
{
    const auto txs = CreateBIP322Transactions(GetScriptForDestination(destination), RULE_MESSAGE);
    PrecomputedTransactionData data;
    data.Init(txs.to_sign, {txs.to_spend.vout[0]}, true);
    ScriptExecutionData exec;
    exec.m_annex_init = true;
    exec.m_annex_present = annex != nullptr;
    if (annex) exec.m_annex_hash = (HashWriter{} << *annex).GetSHA256();
    exec.m_codeseparator_pos_init = true;
    exec.m_codeseparator_pos = 0xffffffff;
    if (leaf) {
        exec.m_tapleaf_hash = *leaf;
        exec.m_tapleaf_hash_init = true;
    }
    uint256 hash;
    BOOST_REQUIRE(SignatureHashSchnorr(hash, exec, txs.to_sign, 0, hash_type,
        leaf ? SigVersion::TAPSCRIPT : SigVersion::TAPROOT, data, MissingDataBehavior::FAIL));
    std::vector<unsigned char> sig(64);
    BOOST_REQUIRE(key.SignSchnorr(hash, sig, merkle_root, uint256{}));
    if (hash_type != SIGHASH_DEFAULT) sig.push_back(hash_type);
    return sig;
}
} // namespace

BOOST_FIXTURE_TEST_SUITE(bip322_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(vector_inventory)
{
    UniValue vectors;
    BOOST_REQUIRE(vectors.read(json_tests::bip322_vectors));
    BOOST_CHECK_EQUAL(vectors["provenance"]["commit"].get_str(), "a34f044bb8d5cd684d7a742d2f6483c14b719046");
    BOOST_CHECK_EQUAL(vectors["provenance"]["license"].get_str(), "CC0-1.0");
    BOOST_CHECK_EQUAL(vectors["basic"]["tx_hashes"].size(), 3);

    size_t rows{0}, signatures{0}, errors{0};
    for (const std::string source : {"basic", "generated"}) {
        const auto& data = vectors[source];
        const auto& scope = vectors["scope"][source];
        BOOST_CHECK_EQUAL(data["simple"].size(), 4);
        BOOST_CHECK_EQUAL(data["error"].size(), 8);
        for (const std::string section : {"simple", "error"}) {
            BOOST_REQUIRE_EQUAL(scope[section].size(), data[section].size());
            for (size_t i{0}; i < scope[section].size(); ++i) {
                BOOST_CHECK_EQUAL(scope[section][i].getInt<size_t>(), i);
            }
        }
        for (const auto& row : data["simple"].getValues()) {
            ++rows;
            signatures += row["bip322_signatures"].size();
            BOOST_CHECK(!row["private_keys"].empty());
        }
        errors += data["error"].size();
    }
    BOOST_CHECK_EQUAL(rows, 8);
    BOOST_CHECK_EQUAL(signatures, 10);
    BOOST_CHECK_EQUAL(errors, 16);
    const auto& counts = vectors["scope"]["counts"];
    BOOST_CHECK_EQUAL(counts["hash_rows"].getInt<size_t>(), 3);
    BOOST_CHECK_EQUAL(counts["simple_rows"].getInt<size_t>(), rows);
    BOOST_CHECK_EQUAL(counts["simple_signatures"].getInt<size_t>(), signatures);
    BOOST_CHECK_EQUAL(counts["errors"].getInt<size_t>(), errors);
    const auto& unsupported = vectors["scope"]["unsupported"]["generated"];
    BOOST_CHECK_EQUAL(unsupported["full"].size(), 10);
    BOOST_CHECK_EQUAL(unsupported["proof_of_funds"].size(), 3);
    BOOST_CHECK_EQUAL(unsupported["error"].size(), 20);
    BOOST_CHECK_EQUAL(unsupported["error"][0].getInt<int>(), 8);
    BOOST_CHECK_EQUAL(unsupported["error"][19].getInt<int>(), 27);
}

BOOST_AUTO_TEST_CASE(virtual_transactions)
{
    // The published addresses are mainnet addresses; no chain or wallet is needed.
    UniValue vectors;
    BOOST_REQUIRE(vectors.read(json_tests::bip322_vectors));
    for (const auto& row : vectors["basic"]["tx_hashes"].getValues()) {
        const auto message = row["message"].get_str();
        BOOST_TEST_CONTEXT("message: " << message) {
            const auto destination = DecodeDestination(row["address"].get_str());
            BOOST_REQUIRE(IsValidDestination(destination));
            const auto challenge = GetScriptForDestination(destination);
            const auto txs = CreateBIP322Transactions(challenge, message);
            const auto& spend = txs.to_spend;
            const auto& sign = txs.to_sign;
            const auto expected_hash = row["message_hash"].get_str();
            BOOST_CHECK_EQUAL(HexStr(BIP322MessageHash(message)), expected_hash);
            BOOST_CHECK_EQUAL(spend.GetHash().ToString(), row["to_spend_tx_hash"].get_str());
            BOOST_CHECK_EQUAL(sign.GetHash().ToString(), row["to_sign_tx_hash"].get_str());
            BOOST_CHECK_EQUAL(spend.version, 0);
            BOOST_CHECK_EQUAL(sign.version, 0);
            BOOST_CHECK_EQUAL(spend.nLockTime, 0);
            BOOST_CHECK_EQUAL(sign.nLockTime, 0);
            BOOST_REQUIRE_EQUAL(spend.vin.size(), 1);
            BOOST_REQUIRE_EQUAL(sign.vin.size(), 1);
            BOOST_REQUIRE_EQUAL(spend.vout.size(), 1);
            BOOST_REQUIRE_EQUAL(sign.vout.size(), 1);
            BOOST_CHECK(spend.vin[0].prevout.hash.IsNull());
            BOOST_CHECK_EQUAL(spend.vin[0].prevout.n, 0xffffffff);
            BOOST_CHECK(sign.vin[0].prevout.hash == spend.GetHash());
            BOOST_CHECK_EQUAL(sign.vin[0].prevout.n, 0);
            BOOST_CHECK_EQUAL(spend.vin[0].nSequence, 0);
            BOOST_CHECK_EQUAL(sign.vin[0].nSequence, 0);
            BOOST_CHECK_EQUAL(HexStr(spend.vin[0].scriptSig), "0020" + expected_hash);
            BOOST_CHECK(sign.vin[0].scriptSig.empty());
            BOOST_CHECK(spend.vin[0].scriptWitness.IsNull());
            BOOST_CHECK(sign.vin[0].scriptWitness.IsNull());
            BOOST_CHECK_EQUAL(spend.vout[0].nValue, 0);
            BOOST_CHECK_EQUAL(sign.vout[0].nValue, 0);
            BOOST_CHECK(spend.vout[0].scriptPubKey == challenge);
            BOOST_CHECK_EQUAL(HexStr(challenge), "00142b05d564e6a7a33c087f16e0f730d1440123799d");
            BOOST_CHECK_EQUAL(HexStr(sign.vout[0].scriptPubKey), "6a");

            // Wire encodings follow the BIP fields, using the external hash constants.
            const std::string expected_spend = "0000000001" + std::string(64, '0') +
                "ffffffff220020" + expected_hash + "0000000001000000000000000016" +
                "00142b05d564e6a7a33c087f16e0f730d1440123799d00000000";
            auto prevout = ParseHex(row["to_spend_tx_hash"].get_str());
            std::ranges::reverse(prevout);
            const std::string expected_sign = "0000000001" + HexStr(prevout) +
                "000000000000000000010000000000000000016a00000000";
            BOOST_CHECK_EQUAL(EncodeHexTx(CTransaction{spend}), expected_spend);
            BOOST_CHECK_EQUAL(EncodeHexTx(CTransaction{sign}), expected_sign);
        }
    }

    // The same builder preserves every supported type's exact challenge.
    for (const std::string source : {"basic", "generated"}) {
        for (const auto& row : vectors[source]["simple"].getValues()) {
            const auto destination = DecodeDestination(row["address"].get_str());
            BOOST_REQUIRE(IsValidDestination(destination));
            const auto challenge = GetScriptForDestination(destination);
            const auto txs = CreateBIP322Transactions(challenge, row["message"].get_str());
            BOOST_CHECK(txs.to_spend.vout[0].scriptPubKey == challenge);
            BOOST_CHECK(txs.to_sign.vin[0].scriptWitness.IsNull());
        }
    }
}

BOOST_AUTO_TEST_CASE(raw_message_bytes)
{
    // Independent SHA256(tag_hash || tag_hash || message) constants from Python hashlib.
    const std::string embedded_nul{"a\0b", 3};
    const std::string long_message(1'000'000, 'x');
    BOOST_CHECK_EQUAL(HexStr(BIP322MessageHash(embedded_nul)), "382b18a8adfc0411f860f79a080dbec622d5f694e1fe843046f5d6d2e25ca2af");
    BOOST_CHECK_EQUAL(HexStr(BIP322MessageHash(long_message)), "5cafa255ee2981ff53c143343b308731769c0ee50912006379bc458b4616b7cd");
    BOOST_CHECK(BIP322MessageHash(embedded_nul) != BIP322MessageHash("a"));
    BOOST_CHECK(BIP322MessageHash(embedded_nul) != MessageHash(embedded_nul));
    for (const auto& message : {embedded_nul, long_message}) {
        const auto txs = CreateBIP322Transactions(CScript() << OP_1, message);
        BOOST_CHECK_EQUAL(HexStr(txs.to_spend.vin[0].scriptSig), "0020" + HexStr(BIP322MessageHash(message)));
    }
}

BOOST_AUTO_TEST_CASE(published_simple_proofs)
{
    UniValue vectors;
    BOOST_REQUIRE(vectors.read(json_tests::bip322_vectors));
    size_t signatures{0}, errors{0};
    for (const std::string source : {"basic", "generated"}) {
        for (const auto& row : vectors[source]["simple"].getValues()) {
            for (const auto& value : row["bip322_signatures"].getValues()) {
                ++signatures;
                const auto destination = DecodeDestination(row["address"].get_str());
                const auto message = row["message"].get_str();
                const auto& proof = value.get_str();
                const auto raw = proof.starts_with("smp") ? proof.substr(3) : proof;
                BOOST_TEST_CONTEXT(source << " signature " << signatures) {
                    BOOST_CHECK(BIP322Verify(destination, proof, message) == Result::OK);
                    BOOST_CHECK(BIP322Verify(destination, raw, message) == Result::OK);
                    BOOST_CHECK(BIP322Verify(destination, "smp" + raw, message) == Result::OK);
                    BOOST_CHECK(BIP322Verify(destination, proof, message + "wrong") == Result::INVALID);
                }
            }
        }
        const std::array<Result, 8> basic_errors{Result::MALFORMED_BASE64, Result::MALFORMED_WITNESS,
            Result::INVALID, Result::INVALID, Result::INVALID, Result::INVALID,
            Result::MALFORMED_BASE64, Result::UNSUPPORTED_FORMAT};
        for (size_t i{0}; i < vectors[source]["error"].size(); ++i) {
            const auto& row = vectors[source]["error"][i];
            ++errors;
            BOOST_TEST_CONTEXT(source << ": " << row["description"].get_str()) {
                BOOST_CHECK(BIP322Verify(DecodeDestination(row["address"].get_str()),
                    row["signature"].get_str(), row["message"].get_str()) ==
                    (source == "basic" ? basic_errors[i] : Result::INVALID));
            }
        }
    }
    BOOST_CHECK_EQUAL(signatures, 10);
    BOOST_CHECK_EQUAL(errors, 16);
}

BOOST_AUTO_TEST_CASE(witness_encoding)
{
    const WitnessV0ScriptHash destination{CScript() << OP_1};
    for (const auto& proof : {"!", "smp!", "smpAA=", "smpAA===", "fooAA=="}) {
        BOOST_CHECK(BIP322Verify(destination, proof, RULE_MESSAGE) == Result::MALFORMED_BASE64);
    }
    for (const std::string hex : {"", "fd", "fd0000", "fe01000000", "ff0000000001000000",
            "ffffffffffffffffff", "01", "0101", "01fd0000", "01fd0001", "01ffffffffffffffffff", "0000"}) {
        BOOST_TEST_CONTEXT(hex) {
            const auto proof = EncodeBase64(ParseHex(hex));
            BOOST_CHECK(BIP322Verify(destination, proof, RULE_MESSAGE) == Result::MALFORMED_WITNESS);
            BOOST_CHECK(BIP322Verify(destination, "smp" + proof, RULE_MESSAGE) == Result::MALFORMED_WITNESS);
        }
    }
    for (const std::string prefix : {"ful", "pof"}) {
        BOOST_CHECK(BIP322Verify(destination, prefix + "AA==", RULE_MESSAGE) == Result::UNSUPPORTED_FORMAT);
    }
    const auto huge = "smp" + EncodeBase64(std::vector<unsigned char>(MAX_BLOCK_WEIGHT + 1));
    BOOST_CHECK(BIP322Verify(destination, huge, RULE_MESSAGE) == Result::MALFORMED_WITNESS);
    CheckProof(destination, {}, Result::INVALID);
    CheckProof(destination, Witness(1002), Result::INVALID);
    CheckProof(PKHash{TestKey(1).GetPubKey()}, {}, Result::UNSUPPORTED_ADDRESS);
    CheckProof(ScriptHash{CScript() << OP_1}, {}, Result::UNSUPPORTED_ADDRESS);
}

BOOST_AUTO_TEST_CASE(prefix_ambiguity)
{
    struct PrefixCase {
        const char* prefix;
        size_t count;
        size_t first_size;
        unsigned char first_byte;
    };
    for (const auto& test : std::array{PrefixCase{"smp", 178, 106, 0x40},
                                     PrefixCase{"ful", 126, 233, 0x40},
                                     PrefixCase{"pof", 166, 135, 0xc0}}) {
        BOOST_TEST_CONTEXT(test.prefix) {
            Witness witness(test.count - 1);
            witness[0].assign(test.first_size, 0);
            witness[0][0] = test.first_byte;
            CScript script;
            for (size_t i{0}; i < (test.count - 1) / 2; ++i) script << OP_2DROP;
            if ((test.count - 1) % 2) script << OP_DROP;
            script << OP_1;
            witness.emplace_back(script.begin(), script.end());
            const WitnessV0ScriptHash destination{script};
            const auto prefixed = EncodeProof(witness);
            const auto raw = prefixed.substr(3);
            BOOST_REQUIRE(raw.starts_with(test.prefix));
            BOOST_REQUIRE(OrdinaryVerify(destination, witness));
            BOOST_CHECK(BIP322Verify(destination, raw, RULE_MESSAGE) == Result::OK);
            BOOST_CHECK(BIP322Verify(destination, prefixed, RULE_MESSAGE) == Result::OK);
            BOOST_CHECK(BIP322Verify(destination, "smp" + prefixed, RULE_MESSAGE) == Result::MALFORMED_BASE64);
            BOOST_CHECK(BIP322Verify(destination, "ful" + raw, RULE_MESSAGE) == Result::UNSUPPORTED_FORMAT);
            BOOST_CHECK(BIP322Verify(destination, "pof" + raw, RULE_MESSAGE) == Result::UNSUPPORTED_FORMAT);
        }
    }
}

BOOST_AUTO_TEST_CASE(ecdsa_sighash_rules)
{
    const auto key = TestKey(1);
    const WitnessV0KeyHash destination{key.GetPubKey()};
    const auto script = GetScriptForDestination(PKHash{key.GetPubKey()});
    for (int type : std::array<int, 6>{SIGHASH_ALL, SIGHASH_NONE, SIGHASH_SINGLE, SIGHASH_ALL | SIGHASH_ANYONECANPAY,
                    SIGHASH_NONE | SIGHASH_ANYONECANPAY, SIGHASH_SINGLE | SIGHASH_ANYONECANPAY}) {
        const Witness witness{SignECDSA(key, destination, script, type), ToByteVector(key.GetPubKey())};
        BOOST_REQUIRE(OrdinaryVerify(destination, witness));
        CheckProof(destination, witness, type == SIGHASH_ALL ? Result::OK : Result::INVALID);
    }
}

BOOST_AUTO_TEST_CASE(schnorr_sighash_rules)
{
    const auto key = TestKey(2);
    const WitnessV1Taproot destination{XOnlyPubKey{key.GetPubKey()}};
    for (int type : std::array<int, 7>{SIGHASH_DEFAULT, SIGHASH_ALL, SIGHASH_NONE, SIGHASH_SINGLE,
                    SIGHASH_ALL | SIGHASH_ANYONECANPAY, SIGHASH_NONE | SIGHASH_ANYONECANPAY,
                    SIGHASH_SINGLE | SIGHASH_ANYONECANPAY}) {
        Witness witness{SignSchnorr(key, destination, type)};
        BOOST_REQUIRE(OrdinaryVerify(destination, witness));
        CheckProof(destination, witness, type == SIGHASH_DEFAULT || type == SIGHASH_ALL ? Result::OK : Result::INVALID);
        if (type == SIGHASH_DEFAULT) {
            witness[0].push_back(0);
            BOOST_CHECK(!OrdinaryVerify(destination, witness));
            CheckProof(destination, witness, Result::INVALID);
        }
    }
}

BOOST_AUTO_TEST_CASE(ecdsa_encoding_rules)
{
    const auto key = TestKey(3);
    const WitnessV0KeyHash destination{key.GetPubKey()};
    auto sig = SignECDSA(key, destination, GetScriptForDestination(PKHash{key.GetPubKey()}), SIGHASH_ALL);
    CheckProof(destination, {sig, ToByteVector(key.GetPubKey())}, Result::OK);
    const size_t rlen = sig[3];
    const size_t slen = sig[5 + rlen];
    std::vector<unsigned char> s(32);
    std::copy(sig.begin() + 6 + rlen, sig.end() - 1, s.end() - slen);
    std::ranges::reverse(s);
    const auto order = uint256::FromHex("fffffffffffffffffffffffffffffffebaaedce6af48a03bbfd25e8cd0364141").value();
    auto high = ToByteVector(ArithToUint256(UintToArith256(order) - UintToArith256(uint256{s})));
    std::ranges::reverse(high);
    if (high[0] & 0x80) high.insert(high.begin(), 0);
    auto high_sig = sig;
    high_sig.resize(6 + rlen);
    high_sig[5 + rlen] = high.size();
    high_sig.insert(high_sig.end(), high.begin(), high.end());
    high_sig.push_back(SIGHASH_ALL);
    high_sig[1] = high_sig.size() - 3;
    const Witness high_witness{high_sig, ToByteVector(key.GetPubKey())};
    BOOST_REQUIRE(OrdinaryVerify(destination, high_witness));
    ScriptError error;
    BOOST_CHECK(BIP322Verify(destination, EncodeProof(high_witness), RULE_MESSAGE, &error) == Result::INVALID);
    BOOST_CHECK(error == SCRIPT_ERR_SIG_HIGH_S);
    sig.insert(sig.begin() + 4, 0);
    ++sig[1];
    ++sig[3];
    CheckProof(destination, {sig, ToByteVector(key.GetPubKey())}, Result::INVALID);
}

BOOST_AUTO_TEST_CASE(required_stack_rules)
{
    auto check = [](const CScript& script, Witness stack, Result expected, bool ordinary = true) {
        const WitnessV0ScriptHash destination{script};
        stack.emplace_back(script.begin(), script.end());
        if (ordinary) BOOST_REQUIRE(OrdinaryVerify(destination, stack));
        CheckProof(destination, stack, expected);
    };
    const auto nonminimal_bytes = ParseHex("4c01117551");
    const CScript nonminimal{nonminimal_bytes.begin(), nonminimal_bytes.end()};
    check(nonminimal, {}, Result::INVALID);
    check(CScript() << OP_IF << OP_1 << OP_ELSE << OP_1 << OP_ENDIF, {{2}}, Result::INVALID);
    check(CScript() << OP_1ADD << OP_DROP << OP_1, {{1, 0}}, Result::INVALID);
    check(CScript() << OP_1 << OP_1, {}, Result::INVALID, false);
    check(CScript() << OP_DROP << OP_1, {std::vector<unsigned char>(521)}, Result::INVALID, false);
    const auto key = TestKey(4);
    const auto script = CScript() << ToByteVector(key.GetPubKey()) << OP_CHECKSIG << OP_NOT;
    check(script, {{}}, Result::OK); // Empty signatures may contribute to a complete satisfaction.
    auto sig = SignECDSA(key, WitnessV0ScriptHash{script}, script, SIGHASH_ALL);
    sig[10] ^= 1;
    check(script, {sig}, Result::INVALID); // NULLFAIL despite the script negating CHECKSIG.
    const auto multisig = CScript() << OP_1 << ToByteVector(key.GetPubKey()) << OP_1 << OP_CHECKMULTISIG;
    const auto valid = SignECDSA(key, WitnessV0ScriptHash{multisig}, multisig, SIGHASH_ALL);
    check(multisig, {{}, valid}, Result::OK);
    check(multisig, {{1}, valid}, Result::INVALID, false); // Consensus NULLDUMMY.
    check(CScript() << std::vector<unsigned char>(32, 0) << OP_CHECKSIG << OP_NOT, {{}}, Result::INVALID); // STRICTENC.
    check(CScript() << OP_0 << OP_CHECKLOCKTIMEVERIFY << OP_DROP << OP_1, {}, Result::OK);
    check(CScript() << OP_1 << OP_CHECKLOCKTIMEVERIFY << OP_DROP << OP_1, {}, Result::INVALID, false);
    check(CScript() << OP_0 << OP_CHECKSEQUENCEVERIFY << OP_DROP << OP_1, {}, Result::INVALID, false); // Simple version is zero.
}

BOOST_AUTO_TEST_CASE(signature_data_and_policy_independence)
{
    // Witness v0 does not FindAndDelete signatures from scriptCode. A signature
    // pushed as script data is hashed along with the rest of the committed script.
    const auto key = TestKey(8);
    const auto base_script = CScript() << ToByteVector(key.GetPubKey()) << OP_CHECKSIG;
    const auto data = SignECDSA(key, WitnessV0ScriptHash{base_script}, base_script, SIGHASH_ALL);
    const auto script = CScript() << data << OP_DROP << ToByteVector(key.GetPubKey()) << OP_CHECKSIG;
    const WitnessV0ScriptHash destination{script};
    const Witness proof{SignECDSA(key, destination, script, SIGHASH_ALL), {script.begin(), script.end()}};
    BOOST_REQUIRE(OrdinaryVerify(destination, proof));
    CheckProof(destination, proof, Result::OK);
    // Compressed-only witness keys are a relay policy, not a BIP322 required rule.
    const auto uncompressed = TestKey(8, false);
    const WitnessV0KeyHash wpkh{uncompressed.GetPubKey()};
    const auto pkh = GetScriptForDestination(PKHash{uncompressed.GetPubKey()});
    const Witness uncompressed_proof{SignECDSA(uncompressed, wpkh, pkh, SIGHASH_ALL), ToByteVector(uncompressed.GetPubKey())};
    BOOST_REQUIRE(OrdinaryVerify(wpkh, uncompressed_proof));
    CheckProof(wpkh, uncompressed_proof, Result::OK);
    BOOST_CHECK(!OrdinaryVerify(wpkh, uncompressed_proof, CONSENSUS_FLAGS | SCRIPT_VERIFY_WITNESS_PUBKEYTYPE));
}

BOOST_AUTO_TEST_CASE(address_and_script_binding)
{
    UniValue vectors;
    BOOST_REQUIRE(vectors.read(json_tests::bip322_vectors));
    const auto& rows = vectors["generated"]["simple"];
    for (size_t i{0}; i < rows.size(); ++i) {
        const auto& row = rows[i];
        const auto wrong = DecodeDestination(rows[(i + 1) % rows.size()]["address"].get_str());
        BOOST_CHECK(BIP322Verify(wrong, row["bip322_signatures"][0].get_str(), row["message"].get_str()) == Result::INVALID);
    }
    const CScript script = CScript() << OP_1;
    CheckProof(WitnessV0ScriptHash{CScript() << OP_2}, {{script.begin(), script.end()}}, Result::INVALID);
    auto tap = MakeTapProof(script);
    CheckProof(tap.destination, tap.witness, Result::OK);
    tap.witness.back()[0] ^= 1; // Control-block parity must bind to the output.
    CheckProof(tap.destination, tap.witness, Result::INVALID);
    tap.witness.back()[0] ^= 1;
    tap.witness.back()[1] ^= 1; // A different internal key is not the claimed address.
    CheckProof(tap.destination, tap.witness, Result::INVALID);
}

BOOST_AUTO_TEST_CASE(taproot_script_paths)
{
    const auto first = TestKey(5), second = TestKey(6);
    const auto p1 = ToByteVector(XOnlyPubKey{first.GetPubKey()});
    const auto p2 = ToByteVector(XOnlyPubKey{second.GetPubKey()});
    const auto script = CScript() << p1 << OP_CHECKSIG;
    auto tap = MakeTapProof(script, {{}});
    tap.witness[0] = SignSchnorr(first, tap.destination, SIGHASH_DEFAULT, &tap.leaf);
    BOOST_REQUIRE(OrdinaryVerify(tap.destination, tap.witness));
    CheckProof(tap.destination, tap.witness, Result::OK);
    tap.witness[0].push_back(0);
    CheckProof(tap.destination, tap.witness, Result::INVALID); // Explicit DEFAULT is forbidden in script paths too.
    tap.witness[0] = SignSchnorr(first, tap.destination, SIGHASH_NONE, &tap.leaf);
    BOOST_REQUIRE(OrdinaryVerify(tap.destination, tap.witness));
    CheckProof(tap.destination, tap.witness, Result::INVALID);
    const auto add_script = CScript() << p1 << OP_CHECKSIG << p2 << OP_CHECKSIGADD << OP_2 << OP_NUMEQUAL;
    tap = MakeTapProof(add_script, {{}, {}});
    tap.witness[0] = SignSchnorr(second, tap.destination, SIGHASH_ALL, &tap.leaf);
    tap.witness[1] = SignSchnorr(first, tap.destination, SIGHASH_DEFAULT, &tap.leaf);
    BOOST_REQUIRE(OrdinaryVerify(tap.destination, tap.witness));
    CheckProof(tap.destination, tap.witness, Result::OK);
    tap.witness[0].clear();
    CheckProof(tap.destination, tap.witness, Result::INVALID);
    tap = MakeTapProof(CScript() << p1 << OP_CHECKSIG << p2 << OP_CHECKSIGADD << OP_1 << OP_NUMEQUAL, {{}, {}});
    tap.witness[1] = SignSchnorr(first, tap.destination, SIGHASH_DEFAULT, &tap.leaf);
    CheckProof(tap.destination, tap.witness, Result::OK); // Empty CHECKSIGADD contribution.
    tap = MakeTapProof(CScript() << OP_1);
    const Witness key_path{SignSchnorr(TestKey(99), tap.destination, SIGHASH_ALL, nullptr, nullptr, &tap.leaf)};
    BOOST_REQUIRE(OrdinaryVerify(tap.destination, key_path));
    CheckProof(tap.destination, key_path, Result::OK); // Key path with a committed tree.
}

BOOST_AUTO_TEST_CASE(code_separator_opcodes)
{
    for (const auto& script : {CScript() << OP_CODESEPARATOR << OP_1,
                             CScript() << OP_0 << OP_IF << OP_CODESEPARATOR << OP_ENDIF << OP_1}) {
        const Witness witness{{script.begin(), script.end()}};
        BOOST_REQUIRE(OrdinaryVerify(WitnessV0ScriptHash{script}, witness));
        CheckProof(WitnessV0ScriptHash{script}, witness, Result::INVALID);
        const auto tap = MakeTapProof(script);
        BOOST_REQUIRE(OrdinaryVerify(tap.destination, tap.witness));
        CheckProof(tap.destination, tap.witness, Result::INVALID);
    }
    const auto pushed = CScript() << std::vector<unsigned char>{OP_CODESEPARATOR} << OP_DROP << OP_1;
    CheckProof(WitnessV0ScriptHash{pushed}, {{pushed.begin(), pushed.end()}}, Result::OK);
    const auto tap = MakeTapProof(pushed);
    CheckProof(tap.destination, tap.witness, Result::OK);
}

BOOST_AUTO_TEST_CASE(inactive_pushes_and_nops)
{
    // MINIMALDATA and upgrade NOP flags apply to executed operations. The
    // separate BIP322 CODESEPARATOR prohibition applies to the entire script.
    const auto raw = ParseHex("00634c0111b06851"); // False IF, nonminimal push, NOP1, ENDIF, true.
    const CScript script{raw.begin(), raw.end()};
    const Witness witness{{script.begin(), script.end()}};
    BOOST_REQUIRE(OrdinaryVerify(WitnessV0ScriptHash{script}, witness));
    CheckProof(WitnessV0ScriptHash{script}, witness, Result::OK);
    const auto tap = MakeTapProof(script);
    BOOST_REQUIRE(OrdinaryVerify(tap.destination, tap.witness));
    CheckProof(tap.destination, tap.witness, Result::OK);
}

BOOST_AUTO_TEST_CASE(upgradeable_rules_and_precedence)
{
    for (const auto opcode : {OP_NOP1, OP_NOP4, OP_NOP5, OP_NOP6, OP_NOP7, OP_NOP8, OP_NOP9, OP_NOP10}) {
        for (const auto final : {OP_1, OP_0}) {
            const auto script = CScript() << opcode << final;
            const Witness witness{{script.begin(), script.end()}};
            CheckProof(WitnessV0ScriptHash{script}, witness, final == OP_1 ? Result::INCONCLUSIVE : Result::INVALID);
            const auto tap = MakeTapProof(script);
            CheckProof(tap.destination, tap.witness, final == OP_1 ? Result::INCONCLUSIVE : Result::INVALID);
        }
    }
    auto tap = MakeTapProof(CScript() << OP_1, {}, 0xc2);
    CheckProof(tap.destination, tap.witness, Result::INCONCLUSIVE);
    tap.witness.back()[1] ^= 1;
    CheckProof(tap.destination, tap.witness, Result::INVALID); // Bind before classifying an unknown leaf.
    tap = MakeTapProof(CScript() << OP_RESERVED);
    CheckProof(tap.destination, tap.witness, Result::INCONCLUSIVE); // OP_SUCCESS80.
    tap = MakeTapProof(CScript() << OP_0 << OP_IF << OP_RESERVED << OP_ENDIF << OP_0);
    CheckProof(tap.destination, tap.witness, Result::INCONCLUSIVE); // OP_SUCCESS also applies in inactive branches.
    tap = MakeTapProof(CScript() << OP_RESERVED << OP_CODESEPARATOR);
    CheckProof(tap.destination, tap.witness, Result::INVALID); // Required separator prohibition survives OP_SUCCESS.
    tap = MakeTapProof(CScript() << OP_CODESEPARATOR, {}, 0xc2);
    CheckProof(tap.destination, tap.witness, Result::INCONCLUSIVE); // Unknown leaf semantics are opaque.
    tap = MakeTapProof(CScript() << OP_RESERVED, Witness(MAX_STACK_SIZE + 1, std::vector<unsigned char>(521)));
    BOOST_REQUIRE(OrdinaryVerify(tap.destination, tap.witness));
    CheckProof(tap.destination, tap.witness, Result::INCONCLUSIVE); // OP_SUCCESS overrides consensus stack limits.
    const auto unknown_key = std::vector<unsigned char>(33, 2);
    tap = MakeTapProof(CScript() << unknown_key << OP_CHECKSIG, {{42}});
    CheckProof(tap.destination, tap.witness, Result::INCONCLUSIVE);
    tap = MakeTapProof(CScript() << unknown_key << OP_CHECKSIG << OP_DROP << OP_0, {{42}});
    CheckProof(tap.destination, tap.witness, Result::INVALID); // Required failure after unknown key.
    const auto nonminimal = ParseHex("b04c01117551"); // NOP1, nonminimal push, DROP, true.
    const CScript nonminimal_script{nonminimal.begin(), nonminimal.end()};
    const Witness nonminimal_proof{{nonminimal_script.begin(), nonminimal_script.end()}};
    BOOST_REQUIRE(OrdinaryVerify(WitnessV0ScriptHash{nonminimal_script}, nonminimal_proof));
    CheckProof(WitnessV0ScriptHash{nonminimal_script}, nonminimal_proof, Result::INVALID);
    tap = MakeTapProof(nonminimal_script);
    CheckProof(tap.destination, tap.witness, Result::INVALID); // Required MINIMALDATA before upgrade classification.
    const auto ecdsa_key = TestKey(9);
    const auto sighash_script = CScript() << OP_NOP1 << ToByteVector(ecdsa_key.GetPubKey()) << OP_CHECKSIG;
    const WitnessV0ScriptHash sighash_destination{sighash_script};
    const Witness sighash_proof{SignECDSA(ecdsa_key, sighash_destination, sighash_script, SIGHASH_NONE),
        {sighash_script.begin(), sighash_script.end()}};
    BOOST_REQUIRE(OrdinaryVerify(sighash_destination, sighash_proof));
    CheckProof(sighash_destination, sighash_proof, Result::INVALID);
    for (const auto final : {OP_1, OP_0}) {
        tap = MakeTapProof(CScript() << final);
        tap.witness.push_back({ANNEX_TAG, 1});
        CheckProof(tap.destination, tap.witness, final == OP_1 ? Result::INCONCLUSIVE : Result::INVALID);
    }
    const auto key = TestKey(7);
    const WitnessV1Taproot dest{XOnlyPubKey{key.GetPubKey()}};
    const std::vector<unsigned char> annex{ANNEX_TAG, 9};
    const auto sig = SignSchnorr(key, dest, SIGHASH_ALL, nullptr, &annex);
    BOOST_REQUIRE(OrdinaryVerify(dest, {sig, annex}));
    CheckProof(dest, {sig, annex}, Result::INCONCLUSIVE);
    auto bad = sig;
    bad[0] ^= 1;
    CheckProof(dest, {bad, annex}, Result::INVALID);
    for (int version{2}; version <= 16; ++version) {
        CheckProof(WitnessUnknown{version, std::vector<unsigned char>(32, 3)}, {}, Result::INCONCLUSIVE);
    }
    CheckProof(PayToAnchor{}, {}, Result::UNSUPPORTED_ADDRESS);
}

BOOST_AUTO_TEST_CASE(large_consensus_witnesses)
{
    CScript script;
    while (script.size() + 525 <= MAX_SCRIPT_SIZE) {
        script << std::vector<unsigned char>(520, 1) << OP_DROP;
    }
    while (script.size() + 1 < MAX_SCRIPT_SIZE) script << OP_NOP;
    script << OP_1;
    BOOST_REQUIRE_EQUAL(script.size(), MAX_SCRIPT_SIZE);
    CheckProof(WitnessV0ScriptHash{script}, {{script.begin(), script.end()}}, Result::OK);
    script << OP_NOP;
    CheckProof(WitnessV0ScriptHash{script}, {{script.begin(), script.end()}}, Result::INVALID);
    CScript large_tapscript;
    for (int i{0}; i < 10'000; ++i) large_tapscript << OP_0 << OP_DROP;
    large_tapscript << OP_1;
    const auto tap = MakeTapProof(large_tapscript, {}, TAPROOT_LEAF_TAPSCRIPT, TAPROOT_CONTROL_MAX_NODE_COUNT);
    BOOST_REQUIRE_EQUAL(tap.witness.back().size(), TAPROOT_CONTROL_MAX_SIZE);
    BOOST_REQUIRE(OrdinaryVerify(tap.destination, tap.witness));
    CheckProof(tap.destination, tap.witness, Result::OK);
    auto too_large = tap;
    too_large.witness.back().push_back(0);
    CheckProof(too_large.destination, too_large.witness, Result::INVALID);
    auto excess_stack = MakeTapProof(CScript() << OP_1, Witness(MAX_STACK_SIZE + 1));
    CheckProof(excess_stack.destination, excess_stack.witness, Result::INVALID);
}

BOOST_AUTO_TEST_SUITE_END()
