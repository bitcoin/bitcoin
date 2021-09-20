// Copyright (c) 2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <node/chainstate.h>

#include <chainparams.h> // for CChainParams
#include <deploymentstatus.h> // for DeploymentActiveAfter
#include <rpc/blockchain.h> // for RPCNotifyBlockChange
#include <util/time.h> // for GetTime
#include <util/translation.h> // for bilingual_str
#include <node/blockstorage.h> // for CleanupBlockRevFiles, fHavePruned, fReindex
#include <node/context.h> // for NodeContext
#include <node/ui_interface.h> // for InitError, uiInterface, and CClientUIInterface member access
#include <shutdown.h> // for ShutdownRequested
#include <validation.h> // for a lot of things

#include <evo/chainhelper.h> // for CChainstateHelper
#include <evo/creditpool.h> // for CCreditPoolManager
#include <evo/deterministicmns.h> // for CDeterministicMNManager
#include <evo/evodb.h> // for CEvoDB
#include <evo/mnhftx.h> // for CMNHFManager
#include <llmq/chainlocks.h> // for llmq::chainLocksHandler
#include <llmq/context.h> // for LLMQContext
#include <llmq/instantsend.h> // for llmq::quorumInstantSendManager
#include <llmq/snapshot.h> // for llmq::quorumSnapshotManager

