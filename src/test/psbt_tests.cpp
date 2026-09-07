// Copyright (c) 2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://www.opensource.org/licenses/mit-license.php.

#include <addresstype.h>
#include <cisa.h>
#include <core_io.h>
#include <key.h>
#include <policy/policy.h>
#include <psbt.h>
#include <script/descriptor.h>
#include <script/interpreter.h>
#include <script/script.h>
#include <script/signingprovider.h>
#include <script/solver.h>
#include <test/data/cisa_psbt_vectors.json.h>
#include <test/util/setup_common.h>
#include <univalue.h>
#include <util/check.h>
#include <util/strencodings.h>
#include <util/string.h>

#include <boost/test/unit_test.hpp>

#include <string>
#include <vector>

BOOST_FIXTURE_TEST_SUITE(psbt_tests, BasicTestingSetup)

static PSBTProprietary MakeProprietary(uint64_t subtype, uint8_t key_data, uint8_t value)
{
    return PSBTProprietary{
        .subtype = subtype,
        .identifier = {'p', 's', 'b', 't'},
        .key = {key_data},
        .value = {value},
    };
}

void CheckTimeLock(const std::string& base64_psbt, std::optional<uint32_t> timelock)
{
    util::Result<PartiallySignedTransaction> psbt = DecodeBase64PSBT(base64_psbt);
    BOOST_CHECK(psbt);

    std::optional<uint32_t> computed_timelock = psbt->ComputeTimeLock();
    std::optional<CMutableTransaction> tx = psbt->GetUnsignedTx();
    if (timelock) {
        BOOST_CHECK(computed_timelock);
        BOOST_CHECK_EQUAL(*computed_timelock, *timelock);
        BOOST_CHECK(tx);
        BOOST_CHECK_EQUAL(tx->nLockTime, *timelock);
    } else {
        BOOST_CHECK(!computed_timelock);
        BOOST_CHECK(!tx);
    }
}

