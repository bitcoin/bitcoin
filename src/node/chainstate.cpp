// Copyright (c) 2021-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <node/chainstate.h>

#include <arith_uint256.h>
#include <chain.h>
#include <coins.h>
#include <consensus/params.h>
#include <kernel/caches.h>
#include <logging.h>
#include <node/blockstorage.h>
#include <sync.h>
#include <threadsafety.h>
#include <tinyformat.h>
#include <txdb.h>
#include <uint256.h>
#include <util/fs.h>
#include <util/signalinterrupt.h>
#include <util/time.h>
#include <util/translation.h>
#include <validation.h>

#include <algorithm>
#include <cassert>
#include <vector>

using kernel::CacheSizes;
using kernel::Interrupted;
using kernel::InterruptResult;

namespace node {
// Complete initialization of chainstates after the initial call has been made
// to ChainstateManager::InitializeChainstate().
static util::Result<InterruptResult, ChainstateLoadError> CompleteChainstateInitialization(
    ChainstateManager& chainman,
    const ChainstateLoadOptions& options) EXCLUSIVE_LOCKS_REQUIRED(::cs_main)
{
    if (chainman.m_interrupt) return Interrupted{};

    util::Result<InterruptResult, ChainstateLoadError> result;

    // LoadBlockIndex will load m_have_pruned if we've ever removed a
    // block file from disk.
    // Note that it also sets m_blockfiles_indexed based on the disk flag!
    auto load_result{chainman.LoadBlockIndex() >> result};
    if (!load_result) {
        result.update({util::Error{_("Error loading block database")}, ChainstateLoadError::FAILURE});
        return result;
    } else if (IsInterrupted(*load_result) || chainman.m_interrupt) {
        result.update(Interrupted{});
        return result;
    }

    if (!chainman.BlockIndex().empty() &&
            !chainman.m_blockman.LookupBlockIndex(chainman.GetConsensus().hashGenesisBlock)) {
        // If the loaded chain has a wrong genesis, bail out immediately
        // (we're likely using a testnet datadir, or the other way around).
        result.update({util::Error{_("Incorrect or no genesis block found. Wrong datadir for network?")}, ChainstateLoadError::FAILURE_INCOMPATIBLE_DB});
        return result;
    }

    // Check for changed -prune state.  What we are concerned about is a user who has pruned blocks
    // in the past, but is now trying to run unpruned.
    if (chainman.m_blockman.m_have_pruned && !options.prune) {
        result.update({util::Error{_("You need to rebuild the database using -reindex to go back to unpruned mode.  This will redownload the entire blockchain")}, ChainstateLoadError::FAILURE});
        return result;
    }

    // At this point blocktree args are consistent with what's on disk.
    // If we're not mid-reindex (based on disk + args), add a genesis block on disk
    // (otherwise we use the one already on disk).
    // This is called again in ImportBlocks after the reindex completes.
    if (chainman.m_blockman.m_blockfiles_indexed && !chainman.ActiveChainstate().LoadGenesisBlock()) {
        result.update({util::Error{_("Error initializing block database")}, ChainstateLoadError::FAILURE});
        return result;
    }

    auto is_coinsview_empty = [&](Chainstate& chainstate) EXCLUSIVE_LOCKS_REQUIRED(::cs_main) {
        return options.wipe_chainstate_db || chainstate.CoinsTip().GetBestBlock().IsNull();
    };

    assert(chainman.m_total_coinstip_cache > 0);
    assert(chainman.m_total_coinsdb_cache > 0);

    // If running with multiple chainstates, limit the cache sizes with a
    // discount factor. If discounted the actual cache size will be
    // recalculated by `chainman.MaybeRebalanceCaches()`. The discount factor
    // is conservatively chosen such that the sum of the caches does not exceed
    // the allowable amount during this temporary initialization state.
    double init_cache_fraction = chainman.HistoricalChainstate() ? 0.2 : 1.0;

    // At this point we're either in reindex or we've loaded a useful
    // block tree into BlockIndex()!

    for (const auto& chainstate : chainman.m_chainstates) {
        LogInfo("Initializing chainstate %s", chainstate->ToString());

        try {
            chainstate->InitCoinsDB(
                /*cache_size_bytes=*/chainman.m_total_coinsdb_cache * init_cache_fraction,
                /*in_memory=*/options.coins_db_in_memory,
                /*should_wipe=*/options.wipe_chainstate_db);
        } catch (dbwrapper_error& err) {
            LogError("%s\n", err.what());
            return {util::Error{_("Error opening coins database")}, ChainstateLoadError::FAILURE};
        }

        if (options.coins_error_cb) {
            chainstate->CoinsErrorCatcher().AddReadErrCallback(options.coins_error_cb);
        }

        // Refuse to load unsupported database format.
        // This is a no-op if we cleared the coinsviewdb with -reindex or -reindex-chainstate
        if (chainstate->CoinsDB().NeedsUpgrade()) {
            result.update({util::Error{_("Unsupported chainstate database format found. "
                                         "Please restart with -reindex-chainstate. This will "
                                         "rebuild the chainstate database.")},
                                       ChainstateLoadError::FAILURE_INCOMPATIBLE_DB});
            return result;
        }

        // ReplayBlocks is a no-op if we cleared the coinsviewdb with -reindex or -reindex-chainstate
        if (!chainstate->ReplayBlocks()) {
            result.update({util::Error{_("Unable to replay blocks. You will need to rebuild the database using -reindex-chainstate.")}, ChainstateLoadError::FAILURE});
            return result;
        }

        // The on-disk coinsdb is now in a good state, create the cache
        chainstate->InitCoinsCache(chainman.m_total_coinstip_cache * init_cache_fraction);
        assert(chainstate->CanFlushToDisk());

        if (!is_coinsview_empty(*chainstate)) {
            // LoadChainTip initializes the chain based on CoinsTip()'s best block
            if (!chainstate->LoadChainTip()) {
                result.update({util::Error{_("Error initializing block database")}, ChainstateLoadError::FAILURE});
                return result;
            }
            assert(chainstate->m_chain.Tip() != nullptr);
        }
    }

    const auto& chainstates{chainman.m_chainstates};
    if (std::any_of(chainstates.begin(), chainstates.end(),
                    [](const auto& cs) EXCLUSIVE_LOCKS_REQUIRED(cs_main) { return cs->NeedsRedownload(); })) {
        result.update({util::Error{strprintf(_("Witness data for blocks after height %d requires validation. Please restart with -reindex."),
                                             chainman.GetConsensus().SegwitHeight)}, ChainstateLoadError::FAILURE});
        return result;
    };

    // Now that chainstates are loaded and we're able to flush to
    // disk, rebalance the coins caches to desired levels based
    // on the condition of each chainstate.
    chainman.MaybeRebalanceCaches();

    return result;
}

util::Result<InterruptResult, ChainstateLoadError> LoadChainstate(ChainstateManager& chainman, const CacheSizes& cache_sizes,
                                                                  const ChainstateLoadOptions& options)
{
    util::Result<InterruptResult, ChainstateLoadError> result;
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
                chainman.m_blockman.GetPruneTarget() / 1024 / 1024);
    }

    LOCK(cs_main);

    chainman.m_total_coinstip_cache = cache_sizes.coins;
    chainman.m_total_coinsdb_cache = cache_sizes.coins_db;

    // Load the fully validated chainstate.
    Chainstate& validated_cs{chainman.InitializeChainstate(options.mempool)};

    // Load a chain created from a UTXO snapshot, if any exist.
    Chainstate* assumeutxo_cs{chainman.LoadAssumeutxoChainstate()};

    if (assumeutxo_cs && options.wipe_chainstate_db) {
        // Reset chainstate target to network tip instead of snapshot block.
        validated_cs.SetTargetBlock(nullptr);
        LogInfo("[snapshot] deleting snapshot chainstate due to reindexing");
        if (!chainman.DeleteChainstate(*assumeutxo_cs)) {
            result.update({util::Error{Untranslated("Couldn't remove snapshot chainstate.")}, ChainstateLoadError::FAILURE_FATAL});
            return result;
        }
        assumeutxo_cs = nullptr;
    }

    result.update(CompleteChainstateInitialization(chainman, options));
    if (!result || IsInterrupted(*result)) {
        return result;
    }

    // If a snapshot chainstate was fully validated by a background chainstate during
    // the last run, detect it here and clean up the now-unneeded background
    // chainstate.
    //
    // Why is this cleanup done here (on subsequent restart) and not just when the
    // snapshot is actually validated? Because this entails unusual
    // filesystem operations to move leveldb data directories around, and that seems
    // too risky to do in the middle of normal runtime.
    auto snapshot_completion{assumeutxo_cs
                             ? chainman.MaybeValidateSnapshot(validated_cs, *assumeutxo_cs)
                             : SnapshotCompletionResult::SKIPPED};

    if (snapshot_completion == SnapshotCompletionResult::SKIPPED) {
        // do nothing; expected case
    } else if (snapshot_completion == SnapshotCompletionResult::SUCCESS) {
        LogInfo("[snapshot] cleaning up unneeded background chainstate, then reinitializing");
        if (!chainman.ValidatedSnapshotCleanup(validated_cs, *assumeutxo_cs)) {
            result.update({util::Error{Untranslated("Background chainstate cleanup failed unexpectedly.")}, ChainstateLoadError::FAILURE_FATAL});
            return result;
        }

        // Because ValidatedSnapshotCleanup() has torn down chainstates with
        // ChainstateManager::ResetChainstates(), reinitialize them here without
        // duplicating the blockindex work above.
        assert(chainman.m_chainstates.empty());

        chainman.InitializeChainstate(options.mempool);

        // A reload of the block index is required to recompute setBlockIndexCandidates
        // for the fully validated chainstate.
        chainman.ActiveChainstate().ClearBlockIndexCandidates();

        auto result{CompleteChainstateInitialization(chainman, options)};
        if (!result || IsInterrupted(*result)) {
            return result;
        }
    } else {
        result.update({util::Error{_(
           "UTXO snapshot failed to validate. "
           "Restart to resume normal initial block download, or try loading a different snapshot.")},
           ChainstateLoadError::FAILURE_FATAL});
        return result;
    }

    return result;
}

