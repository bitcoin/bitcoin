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
#include <util/time.h>
#include <util/translation.h>
#include <validation.h>
// SYSCOIN
#include <services/assetconsensus.h>
#include <evo/evodb.h>
#include <evo/deterministicmns.h>
#include <llmq/quorums_init.h>
#include <governance/governance.h>

#include <algorithm>
#include <atomic>
#include <cassert>
#include <limits>
#include <memory>
#include <vector>

namespace node {
// Complete initialization of chainstates after the initial call has been made
// to ChainstateManager::InitializeChainstate().
static ChainstateLoadResult CompleteChainstateInitialization(
    ChainstateManager& chainman,
    const CacheSizes& cache_sizes,
    const ChainstateLoadOptions& options) EXCLUSIVE_LOCKS_REQUIRED(::cs_main)
{
    auto& pblocktree{chainman.m_blockman.m_block_tree_db};
    // SYSCOIN
    if(fAssetIndex) {
        LogPrintf("Asset Index enabled, allowing for an asset aware spending policy\n");
    }
    LogPrintf("Creating LLMQ and asset databases...\n");
    llmq::DestroyLLMQSystem();
    evoDb.reset();
    evoDb = std::make_unique<CEvoDB>(DBParams{
        .path = chainman.m_options.datadir / "evodb",
        .cache_bytes = static_cast<size_t>(cache_sizes.evo_db),
        .memory_only = options.block_tree_db_in_memory,
        .wipe_data = options.fReindexGeth,
        .options = chainman.m_options.block_tree_db});
    deterministicMNManager.reset();
    deterministicMNManager.reset(new CDeterministicMNManager(*evoDb));
    governance.reset();
    governance.reset(new CGovernanceManager(chainman));
    llmq::InitLLMQSystem(*evoDb, options.block_tree_db_in_memory, *options.connman, *options.banman, *options.peerman, chainman, options.fReindexGeth);
    passetdb.reset();
    passetdb = std::make_unique<CAssetDB>(DBParams{
        .path = chainman.m_options.datadir / "asset",
        .cache_bytes = static_cast<size_t>(cache_sizes.evo_db),
        .memory_only = options.block_tree_db_in_memory,
        .wipe_data = options.fReindexGeth,
        .options = chainman.m_options.block_tree_db});
    passetnftdb.reset();
    passetnftdb = std::make_unique<CAssetNFTDB>(DBParams{
        .path = chainman.m_options.datadir / "assetnft",
        .cache_bytes = static_cast<size_t>(cache_sizes.evo_db),
        .memory_only = options.block_tree_db_in_memory,
        .wipe_data = options.fReindexGeth,
        .options = chainman.m_options.block_tree_db});
    pnevmtxrootsdb.reset();
    pnevmtxrootsdb = std::make_unique<CNEVMTxRootsDB>(DBParams{
        .path = chainman.m_options.datadir / "nevmtxroots",
        .cache_bytes = static_cast<size_t>(cache_sizes.evo_db),
        .memory_only = options.block_tree_db_in_memory,
        .wipe_data = options.fReindexGeth,
        .options = chainman.m_options.block_tree_db});
    pnevmtxmintdb.reset();
    pnevmtxmintdb = std::make_unique<CNEVMMintedTxDB>(DBParams{
        .path = chainman.m_options.datadir / "nevmminttx",
        .cache_bytes = static_cast<size_t>(cache_sizes.evo_db),
        .memory_only = options.block_tree_db_in_memory,
        .wipe_data = options.fReindexGeth,
        .options = chainman.m_options.block_tree_db});
    pblockindexdb.reset();
    pblockindexdb = std::make_unique<CBlockIndexDB>(DBParams{
        .path = chainman.m_options.datadir / "dbblockindex",
        .cache_bytes = static_cast<size_t>(cache_sizes.evo_db),
        .memory_only = options.block_tree_db_in_memory,
        .wipe_data = options.fReindexGeth,
        .options = chainman.m_options.block_tree_db});
    if(!pblockindexdb->ReadLastKnownHeight(nLastKnownHeightOnStart))
        nLastKnownHeightOnStart = 0;
    // PoDA data cannot be deleted from disk on reindex because chain on disk does not have PoDA information to recreate it
    pnevmdatadb.reset();
    pnevmdatadb = std::make_unique<CNEVMDataDB>(DBParams{
        .path = chainman.m_options.datadir / "nevmdata",
        .cache_bytes = static_cast<size_t>(cache_sizes.coins_db),
        .memory_only = options.block_tree_db_in_memory,
        .wipe_data = false,
        .options = chainman.m_options.coins_db});

    pnevmdatadb->ClearZeroMPT();
    if (!evoDb->CommitRootTransaction()) {
        return {ChainstateLoadStatus::FAILURE, _("Failed to commit EvoDB")};
    }
    if (options.fReindexGeth && !evoDb->IsEmpty()) {
        // EvoDB processed some blocks earlier but we have no blocks anymore, something is wrong
        return {ChainstateLoadStatus::FAILURE, _("Error initializing block database")};
    }
    // new CBlockTreeDB tries to delete the existing file, which
    // fails if it's still open from the previous loop. Close it first:
    pblocktree.reset();
    pblocktree = std::make_unique<CBlockTreeDB>(DBParams{
        .path = chainman.m_options.datadir / "blocks" / "index",
        .cache_bytes = static_cast<size_t>(cache_sizes.block_tree_db),
        .memory_only = options.block_tree_db_in_memory,
        .wipe_data = options.reindex,
        .options = chainman.m_options.block_tree_db});

    if (options.reindex) {
        pblocktree->WriteReindexing(true);
        //If we're reindexing in prune mode, wipe away unusable block files and all undo data files
        if (options.prune) {
            CleanupBlockRevFiles();
        }
    }

    if (options.check_interrupt && options.check_interrupt()) return {ChainstateLoadStatus::INTERRUPTED, {}};

    // LoadBlockIndex will load m_have_pruned if we've ever removed a
    // block file from disk.
    // Note that it also sets fReindex global based on the disk flag!
    // From here on, fReindex and options.reindex values may be different!
    if (!chainman.LoadBlockIndex()) {
        if (options.check_interrupt && options.check_interrupt()) return {ChainstateLoadStatus::INTERRUPTED, {}};
        return {ChainstateLoadStatus::FAILURE, _("Error loading block database")};
    }

    if (!chainman.BlockIndex().empty() &&
            !chainman.m_blockman.LookupBlockIndex(chainman.GetConsensus().hashGenesisBlock)) {
        // If the loaded chain has a wrong genesis, bail out immediately
        // (we're likely using a testnet datadir, or the other way around).
        return {ChainstateLoadStatus::FAILURE_INCOMPATIBLE_DB, _("Incorrect or no genesis block found. Wrong datadir for network?")};
    }

    // Check for changed -prune state.  What we are concerned about is a user who has pruned blocks
    // in the past, but is now trying to run unpruned.
    if (chainman.m_blockman.m_have_pruned && !options.prune) {
        return {ChainstateLoadStatus::FAILURE, _("You need to rebuild the database using -reindex to go back to unpruned mode.  This will redownload the entire blockchain")};
    }

    // At this point blocktree args are consistent with what's on disk.
    // If we're not mid-reindex (based on disk + args), add a genesis block on disk
    // (otherwise we use the one already on disk).
    // This is called again in ThreadImport after the reindex completes.
    if (!fReindex && !chainman.ActiveChainstate().LoadGenesisBlock()) {
        return {ChainstateLoadStatus::FAILURE, _("Error initializing block database")};
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
    // SYSCOIN
    bool coinsViewEmpty = false;
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
            return {ChainstateLoadStatus::FAILURE_INCOMPATIBLE_DB, _("Unsupported chainstate database format found. "
                                                                     "Please restart with -reindex-chainstate. This will "
                                                                     "rebuild the chainstate database.")};
        }

        // ReplayBlocks is a no-op if we cleared the coinsviewdb with -reindex or -reindex-chainstate
        if (!chainstate->ReplayBlocks()) {
            return {ChainstateLoadStatus::FAILURE, _("Unable to replay blocks. You will need to rebuild the database using -reindex-chainstate.")};
        }

        // The on-disk coinsdb is now in a good state, create the cache
        chainstate->InitCoinsCache(chainman.m_total_coinstip_cache * init_cache_fraction);
        assert(chainstate->CanFlushToDisk());

        if (!is_coinsview_empty(chainstate)) {
            // LoadChainTip initializes the chain based on CoinsTip()'s best block
            if (!chainstate->LoadChainTip()) {
                return {ChainstateLoadStatus::FAILURE, _("Error initializing block database")};
            }
            assert(chainstate->m_chain.Tip() != nullptr);
        }
        // SYSCOIN
        else {
            coinsViewEmpty = true;
        }
    }

