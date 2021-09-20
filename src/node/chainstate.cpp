// Copyright (c) 2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <node/chainstate.h>

#include <chainparams.h> // for CChainParams
#include <rpc/blockchain.h> // for RPCNotifyBlockChange
#include <util/time.h> // for GetTime
#include <util/translation.h> // for bilingual_str
#include <node/blockstorage.h> // for CleanupBlockRevFiles, fHavePruned, fReindex
#include <node/context.h> // for NodeContext
#include <node/ui_interface.h> // for InitError, uiInterface, and CClientUIInterface member access
#include <shutdown.h> // for ShutdownRequested
#include <validation.h> // for a lot of things

bool LoadChainstate(bool& fLoaded,
                    bilingual_str& strLoadError,
                    bool fReset,
                    ChainstateManager& chainman,
                    NodeContext& node,
                    bool fPruneMode,
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
            chainman.InitializeChainstate(Assert(node.mempool.get()));
            chainman.m_total_coinstip_cache = nCoinCacheUsage;
            chainman.m_total_coinsdb_cache = nCoinDBCache;

            UnloadBlockIndex(node.mempool.get(), chainman);

            auto& pblocktree{chainman.m_blockman.m_block_tree_db};
            // new CBlockTreeDB tries to delete the existing file, which
            // fails if it's still open from the previous loop. Close it first:
            pblocktree.reset();
            pblocktree.reset(new CBlockTreeDB(nBlockTreeDBCache, false, fReset));

            if (fReset) {
                pblocktree->WriteReindexing(true);
                //If we're reindexing in prune mode, wipe away unusable block files and all undo data files
                if (fPruneMode)
                    CleanupBlockRevFiles();
            }

            if (ShutdownRequested()) break;

            // LoadBlockIndex will load fHavePruned if we've ever removed a
            // block file from disk.
            // Note that it also sets fReindex based on the disk flag!
            // From here on out fReindex and fReset mean something different!
            if (!chainman.LoadBlockIndex()) {
                if (ShutdownRequested()) break;
                strLoadError = _("Error loading block database");
                break;
            }

            // If the loaded chain has a wrong genesis, bail out immediately
            // (we're likely using a testnet datadir, or the other way around).
            if (!chainman.BlockIndex().empty() &&
                    !chainman.m_blockman.LookupBlockIndex(chainparams.GetConsensus().hashGenesisBlock)) {
                return InitError(_("Incorrect or no genesis block found. Wrong datadir for network?"));
            }

            // Check for changed -prune state.  What we are concerned about is a user who has pruned blocks
            // in the past, but is now trying to run unpruned.
            if (fHavePruned && !fPruneMode) {
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
        } catch (const std::exception& e) {
            LogPrintf("%s\n", e.what());
            strLoadError = _("Error opening block database");
            break;
        }

        if (!fReset) {
            LOCK(cs_main);
            auto chainstates{chainman.GetAll()};
            if (std::any_of(chainstates.begin(), chainstates.end(),
                            [](const CChainState* cs) EXCLUSIVE_LOCKS_REQUIRED(cs_main) { return cs->NeedsRedownload(); })) {
                strLoadError = strprintf(_("Witness data for blocks after height %d requires validation. Please restart with -reindex."),
                                         chainparams.GetConsensus().SegwitHeight);
                break;
            }
        }

        bool failed_verification = false;

        try {
            LOCK(cs_main);

            for (CChainState* chainstate : chainman.GetAll()) {
                if (!is_coinsview_empty(chainstate)) {
                    uiInterface.InitMessage(_("Verifying blocks…").translated);
                    if (fHavePruned && args.GetIntArg("-checkblocks", DEFAULT_CHECKBLOCKS) > MIN_BLOCKS_TO_KEEP) {
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

                    if (!CVerifyDB().VerifyDB(
                            *chainstate, chainparams, chainstate->CoinsDB(),
                            args.GetIntArg("-checklevel", DEFAULT_CHECKLEVEL),
                            args.GetIntArg("-checkblocks", DEFAULT_CHECKBLOCKS))) {
                        strLoadError = _("Corrupted block database detected");
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
