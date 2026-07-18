// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chain.h>
#include <interfaces/chain.h>
#include <logging.h>
#include <primitives/block.h>
#include <sync.h>
#include <util/check.h>
#include <util/threadpool.h>
#include <wallet/scan.h>
#include <wallet/wallet.h>

#include <algorithm>
#include <deque>
#include <future>
#include <memory>

using interfaces::FoundBlock;

namespace wallet {

int64_t ChainScanner::ScanFromTime(int64_t startTime, const WalletRescanReserver& reserver)
{
    // Find starting block. May be null if nCreateTime is greater than the
    // highest blockchain timestamp, in which case there is nothing that needs
    // to be scanned.
    int start_height = 0;
    uint256 start_block;
    bool start = m_wallet.chain().findFirstBlockWithTimeAndHeight(startTime - TIMESTAMP_WINDOW, 0, FoundBlock().hash(start_block).height(start_height));
    m_wallet.WalletLogPrintf("%s: Rescanning last %i blocks\n", __func__, start ? WITH_LOCK(m_wallet.cs_wallet, return m_wallet.GetLastBlockHeight()) - start_height + 1 : 0);

    if (start) {
        // TODO: this should take into account failure by ScanResult::USER_ABORT
        ScanResult result = Scan(start_block, start_height, /*max_height=*/{}, reserver, /*save_progress=*/false);
        if (result.status == ScanResult::FAILURE) {
            int64_t time_max;
            CHECK_NONFATAL(m_wallet.chain().findBlock(result.last_failed_block, FoundBlock().maxTime(time_max)));
            return time_max + TIMESTAMP_WINDOW + 1;
        }
    }
    return startTime;
}

bool WalletRescanReserver::reserve(bool with_passphrase) {
    assert(!m_could_reserve);
    if (!m_wallet.Scanner().TryReserve(with_passphrase)) {
        return false;
    }
    m_could_reserve = true;
    return true;
}

bool WalletRescanReserver::isReserved() const {
    return m_could_reserve && m_wallet.Scanner().IsScanning();
}

WalletRescanReserver::~WalletRescanReserver() {
    if (m_could_reserve) {
        m_wallet.Scanner().Release();
    }
}

bool ChainScanner::TryReserve(bool with_passphrase) {
    if (m_scanning.exchange(true)) return false;
    // Discard any abort request left over from previous reservation, so
    // that an abort requested while the reservation is held always applies
    // to abort this rescan, even if it arrives before the scan loop starts.
    m_abort = false;
    m_scanning_with_passphrase = with_passphrase;
    m_scanning_start = SteadyClock::now();
    m_scanning_progress = 0;
    return true;
}

void ChainScanner::Release() {
    m_scanning = false;
    m_scanning_with_passphrase = false;
}

class FastWalletRescanFilter
{
public:
    FastWalletRescanFilter(const CWallet& wallet) : m_wallet(wallet)
    {
        // create initial filter with scripts from all ScriptPubKeyMans
        for (auto spkm : m_wallet.GetAllScriptPubKeyMans()) {
            auto desc_spkm{dynamic_cast<DescriptorScriptPubKeyMan*>(spkm)};
            assert(desc_spkm != nullptr);
            AddScriptPubKeys(desc_spkm);
            // save each range descriptor's end for possible future filter updates
            if (desc_spkm->IsHDEnabled()) {
                m_last_range_ends.emplace(desc_spkm->GetID(), desc_spkm->GetEndRange());
            }
        }
    }

    //! Whether a keypool top-up has derived new scripts since the last
    //! UpdateIfNeeded() call.
    bool NeedsUpdate() const
    {
        for (const auto& [desc_spkm_id, last_range_end] : m_last_range_ends) {
            auto desc_spkm{dynamic_cast<DescriptorScriptPubKeyMan*>(m_wallet.GetScriptPubKeyMan(desc_spkm_id))};
            assert(desc_spkm != nullptr);
            if (desc_spkm->GetEndRange() > last_range_end) return true;
        }
        return false;
    }

