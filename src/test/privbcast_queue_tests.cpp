// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <netbase.h>
#include <node/privbcast.h>
#include <primitives/transaction.h>
#include <privbcast/job.h>
#include <privbcast/params.h>
#include <privbcast/report.h>
#include <script/script.h>
#include <sync.h>
#include <test/util/privbcast_queue.h>
#include <test/util/setup_common.h>
#include <test/util/time.h>
#include <uint256.h>
#include <univalue.h>
#include <util/time.h>

#include <boost/test/unit_test.hpp>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

using namespace std::chrono_literals;
using node::PrivbcastQueue;
using privbcast::SeedMaterial;
using SubmitResult = PrivbcastQueue::SubmitResult;

namespace {

/** The longest a test waits for a job thread. */
constexpr auto TIMEOUT{60s};
constexpr NodeSeconds T0{1'700'000'000s};

int64_t Unix(NodeSeconds time) { return TicksSinceEpoch<std::chrono::seconds>(time); }

/** A transaction with a one-byte witness. */
CTransactionRef MakeTx(uint32_t n)
{
    CMutableTransaction tx;
    tx.vin.emplace_back(COutPoint{Txid::FromUint256(uint256::ONE), n});
    tx.vin[0].scriptWitness.stack.push_back({1});
    tx.vout.emplace_back(10'000, CScript{} << OP_TRUE);
    return MakeTransactionRef(std::move(tx));
}

Proxy NodeProxy() { return Proxy{LookupNumeric("127.0.0.1", 9050), /*tor_stream_isolation=*/true}; }

SeedMaterial Seeds()
{
    return {
        .dns_seeds = {"a.seed.", "b.seed."},
        .default_port = 18444,
        .chain = "regtest",
    };
}

void WaitUntil(const std::function<bool()>& done)
{
    const auto deadline{SteadyClock::now() + TIMEOUT};
    while (!done()) {
        BOOST_REQUIRE(SteadyClock::now() < deadline);
        std::this_thread::sleep_for(1ms);
    }
}

/**
 * Stands in for privbcast::Job. Each run records the inputs it got, passes on the end of its
 * discovery when the test says so, and ends when the test says so or once its job is cancelled.
 */
class StubRunner
{
public:
    privbcast::Report Run(const privbcast::JobInputs& inputs, const PrivbcastQueue::ProgressCallback& progress) EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
    {
        WAIT_LOCK(m_mutex, lock);
        Entry& run{m_runs.emplace_back()};
        run.inputs = inputs;
        run.txid = inputs.tx->GetHash();
        run.wtxid = inputs.tx->GetWitnessHash();
        m_cv.notify_all();
        while (true) {
            if (run.discover && !run.discovered) {
                {
                    REVERSE_LOCK(lock, m_mutex);
                    progress(privbcast::Progress{.discovery_done = true});
                }
                run.discovered = true;
                m_cv.notify_all();
            }
            if (run.end || inputs.cancel->load()) break;
            m_cv.wait_for(lock, 2ms);
        }
        privbcast::Report report;
        report.txid = run.txid;
        report.wtxid = run.wtxid;
        report.chain = inputs.seeds.chain;
        report.summary.announcements_written = run.announcements;
        report.summary.interrupted = !run.end;
        report.summary.error = run.error;
        // A run keeps no transaction past its job.
        run.inputs.tx.reset();
        m_cv.notify_all();
        if (run.thrown) throw std::runtime_error{*run.error};
        return report;
    }

    void WaitForRuns(size_t n) EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
    {
        WAIT_LOCK(m_mutex, lock);
        BOOST_REQUIRE(m_cv.wait_for(lock, TIMEOUT, [&]() EXCLUSIVE_LOCKS_REQUIRED(m_mutex) { return m_runs.size() >= n; }));
    }
    size_t Runs() const EXCLUSIVE_LOCKS_REQUIRED(!m_mutex) { return WITH_LOCK(m_mutex, return m_runs.size()); }
    /** Have run i pass on the end of its discovery, and wait until it has. */
    void Discover(size_t i) EXCLUSIVE_LOCKS_REQUIRED(!m_mutex) { BOOST_REQUIRE(Discover(i, TIMEOUT)); }
    /** As above, waiting at most timeout. Returns whether it has. */
    bool Discover(size_t i, std::chrono::milliseconds timeout) EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
    {
        WAIT_LOCK(m_mutex, lock);
        m_runs.at(i).discover = true;
        m_cv.notify_all();
        return m_cv.wait_for(lock, timeout, [&]() EXCLUSIVE_LOCKS_REQUIRED(m_mutex) { return m_runs.at(i).discovered; });
    }
    /** Have run i end, with this many INVs fully written. */
    void End(size_t i, int announcements = 0) EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
    {
        LOCK(m_mutex);
        m_runs.at(i).announcements = announcements;
        m_runs.at(i).end = true;
        m_cv.notify_all();
    }
    /** Have run i end as a job that failed with this error, which its report gives or, if
     *  `thrown`, it throws. */
    void Fail(size_t i, std::string error, bool thrown) EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
    {
        LOCK(m_mutex);
        m_runs.at(i).error = std::move(error);
        m_runs.at(i).thrown = thrown;
        m_runs.at(i).end = true;
        m_cv.notify_all();
    }
    privbcast::JobInputs Inputs(size_t i) const EXCLUSIVE_LOCKS_REQUIRED(!m_mutex) { return WITH_LOCK(m_mutex, return m_runs.at(i).inputs); }
    Txid RunTxid(size_t i) const EXCLUSIVE_LOCKS_REQUIRED(!m_mutex) { return WITH_LOCK(m_mutex, return m_runs.at(i).txid); }
    /** Whether run i's job has been cancelled. Only while the job's record is kept. */
    bool Cancelled(size_t i) const EXCLUSIVE_LOCKS_REQUIRED(!m_mutex) { return WITH_LOCK(m_mutex, return m_runs.at(i).inputs.cancel->load()); }

private:
    struct Entry {
        privbcast::JobInputs inputs;
        Txid txid;
        Wtxid wtxid;
        bool discover{false};
        bool discovered{false};
        bool end{false};
        int announcements{0};
        std::optional<std::string> error;
        bool thrown{false};
    };

    mutable Mutex m_mutex;
    std::condition_variable m_cv;
    std::deque<Entry> m_runs GUARDED_BY(m_mutex);
};

/** A queue whose jobs are stub runs, with the clocks mocked, on a node whose networking, onion
 *  proxy and mempool the test sets. The scheduler runs only when the test says, unless the test
 *  starts its thread. */
struct QueueSetup : public BasicTestingSetup {
    FakeNodeClock clock{T0};
    FakeSteadyClock steady;
    std::atomic<bool> network{true};
    Mutex m_node_mutex;
    std::optional<Proxy> proxy GUARDED_BY(m_node_mutex){NodeProxy()};
    std::set<Txid> mempool GUARDED_BY(m_node_mutex);
    /** Run when the queue asks the mempool, once the mempool has answered. */
    std::function<void()> on_lookup;
    /** Run when a worker picks up a job, before its run. */
    std::function<void()> on_run;
    StubRunner stub;
    std::unique_ptr<PrivbcastQueue> queue;

    QueueSetup()
    {
        queue = std::make_unique<PrivbcastQueue>(
            Seeds(),
            [this] { return network.load(); },
            [this]() EXCLUSIVE_LOCKS_REQUIRED(!m_node_mutex) { return WITH_LOCK(m_node_mutex, return proxy); },
            [this](const Txid& txid) EXCLUSIVE_LOCKS_REQUIRED(!m_node_mutex) {
                const bool held{WITH_LOCK(m_node_mutex, return mempool.contains(txid))};
                if (on_lookup) on_lookup();
                return held;
            },
            [this](const privbcast::JobInputs& inputs, PrivbcastQueue::ProgressCallback progress) {
                if (on_run) on_run();
                return stub.Run(inputs, progress);
            });
    }

    void Poll() { node::PrivbcastQueueTest::Poll(*queue); }
    void AddToMempool(const CTransactionRef& tx) { node::PrivbcastQueueTest::TransactionAddedToMempool(*queue, tx); }
    SubmitResult Submit(const CTransactionRef& tx) { return queue->Submit(tx); }

    /** The latest job of this transaction: the last one listed with its wtxid. */
    PrivbcastQueue::JobInfo Job(const CTransactionRef& tx)
    {
        std::optional<PrivbcastQueue::JobInfo> found;
        for (PrivbcastQueue::JobInfo& info : queue->Info()) {
            if (info.wtxid == tx->GetWitnessHash()) found = std::move(info);
        }
        BOOST_REQUIRE(found);
        return *found;
    }
    std::string StateOf(const CTransactionRef& tx) { return Job(tx).state; }
    void WaitForState(const CTransactionRef& tx, const std::string& state)
    {
        WaitUntil([&] { return StateOf(tx) == state; });
    }
};

} // namespace

BOOST_FIXTURE_TEST_SUITE(privbcast_queue_tests, QueueSetup)

BOOST_AUTO_TEST_CASE(file_descriptor_budget)
{
    // N3: a start waits for a running job only if that job outlives its cap. With starts at least
    // START_SPACING_MIN apart, the jobs started within one JOB_CAP before a start, and that start,
    // must all fit.
    BOOST_CHECK_GE(PrivbcastQueue::MAX_CONCURRENT_JOBS, size_t(privbcast::JOB_CAP / privbcast::START_SPACING_MIN) + 1);
    // N10: the recipient connections of every job that can run (D1), plus one discovery's (N4).
    for (const size_t seeds : {0, 1, 5}) {
        BOOST_CHECK_EQUAL(PrivbcastQueue::FileDescriptorBudget(seeds),
                          PrivbcastQueue::MAX_CONCURRENT_JOBS * privbcast::SLOTS + size_t{privbcast::QUERIES_PER_SEED} * seeds);
    }
    BOOST_CHECK_EQUAL(PrivbcastQueue::FileDescriptorBudget(3, /*max_concurrent=*/2), 2 * privbcast::SLOTS + size_t{privbcast::QUERIES_PER_SEED} * 3);
}

BOOST_AUTO_TEST_CASE(no_start_during_a_discovery)
{
    // N4: no job starts while another's discovery runs, however long ago the window closed.
    const CTransactionRef first{MakeTx(0)}, second{MakeTx(1)};
    BOOST_CHECK(Submit(first) == SubmitResult::Queued);
    BOOST_CHECK(Submit(second) == SubmitResult::Queued);
    Poll();
    stub.WaitForRuns(1);
    clock += 1h;
    Poll();
    BOOST_CHECK_EQUAL(StateOf(second), "queued");
    BOOST_CHECK_EQUAL(Job(first).progress.value_or(privbcast::Progress{}).discovery_done, false);
    stub.Discover(0);
    BOOST_CHECK_EQUAL(Job(first).progress.value_or(privbcast::Progress{}).discovery_done, true);
    Poll();
    BOOST_CHECK_EQUAL(StateOf(second), "running");
}

BOOST_AUTO_TEST_CASE(networking_toggle_unseen_or_whole)
{
    // N6: a short networking toggle cancels everything or nothing, never part.
    const CTransactionRef running{MakeTx(0)}, queued{MakeTx(1)};
    BOOST_CHECK(Submit(running) == SubmitResult::Queued);
    BOOST_CHECK(Submit(queued) == SubmitResult::Queued);
    Poll();
    stub.WaitForRuns(1);
    stub.Discover(0);
    network = false;
    BOOST_CHECK(Submit(MakeTx(2)) == SubmitResult::NetworkOff);
    network = true;
    Poll();
    BOOST_CHECK(!stub.Cancelled(0));
    BOOST_CHECK_EQUAL(StateOf(running), "running");
    BOOST_CHECK_EQUAL(StateOf(queued), "queued");

    // The scheduler's own thread reads networking, and acts on it within a pass.
    queue->Start();
    BOOST_CHECK(node::PrivbcastQueueTest::POLL_INTERVAL <= 500ms);
    network = false;
    WaitForState(queued, "aborted");
    WaitForState(running, "aborted");
    BOOST_CHECK(Job(running).error);
}

BOOST_AUTO_TEST_CASE(failed_jobs)
{
    // Interface/Node: a failed job ends aborted with its error and report; one whose run throws,
    // with an error (here the exception's text) and no report, and it does not hold up the next
    // start (N4).
    const CTransactionRef failed{MakeTx(0)}, thrown{MakeTx(1)}, next{MakeTx(2)};
    BOOST_CHECK(Submit(failed) == SubmitResult::Queued);
    Poll();
    stub.WaitForRuns(1);
    stub.Fail(0, "failed", /*thrown=*/false);
    WaitForState(failed, "aborted");
    PrivbcastQueue::JobInfo job{Job(failed)};
    BOOST_CHECK_EQUAL(job.error.value_or(""), "failed");
    BOOST_CHECK_EQUAL(job.announced.value_or(true), false);
    BOOST_REQUIRE(job.report);
    BOOST_CHECK_EQUAL((*job.report)["summary"]["error"].get_str(), "failed");

    clock += privbcast::START_SPACING_MAX;
    BOOST_CHECK(Submit(thrown) == SubmitResult::Queued);
    Poll();
    stub.WaitForRuns(2);
    stub.Fail(1, "thrown", /*thrown=*/true);
    WaitForState(thrown, "aborted");
    job = Job(thrown);
    BOOST_CHECK_EQUAL(job.error.value_or(""), "thrown");
    BOOST_CHECK(!job.announced && !job.report);
    BOOST_CHECK_EQUAL(thrown.use_count(), 1);

    clock += privbcast::START_SPACING_MAX;
    BOOST_CHECK(Submit(next) == SubmitResult::Queued);
    Poll();
    BOOST_CHECK_EQUAL(StateOf(next), "running");
    stub.WaitForRuns(3);
    BOOST_CHECK(stub.RunTxid(2) == next->GetHash());
}

BOOST_AUTO_TEST_CASE(start_is_the_recorded_one)
{
    // N3, D2, Interface/Node: a job counts from the start the queue records, however late its
    // worker gets going: here ten seconds.
    on_run = [&] { clock += 10s; };
    const CTransactionRef tx{MakeTx(0)};
    BOOST_CHECK(Submit(tx) == SubmitResult::Queued);
    Poll();
    stub.WaitForRuns(1);
    BOOST_CHECK_EQUAL(Job(tx).time_started.value_or(0), Unix(T0));
    const std::optional<NodeClock::time_point> start{stub.Inputs(0).start};
    BOOST_REQUIRE(start);
    BOOST_CHECK(*start == T0);
    BOOST_CHECK(NodeClock::now() == T0 + 10s);
}

BOOST_AUTO_TEST_CASE(seen_in_mempool_at_submission)
{
    // N7, Interface/Node: a transaction in the mempool at submission is seen from its submission,
    // also when it arrives just after the first lookup and its notification comes before the job is
    // queued. A later notification changes nothing.
    for (const bool held : {true, false}) {
        const CTransactionRef tx{MakeTx(held ? 0 : 1)};
        if (held) WITH_LOCK(m_node_mutex, mempool.insert(tx->GetHash()));
        int lookups{0};
        std::thread notifier;
        std::atomic<bool> delivered{false};
        on_lookup = [&] {
            if (++lookups > 1) return;
            clock += 1s;
            WITH_LOCK(m_node_mutex, mempool.insert(tx->GetHash()));
            notifier = std::thread{[&] {
                AddToMempool(tx);
                delivered = true;
            }};
            for (int waited{0}; waited < 100 && !delivered; ++waited) std::this_thread::sleep_for(1ms);
            BOOST_CHECK(delivered);
        };
        BOOST_CHECK(Submit(tx) == SubmitResult::Queued);
        on_lookup = nullptr;
        BOOST_REQUIRE(notifier.joinable());
        notifier.join();
        BOOST_CHECK_EQUAL(lookups, held ? 1 : 2);
        const PrivbcastQueue::JobInfo job{Job(tx)};
        BOOST_CHECK_EQUAL(job.seen_in_mempool.value_or(0), job.time_added);
        clock += 1s;
        AddToMempool(tx);
        BOOST_CHECK_EQUAL(Job(tx).seen_in_mempool.value_or(0), job.time_added);
    }
}

BOOST_AUTO_TEST_CASE(mempool_lookup_outside_the_lock)
{
    // N6: while a submission waits for the mempool, a running job passes on its progress and
    // networking off cancels it within a second; the submission is then refused.
    const CTransactionRef running{MakeTx(0)}, submitted{MakeTx(1)};
    BOOST_CHECK(Submit(running) == SubmitResult::Queued);
    queue->Start();
    stub.WaitForRuns(1);
    Mutex mutex;
    std::condition_variable cv;
    bool waiting{false}, answer{false};
    on_lookup = [&] {
        WAIT_LOCK(mutex, lock);
        waiting = true;
        cv.notify_all();
        cv.wait(lock, [&] { return answer; });
    };
    std::optional<SubmitResult> result;
    std::thread submitter{[&] { result = Submit(submitted); }};
    {
        WAIT_LOCK(mutex, lock);
        BOOST_CHECK(cv.wait_for(lock, TIMEOUT, [&] { return waiting; }));
    }
    // Bounded waits, and no check that ends the test, until the answer is given: the queue cannot
    // stop before.
    BOOST_CHECK(stub.Discover(0, 1s));
    network = false;
    const auto off{SteadyClock::now()};
    while (!stub.Cancelled(0) && SteadyClock::now() < off + 1s) std::this_thread::sleep_for(1ms);
    BOOST_CHECK(stub.Cancelled(0));
    {
        LOCK(mutex);
        answer = true;
    }
    cv.notify_all();
    submitter.join();
    on_lookup = nullptr;
    BOOST_CHECK(result == SubmitResult::NetworkOff);
    WaitForState(running, "aborted");
    BOOST_CHECK(Job(running).error);
    BOOST_CHECK_EQUAL(queue->Info().size(), 1U);
}

BOOST_AUTO_TEST_CASE(transactions_dropped_hashes_kept)
{
    // N9: a finished job drops its transaction and keeps its hashes, and cannot be aborted.
    const CTransactionRef done{MakeTx(0)}, dropped{MakeTx(1)}, cut{MakeTx(2)};
    for (const CTransactionRef& tx : {done, dropped, cut}) BOOST_CHECK(Submit(tx) == SubmitResult::Queued);
    Poll();
    stub.WaitForRuns(1);
    BOOST_CHECK_GT(done.use_count(), 1);
    BOOST_CHECK_GT(dropped.use_count(), 1);
    BOOST_CHECK_EQUAL(queue->Abort(dropped->GetWitnessHash().ToUint256()).size(), 1U);
    BOOST_CHECK_EQUAL(dropped.use_count(), 1);
    stub.Discover(0);
    stub.End(0);
    WaitForState(done, "done");
    BOOST_CHECK_EQUAL(done.use_count(), 1);
    network = false;
    Poll();
    BOOST_CHECK_EQUAL(cut.use_count(), 1);

    for (const CTransactionRef& tx : {done, dropped, cut}) {
        const PrivbcastQueue::JobInfo job{Job(tx)};
        BOOST_CHECK(job.txid == tx->GetHash());
        BOOST_CHECK(queue->Abort(tx->GetHash().ToUint256()).empty());
        BOOST_CHECK(queue->Abort(tx->GetWitnessHash().ToUint256()).empty());
    }
}

BOOST_AUTO_TEST_CASE(cap)
{
    // D2: the node stops a job still running JOB_CAP after its start, on the steady clock, and not
    // before.
    const CTransactionRef tx{MakeTx(0)};
    BOOST_CHECK(Submit(tx) == SubmitResult::Queued);
    Poll();
    stub.WaitForRuns(1);
    stub.Discover(0);
    clock += 1h;
    steady += privbcast::JOB_CAP - 1s;
    Poll();
    BOOST_CHECK(!stub.Cancelled(0));
    BOOST_CHECK(!Job(tx).error);
    steady += 2s;
    Poll();
    BOOST_CHECK(stub.Cancelled(0));
    BOOST_CHECK(Job(tx).error);
    WaitForState(tx, "aborted");
    const PrivbcastQueue::JobInfo job{Job(tx)};
    BOOST_CHECK(job.error);
    BOOST_REQUIRE(job.report);
    BOOST_CHECK_EQUAL((*job.report)["summary"]["interrupted"].get_bool(), true);
}

BOOST_AUTO_TEST_CASE(queue_bound)
{
    // D3: the queue holds MAX_QUEUED_JOBS jobs and refuses another, but a submission that a queued
    // job covers still succeeds (N5).
    const uint32_t bound{static_cast<uint32_t>(privbcast::MAX_QUEUED_JOBS)};
    for (uint32_t n{0}; n < bound; ++n) BOOST_REQUIRE(Submit(MakeTx(n)) == SubmitResult::Queued);
    BOOST_CHECK(Submit(MakeTx(bound)) == SubmitResult::QueueFull);
    BOOST_CHECK(Submit(MakeTx(0)) == SubmitResult::Covered);
}

BOOST_AUTO_TEST_CASE(window_closes_at_its_end)
{
    // N3: a job waiting for the window does not start while it is open, and starts in a pass after
    // it has closed.
    using Policy = PrivbcastQueue::StartPolicy;
    const NodeClock::time_point end{T0 + 1h};
    const Policy::Snapshot snapshot{.last_poll = end - 2s, .next_start = end, .running = {}, .queued = true};
    BOOST_CHECK(!Policy::Decide(snapshot, end - 1s, MockableSteadyClock::time_point{}, privbcast::START_SPACING_MIN).start);
    BOOST_CHECK(Policy::Decide(snapshot, end + 1s, MockableSteadyClock::time_point{}, privbcast::START_SPACING_MIN).start);
}

BOOST_AUTO_TEST_CASE(info_order_and_fields)
{
    // Interface/Node: finished jobs oldest first, then running, then queued, each with its fields.
    const CTransactionRef done{MakeTx(0)}, running{MakeTx(1)}, aborted{MakeTx(2)}, queued{MakeTx(3)};
    BOOST_CHECK(Submit(done) == SubmitResult::Queued);
    Poll();
    stub.WaitForRuns(1);
    stub.Discover(0);
    clock += 55s;
    BOOST_CHECK(Submit(running) == SubmitResult::Queued);
    Poll();
    stub.WaitForRuns(2);
    stub.Discover(1);
    clock += 1s;
    BOOST_CHECK(Submit(aborted) == SubmitResult::Queued);
    BOOST_CHECK(Submit(queued) == SubmitResult::Queued);
    clock += 1s;
    stub.End(0, /*announcements=*/2);
    WaitForState(done, "done");
    clock += 1s;
    BOOST_CHECK_EQUAL(queue->Abort(aborted->GetWitnessHash().ToUint256()).size(), 1U);

    const std::vector<PrivbcastQueue::JobInfo> jobs{queue->Info()};
    const std::vector<CTransactionRef> order{done, aborted, running, queued};
    BOOST_REQUIRE_EQUAL(jobs.size(), order.size());
    for (size_t i{0}; i < jobs.size(); ++i) {
        BOOST_CHECK(jobs[i].txid == order[i]->GetHash());
        BOOST_CHECK(jobs[i].wtxid == order[i]->GetWitnessHash());
        BOOST_CHECK(!jobs[i].error);
        BOOST_CHECK(!jobs[i].seen_in_mempool);
    }

    const PrivbcastQueue::JobInfo& finished{jobs[0]};
    BOOST_CHECK_EQUAL(finished.state, "done");
    BOOST_CHECK_EQUAL(finished.time_added, Unix(T0));
    BOOST_CHECK_EQUAL(finished.time_started.value_or(0), Unix(T0));
    BOOST_CHECK_EQUAL(finished.time_ended.value_or(0), Unix(T0 + 57s));
    BOOST_CHECK(!finished.progress);
    BOOST_CHECK_EQUAL(finished.announced.value_or(false), true);
    privbcast::Report report;
    report.txid = done->GetHash();
    report.wtxid = done->GetWitnessHash();
    report.chain = "regtest";
    report.summary.announcements_written = 2;
    BOOST_REQUIRE(finished.report);
    BOOST_CHECK_EQUAL(finished.report->write(), privbcast::ToUniValue(report).write());
    // The record keeps the report once: every Info() shares it.
    BOOST_CHECK(queue->Info().front().report == finished.report);

    const PrivbcastQueue::JobInfo& dropped{jobs[1]};
    BOOST_CHECK_EQUAL(dropped.state, "aborted");
    BOOST_CHECK_EQUAL(dropped.time_added, Unix(T0 + 56s));
    BOOST_CHECK(!dropped.time_started);
    BOOST_CHECK_EQUAL(dropped.time_ended.value_or(0), Unix(T0 + 58s));
    BOOST_CHECK(!dropped.progress && !dropped.announced && !dropped.report);

    const PrivbcastQueue::JobInfo& run{jobs[2]};
    BOOST_CHECK_EQUAL(run.state, "running");
    BOOST_CHECK_EQUAL(run.time_added, Unix(T0 + 55s));
    BOOST_CHECK_EQUAL(run.time_started.value_or(0), Unix(T0 + 55s));
    BOOST_CHECK(!run.time_ended);
    BOOST_CHECK(run.progress == (privbcast::Progress{.discovery_done = true}));
    BOOST_CHECK(!run.announced && !run.report);

    const PrivbcastQueue::JobInfo& waiting{jobs[3]};
    BOOST_CHECK_EQUAL(waiting.state, "queued");
    BOOST_CHECK_EQUAL(waiting.time_added, Unix(T0 + 56s));
    BOOST_CHECK(!waiting.time_started && !waiting.time_ended);
    BOOST_CHECK(!waiting.progress && !waiting.announced && !waiting.report);
}

BOOST_AUTO_TEST_CASE(job_inputs)
{
    // U3, N2, A1: a job's inputs are the submitted transaction, the node's onion proxy as it is
    // when the job starts, the chain's seed material, divisor 1 and its cancel flag.
    const CTransactionRef tx{MakeTx(0)};
    BOOST_CHECK(Submit(tx) == SubmitResult::Queued);
    const Proxy started_with{LookupNumeric("127.0.0.1", 9150), /*tor_stream_isolation=*/false};
    WITH_LOCK(m_node_mutex, proxy = started_with);
    Poll();
    stub.WaitForRuns(1);
    const privbcast::JobInputs inputs{stub.Inputs(0)};
    BOOST_CHECK(inputs.tx == tx);
    BOOST_CHECK_EQUAL(inputs.proxy.ToString(), started_with.ToString());
    BOOST_CHECK_EQUAL(inputs.proxy.m_tor_stream_isolation, false);
    BOOST_CHECK(inputs.seeds == Seeds());
    BOOST_CHECK_EQUAL(inputs.timing.Divisor(), 1);
    BOOST_REQUIRE(inputs.cancel);
    BOOST_CHECK(!inputs.cancel->load());
}

BOOST_AUTO_TEST_CASE(no_proxy_drops_the_queue)
{
    // Interface/Node: without an onion proxy when the job at the front is due, every queued job is
    // dropped.
    const CTransactionRef first{MakeTx(0)}, second{MakeTx(1)};
    BOOST_CHECK(Submit(first) == SubmitResult::Queued);
    BOOST_CHECK(Submit(second) == SubmitResult::Queued);
    WITH_LOCK(m_node_mutex, proxy.reset());
    Poll();
    for (const CTransactionRef& tx : {first, second}) {
        const PrivbcastQueue::JobInfo job{Job(tx)};
        BOOST_CHECK_EQUAL(job.state, "aborted");
        BOOST_CHECK(job.error);
        BOOST_CHECK(!job.time_started && !job.report);
    }
    BOOST_CHECK_EQUAL(stub.Runs(), 0U);
}

BOOST_AUTO_TEST_SUITE_END()
