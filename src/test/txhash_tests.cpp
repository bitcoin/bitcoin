// Copyright (c) 2011-2023 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/data/txhash_tests.json.h>

#include <common/system.h>
#include <core_io.h>
#include <key.h>
#include <logging.h>
#include <uint256.h>
#include <script/script.h>
#include <script/solver.h>
#include <script/txhash.h>
#include <test/util/json.h>
#include <test/util/setup_common.h>
#include <test/util/transaction_utils.h>
#include <util/strencodings.h>

#include <cstdint>
#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

#include <univalue.h>

BOOST_FIXTURE_TEST_SUITE(txhash_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(txhash_json_test)
{
    // Read tests from test/data/txhash_tests.json
    UniValue test_cases = read_json(json_tests::txhash_tests).get_array();

    for (unsigned int case_idx = 0; case_idx < test_cases.size(); case_idx++) {
        const UniValue& test_case = test_cases[case_idx];

        CMutableTransaction tx;
        assert(DecodeHexTx(tx, test_case["tx"].get_str()));
        std::vector<CTxOut> prevs = {};
        for (unsigned int i = 0; i < test_case["prevs"].get_array().size(); i++) {
            CTxOut txout;
            assert(DecodeHexTxOut(txout, test_case["prevs"].get_array()[i].get_str()));
            prevs.push_back(txout);
        }

        const UniValue& vectors = test_case["vectors"].get_array();
        for (unsigned int vector_idx = 0; vector_idx < vectors.size(); vector_idx++) {
            const UniValue& v = vectors[vector_idx].get_obj();
            const std::string& vid = v["id"].get_str();

            BOOST_TEST_MESSAGE("Starting test " << vid);

            std::vector<unsigned char> txfs = ParseHex(v["txfs"].get_str());
            uint32_t codeseparator_pos;
            if (!v["codeseparator"].isNull()) {
                codeseparator_pos = v["codeseparator"].getInt<uint32_t>();
            } else {
                codeseparator_pos = 0xffffffff;
            }
            uint32_t txin_pos = v["input"].getInt<uint32_t>();

            TxHashCache cache {};

            uint256 txhash;
            bool ret = calculate_txhash(txhash, Span{txfs}, cache, tx, prevs, codeseparator_pos, txin_pos);
            BOOST_CHECK_MESSAGE(ret, "txhash calculation for vector " << vid);

            uint256 expected = uint256SBE(v["txhash"].get_str());
            BOOST_CHECK_MESSAGE(
                txhash == expected,
                "test vector " << vid << ": " << txhash << " != " << expected
            );
        }
    }
}


BOOST_AUTO_TEST_SUITE_END()