BOOST_AUTO_TEST_CASE(psbt2_timelock_test)
{
    CheckTimeLock("cHNidP8BAgQCAAAAAQQBAQEFAQIB+wQCAAAAAAEOIAsK2SFBnByHGXNdctxzn56p4GONH+TB7vD5lECEgV/IAQ8EAAAAAAABAwgACK8vAAAAAAEEFgAUxDD2TEdW2jENvRoIVXLvKZkmJywAAQMIi73rCwAAAAABBBYAFE3Rk6yWSlasG54cyoRU/i9HT4UTAA==", 0);
    CheckTimeLock("cHNidP8BAgQCAAAAAQMEAAAAAAEEAQIBBQEBAfsEAgAAAAABDiAPdY2/vU2nwWyKMwnDyB4RAPVh6mRttbAXUsSF4b3enwEPBAEAAAAAAQ4gOhs7PIN9ZInqejHY5sfdUDwAG+8+BpWOdXSAjWjKeKUBDwQAAAAAAAEDCE+TNXcAAAAAAQQWABQLE1LKzQPPaqG388jWOIZxs0peEQA=", 0);
    CheckTimeLock("cHNidP8BAgQCAAAAAQMEAAAAAAEEAQIBBQEBAfsEAgAAAAABDiAPdY2/vU2nwWyKMwnDyB4RAPVh6mRttbAXUsSF4b3enwEPBAEAAAABEgQQJwAAAAEOIDobOzyDfWSJ6nox2ObH3VA8ABvvPgaVjnV0gI1oynilAQ8EAAAAAAABAwhPkzV3AAAAAAEEFgAUCxNSys0Dz2qht/PI1jiGcbNKXhEA", 10000);
    CheckTimeLock("cHNidP8BAgQCAAAAAQMEAAAAAAEEAQIBBQEBAfsEAgAAAAABDiAPdY2/vU2nwWyKMwnDyB4RAPVh6mRttbAXUsSF4b3enwEPBAEAAAABEgQQJwAAAAEOIDobOzyDfWSJ6nox2ObH3VA8ABvvPgaVjnV0gI1oynilAQ8EAAAAAAESBCgjAAAAAQMIT5M1dwAAAAABBBYAFAsTUsrNA89qobfzyNY4hnGzSl4RAA==", 10000);
    CheckTimeLock("cHNidP8BAgQCAAAAAQMEAAAAAAEEAQIBBQEBAfsEAgAAAAABDiAPdY2/vU2nwWyKMwnDyB4RAPVh6mRttbAXUsSF4b3enwEPBAEAAAABEgQQJwAAAAEOIDobOzyDfWSJ6nox2ObH3VA8ABvvPgaVjnV0gI1oynilAQ8EAAAAAAERBIyNxGIBEgQoIwAAAAEDCE+TNXcAAAAAAQQWABQLE1LKzQPPaqG388jWOIZxs0peEQA=", 10000);
    CheckTimeLock("cHNidP8BAgQCAAAAAQMEAAAAAAEEAQIBBQEBAfsEAgAAAAABDiAPdY2/vU2nwWyKMwnDyB4RAPVh6mRttbAXUsSF4b3enwEPBAEAAAABEQSLjcRiARIEECcAAAABDiA6Gzs8g31kiep6Mdjmx91QPAAb7z4GlY51dICNaMp4pQEPBAAAAAABEQSMjcRiARIEKCMAAAABAwhPkzV3AAAAAAEEFgAUCxNSys0Dz2qht/PI1jiGcbNKXhEA", 10000);
    CheckTimeLock("cHNidP8BAgQCAAAAAQMEAAAAAAEEAQIBBQEBAfsEAgAAAAABDiAPdY2/vU2nwWyKMwnDyB4RAPVh6mRttbAXUsSF4b3enwEPBAEAAAABEQSLjcRiAAEOIDobOzyDfWSJ6nox2ObH3VA8ABvvPgaVjnV0gI1oynilAQ8EAAAAAAERBIyNxGIBEgQoIwAAAAEDCE+TNXcAAAAAAQQWABQLE1LKzQPPaqG388jWOIZxs0peEQA=", 1657048460);
    CheckTimeLock("cHNidP8BAgQCAAAAAQMEAAAAAAEEAQIBBQEBAfsEAgAAAAABDiAPdY2/vU2nwWyKMwnDyB4RAPVh6mRttbAXUsSF4b3enwEPBAEAAAABEQSLjcRiARIEECcAAAABDiA6Gzs8g31kiep6Mdjmx91QPAAb7z4GlY51dICNaMp4pQEPBAAAAAABEQSMjcRiAAEDCE+TNXcAAAAAAQQWABQLE1LKzQPPaqG388jWOIZxs0peEQA=", 1657048460);
    CheckTimeLock("cHNidP8BAgQCAAAAAQMEAAAAAAEEAQIBBQEBAfsEAgAAAAABDiAPdY2/vU2nwWyKMwnDyB4RAPVh6mRttbAXUsSF4b3enwEPBAEAAAAAAQ4gOhs7PIN9ZInqejHY5sfdUDwAG+8+BpWOdXSAjWjKeKUBDwQAAAAAAREEjI3EYgABAwhPkzV3AAAAAAEEFgAUCxNSys0Dz2qht/PI1jiGcbNKXhEA", 1657048460);
    CheckTimeLock("cHNidP8BAgQCAAAAAQMEAAAAAAEEAQIBBQEBAfsEAgAAAAABDiAPdY2/vU2nwWyKMwnDyB4RAPVh6mRttbAXUsSF4b3enwEPBAEAAAABEgQQJwAAAAEOIDobOzyDfWSJ6nox2ObH3VA8ABvvPgaVjnV0gI1oynilAQ8EAAAAAAERBIyNxGIAAQMIT5M1dwAAAAABBBYAFAsTUsrNA89qobfzyNY4hnGzSl4RAA==", std::nullopt);
    CheckTimeLock("cHNidP8BAgQCAAAAAQMEAAAAAAEEAQIBBQEBAfsEAgAAAAABDiA6Gzs8g31kiep6Mdjmx91QPAAb7z4GlY51dICNaMp4pQEPBAAAAAABEQSMjcRiAAEOIA91jb+9TafBbIozCcPIHhEA9WHqZG21sBdSxIXhvd6fAQ8EAQAAAAESBBAnAAAAAQMIT5M1dwAAAAABBBYAFAsTUsrNA89qobfzyNY4hnGzSl4RAA==", std::nullopt);
}

