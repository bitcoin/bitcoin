// Copyright (c) 2017-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <addresstype.h>
#include <chainparams.h>
#include <index/txindex.h>
#include <interfaces/chain.h>
#include <test/util/index.h>
#include <test/util/setup_common.h>
#include <util/threadpool.h>
#include <validation.h>

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(txindex_tests)

BOOST_FIXTURE_TEST_CASE(txindex_initial_sync, TestChain100Setup)
{
    TxIndex txindex(interfaces::MakeChain(m_node), 1 << 20, true);
    BOOST_REQUIRE(txindex.Init());

    CTransactionRef tx_disk;
    uint256 block_hash;

    // Transaction should not be found in the index before it is started.
    for (const auto& txn : m_coinbase_txns) {
        BOOST_CHECK(!txindex.FindTx(txn->GetHash(), block_hash, tx_disk));
    }

    // BlockUntilSyncedToCurrentChain should return false before txindex is started.
    BOOST_CHECK(!txindex.BlockUntilSyncedToCurrentChain());

    BOOST_REQUIRE(txindex.StartBackgroundSync());

    // Allow tx index to catch up with the block index.
    IndexWaitSynced(txindex, *Assert(m_node.shutdown));

    // Check that txindex excludes genesis block transactions.
    const CBlock& genesis_block = Params().GenesisBlock();
    for (const auto& txn : genesis_block.vtx) {
        BOOST_CHECK(!txindex.FindTx(txn->GetHash(), block_hash, tx_disk));
    }

    // Check that txindex has all txs that were in the chain before it started.
    for (const auto& txn : m_coinbase_txns) {
        if (!txindex.FindTx(txn->GetHash(), block_hash, tx_disk)) {
            BOOST_ERROR("FindTx failed");
        } else if (tx_disk->GetHash() != txn->GetHash()) {
            BOOST_ERROR("Read incorrect tx");
        }
    }

    // Check that new transactions in new blocks make it into the index.
    for (int i = 0; i < 10; i++) {
        CScript coinbase_script_pub_key = GetScriptForDestination(PKHash(coinbaseKey.GetPubKey()));
        std::vector<CMutableTransaction> no_txns;
        const CBlock& block = CreateAndProcessBlock(no_txns, coinbase_script_pub_key);
        const CTransaction& txn = *block.vtx[0];

        BOOST_CHECK(txindex.BlockUntilSyncedToCurrentChain());
        if (!txindex.FindTx(txn.GetHash(), block_hash, tx_disk)) {
            BOOST_ERROR("FindTx failed");
        } else if (tx_disk->GetHash() != txn.GetHash()) {
            BOOST_ERROR("Read incorrect tx");
        }
    }

    // It is not safe to stop and destroy the index until it finishes handling
    // the last BlockConnected notification. The BlockUntilSyncedToCurrentChain()
    // call above is sufficient to ensure this, but the
    // SyncWithValidationInterfaceQueue() call below is also needed to ensure
    // TSAN always sees the test thread waiting for the notification thread, and
    // avoid potential false positive reports.
    m_node.validation_signals->SyncWithValidationInterfaceQueue();

    // shutdown sequence (c.f. Shutdown() in init.cpp)
    txindex.Stop();
}

BOOST_FIXTURE_TEST_CASE(txindex_parallel_initial_sync, TestChain100Setup)
{
    int tip_height = 100; // pre-mined blocks
    const uint16_t MINE_BLOCKS = 650;
    for (int round = 0; round < 2; round++) { // two rounds to test sync from genesis and from a higher block
        // Generate blocks
        mineBlocks(MINE_BLOCKS);
        const CBlockIndex* tip = WITH_LOCK(::cs_main, return m_node.chainman->ActiveChain().Tip());
        BOOST_REQUIRE(tip->nHeight == MINE_BLOCKS + tip_height);
        tip_height = tip->nHeight;

        // Init and start index
        TxIndex txindex(interfaces::MakeChain(m_node), 1 << 20, /*f_memory=*/false);
        BOOST_REQUIRE(txindex.Init());
        std::shared_ptr<ThreadPool> thread_pool = std::make_shared<ThreadPool>();
        thread_pool->Start(2);
        txindex.SetThreadPool(thread_pool);
        txindex.SetTasksPerWorker(200);

        BOOST_CHECK(!txindex.BlockUntilSyncedToCurrentChain());
        BOOST_REQUIRE(txindex.StartBackgroundSync());

        // Allow tx index to catch up with the block index.
        IndexWaitSynced(txindex, *Assert(m_node.shutdown));

        // Check that txindex has all txs that were in the chain before it started.
        CTransactionRef tx_disk;
        uint256 block_hash;
        for (const auto& txn : m_coinbase_txns) {
            if (!txindex.FindTx(txn->GetHash(), block_hash, tx_disk)) {
                BOOST_ERROR("FindTx failed");
            } else if (tx_disk->GetHash() != txn->GetHash()) {
                BOOST_ERROR("Read incorrect tx");
            }
        }

        txindex.Interrupt();
        txindex.Stop();
    }
}

BOOST_AUTO_TEST_SUITE_END()
