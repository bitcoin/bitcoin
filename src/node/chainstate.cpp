// Copyright (c) 2021-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <node/chainstate.h>

#include <arith_uint256.h>
#include <chain.h>
#include <coins.h>
#include <consensus/params.h>
#include <kernel/caches.h>
#include <node/blockstorage.h>
#include <sync.h>
#include <tinyformat.h>
#include <txdb.h>
#include <uint256.h>
#include <util/byte_units.h>
#include <util/fs.h>
#include <util/log.h>
#include <util/signalinterrupt.h>
#include <util/time.h>
#include <util/translation.h>
#include <validation.h>

#include <cassert>
#include <utility>

using kernel::CacheSizes;

namespace node {
ChainstateLoadResult LoadChainstate(ChainstateManager& chainman, const CacheSizes& cache_sizes,
                                    const ChainstateLoadOptions& options)
{
    if (!chainman.AssumedValidBlock().IsNull()) {
        LogInfo("Assuming ancestors of block %s have valid signatures.", chainman.AssumedValidBlock().GetHex());
    } else {
        LogInfo("Validating signatures for all blocks.");
    }
    LogInfo("Setting nMinimumChainWork=%s", chainman.MinimumChainWork().GetHex());
    if (chainman.MinimumChainWork() < UintToArith256(chainman.GetConsensus().nMinimumChainWork)) {
        LogWarning("nMinimumChainWork set below default value of %s", chainman.GetConsensus().nMinimumChainWork.GetHex());
    }
    if (chainman.m_blockman.GetPruneTarget() == BlockManager::PRUNE_TARGET_MANUAL) {
        LogInfo("Block pruning enabled. Use RPC call pruneblockchain(height) to manually prune block and undo files.");
    } else if (chainman.m_blockman.GetPruneTarget()) {
        LogInfo("Prune configured to target %u MiB on disk for block and undo files.",
                chainman.m_blockman.GetPruneTarget() / 1_MiB);
    }

    LOCK(cs_main);

    chainman.InitializeChainstate(options.mempool);

    if (chainman.m_interrupt) return {ChainstateLoadStatus::INTERRUPTED, {}};

    // LoadBlockIndex will load m_have_pruned if we've ever removed a
    // block file from disk.
    // Note that it also sets m_blockfiles_indexed based on the disk flag!
    if (!chainman.LoadBlockIndex()) {
        if (chainman.m_interrupt) return {ChainstateLoadStatus::INTERRUPTED, {}};
        return {ChainstateLoadStatus::FAILURE, _("Error loading block database")};
    }

    if (!chainman.BlockIndex().empty() &&
            !chainman.m_blockman.LookupBlockIndex(chainman.GetConsensus().hashGenesisBlock)) {
        // If the loaded chain has a wrong genesis, bail out immediately
        // (we're likely using a testnet datadir, or the other way around).
        return {ChainstateLoadStatus::FAILURE_INCOMPATIBLE_DB, _("Incorrect or no genesis block found. Wrong datadir for network?")};
    }

    // Genesis is never connected, so its validity need not reach BLOCK_VALID_CHAIN.
    for (const auto& [hash, block] : chainman.BlockIndex()) {
        if (block.nHeight > 1 && (block.nStatus & BLOCK_VALID_MASK) >= BLOCK_VALID_CHAIN &&
            (block.pprev->nStatus & BLOCK_VALID_MASK) < BLOCK_VALID_CHAIN) {
            return {ChainstateLoadStatus::FAILURE_INCOMPATIBLE_DB, _("Block index contains unfinished AssumeUTXO validation. Please restart with -reindex to rebuild the block index.")};
        }
    }

    // Check for changed -prune state.  What we are concerned about is a user who has pruned blocks
    // in the past, but is now trying to run unpruned.
    if (chainman.m_blockman.m_have_pruned && !options.prune) {
        return {ChainstateLoadStatus::FAILURE, _("You need to rebuild the database using -reindex to go back to unpruned mode.  This will redownload the entire blockchain")};
    }

    // At this point blocktree args are consistent with what's on disk.
    // If we're not mid-reindex (based on disk + args), add a genesis block on disk
    // (otherwise we use the one already on disk).
    // This is called again in ImportBlocks after the reindex completes.
    if (chainman.m_blockman.m_blockfiles_indexed && !chainman.LoadGenesisBlock()) {
        return {ChainstateLoadStatus::FAILURE, _("Error initializing block database")};
    }

    assert(cache_sizes.coins > 0);
    assert(cache_sizes.coins_db > 0);

    // At this point we're either in reindex or we've loaded a useful
    // block tree into BlockIndex()!

    Chainstate& chainstate{chainman.ActiveChainstate()};

    try {
        chainstate.InitCoinsDB(
            /*cache_size_bytes=*/cache_sizes.coins_db,
            /*in_memory=*/options.coins_db_in_memory,
            /*should_wipe=*/options.wipe_chainstate_db);
    } catch (dbwrapper_error& err) {
        LogError("%s\n", err.what());
        return {ChainstateLoadStatus::FAILURE, _("Error opening coins database")};
    }

    if (options.coins_error_cb) {
        chainstate.CoinsErrorCatcher().AddReadErrCallback(options.coins_error_cb);
    }

    // Refuse to load unsupported database format.
    // This is a no-op if we cleared the coinsviewdb with -reindex or -reindex-chainstate
    if (chainstate.CoinsDB().NeedsUpgrade()) {
        return {ChainstateLoadStatus::FAILURE_INCOMPATIBLE_DB, _("Unsupported chainstate database format found. "
                                                                 "Please restart with -reindex-chainstate. This will "
                                                                 "rebuild the chainstate database.")};
    }

    // ReplayBlocks is a no-op if we cleared the coinsviewdb with -reindex or -reindex-chainstate
    if (!chainstate.ReplayBlocks()) {
        return {ChainstateLoadStatus::FAILURE, _("Unable to replay blocks. You will need to rebuild the database using -reindex-chainstate.")};
    }

    // The on-disk coinsdb is now in a good state, create the cache
    chainstate.InitCoinsCache(cache_sizes.coins);
    assert(chainstate.CanFlushToDisk());

    if (!options.wipe_chainstate_db && !chainstate.CoinsTip().GetBestBlock().IsNull()) {
        // LoadChainTip initializes the chain based on CoinsTip()'s best block
        if (!chainstate.LoadChainTip()) {
            return {ChainstateLoadStatus::FAILURE, _("Error initializing block database")};
        }
        assert(chainstate.m_chain.Tip() != nullptr);
    }

    // Populate setBlockIndexCandidates after LoadChainTip() has finished
    // modifying nSequenceId. Because nSequenceId is used in the
    // set's comparator, changing it while blocks are in the set would be UB.
    chainstate.PopulateBlockIndexCandidates();

    if (chainstate.NeedsRedownload()) {
        return {ChainstateLoadStatus::FAILURE, strprintf(_("Witness data for blocks after height %d requires validation. Please restart with -reindex."),
                                                         chainman.GetConsensus().SegwitHeight)};
    };

    return {ChainstateLoadStatus::SUCCESS, {}};
}

ChainstateLoadResult VerifyLoadedChainstate(ChainstateManager& chainman, const ChainstateLoadOptions& options)
{
    LOCK(cs_main);

    Chainstate& chainstate{chainman.ActiveChainstate()};
    if (!options.wipe_chainstate_db && !chainstate.CoinsTip().GetBestBlock().IsNull()) {
        const CBlockIndex* tip = chainstate.m_chain.Tip();
        if (tip && tip->nTime > GetTime() + MAX_FUTURE_BLOCK_TIME) {
            return {ChainstateLoadStatus::FAILURE, _("The block database contains a block which appears to be from the future. "
                                                     "This may be due to your computer's date and time being set incorrectly. "
                                                     "Only rebuild the block database if you are sure that your computer's date and time are correct")};
        }

        VerifyDBResult result = CVerifyDB(chainman.GetNotifications()).VerifyDB(
            chainstate, chainman.GetConsensus(), chainstate.CoinsDB(),
            options.check_level,
            options.check_blocks);
        switch (result) {
        case VerifyDBResult::SUCCESS:
        case VerifyDBResult::SKIPPED_MISSING_BLOCKS:
            break;
        case VerifyDBResult::INTERRUPTED:
            return {ChainstateLoadStatus::INTERRUPTED, _("Block verification was interrupted")};
        case VerifyDBResult::CORRUPTED_BLOCK_DB:
            return {ChainstateLoadStatus::FAILURE, _("Corrupted block database detected")};
        case VerifyDBResult::SKIPPED_L3_CHECKS:
            if (options.require_full_verification) {
                return {ChainstateLoadStatus::FAILURE_INSUFFICIENT_DBCACHE, _("Insufficient dbcache for block verification")};
            }
            break;
        } // no default case, so the compiler can warn about missing cases
    }

    return {ChainstateLoadStatus::SUCCESS, {}};
}
} // namespace node
