// Copyright (c) 2011-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <amount.h>
#include <clientversion.h>
#include <consensus/tx_check.h>
#include <consensus/validation.h>
#include <core_io.h>
#include <key_io.h>
#include <rpc/util.h>
#include <script/interpreter.h>
#include <script/script.h>
#include <streams.h>
#include <test/util/setup_common.h>
#include <txmempool.h>
#include <txrebroadcast.h>
#include <util/time.h>
#include <validation.h>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(txrebroadcast_tests, TestChain100Setup)

BOOST_AUTO_TEST_CASE(recency)
{
    // Since the test chain comes with 100 blocks, the first coinbase is
    // already valid to spend. Generate another block to have two valid
    // coinbase inputs to spend.
    CreateAndProcessBlock(std::vector<CMutableTransaction>(), CScript());

    // Create a transaction
    CKey key;
    key.MakeNewKey(true);
    CScript output_destination = GetScriptForDestination(PKHash(key.GetPubKey()));
    CMutableTransaction tx_old = CreateValidMempoolTransaction(m_coinbase_txns[0], /* vout */ 0, /* input_height */ 0, coinbaseKey, output_destination, CAmount(48 * COIN));

    // Age transaction to be older than REBROADCAST_MIN_TX_AGE
    SetMockTime(GetTime<std::chrono::seconds>() + 35min);

    // Create a recent transaction
    CMutableTransaction tx_new = CreateValidMempoolTransaction(m_coinbase_txns[1], /* vout */ 0, /* input_height */ 1, coinbaseKey, output_destination, CAmount(48 * COIN));

    // Confirm both transactions successfully made it into the mempool
    BOOST_CHECK_EQUAL(m_node.mempool->size(), 2U);

    // Instantiate rebroadcast module & mine a block, so when we run
    // GetRebroadcastTransactions, Chain tip will be beyond m_tip_at_cache_time
    const auto chain_params = CreateChainParams(*m_node.args, CBaseChainParams::MAIN);
    TxRebroadcastHandler tx_rebroadcast(*m_node.mempool, *m_node.chainman, *chain_params);
    CBlock recent_block = CreateAndProcessBlock(std::vector<CMutableTransaction>(), CScript());
    CBlockIndex recent_block_index{recent_block.GetBlockHeader()};

    // Update the fee rate to be >0 so the rebroadcast logic doesn't return early
    CFeeRate cached_fee_rate(100, 100); // 1 sat/vB
    tx_rebroadcast.UpdateCachedFeeRate(cached_fee_rate);

    // Confirm that only the old transaction is included
    const std::shared_ptr<const CBlock> empty_block;
    std::vector<TxIds> candidates = tx_rebroadcast.GetRebroadcastTransactions(empty_block, recent_block_index);
    BOOST_REQUIRE_EQUAL(candidates.size(), 1U);
    BOOST_CHECK_EQUAL(candidates.front().m_txid, tx_old.GetHash());
}

BOOST_AUTO_TEST_SUITE_END()
