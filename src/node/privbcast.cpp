// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <node/privbcast.h>

#include <kernel/mempool_entry.h>
#include <netbase.h>
#include <primitives/transaction.h>
#include <privbcast/job.h>
#include <privbcast/params.h>
#include <privbcast/report.h>
#include <sync.h>
#include <uint256.h>
#include <univalue.h>
#include <util/check.h>
#include <util/log.h>
#include <util/thread.h>
#include <util/time.h>

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstddef>
#include <exception>
#include <memory>
#include <optional>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace node {
namespace {

std::optional<int64_t> UnixSeconds(const std::optional<NodeClock::time_point>& time)
{
    if (!time) return std::nullopt;
    return TicksSinceEpoch<std::chrono::seconds>(*time);
}

} // namespace

PrivbcastQueue::PrivbcastQueue(privbcast::SeedMaterial seeds,
                               privbcast::Timing timing,
                               std::function<bool()> network_active,
                               std::function<std::optional<Proxy>()> onion_proxy,
                               std::function<bool(const Txid&)> in_mempool,
                               JobRunner runner,
                               size_t max_concurrent)
    : m_seeds{std::move(seeds)},
      m_timing{timing},
      m_network_active{std::move(network_active)},
      m_onion_proxy{std::move(onion_proxy)},
      m_in_mempool{std::move(in_mempool)},
      m_runner{runner ? std::move(runner) : JobRunner{[](const privbcast::JobInputs& inputs, ProgressCallback progress) { return privbcast::Job{inputs}.Run(progress); }}},
      m_max_concurrent{max_concurrent}
{
    m_job_pool.Start(static_cast<int>(m_max_concurrent));
}

PrivbcastQueue::~PrivbcastQueue()
{
    Stop();
}

void PrivbcastQueue::Start()
{
    m_scheduler = std::thread{&util::TraceThread, "privbcast", [this] { ThreadScheduler(); }};
}

void PrivbcastQueue::Interrupt()
{
    {
        LOCK(m_mutex);
        if (!m_interrupted) {
            m_interrupted = true;
            const auto running{std::ranges::count_if(m_running, [](const auto& record) { return !record->cancel; })};
            const size_t queued{m_queued.size()};
            CancelAll("shutting down");
            // Counts only, at the default level (N8): the operator learns that transactions may not
            // have been sent.
            if (running > 0 || queued > 0) {
                LogInfo("Shutdown stopped %d running and %d queued private broadcast jobs; submit their transactions again after restart if they have not reached the network", running, queued);
            }
        }
        m_wake = true;
    }
    m_cv.notify_all();
}

void PrivbcastQueue::Stop()
{
    Interrupt();
    if (m_scheduler.joinable()) m_scheduler.join();
    // Cancelled, the jobs end at their next step and hand back their reports on the way out.
    m_job_pool.Stop();
}

PrivbcastQueue::SubmitResult PrivbcastQueue::Submit(CTransactionRef tx)
{
    const Txid txid{tx->GetHash()};
    const Wtxid wtxid{tx->GetWitnessHash()};
    // A transaction the mempool holds at submission is seen from then on (N7). The mempool, which
    // can be busy for a while, is asked without the queue's lock, so that the scheduler and the jobs
    // never wait for it: before the record is queued and, unless it held the transaction then, once
    // more after, in case the transaction arrived in between and its notification found no record.
    const bool held{m_in_mempool(txid)};
    uint64_t sequence{0};
    {
        LOCK(m_mutex);
        if (m_interrupted) return SubmitResult::ShuttingDown;
        if (!m_network_active()) return SubmitResult::NetworkOff;
        if (HasJobFor(wtxid)) return SubmitResult::Covered;
        if (m_queued.size() >= privbcast::MAX_QUEUED_JOBS) return SubmitResult::QueueFull;
        auto record{std::make_unique<JobRecord>(std::move(tx), NodeClock::now())};
        if (held) record->seen_in_mempool = record->time_added;
        sequence = record->sequence = ++m_submissions;
        m_queued.push_back(std::move(record));
        m_wake = true;
        LogDebug(BCLog::PRIVBROADCAST, "Queued the private broadcast of txid=%s wtxid=%s, %d queued",
                 txid.ToString(), wtxid.ToString(), m_queued.size());
    }
    m_cv.notify_all();
    if (!held && m_in_mempool(txid)) {
        LOCK(m_mutex);
        // The job may have started or ended since, or its record be gone.
        ForEachRecord([&](JobRecord& record) {
            if (record.sequence == sequence && !record.seen_in_mempool) record.seen_in_mempool = record.time_added;
        });
    }
    return SubmitResult::Queued;
}

