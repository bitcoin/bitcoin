// Copyright (c) 2017-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>
#include <index/base.h>
#include <interfaces/chain.h>
#include <kernel/chain.h>
#include <node/blockstorage.h>
#include <node/context.h>
#include <node/interface_ui.h>
#include <shutdown.h>
#include <tinyformat.h>
#include <util/system.h>
#include <util/thread.h>
#include <util/translation.h>
#include <validation.h>
#include <warnings.h>

#include <string>
#include <utility>

using node::PruneLockInfo;
using node::ReadBlockFromDisk;
using node::g_indexes_ready_to_sync;

constexpr uint8_t DB_BEST_BLOCK{'B'};

constexpr auto SYNC_LOG_INTERVAL{30s};
constexpr auto SYNC_LOCATOR_WRITE_INTERVAL{30s};

void BaseIndex::FatalErrorImpl(const std::string& message)
{
    AbortNode(message);
}

CBlockLocator GetLocator(interfaces::Chain& chain, const uint256& block_hash)
{
    CBlockLocator locator;
    bool found = chain.findBlock(block_hash, interfaces::FoundBlock().locator(locator));
    assert(found);
    assert(!locator.IsNull());
    return locator;
}

BaseIndex::DB::DB(const fs::path& path, size_t n_cache_size, bool f_memory, bool f_wipe, bool f_obfuscate) :
    CDBWrapper(path, n_cache_size, f_memory, f_wipe, f_obfuscate)
{}

CBlockLocator BaseIndex::DB::ReadBestBlock() const
{
    CBlockLocator locator;

    bool success = Read(DB_BEST_BLOCK, locator);
    if (!success) {
        locator.SetNull();
    }

    return locator;
}

void BaseIndex::DB::WriteBestBlock(CDBBatch& batch, const CBlockLocator& locator)
{
    batch.Write(DB_BEST_BLOCK, locator);
}

BaseIndex::BaseIndex(std::unique_ptr<interfaces::Chain> chain, std::string name)
    : m_chain{std::move(chain)}, m_name{std::move(name)} {}

BaseIndex::~BaseIndex()
{
    Interrupt();
    Stop();
}

