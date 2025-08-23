// Copyright (c) 2018-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/data/blockfilters.json.h>
#include <test/util/setup_common.h>

#include <blockfilter.h>
#include <core_io.h>
#include <evo/assetlocktx.h>
#include <evo/netinfo.h>
#include <evo/providertx.h>
#include <evo/specialtx.h>
#include <netaddress.h>
#include <netbase.h>
#include <serialize.h>
#include <span.h>
#include <streams.h>
#include <univalue.h>
#include <util/strencodings.h>

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(blockfilter_tests)

BOOST_AUTO_TEST_CASE(gcsfilter_test)
{
    GCSFilter::ElementSet included_elements, excluded_elements;
    for (int i = 0; i < 100; ++i) {
        GCSFilter::Element element1(32);
        element1[0] = i;
        included_elements.insert(std::move(element1));

        GCSFilter::Element element2(32);
        element2[1] = i;
        excluded_elements.insert(std::move(element2));
    }

    GCSFilter filter({0, 0, 10, 1 << 10}, included_elements);
    for (const auto& element : included_elements) {
        BOOST_CHECK(filter.Match(element));

        auto insertion = excluded_elements.insert(element);
        BOOST_CHECK(filter.MatchAny(excluded_elements));
        excluded_elements.erase(insertion.first);
    }
}

BOOST_AUTO_TEST_CASE(gcsfilter_default_constructor)
{
    GCSFilter filter;
    BOOST_CHECK_EQUAL(filter.GetN(), 0U);
    BOOST_CHECK_EQUAL(filter.GetEncoded().size(), 1U);

    const GCSFilter::Params& params = filter.GetParams();
    BOOST_CHECK_EQUAL(params.m_siphash_k0, 0U);
    BOOST_CHECK_EQUAL(params.m_siphash_k1, 0U);
    BOOST_CHECK_EQUAL(params.m_P, 0);
    BOOST_CHECK_EQUAL(params.m_M, 1U);
}

BOOST_AUTO_TEST_CASE(blockfilter_basic_test)
{
    CScript included_scripts[5], excluded_scripts[4];

    // First two are outputs on a single transaction.
    included_scripts[0] << std::vector<unsigned char>(0, 65) << OP_CHECKSIG;
    included_scripts[1] << OP_DUP << OP_HASH160 << std::vector<unsigned char>(1, 20) << OP_EQUALVERIFY << OP_CHECKSIG;

    // Third is an output on in a second transaction.
    included_scripts[2] << OP_1 << std::vector<unsigned char>(2, 33) << OP_1 << OP_CHECKMULTISIG;

    // Last two are spent by a single transaction.
    included_scripts[3] << OP_0 << std::vector<unsigned char>(3, 32);
    included_scripts[4] << OP_4 << OP_ADD << OP_8 << OP_EQUAL;

    // OP_RETURN output is an output on the second transaction.
    excluded_scripts[0] << OP_RETURN << std::vector<unsigned char>(4, 40);

    // This script is not related to the block at all.
    excluded_scripts[1] << std::vector<unsigned char>(5, 33) << OP_CHECKSIG;

    // OP_RETURN is non-standard since it's not followed by a data push, but is still excluded from
    // filter.
    excluded_scripts[2] << OP_RETURN << OP_4 << OP_ADD << OP_8 << OP_EQUAL;

    CMutableTransaction tx_1;
    tx_1.vout.emplace_back(100, included_scripts[0]);
    tx_1.vout.emplace_back(200, included_scripts[1]);
    tx_1.vout.emplace_back(0, excluded_scripts[0]);

    CMutableTransaction tx_2;
    tx_2.vout.emplace_back(300, included_scripts[2]);
    tx_2.vout.emplace_back(0, excluded_scripts[2]);
    tx_2.vout.emplace_back(400, excluded_scripts[3]); // Script is empty

    CBlock block;
    block.vtx.push_back(MakeTransactionRef(tx_1));
    block.vtx.push_back(MakeTransactionRef(tx_2));

    CBlockUndo block_undo;
    block_undo.vtxundo.emplace_back();
    block_undo.vtxundo.back().vprevout.emplace_back(CTxOut(500, included_scripts[3]), 1000, true);
    block_undo.vtxundo.back().vprevout.emplace_back(CTxOut(600, included_scripts[4]), 10000, false);
    block_undo.vtxundo.back().vprevout.emplace_back(CTxOut(700, excluded_scripts[3]), 100000, false);

    BlockFilter block_filter(BlockFilterType::BASIC_FILTER, block, block_undo);
    const GCSFilter& filter = block_filter.GetFilter();

    for (const CScript& script : included_scripts) {
        BOOST_CHECK(filter.Match(GCSFilter::Element(script.begin(), script.end())));
    }
    for (const CScript& script : excluded_scripts) {
        BOOST_CHECK(!filter.Match(GCSFilter::Element(script.begin(), script.end())));
    }

    // Test serialization/unserialization.
    BlockFilter block_filter2;

    CDataStream stream(SER_NETWORK, PROTOCOL_VERSION);
    stream << block_filter;
    stream >> block_filter2;

    BOOST_CHECK_EQUAL(block_filter.GetFilterType(), block_filter2.GetFilterType());
    BOOST_CHECK_EQUAL(block_filter.GetBlockHash(), block_filter2.GetBlockHash());
    BOOST_CHECK(block_filter.GetEncodedFilter() == block_filter2.GetEncodedFilter());

    BlockFilter default_ctor_block_filter_1;
    BlockFilter default_ctor_block_filter_2;
    BOOST_CHECK_EQUAL(default_ctor_block_filter_1.GetFilterType(), default_ctor_block_filter_2.GetFilterType());
    BOOST_CHECK_EQUAL(default_ctor_block_filter_1.GetBlockHash(), default_ctor_block_filter_2.GetBlockHash());
    BOOST_CHECK(default_ctor_block_filter_1.GetEncodedFilter() == default_ctor_block_filter_2.GetEncodedFilter());
}

