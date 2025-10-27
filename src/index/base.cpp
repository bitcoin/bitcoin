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
            return InitError(Untranslated(strprintf("best block of %s not found. Please rebuild the index.", GetName())));
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
            FatalErrorf("Failed to read block %s from disk",
                        pindex->GetBlockHash().ToString());
            return {};
        }
        block_info.data = &block;
    }

    CBlockUndo block_undo;
    if (CustomOptions().connect_undo_data) {
        if (pindex->nHeight > 0 && !m_chainstate->m_blockman.ReadBlockUndo(block_undo, *pindex)) {
            FatalErrorf("Failed to read undo block data %s from disk",
                        pindex->GetBlockHash().ToString());
            return {};
        }
        block_info.undo_data = &block_undo;
    }

    const auto& any_obj = CustomProcessBlock(block_info);
    if (!any_obj.has_value()) {
        FatalErrorf("Failed to process block %s for index %s",
                    pindex->GetBlockHash().GetHex(), GetName());
        return {};
    }
    return any_obj;
}

std::vector<std::any> BaseIndex::ProcessBlocks(bool process_in_order, const CBlockIndex* start, const CBlockIndex* end)
{
    std::vector<std::any> results;
    results.reserve(end->nHeight - start->nHeight + 1);
    if (process_in_order) {
        // When ordering is required, collect all block indexes from [end..start] in order
        std::vector<const CBlockIndex*> ordered_blocks;
        for (const CBlockIndex* block = end; block && start->pprev != block; block = block->pprev) {
            ordered_blocks.emplace_back(block);
        }

        // And process blocks in forward order: from start to end
        for (auto it = ordered_blocks.rbegin(); it != ordered_blocks.rend(); ++it) {
            auto op_res = ProcessBlock(*it);
            if (!op_res.has_value()) return {};
            results.emplace_back(std::move(op_res));
        }
        return results;
    }

    // If ordering is not required, process blocks directly from end to start
    for (const CBlockIndex* block = end; block && start->pprev != block; block = block->pprev) {
        auto op_res = ProcessBlock(block);
        if (!op_res.has_value()) return {};
        results.emplace_back(std::move(op_res));
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

// Shared state across sync workers.
struct SyncContext {
    std::atomic<int> num_tasks{0};
    Mutex mutex_processed_tasks;
    std::map<int, std::unique_ptr<Task>> processed_tasks GUARDED_BY(mutex_processed_tasks);
    std::atomic<int> next_id_to_process{0}; // the task position we are at (important for sequential disk dumps)

    std::atomic<NodeClock::time_point> next_log_time;
    std::atomic<NodeClock::time_point> next_locator_write_time;
};

// NOLINTNEXTLINE(misc-no-recursion)
void BaseIndex::RunTask(std::unique_ptr<Task> ptr_task, std::shared_ptr<SyncContext>& ctx, bool process_in_order)
{
    if (m_interrupt) return;

    // Process task
    if (ptr_task) {
        ptr_task->result = ProcessBlocks(process_in_order, ptr_task->start_index, ptr_task->end_index);
        if (ptr_task->result.empty()) {
            // Empty result indicates an internal error (logged internally).
            m_interrupt();  // notify other workers and abort
            return;
        }
    }

    // Post-process completed tasks opportunistically
    std::vector<std::unique_ptr<Task>> to_process;
    {
        LOCK(ctx->mutex_processed_tasks);
        int next_id = ctx->next_id_to_process.load();

        // If we received a task, decide whether to process or queue it
        if (ptr_task) {
            if (ptr_task->id == next_id) {
                to_process.emplace_back(std::move(ptr_task));
                next_id++;
            } else {
                // Push it to post-process it when the previous tasks are ready
                ctx->processed_tasks.emplace(ptr_task->id, std::move(ptr_task));
            }
        }

        // Collect ready-to-process tasks in order
        while (true) {
            auto it = ctx->processed_tasks.find(next_id);
            if (it == ctx->processed_tasks.end()) break;
            to_process.push_back(std::move(it->second));
            ctx->processed_tasks.erase(it);
            next_id++;
        }
    }

    // Post-Processing helper
    const auto fn_post_process = [this](auto begin, auto end) {
        return std::all_of(begin, end, [this](const auto& data) { return CustomPostProcessBlocks(data); });
    };

    // Post-Process tasks
    for (const auto& task : to_process) {
        // Depending on the processing order, we need to iterate the result forward or backwards
        bool complete = process_in_order ?
                        fn_post_process(task->result.begin(), task->result.end()) :
                        fn_post_process(task->result.rbegin(), task->result.rend());
        if (!complete) {
            m_interrupt();
            FatalErrorf("Index %s: Failed to post process blocks", GetName());
            return;
        }

        // Update progress
        SetBestBlockIndex(task->end_index);
        ctx->next_id_to_process.fetch_add(1);
    }


    // Check if there's anything left to do
    if (ctx->next_id_to_process.load() == ctx->num_tasks.load() && WITH_LOCK(ctx->mutex_processed_tasks, return ctx->processed_tasks.empty())) {
        // No need to handle errors in Commit. If it fails, the error will already be
        // logged. The best way to recover is to continue, as index cannot be corrupted by
        // a missed commit to disk for an advanced index state.
        Commit();

        // Before finishing, check if any new blocks were connected while we were syncing.
        // If so, process a final task with all remaining blocks.
        const CBlockIndex* curr_tip{m_best_block_index.load()};
        const CBlockIndex* pindex_next;
        const CBlockIndex* pindex_end;
        {
            // Note: it is important for cs_main to be locked while setting m_synced = true,
            // otherwise a new block could be attached while m_synced is still false, and
            // it would not be indexed.
            LOCK(::cs_main);
            // Note: Could return null if there is no more work to do or if 'curr_tip' is not found (erased blocks dir).
            pindex_next = NextSyncBlock(curr_tip, m_chainstate->m_chain);
            if (!pindex_next) {
                // If the next block is null, it means we are done!
                m_synced = true;
                LogInfo("%s is enabled at height %d\n", GetName(), curr_tip ? curr_tip->nHeight : 0);
                return;
            }
            pindex_end = m_chainstate->m_chain.Tip();
        }

        // If the next block's parent doesn't match our current tip,
        // rewind our index state to match the chain and resume from there.
        if (pindex_next->pprev != curr_tip && !Rewind(curr_tip, pindex_next->pprev)) {
            m_interrupt();
            FatalErrorf("Failed to rewind index %s to a previous chain tip", GetName());
            return;
        }

        // Run the final range of blocks synchronously
        ctx->num_tasks.fetch_add(1);
        RunTask(std::make_unique<Task>(ctx->next_id_to_process.load(), pindex_next, pindex_end),
                   ctx, process_in_order);
        return;
    }

    auto now{NodeClock::now()};
    // Log periodically
    auto next_log = ctx->next_log_time.load(std::memory_order_relaxed);
    if (now >= next_log && ctx->next_log_time.compare_exchange_weak(next_log, now + SYNC_LOG_INTERVAL, std::memory_order_relaxed)) {
        LogInfo("Syncing %s with block chain from height %d",
                GetName(), m_best_block_index ? m_best_block_index.load()->nHeight : 0);
    }

    // Commit periodically
    auto next_commit = ctx->next_locator_write_time.load(std::memory_order_relaxed);
    if (now >= next_commit && ctx->next_locator_write_time.compare_exchange_weak(next_commit, now + SYNC_LOCATOR_WRITE_INTERVAL, std::memory_order_relaxed)) {
        Commit(); // No need to handle errors in Commit. See rationale above.
    }
}

// Synchronizes the index with the active chain.
//
// If parallel sync is enabled, this method submits ranges of blocks to be processed concurrently.
// Each worker handles up to 'm_blocks_per_worker' blocks each time (this is called a "task"),
// which are processed via CustomProcessBlock calls. Results are stored in the SyncContext's
// 'processed_tasks' map, so they can be sequentially post-processed later.
//
// After completing a task, workers opportunistically post-process completed tasks *in order* using
// CustomPostProcessBlocks. This continues until all blocks have been fully processed and committed.
//
// Reorgs are detected and handled before syncing begins, ensuring the index starts aligned with the active chain.
void BaseIndex::Sync()
{
    if (m_synced) return; // Already synced, nothing to do

    // Before anything, ensure we are in the active chain
    const CBlockIndex* curr_tip = m_best_block_index.load();
    const CBlockIndex* pindex_next = WITH_LOCK(cs_main, return NextSyncBlock(curr_tip, m_chainstate->m_chain));
    if (!pindex_next) {
        m_synced = true;
        return;
    }

    // If the next block's parent doesn't match our current tip,
    // rewind our index state to match the chain and resume from there.
    if (pindex_next->pprev != curr_tip && !Rewind(curr_tip, pindex_next->pprev)) {
        FatalErrorf("Failed to rewind index %s to a previous chain tip", GetName());
        return;
    }

    // Compute tasks ranges
    const int blocks_to_sync = WITH_LOCK(cs_main, return m_chainstate->m_chain.Height()) - pindex_next->nHeight + 1;
    const int num_tasks = blocks_to_sync / m_blocks_per_worker;
    const int remaining_blocks = blocks_to_sync % m_blocks_per_worker;
    const bool process_in_order = OrderingRequired();

    std::shared_ptr<SyncContext> ctx = std::make_shared<SyncContext>();
    std::vector<std::unique_ptr<Task>> pending_tasks;
    {
        LOCK(::cs_main);
        // Create fixed-size tasks
        const CBlockIndex* it_start = pindex_next;
        const CBlockIndex* it_end;
        for (int id = 0; id < num_tasks; ++id) {
            it_end = m_chainstate->m_chain[it_start->nHeight + m_blocks_per_worker - 1];
            pending_tasks.emplace_back(std::make_unique<Task>(id, it_start, it_end));
            it_start = Assert(NextSyncBlock(it_end, m_chainstate->m_chain));
        }

        // Add final task with the remaining blocks, if any
        if (remaining_blocks > 0) {
            it_end = m_chainstate->m_chain[it_start->nHeight + remaining_blocks - 1];
            pending_tasks.emplace_back(std::make_unique<Task>(/*task_id=*/num_tasks, it_start, it_end));
        }

        ctx->num_tasks.store(pending_tasks.size());
    }

    // Init next interval
    ctx->next_log_time.store(NodeClock::now() + SYNC_LOG_INTERVAL);
    ctx->next_locator_write_time.store(NodeClock::now() + SYNC_LOCATOR_WRITE_INTERVAL);

    if (AllowParallelSync() && m_thread_pool) {
        // Process task in parallel if enabled
        std::vector<std::future<void>> futures;
        futures.reserve(pending_tasks.size());
        for (auto& t : pending_tasks) {
            futures.emplace_back(m_thread_pool->Submit([this, ctx, process_in_order, task = std::move(t)]() mutable {
                RunTask(std::move(task), ctx, process_in_order);
            }));
        }
        for (auto& f : futures) if (!m_interrupt) f.wait();
        // In case several tasks finish roughly at the same time, run a final post-processing round
        if (WITH_LOCK(ctx->mutex_processed_tasks, return !ctx->processed_tasks.empty())) {
            RunTask(/*ptr_task=*/nullptr, ctx, process_in_order);
        }
    } else {
        // Sequential mode, just execute tasks one by one
        for (auto& t : pending_tasks) {
            RunTask(std::move(t), ctx, process_in_order);
            if (m_interrupt) return;
        }
    }
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
        LogError("Failed to commit latest %s state", GetName());
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
                LogError("Failed to read block %s from disk",
                         iter_tip->GetBlockHash().ToString());
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

    // Don't commit here - the committed index state must never be ahead of the
    // flushed chainstate, otherwise unclean restarts would lead to index corruption.
    // Pruning has a minimum of 288 blocks-to-keep and getting the index
    // out of sync may be possible but a users fault.
    // In case we reorg beyond the pruned depth, ReadBlock would
    // throw and lead to a graceful shutdown
    SetBestBlockIndex(new_tip);
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
            FatalErrorf("First block connected is not the genesis block (height=%d)",
                       pindex->nHeight);
            return;
        }
    } else {
        // Ensure block connects to an ancestor of the current best block. This should be the case
        // most of the time, but may not be immediately after the sync thread catches up and sets
        // m_synced. Consider the case where there is a reorg and the blocks on the stale branch are
        // in the ValidationInterface queue backlog even after the sync thread has caught up to the
        // new chain tip. In this unlikely event, log a warning and let the queue clear.
        if (best_block_index->GetAncestor(pindex->nHeight - 1) != pindex->pprev) {
            LogWarning("Block %s does not connect to an ancestor of "
                      "known best chain (tip=%s); not updating index",
                      pindex->GetBlockHash().ToString(),
                      best_block_index->GetBlockHash().ToString());
            return;
        }
        if (best_block_index != pindex->pprev && !Rewind(best_block_index, pindex->pprev)) {
            FatalErrorf("Failed to rewind %s to a previous chain tip",
                       GetName());
            return;
        }
    }

    // Dispatch block to child class; errors are logged internally and abort the node.
    if (const auto& res = ProcessBlock(pindex, block.get()); res.has_value()) {
        if (!CustomPostProcessBlocks(res)) {
            FatalErrorf("Index %s: Failed to post process block %s", GetName(), block->GetHash().GetHex());
            return;
        }
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
        FatalErrorf("First block (hash=%s) in locator was not found",
                   locator_tip_hash.ToString());
        return;
    }

    // This checks that ChainStateFlushed callbacks are received after BlockConnected. The check may fail
    // immediately after the sync thread catches up and sets m_synced. Consider the case where
    // there is a reorg and the blocks on the stale branch are in the ValidationInterface queue
    // backlog even after the sync thread has caught up to the new chain tip. In this unlikely
    // event, log a warning and let the queue clear.
    const CBlockIndex* best_block_index = m_best_block_index.load();
    if (best_block_index->GetAncestor(locator_tip_index->nHeight) != locator_tip_index) {
        LogWarning("Locator contains block (hash=%s) not on known best "
                  "chain (tip=%s); not writing index locator",
                  locator_tip_hash.ToString(),
                  best_block_index->GetBlockHash().ToString());
        return;
    }

    // No need to handle errors in Commit. If it fails, the error will already be logged. The
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

    LogInfo("%s is catching up on block notifications", GetName());
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