    if (!options.reindex) {
        auto chainstates{chainman.GetAll()};
        if (std::any_of(chainstates.begin(), chainstates.end(),
                        [](const Chainstate* cs) EXCLUSIVE_LOCKS_REQUIRED(cs_main) { return cs->NeedsRedownload(); })) {
            return {ChainstateLoadStatus::FAILURE, strprintf(_("Witness data for blocks after height %d requires validation. Please restart with -reindex."),
                                                             chainman.GetConsensus().SegwitHeight)};
        };
    }
    // if coinsview is empty we clear all SYS db's overriding anything we did before
    if(coinsViewEmpty && !options.fReindexGeth) {
        LogPrintf("coinsViewEmpty recreating LLMQ and asset databases\n");
        llmq::DestroyLLMQSystem();
        evoDb.reset();
        evoDb = std::make_unique<CEvoDB>(DBParams{
            .path = chainman.m_options.datadir / "evodb",
            .cache_bytes = static_cast<size_t>(cache_sizes.evo_db),
            .memory_only = options.block_tree_db_in_memory,
            .wipe_data = coinsViewEmpty,
            .options = chainman.m_options.block_tree_db});
        deterministicMNManager.reset();
        deterministicMNManager.reset(new CDeterministicMNManager(*evoDb));
        llmq::InitLLMQSystem(*evoDb, options.block_tree_db_in_memory, *options.connman, *options.banman, *options.peerman, chainman, coinsViewEmpty);
        passetdb.reset();
        passetdb = std::make_unique<CAssetDB>(DBParams{
            .path = chainman.m_options.datadir / "asset",
            .cache_bytes = static_cast<size_t>(cache_sizes.evo_db),
            .memory_only = options.block_tree_db_in_memory,
            .wipe_data = coinsViewEmpty,
            .options = chainman.m_options.block_tree_db});
        passetnftdb.reset();
        passetnftdb = std::make_unique<CAssetNFTDB>(DBParams{
            .path = chainman.m_options.datadir / "assetnft",
            .cache_bytes = static_cast<size_t>(cache_sizes.evo_db),
            .memory_only = options.block_tree_db_in_memory,
            .wipe_data = coinsViewEmpty,
            .options = chainman.m_options.block_tree_db});
        pnevmtxrootsdb.reset();
        pnevmtxrootsdb = std::make_unique<CNEVMTxRootsDB>(DBParams{
            .path = chainman.m_options.datadir / "nevmtxroots",
            .cache_bytes = static_cast<size_t>(cache_sizes.evo_db),
            .memory_only = options.block_tree_db_in_memory,
            .wipe_data = coinsViewEmpty,
            .options = chainman.m_options.block_tree_db});
        pnevmtxmintdb.reset();
        pnevmtxmintdb = std::make_unique<CNEVMMintedTxDB>(DBParams{
            .path = chainman.m_options.datadir / "nevmminttx",
            .cache_bytes = static_cast<size_t>(cache_sizes.evo_db),
            .memory_only = options.block_tree_db_in_memory,
            .wipe_data = coinsViewEmpty,
            .options = chainman.m_options.block_tree_db});
        pblockindexdb.reset();
        pblockindexdb = std::make_unique<CBlockIndexDB>(DBParams{
            .path = chainman.m_options.datadir / "dbblockindex",
            .cache_bytes = static_cast<size_t>(cache_sizes.evo_db),
            .memory_only = options.block_tree_db_in_memory,
            .wipe_data = coinsViewEmpty,
            .options = chainman.m_options.block_tree_db});
        pnevmdatadb.reset();
        pnevmdatadb = std::make_unique<CNEVMDataDB>(DBParams{
            .path = chainman.m_options.datadir / "nevmdata",
            .cache_bytes = static_cast<size_t>(cache_sizes.coins_db),
            .memory_only = options.block_tree_db_in_memory,
            .wipe_data = false,
            .options = chainman.m_options.coins_db});
        if (!evoDb->CommitRootTransaction()) {
            return {ChainstateLoadStatus::FAILURE, _("Failed to commit EvoDB")};
        }
        if (!evoDb->IsEmpty()) {
            // EvoDB processed some blocks earlier but we have no blocks anymore, something is wrong
            return {ChainstateLoadStatus::FAILURE, _("Error initializing block database")};
        }
    }