std::vector<PrivbcastQueue::JobInfo> PrivbcastQueue::Info() const
{
    LOCK(m_mutex);
    std::vector<JobInfo> jobs;
    jobs.reserve(m_finished.size() + m_running.size() + m_queued.size());
    for (const auto* records : {&m_finished, &m_running, &m_queued}) {
        for (const auto& record : *records) {
            JobInfo& info{jobs.emplace_back()};
            info.txid = record->txid;
            info.wtxid = record->wtxid;
            info.state = StateName(record->state);
            info.time_added = TicksSinceEpoch<std::chrono::seconds>(record->time_added);
            info.time_started = UnixSeconds(record->time_started);
            info.time_ended = UnixSeconds(record->time_ended);
            info.seen_in_mempool = UnixSeconds(record->seen_in_mempool);
            info.error = record->error;
            if (record->state == State::Running) info.progress = record->progress;
            info.announced = record->announced;
            info.report = record->report;
        }
    }
    return jobs;
}

std::vector<PrivbcastQueue::Removed> PrivbcastQueue::Abort(const uint256& id)
{
    std::vector<Removed> removed;
    {
        LOCK(m_mutex);
        const auto matches{[&](const JobRecord& record) { return record.txid.ToUint256() == id || record.wtxid.ToUint256() == id; }};
        for (const auto& record : m_running) {
            if (!matches(*record)) continue;
            LogDebug(BCLog::PRIVBROADCAST, "Aborting the running private broadcast of txid=%s wtxid=%s",
                     record->txid.ToString(), record->wtxid.ToString());
            record->cancel = true;
            removed.push_back({record->txid, record->wtxid, record->tx, StateName(State::Running)});
        }
        const NodeClock::time_point now{NodeClock::now()};
        for (auto it{m_queued.begin()}; it != m_queued.end();) {
            if (!matches(**it)) {
                ++it;
                continue;
            }
            JobRecord& record{**it};
            LogDebug(BCLog::PRIVBROADCAST, "Dropped the queued private broadcast of txid=%s wtxid=%s",
                     record.txid.ToString(), record.wtxid.ToString());
            removed.push_back({record.txid, record.wtxid, record.tx, StateName(State::Aborted)});
            Finish(*it, State::Aborted, now);
            it = m_queued.erase(it);
        }
    }
    return removed;
}

size_t PrivbcastQueue::FileDescriptorBudget(size_t num_dns_seeds, size_t max_concurrent)
{
    return max_concurrent * privbcast::SLOTS + size_t{privbcast::QUERIES_PER_SEED} * num_dns_seeds;
}

PrivbcastQueue::StartPolicy::Decision PrivbcastQueue::StartPolicy::Decide(const Snapshot& snapshot, NodeClock::time_point now,
                                                                        MockableSteadyClock::time_point steady_now,
                                                                        std::chrono::milliseconds spacing)
{
    Decision decision;
    decision.next_start = snapshot.next_start;
    // After the clock steps back, by any amount, the next start comes a spacing after the step
    // (N3). Before the first start there is no window to open again.
    if (snapshot.next_start && snapshot.last_poll && now < *snapshot.last_poll) {
        decision.next_start = now + spacing;
        decision.stepped_back = true;
    }
    if (!snapshot.network_active) {
        decision.cancel_all = true;
        return decision;
    }
    for (size_t i{0}; i < snapshot.running.size(); ++i) {
        const RunningJob& job{snapshot.running[i]};
        if (!job.cancelled && steady_now - job.steady_started >= privbcast::JOB_CAP) decision.capped.push_back(i);
    }
    // The start rule (N3, N4).
    if (snapshot.queued && (!decision.next_start || now >= *decision.next_start) && snapshot.running.size() < snapshot.max_concurrent &&
        std::ranges::all_of(snapshot.running, &RunningJob::discovery_done)) {
        if (snapshot.proxy) {
            decision.start = true;
            decision.next_start = now + spacing;
        } else {
            decision.drop_queued = true;
        }
    }
    return decision;
}

void PrivbcastQueue::ThreadScheduler()
{
    while (true) {
        Poll();
        WAIT_LOCK(m_mutex, lock);
        m_cv.wait_for(lock, POLL_INTERVAL, [this]() EXCLUSIVE_LOCKS_REQUIRED(m_mutex) { return m_wake; });
        m_wake = false;
        if (m_interrupted) return;
    }
}