bool LoadChainstate(bool& fLoaded,
                    bilingual_str& strLoadError,
                    bool fReset,
                    ChainstateManager& chainman,
                    NodeContext& node,
                    bool fPruneMode,
                    bool is_governance_enabled,
                    const CChainParams& chainparams,
                    const ArgsManager& args,
                    bool fReindexChainState,
                    int64_t nBlockTreeDBCache,
                    int64_t nCoinDBCache,
                    int64_t nCoinCacheUsage) {
    auto is_coinsview_empty = [&](CChainState* chainstate) EXCLUSIVE_LOCKS_REQUIRED(::cs_main) {
        return fReset || fReindexChainState || chainstate->CoinsTip().GetBestBlock().IsNull();
    };

    do {
        try {
            LOCK(cs_main);

            int64_t nEvoDbCache{64 * 1024 * 1024}; // TODO
            node.evodb.reset();
            node.evodb = std::make_unique<CEvoDB>(nEvoDbCache, false, fReset || fReindexChainState);

            node.mnhf_manager.reset();
            node.mnhf_manager = std::make_unique<CMNHFManager>(*node.evodb);

            chainman.InitializeChainstate(Assert(node.mempool.get()), *node.evodb, node.chain_helper, llmq::chainLocksHandler, llmq::quorumInstantSendManager);
            chainman.m_total_coinstip_cache = nCoinCacheUsage;
            chainman.m_total_coinsdb_cache = nCoinDBCache;

            auto& pblocktree{chainman.m_blockman.m_block_tree_db};
            // new CBlockTreeDB tries to delete the existing file, which
            // fails if it's still open from the previous loop. Close it first:
            pblocktree.reset();
            pblocktree.reset(new CBlockTreeDB(nBlockTreeDBCache, false, fReset));

            // Same logic as above with pblocktree
            node.dmnman.reset();
            node.dmnman = std::make_unique<CDeterministicMNManager>(chainman.ActiveChainstate(), *node.evodb);
            node.mempool->ConnectManagers(node.dmnman.get());

            node.cpoolman.reset();
            node.cpoolman = std::make_unique<CCreditPoolManager>(*node.evodb);

            llmq::quorumSnapshotManager.reset();
            llmq::quorumSnapshotManager.reset(new llmq::CQuorumSnapshotManager(*node.evodb));

            if (node.llmq_ctx) {
                node.llmq_ctx->Interrupt();
                node.llmq_ctx->Stop();
            }
            node.llmq_ctx.reset();
            node.llmq_ctx = std::make_unique<LLMQContext>(chainman, *node.dmnman, *node.evodb, *node.mn_metaman, *node.mnhf_manager, *node.sporkman,
                                                          *node.mempool, node.mn_activeman.get(), *node.mn_sync, /*unit_tests=*/false, /*wipe=*/fReset || fReindexChainState);
            // Enable CMNHFManager::{Process, Undo}Block
            node.mnhf_manager->ConnectManagers(node.chainman.get(), node.llmq_ctx->qman.get());

            node.chain_helper.reset();
            node.chain_helper = std::make_unique<CChainstateHelper>(*node.cpoolman, *node.dmnman, *node.mnhf_manager, *node.govman, *(node.llmq_ctx->quorum_block_processor), *node.chainman,
                                                                    chainparams.GetConsensus(), *node.mn_sync, *node.sporkman, *(node.llmq_ctx->clhandler), *(node.llmq_ctx->qman));

            if (fReset) {
                pblocktree->WriteReindexing(true);
                //If we're reindexing in prune mode, wipe away unusable block files and all undo data files
                if (fPruneMode)
                    CleanupBlockRevFiles();
            }

            if (ShutdownRequested()) break;

            // LoadBlockIndex will load m_have_pruned if we've ever removed a
            // block file from disk.
            // Note that it also sets fReindex based on the disk flag!
            // From here on out fReindex and fReset mean something different!
            if (!chainman.LoadBlockIndex()) {
                if (ShutdownRequested()) break;
                strLoadError = _("Error loading block database");
                break;
            }

            if (is_governance_enabled && !args.GetBoolArg("-txindex", DEFAULT_TXINDEX) && chainparams.NetworkIDString() != CBaseChainParams::REGTEST) { // TODO remove this when pruning is fixed. See https://github.com/dashpay/dash/pull/1817 and https://github.com/dashpay/dash/pull/1743
                return InitError(_("Transaction index can't be disabled with governance validation enabled. Either start with -disablegovernance command line switch or enable transaction index."));
            }

            // If the loaded chain has a wrong genesis, bail out immediately
            // (we're likely using a testnet datadir, or the other way around).
            if (!chainman.BlockIndex().empty() &&
                    !chainman.m_blockman.LookupBlockIndex(chainparams.GetConsensus().hashGenesisBlock)) {
                return InitError(_("Incorrect or no genesis block found. Wrong datadir for network?"));
            }

            if (!chainparams.GetConsensus().hashDevnetGenesisBlock.IsNull() && !chainman.BlockIndex().empty() &&
                    !chainman.m_blockman.LookupBlockIndex(chainparams.GetConsensus().hashDevnetGenesisBlock)) {
                return InitError(_("Incorrect or no devnet genesis block found. Wrong datadir for devnet specified?"));
            }

            if (!fReset && !fReindexChainState) {
                // Check for changed -addressindex state
                if (!fAddressIndex && fAddressIndex != args.GetBoolArg("-addressindex", DEFAULT_ADDRESSINDEX)) {
                    strLoadError = _("You need to rebuild the database using -reindex to enable -addressindex");
                    break;
                }

                // Check for changed -timestampindex state
                if (!fTimestampIndex && fTimestampIndex != args.GetBoolArg("-timestampindex", DEFAULT_TIMESTAMPINDEX)) {
                    strLoadError = _("You need to rebuild the database using -reindex to enable -timestampindex");
                    break;
                }

                // Check for changed -spentindex state
                if (!fSpentIndex && fSpentIndex != args.GetBoolArg("-spentindex", DEFAULT_SPENTINDEX)) {
                    strLoadError = _("You need to rebuild the database using -reindex to enable -spentindex");
                    break;
                }
            }

            chainman.InitAdditionalIndexes();

            LogPrintf("%s: address index %s\n", __func__, fAddressIndex ? "enabled" : "disabled");
            LogPrintf("%s: timestamp index %s\n", __func__, fTimestampIndex ? "enabled" : "disabled");
            LogPrintf("%s: spent index %s\n", __func__, fSpentIndex ? "enabled" : "disabled");

            // Check for changed -prune state.  What we are concerned about is a user who has pruned blocks
            // in the past, but is now trying to run unpruned.
            if (chainman.m_blockman.m_have_pruned && !fPruneMode) {
                strLoadError = _("You need to rebuild the database using -reindex to go back to unpruned mode.  This will redownload the entire blockchain");
                break;
            }

            // At this point blocktree args are consistent with what's on disk.
            // If we're not mid-reindex (based on disk + args), add a genesis block on disk
            // (otherwise we use the one already on disk).
            // This is called again in ThreadImport after the reindex completes.
            if (!fReindex && !chainman.ActiveChainstate().LoadGenesisBlock()) {
                strLoadError = _("Error initializing block database");
                break;
            }

            // At this point we're either in reindex or we've loaded a useful
            // block tree into BlockIndex()!

            bool failed_chainstate_init = false;

            for (CChainState* chainstate : chainman.GetAll()) {
                chainstate->InitCoinsDB(
                    /* cache_size_bytes */ nCoinDBCache,
                    /* in_memory */ false,
                    /* should_wipe */ fReset || fReindexChainState);

                chainstate->CoinsErrorCatcher().AddReadErrCallback([]() {
                    uiInterface.ThreadSafeMessageBox(
                        _("Error reading from database, shutting down."),
                        "", CClientUIInterface::MSG_ERROR);
                });

                // If necessary, upgrade from older database format.
                // This is a no-op if we cleared the coinsviewdb with -reindex or -reindex-chainstate
                if (!chainstate->CoinsDB().Upgrade()) {
                    strLoadError = _("Error upgrading chainstate database");
                    failed_chainstate_init = true;
                    break;
                }

                // ReplayBlocks is a no-op if we cleared the coinsviewdb with -reindex or -reindex-chainstate
                if (!chainstate->ReplayBlocks()) {
                    strLoadError = _("Unable to replay blocks. You will need to rebuild the database using -reindex-chainstate.");
                    failed_chainstate_init = true;
                    break;
                }

                // The on-disk coinsdb is now in a good state, create the cache
                chainstate->InitCoinsCache(nCoinCacheUsage);
                assert(chainstate->CanFlushToDisk());

                // flush evodb
                // TODO: CEvoDB instance should probably be a part of CChainState
                // (for multiple chainstates to actually work in parallel)
                // and not a global
                if (&chainman.ActiveChainstate() == chainstate && !node.evodb->CommitRootTransaction()) {
                    strLoadError = _("Failed to commit EvoDB");
                    failed_chainstate_init = true;
                    break;
                }

                if (!is_coinsview_empty(chainstate)) {
                    // LoadChainTip initializes the chain based on CoinsTip()'s best block
                    if (!chainstate->LoadChainTip()) {
                        strLoadError = _("Error initializing block database");
                        failed_chainstate_init = true;
                        break; // out of the per-chainstate loop
                    }
                    assert(chainstate->m_chain.Tip() != nullptr);
                }
            }

            if (failed_chainstate_init) {
                break; // out of the chainstate activation do-while
            }

            if (!node.dmnman->MigrateDBIfNeeded()) {
                strLoadError = _("Error upgrading evo database");
                break;
            }
            if (!node.dmnman->MigrateDBIfNeeded2()) {
                strLoadError = _("Error upgrading evo database");
                break;
            }
            if (!node.mnhf_manager->ForceSignalDBUpdate()) {
                strLoadError = _("Error upgrading evo database for EHF");
                break;
            }
        } catch (const std::exception& e) {
            LogPrintf("%s\n", e.what());
            strLoadError = _("Error opening block database");
            break;
        }

        bool failed_verification = false;

        try {
            LOCK(cs_main);

            for (CChainState* chainstate : chainman.GetAll()) {
                if (!is_coinsview_empty(chainstate)) {
                    uiInterface.InitMessage(_("Verifying blocks…").translated);
                    if (chainman.m_blockman.m_have_pruned && args.GetArg("-checkblocks", DEFAULT_CHECKBLOCKS) > MIN_BLOCKS_TO_KEEP) {
                        LogPrintf("Prune: pruned datadir may not have more than %d blocks; only checking available blocks\n",
                            MIN_BLOCKS_TO_KEEP);
                    }

                    const CBlockIndex* tip = chainstate->m_chain.Tip();
                    RPCNotifyBlockChange(tip);
                    if (tip && tip->nTime > GetTime() + MAX_FUTURE_BLOCK_TIME) {
                        strLoadError = _("The block database contains a block which appears to be from the future. "
                                "This may be due to your computer's date and time being set incorrectly. "
                                "Only rebuild the block database if you are sure that your computer's date and time are correct");
                        failed_verification = true;
                        break;
                    }
                    const bool v19active{DeploymentActiveAfter(tip, chainparams.GetConsensus(), Consensus::DEPLOYMENT_V19)};
                    if (v19active) {
                        bls::bls_legacy_scheme.store(false);
                        LogPrintf("%s: bls_legacy_scheme=%d\n", __func__, bls::bls_legacy_scheme.load());
                    }

                    if (!CVerifyDB().VerifyDB(
                            *chainstate, chainparams, chainstate->CoinsDB(),
                            *node.evodb,
                            args.GetArg("-checklevel", DEFAULT_CHECKLEVEL),
                            args.GetArg("-checkblocks", DEFAULT_CHECKBLOCKS))) {
                        strLoadError = _("Corrupted block database detected");
                        failed_verification = true;
                        break;
                    }

                    // VerifyDB() disconnects blocks which might result in us switching back to legacy.
                    // Make sure we use the right scheme.
                    if (v19active && bls::bls_legacy_scheme.load()) {
                        bls::bls_legacy_scheme.store(false);
                        LogPrintf("%s: bls_legacy_scheme=%d\n", __func__, bls::bls_legacy_scheme.load());
                    }

                    if (args.GetArg("-checklevel", DEFAULT_CHECKLEVEL) >= 3) {
                        chainstate->ResetBlockFailureFlags(nullptr);
                    }

                } else {
                    // TODO: CEvoDB instance should probably be a part of CChainState
                    // (for multiple chainstates to actually work in parallel)
                    // and not a global
                    if (&chainman.ActiveChainstate() == chainstate && !node.evodb->IsEmpty()) {
                        // EvoDB processed some blocks earlier but we have no blocks anymore, something is wrong
                        strLoadError = _("Error initializing block database");
                        failed_verification = true;
                        break;
                    }
                }
            }
        } catch (const std::exception& e) {
            LogPrintf("%s\n", e.what());
            strLoadError = _("Error opening block database");
            failed_verification = true;
            break;
        }

        if (!failed_verification) {
            fLoaded = true;
        }
    } while(false);
    return true;
}