BOOST_AUTO_TEST_CASE(blockfilters_json_test)
{
    UniValue json;
    std::string json_data(json_tests::blockfilters,
                          json_tests::blockfilters + sizeof(json_tests::blockfilters));
    if (!json.read(json_data) || !json.isArray()) {
        BOOST_ERROR("Parse error.");
        return;
    }

    const UniValue& tests = json.get_array();
    for (unsigned int i = 0; i < tests.size(); i++) {
        UniValue test = tests[i];
        std::string strTest = test.write();

        if (test.size() == 1) {
            continue;
        } else if (test.size() < 7) {
            BOOST_ERROR("Bad test: " << strTest);
            continue;
        }

        unsigned int pos = 0;
        /*int block_height =*/ test[pos++].getInt<int>();
        uint256 block_hash;
        BOOST_CHECK(ParseHashStr(test[pos++].get_str(), block_hash));

        CBlock block;
        BOOST_REQUIRE(DecodeHexBlk(block, test[pos++].get_str()));

        CBlockUndo block_undo;
        block_undo.vtxundo.emplace_back();
        CTxUndo& tx_undo = block_undo.vtxundo.back();
        const UniValue& prev_scripts = test[pos++].get_array();
        for (unsigned int ii = 0; ii < prev_scripts.size(); ii++) {
            std::vector<unsigned char> raw_script = ParseHex(prev_scripts[ii].get_str());
            CTxOut txout(0, CScript(raw_script.begin(), raw_script.end()));
            tx_undo.vprevout.emplace_back(txout, 0, false);
        }

        uint256 prev_filter_header_basic;
        BOOST_CHECK(ParseHashStr(test[pos++].get_str(), prev_filter_header_basic));
        std::vector<unsigned char> filter_basic = ParseHex(test[pos++].get_str());
        uint256 filter_header_basic;
        BOOST_CHECK(ParseHashStr(test[pos++].get_str(), filter_header_basic));

        BlockFilter computed_filter_basic(BlockFilterType::BASIC_FILTER, block, block_undo);
        BOOST_CHECK(computed_filter_basic.GetFilter().GetEncoded() == filter_basic);

        uint256 computed_header_basic = computed_filter_basic.ComputeHeader(prev_filter_header_basic);
        BOOST_CHECK(computed_header_basic == filter_header_basic);
    }
}

BOOST_AUTO_TEST_CASE(blockfilter_type_names)
{
    BOOST_CHECK_EQUAL(BlockFilterTypeName(BlockFilterType::BASIC_FILTER), "basic");
    BOOST_CHECK_EQUAL(BlockFilterTypeName(static_cast<BlockFilterType>(255)), "");

    BlockFilterType filter_type;
    BOOST_CHECK(BlockFilterTypeByName("basic", filter_type));
    BOOST_CHECK_EQUAL(filter_type, BlockFilterType::BASIC_FILTER);

    BOOST_CHECK(!BlockFilterTypeByName("unknown", filter_type));
}

