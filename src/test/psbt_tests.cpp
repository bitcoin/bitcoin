// Copyright (c) 2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://www.opensource.org/licenses/mit-license.php.

#include <addresstype.h>
#include <key.h>
#include <psbt.h>
#include <script/descriptor.h>
#include <script/interpreter.h>
#include <script/script.h>
#include <script/signingprovider.h>
#include <script/solver.h>
#include <test/util/setup_common.h>
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

BOOST_AUTO_TEST_CASE(infer_taproot_tree)
{
    // InferTaprootTree() filters each (script, leaf_ver, control_block) candidate recorded for a
    // taproot input through four independent structural checks (see the "Skip script records
    // that ..." comments in script/signingprovider.cpp) before trusting it as a genuine leaf of
    // the tree committed to by TaprootSpendData's Merkle root. This locks down that a candidate
    // failing any one of those checks is rejected -- which, since the tree can then no longer be
    // fully explored, currently surfaces as InferTaprootTree() returning std::nullopt for the
    // whole input -- while an uncorrupted two-leaf tree is inferred successfully.
    const int leaf_ver{TAPROOT_LEAF_TAPSCRIPT};
    const std::vector<unsigned char> script1{ToByteVector(CScript() << OP_1)};
    const std::vector<unsigned char> script2{ToByteVector(CScript() << OP_2)};

    TaprootBuilder builder;
    builder.Add(/*depth=*/1, script1, leaf_ver);
    builder.Add(/*depth=*/1, script2, leaf_ver);
    builder.Finalize(XOnlyPubKey::NUMS_H);
    const XOnlyPubKey output{builder.GetOutput()};
    const TaprootSpendData good{builder.GetSpendData()};

    // Sanity check the fixture before corrupting it: both leaves are correctly inferred.
    {
        const auto inferred{InferTaprootTree(good, output)};
        BOOST_REQUIRE(inferred.has_value());
        BOOST_CHECK_EQUAL(inferred->size(), 2U);
    }

    const auto it1{good.scripts.find({script1, leaf_ver})};
    BOOST_REQUIRE(it1 != good.scripts.end());
    BOOST_REQUIRE_EQUAL(it1->second.size(), 1U);
    const std::vector<unsigned char> good_control_block{*it1->second.begin()};
    // One Merkle branch node (the sibling leaf, script2) on top of the base control block.
    BOOST_REQUIRE_EQUAL(good_control_block.size(), TAPROOT_CONTROL_BASE_SIZE + TAPROOT_CONTROL_NODE_SIZE);

    // Replace script1's only control block with a corrupted one (optionally also moving it to a
    // corrupted leaf version), leaving script2's entry and the Merkle root / internal key
    // untouched, and check that inference now fails: script1's node is never explored, so the
    // overall tree is incomplete.
    auto check_rejected = [&](std::vector<unsigned char> corrupted_control_block, int corrupted_leaf_ver) {
        TaprootSpendData corrupted{good};
        corrupted.scripts.erase({script1, leaf_ver});
        corrupted.scripts[{script1, corrupted_leaf_ver}] = {std::move(corrupted_control_block)};
        BOOST_CHECK(!InferTaprootTree(corrupted, output).has_value());
    };

    // Skip script records with nonsensical leaf version.
    check_rejected(good_control_block, /*corrupted_leaf_ver=*/leaf_ver + 1);

    // Skip script records with invalid control block sizes.
    {
        std::vector<unsigned char> too_short{good_control_block};
        too_short.pop_back();
        check_rejected(too_short, leaf_ver);
    }

    // Skip script records that don't match the control block.
    {
        std::vector<unsigned char> mismatched_leaf_version{good_control_block};
        mismatched_leaf_version[0] ^= 0x04;
        check_rejected(mismatched_leaf_version, leaf_ver);
    }

    // Skip script records that don't match the provided Merkle root.
    {
        std::vector<unsigned char> wrong_merkle_root{good_control_block};
        wrong_merkle_root.back() ^= 0x01;
        check_rejected(wrong_merkle_root, leaf_ver);
    }
}

BOOST_AUTO_TEST_SUITE_END()
