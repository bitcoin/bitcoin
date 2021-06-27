// Copyright (c) 2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/data/blockfilters.json.h>
#include <test/test_dash.h>

#include <blockfilter.h>
#include <core_io.h>
#include <serialize.h>
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

    GCSFilter filter(0, 0, 10, 1 << 10, included_elements);
    for (const auto& element : included_elements) {
        BOOST_CHECK(filter.Match(element));

        auto insertion = excluded_elements.insert(element);
        BOOST_CHECK(filter.MatchAny(excluded_elements));
        excluded_elements.erase(insertion.first);
    }
}

BOOST_AUTO_TEST_CASE(blockfilter_basic_test)
{
    CScript included_scripts[5], excluded_scripts[3];

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

    CMutableTransaction tx_1;
    tx_1.vout.emplace_back(100, included_scripts[0]);
    tx_1.vout.emplace_back(200, included_scripts[1]);

    CMutableTransaction tx_2;
    tx_2.vout.emplace_back(300, included_scripts[2]);
    tx_2.vout.emplace_back(0, excluded_scripts[0]);
    tx_2.vout.emplace_back(400, excluded_scripts[2]); // Script is empty

    CBlock block;
    block.vtx.push_back(MakeTransactionRef(tx_1));
    block.vtx.push_back(MakeTransactionRef(tx_2));

    CBlockUndo block_undo;
    block_undo.vtxundo.emplace_back();
    block_undo.vtxundo.back().vprevout.emplace_back(CTxOut(500, included_scripts[3]), 1000, true);
    block_undo.vtxundo.back().vprevout.emplace_back(CTxOut(600, included_scripts[4]), 10000, false);
    block_undo.vtxundo.back().vprevout.emplace_back(CTxOut(700, excluded_scripts[2]), 100000, false);

    BlockFilter block_filter(BlockFilterType::BASIC_FILTER, block, block_undo);
    const GCSFilter& filter = block_filter.GetFilter();

    for (const CScript& script : included_scripts) {
        BOOST_CHECK(filter.Match(GCSFilter::Element(script.begin(), script.end())));
    }
    for (const CScript& script : excluded_scripts) {
        BOOST_CHECK(!filter.Match(GCSFilter::Element(script.begin(), script.end())));
    }
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
        /*int block_height =*/ test[pos++].get_int();
        /*uint256 block_hash =*/ ParseHashStr(test[pos++].get_str(), "block_hash");

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

        uint256 prev_filter_header_basic = ParseHashStr(test[pos++].get_str(), "prev_filter_header_basic");
        std::vector<unsigned char> filter_basic = ParseHex(test[pos++].get_str());
        uint256 filter_header_basic = ParseHashStr(test[pos++].get_str(), "filter_header_basic");

        BlockFilter computed_filter_basic(BlockFilterType::BASIC_FILTER, block, block_undo);
        BOOST_CHECK(computed_filter_basic.GetFilter().GetEncoded() == filter_basic);

        uint256 computed_header_basic = computed_filter_basic.ComputeHeader(prev_filter_header_basic);
        BOOST_CHECK(computed_header_basic == filter_header_basic);
    }
}

BOOST_AUTO_TEST_SUITE_END()