    //! Repopulate the filter with new scripts derived by a top-up, and
    //! return the scripts that were added.
    //!
    //! MatchesBlock() reads m_filter_set without synchronization, and filter
    //! checks may run on worker threads. This must therefore never be called
    //! while any filter check is pending; ParallelFilterChecker::FinishChecks()
    //! finishes all submitted checks to uphold this.
    GCSFilter::ElementSet UpdateIfNeeded()
    {
        GCSFilter::ElementSet delta;
        // repopulate filter with new scripts if top-up has happened since last iteration
        for (const auto& [desc_spkm_id, last_range_end] : m_last_range_ends) {
            auto desc_spkm{dynamic_cast<DescriptorScriptPubKeyMan*>(m_wallet.GetScriptPubKeyMan(desc_spkm_id))};
            assert(desc_spkm != nullptr);
            int32_t current_range_end{desc_spkm->GetEndRange()};
            if (current_range_end > last_range_end) {
                for (const auto& script_pub_key : desc_spkm->GetScriptPubKeys(last_range_end)) {
                    delta.emplace(script_pub_key.begin(), script_pub_key.end());
                    m_filter_set.emplace(script_pub_key.begin(), script_pub_key.end());
                }
                m_last_range_ends.at(desc_spkm->GetID()) = current_range_end;
            }
        }
        return delta;
    }

    std::optional<bool> MatchesBlock(const uint256& block_hash) const
    {
        return m_wallet.chain().blockFilterMatchesAny(BlockFilterType::BASIC, block_hash, m_filter_set);
    }

private:
    const CWallet& m_wallet;
    /** Map for keeping track of each range descriptor's last seen end range.
      * This information is used to detect whether new addresses were derived
      * (that is, if the current end range is larger than the saved end range)
      * after processing a block and hence a filter set update is needed to
      * take possible keypool top-ups into account.
      */
    std::map<uint256, int32_t> m_last_range_ends;
    GCSFilter::ElementSet m_filter_set;

    void AddScriptPubKeys(const DescriptorScriptPubKeyMan* desc_spkm, int32_t last_range_end = 0)
    {
        for (const auto& script_pub_key : desc_spkm->GetScriptPubKeys(last_range_end)) {
            m_filter_set.emplace(script_pub_key.begin(), script_pub_key.end());
        }
    }
};

//! Maximum number of blocks queued in the filter checker at any time,
//! bounding the memory used for read-ahead. Also bounds the number of blocks
//! ReadNextBlocks hands to the scan loop at once.
constexpr size_t MAX_BLOCKQUEUE_SIZE{1000};

//! Number of blocks checked by a single thread-pool task. Batching amortizes
//! the scheduling cost of a task (queue locking, future, worker wake-up),
//! which would otherwise rival the cost of the filter checks themselves.
constexpr size_t FILTER_TASK_SPAN{8};

enum FilterRes {
    FILTER_NO_MATCH,
    FILTER_MATCH,
    FILTER_NO_FILTER,
};

/**
 * Checks block filters in parallel on its thread pool. Push() queues a
 * block, Submit() starts batches of checks for the queued blocks, and
 * TryPop() pops the front block together with its verdict, in block order.
 * Pairing blocks with verdicts makes the delivery ordering structural. Each
 * task checks a span of up to FILTER_TASK_SPAN blocks, amortizing per-task
 * scheduling overhead.
 *
 * When a keypool top-up derives new scripts, FinishChecks() completes every
 * submitted check so the filter set can be updated safely, and ApplyDelta()
 * patches the verdicts that have not been delivered yet.
 *
 * Workers only ever read the wallet's filter set via
 * FastWalletRescanFilter::MatchesBlock. To keep that race-free,
 * FinishChecks() and the destructor drain every submitted check, so the
 * filter set is never updated while a check is pending.
 */
class ParallelFilterChecker {
    const FastWalletRescanFilter& m_filter;
    interfaces::Chain& m_chain;
    ThreadPool m_pool;
    //! Blocks queued for filter checks, oldest first. Verdicts are produced
    //! in this order; the front block is popped together with its verdict.
    std::deque<ChainScanner::QueuedBlock> m_blocks;
    //! Height of the last block whose filter check was submitted; queued
    //! blocks above it have not been submitted yet.
    int m_last_submitted_height;
    //! Verdict for the front block, handed back via PushBack().
    std::optional<FilterRes> m_carry;
    //! One future per submitted span task, in block order.
    std::deque<std::future<std::vector<FilterRes>>> m_futures;
    //! Verdicts decoded from completed spans, awaiting delivery.
    std::deque<FilterRes> m_ready;
    //! Number of submitted checks whose verdicts have not been popped yet.
    size_t m_pending{0};

