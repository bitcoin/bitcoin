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
                               std::function<bool()> network_active,
                               std::function<std::optional<Proxy>()> onion_proxy,
                               std::function<bool(const Txid&)> in_mempool,
                               JobRunner runner,
                               size_t max_concurrent)
    : m_seeds{std::move(seeds)},
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
            CancelAll("shutting down");
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
        for (const auto* records : {&m_queued, &m_running, &m_finished}) {
            for (const auto& record : *records) {
                if (record->sequence == sequence && !record->seen_in_mempool) record->seen_in_mempool = record->time_added;
            }
        }
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
            std::unique_ptr<JobRecord> record{std::move(*it)};
            it = m_queued.erase(it);
            LogDebug(BCLog::PRIVBROADCAST, "Dropped the queued private broadcast of txid=%s wtxid=%s",
                     record->txid.ToString(), record->wtxid.ToString());
            removed.push_back({record->txid, record->wtxid, record->tx, StateName(State::Aborted)});
            Finish(std::move(record), State::Aborted, now);
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
        std::unique_ptr<JobRecord> record{std::move(m_queued.front())};
        m_queued.pop_front();
        LogDebug(BCLog::PRIVBROADCAST, "Cannot start the private broadcast of txid=%s wtxid=%s: no Tor proxy",
                 record->txid.ToString(), record->wtxid.ToString());
        record->error = "no Tor proxy";
        Finish(std::move(record), State::Aborted, now);
    }
}

void PrivbcastQueue::StartJob(const Proxy& proxy, NodeClock::time_point now, MockableSteadyClock::time_point steady_now)
{
    AssertLockHeld(m_mutex);
    std::unique_ptr<JobRecord> owned{std::move(m_queued.front())};
    m_queued.pop_front();
    JobRecord& record{*owned};
    record.state = State::Running;
    record.time_started = now;
    record.steady_started = steady_now;
    m_running.push_back(std::move(owned));
    LogDebug(BCLog::PRIVBROADCAST, "Starting the private broadcast of txid=%s wtxid=%s, %d running",
             record.txid.ToString(), record.wtxid.ToString(), m_running.size());
    // The job gets the node's proxy and the chain's seed material, and nothing else of the node
    // (A1). It counts from the start recorded here, which the next start is spaced from, however
    // late its thread gets going (N3, D2).
    privbcast::JobInputs inputs{
        .tx = record.tx,
        .proxy = proxy,
        .seeds = m_seeds,
        .timing = privbcast::Timing{1},
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
    try {
        const privbcast::Report report{m_runner(inputs, [&](const privbcast::Progress& progress) EXCLUSIVE_LOCKS_REQUIRED(!m_mutex) {
            LOCK(m_mutex);
            record.progress = progress;
        })};
        error = report.summary.error;
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
        std::unique_ptr<JobRecord> owned{std::move(*it)};
        m_running.erase(it);
        owned->announced = announced;
        owned->report = std::move(json);
        if (error) owned->error = error;
        // A job that failed, or was cancelled before it ended, was aborted, whatever it got done.
        const State state{owned->cancel || error ? State::Aborted : State::Done};
        LogDebug(BCLog::PRIVBROADCAST, "The private broadcast of txid=%s wtxid=%s has ended: %s",
                 owned->txid.ToString(), owned->wtxid.ToString(), StateName(state));
        Finish(std::move(owned), state, NodeClock::now());
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
        std::unique_ptr<JobRecord> record{std::move(m_queued.front())};
        m_queued.pop_front();
        record->error = error;
        Finish(std::move(record), State::Aborted, now);
    }
    if (cancelled > 0 || dropped > 0) {
        LogDebug(BCLog::PRIVBROADCAST, "Cancelled %d running private broadcasts and dropped %d queued: %s", cancelled, dropped, error);
    }
}

void PrivbcastQueue::Finish(std::unique_ptr<JobRecord> record, State state, NodeClock::time_point now)
{
    AssertLockHeld(m_mutex);
    record->state = state;
    record->time_ended = now;
    record->tx.reset();
    m_finished.push_back(std::move(record));
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
    for (const auto* records : {&m_queued, &m_running, &m_finished}) {
        for (const auto& record : *records) {
            if (record->txid == txid && !record->seen_in_mempool) record->seen_in_mempool = now;
        }
    }
}

} // namespace node
