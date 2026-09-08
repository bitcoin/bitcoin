// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <addresstype.h>
#include <key.h>
#include <key_io.h>
#include <primitives/transaction.h>
#include <pubkey.h>
#include <script/interpreter.h>
#include <script/script.h>
#include <script/script_error.h>
#include <streams.h>
#include <test/data/cisa_consensus_vectors.json.h>
#include <test/data/cisa_wallet_vectors.json.h>
#include <test/util/setup_common.h>
#include <uint256.h>
#include <univalue.h>
#include <util/check.h>
#include <util/strencodings.h>

#include <boost/test/unit_test.hpp>

#include <string>
#include <vector>

BOOST_FIXTURE_TEST_SUITE(cisa_tests, BasicTestingSetup)

static constexpr script_verify_flags CISA_TEST_FLAGS{SCRIPT_VERIFY_P2SH | SCRIPT_VERIFY_WITNESS | SCRIPT_VERIFY_TAPROOT | SCRIPT_VERIFY_WITNESS_V2};

//! Parse a marker given as "0xbc" style string.
static uint8_t ParseMarker(const std::string& str)
{
    BOOST_REQUIRE_EQUAL(str.substr(0, 2), "0x");
    const auto bytes{ParseHex(str.substr(2))};
    BOOST_REQUIRE_EQUAL(bytes.size(), 1);
    return bytes[0];
}

//! Validate a transaction the way consensus does for witness v2: per-input
//! script checks followed by the transaction level aggregate verification.
static bool ValidateCISATransaction(const CTransaction& tx, const std::vector<CTxOut>& spent_outputs, PrecomputedTransactionData& txdata)
{
    for (unsigned int i = 0; i < tx.vin.size(); i++) {
        TransactionSignatureChecker checker(&tx, i, spent_outputs[i].nValue, txdata, MissingDataBehavior::FAIL);
        if (!VerifyScript(tx.vin[i].scriptSig, spent_outputs[i].scriptPubKey, &tx.vin[i].scriptWitness, CISA_TEST_FLAGS, checker)) {
            return false;
        }
    }
    return VerifyCISATransaction(tx, spent_outputs, CISA_TEST_FLAGS, txdata);
}

BOOST_AUTO_TEST_CASE(cisa_scriptpubkey_vectors)
{
    UniValue tests;
    Assert(tests.read(json_tests::cisa_wallet_vectors));

    for (const auto& vec : tests["scriptPubKey"].getValues()) {
        const XOnlyPubKey internal_pubkey{ParseHex(vec["given"]["internalPubkey"].get_str())};

        const uint256 tweak{internal_pubkey.ComputeTapTweakHash(/*merkle_root=*/nullptr)};
        BOOST_CHECK_EQUAL(HexStr(tweak), vec["intermediary"]["tweak"].get_str());

        const auto tweaked{internal_pubkey.CreateTapTweak(/*merkle_root=*/nullptr)};
        BOOST_REQUIRE(tweaked.has_value());
        const XOnlyPubKey output_pubkey{tweaked->first};
        BOOST_CHECK_EQUAL(HexStr(output_pubkey), vec["intermediary"]["tweakedPubkey"].get_str());

        const CScript script_pubkey{CScript() << OP_2 << ToByteVector(output_pubkey)};
        BOOST_CHECK_EQUAL(HexStr(script_pubkey), vec["expected"]["scriptPubKey"].get_str());

        // The bech32m address must encode and decode to the same output.
        const std::string address{vec["expected"]["address"].get_str()};
        BOOST_CHECK_EQUAL(EncodeDestination(WitnessV2Cisa{output_pubkey}), address);
        const CTxDestination decoded{DecodeDestination(address)};
        BOOST_CHECK(std::get<WitnessV2Cisa>(decoded) == WitnessV2Cisa{output_pubkey});
        BOOST_CHECK(GetScriptForDestination(decoded) == script_pubkey);
    }
}

