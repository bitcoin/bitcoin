// Copyright (c) 2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <node/chainstate.h>

#include <chain.h>
#include <coins.h>
#include <chainparamsbase.h>
#include <consensus/params.h>
#include <deploymentstatus.h>
#include <node/blockstorage.h>
#include <sync.h>
#include <threadsafety.h>
#include <tinyformat.h>
#include <txdb.h>
#include <txmempool.h>
#include <uint256.h>
#include <util/translation.h>
#include <validation.h>

#include <bls/bls.h>
#include <evo/chainhelper.h>
#include <evo/deterministicmns.h>
#include <evo/evodb.h>
#include <evo/mnhftx.h>
#include <gsl/pointers.h>
#include <llmq/context.h>

#include <atomic>
#include <cassert>
#include <memory>
#include <vector>

namespace node {
std::optional<ChainstateLoadingError> LoadChainstate(bool fReset,
                                                     ChainstateManager& chainman,
                                                     CGovernanceManager& govman,
                                                     CMasternodeMetaMan& mn_metaman,
                                                     CMasternodeSync& mn_sync,
                                                     CSporkManager& sporkman,
                                                     chainlock::Chainlocks& chainlocks,
                                                     std::unique_ptr<CChainstateHelper>& chain_helper,
                                                     std::unique_ptr<CDeterministicMNManager>& dmnman,
                                                     std::unique_ptr<CEvoDB>& evodb,
                                                     std::unique_ptr<LLMQContext>& llmq_ctx,
                                                     CTxMemPool* mempool,
                                                     const fs::path& data_dir,
                                                     bool fPruneMode,
                                                     bool is_addrindex_enabled,
                                                     bool is_spentindex_enabled,
                                                     bool is_timeindex_enabled,
                                                     const Consensus::Params& consensus_params,
                                                     bool fReindexChainState,
                                                     int64_t nBlockTreeDBCache,
                                                     int64_t nCoinDBCache,
                                                     int64_t nCoinCacheUsage,
                                                     bool block_tree_db_in_memory,
                                                     bool coins_db_in_memory,
                                                     bool dash_dbs_in_memory,
                                                     int8_t bls_threads,
                                                     int64_t max_recsigs_age,
                                                     std::function<bool()> shutdown_requested,
                                                     std::function<void()> coins_error_cb)
{
    auto is_coinsview_empty = [&](CChainState* chainstate) EXCLUSIVE_LOCKS_REQUIRED(::cs_main) {
        return fReset || fReindexChainState || chainstate->CoinsTip().GetBestBlock().IsNull();
    };

    LOCK(cs_main);

    evodb.reset();
    evodb = std::make_unique<CEvoDB>(util::DbWrapperParams{.path = data_dir, .memory = dash_dbs_in_memory, .wipe = fReset || fReindexChainState});

    chainman.InitializeChainstate(mempool, *evodb, chain_helper);
    chainman.m_total_coinstip_cache = nCoinCacheUsage;
    chainman.m_total_coinsdb_cache = nCoinDBCache;

    auto& pblocktree{chainman.m_blockman.m_block_tree_db};
    // new CBlockTreeDB tries to delete the existing file, which
    // fails if it's still open from the previous loop. Close it first:
    pblocktree.reset();
    pblocktree.reset(new CBlockTreeDB(nBlockTreeDBCache, block_tree_db_in_memory, fReset));

    DashChainstateSetup(chainman, govman, mn_metaman, mn_sync, sporkman, chainlocks, chain_helper,
                        dmnman, *evodb, llmq_ctx, mempool, data_dir, dash_dbs_in_memory,
                        /*llmq_dbs_wipe=*/fReset || fReindexChainState, bls_threads, max_recsigs_age, consensus_params);

    if (fReset) {
        pblocktree->WriteReindexing(true);
        //If we're reindexing in prune mode, wipe away unusable block files and all undo data files
        if (fPruneMode)
            CleanupBlockRevFiles();
    }

    if (shutdown_requested && shutdown_requested()) return ChainstateLoadingError::SHUTDOWN_PROBED;

    // LoadBlockIndex will load m_have_pruned if we've ever removed a
    // block file from disk.
    // Note that it also sets fReindex based on the disk flag!
    // From here on out fReindex and fReset mean something different!
    if (!chainman.LoadBlockIndex()) {
        if (shutdown_requested && shutdown_requested()) return ChainstateLoadingError::SHUTDOWN_PROBED;
        return ChainstateLoadingError::ERROR_LOADING_BLOCK_DB;
    }

    if (!chainman.BlockIndex().empty() &&
            !chainman.m_blockman.LookupBlockIndex(consensus_params.hashGenesisBlock)) {
        return ChainstateLoadingError::ERROR_BAD_GENESIS_BLOCK;
    }

    if (!consensus_params.hashDevnetGenesisBlock.IsNull() && !chainman.BlockIndex().empty() &&
            !chainman.m_blockman.LookupBlockIndex(consensus_params.hashDevnetGenesisBlock)) {
        return ChainstateLoadingError::ERROR_BAD_DEVNET_GENESIS_BLOCK;
    }

    if (!fReset && !fReindexChainState) {
        // Check for changed -addressindex state
        if (!fAddressIndex && fAddressIndex != is_addrindex_enabled) {
            return ChainstateLoadingError::ERROR_ADDRIDX_NEEDS_REINDEX;
        }

        // Check for changed -timestampindex state
        if (!fTimestampIndex && fTimestampIndex != is_timeindex_enabled) {
            return ChainstateLoadingError::ERROR_TIMEIDX_NEEDS_REINDEX;
        }

        // Check for changed -spentindex state
        if (!fSpentIndex && fSpentIndex != is_spentindex_enabled) {
            return ChainstateLoadingError::ERROR_SPENTIDX_NEEDS_REINDEX;
        }
    }

    chainman.InitAdditionalIndexes();

    // Check for changed -prune state.  What we are concerned about is a user who has pruned blocks
    // in the past, but is now trying to run unpruned.
    if (chainman.m_blockman.m_have_pruned && !fPruneMode) {
        return ChainstateLoadingError::ERROR_PRUNED_NEEDS_REINDEX;
    }

    // At this point blocktree args are consistent with what's on disk.
    // If we're not mid-reindex (based on disk + args), add a genesis block on disk
    // (otherwise we use the one already on disk).
    // This is called again in ThreadImport after the reindex completes.
    if (!fReindex && !chainman.ActiveChainstate().LoadGenesisBlock()) {
        return ChainstateLoadingError::ERROR_LOAD_GENESIS_BLOCK_FAILED;
    }

    // At this point we're either in reindex or we've loaded a useful
    // block tree into BlockIndex()!

    for (CChainState* chainstate : chainman.GetAll()) {
        chainstate->InitCoinsDB(
            /*cache_size_bytes=*/nCoinDBCache,
            /*in_memory=*/coins_db_in_memory,
            /*should_wipe=*/fReset || fReindexChainState);

        if (coins_error_cb) {
            chainstate->CoinsErrorCatcher().AddReadErrCallback(coins_error_cb);
        }

        // Refuse to load unsupported database format.
        // This is a no-op if we cleared the coinsviewdb with -reindex or -reindex-chainstate
        if (chainstate->CoinsDB().NeedsUpgrade()) {
            return ChainstateLoadingError::ERROR_CHAINSTATE_UPGRADE_FAILED;
        }

        // ReplayBlocks is a no-op if we cleared the coinsviewdb with -reindex or -reindex-chainstate
        if (!chainstate->ReplayBlocks()) {
            return ChainstateLoadingError::ERROR_REPLAYBLOCKS_FAILED;
        }

        // The on-disk coinsdb is now in a good state, create the cache
        chainstate->InitCoinsCache(nCoinCacheUsage);
        assert(chainstate->CanFlushToDisk());

        // flush evodb
        // TODO: CEvoDB instance should probably be a part of CChainState
        // (for multiple chainstates to actually work in parallel)
        // and not a global
        if (&chainman.ActiveChainstate() == chainstate && !evodb->CommitRootTransaction()) {
            return ChainstateLoadingError::ERROR_COMMITING_EVO_DB;
        }

        if (!is_coinsview_empty(chainstate)) {
            // LoadChainTip initializes the chain based on CoinsTip()'s best block
            if (!chainstate->LoadChainTip()) {
                return ChainstateLoadingError::ERROR_LOADCHAINTIP_FAILED;
            }
            assert(chainstate->m_chain.Tip() != nullptr);
        }
    }

    if (!chain_helper->ehf_manager->ForceSignalDBUpdate()) {
        return ChainstateLoadingError::ERROR_UPGRADING_SIGNALS_DB;
    }

    // Check if nVersion-first migration is needed and perform it
    if (dmnman->IsMigrationRequired() && !dmnman->MigrateLegacyDiffs(chainman.ActiveChainstate().m_chain.Tip())) {
        return ChainstateLoadingError::ERROR_UPGRADING_EVO_DB;
    }

    return std::nullopt;
}

void DashChainstateSetup(ChainstateManager& chainman,
                         CGovernanceManager& govman,
                         CMasternodeMetaMan& mn_metaman,
                         CMasternodeSync& mn_sync,
                         CSporkManager& sporkman,
                         chainlock::Chainlocks& chainlocks,
                         std::unique_ptr<CChainstateHelper>& chain_helper,
                         std::unique_ptr<CDeterministicMNManager>& dmnman,
                         CEvoDB& evodb,
                         std::unique_ptr<LLMQContext>& llmq_ctx,
                         CTxMemPool* mempool,
                         const fs::path& data_dir,
                         bool llmq_dbs_in_memory,
                         bool llmq_dbs_wipe,
                         int8_t bls_threads,
                         int64_t max_recsigs_age,
                         const Consensus::Params& consensus_params)
{
    // Same logic as pblocktree
    dmnman.reset();
    dmnman = std::make_unique<CDeterministicMNManager>(evodb, mn_metaman);

    llmq_ctx.reset();
    llmq_ctx = std::make_unique<LLMQContext>(*dmnman, evodb, sporkman, chainlocks, *mempool, chainman, mn_sync,
                                             util::DbWrapperParams{.path = data_dir, .memory = llmq_dbs_in_memory, .wipe = llmq_dbs_wipe},
                                             bls_threads, max_recsigs_age);
    mempool->ConnectManagers(dmnman.get(), llmq_ctx->isman.get());
    chain_helper.reset();
    chain_helper = std::make_unique<CChainstateHelper>(evodb, *dmnman, govman, *(llmq_ctx->isman), *(llmq_ctx->quorum_block_processor),
                                                       *(llmq_ctx->qsnapman), chainman, consensus_params, mn_sync, sporkman, chainlocks,
                                                       *(llmq_ctx->qman));
}

void DashChainstateSetupClose(std::unique_ptr<CChainstateHelper>& chain_helper,
                              std::unique_ptr<CDeterministicMNManager>& dmnman,
                              std::unique_ptr<LLMQContext>& llmq_ctx,
                              CTxMemPool* mempool)

{
    chain_helper.reset();
    llmq_ctx.reset();
    mempool->DisconnectManagers();
    dmnman.reset();
}

std::optional<ChainstateLoadVerifyError> VerifyLoadedChainstate(ChainstateManager& chainman,
                                                                CEvoDB& evodb,
                                                                bool fReset,
                                                                bool fReindexChainState,
                                                                const Consensus::Params& consensus_params,
                                                                int check_blocks,
                                                                int check_level,
                                                                std::function<int64_t()> get_unix_time_seconds,
                                                                std::function<void(bool)> notify_bls_state)
{
    auto is_coinsview_empty = [&](CChainState* chainstate) EXCLUSIVE_LOCKS_REQUIRED(::cs_main) {
        return fReset || fReindexChainState || chainstate->CoinsTip().GetBestBlock().IsNull();
    };

    LOCK(cs_main);

    for (CChainState* chainstate : chainman.GetAll()) {
        if (!is_coinsview_empty(chainstate)) {
            const CBlockIndex* tip = chainstate->m_chain.Tip();
            if (tip && tip->nTime > get_unix_time_seconds() + MAX_FUTURE_BLOCK_TIME) {
                return ChainstateLoadVerifyError::ERROR_BLOCK_FROM_FUTURE;
            }
            const bool v19active{DeploymentActiveAfter(tip, consensus_params, Consensus::DEPLOYMENT_V19)};
            if (v19active) {
                bls::bls_legacy_scheme.store(false);
                if (notify_bls_state) notify_bls_state(bls::bls_legacy_scheme.load());
            }

            if (!CVerifyDB().VerifyDB(
                    *chainstate, consensus_params, chainstate->CoinsDB(),
                    evodb,
                    check_level,
                    check_blocks)) {
                return ChainstateLoadVerifyError::ERROR_CORRUPTED_BLOCK_DB;
            }

            // VerifyDB() disconnects blocks which might result in us switching back to legacy.
            // Make sure we use the right scheme.
            if (v19active && bls::bls_legacy_scheme.load()) {
                bls::bls_legacy_scheme.store(false);
                if (notify_bls_state) notify_bls_state(bls::bls_legacy_scheme.load());
            }

            if (check_level >= 3) {
                chainstate->ResetBlockFailureFlags(nullptr);
            }

        } else {
            // TODO: CEvoDB instance should probably be a part of CChainState
            // (for multiple chainstates to actually work in parallel)
            // and not a global
            if (&chainman.ActiveChainstate() == chainstate && !evodb.IsEmpty()) {
                // EvoDB processed some blocks earlier but we have no blocks anymore, something is wrong
                return ChainstateLoadVerifyError::ERROR_EVO_DB_SANITY_FAILED;
            }
        }
    }

    return std::nullopt;
}
} // namespace node
