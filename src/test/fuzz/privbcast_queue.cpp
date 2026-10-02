// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <core_io.h>
#include <netbase.h>
#include <node/privbcast.h>
#include <primitives/transaction.h>
#include <privbcast/job.h>
#include <privbcast/params.h>
#include <privbcast/report.h>
#include <script/script.h>
#include <sync.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/util/privbcast_queue.h>
#include <test/util/random.h>
#include <test/util/time.h>
#include <uint256.h>
#include <util/time.h>

#include <algorithm>
#include <atomic>
#include <cassert>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <functional>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <thread>
#include <tuple>
#include <utility>
#include <vector>

using node::PrivbcastQueue;
using Policy = PrivbcastQueue::StartPolicy;
using SubmitResult = PrivbcastQueue::SubmitResult;
using namespace std::chrono_literals;
using std::chrono::milliseconds;

namespace {

/** Six transactions, each with two witnesses: two wtxids per txid, so that a witness variant gets a
 *  job of its own (N5). */
const std::vector<CTransactionRef>& Pool()
{
    static const std::vector<CTransactionRef> pool{[] {
        std::vector<CTransactionRef> txs;
        for (uint32_t n{0}; n < 6; ++n) {
            for (const uint8_t witness : {1, 2}) {
                CMutableTransaction tx;
                tx.vin.emplace_back(COutPoint{Txid::FromUint256(uint256::ONE), n});
                tx.vin[0].scriptWitness.stack.push_back({witness});
                tx.vout.emplace_back(10'000, CScript{} << OP_TRUE);
                txs.push_back(MakeTransactionRef(std::move(tx)));
            }
        }
        return txs;
    }()};
    return pool;
}

/**
 * Stands in for privbcast::Job on the queue's job threads. A run passes on the end of its discovery
 * and ends only when the target says, so that the target decides how far every job gets and when
 * it ends, whatever the threads do.
 */
class Runner
{
public:
    /** How a run ends: as a job that wrote this many INVs in full and, if error is set, failed with
     *  it, or as one whose run throws error. */
    struct Ending {
        int announcements{0};
        std::optional<std::string> error{};
        bool thrown{false};
    };

    privbcast::Report Run(const privbcast::JobInputs& inputs, const PrivbcastQueue::ProgressCallback& progress) EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
    {
        WAIT_LOCK(m_mutex, lock);
        Entry& run{m_runs.emplace_back()};
        run.cancel = inputs.cancel;
        m_cv.notify_all();
        while (!run.ending && !m_closed) {
            if (run.discover && !run.discovered) {
                {
                    REVERSE_LOCK(lock, m_mutex);
                    progress(privbcast::Progress{.discovery_done = true});
                }
                run.discovered = true;
                m_cv.notify_all();
                continue;
            }
            m_cv.wait(lock);
        }
        const Ending ending{run.ending.value_or(Ending{})};
        if (ending.thrown) throw std::runtime_error{ending.error.value_or("")};
        privbcast::Report report;
        report.txid = inputs.tx->GetHash();
        report.wtxid = inputs.tx->GetWitnessHash();
        report.chain = inputs.seeds.chain;
        report.summary.announcements_written = ending.announcements;
        report.summary.interrupted = inputs.cancel->load();
        report.summary.error = ending.error;
        return report;
    }

    /** Wait until n runs have begun. */
    void WaitForRuns(size_t n) EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
    {
        WAIT_LOCK(m_mutex, lock);
        assert(m_cv.wait_for(lock, 60s, [&]() EXCLUSIVE_LOCKS_REQUIRED(m_mutex) { return m_runs.size() >= n; }));
    }
    /** Have run i pass on the end of its discovery, and wait until it has. */
    void Discover(size_t i) EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
    {
        WAIT_LOCK(m_mutex, lock);
        m_runs.at(i).discover = true;
        m_cv.notify_all();
        assert(m_cv.wait_for(lock, 60s, [&]() EXCLUSIVE_LOCKS_REQUIRED(m_mutex) { return m_runs.at(i).discovered; }));
    }
    void End(size_t i, Ending ending) EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
    {
        LOCK(m_mutex);
        m_runs.at(i).ending = std::move(ending);
        m_cv.notify_all();
    }
    /** Whether the queue has cancelled run i, which has not ended. */
    bool Cancelled(size_t i) EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
    {
        LOCK(m_mutex);
        const std::atomic<bool>* cancel{m_runs.at(i).cancel};
        return cancel && cancel->load();
    }
    /** Have every run, begun or not, end at once. */
    void Close() EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
    {
        LOCK(m_mutex);
        m_closed = true;
        m_cv.notify_all();
    }

private:
    struct Entry {
        bool discover{false};
        bool discovered{false};
        std::optional<Ending> ending;
        const std::atomic<bool>* cancel{nullptr};
    };

    Mutex m_mutex;
    std::condition_variable m_cv;
    std::deque<Entry> m_runs GUARDED_BY(m_mutex);
    bool m_closed GUARDED_BY(m_mutex){false};
};

/** One queue as the start rule sees it, driven by StartPolicy as Poll() drives it. Its starts are
 *  checked as a sequence against the specification; the state the policy keeps between passes is
 *  passed on as Poll() passes it, and not checked itself. */
struct RuleQueue {
    struct Job {
        int id{0};
        NodeClock::time_point started{};
        MockableSteadyClock::time_point steady_started{};
        bool cancelled{false};
        bool discovery_done{false};
    };

    /** Jobs that run at once, at most. */
    size_t max_concurrent{PrivbcastQueue::MAX_CONCURRENT_JOBS};
    /** What Poll() keeps from one pass to the next. */
    std::optional<NodeClock::time_point> last_poll;
    std::optional<NodeClock::time_point> next_start;
    std::deque<int> queued;
    std::vector<Job> running;
    /** The last start, or the step back of the clock since, and the spacing drawn then: no job
     *  starts before that spacing has passed, and one that is due then starts (N3). */
    std::optional<NodeClock::time_point> spaced_from;
    milliseconds drawn{0};

    /** A pass of the scheduler. When sane, with the node clock never stepped back and as many
     *  jobs allowed at once as the node allows, a start that the spacing allows waits for no running
     *  job. */
    void Pass(NodeClock::time_point now, MockableSteadyClock::time_point steady_now, milliseconds spacing, bool network, bool proxy, bool sane)
    {
        // N3: once a job has started, a pass that reads the clock earlier than the one before spaces
        // the next start from now, by the pass's draw.
        if (spaced_from && last_poll && now < *last_poll) {
            spaced_from = now;
            drawn = spacing;
        }
        std::vector<Policy::RunningJob> jobs;
        jobs.reserve(running.size());
        for (const Job& job : running) jobs.push_back({.steady_started = job.steady_started, .cancelled = job.cancelled, .discovery_done = job.discovery_done});
        const Policy::Snapshot snapshot{
            .last_poll = last_poll,
            .next_start = next_start,
            .running = std::move(jobs),
            .queued = !queued.empty(),
            .max_concurrent = max_concurrent,
            .network_active = network,
            .proxy = proxy,
        };
        const Policy::Decision decision{Policy::Decide(snapshot, now, steady_now, spacing)};

        // N6: networking off cancels every running job and drops every queued one; nothing starts.
        assert(decision.cancel_all == !network);
        if (decision.cancel_all) assert(!decision.start && !decision.drop_queued && decision.capped.empty());
        // D2: a job that has run past JOB_CAP on the steady clock is stopped, unless it is being
        // cancelled already, and none is stopped before; a pass exactly at JOB_CAP may do either
        // (the invariants hold to the precision of a step).
        for (size_t i{0}; i < running.size(); ++i) {
            const auto ran{steady_now - running[i].steady_started};
            const auto capped{std::ranges::count(decision.capped, i)};
            assert(capped <= 1);
            if (!network || running[i].cancelled || ran < privbcast::JOB_CAP) assert(capped == 0);
            if (network && !running[i].cancelled && ran > privbcast::JOB_CAP) assert(capped == 1);
        }
        assert(std::ranges::all_of(decision.capped, [&](size_t i) { return i < running.size(); }));
        // N3, N4: a start, or without a proxy the drop of every queued job, needs a queued job, fewer
        // than the most jobs running, every running job's discovery ended (N4 comes first) and the
        // drawn spacing passed. A job that is due when the spacing has passed starts in that pass,
        // or at once if none has started.
        const bool acted{decision.start || decision.drop_queued};
        assert(!(decision.start && decision.drop_queued));
        if (decision.start) assert(proxy);
        if (decision.drop_queued) assert(!proxy);
        const bool ready{network && !queued.empty() && running.size() < max_concurrent &&
                         std::ranges::all_of(running, &Job::discovery_done)};
        if (acted) assert(ready && (!spaced_from || now >= *spaced_from + drawn));
        if (ready && (!spaced_from || now >= *spaced_from + drawn)) assert(acted);
        // Start times depend on no earlier job's end and on no recipient (N3): with sane clocks, a
        // discovery ends within the spacing and a job within its scheduled bound, and the spacing
        // keeps the most jobs from running at once, so a waiting job starts whatever the running
        // jobs do.
        if (sane && network && !queued.empty() && (!spaced_from || now >= *spaced_from + drawn)) assert(acted);

        last_poll = now;
        next_start = decision.next_start;
        if (decision.cancel_all) {
            for (Job& job : running) job.cancelled = true;
            queued.clear();
        }
        for (const size_t i : decision.capped) running[i].cancelled = true;
        if (decision.start) {
            running.push_back({.id = queued.front(), .started = now, .steady_started = steady_now});
            queued.pop_front();
            spaced_from = now;
            drawn = spacing;
        }
        if (decision.drop_queued) queued.clear();
    }

    /** The jobs' own clock, the node clock, reads now: a discovery ends by the end of its window
     *  (C3), a job by its scheduled bound (D2). */
    void Advance(NodeClock::time_point now)
    {
        for (Job& job : running) {
            if (now >= job.started + privbcast::DISCOVERY_WINDOW) job.discovery_done = true;
        }
        std::erase_if(running, [&](const Job& job) { return now >= job.started + privbcast::SCHEDULED_BOUND; });
    }

    /** abortprivatebroadcast: a queued job is dropped, a running one cancelled. */
    void Abort(int id)
    {
        std::erase(queued, id);
        for (Job& job : running) {
            if (job.id == id) job.cancelled = true;
        }
    }
};

} // namespace

/**
 * The start rule (N3, N4, N6, D2) through StartPolicy alone, on a queue as the rule sees it: its
 * submissions, aborts, clocks, networking and draws, and its jobs' discoveries and the jobs
 * themselves ending when the input says. The starts are checked as a sequence: none before the
 * spacing drawn at the last start or step back of the clock has passed, during a discovery or with
 * the most jobs running, and a job that is due then starts. While the node clock has not stepped
 * back and the node allows MAX_CONCURRENT_JOBS at once, that holds whatever the running jobs do:
 * start times depend on no recipient (N3).
 */
FUZZ_TARGET(privbcast_start_policy)
{
    FuzzedDataProvider provider{buffer.data(), buffer.size()};
    RuleQueue queue;
    // A lower limit makes the limit reachable.
    queue.max_concurrent = provider.ConsumeBool() ? PrivbcastQueue::MAX_CONCURRENT_JOBS : provider.ConsumeIntegralInRange<size_t>(1, PrivbcastQueue::MAX_CONCURRENT_JOBS);
    NodeClock::time_point now{std::chrono::seconds{provider.ConsumeIntegralInRange<int64_t>(0, 4'000'000'000)}};
    MockableSteadyClock::time_point steady_now{milliseconds{provider.ConsumeIntegralInRange<int64_t>(0, 4'000'000'000)}};
    bool network{true};
    bool proxy{true};
    // The node clock has stepped back: windows open again from the step, and a discovery whose
    // window it stretches can hold a start (N3, N4).
    bool stepped_back{false};
    int next_id{0};

    LIMITED_WHILE(provider.ConsumeBool(), 3'000)
    {
        CallOneOf(
            provider,
            [&] {
                // A submission.
                queue.queued.push_back(next_id++);
            },
            [&] {
                if (next_id == 0) return;
                queue.Abort(provider.ConsumeIntegralInRange<int>(0, next_id - 1));
            },
            [&] {
                // Time passes on both clocks. A cancelled job stops at its next step.
                const milliseconds elapsed{provider.ConsumeIntegralInRange<int64_t>(0, 700'000)};
                now += elapsed;
                steady_now += elapsed;
                std::erase_if(queue.running, [](const RuleQueue::Job& job) { return job.cancelled; });
                queue.Advance(now);
            },
            [&] {
                // The node clock steps alone: setmocktime, or the system clock set.
                const milliseconds step{provider.ConsumeIntegralInRange<int64_t>(-7'200'000, 7'200'000)};
                now += step;
                if (step < 0ms) stepped_back = true;
                queue.Advance(now);
            },
            [&] {
                // The steady clock runs on alone.
                steady_now += milliseconds{provider.ConsumeIntegralInRange<int64_t>(0, 700'000)};
                std::erase_if(queue.running, [](const RuleQueue::Job& job) { return job.cancelled; });
            },
            [&] { network = provider.ConsumeBool(); },
            [&] { proxy = provider.ConsumeBool(); },
            [&] {
                // The seeds have answered: a running job's discovery ends.
                if (queue.running.empty()) return;
                queue.running[provider.ConsumeIntegralInRange<size_t>(0, queue.running.size() - 1)].discovery_done = true;
            },
            [&] {
                // The recipients are done with a running job.
                if (queue.running.empty()) return;
                queue.running.erase(queue.running.begin() + provider.ConsumeIntegralInRange<size_t>(0, queue.running.size() - 1));
            },
            [&] {
                // A pass of the scheduler, with a draw of the spacing.
                const milliseconds spacing{provider.ConsumeIntegralInRange<int64_t>(milliseconds{privbcast::START_SPACING_MIN}.count(),
                                                                                    milliseconds{privbcast::START_SPACING_MAX}.count() - 1)};
                const bool sane{!stepped_back && queue.max_concurrent == PrivbcastQueue::MAX_CONCURRENT_JOBS};
                queue.Pass(now, steady_now, spacing, network, proxy, sane);
            });
    }
}

/**
 * The node's queue through its interface, with the jobs run by a Runner: submissions (N5, N6),
 * aborts, passes of the scheduler, both clocks, networking, the onion proxy, the mempool (N7),
 * discoveries and jobs that end when the target says, and shutdown. The target keeps what it alone
 * knows: what it submitted and the queue took, the runs it gave the running jobs, which of them it
 * cancelled, and the bounds of the start window, whose spacing is the queue's own draw. What Info()
 * shows is checked against the Info() before the step: what an entry keeps, and what the step
 * changes.
 */
FUZZ_TARGET(privbcast_queue)
{
    SeedRandomStateForTest(SeedRand::ZEROS);
    FuzzedDataProvider provider{buffer.data(), buffer.size()};
    const std::vector<CTransactionRef>& pool{Pool()};
    // Far enough from 0, which would end the mock, for every step back.
    NodeSeconds now{std::chrono::seconds{provider.ConsumeIntegralInRange<int64_t>(100'000'000, 4'000'000'000)}};
    FakeNodeClock clock{now};
    FakeSteadyClock steady;
    milliseconds steady_now{0};
    bool network{true};
    bool proxy{true};
    std::set<Txid> mempool;
    const Proxy tor{LookupNumeric("127.0.0.1", 9050), /*tor_stream_isolation=*/true};
    // A lower limit makes the limit reachable.
    const size_t max_concurrent{provider.ConsumeBool() ? PrivbcastQueue::MAX_CONCURRENT_JOBS : provider.ConsumeIntegralInRange<size_t>(1, PrivbcastQueue::MAX_CONCURRENT_JOBS)};
    Runner runner;
    PrivbcastQueue queue{
        privbcast::SeedMaterial{.dns_seeds = {"a.seed."}, .default_port = 18444, .chain = "regtest"},
        [&] { return network; },
        [&]() -> std::optional<Proxy> {
            if (!proxy) return std::nullopt;
            return tor;
        },
        [&](const Txid& txid) { return mempool.contains(txid); },
        [&](const privbcast::JobInputs& inputs, PrivbcastQueue::ProgressCallback progress) { return runner.Run(inputs, progress); },
        max_concurrent};

    // The jobs the queue took and that have not finished: what each carries, in the order the queue
    // took them, and for a running job its run, whether the target cancelled it
    // (abortprivatebroadcast, networking off, the cap or shutdown), whether its discovery ended and
    // when it started on the steady clock. With each, its entry as Info() last showed it.
    struct Job {
        size_t tx{0};
        std::optional<size_t> parent{};
        size_t run{0};
        bool cancel{false};
        bool discovery_done{false};
        milliseconds steady_started{0};
        PrivbcastQueue::JobInfo info{};
    };
    std::deque<Job> queued;
    std::vector<Job> running;
    // A job that finished in the current step, with the state it ends in and, if it ran to a report,
    // whether it announced.
    struct Ended {
        Job job;
        std::string state;
        std::optional<bool> announced{};
    };
    std::vector<Ended> ended;
    // The finished jobs as Info() last showed them.
    std::vector<PrivbcastQueue::JobInfo> finished;
    bool interrupted{false};
    size_t runs{0};
    // The node clock as the last pass read it, and the last start, or the last step back of the
    // clock since: the window ends a drawn spacing after it (N3).
    std::optional<NodeSeconds> last_poll;
    std::optional<NodeSeconds> window_from;
    // The transaction the queue was told arrived in the mempool, in the current step (N7).
    std::optional<Txid> arrived;

    const auto ToUnix{[](NodeSeconds time) { return TicksSinceEpoch<std::chrono::seconds>(time); }};
    const auto drop_queued{[&] {
        for (Job& job : queued) ended.push_back({.job = std::move(job), .state = "aborted"});
        queued.clear();
    }};
    const auto cancel_all{[&] {
        for (Job& job : running) job.cancel = true;
        drop_queued();
    }};

    // A submission (N5, N6). The pool is too small to fill the queue: the bound is a unit case.
    const auto submit{[&](size_t tx, std::optional<size_t> parent) {
        // A transaction does not spend itself.
        if (parent && pool[*parent]->GetHash() == pool[tx]->GetHash()) parent.reset();
        const CTransactionRef parent_tx{parent ? pool[*parent] : nullptr};
        // N5: a queued job, or a running one not being aborted, with the same wtxid covers the
        // submission; with a parent, only one with the same parent does.
        const auto covers{[&](const Job& job) {
            return pool[job.tx]->GetWitnessHash() == pool[tx]->GetWitnessHash() &&
                   (!parent || (job.parent && pool[*job.parent]->GetWitnessHash() == pool[*parent]->GetWitnessHash()));
        }};
        SubmitResult expected{SubmitResult::Queued};
        if (interrupted) {
            expected = SubmitResult::ShuttingDown;
        } else if (!network) {
            expected = SubmitResult::NetworkOff;
        } else if (std::ranges::any_of(queued, covers) || std::ranges::any_of(running, [&](const Job& job) { return !job.cancel && covers(job); })) {
            expected = SubmitResult::Covered;
        }
        assert(queue.Submit(pool[tx], parent_tx) == expected);
        if (expected != SubmitResult::Queued) return;
        // Added now, and seen from its submission if the mempool holds its txid (N7, Interface/Node).
        PrivbcastQueue::JobInfo info;
        info.txid = pool[tx]->GetHash();
        info.wtxid = pool[tx]->GetWitnessHash();
        if (parent) info.parent_txid = pool[*parent]->GetHash();
        info.state = "queued";
        info.time_added = ToUnix(now);
        if (mempool.contains(info.txid)) info.seen_in_mempool = info.time_added;
        queued.push_back({.tx = tx, .parent = parent, .info = std::move(info)});
        // N5: the same submission again queues nothing.
        assert(queue.Submit(pool[tx], parent_tx) == SubmitResult::Covered);
    }};
    // abortprivatebroadcast: it lists the jobs that Info() shows queued or running whose
    // transaction, the child for a package, has the id as its txid or wtxid. The queued ones end
    // aborted; the running ones are cancelled.
    const auto abort_job{[&](const uint256& id) {
        std::vector<PrivbcastQueue::Removed> expected;
        for (const PrivbcastQueue::JobInfo& info : queue.Info()) {
            if ((info.state != "queued" && info.state != "running") || (info.txid.ToUint256() != id && info.wtxid.ToUint256() != id)) continue;
            const auto tx{std::ranges::find(pool, info.wtxid, [](const CTransactionRef& tx) { return tx->GetWitnessHash(); })};
            expected.push_back({info.txid, info.wtxid, *tx, info.state == "queued" ? "aborted" : "running"});
        }
        // In any order.
        const auto key{[](const PrivbcastQueue::Removed& entry) { return std::tuple{entry.wtxid, entry.state, entry.txid, EncodeHexTx(*entry.tx)}; }};
        std::vector<PrivbcastQueue::Removed> removed{queue.Abort(id)};
        std::ranges::sort(removed, {}, key);
        std::ranges::sort(expected, {}, key);
        assert(std::ranges::equal(removed, expected, {}, key, key));
        const auto matches{[&](const Job& job) { return pool[job.tx]->GetHash().ToUint256() == id || pool[job.tx]->GetWitnessHash().ToUint256() == id; }};
        for (Job& job : running) job.cancel = job.cancel || matches(job);
        for (auto it{queued.begin()}; it != queued.end();) {
            if (!matches(*it)) {
                ++it;
                continue;
            }
            ended.push_back({.job = std::move(*it), .state = "aborted"});
            it = queued.erase(it);
        }
    }};

    LIMITED_WHILE(provider.ConsumeBool(), 1'000)
    {
        CallOneOf(
            provider,
            [&] {
                // sendrawtransaction, or submitpackage with a parent.
                const size_t tx{provider.ConsumeIntegralInRange<size_t>(0, pool.size() - 1)};
                std::optional<size_t> parent;
                if (provider.ConsumeBool()) parent = provider.ConsumeIntegralInRange<size_t>(0, pool.size() - 1);
                submit(tx, parent);
            },
            [&] {
                // abortprivatebroadcast, by txid or wtxid.
                uint256 id{ConsumeUInt256(provider)};
                if (provider.ConsumeBool()) {
                    const CTransactionRef& tx{pool[provider.ConsumeIntegralInRange<size_t>(0, pool.size() - 1)]};
                    id = provider.ConsumeBool() ? tx->GetHash().ToUint256() : tx->GetWitnessHash().ToUint256();
                }
                abort_job(id);
            },
            [&] {
                // Many submissions, each aborted at once, to fill the finished jobs (D3).
                for (int n{provider.ConsumeIntegralInRange<int>(0, 60)}; n > 0; --n) {
                    const size_t tx{static_cast<size_t>(n) % pool.size()};
                    submit(tx, std::nullopt);
                    abort_job(pool[tx]->GetWitnessHash().ToUint256());
                }
            },
            [&] {
                // A pass of the scheduler.
                const size_t queued_before{queued.size()};
                node::PrivbcastQueueTest::Poll(queue);
                if (interrupted) return;
                // N3: once a job has started, a pass that reads the clock earlier than the one before
                // opens the window again, from now.
                if (window_from && last_poll && now < *last_poll) window_from = now;
                last_poll = now;
                if (!network) {
                    // N6: every running job is cancelled, every queued one dropped.
                    cancel_all();
                    return;
                }
                // D2: on the steady clock. A pass exactly JOB_CAP after a start may stop that job or
                // leave it to the next pass (the invariants hold to the precision of a step), so the
                // target takes what the queue did then.
                for (Job& job : running) {
                    const auto ran{steady_now - job.steady_started};
                    job.cancel = job.cancel || ran > privbcast::JOB_CAP || (ran == privbcast::JOB_CAP && runner.Cancelled(job.run));
                }
                // N3, N4: the job at the front may start once START_SPACING_MIN has passed, and must
                // once START_SPACING_MAX has, unless the most jobs run or a discovery has not ended.
                const bool can{!queued.empty() && running.size() < max_concurrent &&
                               std::ranges::all_of(running, &Job::discovery_done)};
                const bool may{can && (!window_from || now >= *window_from + privbcast::START_SPACING_MIN)};
                const bool must{can && (!window_from || now > *window_from + privbcast::START_SPACING_MAX)};
                size_t queued_after{0};
                for (const PrivbcastQueue::JobInfo& info : queue.Info()) queued_after += info.state == "queued";
                if (proxy) {
                    const bool started{queued_after + 1 == queued_before};
                    assert(started || queued_after == queued_before);
                    assert(may || !started);
                    assert(started || !must);
                    if (!started) return;
                    Job job{std::move(queued.front())};
                    queued.pop_front();
                    window_from = now;
                    job.run = runs++;
                    job.steady_started = steady_now;
                    running.push_back(std::move(job));
                    runner.WaitForRuns(runs);
                } else {
                    // The jobs that are due cannot run: every queued one is dropped.
                    const bool dropped{queued_before > 0 && queued_after == 0};
                    assert(dropped || queued_after == queued_before);
                    assert(may || !dropped);
                    assert(dropped || !must);
                    if (dropped) drop_queued();
                }
            },
            [&] {
                // Time passes on both clocks.
                const std::chrono::seconds elapsed{provider.ConsumeIntegralInRange<int64_t>(0, 700)};
                now += elapsed;
                clock.set(now);
                steady += elapsed;
                steady_now += elapsed;
            },
            [&] {
                // The node clock steps alone, forward or back.
                now += std::chrono::seconds{provider.ConsumeIntegralInRange<int64_t>(-7'200, 7'200)};
                clock.set(now);
            },
            [&] {
                // The steady clock runs on alone.
                const milliseconds elapsed{provider.ConsumeIntegralInRange<int64_t>(0, 700'000)};
                steady += elapsed;
                steady_now += elapsed;
            },
            [&] { network = provider.ConsumeBool(); },
            [&] { proxy = provider.ConsumeBool(); },
            [&] {
                // A transaction arrives in the mempool, and the queue may be told (N7).
                const CTransactionRef& tx{pool[provider.ConsumeIntegralInRange<size_t>(0, pool.size() - 1)]};
                mempool.insert(tx->GetHash());
                if (!provider.ConsumeBool()) return;
                node::PrivbcastQueueTest::TransactionAddedToMempool(queue, tx);
                arrived = tx->GetHash();
            },
            [&] {
                // A running job's discovery ends.
                if (running.empty()) return;
                Job& job{running[provider.ConsumeIntegralInRange<size_t>(0, running.size() - 1)]};
                if (job.discovery_done) return;
                runner.Discover(job.run);
                job.discovery_done = true;
            },
            [&] {
                // A running job ends: it ran, maybe announcing, or it failed, with a report or by
                // throwing.
                if (running.empty()) return;
                const auto it{running.begin() + provider.ConsumeIntegralInRange<size_t>(0, running.size() - 1)};
                Runner::Ending ending;
                ending.announcements = provider.ConsumeBool() ? 1 : 0;
                if (provider.ConsumeBool()) {
                    ending.error = "failed";
                    ending.thrown = provider.ConsumeBool();
                }
                runner.End(it->run, ending);
                // Its worker hands the job back. No other job ends meanwhile: each waits for the target.
                while (static_cast<size_t>(std::ranges::count(queue.Info(), "running", &PrivbcastQueue::JobInfo::state)) == running.size()) {
                    std::this_thread::yield();
                }
                // A job that failed, or was cancelled before it ended, was aborted, whatever it did.
                const bool aborted{it->cancel || ending.error};
                ended.push_back({.job = std::move(*it), .state = aborted ? "aborted" : "done", .announced = ending.thrown ? std::nullopt : std::optional{ending.announcements > 0}});
                running.erase(it);
            },
            [&] {
                // Shutdown (N6).
                queue.Interrupt();
                if (!interrupted) cancel_all();
                interrupted = true;
            });

        // Info() against the Info() before the step. An entry keeps its transactions and when it
        // was added, and when it started, ended and was seen in the mempool once set, but for the
        // first arrival in the mempool the queue was told of (N7, Interface/Node).
        const auto kept{[&](const PrivbcastQueue::JobInfo& was, const PrivbcastQueue::JobInfo& is) {
            assert(is.txid == was.txid && is.wtxid == was.wtxid && is.parent_txid == was.parent_txid && is.time_added == was.time_added);
            if (was.time_started) assert(is.time_started == was.time_started);
            if (was.time_ended) assert(is.time_ended == was.time_ended);
            if (!was.seen_in_mempool && arrived == was.txid) {
                assert(is.seen_in_mempool == ToUnix(now));
            } else {
                assert(is.seen_in_mempool == was.seen_in_mempool);
            }
        }};
        // It lists the finished jobs, oldest first, then the running ones in the order they
        // started, then the queued ones in the order the queue took them (D3, N3, Interface/Node).
        // The finished ones are those finished before and those that finished in the step, but for
        // the oldest past MAX_FINISHED_JOBS.
        const std::vector<PrivbcastQueue::JobInfo> infos{queue.Info()};
        const size_t total{finished.size() + ended.size()};
        const size_t n_finished{std::min(total, privbcast::MAX_FINISHED_JOBS)};
        assert(infos.size() == n_finished + running.size() + queued.size());
        for (size_t i{0}; i < n_finished; ++i) {
            const PrivbcastQueue::JobInfo& is{infos[i]};
            assert(!is.progress && is.time_ended && (is.report != nullptr) == is.announced.has_value());
            const size_t k{total - n_finished + i};
            if (k < finished.size()) {
                const PrivbcastQueue::JobInfo& was{finished[k]};
                kept(was, is);
                assert(is.state == was.state && is.time_started == was.time_started && is.error == was.error && is.announced == was.announced && is.report == was.report);
            } else {
                // How it ended is the target's doing; its report and announcement are the run's.
                const Ended& end{ended[k - finished.size()]};
                kept(end.job.info, is);
                assert(is.state == end.state && is.time_ended == ToUnix(now) && is.time_started == end.job.info.time_started && is.announced == end.announced);
            }
        }
        for (size_t i{0}; i < running.size(); ++i) {
            const PrivbcastQueue::JobInfo& is{infos[n_finished + i]};
            const Job& job{running[i]};
            kept(job.info, is);
            assert(is.state == "running" && is.time_started && !is.time_ended && !is.announced && !is.report);
            assert(is.progress && is.progress->discovery_done == job.discovery_done);
            // Started in the step: now.
            if (job.info.state == "queued") assert(is.time_started == ToUnix(now));
        }
        for (size_t i{0}; i < queued.size(); ++i) {
            const PrivbcastQueue::JobInfo& is{infos[n_finished + running.size() + i]};
            kept(queued[i].info, is);
            assert(is.state == "queued" && !is.time_started && !is.time_ended && !is.progress && !is.announced && !is.report);
        }
        finished.assign(infos.begin(), infos.begin() + n_finished);
        for (size_t i{0}; i < running.size(); ++i) running[i].info = infos[n_finished + i];
        for (size_t i{0}; i < queued.size(); ++i) queued[i].info = infos[n_finished + running.size() + i];
        ended.clear();
        arrived.reset();
        // N9: a transaction is held only while a job that carries it is queued or running.
        for (size_t tx{0}; tx < pool.size(); ++tx) {
            const auto carries{[&](const Job& job) { return job.tx == tx || job.parent == tx; }};
            const bool held{std::ranges::any_of(queued, carries) || std::ranges::any_of(running, carries)};
            assert(held == (pool[tx].use_count() > 1));
        }
    }
    runner.Close();
    queue.Stop();
    for (const CTransactionRef& tx : pool) assert(tx.use_count() == 1);
}