BOOST_AUTO_TEST_CASE(cisa_witness_structure_table)
{
    // Check every element size up to 300 bytes against the BIP460 structure
    // table, with the last byte set to each marker and to a valid sighash type.
    for (size_t size = 0; size <= 300; size++) {
        for (const uint8_t last : {CISA_MARKER_HALFAGG, CISA_MARKER_FULLAGG, uint8_t{0x01}}) {
            std::vector<unsigned char> elem(size);
            if (size >= 2) elem[size - 2] = 0x01;
            if (size >= 1) elem.back() = last;
            const bool is_marker{last == CISA_MARKER_HALFAGG || last == CISA_MARKER_FULLAGG};
            ScriptError serror;
            const auto parsed{ParseCISAWitness(elem, &serror)};

            if (size == 64 || (size == 65 && !is_marker)) {
                BOOST_REQUIRE(parsed.has_value());
                BOOST_CHECK(!parsed->marker);
                BOOST_CHECK_EQUAL(parsed->signature.size(), size);
            } else if (size == 0 || size == 1 || size == 32 || size == 33) {
                const bool has_sighash{size % 2 == 1};
                if (has_sighash && is_marker) {
                    BOOST_CHECK(!parsed);
                    BOOST_CHECK_EQUAL(serror, SCRIPT_ERR_WITNESS_V2_INVALID_SIGHASH);
                } else {
                    BOOST_REQUIRE(parsed.has_value());
                    BOOST_CHECK(parsed->marker == (size < 32 ? CISA_MARKER_FULLAGG : CISA_MARKER_HALFAGG));
                    BOOST_CHECK(!parsed->is_final);
                    BOOST_CHECK_EQUAL(parsed->sighash_type, has_sighash ? 0x01 : SIGHASH_DEFAULT);
                    BOOST_CHECK_EQUAL(parsed->signature.size(), size - has_sighash);
                }
            } else if ((size == 65 || size == 66) && is_marker) {
                BOOST_REQUIRE(parsed.has_value());
                BOOST_CHECK(parsed->marker == last);
                BOOST_CHECK(parsed->is_final);
                BOOST_CHECK_EQUAL(parsed->sighash_type, size == 66 ? 0x01 : SIGHASH_DEFAULT);
                BOOST_CHECK_EQUAL(parsed->signature.size(), 64);
            } else {
                BOOST_CHECK(!parsed);
                BOOST_CHECK_EQUAL(serror, SCRIPT_ERR_WITNESS_V2_INVALID_MARKER);
            }
        }
    }

    for (unsigned int b = 0x00; b <= 0xff; b++) {
        const bool is_marker{b == CISA_MARKER_HALFAGG || b == CISA_MARKER_FULLAGG};
        std::vector<unsigned char> elem(65);
        elem.back() = b;
        const auto parsed{ParseCISAWitness(elem)};
        BOOST_REQUIRE(parsed.has_value());
        BOOST_CHECK_EQUAL(parsed->marker.has_value(), is_marker);
        BOOST_CHECK_EQUAL(parsed->is_final, is_marker);
    }

    // An explicit sighash byte must be one of the six valid non-zero types.
    // SIGHASH_DEFAULT can only be expressed by omitting the sighash byte.
    for (unsigned int b = 0x00; b <= 0xff; b++) {
        ScriptError serror;
        const auto parsed{ParseCISAWitness(std::vector<unsigned char>{static_cast<unsigned char>(b)}, &serror)};
        const bool is_valid_sighash{(b >= 0x01 && b <= 0x03) || (b >= 0x81 && b <= 0x83)};
        BOOST_CHECK_EQUAL(parsed.has_value(), is_valid_sighash);
        if (parsed) {
            BOOST_CHECK_EQUAL(parsed->sighash_type, b);
        } else {
            BOOST_CHECK_EQUAL(serror, SCRIPT_ERR_WITNESS_V2_INVALID_SIGHASH);
        }
    }

    // Parsed fields point at the right pieces of the element.
    std::vector<unsigned char> elem(66);
    elem[64] = 0x83;
    elem[65] = CISA_MARKER_HALFAGG;
    const auto parsed{ParseCISAWitness(elem)};
    BOOST_REQUIRE(parsed.has_value());
    BOOST_CHECK(parsed->marker == CISA_MARKER_HALFAGG);
    BOOST_CHECK(parsed->is_final);
    BOOST_CHECK_EQUAL(parsed->sighash_type, 0x83);
    BOOST_CHECK_EQUAL(parsed->signature.size(), 64);
    BOOST_CHECK(parsed->signature.data() == elem.data());

    std::vector<unsigned char> member(33);
    member[32] = 0x02;
    const auto parsed_member{ParseCISAWitness(member)};
    BOOST_REQUIRE(parsed_member.has_value());
    BOOST_CHECK(parsed_member->marker == CISA_MARKER_HALFAGG);
    BOOST_CHECK(!parsed_member->is_final);
    BOOST_CHECK_EQUAL(parsed_member->sighash_type, 0x02);
    BOOST_CHECK_EQUAL(parsed_member->signature.size(), 32);

    // Opted-out elements are returned whole as the signature.
    std::vector<unsigned char> optout(65, 0x01);
    const auto parsed_optout{ParseCISAWitness(optout)};
    BOOST_REQUIRE(parsed_optout.has_value());
    BOOST_CHECK(!parsed_optout->marker);
    BOOST_CHECK_EQUAL(parsed_optout->signature.size(), 65);
}