BOOST_AUTO_TEST_CASE(psbt2_addinput)
{
    FastRandomContext rng(/*fDeterministic=*/true);

    CMutableTransaction mtx;
    PartiallySignedTransaction psbt(mtx, /*version=*/2);
    psbt.m_tx_modifiable.emplace();
    psbt.m_tx_modifiable->set(0, true);
    BOOST_CHECK_EQUAL(psbt.inputs.size(), 0);

    // Same PSBT version is required
    uint256 txid;
    rng.fillrand(MakeWritableByteSpan(txid));
    PSBTInput psbtin_v0(/*psbt_version=*/0, Txid::FromUint256(txid), /*prev_out=*/0);
    BOOST_CHECK(!psbt.AddInput(psbtin_v0));
    BOOST_CHECK_EQUAL(psbt.inputs.size(), 0);
    rng.fillrand(MakeWritableByteSpan(txid));
    PSBTInput psbtin(/*psbt_version=*/2, Txid::FromUint256(txid), /*prev_out=*/0);
    BOOST_CHECK(psbt.AddInput(psbtin));
    BOOST_CHECK_EQUAL(psbt.inputs.size(), 1);

    // Duplicates are not allowed
    BOOST_CHECK(!psbt.AddInput(psbtin));
    BOOST_CHECK_EQUAL(psbt.inputs.size(), 1);

    // Input with a unique txid is allowed
    rng.fillrand(MakeWritableByteSpan(txid));
    PSBTInput psbtin2(/*psbt_version=*/2, Txid::FromUint256(txid), /*prev_out=*/0);
    BOOST_CHECK(psbt.AddInput(psbtin2));
    BOOST_CHECK_EQUAL(psbt.inputs.size(), 2);

    // Disabling inputs modifiable flag prevents adding new inputs
    psbt.m_tx_modifiable->set(0, false);
    rng.fillrand(MakeWritableByteSpan(txid));
    PSBTInput psbtin3(/*psbt_version=*/2, Txid::FromUint256(txid), /*prev_out=*/0);
    BOOST_CHECK(!psbt.AddInput(psbtin3));
    BOOST_CHECK_EQUAL(psbt.inputs.size(), 2);
    psbt.m_tx_modifiable->set(0, true);

    // Make sure that timelock compatibility checks are working
    // No previous required timelocks, new input with both height and time timelocks is allowed
    rng.fillrand(MakeWritableByteSpan(txid));
    PSBTInput psbtin4(/*psbt_version=*/2, Txid::FromUint256(txid), /*prev_out=*/0);
    psbtin4.time_locktime = LOCKTIME_THRESHOLD;
    psbtin4.height_locktime = 100;
    BOOST_CHECK(psbt.AddInput(psbtin4));
    BOOST_CHECK_EQUAL(psbt.inputs.size(), 3);

    // Input with only a time timelock is allowed
    rng.fillrand(MakeWritableByteSpan(txid));
    PSBTInput psbtin5(/*psbt_version=*/2, Txid::FromUint256(txid), /*prev_out=*/0);
    psbtin5.time_locktime = LOCKTIME_THRESHOLD + 1;
    BOOST_CHECK(psbt.AddInput(psbtin5));
    BOOST_CHECK_EQUAL(psbt.inputs.size(), 4);

    // Input with only a height timelock is not allowed because of previous
    rng.fillrand(MakeWritableByteSpan(txid));
    PSBTInput psbtin6(/*psbt_version=*/2, Txid::FromUint256(txid), /*prev_out=*/0);
    psbtin6.height_locktime = 100;
    BOOST_CHECK(!psbt.AddInput(psbtin6));
    BOOST_CHECK_EQUAL(psbt.inputs.size(), 4);

    // Adding an input that already has a signature is allowed
    rng.fillrand(MakeWritableByteSpan(txid));
    PSBTInput psbtin7(/*psbt_version=*/2, Txid::FromUint256(txid), /*prev_out=*/0);
    psbtin7.final_script_sig << OP_1;
    BOOST_CHECK(psbt.AddInput(psbtin7));
    BOOST_CHECK_EQUAL(psbt.inputs.size(), 5);

    // Same thing, but with other things that have signatures
    psbtin7.final_script_sig.clear();
    psbtin7.final_script_witness.stack.emplace_back();
    BOOST_CHECK(!psbt.AddInput(psbtin7));
    BOOST_CHECK_EQUAL(psbt.inputs.size(), 5);
    psbtin7.final_script_witness.SetNull();
    psbtin7.partial_sigs.emplace();
    BOOST_CHECK(!psbt.AddInput(psbtin7));
    BOOST_CHECK_EQUAL(psbt.inputs.size(), 5);
    psbtin7.partial_sigs.clear();
    psbtin7.m_tap_key_sig.push_back(0);
    BOOST_CHECK(!psbt.AddInput(psbtin7));
    BOOST_CHECK_EQUAL(psbt.inputs.size(), 5);
    psbtin7.m_tap_key_sig.clear();
    psbtin7.m_tap_script_sigs.emplace();
    BOOST_CHECK(!psbt.AddInput(psbtin7));
    BOOST_CHECK_EQUAL(psbt.inputs.size(), 5);
    psbtin7.m_tap_script_sigs.clear();
    psbtin7.m_musig2_partial_sigs.emplace();
    BOOST_CHECK(!psbt.AddInput(psbtin7));
    BOOST_CHECK_EQUAL(psbt.inputs.size(), 5);

    // Adding an input that changes the timelock is no longer allowed
    rng.fillrand(MakeWritableByteSpan(txid));
    PSBTInput psbtin8(/*psbt_version=*/2, Txid::FromUint256(txid), /*prev_out=*/0);
    psbtin8.time_locktime = LOCKTIME_THRESHOLD + 2;
    BOOST_CHECK(!psbt.AddInput(psbtin8));
    BOOST_CHECK_EQUAL(psbt.inputs.size(), 5);
}