void PrivbcastQueue::Poll()
{
    LOCK(m_mutex);
    if (m_interrupted) return;
    const NodeClock::time_point now{NodeClock::now()};
    const MockableSteadyClock::time_point steady_now{MockableSteadyClock::now()};
    const std::optional<Proxy> proxy{m_onion_proxy()};
    std::vector<StartPolicy::RunningJob> running;
    running.reserve(m_running.size());
    for (const auto& record : m_running) {
        running.push_back({.steady_started = record->steady_started,
                           .cancelled = record->cancel,
                           .discovery_done = record->progress.discovery_done});
    }
    const StartPolicy::Snapshot snapshot{
        .last_poll = m_last_poll,
        .next_start = m_next_start,
        .running = std::move(running),
        .queued = !m_queued.empty(),
        .max_concurrent = m_max_concurrent,
        .network_active = m_network_active(),
        .proxy = proxy.has_value(),
    };
    const StartPolicy::Decision decision{StartPolicy::Decide(snapshot, now, steady_now, DrawSpacing())};
    m_last_poll = now;
    m_next_start = decision.next_start;
    if (decision.stepped_back) {
        LogDebug(BCLog::PRIVBROADCAST, "The clock stepped back: the next private broadcast starts in %d ms at the earliest",
                 Ticks<std::chrono::milliseconds>(*m_next_start - now));
    }
    if (decision.cancel_all) {
        CancelAll("networking disabled");
        return;
    }
    for (const size_t i : decision.capped) {
        JobRecord& record{*m_running[i]};
        LogDebug(BCLog::PRIVBROADCAST, "Stopping the private broadcast of txid=%s wtxid=%s: it has run for %d s",
                 record.txid.ToString(), record.wtxid.ToString(), Ticks<std::chrono::seconds>(steady_now - record.steady_started));
        record.error = "cap";
        record.cancel = true;
    }
    if (decision.start) StartJob(*Assert(proxy), now, steady_now);
    while (decision.drop_queued && !m_queued.empty()) {
        JobRecord& record{*m_queued.front()};
        LogDebug(BCLog::PRIVBROADCAST, "Cannot start the private broadcast of txid=%s wtxid=%s: no Tor proxy",
                 record.txid.ToString(), record.wtxid.ToString());
        record.error = "no Tor proxy";
        Finish(m_queued.front(), State::Aborted, now);
        m_queued.pop_front();
    }
}

void PrivbcastQueue::StartJob(const Proxy& proxy, NodeClock::time_point now, MockableSteadyClock::time_point steady_now)
{
    AssertLockHeld(m_mutex);
    m_running.emplace_back(); // before anything moves, so that a failure leaves the record queued
    m_running.back() = std::move(m_queued.front());
    m_queued.pop_front();
    JobRecord& record{*m_running.back()};
    record.state = State::Running;
    record.time_started = now;
    record.steady_started = steady_now;
    LogDebug(BCLog::PRIVBROADCAST, "Starting the private broadcast of txid=%s wtxid=%s, %d running",
             record.txid.ToString(), record.wtxid.ToString(), m_running.size());
    // The job gets the node's proxy and the chain's seed material, and nothing else of the node
    // (A1) but the regtest timing (U1). It counts from the start recorded here, which the next
    // start is spaced from, however late its thread gets going (N3, D2).
    privbcast::JobInputs inputs{
        .tx = record.tx,
        .proxy = proxy,
        .seeds = m_seeds,
        .timing = m_timing,
        .cancel = &record.cancel,
        .start = now,
    };
    // The pool has a worker for every job that can run at once.
    Assume(m_job_pool.Submit([this, &record, inputs = std::move(inputs)]() mutable { ThreadJob(record, std::move(inputs)); }));
}

