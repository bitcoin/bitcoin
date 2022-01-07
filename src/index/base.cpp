// Copyright (c) 2017-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>
#include <index/base.h>
#include <node/blockstorage.h>
#include <node/ui_interface.h>
#include <shutdown.h>
#include <tinyformat.h>
#include <util/syscall_sandbox.h>
#include <util/thread.h>
#include <util/translation.h>
#include <validation.h> // For g_chainman
#include <warnings.h>

constexpr uint8_t DB_BEST_BLOCK{'B'};

constexpr int64_t SYNC_LOG_INTERVAL = 30; // seconds
constexpr int64_t SYNC_LOCATOR_WRITE_INTERVAL = 30; // seconds

template <typename... Args>
static void FatalError(const char* fmt, const Args&... args)
{
    std::string strMessage = tfm::format(fmt, args...);
    SetMiscWarning(Untranslated(strMessage));
    LogPrintf("*** %s\n", strMessage);
    AbortError(_("A fatal internal error occurred, see debug.log for details"));
    StartShutdown();
}

BaseIndex::DB::DB(const fs::path& path, size_t n_cache_size, bool f_memory, bool f_wipe, bool f_obfuscate) :
    CDBWrapper(path, n_cache_size, f_memory, f_wipe, f_obfuscate)
{}

bool BaseIndex::DB::ReadBestBlock(CBlockLocator& locator) const
{
    bool success = Read(DB_BEST_BLOCK, locator);
    if (!success) {
        locator.SetNull();
    }
    return success;
}

void BaseIndex::DB::WriteBestBlock(CDBBatch& batch, const CBlockLocator& locator)
{
    batch.Write(DB_BEST_BLOCK, locator);
}

BaseIndex::~BaseIndex()
{
    Interrupt();
    Stop();
}

bool BaseIndex::Init()
{
    CBlockLocator locator;
    if (!GetDB().ReadBestBlock(locator)) {
        locator.SetNull();
    }

    LOCK(cs_main);
    CChainState& index_chainstate = m_chainman->getChainstateForIndexing();
    CChain& index_chain = index_chainstate.m_chain;

    if (locator.IsNull()) {
        m_best_block_index = nullptr;
    } else {
        m_best_block_index = index_chainstate.FindForkInGlobalIndex(locator);
    }
    m_synced = m_best_block_index.load() == index_chain.Tip();
    if (!m_synced) {
        bool prune_violation = false;
        if (!m_best_block_index) {
            // index is not built yet
            // make sure we have all block data back to the genesis
            const CBlockIndex* block = index_chain.Tip();
            while (block->pprev && (block->pprev->nStatus & BLOCK_HAVE_DATA)) {
                block = block->pprev;
            }
            prune_violation = block != index_chain.Genesis();
        }
        // in case the index has a best block set and is not fully synced
        // check if we have the required blocks to continue building the index
        else {
            const CBlockIndex* block_to_test = m_best_block_index.load();
            if (!index_chain.Contains(block_to_test)) {
                // if the bestblock is not part of the mainchain, find the fork
                // and make sure we have all data down to the fork
                block_to_test = index_chain.FindFork(block_to_test);
            }
            const CBlockIndex* block = index_chain.Tip();
            prune_violation = true;
            // check backwards from the tip if we have all block data until we reach the indexes bestblock
            while (block_to_test && block && (block->nStatus & BLOCK_HAVE_DATA)) {
                if (block_to_test == block) {
                    prune_violation = false;
                    break;
                }
                // block->pprev must exist at this point, since block_to_test is part of the chain
                // and thus must be encountered when going backwards from the tip
                assert(block->pprev);
                block = block->pprev;
            }
        }
        if (prune_violation) {
            return InitError(strprintf(Untranslated("%s best block of the index goes beyond pruned data. Please disable the index or reindex (which will download the whole blockchain again)"), GetName()));
        }
    }
    return true;
}

static const CBlockIndex* NextSyncBlock(const CBlockIndex* pindex_prev, CChain& chain) EXCLUSIVE_LOCKS_REQUIRED(cs_main)
{
    AssertLockHeld(cs_main);

    if (!pindex_prev) {
        return chain.Genesis();
    }

    const CBlockIndex* pindex = chain.Next(pindex_prev);
    if (pindex) {
        return pindex;
    }

    return chain.Next(chain.FindFork(pindex_prev));
}

