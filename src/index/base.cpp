// Copyright (c) 2017-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>
#include <common/args.h>
#include <index/base.h>
#include <interfaces/chain.h>
#include <kernel/chain.h>
#include <logging.h>
#include <node/abort.h>
#include <node/blockstorage.h>
#include <node/context.h>
#include <node/database_args.h>
#include <node/interface_ui.h>
#include <util/threadpool.h>
#include <tinyformat.h>
#include <undo.h>
#include <util/string.h>
#include <util/thread.h>
#include <util/translation.h>
#include <validation.h>

#include <algorithm>
#include <chrono>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>

constexpr uint8_t DB_BEST_BLOCK{'B'};

constexpr auto SYNC_LOG_INTERVAL{30s};
constexpr auto SYNC_LOCATOR_WRITE_INTERVAL{30s};

template <typename... Args>
void BaseIndex::FatalErrorf(util::ConstevalFormatString<sizeof...(Args)> fmt, const Args&... args)
{
    auto message = tfm::format(fmt, args...);
    node::AbortNode(m_chain->context()->shutdown_request, m_chain->context()->exit_status, Untranslated(message), m_chain->context()->warnings.get());
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
    CDBWrapper{DBParams{
        .path = path,
        .cache_bytes = n_cache_size,
        .memory_only = f_memory,
        .wipe_data = f_wipe,
        .obfuscate = f_obfuscate,
        .options = [] { DBOptions options; node::ReadDatabaseArgs(gArgs, options); return options; }()}}
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

BaseIndex::BaseIndex(std::unique_ptr<interfaces::Chain> chain, std::string name)
    : m_chain{std::move(chain)}, m_name{std::move(name)} {}

BaseIndex::~BaseIndex()
{
    Interrupt();
    Stop();
}

bool BaseIndex::Init()
{
    AssertLockNotHeld(cs_main);

    // May need reset if index is being restarted.
    m_interrupt.reset();

    // m_chainstate member gives indexing code access to node internals. It is
    // removed in followup https://github.com/bitcoin/bitcoin/pull/24230
    m_chainstate = WITH_LOCK(::cs_main,
        return &m_chain->context()->chainman->GetChainstateForIndexing());
    // Register to validation interface before setting the 'm_synced' flag, so that
    // callbacks are not missed once m_synced is true.
    m_chain->context()->validation_signals->RegisterValidationInterface(this);

    CBlockLocator locator;
    if (!GetDB().ReadBestBlock(locator)) {
        locator.SetNull();
    }

    LOCK(cs_main);
    CChain& index_chain = m_chainstate->m_chain;

    if (locator.IsNull()) {
        SetBestBlockIndex(nullptr);
    } else {
        // Setting the best block to the locator's top block. If it is not part of the
        // best chain, we will rewind to the fork point during index sync
        const CBlockIndex* locator_index{m_chainstate->m_blockman.LookupBlockIndex(locator.vHave.at(0))};
        if (!locator_index) {
            return InitError(Untranslated(strprintf("%s: best block of the index not found. Please rebuild the index.", GetName())));
        }
        SetBestBlockIndex(locator_index);
    }

    // Child init
    const CBlockIndex* start_block = m_best_block_index.load();
    if (!CustomInit(start_block ? std::make_optional(interfaces::BlockRef{start_block->GetBlockHash(), start_block->nHeight}) : std::nullopt)) {
        return false;
    }

    // Note: this will latch to true immediately if the user starts up with an empty
    // datadir and an index enabled. If this is the case, indexation will happen solely
    // via `BlockConnected` signals until, possibly, the next restart.
    m_synced = start_block == index_chain.Tip();
    m_init = true;
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

    // Since block is not in the chain, return the next block in the chain AFTER the last common ancestor.
    // Caller will be responsible for rewinding back to the common ancestor.
    return chain.Next(chain.FindFork(pindex_prev));
}

std::any BaseIndex::ProcessBlock(const CBlockIndex* pindex, const CBlock* block_data)
{
    interfaces::BlockInfo block_info = kernel::MakeBlockInfo(pindex, block_data);

    CBlock block;
    if (!block_data) { // disk lookup if block data wasn't provided
        if (!m_chainstate->m_blockman.ReadBlock(block, *pindex)) {
            FatalErrorf("%s: Failed to read block %s from disk",
                        __func__, pindex->GetBlockHash().ToString());
            return false;
        }
        block_info.data = &block;
    }

    CBlockUndo block_undo;
    if (CustomOptions().connect_undo_data) {
        if (pindex->nHeight > 0 && !m_chainstate->m_blockman.ReadBlockUndo(block_undo, *pindex)) {
            FatalErrorf("%s: Failed to read undo block data %s from disk",
                        __func__, pindex->GetBlockHash().ToString());
            return false;
        }
        block_info.undo_data = &block_undo;
    }

    const auto& any_obj = CustomProcessBlock(block_info);
    if (!any_obj.has_value()) {
        throw std::runtime_error(strprintf("%s: Failed to process block %s for index %s", __func__, pindex->GetBlockHash().GetHex(), GetName()));
    }
    return any_obj;
}

std::vector<std::any> BaseIndex::ProcessBlocks(bool process_in_order, const CBlockIndex* start, const CBlockIndex* end)
{
    std::vector<std::any> results;
    if (process_in_order) {
        // When ordering is required, collect all block indexes from [end..start] in order
        std::vector<const CBlockIndex*> ordered_blocks;
        for (const CBlockIndex* block = end; block && start->pprev != block; block = block->pprev) {
            ordered_blocks.emplace_back(block);
        }

        // And process blocks in forward order: from start to end
        for (auto it = ordered_blocks.rbegin(); it != ordered_blocks.rend(); ++it) {
            results.emplace_back(ProcessBlock(*it));
        }
        return results;
    }

    // If ordering is not required, process blocks directly from end to start
    for (const CBlockIndex* block = end; block && start->pprev != block; block = block->pprev) {
        results.emplace_back(ProcessBlock(block));
    }

    return results;
}

struct Task {
    int id;
    const CBlockIndex* start_index;
    const CBlockIndex* end_index;
    std::vector<std::any> result;

    Task(int task_id, const CBlockIndex* start, const CBlockIndex* end)
            : id(task_id), start_index(start), end_index(end) {}

    // Disallow copy
    Task(const Task&) = delete;
    Task& operator=(const Task&) = delete;
    Task(Task&&) noexcept = default;
};

// Context shared across the initial sync workers
struct SyncContext {
    Mutex mutex_pending_tasks;
    std::queue<Task> pending_tasks GUARDED_BY(mutex_pending_tasks);

    Mutex mutex_processed_tasks;
    std::map<int, Task> processed_tasks GUARDED_BY(mutex_processed_tasks);

    std::atomic<int> next_id_to_process{0};
};

// Synchronizes the index with the active chain.
//
// If parallel sync is enabled, this method uses WorkersCount()+1 threads (including the current thread)
// to process block ranges concurrently. Each worker handles up to 'm_blocks_per_worker' blocks each time
// (this is called a "task"), which are processed via CustomProcessBlock calls. Results are stored in the
// SyncContext's 'processed_tasks' map so they can be sequentially post-processed later.
//
// After completing a task, workers opportunistically post-process completed tasks *in order* using
// CustomPostProcessBlocks. This continues until all blocks have been fully processed and committed.
//
// Reorgs are detected and handled before syncing begins, ensuring the index starts aligned with the active chain.
void BaseIndex::Sync()
{
    if (m_synced) return; // we are synced, nothing to do

    // Before anything, verify we are in the active chain
    const CBlockIndex* pindex = m_best_block_index.load();
    const int tip_height = WITH_LOCK(cs_main, return m_chainstate->m_chain.Height());
    // Note: be careful, could return null if there is no more work to do or if pindex is not found (erased blocks dir).
    const CBlockIndex* pindex_next = WITH_LOCK(cs_main, return NextSyncBlock(pindex, m_chainstate->m_chain));
    if (!pindex_next) {
        m_synced = true;
        return;
    }

    // Handle potential reorgs; if the next block's parent doesn't match our current tip,
    // rewind our index state to match the chain and resume from there.
    if (pindex_next->pprev != pindex && !Rewind(pindex, pindex_next->pprev)) {
        FatalErrorf("%s: Failed to rewind index %s to a previous chain tip", __func__, GetName());
        return;
    }

    // Compute tasks ranges
    const int blocks_to_sync = tip_height - pindex_next->nHeight;
    const int num_tasks = blocks_to_sync / m_blocks_per_worker;
    const int remaining_blocks = blocks_to_sync % m_blocks_per_worker;
    const bool process_in_order = !AllowParallelSync();

    std::shared_ptr<SyncContext> ctx = std::make_shared<SyncContext>();
    {
        LOCK2(ctx->mutex_pending_tasks, ::cs_main);
        // Create fixed-size tasks
        const CBlockIndex* it_start = pindex;
        const CBlockIndex* it_end;
        for (int id = 0; id < num_tasks; ++id) {
            it_start = Assert(NextSyncBlock(it_start, m_chainstate->m_chain));
            it_end = m_chainstate->m_chain[it_start->nHeight + m_blocks_per_worker - 1];
            ctx->pending_tasks.emplace(id, it_start, it_end);
            it_start = it_end;
        }

        // Add final task with the remaining blocks, if any
        if (remaining_blocks > 0) {
            it_start = Assert(NextSyncBlock(it_start, m_chainstate->m_chain));
            it_end = m_chainstate->m_chain[it_start->nHeight + remaining_blocks];
            ctx->pending_tasks.emplace(/*task_id=*/num_tasks, it_start, it_end);
        }
    }

    // Returns nullopt only when there are no pending tasks
    const auto& try_get_task = [](auto& ctx) -> std::optional<Task> {
        LOCK(ctx->mutex_pending_tasks);
        if (ctx->pending_tasks.empty()) return std::nullopt;
        Task t = std::move(ctx->pending_tasks.front());
        ctx->pending_tasks.pop();
        return t;
    };

    enum class WorkerStatus { ABORT, PROCESSING, FINISHED };

    const auto& func_worker = [this, try_get_task, process_in_order](auto& ctx) -> WorkerStatus {
        if (m_interrupt) return WorkerStatus::FINISHED;

        // Try to obtain a task and process it
        if (std::optional<Task> maybe_task = try_get_task(ctx)) {
            Task task = std::move(*maybe_task);
            task.result = ProcessBlocks(process_in_order, task.start_index, task.end_index);

            LOCK(ctx->mutex_processed_tasks);
            ctx->processed_tasks.emplace(task.id, std::move(task));
        } else {
            // No pending tasks — might be finished
            // If we still have processed task to consume, proceed to finalize them.
            LOCK(ctx->mutex_processed_tasks);
            if (ctx->processed_tasks.empty()) return WorkerStatus::FINISHED;
        }

        // Post-process completed tasks opportunistically
        std::vector<Task> to_process;
        {
            TRY_LOCK(ctx->mutex_processed_tasks, locked);
            if (!locked) return WorkerStatus::PROCESSING;

            // Collect ready-to-process tasks in order
            int next_id = ctx->next_id_to_process.load();
            while (true) {
                auto it = ctx->processed_tasks.find(next_id);
                if (it == ctx->processed_tasks.end()) break;
                to_process.push_back(std::move(it->second));
                ctx->processed_tasks.erase(it);
                ++next_id;
            }

            // Nothing to process right now, keep processing
            if (to_process.empty()) return WorkerStatus::PROCESSING;
        }

        // Post-Process tasks
        for (const Task& task : to_process) {
            for (auto it = task.result.rbegin(); it != task.result.rend(); ++it) {
                if (!CustomPostProcessBlocks(*it)) { // error logged internally
                    m_interrupt();
                    FatalErrorf("Index %s: Failed to post process blocks", GetName());
                    return WorkerStatus::ABORT;
                }
            }
            // Update progress
            SetBestBlockIndex(task.end_index);
            ctx->next_id_to_process.store(task.id + 1);
        }

        // Check if there's anything left to do
        LOCK2(ctx->mutex_pending_tasks, ctx->mutex_processed_tasks);
        if (ctx->pending_tasks.empty() && ctx->processed_tasks.empty()) {
            return WorkerStatus::FINISHED;
        }

        return WorkerStatus::PROCESSING;
    };

    // Process task in parallel if enabled
    std::vector<std::future<void>> workers_job;
    if (m_thread_pool) {
        for (size_t i = 0; i < m_thread_pool->WorkersCount(); ++i) {
            workers_job.emplace_back(m_thread_pool->Submit([this, ctx, func_worker]() {
                WorkerStatus status{WorkerStatus::PROCESSING};
                while (!m_synced && status == WorkerStatus::PROCESSING) {
                    status = func_worker(ctx);
                    if (m_interrupt) return;
                }
            }));
        }
    }

    // Main index thread
    // Active-wait: we process blocks in this thread too.
    auto last_log_time = std::chrono::steady_clock::now();
    auto last_locator_write_time = std::chrono::steady_clock::now();

    while (!m_synced) {
        const WorkerStatus status{func_worker(ctx)};
        if (m_interrupt || status == WorkerStatus::ABORT) return;

        if (status == WorkerStatus::FINISHED) {
            // No more tasks to process; wait for all workers to finish their current tasks
            for (const auto& job : workers_job) job.wait();
            // No need to handle errors in Commit. If it fails, the error will be already be
            // logged. The best way to recover is to continue, as index cannot be corrupted by
            // a missed commit to disk for an advanced index state.
            Commit();

            // Before finishing, check if any new blocks were connected while we were syncing.
            // If so, process them synchronously before exiting.
            //
            // Note: it is important for cs_main to be locked while setting m_synced = true,
            // otherwise a new block could be attached while m_synced is still false, and
            // it would not be indexed.
            LOCK2(ctx->mutex_pending_tasks, ::cs_main);
            const CBlockIndex* curr_tip{m_best_block_index.load()};
            pindex_next = NextSyncBlock(curr_tip, m_chainstate->m_chain);
            // If the next block is null, it means we are done!
            if (!pindex_next) {
                m_synced = true;
                break;
            }

            // New blocks arrived during sync.
            // Handle potential reorgs; if the next block's parent doesn't match our tip,
            // rewind index state to the correct chain, then resume.
            if (pindex_next->pprev != curr_tip && !Rewind(curr_tip, pindex_next->pprev)) {
                FatalErrorf("%s: Failed to rewind index %s to a previous chain tip", __func__, GetName());
                return;
            }

            // Queue the final range of blocks to process.
            ctx->pending_tasks.emplace(ctx->next_id_to_process.load(),
                                       /*start_index=*/pindex_next,
                                       /*end_index=*/m_chainstate->m_chain.Tip());
        }

        const auto now = std::chrono::steady_clock::now();
        // Log progress every so often
        if (last_log_time + SYNC_LOG_INTERVAL < now) {
            LogPrintf("Syncing %s with block chain from height %d\n",
                      GetName(), m_best_block_index.load()->nHeight);
            last_log_time = now;
        }

        // Commit changes every so often
        if (last_locator_write_time + SYNC_LOCATOR_WRITE_INTERVAL < now) {
            Commit(); // No need to handle errors in Commit. See rationale above.
            last_locator_write_time = now;
        }
    }

    LogInfo("%s is enabled at height %d\n", GetName(), (m_best_block_index) ? m_best_block_index.load()->nHeight : 0);
}

bool BaseIndex::Commit()
{
    // Don't commit anything if we haven't indexed any block yet
    // (this could happen if init is interrupted).
    bool ok = m_best_block_index != nullptr;
    if (ok) {
        CDBBatch batch(GetDB());
        ok = CustomCommit(batch);
        if (ok) {
            GetDB().WriteBestBlock(batch, GetLocator(*m_chain, m_best_block_index.load()->GetBlockHash()));
            ok = GetDB().WriteBatch(batch);
        }
    }
    if (!ok) {
        LogError("%s: Failed to commit latest %s state\n", __func__, GetName());
        return false;
    }
    return true;
}

bool BaseIndex::Rewind(const CBlockIndex* current_tip, const CBlockIndex* new_tip)
{
    assert(current_tip == m_best_block_index);
    assert(current_tip->GetAncestor(new_tip->nHeight) == new_tip);

    CBlock block;
    CBlockUndo block_undo;

    for (const CBlockIndex* iter_tip = current_tip; iter_tip != new_tip; iter_tip = iter_tip->pprev) {
        interfaces::BlockInfo block_info = kernel::MakeBlockInfo(iter_tip);
        if (CustomOptions().disconnect_data) {
            if (!m_chainstate->m_blockman.ReadBlock(block, *iter_tip)) {
                LogError("%s: Failed to read block %s from disk",
                             __func__, iter_tip->GetBlockHash().ToString());
                return false;
            }
            block_info.data = &block;
        }
        if (CustomOptions().disconnect_undo_data && iter_tip->nHeight > 0) {
            if (!m_chainstate->m_blockman.ReadBlockUndo(block_undo, *iter_tip)) {
                return false;
            }
            block_info.undo_data = &block_undo;
        }
        if (!CustomRemove(block_info)) {
            return false;
        }
    }

    // In the case of a reorg, ensure persisted block locator is not stale.
    // Pruning has a minimum of 288 blocks-to-keep and getting the index
    // out of sync may be possible but a users fault.
    // In case we reorg beyond the pruned depth, ReadBlock would
    // throw and lead to a graceful shutdown
    SetBestBlockIndex(new_tip);
    if (!Commit()) {
        // If commit fails, revert the best block index to avoid corruption.
        SetBestBlockIndex(current_tip);
        return false;
    }

    return true;
}

void BaseIndex::BlockConnected(ChainstateRole role, const std::shared_ptr<const CBlock>& block, const CBlockIndex* pindex)
{
    // Ignore events from the assumed-valid chain; we will process its blocks
    // (sequentially) after it is fully verified by the background chainstate. This
    // is to avoid any out-of-order indexing.
    //
    // TODO at some point we could parameterize whether a particular index can be
    // built out of order, but for now just do the conservative simple thing.
    if (role == ChainstateRole::ASSUMEDVALID) {
        return;
    }

    // Ignore BlockConnected signals until we have fully indexed the chain.
    if (!m_synced) {
        return;
    }

    const CBlockIndex* best_block_index = m_best_block_index.load();
    if (!best_block_index) {
        if (pindex->nHeight != 0) {
            FatalErrorf("%s: First block connected is not the genesis block (height=%d)",
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
            LogPrintf("%s: WARNING: Block %s does not connect to an ancestor of "
                      "known best chain (tip=%s); not updating index\n",
                      __func__, pindex->GetBlockHash().ToString(),
                      best_block_index->GetBlockHash().ToString());
            return;
        }
        if (best_block_index != pindex->pprev && !Rewind(best_block_index, pindex->pprev)) {
            FatalErrorf("%s: Failed to rewind index %s to a previous chain tip",
                       __func__, GetName());
            return;
        }
    }

    // Dispatch block to child class; errors are logged internally and abort the node.
    if (CustomPostProcessBlocks(ProcessBlock(pindex, block.get()))) {
        // Setting the best block index is intentionally the last step of this
        // function, so BlockUntilSyncedToCurrentChain callers waiting for the
        // best block index to be updated can rely on the block being fully
        // processed, and the index object being safe to delete.
        SetBestBlockIndex(pindex);
    }
}

void BaseIndex::ChainStateFlushed(ChainstateRole role, const CBlockLocator& locator)
{
    // Ignore events from the assumed-valid chain; we will process its blocks
    // (sequentially) after it is fully verified by the background chainstate.
    if (role == ChainstateRole::ASSUMEDVALID) {
        return;
    }

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
        FatalErrorf("%s: First block (hash=%s) in locator was not found",
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
        LogPrintf("%s: WARNING: Locator contains block (hash=%s) not on known best "
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
    m_chain->context()->validation_signals->SyncWithValidationInterfaceQueue();
    return true;
}

void BaseIndex::Interrupt()
{
    m_interrupt();
}

bool BaseIndex::StartBackgroundSync()
{
    if (!m_init) throw std::logic_error("Error: Cannot start a non-initialized index");

    m_thread_sync = std::thread(&util::TraceThread, GetName(), [this] { Sync(); });
    return true;
}

void BaseIndex::Stop()
{
    if (m_chain->context()->validation_signals) {
        m_chain->context()->validation_signals->UnregisterValidationInterface(this);
    }

    if (m_thread_sync.joinable()) {
        m_thread_sync.join();
    }
}

IndexSummary BaseIndex::GetSummary() const
{
    IndexSummary summary{};
    summary.name = GetName();
    summary.synced = m_synced;
    if (const auto& pindex = m_best_block_index.load()) {
        summary.best_block_height = pindex->nHeight;
        summary.best_block_hash = pindex->GetBlockHash();
    } else {
        summary.best_block_height = 0;
        summary.best_block_hash = m_chain->getBlockHash(0);
    }
    return summary;
}

void BaseIndex::SetBestBlockIndex(const CBlockIndex* block)
{
    assert(!m_chainstate->m_blockman.IsPruneMode() || AllowPrune());

    if (AllowPrune() && block) {
        node::PruneLockInfo prune_lock;
        prune_lock.height_first = block->nHeight;
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