    bool FrontReady() {
        return !m_futures.empty() &&
            m_futures.front().wait_for(std::chrono::seconds::zero()) == std::future_status::ready;
    }

    void Drain() {
        // Run every queued check so that no worker still references the
        // filter set, then wait out the checks already running.
        while (m_pool.WorkQueueSize() > 0) {
            m_pool.ProcessTask();
        }
        for (auto& fut : m_futures) fut.wait();
    }

    //! Produce the next verdict in block order, if one is ready.
    std::optional<FilterRes> TryPopVerdict() {
        if (m_ready.empty()) {
            if (!FrontReady()) return std::nullopt;
            const auto span = m_futures.front().get();
            m_futures.pop_front();
            m_ready.insert(m_ready.end(), span.begin(), span.end());
        }
        const auto res = m_ready.front();
        m_ready.pop_front();
        --m_pending;
        return res;
    }

public:
    explicit ParallelFilterChecker(const FastWalletRescanFilter& filter, interfaces::Chain& chain, int start_height, const std::string& thread_name, int num_threads)
        : m_filter(filter), m_chain(chain), m_pool(thread_name), m_last_submitted_height(start_height - 1) {
        m_pool.Start(num_threads);
    }

    ~ParallelFilterChecker() { Drain(); }

    bool Empty() const { return m_blocks.empty(); }
    bool Full() const { return m_blocks.size() >= MAX_BLOCKQUEUE_SIZE; }
    void Push(const ChainScanner::QueuedBlock& block) { m_blocks.push_back(block); }
    //! Number of submitted checks whose verdicts have not been popped yet.
    size_t Pending() const { return m_pending; }

    //! Pop the front block together with its verdict, if one is ready.
    std::optional<std::pair<ChainScanner::QueuedBlock, FilterRes>> TryPop() {
        const auto res = m_carry ? std::exchange(m_carry, std::nullopt) : TryPopVerdict();
        if (!res) return std::nullopt;
        const ChainScanner::QueuedBlock block = m_blocks.front();
        m_blocks.pop_front();
        return {{block, *res}};
    }

    //! Hand the front block and its verdict back, to be delivered again by
    //! the next TryPop().
    void PushBack(const ChainScanner::QueuedBlock& block, FilterRes res) {
        Assume(!m_carry);
        m_blocks.push_front(block);
        m_carry = res;
    }

    //! Finish every submitted check, collecting the verdicts for delivery,
    //! so that no worker references the filter set while it is updated.
    void FinishChecks() {
        Drain();
        // Collect the finished span verdicts, preserving block order.
        for (auto& fut : m_futures) {
            const auto span = fut.get();
            m_ready.insert(m_ready.end(), span.begin(), span.end());
        }
        m_futures.clear();
    }

    //! Patch all undelivered verdicts with the scripts newly derived by a
    //! keypool top-up. The filter set only grows, so a MATCH stays valid; a
    //! NO_MATCH is re-checked against just the delta and upgraded on a hit.
    //! The re-checks for ready verdicts are re-queued as span tasks taking
    //! the place of the collected spans, so they run on the pool alongside
    //! fresh checks instead of stalling the scan thread.
    void ApplyDelta(const GCSFilter::ElementSet& delta) {
        // FinishChecks() must have been called before the filter set update,
        // so no checks are still running.
        Assume(m_futures.empty());
        // The carried verdict belongs to the front block; it may be needed
        // by the very next TryPop(), so patch it inline.
        if (m_carry && *m_carry == FILTER_NO_MATCH &&
            m_chain.blockFilterMatchesAny(BlockFilterType::BASIC, m_blocks.front().hash, delta).value_or(false)) {
            m_carry = FILTER_MATCH;
        }
        if (m_ready.empty()) return;

        // Pair the ready verdicts with their blocks: they belong to the
        // front blocks, after the carried one.
        std::vector<std::pair<uint256, FilterRes>> to_patch;
        to_patch.reserve(m_ready.size());
        size_t block_index = m_carry ? 1 : 0;
        for (const auto res : m_ready) {
            to_patch.emplace_back(m_blocks[block_index].hash, res);
            ++block_index;
        }
        m_ready.clear();

        // Chunk the pairs into one task per span, sharing one copy of the
        // delta. The futures are queued in block order, ahead of any span
        // Submit() adds later, so delivery is unchanged.
        const auto shared_delta = std::make_shared<const GCSFilter::ElementSet>(delta);
        std::vector<std::function<std::vector<FilterRes>()>> tasks;
        tasks.reserve((to_patch.size() + FILTER_TASK_SPAN - 1) / FILTER_TASK_SPAN);
        for (size_t begin = 0; begin < to_patch.size(); begin += FILTER_TASK_SPAN) {
            const size_t end = std::min(begin + FILTER_TASK_SPAN, to_patch.size());
            std::vector<std::pair<uint256, FilterRes>> span(to_patch.begin() + begin, to_patch.begin() + end);
            tasks.emplace_back([this, shared_delta, span = std::move(span)] {
                std::vector<FilterRes> res;
                res.reserve(span.size());
                for (const auto& [hash, verdict] : span) {
                    if (verdict == FILTER_NO_MATCH &&
                        m_chain.blockFilterMatchesAny(BlockFilterType::BASIC, hash, *shared_delta).value_or(false)) {
                        res.push_back(FILTER_MATCH);
                    } else {
                        res.push_back(verdict);
                    }
                }
                return res;
            });
        }

        auto futures = m_pool.Submit(std::move(tasks));
        // The pool is private to the checker and never stopped while it
        // exists, so submission cannot fail.
        Assume(futures.has_value());
        if (!futures) return;
        for (auto& fut : *futures) m_futures.emplace_back(std::move(fut));
    }