BOOST_AUTO_TEST_CASE(psbt2_addoutput)
{
    CMutableTransaction mtx;
    PartiallySignedTransaction psbt(mtx, /*version=*/2);
    psbt.m_tx_modifiable.emplace();
    psbt.m_tx_modifiable->set(1, true);
    BOOST_CHECK_EQUAL(psbt.outputs.size(), 0);

    // Same PSBT version is required
    PSBTOutput psbtout_v0(/*psbt_version=*/0, /*amount=*/1, CScript());
    BOOST_CHECK(!psbt.AddOutput(psbtout_v0));
    BOOST_CHECK_EQUAL(psbt.outputs.size(), 0);
    PSBTOutput psbtout(/*psbt_version=*/2, /*amount=*/1, CScript());
    BOOST_CHECK(psbt.AddOutput(psbtout));
    BOOST_CHECK_EQUAL(psbt.outputs.size(), 1);

    // Disabling outputs modifiable flag prevents adding new outputs
    psbt.m_tx_modifiable->set(1, false);
    PSBTOutput psbtout2(/*psbt_version=*/2, /*amount=*/1, CScript());
    BOOST_CHECK(!psbt.AddOutput(psbtout2));
    BOOST_CHECK_EQUAL(psbt.outputs.size(), 1);
    psbt.m_tx_modifiable->set(1, true);
    PSBTOutput psbtout3(/*psbt_version=*/2, /*amount=*/1, CScript());
    BOOST_CHECK(psbt.AddOutput(psbtout3));
    BOOST_CHECK_EQUAL(psbt.outputs.size(), 2);
}