util::Result<InterruptResult, ChainstateLoadError> VerifyLoadedChainstate(ChainstateManager& chainman, const ChainstateLoadOptions& options)
{
    auto is_coinsview_empty = [&](Chainstate& chainstate) EXCLUSIVE_LOCKS_REQUIRED(::cs_main) {
        return options.wipe_chainstate_db || chainstate.CoinsTip().GetBestBlock().IsNull();
    };

    LOCK(cs_main);

    util::Result<InterruptResult, ChainstateLoadError> result;
    for (auto& chainstate : chainman.m_chainstates) {
        if (!is_coinsview_empty(*chainstate)) {
            const CBlockIndex* tip = chainstate->m_chain.Tip();
            if (tip && tip->nTime > GetTime() + MAX_FUTURE_BLOCK_TIME) {
                result.update({util::Error{_("The block database contains a block which appears to be from the future. "
                                             "This may be due to your computer's date and time being set incorrectly. "
                                             "Only rebuild the block database if you are sure that your computer's date and time are correct")},
                               ChainstateLoadError::FAILURE});
                return result;
            }

            VerifyDBResult verify_result = CVerifyDB(chainman.GetNotifications()).VerifyDB(
                *chainstate, chainman.GetConsensus(), chainstate->CoinsDB(),
                options.check_level,
                options.check_blocks);
            switch (verify_result) {
            case VerifyDBResult::SUCCESS:
            case VerifyDBResult::SKIPPED_MISSING_BLOCKS:
                break;
            case VerifyDBResult::INTERRUPTED:
                result.update(Interrupted{});
                return result;
            case VerifyDBResult::CORRUPTED_BLOCK_DB:
                result.update({util::Error{_("Corrupted block database detected")}, ChainstateLoadError::FAILURE});
                return result;
            case VerifyDBResult::SKIPPED_L3_CHECKS:
                if (options.require_full_verification) {
                    result.update({util::Error{_("Insufficient dbcache for block verification")}, ChainstateLoadError::FAILURE_INSUFFICIENT_DBCACHE});
                    return result;
                }
                break;
            } // no default case, so the compiler can warn about missing cases
        }
    }

    return result;
}
} // namespace node