void PrivbcastQueue::ThreadJob(JobRecord& record, privbcast::JobInputs inputs)
{
    std::optional<bool> announced;
    std::shared_ptr<const UniValue> json;
    std::optional<std::string> error;
    // Whether a cancel cut the job short, which only its report tells: one that lands once the job
    // has ended changes nothing.
    bool interrupted{true};
    try {
        const privbcast::Report report{m_runner(inputs, [&](const privbcast::Progress& progress) EXCLUSIVE_LOCKS_REQUIRED(!m_mutex) {
            LOCK(m_mutex);
            record.progress = progress;
        })};
        error = report.summary.error;
        interrupted = report.summary.interrupted;
        json = std::make_shared<const UniValue>(privbcast::ToUniValue(report));
        announced = report.summary.announcements_written > 0;
    } catch (const std::exception& e) {
        // The job ends without a report. Its error can name a peer or the transaction, so it goes
        // to the debug log only (N8).
        LogDebug(BCLog::PRIVBROADCAST, "The private broadcast of txid=%s wtxid=%s failed: %s",
                 record.txid.ToString(), record.wtxid.ToString(), e.what());
        error = e.what();
    } catch (...) {
        // Not left to the pool, which would keep it in a future no one reads, the job never ending.
        error = "unknown exception";
    }
    inputs.tx.reset();
    {
        LOCK(m_mutex);
        const auto it{std::ranges::find_if(m_running, [&](const auto& running) { return running.get() == &record; })};
        Assert(it != m_running.end());
        record.announced = announced;
        record.report = std::move(json);
        if (error) {
            record.error = error;
        } else if (!interrupted) {
            // It ran to its end: the error a later cancel came with ("cap") does not apply.
            record.error.reset();
        }
        // A job that failed, or that a cancel cut short, was aborted, whatever it got done.
        const State state{interrupted || error ? State::Aborted : State::Done};
        LogDebug(BCLog::PRIVBROADCAST, "The private broadcast of txid=%s wtxid=%s has ended: %s",
                 record.txid.ToString(), record.wtxid.ToString(), StateName(state));
        Finish(*it, state, NodeClock::now());
        m_running.erase(it);
        m_wake = true;
    }
    m_cv.notify_all();
}

void PrivbcastQueue::CancelAll(const std::string& error)
{
    AssertLockHeld(m_mutex);
    size_t cancelled{0};
    for (const auto& record : m_running) {
        if (record->cancel) continue;
        record->error = error;
        record->cancel = true;
        ++cancelled;
    }
    const size_t dropped{m_queued.size()};
    const NodeClock::time_point now{NodeClock::now()};
    while (!m_queued.empty()) {
        m_queued.front()->error = error;
        Finish(m_queued.front(), State::Aborted, now);
        m_queued.pop_front();
    }
    if (cancelled > 0 || dropped > 0) {
        LogDebug(BCLog::PRIVBROADCAST, "Cancelled %d running private broadcasts and dropped %d queued: %s", cancelled, dropped, error);
    }
}

void PrivbcastQueue::Finish(std::unique_ptr<JobRecord>& record, State state, NodeClock::time_point now)
{
    AssertLockHeld(m_mutex);
    m_finished.emplace_back(); // the only allocation, before anything changes
    record->state = state;
    record->time_ended = now;
    record->tx.reset();
    m_finished.back() = std::move(record);
    while (m_finished.size() > privbcast::MAX_FINISHED_JOBS) m_finished.pop_front();
}

bool PrivbcastQueue::HasJobFor(const Wtxid& wtxid) const
{
    AssertLockHeld(m_mutex);
    // A job that is still to deliver covers the submission; one being aborted does not (N5).
    return std::ranges::any_of(m_queued, [&](const auto& record) { return record->wtxid == wtxid; }) ||
           std::ranges::any_of(m_running, [&](const auto& record) { return record->wtxid == wtxid && !record->cancel; });
}

std::chrono::milliseconds PrivbcastQueue::DrawSpacing()
{
    AssertLockHeld(m_mutex);
    return privbcast::START_SPACING_MIN + m_rng.randrange<std::chrono::milliseconds>(privbcast::START_SPACING_MAX - privbcast::START_SPACING_MIN);
}

std::string PrivbcastQueue::StateName(State state)
{
    switch (state) {
    case State::Queued: return "queued";
    case State::Running: return "running";
    case State::Done: return "done";
    case State::Aborted: return "aborted";
    } // no default case, so the compiler can warn about missing cases
    assert(false);
}

void PrivbcastQueue::TransactionAddedToMempool(const NewMempoolTransactionInfo& tx, uint64_t)
{
    const Txid& txid{tx.info.m_tx->GetHash()};
    const NodeClock::time_point now{NodeClock::now()};
    LOCK(m_mutex);
    ForEachRecord([&](JobRecord& record) {
        // now was read before taking the lock, so it can predate the submission; the value does not.
        if (record.txid == txid && !record.seen_in_mempool) record.seen_in_mempool = std::max(now, record.time_added);
    });
}

} // namespace node