BOOST_AUTO_TEST_CASE(cisa_halfagg_single_verify)
{
    // BIP458 fixes the first aggregation coefficient to 1, so a half-aggregate
    // signature over a single signature is the signature itself and the
    // wrapper must agree with plain BIP340 verification.
    CKey key;
    key.MakeNewKey(true);
    const XOnlyPubKey pubkey{key.GetPubKey()};
    const uint256 msg{m_rng.rand256()};
    std::vector<unsigned char> sig(64);
    BOOST_REQUIRE(key.SignSchnorr(msg, sig, /*merkle_root=*/nullptr, uint256{}));

    BOOST_CHECK(VerifyHalfAggSchnorr({pubkey}, {msg}, sig));
    BOOST_CHECK(!VerifyHalfAggSchnorr({pubkey}, {m_rng.rand256()}, sig));
    sig[10] ^= 0x01;
    BOOST_CHECK(!VerifyHalfAggSchnorr({pubkey}, {msg}, sig));

    // Size mismatches and empty inputs are rejected before they can trip the
    // argument checks in secp256k1.
    BOOST_CHECK(!VerifyHalfAggSchnorr({pubkey}, {msg}, std::vector<unsigned char>(96)));
    BOOST_CHECK(!VerifyHalfAggSchnorr({pubkey}, {msg}, {}));
    BOOST_CHECK(!VerifyHalfAggSchnorr({}, {}, {}));
}

