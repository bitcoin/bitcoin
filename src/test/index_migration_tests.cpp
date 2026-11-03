// Copyright (c) 2026 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <coins.h>
#include <index/timestampindex.h>
#include <interfaces/chain.h>
#include <node/blockstorage.h>
#include <test/util/setup_common.h>
#include <txdb.h>
#include <util/system.h>
#include <validation.h>

#include <boost/test/unit_test.hpp>

namespace {
struct IndexMigrationSetup : public TestChainSetup {
    IndexMigrationSetup() :
        TestChainSetup{100, CBaseChainParams::REGTEST, {"-timestampindex=1"}}
    {
    }

    //! Give MigrateOldIndexData() the on-disk chainstate best block it reads on a real node.
    void WriteChainstateBestBlock()
    {
        const uint256 tip{WITH_LOCK(cs_main, return m_node.chainman->ActiveChain().Tip()->GetBlockHash())};
        CCoinsViewDB coins_db{gArgs.GetDataDirNet() / "chainstate", /*nCacheSize=*/1 << 20, /*fMemory=*/false,
                              /*fWipe=*/false};
        CCoinsMapMemoryResource resource;
        CCoinsMap no_coins{0, CCoinsMap::hasher{}, CCoinsMap::key_equal{}, &resource};
        BOOST_REQUIRE(coins_db.BatchWrite(no_coins, tip));
    }

    //! Height the timestamp index resumes from, read before its sync thread can advance it.
    int LoadedTimestampIndexHeight()
    {
        node::g_indexes_ready_to_sync = false;
        TimestampIndex index{interfaces::MakeChain(m_node), /*n_cache_size=*/1 << 20};
        BOOST_REQUIRE(index.Start());
        const int height{index.GetSummary().best_block_height};
        index.Interrupt();
        index.Stop();
        node::g_indexes_ready_to_sync = true;
        return height;
    }
};
} // namespace

BOOST_AUTO_TEST_SUITE(index_migration_tests)

// A datadir upgraded from v23 keeps its legacy "timestampindex" flag. If the index's own
// database is then missing (deleted to rebuild it), there is nothing left to migrate, and
// the empty database must not be stamped with the chain tip as if it were synced.
BOOST_FIXTURE_TEST_CASE(empty_index_is_not_adopted_as_migrated, IndexMigrationSetup)
{
    WriteChainstateBestBlock();
    CBlockTreeDB& block_tree_db{*WITH_LOCK(cs_main, return m_node.chainman->m_blockman.m_block_tree_db.get())};
    BOOST_REQUIRE(block_tree_db.WriteFlag("timestampindex", true));

    BOOST_REQUIRE(WITH_LOCK(cs_main, return block_tree_db.MigrateOldIndexData()));

    BOOST_CHECK_EQUAL(LoadedTimestampIndexHeight(), 0);
    // The legacy entries are gone, so older versions must not trust their synchronous index either.
    bool legacy_current{true};
    BOOST_REQUIRE(block_tree_db.ReadFlag("timestampindex", legacy_current));
    BOOST_CHECK(!legacy_current);
}

BOOST_AUTO_TEST_SUITE_END()
