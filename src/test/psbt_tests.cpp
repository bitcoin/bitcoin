// Copyright (c) 2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://www.opensource.org/licenses/mit-license.php.

#include <psbt.h>

#include <boost/test/unit_test.hpp>
#include <test/util/setup_common.h>

BOOST_FIXTURE_TEST_SUITE(psbt_tests, BasicTestingSetup)

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

BOOST_AUTO_TEST_SUITE_END()