BOOST_AUTO_TEST_CASE(cisa_wallet_keypath_vectors)
{
    UniValue tests;
    Assert(tests.read(json_tests::cisa_wallet_vectors));

    for (const auto& vec : tests["keyPathSpending"].getValues()) {
        auto txhex = ParseHex(vec["given"]["rawUnsignedTx"].get_str());
        CMutableTransaction tx;
        SpanReader{txhex} >> TX_WITH_WITNESS(tx);
        std::vector<CTxOut> utxos;
        for (const auto& utxo_spent : vec["given"]["utxosSpent"].getValues()) {
            auto script_bytes = ParseHex(utxo_spent["scriptPubKey"].get_str());
            CScript script{script_bytes.begin(), script_bytes.end()};
            CAmount amount{utxo_spent["amountSats"].getInt<int>()};
            utxos.emplace_back(amount, script);
        }

        PrecomputedTransactionData txdata;
        txdata.Init(tx, std::vector<CTxOut>{utxos}, /*force=*/true);
        BOOST_CHECK(txdata.m_bip341_taproot_ready);

        for (const auto& input : vec["inputSpending"].getValues()) {
            int txinpos = input["given"]["txinIndex"].getInt<int>();
            const uint8_t hashtype(input["given"]["hashType"].getInt<int>());
            const bool opt_out{input["given"]["aggMode"].isNull()};

            // Load key and check the intermediary key derivation values.
            auto privkey = ParseHex(input["given"]["internalPrivkey"].get_str());
            CKey key;
            key.Set(privkey.begin(), privkey.end(), true);
            const XOnlyPubKey pubkey{key.GetPubKey()};
            BOOST_CHECK_EQUAL(HexStr(pubkey), input["intermediary"]["internalPubkey"].get_str());
            BOOST_CHECK_EQUAL(HexStr(pubkey.ComputeTapTweakHash(nullptr)), input["intermediary"]["tweak"].get_str());

            // The tweaked private key must correspond to the spent witness program.
            auto tweaked_privkey = ParseHex(input["intermediary"]["tweakedPrivkey"].get_str());
            CKey tweaked_key;
            tweaked_key.Set(tweaked_privkey.begin(), tweaked_privkey.end(), true);
            int witver;
            std::vector<unsigned char> witprog;
            BOOST_REQUIRE(utxos[txinpos].scriptPubKey.IsWitnessProgram(witver, witprog));
            BOOST_CHECK_EQUAL(witver, 2);
            BOOST_CHECK_EQUAL(HexStr(XOnlyPubKey{tweaked_key.GetPubKey()}), HexStr(witprog));

            // Compute and check the signature message.
            ScriptExecutionData execdata;
            execdata.m_annex_init = true;
            execdata.m_annex_present = false;
            uint256 sighash;
            if (opt_out) {
                BOOST_REQUIRE(SignatureHashSchnorr(sighash, execdata, tx, txinpos, hashtype, SigVersion::TAPROOT, txdata, MissingDataBehavior::FAIL));
            } else {
                execdata.m_cisa_agg_mode = ParseMarker(input["given"]["aggMode"].get_str());
                BOOST_REQUIRE(SignatureHashSchnorr(sighash, execdata, tx, txinpos, hashtype, SigVersion::WITNESS_V2_KEYPATH, txdata, MissingDataBehavior::FAIL));
            }
            BOOST_CHECK_EQUAL(HexStr(sighash), input["intermediary"]["sigHash"].get_str());

            // Signatures are created with all-zero aux randomness in the vectors.
            const uint256 merkle_root; // empty: keypath tweak without script tree
            std::vector<unsigned char> sig(64);
            BOOST_REQUIRE(key.SignSchnorr(sighash, sig, &merkle_root, uint256{}));
            const std::string expected_witness{input["expected"]["witness"][0].get_str()};
            if (opt_out) {
                if (hashtype != SIGHASH_DEFAULT) sig.push_back(hashtype);
                BOOST_CHECK_EQUAL(HexStr(sig), expected_witness);
            } else if (execdata.m_cisa_agg_mode == CISA_MARKER_HALFAGG) {
                // The element starts with the nonce share of the input's own signature
                BOOST_CHECK_EQUAL(HexStr(std::span{sig}.first(32)), expected_witness.substr(0, 64));
            }
        }

        // The fully signed transaction must round-trip and validate.
        auto signed_txhex = ParseHex(vec["expected"]["rawSignedTx"].get_str());
        CMutableTransaction signed_tx;
        SpanReader{signed_txhex} >> TX_WITH_WITNESS(signed_tx);
        const CTransaction final_tx{signed_tx};
        PrecomputedTransactionData signed_txdata;
        signed_txdata.Init(final_tx, std::vector<CTxOut>{utxos}, /*force=*/true);
        BOOST_CHECK(ValidateCISATransaction(final_tx, utxos, signed_txdata));
    }
}

