// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit/.

#include <arith_uint256.h>
#include <consensus/consensus.h>
#include <node/block_template_manager.h>
#include <node/tx_collection.h>
#include <test/util/common.h>
#include <test/util/setup_common.h>
#include <tinyformat.h>
#include <uint256.h>

#include <boost/test/unit_test.hpp>

#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

BOOST_FIXTURE_TEST_SUITE(tx_collection_tests, RegTestingSetup)

BOOST_AUTO_TEST_CASE(collection_limits)
{
    auto& manager{*m_node.block_template_manager};
    BOOST_CHECK(manager.CreateTxCollection({}));
    const auto wtxid{Wtxid::FromUint256(uint256{1})};
    BOOST_CHECK_EXCEPTION(manager.CreateTxCollection({wtxid, wtxid}), std::runtime_error,
                          HasReason{"duplicate wtxid " + wtxid.ToString()});

    std::vector<Wtxid> wtxids;
    const auto max_txs{MAX_BLOCK_WEIGHT / MIN_TRANSACTION_WEIGHT};
    for (uint32_t i{0}; i < max_txs; ++i) {
        wtxids.push_back(Wtxid::FromUint256(ArithToUint256(arith_uint256{i})));
    }
    // The maximum succeeds; one more request fails before any mempool lookup.
    BOOST_CHECK(manager.CreateTxCollection(wtxids));
    wtxids.push_back(Wtxid::FromUint256(ArithToUint256(arith_uint256{MAX_BLOCK_WEIGHT})));
    BOOST_CHECK_EXCEPTION(manager.CreateTxCollection(wtxids), std::runtime_error,
                          HasReason{strprintf("too many wtxids (%d > %d)", wtxids.size(), max_txs)});
}

BOOST_AUTO_TEST_SUITE_END()
