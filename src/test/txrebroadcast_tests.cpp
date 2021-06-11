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

BOOST_AUTO_TEST_CASE(fee_rate)
{
    // Since the test chain comes with 100 blocks, the first coinbase is
    // already valid to spend. Generate another block to have two valid
    // coinbase inputs to spend.
    CreateAndProcessBlock(std::vector<CMutableTransaction>(), CScript());

    // Instantiate rebroadcast module & mine a block, so when we run
    // GetRebroadcastTransactions, Chain tip will be beyond m_tip_at_cache_time
    const auto chain_params = CreateChainParams(*m_node.args, CBaseChainParams::MAIN);
    TxRebroadcastHandler tx_rebroadcast(*m_node.mempool, *m_node.chainman, *chain_params);
    CBlock recent_block = CreateAndProcessBlock(std::vector<CMutableTransaction>(), CScript());
    CBlockIndex recent_block_index{recent_block.GetBlockHeader()};

    // Update m_cached_fee_rate
    // The transactions created in this test are each 157 bytes, and they set
    // the fee at 1 BTC and 2 BTC.
    CFeeRate cached_fee_rate(1.5 * COIN, 157);
    tx_rebroadcast.UpdateCachedFeeRate(cached_fee_rate);

    // Create two transactions
    CKey key;
    key.MakeNewKey(true);
    CScript output_destination = GetScriptForDestination(PKHash(key.GetPubKey()));

    // - One with a low fee rate
    CMutableTransaction tx_low = CreateValidMempoolTransaction(m_coinbase_txns[0], /* vout */ 0, /* input_height */ 0, coinbaseKey, output_destination, CAmount(49 * COIN));
    // - One with a high fee rate
    CMutableTransaction tx_high = CreateValidMempoolTransaction(m_coinbase_txns[1], /* vout */ 0, /* input_height */ 1, coinbaseKey, output_destination, CAmount(48 * COIN));

    // Confirm both transactions successfully made it into the mempool
    BOOST_CHECK_EQUAL(m_node.mempool->size(), 2U);

    // Age transaction to be older than REBROADCAST_MIN_TX_AGE
    SetMockTime(GetTime<std::chrono::seconds>() + 35min);

    // Check that only the high fee rate transaction would be selected
    const std::shared_ptr<const CBlock> empty_block;
    std::vector<TxIds> candidates = tx_rebroadcast.GetRebroadcastTransactions(empty_block, recent_block_index);
    BOOST_REQUIRE_EQUAL(candidates.size(), 1U);
    BOOST_CHECK_EQUAL(candidates.front().m_txid, tx_high.GetHash());
}

BOOST_AUTO_TEST_CASE(max_rebroadcast)
{
    // Create a transaction
    CKey key;
    key.MakeNewKey(true);
    CScript output_destination = GetScriptForDestination(PKHash(key.GetPubKey()));
    CMutableTransaction tx = CreateValidMempoolTransaction(m_coinbase_txns[0], /* vout */ 0, /* input_height */ 0, coinbaseKey, output_destination, CAmount(48 * COIN));
    uint256 txhsh = tx.GetHash();

    // Instantiate rebroadcast module & mine a block, so when we run
    // GetRebroadcastTransactions, Chain tip will be beyond m_tip_at_cache_time
    const auto chain_params = CreateChainParams(*m_node.args, CBaseChainParams::MAIN);
    TxRebroadcastHandler tx_rebroadcast(*m_node.mempool, *m_node.chainman, *chain_params);
    CBlock recent_block = CreateAndProcessBlock(std::vector<CMutableTransaction>(), CScript());
    CBlockIndex recent_block_index{recent_block.GetBlockHeader()};

    // Age transaction by 35 minutes, to be older than REBROADCAST_MIN_TX_AGE
    std::chrono::seconds current_time = GetTime<std::chrono::seconds>();
    current_time += 35min;
    SetMockTime(current_time);

    // Update the fee rate to be >0 so the rebroadcast logic doesn't return early
    CFeeRate cached_fee_rate(100, 100); // 1 sat/vB
    tx_rebroadcast.UpdateCachedFeeRate(cached_fee_rate);

    // Check that the transaction gets returned to rebroadcast
    const std::shared_ptr<const CBlock> empty_block;
    std::vector<TxIds> candidates = tx_rebroadcast.GetRebroadcastTransactions(empty_block, recent_block_index);
    BOOST_REQUIRE_EQUAL(candidates.size(), 1U);
    BOOST_CHECK_EQUAL(candidates.front().m_txid, txhsh);

    // Check if transaction was properly added to m_attempt_tracker
    // The attempt tracker records wtxids, but since this transaction does not
    // have a witness, the txhsh = wtxhsh so works for look ups.
    BOOST_CHECK(tx_rebroadcast.CheckRecordedAttempt(txhsh, 1, current_time));

    // Since the transaction was returned within the last
    // REBROADCAST_MIN_TX_AGE time, check it does not get returned again
    candidates = tx_rebroadcast.GetRebroadcastTransactions(empty_block, recent_block_index);
    BOOST_CHECK_EQUAL(candidates.size(), 0U);
    // And that the m_attempt_tracker entry is not updated
    BOOST_CHECK(tx_rebroadcast.CheckRecordedAttempt(txhsh, 1, current_time));

    // Bump time by 4 hours, to pass the MIN_INTERVAL time
    current_time += 4h;
    SetMockTime(current_time);
    // Then check that it gets returned for rebroadacst
    candidates = tx_rebroadcast.GetRebroadcastTransactions(empty_block, recent_block_index);
    BOOST_REQUIRE_EQUAL(candidates.size(), 1U);
    // And that m_attempt_tracker is properly updated
    BOOST_CHECK(tx_rebroadcast.CheckRecordedAttempt(txhsh, 2, current_time));

    // Update the record to have m_count to be MAX_REBROADCAST_COUNT, and last
    // attempt time of 4 hours ago
    auto attempt_time = GetTime<std::chrono::microseconds>() - 4h;
    tx_rebroadcast.UpdateAttempt(txhsh, 4, attempt_time);
    // Check that transaction is not rebroadcast
    candidates = tx_rebroadcast.GetRebroadcastTransactions(empty_block, recent_block_index);
    BOOST_CHECK_EQUAL(candidates.size(), 0U);
    // Check that the entry is removed from the m_attempt_tracker
    // And added to the m_max_filter
    BOOST_CHECK(tx_rebroadcast.CheckMaxAttempt(txhsh));
}

BOOST_AUTO_TEST_SUITE_END()