void BaseIndex::ThreadSync()
{
    SetSyscallSandboxPolicy(SyscallSandboxPolicy::TX_INDEX);
    const CBlockIndex* pindex = m_best_block_index.load();
    if (!m_synced) {
        auto& consensus_params = Params().GetConsensus();

        int64_t last_log_time = 0;
        int64_t last_locator_write_time = 0;
        while (true) {
            if (m_interrupt) {
                m_best_block_index = pindex;
                // No need to handle errors in Commit. If it fails, the error will be already be
                // logged. The best way to recover is to continue, as index cannot be corrupted by
                // a missed commit to disk for an advanced index state.
                Commit();
                return;
            }

            {
                LOCK(cs_main);
                CChain& index_chain = m_chainman->getChainstateForIndexing().m_chain;
                const CBlockIndex* pindex_next = NextSyncBlock(pindex, index_chain);
                if (!pindex_next) {
                    m_best_block_index = pindex;
                    m_synced = true;
                    // No need to handle errors in Commit. See rationale above.
                    Commit();
                    break;
                }
                if (pindex_next->pprev != pindex && !Rewind(pindex, pindex_next->pprev)) {
                    FatalError("%s: Failed to rewind index %s to a previous chain tip",
                               __func__, GetName());
                    return;
                }
                pindex = pindex_next;
            }

            int64_t current_time = GetTime();
            if (last_log_time + SYNC_LOG_INTERVAL < current_time) {
                LogPrintf("Syncing %s with block chain from height %d\n",
                          GetName(), pindex->nHeight);
                last_log_time = current_time;
            }

            if (last_locator_write_time + SYNC_LOCATOR_WRITE_INTERVAL < current_time) {
                m_best_block_index = pindex;
                last_locator_write_time = current_time;
                // No need to handle errors in Commit. See rationale above.
                Commit();
            }

            CBlock block;
            if (!ReadBlockFromDisk(block, pindex, consensus_params)) {
                FatalError("%s: Failed to read block %s from disk",
                           __func__, pindex->GetBlockHash().ToString());
                return;
            }
            if (!WriteBlock(block, pindex)) {
                FatalError("%s: Failed to write block %s to index database",
                           __func__, pindex->GetBlockHash().ToString());
                return;
            }
        }
    }

    if (pindex) {
        LogPrintf("%s is enabled at height %d\n", GetName(), pindex->nHeight);
    } else {
        LogPrintf("%s is enabled\n", GetName());
    }
}

bool BaseIndex::Commit()
{
    CDBBatch batch(GetDB());
    if (!CommitInternal(batch) || !GetDB().WriteBatch(batch)) {
        return error("%s: Failed to commit latest %s state", __func__, GetName());
    }
    return true;
}

bool BaseIndex::CommitInternal(CDBBatch& batch)
{
    LOCK(cs_main);
    CChain& chain = m_chainman->getChainstateForIndexing().m_chain;
    GetDB().WriteBestBlock(batch, chain.GetLocator(m_best_block_index));
    return true;
}

bool BaseIndex::Rewind(const CBlockIndex* current_tip, const CBlockIndex* new_tip)
{
    assert(current_tip == m_best_block_index);
    assert(current_tip->GetAncestor(new_tip->nHeight) == new_tip);

    // In the case of a reorg, ensure persisted block locator is not stale.
    // Pruning has a minimum of 288 blocks-to-keep and getting the index
    // out of sync may be possible but a users fault.
    // In case we reorg beyond the pruned depth, ReadBlockFromDisk would
    // throw and lead to a graceful shutdown
    m_best_block_index = new_tip;
    if (!Commit()) {
        // If commit fails, revert the best block index to avoid corruption.
        m_best_block_index = current_tip;
        return false;
    }

    return true;
}

void BaseIndex::BlockConnected(const std::shared_ptr<const CBlock>& block, const CBlockIndex* pindex)
{
    return this->blockConnectedInternal(block, pindex, false);
}

void BaseIndex::BackgroundBlockConnected(
    const std::shared_ptr<const CBlock>& block,
    const CBlockIndex* pindex)
{
    this->blockConnectedInternal(block, pindex, true);
}