BOOST_AUTO_TEST_CASE(merge_proprietary_fields)
{
    CMutableTransaction tx;
    tx.vin.emplace_back(COutPoint{});
    tx.vout.emplace_back(0, CScript{});

    PartiallySignedTransaction left(tx);
    PartiallySignedTransaction right(tx);

    const auto left_prop = MakeProprietary(/*subtype=*/1, /*key_data=*/0x01, /*value=*/0xaa);
    const auto right_prop = MakeProprietary(/*subtype=*/2, /*key_data=*/0x02, /*value=*/0xbb);

    left.m_proprietary.insert(left_prop);
    left.inputs[0].m_proprietary.insert(left_prop);
    left.outputs[0].m_proprietary.insert(left_prop);

    right.m_proprietary.insert(right_prop);
    right.inputs[0].m_proprietary.insert(right_prop);
    right.outputs[0].m_proprietary.insert(right_prop);

    BOOST_REQUIRE(left.Merge(right));

    BOOST_REQUIRE_EQUAL(left.m_proprietary.size(), 2U);
    BOOST_REQUIRE_EQUAL(left.inputs[0].m_proprietary.size(), 2U);
    BOOST_REQUIRE_EQUAL(left.outputs[0].m_proprietary.size(), 2U);

    const auto global_it = left.m_proprietary.find(right_prop);
    BOOST_REQUIRE(global_it != left.m_proprietary.end());
    BOOST_CHECK(global_it->value == right_prop.value);

    const auto input_it = left.inputs[0].m_proprietary.find(right_prop);
    BOOST_REQUIRE(input_it != left.inputs[0].m_proprietary.end());
    BOOST_CHECK(input_it->value == right_prop.value);

    const auto output_it = left.outputs[0].m_proprietary.find(right_prop);
    BOOST_REQUIRE(output_it != left.outputs[0].m_proprietary.end());
    BOOST_CHECK(output_it->value == right_prop.value);
}

struct PSBTOutputTest {
    CPubKey pubkey;
    FlatSigningProvider provider;
    CScript script_pubkey;

    explicit PSBTOutputTest(std::string descriptor)
    {
        CKey key{GenerateRandomKey()};
        pubkey = key.GetPubKey();
        provider.keys.emplace(pubkey.GetID(), key);

        util::ReplaceAll(descriptor, "<KEY>", HexStr(pubkey));
        std::string error;
        auto descriptors{Parse(descriptor, provider, error, /*require_checksum=*/false)};
        BOOST_REQUIRE_MESSAGE(!descriptors.empty(), error);
        std::vector<CScript> output_scripts;
        BOOST_REQUIRE(descriptors[0]->Expand(/*pos=*/0, provider, output_scripts, provider));
        BOOST_REQUIRE_EQUAL(output_scripts.size(), 1);
        script_pubkey = output_scripts[0];
    }

    PSBTOutput UpdateOutput(bool has_input) const
    {
        CMutableTransaction tx;
        if (has_input) tx.vin.emplace_back();
        tx.vout.emplace_back(0, script_pubkey);
        PartiallySignedTransaction psbt{tx};
        UpdatePSBTOutput(provider, psbt, 0);
        return psbt.outputs[0];
    }
};

BOOST_AUTO_TEST_CASE(update_psbt_output_keypaths)
{
    for (bool has_input : {false, true}) {
        for (const auto& descriptor : {"pkh(<KEY>)", "wpkh(<KEY>)"}) {
            PSBTOutputTest test{descriptor};
            auto out{test.UpdateOutput(has_input)};
            BOOST_CHECK(out.hd_keypaths.contains(test.pubkey));
            BOOST_CHECK(out.redeem_script.empty());
            BOOST_CHECK(out.witness_script.empty());
        }
    }
}

