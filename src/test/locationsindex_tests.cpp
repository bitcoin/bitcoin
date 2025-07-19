// Copyright (c) 2024-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <addresstype.h>
#include <chainparams.h>
#include <index/locationsindex.h>
#include <interfaces/chain.h>
#include <sync.h>
#include <test/util/setup_common.h>
#include <validation.h>

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(locationsindex_tests)

struct LocationsIndexSetup : public TestChain100Setup {
    LocationsIndex index;

    LocationsIndexSetup() : index(interfaces::MakeChain(m_node), 1 << 20, true)
    {
        BOOST_REQUIRE(index.Init());
    }

    CTransactionRef ReadTransaction(const uint256& block_hash, size_t i)
    {
        std::vector<std::byte> out;
        if (!index.ReadRawTransaction(block_hash, i, out)) {
            return nullptr;
        }
        CTransactionRef tx;
        DataStream ssTx(out);
        ssTx >> TX_WITH_WITNESS(tx);
        return tx;
    }

    CTransactionRef ReadTransactionFallback(FlatFilePos block_pos, size_t i)
    {
        CTransactionRef tx{};
        if (!m_node.chainman->m_blockman.ReadTxFromBlock(tx, block_pos, i)) {
            return nullptr;
        }
        return tx;
    }
};


BOOST_FIXTURE_TEST_CASE(index_initial_sync, LocationsIndexSetup)
{
    // Transaction should not be found in the index before it is started.
    BOOST_CHECK(!ReadTransaction(Params().GenesisBlock().GetHash(), 0));

    // BlockUntilSyncedToCurrentChain should return false before index is started.
    BOOST_CHECK(!index.BlockUntilSyncedToCurrentChain());

    index.Sync();

    // Check that index includes all mined transactions.
    const CBlockIndex* tip = WITH_LOCK(cs_main, return m_node.chainman->ActiveTip());
    BOOST_REQUIRE(tip);
    while (tip) {
        uint256 block_hash = tip->GetBlockHash();
        CBlockIndex* block_index{nullptr};
        FlatFilePos block_pos{};
        {
            LOCK(cs_main);
            block_index = m_node.chainman->m_blockman.LookupBlockIndex(block_hash);
            block_pos = block_index->GetBlockPos();
        }
        BOOST_REQUIRE(block_index);
        BOOST_REQUIRE(!block_pos.IsNull());
        CBlock block;
        BOOST_REQUIRE(m_node.chainman->m_blockman.ReadBlock(block, *block_index));
        for (size_t i = 0; i < block.vtx.size(); ++i) {
            CTransactionRef tx = ReadTransaction(block_hash, i);
            BOOST_CHECK(tx);
            BOOST_CHECK(*tx == *block.vtx[i]);
            CTransactionRef tx_fallback = ReadTransactionFallback(block_pos, i);
            BOOST_CHECK(tx_fallback);
            BOOST_CHECK(*tx_fallback == *block.vtx[i]);
        }
        BOOST_CHECK(!ReadTransaction(block_hash, block.vtx.size()));
        BOOST_CHECK(!ReadTransaction(block_hash, block.vtx.size() + 1));
        BOOST_CHECK(!ReadTransactionFallback(block_pos, block.vtx.size()));
        BOOST_CHECK(!ReadTransactionFallback(block_pos, block.vtx.size() + 1));
        tip = tip->pprev;
    }

    // Test invalid blockhashes
    BOOST_CHECK(!ReadTransaction(uint256::ZERO, 0));
    BOOST_CHECK(!ReadTransaction(uint256::ONE, 0));

    // It is not safe to stop and destroy the index until it finishes handling
    // the last BlockConnected notification. The BlockUntilSyncedToCurrentChain()
    // call above is sufficient to ensure this, but the
    // SyncWithValidationInterfaceQueue() call below is also needed to ensure
    // TSAN always sees the test thread waiting for the notification thread, and
    // avoid potential false positive reports.
    m_node.validation_signals->SyncWithValidationInterfaceQueue();

    // shutdown sequence (c.f. Shutdown() in init.cpp)
    index.Stop();
}

BOOST_AUTO_TEST_SUITE_END()