    // Now that chainstates are loaded and we're able to flush to
    // disk, rebalance the coins caches to desired levels based
    // on the condition of each chainstate.
    chainman.MaybeRebalanceCaches();

    return {ChainstateLoadStatus::SUCCESS, {}};
}

ChainstateLoadResult LoadChainstate(ChainstateManager& chainman, const CacheSizes& cache_sizes,
                                    const ChainstateLoadOptions& options)
{
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
    chainman.DetectSnapshotChainstate(options.mempool);

    auto [init_status, init_error] = CompleteChainstateInitialization(chainman, cache_sizes, options);
    if (init_status != ChainstateLoadStatus::SUCCESS) {
        return {init_status, init_error};
    }

    // If a snapshot chainstate was fully validated by a background chainstate during
    // the last run, detect it here and clean up the now-unneeded background
    // chainstate.
    //
    // Why is this cleanup done here (on subsequent restart) and not just when the
    // snapshot is actually validated? Because this entails unusual
    // filesystem operations to move leveldb data directories around, and that seems
    // too risky to do in the middle of normal runtime.
    auto snapshot_completion = chainman.MaybeCompleteSnapshotValidation();

    if (snapshot_completion == SnapshotCompletionResult::SKIPPED) {
        // do nothing; expected case
    } else if (snapshot_completion == SnapshotCompletionResult::SUCCESS) {
        LogPrintf("[snapshot] cleaning up unneeded background chainstate, then reinitializing\n");
        if (!chainman.ValidatedSnapshotCleanup()) {
            AbortNode("Background chainstate cleanup failed unexpectedly.");
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
        chainman.ActiveChainstate().UnloadBlockIndex();

        auto [init_status, init_error] = CompleteChainstateInitialization(chainman, cache_sizes, options);
        if (init_status != ChainstateLoadStatus::SUCCESS) {
            return {init_status, init_error};
        }
    } else {
        return {ChainstateLoadStatus::FAILURE, _(
           "UTXO snapshot failed to validate. "
           "Restart to resume normal initial block download, or try loading a different snapshot.")};
    }

    return {ChainstateLoadStatus::SUCCESS, {}};
}

ChainstateLoadResult VerifyLoadedChainstate(ChainstateManager& chainman, const ChainstateLoadOptions& options)
{
    auto is_coinsview_empty = [&](Chainstate* chainstate) EXCLUSIVE_LOCKS_REQUIRED(::cs_main) {
        return options.reindex || options.reindex_chainstate || chainstate->CoinsTip().GetBestBlock().IsNull();
    };

    LOCK(cs_main);

    for (Chainstate* chainstate : chainman.GetAll()) {
        if (!is_coinsview_empty(chainstate)) {
            const CBlockIndex* tip = chainstate->m_chain.Tip();
            if (tip && tip->nTime > GetTime() + MAX_FUTURE_BLOCK_TIME) {
                return {ChainstateLoadStatus::FAILURE, _("The block database contains a block which appears to be from the future. "
                                                         "This may be due to your computer's date and time being set incorrectly. "
                                                         "Only rebuild the block database if you are sure that your computer's date and time are correct")};
            }

            VerifyDBResult result = CVerifyDB().VerifyDB(
                *chainstate, chainman.GetConsensus(), chainstate->CoinsDB(),
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
    }

    return {ChainstateLoadStatus::SUCCESS, {}};
}
} // namespace node