BOOST_AUTO_TEST_CASE(update_psbt_output_redeem_script)
{
    for (bool has_input : {false, true}) {
        PSBTOutputTest test{"sh(wpkh(<KEY>))"};
        auto out{test.UpdateOutput(has_input)};
        BOOST_CHECK(out.redeem_script == GetScriptForDestination(WitnessV0KeyHash{test.pubkey}));
        BOOST_CHECK(out.hd_keypaths.contains(test.pubkey));
    }
}

BOOST_AUTO_TEST_CASE(update_psbt_output_witness_script)
{
    for (bool has_input : {false, true}) {
        PSBTOutputTest test{"wsh(pk(<KEY>))"};
        auto out{test.UpdateOutput(has_input)};
        BOOST_CHECK(out.witness_script == GetScriptForRawPubKey(test.pubkey));
        BOOST_CHECK(out.hd_keypaths.contains(test.pubkey));
    }
}

BOOST_AUTO_TEST_CASE(update_psbt_output_miniscript_timelock)
{
    for (bool has_input : {false, true}) {
        PSBTOutputTest test{"wsh(and_v(v:pk(<KEY>),older(144)))"};
        auto out{test.UpdateOutput(has_input)};
        BOOST_CHECK(GetScriptForDestination(WitnessV0ScriptHash{out.witness_script}) == test.script_pubkey);
        BOOST_CHECK(out.hd_keypaths.contains(test.pubkey));
    }
}

BOOST_AUTO_TEST_CASE(update_psbt_output_taproot)
{
    for (bool has_input : {false, true}) {
        PSBTOutputTest test{"rawtr(<KEY>)"};
        auto out{test.UpdateOutput(has_input)};
        BOOST_CHECK(out.m_tap_bip32_paths.contains(XOnlyPubKey{test.pubkey}));
        BOOST_CHECK(out.hd_keypaths.empty());
    }
}


static std::string EncodeBase64PSBT(const PartiallySignedTransaction& psbt)
{
    DataStream ss;
    ss << psbt;
    return EncodeBase64(ss);
}

//! Validate like consensus does for witness v2: per-input script checks plus the transaction level aggregate verification
static bool ValidateCISATransaction(const CMutableTransaction& mtx, const std::vector<CTxOut>& spent_outputs)
{
    const CTransaction tx{mtx};
    PrecomputedTransactionData txdata;
    txdata.Init(tx, std::vector<CTxOut>{spent_outputs}, /*force=*/true);
    for (unsigned int i = 0; i < tx.vin.size(); i++) {
        TransactionSignatureChecker checker(&tx, i, spent_outputs[i].nValue, txdata, MissingDataBehavior::FAIL);
        if (!VerifyScript(tx.vin[i].scriptSig, spent_outputs[i].scriptPubKey, &tx.vin[i].scriptWitness, STANDARD_SCRIPT_VERIFY_FLAGS, checker)) return false;
    }
    return VerifyCISATransaction(tx, spent_outputs, STANDARD_SCRIPT_VERIFY_FLAGS, txdata);
}

