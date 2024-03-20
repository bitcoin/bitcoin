// Copyright (c) 2021-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <node/chainstate.h>

#include <arith_uint256.h>
#include <chain.h>
#include <coins.h>
#include <consensus/params.h>
#include <logging.h>
#include <node/blockstorage.h>
#include <node/caches.h>
#include <sync.h>
#include <threadsafety.h>
#include <tinyformat.h>
#include <txdb.h>
#include <uint256.h>
#include <util/fs.h>
#include <util/signalinterrupt.h>
#include <util/time.h>
#include <util/overloaded.h>
#include <util/translation.h>
#include <validation.h>

#include <algorithm>
#include <atomic>
#include <cassert>
#include <limits>
#include <memory>
#include <vector>

using kernel::AbortFailure;
using kernel::FlushResult;
using kernel::Interrupted;
using kernel::InterruptResult;

namespace node {
// Complete initialization of chainstates after the initial call has been made
// to ChainstateManager::InitializeChainstate().
static FlushResult<InterruptResult, ChainstateLoadError> CompleteChainstateInitialization(
    ChainstateManager& chainman,
    const CacheSizes& cache_sizes,
    const ChainstateLoadOptions& options) EXCLUSIVE_LOCKS_REQUIRED(::cs_main)
{
    FlushResult<InterruptResult, ChainstateLoadError> result;
    auto& pblocktree{chainman.m_blockman.m_block_tree_db};
    // new BlockTreeDB tries to delete the existing file, which
    // fails if it's still open from the previous loop. Close it first:
    pblocktree.reset();
    pblocktree = std::make_unique<BlockTreeDB>(DBParams{
        .path = chainman.m_options.datadir / "blocks" / "index",
        .cache_bytes = static_cast<size_t>(cache_sizes.block_tree_db),
        .memory_only = options.block_tree_db_in_memory,
        .wipe_data = options.reindex,
        .options = chainman.m_options.block_tree_db});

    if (options.reindex) {
        pblocktree->WriteReindexing(true);
        //If we're reindexing in prune mode, wipe away unusable block files and all undo data files
        if (options.prune) {
            chainman.m_blockman.CleanupBlockRevFiles();
        }
    }

    if (chainman.m_interrupt) {
        result.Update(Interrupted{});
        return result;
    }

    // LoadBlockIndex will load m_have_pruned if we've ever removed a
    // block file from disk.
    // Note that it also sets fReindex global based on the disk flag!
    // From here on, fReindex and options.reindex values may be different!
    auto load_result{chainman.LoadBlockIndex() >> result};
    if (!load_result) {
        result.Update({util::Error{_("Error loading block database")}, ChainstateLoadError::FAILURE});
        return result;
    } else if (IsInterrupted(*load_result) || chainman.m_interrupt) {
        result.Update(Interrupted{});
        return result;
    }

    if (!chainman.BlockIndex().empty() &&
            !chainman.m_blockman.LookupBlockIndex(chainman.GetConsensus().hashGenesisBlock)) {
        // If the loaded chain has a wrong genesis, bail out immediately
        // (we're likely using a testnet datadir, or the other way around).
        result.Update({util::Error{_("Incorrect or no genesis block found. Wrong datadir for network?")}, ChainstateLoadError::FAILURE_INCOMPATIBLE_DB});
        return result;
    }

    // Check for changed -prune state.  What we are concerned about is a user who has pruned blocks
    // in the past, but is now trying to run unpruned.
    if (chainman.m_blockman.m_have_pruned && !options.prune) {
        result.Update({util::Error{_("You need to rebuild the database using -reindex to go back to unpruned mode.  This will redownload the entire blockchain")}, ChainstateLoadError::FAILURE});
        return result;
    }

    // At this point blocktree args are consistent with what's on disk.
    // If we're not mid-reindex (based on disk + args), add a genesis block on disk
    // (otherwise we use the one already on disk).
    // This is called again in ImportBlocks after the reindex completes.
    if (!fReindex && !chainman.ActiveChainstate().LoadGenesisBlock()) {
        result.Update({util::Error{_("Error initializing block database")}, ChainstateLoadError::FAILURE});
        return result;
    }

    auto is_coinsview_empty = [&](Chainstate* chainstate) EXCLUSIVE_LOCKS_REQUIRED(::cs_main) {
        return options.reindex || options.reindex_chainstate || chainstate->CoinsTip().GetBestBlock().IsNull();
    };

    assert(chainman.m_total_coinstip_cache > 0);
    assert(chainman.m_total_coinsdb_cache > 0);

    // Conservative value which is arbitrarily chosen, as it will ultimately be changed
    // by a call to `chainman.MaybeRebalanceCaches()`. We just need to make sure
    // that the sum of the two caches (40%) does not exceed the allowable amount
    // during this temporary initialization state.
    double init_cache_fraction = 0.2;

    // At this point we're either in reindex or we've loaded a useful
    // block tree into BlockIndex()!

    for (Chainstate* chainstate : chainman.GetAll()) {
        LogPrintf("Initializing chainstate %s\n", chainstate->ToString());

        chainstate->InitCoinsDB(
            /*cache_size_bytes=*/chainman.m_total_coinsdb_cache * init_cache_fraction,
            /*in_memory=*/options.coins_db_in_memory,
            /*should_wipe=*/options.reindex || options.reindex_chainstate);

        if (options.coins_error_cb) {
            chainstate->CoinsErrorCatcher().AddReadErrCallback(options.coins_error_cb);
        }

        // Refuse to load unsupported database format.
        // This is a no-op if we cleared the coinsviewdb with -reindex or -reindex-chainstate
        if (chainstate->CoinsDB().NeedsUpgrade()) {
            result.Update({util::Error{_("Unsupported chainstate database format found. "
                                         "Please restart with -reindex-chainstate. This will "
                                         "rebuild the chainstate database.")},
                                       ChainstateLoadError::FAILURE_INCOMPATIBLE_DB});
            return result;
        }

        // ReplayBlocks is a no-op if we cleared the coinsviewdb with -reindex or -reindex-chainstate
        if (!chainstate->ReplayBlocks()) {
            result.Update({util::Error{_("Unable to replay blocks. You will need to rebuild the database using -reindex-chainstate.")}, ChainstateLoadError::FAILURE});
            return result;
        }

        // The on-disk coinsdb is now in a good state, create the cache
        chainstate->InitCoinsCache(chainman.m_total_coinstip_cache * init_cache_fraction);
        assert(chainstate->CanFlushToDisk());

        if (!is_coinsview_empty(chainstate)) {
            // LoadChainTip initializes the chain based on CoinsTip()'s best block
            if (!chainstate->LoadChainTip()) {
                result.Update({util::Error{_("Error initializing block database")}, ChainstateLoadError::FAILURE});
                return result;
            }
            assert(chainstate->m_chain.Tip() != nullptr);
        }
    }

    if (!options.reindex) {
        auto chainstates{chainman.GetAll()};
        if (std::any_of(chainstates.begin(), chainstates.end(),
                        [](const Chainstate* cs) EXCLUSIVE_LOCKS_REQUIRED(cs_main) { return cs->NeedsRedownload(); })) {
            result.Update({util::Error{strprintf(_("Witness data for blocks after height %d requires validation. Please restart with -reindex."),
                                                 chainman.GetConsensus().SegwitHeight)}, ChainstateLoadError::FAILURE});
            return result;
        };
    }

    // Now that chainstates are loaded and we're able to flush to
    // disk, rebalance the coins caches to desired levels based
    // on the condition of each chainstate.
    // Ignore failure value, do not treat flush error as failure.
    chainman.MaybeRebalanceCaches() >> result;

    return result;
}

FlushResult<InterruptResult, ChainstateLoadError> LoadChainstate(ChainstateManager& chainman, const CacheSizes& cache_sizes,
                                                                 const ChainstateLoadOptions& options)
{
    FlushResult<InterruptResult, ChainstateLoadError> result;
    if (!chainman.AssumedValidBlock().IsNull()) {
        LogPrintf("Assuming ancestors of block %s have valid signatures.\n", chainman.AssumedValidBlock().GetHex());
    } else {
        LogPrintf("Validating signatures for all blocks.\n");
    }
    LogPrintf("Setting nMinimumChainWork=%s\n", chainman.MinimumChainWork().GetHex());
    if (chainman.MinimumChainWork() < UintToArith256(chainman.GetConsensus().nMinimumChainWork)) {
        LogPrintf("Warning: nMinimumChainWork set below default value of %s\n", chainman.GetConsensus().nMinimumChainWork.GetHex());
    }
    if (chainman.m_blockman.GetPruneTarget() == BlockManager::PRUNE_TARGET_MANUAL) {
        LogPrintf("Block pruning enabled.  Use RPC call pruneblockchain(height) to manually prune block and undo files.\n");
    } else if (chainman.m_blockman.GetPruneTarget()) {
        LogPrintf("Prune configured to target %u MiB on disk for block and undo files.\n", chainman.m_blockman.GetPruneTarget() / 1024 / 1024);
    }

    LOCK(cs_main);

    chainman.m_total_coinstip_cache = cache_sizes.coins;
    chainman.m_total_coinsdb_cache = cache_sizes.coins_db;

    // Load the fully validated chainstate.
    chainman.InitializeChainstate(options.mempool);

    // Load a chain created from a UTXO snapshot, if any exist.
    bool has_snapshot = chainman.DetectSnapshotChainstate();

    if (has_snapshot && (options.reindex || options.reindex_chainstate)) {
        LogPrintf("[snapshot] deleting snapshot chainstate due to reindexing\n");
        if (!chainman.DeleteSnapshotChainstate()) {
            result.Update({util::Error{Untranslated("Couldn't remove snapshot chainstate.")}, ChainstateLoadError::FAILURE_FATAL});
            return result;
        }
    }

    result.Update(CompleteChainstateInitialization(chainman, cache_sizes, options));
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
    FlushResult<void, AbortFailure> snapshot_result;
    auto snapshot_completion = chainman.MaybeCompleteSnapshotValidation(snapshot_result);
    // Ignore failure value, do not treat flush error as failure.
    snapshot_result >> result;

    if (snapshot_completion == SnapshotCompletionResult::SKIPPED) {
        // do nothing; expected case
    } else if (snapshot_completion == SnapshotCompletionResult::SUCCESS) {
        LogPrintf("[snapshot] cleaning up unneeded background chainstate, then reinitializing\n");
        if (!chainman.ValidatedSnapshotCleanup()) {
            result.Update({util::Error{Untranslated("Background chainstate cleanup failed unexpectedly.")}, ChainstateLoadError::FAILURE_FATAL});
            return result;
        }

        // Because ValidatedSnapshotCleanup() has torn down chainstates with
        // ChainstateManager::ResetChainstates(), reinitialize them here without
        // duplicating the blockindex work above.
        assert(chainman.GetAll().empty());
        assert(!chainman.IsSnapshotActive());
        assert(!chainman.IsSnapshotValidated());

        chainman.InitializeChainstate(options.mempool);

        // A reload of the block index is required to recompute setBlockIndexCandidates
        // for the fully validated chainstate.
        chainman.ActiveChainstate().ClearBlockIndexCandidates();

        auto result{CompleteChainstateInitialization(chainman, cache_sizes, options)};
        if (!result || IsInterrupted(*result)) {
            return result;
        }
    } else {
        result.Update({util::Error{_(
           "UTXO snapshot failed to validate. "
           "Restart to resume normal initial block download, or try loading a different snapshot.")},
           ChainstateLoadError::FAILURE});
        return result;
    }

    return result;
}

FlushResult<InterruptResult, ChainstateLoadError> VerifyLoadedChainstate(ChainstateManager& chainman, const ChainstateLoadOptions& options)
{
    auto is_coinsview_empty = [&](Chainstate* chainstate) EXCLUSIVE_LOCKS_REQUIRED(::cs_main) {
        return options.reindex || options.reindex_chainstate || chainstate->CoinsTip().GetBestBlock().IsNull();
    };

    LOCK(cs_main);

    FlushResult<InterruptResult, ChainstateLoadError> result;
    for (Chainstate* chainstate : chainman.GetAll()) {
        if (!is_coinsview_empty(chainstate)) {
            const CBlockIndex* tip = chainstate->m_chain.Tip();
            if (tip && tip->nTime > GetTime() + MAX_FUTURE_BLOCK_TIME) {
                result.Update({util::Error{_("The block database contains a block which appears to be from the future. "
                                             "This may be due to your computer's date and time being set incorrectly. "
                                             "Only rebuild the block database if you are sure that your computer's date and time are correct")},
                               ChainstateLoadError::FAILURE});
                return result;
            }

            auto verify_result{CVerifyDB(chainman.GetNotifications()).VerifyDB(
                *chainstate, chainman.GetConsensus(), chainstate->CoinsDB(),
                options.check_level,
                options.check_blocks) >> result};
            if (verify_result) {
                std::visit(util::Overloaded{
                    [&](Success) {},
                    [&](SkippedMissingBlocks) {},
                    [&](SkippedL3Checks) {
                        if (options.require_full_verification) {
                            result.Update({util::Error{_("Insufficient dbcache for block verification")}, ChainstateLoadError::FAILURE_INSUFFICIENT_DBCACHE});
                        }
                    },
                    [&](kernel::Interrupted) {
                       result.Update(Interrupted{});
                    }}, *verify_result);
            } else {
                result.Update({util::Error{_("Corrupted block database detected")}, ChainstateLoadError::FAILURE});
            }
        }
    }

    return result;
}
} // namespace node