    //! Whether any queued blocks still need their filter check submitted.
    bool CanSubmit() const {
        return !m_blocks.empty() && m_blocks.back().height > m_last_submitted_height;
    }

    //! Start filter checks for queued blocks that have not been submitted
    //! yet. Unless flush is set, partial spans are held back for batching.
    void Submit(bool flush) {
        // Cap the number of in-flight span tasks: if all checks were queued
        // upfront and the filtered range turns out smaller than expected,
        // workers would check blocks that get discarded, wasting CPU cycles.
        const size_t max_tasks = m_pool.WorkersCount() * 2;
        if (m_futures.size() >= max_tasks) return;

        // Collect the blocks needing a check.
        const size_t max_hashes = (max_tasks - m_futures.size()) * FILTER_TASK_SPAN;
        std::vector<std::pair<uint256, int>> to_check;
        to_check.reserve(std::min(max_hashes, m_blocks.size()));
        for (const auto& block : m_blocks) {
            if (block.height <= m_last_submitted_height) continue;
            if (to_check.size() == max_hashes) break;
            to_check.emplace_back(block.hash, block.height);
        }
        // Hold a partial span back for batching unless asked to flush.
        if (!flush) to_check.resize(to_check.size() - to_check.size() % FILTER_TASK_SPAN);
        if (to_check.empty()) return;
        m_last_submitted_height = to_check.back().second;
        m_pending += to_check.size();

        // Chunk the hashes into one task per span.
        std::vector<std::function<std::vector<FilterRes>()>> tasks;
        tasks.reserve((to_check.size() + FILTER_TASK_SPAN - 1) / FILTER_TASK_SPAN);
        for (size_t begin = 0; begin < to_check.size(); begin += FILTER_TASK_SPAN) {
            const size_t end = std::min(begin + FILTER_TASK_SPAN, to_check.size());
            std::vector<uint256> span;
            span.reserve(end - begin);
            for (size_t i = begin; i < end; ++i) span.push_back(to_check[i].first);
            tasks.emplace_back([this, span = std::move(span)] {
                std::vector<FilterRes> res;
                res.reserve(span.size());
                for (const auto& hash : span) {
                    const auto matches = m_filter.MatchesBlock(hash);
                    if (!matches.has_value()) res.push_back(FILTER_NO_FILTER);
                    else res.push_back(*matches ? FILTER_MATCH : FILTER_NO_MATCH);
                }
                return res;
            });
        }

        auto futures = m_pool.Submit(std::move(tasks));
        // The pool is private to the checker and never stopped while it
        // exists, so submission cannot fail.
        Assume(futures.has_value());
        if (!futures) return;
        for (auto& fut : *futures) m_futures.emplace_back(std::move(fut));
    }