BOOST_AUTO_TEST_CASE(cisa_wallet_scriptpath_vectors)
{
    UniValue tests;
    Assert(tests.read(json_tests::cisa_wallet_vectors));

    for (const auto& vec : tests["scriptPathSpending"].getValues()) {
        auto txhex = ParseHex(vec["given"]["rawUnsignedTx"].get_str());
        CMutableTransaction tx;
        SpanReader{txhex} >> TX_WITH_WITNESS(tx);
        std::vector<CTxOut> utxos;
        for (const auto& utxo_spent : vec["given"]["utxosSpent"].getValues()) {
            auto script_bytes = ParseHex(utxo_spent["scriptPubKey"].get_str());
            CScript script{script_bytes.begin(), script_bytes.end()};
            CAmount amount{utxo_spent["amountSats"].getInt<int>()};
            utxos.emplace_back(amount, script);
        }

        // The intermediary values follow BIP341/342 unchanged.
        const uint8_t leaf_version{ParseMarker(vec["given"]["leafVersion"].get_str())};
        const auto leaf_script{ParseHex(vec["given"]["script"].get_str())};
        const uint256 leaf_hash{ComputeTapleafHash(leaf_version, leaf_script)};
        BOOST_CHECK_EQUAL(HexStr(leaf_hash), vec["intermediary"]["leafHash"].get_str());

        const XOnlyPubKey internal_pubkey{ParseHex(vec["given"]["internalPubkey"].get_str())};
        BOOST_CHECK_EQUAL(HexStr(internal_pubkey.ComputeTapTweakHash(&leaf_hash)), vec["intermediary"]["tweak"].get_str());
        const auto tweaked{internal_pubkey.CreateTapTweak(&leaf_hash)};
        BOOST_REQUIRE(tweaked.has_value());
        BOOST_CHECK_EQUAL(HexStr(XOnlyPubKey{tweaked->first}), vec["intermediary"]["tweakedPubkey"].get_str());

        // The fully signed transaction must round-trip and validate.
        auto signed_txhex = ParseHex(vec["expected"]["rawSignedTx"].get_str());
        CMutableTransaction signed_tx;
        SpanReader{signed_txhex} >> TX_WITH_WITNESS(signed_tx);
        const CTransaction final_tx{signed_tx};
        PrecomputedTransactionData signed_txdata;
        signed_txdata.Init(final_tx, std::vector<CTxOut>{utxos}, /*force=*/true);
        BOOST_CHECK(ValidateCISATransaction(final_tx, utxos, signed_txdata));
    }
}

BOOST_AUTO_TEST_CASE(cisa_consensus_vectors)
{
    UniValue tests;
    Assert(tests.read(json_tests::cisa_consensus_vectors));

    for (const auto& vec : tests["testCases"].getValues()) {
        const std::string id{vec["id"].get_str()};
        const bool expected_valid{vec["valid"].get_bool()};

        auto txhex = ParseHex(vec["tx"].get_str());
        CMutableTransaction mtx;
        SpanReader{txhex} >> TX_WITH_WITNESS(mtx);
        const CTransaction tx{mtx};

        std::vector<CTxOut> utxos;
        for (const auto& prevout : vec["prevouts"].getValues()) {
            auto script_bytes = ParseHex(prevout["scriptPubKey"].get_str());
            CScript script{script_bytes.begin(), script_bytes.end()};
            CAmount amount{prevout["amountSats"].getInt<int>()};
            utxos.emplace_back(amount, script);
        }
        BOOST_REQUIRE_EQUAL(utxos.size(), tx.vin.size());

        PrecomputedTransactionData txdata;
        txdata.Init(tx, std::vector<CTxOut>{utxos}, /*force=*/true);

        BOOST_CHECK_MESSAGE(ValidateCISATransaction(tx, utxos, txdata) == expected_valid,
                            "consensus vector mismatch: " << id);
    }
}

BOOST_AUTO_TEST_SUITE_END()