void BaseIndex::blockConnectedInternal(
    const std::shared_ptr<const CBlock>& block,
    const CBlockIndex* pindex,
    bool from_background)
{
    // Ignore BlockConnected signals until we have fully indexed the chain.
    if (!m_synced) {
        return;
    }

    bool bg_chainstates_in_use = m_chainman->hasBgChainstateInUse();

    // Did we receive this event from a chain for which we can be sure
    // that we have built indexes for all blocks beneath it?
    //
    // If we are receiving this event from an assumed-valid chain, then don't
    // perform certain checks and don't update the m_best_block_index marker.
    bool is_on_indexing_chain = !bg_chainstates_in_use || from_background;

    const CBlockIndex* best_block_index = m_best_block_index.load();
    if (!best_block_index) {
        if (pindex->nHeight != 0) {
            FatalError("%s: First block connected is not the genesis block (height=%d)",
                       __func__, pindex->nHeight);
            return;
        }
    } else if (is_on_indexing_chain) {
        // Ensure block connects to an ancestor of the current best block. This should be the case
        // most of the time, but may not be immediately after the sync thread catches up and sets
        // m_synced. Consider the case where there is a reorg and the blocks on the stale branch are
        // in the ValidationInterface queue backlog even after the sync thread has caught up to the
        // new chain tip. In this unlikely event, log a warning and let the queue clear.
        if (best_block_index->GetAncestor(pindex->nHeight - 1) != pindex->pprev) {
            LogPrintf("%s: WARNING: Block %s does not connect to an ancestor of " /* Continued */
                      "known best chain (tip=%s); not updating index\n",
                      __func__, pindex->GetBlockHash().ToString(),
                      best_block_index->GetBlockHash().ToString());
            return;
        }
        if (best_block_index != pindex->pprev && !Rewind(best_block_index, pindex->pprev)) {
            FatalError("%s: Failed to rewind index %s to a previous chain tip",
                       __func__, GetName());
            return;
        }
    } else {
        // Otherwise we're on an assumed-valid chain, so just ensure this block is
        // contained in the active chain.
        if (!m_chainman->ActiveChain().Contains(pindex)) {
            LogPrintf("%s: WARNING: Block %s is not found on the known best chain; " /* Continued */
                "not updating index\n",
                __func__, pindex->GetBlockHash().ToString());
            return;
        }
    }

    if (WriteBlock(*block, pindex)) {
        // Since blocks may be submitted to this indexer out of order (i.e.
        // earlier blocks from the background validation chainstate submitted
        // *after* more recent blocks from the assumed-valid chainstate), only
        // update m_best_block_index when pindex is on a chain where we can
        // reasonably say that all blocks behind it have been indexed; in other
        // words, when it's on a fully-validated chain.
        //
        // Once the background chain's use ends (i.e. we have validated the
        // assumed-valid chain), we will update this in lockstep with the active
        // chain and will have all historical index entries.
        if (is_on_indexing_chain) {
            m_best_block_index = pindex;
        }
    } else {
        FatalError("%s: Failed to write block %s to index",
                   __func__, pindex->GetBlockHash().ToString());
        return;
    }
}

void BaseIndex::ChainStateFlushed(const CBlockLocator& locator)
{
    if (!m_synced) {
        return;
    }

    // This checks that ChainStateFlushed callbacks are received after BlockConnected. The check may fail
    // immediately after the sync thread catches up and sets m_synced. Consider the case where
    // there is a reorg and the blocks on the stale branch are in the ValidationInterface queue
    // backlog even after the sync thread has caught up to the new chain tip. In this unlikely
    // event, log a warning and let the queue clear.
    const CBlockIndex* best_block_index = m_best_block_index.load();
    if (!m_chainman->ActiveChain().Contains(best_block_index)) {
        LogPrintf("%s: WARNING: index progress marker (%s) not on known best " /* Continued */
                  "chain; not writing marker\n",
                  __func__, best_block_index->GetBlockHash().ToString());
        return;
    }

    // No need to handle errors in Commit. If it fails, the error will be already be logged. The
    // best way to recover is to continue, as index cannot be corrupted by a missed commit to disk
    // for an advanced index state.
    Commit();
}

bool BaseIndex::BlockUntilSyncedToCurrentChain() const
{
    AssertLockNotHeld(cs_main);

    if (!m_synced) {
        return false;
    }

    {
        // Skip the queue-draining stuff if we know we're caught up with
        // m_chain.Tip().
        LOCK(cs_main);
        const CBlockIndex* chain_tip = m_chainman->ActiveTip();
        const CBlockIndex* best_block_index = m_best_block_index.load();
        if (best_block_index->GetAncestor(chain_tip->nHeight) == chain_tip) {
            return true;
        }
    }

    LogPrintf("%s: %s is catching up on block notifications\n", __func__, GetName());
    SyncWithValidationInterfaceQueue();
    return true;
}

void BaseIndex::Interrupt()
{
    m_interrupt();
}

bool BaseIndex::Start(ChainstateManager& chainman)
{
    m_chainman = &chainman;
    // Need to register this ValidationInterface before running Init(), so that
    // callbacks are not missed if Init sets m_synced to true.
    RegisterValidationInterface(this);
    if (!Init()) {
        return false;
    }

    m_thread_sync = std::thread(&util::TraceThread, GetName(), [this] { ThreadSync(); });
    return true;
}

void BaseIndex::Stop()
{
    UnregisterValidationInterface(this);

    if (m_thread_sync.joinable()) {
        m_thread_sync.join();
    }
}

IndexSummary BaseIndex::GetSummary() const
{
    IndexSummary summary{};
    summary.name = GetName();
    summary.synced = m_synced;
    summary.best_block_height = m_best_block_index ? m_best_block_index.load()->nHeight : 0;
    return summary;
}