    //! Block until the oldest pending verdict is ready.
    void WaitForResult() {
        if (!m_ready.empty() || m_futures.empty()) return;
        // The oldest span may still be sitting in the work queue: help by
        // running queued checks on this thread, then block until it is done.
        while (!FrontReady() && m_pool.ProcessTask()) {}
        m_futures.front().wait();
    }
};

ChainScanner::ScanContext::ScanContext(std::optional<int> max_height_in) : max_height{max_height_in} {}

std::optional<ChainScanner::QueuedBlock> ChainScanner::ReadNextBlock(ScanContext& ctx) {
    if (!ctx.next_block) return std::nullopt;
    const auto [block_hash, block_height] = *ctx.next_block;
    ctx.next_block.reset();

    // Look up the block's position separately from reading its data later,
    // because reading is slow and there might be a reorg while it is read.
    bool block_still_active = false;
    bool has_next_block = false;
    uint256 next_block_hash;
    m_wallet.chain().findBlock(block_hash, FoundBlock().inActiveChain(block_still_active).nextBlock(FoundBlock().inActiveChain(has_next_block).hash(next_block_hash)));

    // Queue the next block if it exists and is within range. Whether the scan
    // has caught up with the wallet's tip is checked after the current block
    // is processed, so blocks connected while it was being processed are not
    // missed.
    if (has_next_block && (!ctx.max_height || block_height < *ctx.max_height)) {
        ctx.next_block = {{next_block_hash, block_height + 1}};
    }

    return QueuedBlock{block_hash, block_height, block_still_active};
}

std::vector<ChainScanner::QueuedBlock> ChainScanner::ReadNextBlocks(FastWalletRescanFilter* filter, ScanContext& ctx, ScanResult& result, double& progress_current) {
    if (!ctx.checker) {
        const auto block = ReadNextBlock(ctx);
        if (!block) return {};

        if (!filter) {
            // Slow scan: no block filter index, inspect all blocks.
            return {*block};
        }

        // Single-threaded fast scan: check the filter inline, one block at a
        // time.
        filter->UpdateIfNeeded();

        auto matches_block{filter->MatchesBlock(block->hash)};

        if (matches_block.has_value() && *matches_block) {
            LogDebug(BCLog::SCAN, "Fast rescan: inspect block %d [%s] (filter matched)\n", block->height, block->hash.ToString());
            return {*block};
        } else if (!matches_block.has_value()) {
            LogDebug(BCLog::SCAN, "Fast rescan: inspect block %d [%s] (WARNING: block filter not found!)\n", block->height, block->hash.ToString());
            return {*block};
        }

        // No match: this block needs no inspection and is skipped, even if it
        // left the active chain. Record it as the most recent successfully
        // scanned block.
        result.last_scanned_block = block->hash;
        result.last_scanned_height = block->height;
        progress_current = m_wallet.chain().guessVerificationProgress(block->hash);
        return {};
    }

    // Parallel fast scan: pipeline reads and filter checks through the
    // checker.
    if (filter->NeedsUpdate()) {
        // No check may run while the filter set is updated; finishing them
        // also collects their verdicts. Those verdicts may miss the newly
        // derived scripts: patch them with the scripts the update added.
        ctx.checker->FinishChecks();
        ctx.checker->ApplyDelta(filter->UpdateIfNeeded());
    }
    ParallelFilterChecker& checker = *ctx.checker;

    std::vector<QueuedBlock> to_scan;
    while (true) {
        // Keep the checker filled with blocks and their checks running.
        bool did_read = false;
        if (!checker.Full()) {
            if (const auto block = ReadNextBlock(ctx)) {
                checker.Push(*block);
                did_read = true;
            }
        }
        if (checker.CanSubmit()) checker.Submit(/*flush=*/!did_read);

        const auto popped = checker.TryPop();
        if (!popped) {
            if (!did_read && !checker.CanSubmit()) {
                // Nothing to read or submit: stop if no verdicts are pending,
                // otherwise block for the next one instead of spinning.
                if (checker.Pending() == 0) break;
                checker.WaitForResult();
            }
            continue;
        }

        const auto& [block, filter_res] = *popped;
        if (filter_res == FILTER_NO_MATCH) {
            if (!to_scan.empty()) {
                // Stop scanning just before this block; deliver its verdict
                // again on the next call.
                checker.PushBack(block, FILTER_NO_MATCH);
                break;
            }
            // No match: this block needs no inspection and is skipped, even
            // if it left the active chain. Record it as the most recent
            // successfully scanned block.
            result.last_scanned_block = block.hash;
            result.last_scanned_height = block.height;
            progress_current = m_wallet.chain().guessVerificationProgress(block.hash);
        } else {
            if (filter_res == FILTER_MATCH)
                LogDebug(BCLog::SCAN, "Fast rescan: inspect block %d [%s] (filter matched)\n", block.height, block.hash.ToString());
            else // FILTER_NO_FILTER
                LogDebug(BCLog::SCAN, "Fast rescan: inspect block %d [%s] (WARNING: block filter not found!)\n", block.height, block.hash.ToString());

            to_scan.push_back(block);
            // Bound the number of blocks handed to the scan loop at once.
            if (to_scan.size() == MAX_BLOCKQUEUE_SIZE) break;
        }
    }
    return to_scan;
}

void ChainScanner::UpdateProgress(const LoopState& state, double progress_current, int block_height) {
    if (state.progress_end - state.progress_begin > 0.0) {
        m_scanning_progress = (progress_current - state.progress_begin) / (state.progress_end - state.progress_begin);
    } else {
        // avoid divide-by-zero for single block scan range (i.e. start and stop hashes are equal)
        m_scanning_progress = 0;
    }
    if (block_height % 100 == 0 && state.progress_end - state.progress_begin > 0.0) {
        m_wallet.ShowProgress(strprintf("[%s] %s", m_wallet.DisplayName(), _("Rescanning…")),
                              std::max(1, std::min(99, (int)(m_scanning_progress.load() * 100))));
    }
}

void ChainScanner::UpdateTipIfChanged(LoopState& state) {
    const uint256 new_tip = WITH_LOCK(m_wallet.cs_wallet, return m_wallet.GetLastBlockHash());
    if (new_tip != state.tip_hash) {
        state.tip_hash = new_tip;
        state.progress_end = m_wallet.chain().guessVerificationProgress(state.tip_hash);
    }
}

bool ChainScanner::ScanBlock(const uint256& block_hash, int block_height, bool save_progress) {
    // Read block data and locator if needed (the locator is usually null unless we need to save progress)
    CBlock block;
    CBlockLocator loc;
    // Find block
    FoundBlock found_block{FoundBlock().data(block)};
    if (save_progress) found_block.locator(loc);
    m_wallet.chain().findBlock(block_hash, found_block);

    if (block.IsNull()) return false;

    {
        LOCK(m_wallet.cs_wallet);
        for (size_t posInBlock = 0; posInBlock < block.vtx.size(); ++posInBlock) {
            m_wallet.SyncTransaction(
                block.vtx[posInBlock], TxStateConfirmed{block_hash, block_height,
                static_cast<int>(posInBlock)},
                /*rescanning_old_block=*/true);
        }

        if (!loc.IsNull()) {
            m_wallet.WalletLogPrintf("Saving scan progress %d.\n", block_height);
            WalletBatch batch(m_wallet.GetDatabase());
            batch.WriteBestBlock(loc);
        }
    }
    return true;
}

ScanResult ChainScanner::Scan(const uint256& start_block, int start_height, std::optional<int> max_height,
                              const WalletRescanReserver& reserver, bool save_progress) {
    constexpr auto INTERVAL_TIME{60s};
    auto current_time{reserver.now()};
    auto start_time{reserver.now()};

    assert(reserver.isReserved());
    auto& chain = m_wallet.chain();

    ScanContext ctx{max_height};
    std::string log_message = "slow variant inspecting all blocks";
    std::unique_ptr<FastWalletRescanFilter> fast_rescan_filter;
    if (chain.hasBlockFilterIndex(BlockFilterType::BASIC)) {
        fast_rescan_filter = std::make_unique<FastWalletRescanFilter>(m_wallet);
        if (m_wallet.m_wallet_par > 1) {
            ctx.checker = std::make_unique<ParallelFilterChecker>(*fast_rescan_filter, chain, start_height,
                "walletscan", m_wallet.m_wallet_par);
            log_message = strprintf("fast variant using block filters (%d threads)", m_wallet.m_wallet_par);
        } else {
            log_message = "fast variant using block filters";
        }
    }

    m_wallet.WalletLogPrintf("Rescan started from block %s... (%s)\n",
            start_block.ToString(), log_message);

    // show rescan progress in GUI as dialog or on splashscreen, if rescan required on startup (e.g. due to corruption)
    m_wallet.ShowProgress(strprintf("[%s] %s", m_wallet.DisplayName(), _("Rescanning…")), 0);

    ScanResult result;
    LoopState state;
    state.tip_hash = WITH_LOCK(m_wallet.cs_wallet, return m_wallet.GetLastBlockHash());
    uint256 end_hash = state.tip_hash;
    if (max_height) chain.findAncestorByHeight(state.tip_hash, *max_height, FoundBlock().hash(end_hash));
    state.progress_begin = chain.guessVerificationProgress(start_block);
    state.progress_end = chain.guessVerificationProgress(end_hash);
    double progress_current = state.progress_begin;
    ctx.next_block = {{start_block, start_height}};
    int block_height = start_height;
    bool scan_done = false;
    // Report progress for the given height, logging every INTERVAL_TIME.
    // Returns whether the interval elapsed.
    const auto update_progress = [&](int height) {
        UpdateProgress(state, progress_current, height);
        const bool next_interval = reserver.now() >= current_time + INTERVAL_TIME;
        if (next_interval) {
            current_time = reserver.now();
            m_wallet.WalletLogPrintf("Still rescanning. At block %d. Progress=%f\n", height, progress_current);
        }
        return next_interval;
    };
    while (!scan_done && (ctx.next_block || (ctx.checker && !ctx.checker->Empty())) && !m_abort && !chain.shutdownRequested()) {
        const auto last_scanned_before = result.last_scanned_height;
        const std::vector<QueuedBlock> blocks_to_scan = ReadNextBlocks(fast_rescan_filter.get(), ctx, result, progress_current);
        if (blocks_to_scan.empty() && result.last_scanned_height != last_scanned_before) {
            // Blocks were skipped via the filter: keep the reported progress
            // and the periodic log current across skipped ranges.
            block_height = *result.last_scanned_height;
            update_progress(block_height);
        }
        for (const QueuedBlock& block : blocks_to_scan) {
            block_height = block.height;

            progress_current = chain.guessVerificationProgress(block.hash);
            const bool next_interval = update_progress(block_height);

            if (!block.still_active) {
                // Abort scan if a block that needs to be inspected is no longer
                // active, to prevent marking transactions as coming from the
                // wrong block. A block skipped by the filter stays skipped:
                // it has no successor in the active chain, so the scan ends
                // successfully at the reorg point and the replacement blocks
                // are handled by blockConnected notifications.
                result.last_failed_block = block.hash;
                result.status = ScanResult::FAILURE;
                scan_done = true;
                break;
            }
            if (ScanBlock(block.hash, block_height, save_progress && next_interval)) {
                result.last_scanned_block = block.hash;
                result.last_scanned_height = block_height;
            } else {
                // could not scan block, keep scanning but record this block as the most recent failure
                result.last_failed_block = block.hash;
                result.status = ScanResult::FAILURE;
            }

            // Stop scanning once the wallet's tip is reached, re-reading the height
            // after the block was processed so a tip extension that happened
            // meanwhile is picked up. If scanning with cs_wallet locked (AttachChain),
            // blocks connected during rescan are handled after scanning is complete
            // via blockConnected notifications. Without the lock, newly added blocks
            // are re-processed here if the notifications were handled and the last
            // block height was updated.
            if (block_height >= WITH_LOCK(m_wallet.cs_wallet, return m_wallet.GetLastBlockHeight())) {
                scan_done = true;
                break;
            }

            if (!ctx.max_height) UpdateTipIfChanged(state);
        }
    }
    if (!max_height) {
        m_wallet.WalletLogPrintf("Scanning current mempool transactions.\n");
        WITH_LOCK(m_wallet.cs_wallet, chain.requestMempoolTransactions(m_wallet));
    }
    m_wallet.ShowProgress(strprintf("[%s] %s", m_wallet.DisplayName(), _("Rescanning…")), 100);
    if (m_abort) {
        m_wallet.WalletLogPrintf("Rescan aborted at block %d. Progress=%f\n", block_height, progress_current);
        result.status = ScanResult::USER_ABORT;
    } else if (chain.shutdownRequested()) {
        m_wallet.WalletLogPrintf("Rescan interrupted by shutdown request at block %d. Progress=%f\n", block_height, progress_current);
        result.status = ScanResult::USER_ABORT;
    } else {
        m_wallet.WalletLogPrintf("Rescan completed in %15dms\n", Ticks<std::chrono::milliseconds>(reserver.now() - start_time));
    }
    return result;
}
}