BOOST_AUTO_TEST_CASE(blockfilter_special_transactions_test)
{
    // Test that special transaction fields are properly included in block filters
    // This ensures parity with bloom filter functionality

    // Test 1: ProRegTx (Provider Registration Transaction)
    CMutableTransaction protx;
    protx.nVersion = 3;
    protx.nType = TRANSACTION_PROVIDER_REGISTER;

    CProRegTx proRegPayload;
    proRegPayload.nVersion = 1;
    proRegPayload.nType = MnType::Regular;
    proRegPayload.nMode = 0;
    proRegPayload.collateralOutpoint = COutPoint(uint256S("1234567890abcdef1234567890abcdef1234567890abcdef1234567890abcdef"), 0);

    // Set up network info (required for serialization)
    proRegPayload.netInfo = NetInfoInterface::MakeNetInfo(1);  // Version 1

    proRegPayload.keyIDOwner = CKeyID(uint160(ParseHex("1111111111111111111111111111111111111111")));
    proRegPayload.keyIDVoting = CKeyID(uint160(ParseHex("2222222222222222222222222222222222222222")));

    // Generate a valid BLS key
    CBLSSecretKey blsKey;
    blsKey.MakeNewKey();
    proRegPayload.pubKeyOperator.Set(blsKey.GetPublicKey(), false);
    proRegPayload.inputsHash = uint256();  // Empty inputs hash

    // Create payout script
    CScript payoutScript = CScript() << OP_DUP << OP_HASH160 << ToByteVector(ParseHex("3333333333333333333333333333333333333333")) << OP_EQUALVERIFY << OP_CHECKSIG;
    proRegPayload.scriptPayout = payoutScript;

    SetTxPayload(protx, proRegPayload);

    // Test 2: ProUpServTx (Provider Update Service Transaction)
    CMutableTransaction proupservtx;
    proupservtx.nVersion = 3;
    proupservtx.nType = TRANSACTION_PROVIDER_UPDATE_SERVICE;

    CProUpServTx proUpServPayload;
    proUpServPayload.nVersion = 1;
    proUpServPayload.proTxHash = uint256S("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
    proUpServPayload.netInfo = NetInfoInterface::MakeNetInfo(1);
    CScript operatorPayoutScript = CScript() << OP_DUP << OP_HASH160 << ToByteVector(ParseHex("5555555555555555555555555555555555555555")) << OP_EQUALVERIFY << OP_CHECKSIG;
    proUpServPayload.scriptOperatorPayout = operatorPayoutScript;
    proUpServPayload.inputsHash = uint256();

    SetTxPayload(proupservtx, proUpServPayload);

    // Test 3: ProUpRegTx (Provider Update Registrar Transaction)
    CMutableTransaction proupregtx;
    proupregtx.nVersion = 3;
    proupregtx.nType = TRANSACTION_PROVIDER_UPDATE_REGISTRAR;

    CProUpRegTx proUpRegPayload;
    proUpRegPayload.nVersion = 1;
    proUpRegPayload.proTxHash = uint256S("bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb");
    proUpRegPayload.nMode = 0;
    proUpRegPayload.pubKeyOperator.Set(blsKey.GetPublicKey(), false);
    proUpRegPayload.keyIDVoting = CKeyID(uint160(ParseHex("6666666666666666666666666666666666666666")));
    CScript newPayoutScript = CScript() << OP_DUP << OP_HASH160 << ToByteVector(ParseHex("7777777777777777777777777777777777777777")) << OP_EQUALVERIFY << OP_CHECKSIG;
    proUpRegPayload.scriptPayout = newPayoutScript;
    proUpRegPayload.inputsHash = uint256();

    SetTxPayload(proupregtx, proUpRegPayload);

    // Test 4: ProUpRevTx (Provider Update Revoke Transaction)
    CMutableTransaction prouprevtx;
    prouprevtx.nVersion = 3;
    prouprevtx.nType = TRANSACTION_PROVIDER_UPDATE_REVOKE;

    CProUpRevTx proUpRevPayload;
    proUpRevPayload.nVersion = 1;
    proUpRevPayload.proTxHash = uint256S("cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc");
    proUpRevPayload.nReason = 1;
    proUpRevPayload.inputsHash = uint256();

    SetTxPayload(prouprevtx, proUpRevPayload);

    // Test 5: AssetLockTx
    CMutableTransaction assetlockTx;
    assetlockTx.nVersion = 3;
    assetlockTx.nType = TRANSACTION_ASSET_LOCK;

    CAssetLockPayload assetLockPayload;
    CScript creditScript = CScript() << OP_DUP << OP_HASH160 << ToByteVector(ParseHex("4444444444444444444444444444444444444444")) << OP_EQUALVERIFY << OP_CHECKSIG;
    CTxOut creditOut(1000, creditScript);
    std::vector<CTxOut> creditOutputs = {creditOut};
    assetLockPayload = CAssetLockPayload(creditOutputs);

    SetTxPayload(assetlockTx, assetLockPayload);

    // Create block with all special transactions
    CBlock block;
    block.vtx.push_back(MakeTransactionRef(protx));
    block.vtx.push_back(MakeTransactionRef(proupservtx));
    block.vtx.push_back(MakeTransactionRef(proupregtx));
    block.vtx.push_back(MakeTransactionRef(prouprevtx));
    block.vtx.push_back(MakeTransactionRef(assetlockTx));

    // Create minimal block undo
    CBlockUndo block_undo;
    for (size_t i = 0; i < block.vtx.size(); i++) {
        block_undo.vtxundo.emplace_back();
    }

    // Create block filter
    BlockFilter block_filter(BlockFilterType::BASIC_FILTER, block, block_undo);
    const GCSFilter& filter = block_filter.GetFilter();

    // Test ProRegTx fields
    // collateralOutpoint
    CDataStream outpointStream(SER_NETWORK, PROTOCOL_VERSION);
    outpointStream << proRegPayload.collateralOutpoint;
    auto outpointSpan = MakeUCharSpan(outpointStream);
    BOOST_CHECK(filter.Match(GCSFilter::Element(outpointSpan.begin(), outpointSpan.end())));

    // keyIDOwner
    BOOST_CHECK(filter.Match(GCSFilter::Element(proRegPayload.keyIDOwner.begin(), proRegPayload.keyIDOwner.end())));

    // keyIDVoting
    BOOST_CHECK(filter.Match(GCSFilter::Element(proRegPayload.keyIDVoting.begin(), proRegPayload.keyIDVoting.end())));

    // scriptPayout
    BOOST_CHECK(filter.Match(GCSFilter::Element(payoutScript.begin(), payoutScript.end())));

    // Test ProUpServTx fields
    // proTxHash
    BOOST_CHECK(filter.Match(GCSFilter::Element(proUpServPayload.proTxHash.begin(), proUpServPayload.proTxHash.end())));

    // scriptOperatorPayout
    BOOST_CHECK(filter.Match(GCSFilter::Element(operatorPayoutScript.begin(), operatorPayoutScript.end())));

    // Test ProUpRegTx fields
    // proTxHash
    BOOST_CHECK(filter.Match(GCSFilter::Element(proUpRegPayload.proTxHash.begin(), proUpRegPayload.proTxHash.end())));

    // keyIDVoting
    BOOST_CHECK(filter.Match(GCSFilter::Element(proUpRegPayload.keyIDVoting.begin(), proUpRegPayload.keyIDVoting.end())));

    // scriptPayout
    BOOST_CHECK(filter.Match(GCSFilter::Element(newPayoutScript.begin(), newPayoutScript.end())));

    // Test ProUpRevTx fields
    // proTxHash
    BOOST_CHECK(filter.Match(GCSFilter::Element(proUpRevPayload.proTxHash.begin(), proUpRevPayload.proTxHash.end())));

    // Test AssetLock creditOutputs script
    BOOST_CHECK(filter.Match(GCSFilter::Element(creditScript.begin(), creditScript.end())));

    // Check that unrelated data is not in the filter
    CKeyID unrelatedKey(uint160(ParseHex("9999999999999999999999999999999999999999")));
    BOOST_CHECK(!filter.Match(GCSFilter::Element(unrelatedKey.begin(), unrelatedKey.end())));
}

BOOST_AUTO_TEST_CASE(blockfilter_special_transactions_edge_cases)
{
    // Test edge cases for special transaction filtering

    // Test 1: Empty scripts should be handled gracefully
    CMutableTransaction protx_empty;
    protx_empty.nVersion = 3;
    protx_empty.nType = TRANSACTION_PROVIDER_REGISTER;

    CProRegTx proRegPayload_empty;
    proRegPayload_empty.nVersion = 1;
    proRegPayload_empty.nType = MnType::Regular;
    proRegPayload_empty.nMode = 0;
    proRegPayload_empty.collateralOutpoint = COutPoint(uint256S("1234567890abcdef1234567890abcdef1234567890abcdef1234567890abcdef"), 0);
    proRegPayload_empty.netInfo = NetInfoInterface::MakeNetInfo(1);
    proRegPayload_empty.keyIDOwner = CKeyID(uint160(ParseHex("1111111111111111111111111111111111111111")));
    proRegPayload_empty.keyIDVoting = CKeyID(uint160(ParseHex("2222222222222222222222222222222222222222")));

    CBLSSecretKey blsKey;
    blsKey.MakeNewKey();
    proRegPayload_empty.pubKeyOperator.Set(blsKey.GetPublicKey(), false);
    proRegPayload_empty.inputsHash = uint256();

    // Empty payout script
    proRegPayload_empty.scriptPayout = CScript();

    SetTxPayload(protx_empty, proRegPayload_empty);

    // Test 2: AssetLock with OP_RETURN output (should be excluded)
    CMutableTransaction assetlock_opreturn;
    assetlock_opreturn.nVersion = 3;
    assetlock_opreturn.nType = TRANSACTION_ASSET_LOCK;

    CAssetLockPayload assetLockPayload_opreturn;
    CScript opReturnScript = CScript() << OP_RETURN << ToByteVector(ParseHex("deadbeef"));
    CScript normalScript = CScript() << OP_DUP << OP_HASH160 << ToByteVector(ParseHex("8888888888888888888888888888888888888888")) << OP_EQUALVERIFY << OP_CHECKSIG;

    CTxOut opReturnOut(0, opReturnScript);
    CTxOut normalOut(1000, normalScript);
    std::vector<CTxOut> mixedOutputs = {opReturnOut, normalOut};
    assetLockPayload_opreturn = CAssetLockPayload(mixedOutputs);

    SetTxPayload(assetlock_opreturn, assetLockPayload_opreturn);

    // Test 3: ProUpServTx with empty operator payout script
    CMutableTransaction proupserv_empty;
    proupserv_empty.nVersion = 3;
    proupserv_empty.nType = TRANSACTION_PROVIDER_UPDATE_SERVICE;

    CProUpServTx proUpServPayload_empty;
    proUpServPayload_empty.nVersion = 1;
    proUpServPayload_empty.proTxHash = uint256S("dddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd");
    proUpServPayload_empty.netInfo = NetInfoInterface::MakeNetInfo(1);
    proUpServPayload_empty.scriptOperatorPayout = CScript();  // Empty script
    proUpServPayload_empty.inputsHash = uint256();

    SetTxPayload(proupserv_empty, proUpServPayload_empty);

    // Create block with edge case transactions
    CBlock block;
    block.vtx.push_back(MakeTransactionRef(protx_empty));
    block.vtx.push_back(MakeTransactionRef(assetlock_opreturn));
    block.vtx.push_back(MakeTransactionRef(proupserv_empty));

    // Create minimal block undo
    CBlockUndo block_undo;
    for (size_t i = 0; i < block.vtx.size(); i++) {
        block_undo.vtxundo.emplace_back();
    }

    // Create block filter - should not crash with edge cases
    BlockFilter block_filter(BlockFilterType::BASIC_FILTER, block, block_undo);
    const GCSFilter& filter = block_filter.GetFilter();

    // Test that non-empty fields are still included
    BOOST_CHECK(filter.Match(GCSFilter::Element(proRegPayload_empty.keyIDOwner.begin(),
                                                proRegPayload_empty.keyIDOwner.end())));

    // Test that OP_RETURN script is NOT in filter
    BOOST_CHECK(!filter.Match(GCSFilter::Element(opReturnScript.begin(), opReturnScript.end())));

    // Test that normal script from AssetLock IS in filter
    BOOST_CHECK(filter.Match(GCSFilter::Element(normalScript.begin(), normalScript.end())));

    // Test that ProUpServTx hash is still included even with empty payout script
    BOOST_CHECK(filter.Match(GCSFilter::Element(proUpServPayload_empty.proTxHash.begin(),
                                               proUpServPayload_empty.proTxHash.end())));
}

BOOST_AUTO_TEST_SUITE_END()