bool BaseIndex::Init()
{
    const auto locator{GetDB().ReadBestBlock()};

    LOCK(cs_main);
    CChain& active_chain = m_chainstate->m_chain;
    if (locator.IsNull()) {
        SetBestBlockIndex(nullptr);
    } else {
        // Setting the best block to the locator's top block. If it is not part of the
        // best chain, we will rewind to the fork point during index sync
        const CBlockIndex* locator_index{m_chainstate->m_blockman.LookupBlockIndex(locator.vHave.at(0))};
        if (!locator_index) {
            return InitError(strprintf(Untranslated("%s: best block of the index not found. Please rebuild the index."), GetName()));
        }
        SetBestBlockIndex(locator_index);
    }

    // Note: this will latch to true immediately if the user starts up with an empty
    // datadir and an index enabled. If this is the case, indexation will happen solely
    // via `BlockConnected` signals until, possibly, the next restart.
    m_synced = m_best_block_index.load() == active_chain.Tip();

    // Skip pruning check if indexes are not ready to sync (because reindex-chainstate has wiped the chain).
    if (!m_synced && g_indexes_ready_to_sync) {
        bool prune_violation = false;
        if (!m_best_block_index) {
            // index is not built yet
            // make sure we have all block data back to the genesis
            prune_violation = m_chainstate->m_blockman.GetFirstStoredBlock(*active_chain.Tip()) != active_chain.Genesis();
        }
        // in case the index has a best block set and is not fully synced
        // check if we have the required blocks to continue building the index
        else {
            const CBlockIndex* block_to_test = m_best_block_index.load();
            if (!active_chain.Contains(block_to_test)) {
                // if the bestblock is not part of the mainchain, find the fork
                // and make sure we have all data down to the fork
                block_to_test = active_chain.FindFork(block_to_test);
            }
            const CBlockIndex* block = active_chain.Tip();
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

    if (const auto* pindex{chain.Next(pindex_prev)}) {
        return pindex;
    }

    // If there is no next block, we might be synced
    if (pindex_prev == chain.Tip()) {
        return nullptr;
    }

    // Since block is not in the chain, return the next block in the chain AFTER the last common ancestor.
    // Caller will be responsible for rewinding back to the common ancestor.
    return chain.Next(chain.FindFork(pindex_prev));
}

void BaseIndex::ThreadSync()
{
    // Wait for a possible reindex-chainstate to finish until continuing
    // with the index sync
    while (!g_indexes_ready_to_sync) {
        if (!m_interrupt.sleep_for(std::chrono::milliseconds(500))) return;
    }

    const CBlockIndex* pindex = m_best_block_index.load();
    if (!m_synced) {
        auto& consensus_params = Params().GetConsensus();

        auto last_log_time{NodeClock::now()};
        auto last_locator_write_time{last_log_time};
        while (true) {
            if (m_interrupt) {
                SetBestBlockIndex(pindex);
                // No need to handle errors in Commit. If it fails, the error will be already be
                // logged. The best way to recover is to continue, as index cannot be corrupted by
                // a missed commit to disk for an advanced index state.
                Commit();
                return;
            }

            {
                LOCK(cs_main);
                const CBlockIndex* pindex_next = NextSyncBlock(pindex, m_chainstate->m_chain);
                if (!pindex_next) {
                    SetBestBlockIndex(pindex);
                    // No need to handle errors in Commit. See rationale above.
                    Commit();
                    m_synced = true;
                    break;
                }
                if (pindex_next->pprev != pindex && !Rewind(pindex, pindex_next->pprev)) {
                    FatalError("%s: Failed to rewind index %s to a previous chain tip",
                               __func__, GetName());
                    return;
                }
                pindex = pindex_next;
            }

            CBlock block;
            interfaces::BlockInfo block_info = kernel::MakeBlockInfo(pindex);
            if (!ReadBlockFromDisk(block, pindex, consensus_params)) {
                FatalError("%s: Failed to read block %s from disk",
                           __func__, pindex->GetBlockHash().ToString());
                return;
            } else {
                block_info.data = &block;
            }
            if (!CustomAppend(block_info)) {
                FatalError("%s: Failed to write block %s to index database",
                           __func__, pindex->GetBlockHash().ToString());
                return;
            }

            auto current_time{NodeClock::now()};
            if (current_time - last_log_time >= SYNC_LOG_INTERVAL) {
                LogPrintf("Syncing %s with block chain from height %d\n",
                          GetName(), pindex->nHeight);
                last_log_time = current_time;
            }

            if (current_time - last_locator_write_time >= SYNC_LOCATOR_WRITE_INTERVAL) {
                SetBestBlockIndex(pindex);
                last_locator_write_time = current_time;
                // No need to handle errors in Commit. See rationale above.
                Commit();
            }
        }
    }

    if (pindex) {
        LogPrintf("%s is enabled at height %d\n", GetName(), pindex->nHeight);
    } else {
        LogPrintf("%s is enabled\n", GetName());
    }
}

void BaseIndex::Commit()
{
    // Don't commit anything if we haven't indexed any block yet
    // (this could happen if init is interrupted).
    bool ok = m_best_block_index != nullptr;
    if (ok) {
        // Don't commit if the index best block is not an ancestor of the chainstate's last flushed
        // block. Otherwise, after an unclean shutdown, the index could be
        // persisted ahead of a chainstate it can no longer roll back to, which
        // would corrupt indexes with state (e.g. coinstatsindex).
        const CBlockIndex* index_tip = m_best_block_index.load();
        const CBlockIndex* last_flushed = WITH_LOCK(::cs_main, return m_chainstate->GetLastFlushedBlock());
        if (!last_flushed || last_flushed->GetAncestor(index_tip->nHeight) != index_tip) {
            LogDebug(BCLog::COINDB, "Skipping commit, index is ahead of flushed chainstate (index height %d, last flush at height %d)\n",
                    index_tip->nHeight, last_flushed ? last_flushed->nHeight : -1);
            return;
        }
        CDBBatch batch(GetDB());
        ok = CustomCommit(batch);
        if (ok) {
            GetDB().WriteBestBlock(batch, GetLocator(*m_chain, m_best_block_index.load()->GetBlockHash()));
            ok = GetDB().WriteBatch(batch);
        }
    }
    if (!ok) {
        LogError("%s: Failed to commit latest %s state\n", __func__, GetName());
    }
}

void BaseIndex::WriteRewindLocator(CDBBatch& batch, const CBlockIndex* new_tip) const
{
    const auto locator{GetDB().ReadBestBlock()};
    if (locator.IsNull()) return;
    const CBlockIndex* committed{WITH_LOCK(::cs_main, return m_chainstate->m_blockman.LookupBlockIndex(locator.vHave.front()))};
    // The committed tip was an ancestor of a flushed block when it was written, so it and new_tip below it are in
    // the block index after a restart. A committed tip at or below new_tip needs no change.
    if (committed && committed->nHeight > new_tip->nHeight && committed->GetAncestor(new_tip->nHeight) == new_tip) {
        GetDB().WriteBestBlock(batch, GetLocator(*m_chain, new_tip->GetBlockHash()));
    }
}

bool BaseIndex::RewindBlockByBlock(const CBlockIndex* current_tip, const CBlockIndex* new_tip,
                                   const std::function<bool(const CBlockIndex* pindex, CDBBatch& batch)>& rewind_block)
{
    for (const CBlockIndex* pindex = current_tip; pindex != new_tip; pindex = pindex->pprev) {
        CDBBatch batch(GetDB());
        if (!rewind_block(pindex, batch)) return false;
        WriteRewindLocator(batch, pindex->pprev);
        if (!GetDB().WriteBatch(batch)) return false;
    }
    return true;
}

bool BaseIndex::Rewind(const CBlockIndex* current_tip, const CBlockIndex* new_tip)
{
    assert(current_tip->GetAncestor(new_tip->nHeight) == new_tip);

    if (!CustomRewind({current_tip->GetBlockHash(), current_tip->nHeight}, {new_tip->GetBlockHash(), new_tip->nHeight})) {
        return false;
    }

    // Don't commit here - the committed index state must never be ahead of the
    // flushed chainstate, otherwise unclean restarts would lead to index corruption.
    // Pruning has a minimum of 288 blocks-to-keep and getting the index
    // out of sync may be possible but a users fault.
    // In case we reorg beyond the pruned depth, ReadBlockFromDisk would
    // throw and lead to a graceful shutdown
    SetBestBlockIndex(new_tip);
    return true;
}

void BaseIndex::BlockConnected(const std::shared_ptr<const CBlock>& block, const CBlockIndex* pindex)
{
    if (!m_synced) {
        return;
    }

    const CBlockIndex* best_block_index = m_best_block_index.load();
    if (!best_block_index) {
        if (pindex->nHeight != 0) {
            FatalError("%s: First block connected is not the genesis block (height=%d)",
                       __func__, pindex->nHeight);
            return;
        }
    } else {
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
    }
    interfaces::BlockInfo block_info = kernel::MakeBlockInfo(pindex, block.get());
    if (CustomAppend(block_info)) {
        // Setting the best block index is intentionally the last step of this
        // function, so BlockUntilSyncedToCurrentChain callers waiting for the
        // best block index to be updated can rely on the block being fully
        // processed, and the index object being safe to delete.
        SetBestBlockIndex(pindex);
    } else {
        FatalError("%s: Failed to write block %s to index",
                   __func__, pindex->GetBlockHash().ToString());
        return;
    }
}

void BaseIndex::BlockDisconnected(const std::shared_ptr<const CBlock>& block, const CBlockIndex* pindex)
{
    if (!m_synced) {
        return;
    }

    const CBlockIndex* best_block_index = m_best_block_index.load();

    // Ignore stale-branch disconnect notifications that do not connect to the indexed chain.
    // We must check that pindex itself is on the indexed chain, not just that it shares
    // a parent — otherwise same-height siblings would incorrectly trigger a rewind.
    if (best_block_index && best_block_index->nHeight >= pindex->nHeight && pindex->pprev) {
        if (best_block_index->GetAncestor(pindex->nHeight) != pindex) {
            LogPrintf("%s: WARNING: Block %s is not on the indexed chain " /* Continued */
                      "(tip=%s); not updating index\n",
                      __func__, pindex->GetBlockHash().ToString(), best_block_index->GetBlockHash().ToString());
            return;
        }
        if (!Rewind(best_block_index, pindex->pprev)) {
            FatalError("%s: Failed to rewind %s to previous block after disconnect", __func__, GetName());
        }
    }
}

void BaseIndex::ChainStateFlushed(const CBlockLocator& locator)
{
    if (!m_synced) {
        return;
    }

    const uint256& locator_tip_hash = locator.vHave.front();
    const CBlockIndex* locator_tip_index;
    {
        LOCK(cs_main);
        locator_tip_index = m_chainstate->m_blockman.LookupBlockIndex(locator_tip_hash);
    }

    if (!locator_tip_index) {
        FatalError("%s: First block (hash=%s) in locator was not found",
                   __func__, locator_tip_hash.ToString());
        return;
    }

    // This checks that ChainStateFlushed callbacks are received after BlockConnected. The check may fail
    // immediately after the sync thread catches up and sets m_synced. Consider the case where
    // there is a reorg and the blocks on the stale branch are in the ValidationInterface queue
    // backlog even after the sync thread has caught up to the new chain tip. In this unlikely
    // event, log a warning and let the queue clear.
    const CBlockIndex* best_block_index = m_best_block_index.load();
    if (best_block_index->GetAncestor(locator_tip_index->nHeight) != locator_tip_index) {
        LogPrintf("%s: WARNING: Locator contains block (hash=%s) not on known best " /* Continued */
                  "chain (tip=%s); not writing index locator\n",
                  __func__, locator_tip_hash.ToString(),
                  best_block_index->GetBlockHash().ToString());
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
        const CBlockIndex* chain_tip = m_chainstate->m_chain.Tip();
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

bool BaseIndex::Start()
{
    // m_chainstate member gives indexing code access to node internals. It is
    // removed in followup https://github.com/bitcoin/bitcoin/pull/24230
    m_chainstate = &m_chain->context()->chainman->ActiveChainstate();
    // Need to register this ValidationInterface before running Init(), so that
    // callbacks are not missed if Init sets m_synced to true.
    RegisterValidationInterface(this);
    if (!Init()) return false;

    const CBlockIndex* index = m_best_block_index.load();
    if (!CustomInit(index ? std::make_optional(interfaces::BlockKey{index->GetBlockHash(), index->nHeight}) : std::nullopt)) {
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

void BaseIndex::SetBestBlockIndex(const CBlockIndex* block)
{
    assert(!m_chainstate->m_blockman.IsPruneMode() || AllowPrune());

    if (AllowPrune()) {
        PruneLockInfo prune_lock;
        prune_lock.height_first = block ? block->nHeight : 0;
        WITH_LOCK(::cs_main, m_chainstate->m_blockman.UpdatePruneLock(GetName(), prune_lock));
    }

    // Intentionally set m_best_block_index as the last step in this function,
    // after updating prune locks above, and after making any other references
    // to *this, so the BlockUntilSyncedToCurrentChain function (which checks
    // m_best_block_index as an optimization) can be used to wait for the last
    // BlockConnected notification and safely assume that prune locks are
    // updated and that the index object is safe to delete.
    m_best_block_index = block;
}