BOOST_AUTO_TEST_CASE(cisa_psbt_vectors)
{
    UniValue tests;
    Assert(tests.read(json_tests::cisa_psbt_vectors));

    for (const auto& vec : tests["invalid"].getValues()) {
        BOOST_CHECK(!DecodeBase64PSBT(vec["base64"].get_str()));
    }

    for (const auto& vec : tests["valid"].getValues()) {
        const auto& stages{vec["stages"].getValues()};
        const std::string& signed_stage{stages[stages.size() - 2]["base64"].get_str()};
        const std::string& finalized_stage{stages.back()["base64"].get_str()};
        const std::string& expected_tx{vec["expected"]["transaction"].get_str()};
        const auto& intermediary{vec["intermediary"]};
        for (const auto& stage : stages) {
            const auto psbt{DecodeBase64PSBT(stage["base64"].get_str())};
            BOOST_REQUIRE(psbt);
            BOOST_CHECK_EQUAL(EncodeBase64PSBT(*psbt), stage["base64"].get_str());
        }

        // The tweaked secret keys sign as the output keys
        FlatSigningProvider keys;
        std::map<uint256, FullAggSecNonce> secnonces;
        keys.cisa_secnonces = &secnonces;
        std::vector<XOnlyPubKey> pubkeys;
        std::vector<uint8_t> modes;
        for (const auto& input : vec["given"]["inputs"].getValues()) {
            CKey key;
            const auto seckey{ParseHex(input["tweakedSecretKey"].get_str())};
            key.Set(seckey.begin(), seckey.end(), true);
            keys.keys.emplace(key.GetPubKey().GetID(), key);
            pubkeys.emplace_back(ParseHex(input["outputKey"].get_str()));
            modes.push_back(ParseHex(input["aggregationMode"].get_str()).at(0));
        }
        const bool has_fullagg{std::ranges::find(modes, CISA_MARKER_FULLAGG) != modes.end()};

        // Sign every input from the first stage, half-aggregation signatures are deterministic
        auto psbt{*DecodeBase64PSBT(stages[0]["base64"].get_str())};
        const PrecomputedTransactionData txdata{*PrecomputePSBTData(psbt)};
        std::vector<CTxOut> utxos;
        for (size_t i = 0; i < psbt.inputs.size(); i++) {
            PSBTInput& input = psbt.inputs[i];
            utxos.push_back(input.witness_utxo);
            const int sighash_type{input.sighash_type.value_or(SIGHASH_DEFAULT)};
            ScriptExecutionData execdata;
            execdata.m_annex_init = true;
            execdata.m_annex_present = false;
            execdata.m_cisa_agg_mode = modes[i];
            uint256 msg;
            BOOST_REQUIRE(SignatureHashSchnorr(msg, execdata, *psbt.GetUnsignedTx(), i, sighash_type, modes[i] ? SigVersion::WITNESS_V2_KEYPATH : SigVersion::TAPROOT, txdata, MissingDataBehavior::FAIL));
            BOOST_CHECK_EQUAL(HexStr(msg), intermediary["messages"][i].get_str());

            const auto result{SignPSBTInput(keys, psbt, i, &txdata, {.sighash_type = sighash_type, .finalize = false})};
            if (modes[i] == CISA_MARKER_FULLAGG) {
                // The first pass only produces the nonce
                BOOST_CHECK(!result && result.error() == PSBTError::INCOMPLETE);
                BOOST_CHECK_EQUAL(input.m_cisa_fullagg_pubnonce.size(), FULLAGG_PUBNONCE_SIZE);
                continue;
            }
            BOOST_CHECK(result);
            auto expected_sig{ParseHex(intermediary["signatures"][i].get_str())};
            if (sighash_type != SIGHASH_DEFAULT) expected_sig.push_back(sighash_type);
            BOOST_CHECK(modes[i] == CISA_MARKER_HALFAGG ? input.m_cisa_halfagg_sig == expected_sig : input.m_tap_key_sig == expected_sig);
        }
        if (!has_fullagg) BOOST_CHECK_EQUAL(EncodeBase64PSBT(psbt), signed_stage);
        for (size_t i = 0; i < psbt.inputs.size(); i++) {
            BOOST_CHECK(SignPSBTInput(keys, psbt, i, &txdata, {.sighash_type = psbt.inputs[i].sighash_type, .finalize = false}));
        }
        CMutableTransaction tx;
        BOOST_REQUIRE(FinalizeAndExtractPSBT(psbt, tx));
        BOOST_CHECK(ValidateCISATransaction(tx, utxos));
        if (!has_fullagg) {
            BOOST_CHECK_EQUAL(EncodeBase64PSBT(psbt), finalized_stage);
            BOOST_CHECK_EQUAL(EncodeHexTx(CTransaction(tx)), expected_tx);
        }

        // The signature material of the vector aggregates to its aggregate signatures
        for (const uint8_t marker : {CISA_MARKER_HALFAGG, CISA_MARKER_FULLAGG}) {
            std::vector<XOnlyPubKey> group_pubkeys;
            std::vector<uint256> group_msgs;
            std::vector<std::vector<uint8_t>> sigs, pubnonces;
            std::vector<uint256> partial_sigs;
            for (size_t i = 0; i < modes.size(); i++) {
                if (modes[i] != marker) continue;
                group_pubkeys.push_back(pubkeys[i]);
                group_msgs.emplace_back(ParseHex(intermediary["messages"][i].get_str()));
                if (marker == CISA_MARKER_HALFAGG) {
                    sigs.push_back(ParseHex(intermediary["signatures"][i].get_str()));
                } else {
                    pubnonces.push_back(ParseHex(intermediary["publicNonces"][i].get_str()));
                    partial_sigs.emplace_back(ParseHex(intermediary["partialSignatures"][i].get_str()));
                }
            }
            if (group_pubkeys.empty()) continue;
            if (marker == CISA_MARKER_HALFAGG) {
                BOOST_CHECK(AggregateHalfAggSigs(group_pubkeys, group_msgs, sigs) == ParseHex(intermediary["halfaggAggregateSignature"].get_str()));
            } else {
                for (size_t pos = 0; pos < partial_sigs.size(); pos++) {
                    BOOST_CHECK(VerifyFullAggPartialSig(partial_sigs[pos], group_pubkeys, group_msgs, pubnonces, pos));
                    BOOST_CHECK(!VerifyFullAggPartialSig(partial_sigs[pos], group_pubkeys, group_msgs, pubnonces, pos ^ 1));
                }
                BOOST_CHECK(AggregateFullAggSigs(group_pubkeys, group_msgs, pubnonces, partial_sigs) == ParseHex(intermediary["fullaggAggregateSignature"].get_str()));
            }
        }

        // The Finalizer builds the vector's witnesses from its signature material
        auto vec_psbt{*DecodeBase64PSBT(signed_stage)};
        CMutableTransaction vec_tx;
        BOOST_REQUIRE(FinalizeAndExtractPSBT(vec_psbt, vec_tx));
        BOOST_CHECK_EQUAL(EncodeBase64PSBT(vec_psbt), finalized_stage);
        BOOST_CHECK_EQUAL(EncodeHexTx(CTransaction(vec_tx)), expected_tx);
        for (size_t k = 0; k + 2 < stages.size(); k++) {
            auto partial{*DecodeBase64PSBT(stages[k]["base64"].get_str())};
            BOOST_CHECK(!FinalizePSBT(partial));
        }

        // An aggregated input carrying an opted-out signature must not be finalized
        auto optout_psbt{*DecodeBase64PSBT(signed_stage)};
        PSBTInput& aggregated = optout_psbt.inputs[std::ranges::find_if(modes, [](uint8_t m) { return m != 0; }) - modes.begin()];
        aggregated.m_tap_key_sig.assign(64, 0);
        BOOST_CHECK(!FinalizePSBT(optout_psbt));
        BOOST_CHECK(!PSBTInputSigned(aggregated));
    }

    // A Combiner must fail on differing modes, nonces or partial signatures of an input
    const auto& fullagg_case{tests["valid"][1]};
    auto nonces{*DecodeBase64PSBT(fullagg_case["stages"][2]["base64"].get_str())};
    auto other_nonces{nonces};
    BOOST_CHECK(nonces.Merge(other_nonces));
    other_nonces.inputs[1].m_cisa_fullagg_pubnonce[5] ^= 1;
    BOOST_CHECK(!nonces.Merge(other_nonces));
    auto other_mode{nonces};
    other_mode.inputs[0].m_cisa_mode = CISA_MARKER_HALFAGG;
    BOOST_CHECK(!nonces.Merge(other_mode));
    auto psigs{*DecodeBase64PSBT(fullagg_case["stages"][3]["base64"].get_str())};
    auto other_psigs{psigs};
    other_psigs.inputs[1].m_cisa_fullagg_partial_sig = uint256::ONE;
    BOOST_CHECK(!psigs.Merge(other_psigs));
}

BOOST_AUTO_TEST_SUITE_END()
