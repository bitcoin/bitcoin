// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NODE_PRIVBCAST_H
#define BITCOIN_NODE_PRIVBCAST_H

#include <netbase.h>
#include <primitives/transaction.h>
#include <primitives/transaction_identifier.h>
#include <privbcast/job.h>
#include <privbcast/report.h>
#include <random.h>
#include <sync.h>
#include <threadsafety.h>
#include <uint256.h>
#include <univalue.h>
#include <util/threadpool.h>
#include <util/time.h>
#include <validationinterface.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <thread>
#include <vector>

namespace node {

/**
 * The node's private broadcast jobs (doc/design/private-broadcast-tool.md, section N): submitted
 * transactions wait in a queue, each runs as a privbcast::Job on a worker of its own, and the
 * records of finished jobs are kept, in memory only, for getprivatebroadcastinfo.
 *
 * A scheduler thread starts queued jobs in submission order, each a drawn spacing after the
 * previous start (N3) and none while another job's discovery runs (N4). It cancels every job and
 * drops the queue when networking is off (N6), and stops a job still running JOB_CAP after its
 * start on the steady clock (D2). The node registers the queue with its validation signals: a
 * transaction's arrival in the mempool is recorded for getprivatebroadcastinfo and nothing else
 * (N7).
 */
class PrivbcastQueue final : public CValidationInterface
{
public:
    enum class SubmitResult {
        /** A new job was queued. */
        Queued,
        /** A queued job, or a running one that is not being aborted, has the same wtxid (N5). */
        Covered,
        /** MAX_QUEUED_JOBS jobs are queued already (D3). */
        QueueFull,
        /** Networking is off: nothing is queued (N6). */
        NetworkOff,
        /** The node is stopping: nothing is queued (N6). */
        ShuttingDown,
    };

    /** One job, as getprivatebroadcastinfo lists it (Interface/Node). Times are Unix seconds. */
    struct JobInfo {
        Txid txid;
        Wtxid wtxid;
        /** queued, running, done or aborted. */
        std::string state;
        int64_t time_added{0};
        std::optional<int64_t> time_started;
        std::optional<int64_t> time_ended;
        std::optional<int64_t> seen_in_mempool;
        /** Why the job could not run or failed, or what stopped it other than abortprivatebroadcast. */
        std::optional<std::string> error;
        /** While it runs. */
        std::optional<privbcast::Progress> progress;
        /** Once it has run: whether an INV was fully written, and the report. */
        std::optional<bool> announced;
        std::shared_ptr<const UniValue> report;
    };

    /** A job that Abort() matched (abortprivatebroadcast: removed_transactions). */
    struct Removed {
        Txid txid;
        Wtxid wtxid;
        /** The submitted transaction. */
        CTransactionRef tx;
        /** The job's state after the call: aborted for a queued job, running for a running one,
         *  which stops shortly. */
        std::string state;
    };

    /** What a job thread runs: by default a privbcast::Job, run to its end. It passes the job's
     *  progress on as it changes and returns the report. */
    using ProgressCallback = std::function<void(const privbcast::Progress&)>;
    using JobRunner = std::function<privbcast::Report(const privbcast::JobInputs&, ProgressCallback)>;

    /** Jobs that run at once, at most. Not normative (N3, N10): 17 earlier starts fit in one
     *  JOB_CAP at the shortest spacing, plus the new one. */
    static constexpr size_t MAX_CONCURRENT_JOBS{18};

    /**
     * What one pass of the scheduler decides (N3, N4, N6, D2), from what the pass read of the queue,
     * the node and both clocks, and a spacing drawn in [START_SPACING_MIN, START_SPACING_MAX).
     * Poll() reads the inputs and acts on the decision.
     */
    struct StartPolicy {
        /** A running job, as the rule sees it. */
        struct RunningJob {
            /** Its start, on the steady clock. */
            MockableSteadyClock::time_point steady_started{};
            /** It is being cancelled. */
            bool cancelled{false};
            /** Its discovery has ended. */
            bool discovery_done{false};
        };
        /** What the pass read, under the queue's lock. */
        struct Snapshot {
            /** The node clock as the previous pass read it. */
            std::optional<NodeClock::time_point> last_poll;
            /** The end of the window that the last start, or the last step back of the clock since,
             *  opened. None before the first start. */
            std::optional<NodeClock::time_point> next_start;
            /** In the order they started. */
            std::vector<RunningJob> running;
            /** A job is queued. */
            bool queued{false};
            size_t max_concurrent{MAX_CONCURRENT_JOBS};
            bool network_active{true};
            /** The node has an onion proxy for a job that starts now. */
            bool proxy{true};
        };
        struct Decision {
            /** The end of the window after the pass. */
            std::optional<NodeClock::time_point> next_start;
            /** Once a job has started, the node clock read earlier than in the previous pass: the
             *  window runs again from now (N3). */
            bool stepped_back{false};
            /** Networking is off: cancel every running job and drop every queued one (N6). */
            bool cancel_all{false};
            /** The running jobs, by position, to stop at the cap (D2). */
            std::vector<size_t> capped;
            /** Start the job at the front of the queue. Its start opens the window (N3). */
            bool start{false};
            /** The job at the front of the queue is due, but no job can run without an onion
             *  proxy: drop every queued one. */
            bool drop_queued{false};
        };

        static Decision Decide(const Snapshot& snapshot, NodeClock::time_point now, MockableSteadyClock::time_point steady_now,
                               std::chrono::milliseconds spacing);
    };

    /**
     * @param[in] seeds           What every job gets as its seed material.
     * @param[in] network_active  Whether the node's networking is enabled.
     * @param[in] onion_proxy     The node's onion proxy, read by each pass of the scheduler: a job
     *                            gets the one read when it starts (B3).
     * @param[in] in_mempool      Whether the mempool holds a transaction with this txid. Called
     *                            without the queue's lock, as it can wait for the mempool.
     * @param[in] runner          Runs one job; a privbcast::Job when empty.
     * @param[in] max_concurrent  Jobs that run at once, at most.
     */
    PrivbcastQueue(privbcast::SeedMaterial seeds,
                   std::function<bool()> network_active,
                   std::function<std::optional<Proxy>()> onion_proxy,
                   std::function<bool(const Txid&)> in_mempool,
                   JobRunner runner = {},
                   size_t max_concurrent = MAX_CONCURRENT_JOBS);
    ~PrivbcastQueue();

    /** Start the scheduler thread. */
    void Start();
    /** Cancel every running job and drop the queue, for good: the node is stopping (N6). */
    void Interrupt() EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);
    /** Interrupt(), then wait for the scheduler and every job to end. */
    void Stop() EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);

    /** Queue a job for this transaction, as submitted (N2, N5, N6, D3). */
    SubmitResult Submit(CTransactionRef tx) EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);
    /** Every job: the retained finished ones, oldest first, then the running ones in the order
     *  they started, then the queued ones in submission order. */
    std::vector<JobInfo> Info() const EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);
    /** Abort every queued or running job whose txid or wtxid is id. A queued job is dropped at
     *  once; a running one is cancelled and ends shortly, with a report. Empty if none matched. */
    std::vector<Removed> Abort(const uint256& id) EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);

    /** The file descriptors jobs can use at once (N10): every recipient connection of
     *  max_concurrent jobs (D1) and the RESOLVE streams of one discovery (N4). */
    static size_t FileDescriptorBudget(size_t num_dns_seeds, size_t max_concurrent = MAX_CONCURRENT_JOBS);

private:
    friend struct PrivbcastQueueTest;

    enum class State { Queued, Running, Done, Aborted };

    /** One job, from submission until it is trimmed from the finished ones. Guarded by m_mutex,
     *  except that the job reads cancel without it. */
    struct JobRecord {
        JobRecord(CTransactionRef tx_in, NodeClock::time_point added)
            : txid{tx_in->GetHash()}, wtxid{tx_in->GetWitnessHash()}, tx{std::move(tx_in)}, time_added{added} {}

        const Txid txid;
        const Wtxid wtxid;
        /** The submitted transaction, held while the job is queued or running (N9). */
        CTransactionRef tx;
        State state{State::Queued};
        NodeClock::time_point time_added;
        std::optional<NodeClock::time_point> time_started;
        std::optional<NodeClock::time_point> time_ended;
        std::optional<NodeClock::time_point> seen_in_mempool;
        /** The start, on the steady clock (D2). */
        MockableSteadyClock::time_point steady_started{};
        std::optional<std::string> error;
        /** Set to cancel the running job (C8). A running job with it set is being aborted. */
        std::atomic<bool> cancel{false};
        /** As the job last passed it on. */
        privbcast::Progress progress;
        /** Which submission queued it, counted from 1: Submit() finds it again by this. */
        uint64_t sequence{0};
        std::optional<bool> announced;
        /** Set once, when the job ends, and shared with every JobInfo rather than copied. */
        std::shared_ptr<const UniValue> report;
    };

    /** The longest the scheduler waits before it reads the clocks and networking again (N6). */
    static constexpr std::chrono::milliseconds POLL_INTERVAL{500};

    const privbcast::SeedMaterial m_seeds;
    const std::function<bool()> m_network_active;
    const std::function<std::optional<Proxy>()> m_onion_proxy;
    const std::function<bool(const Txid&)> m_in_mempool;
    const JobRunner m_runner;
    const size_t m_max_concurrent;

    mutable Mutex m_mutex;
    std::condition_variable m_cv;
    /** Something changed that the scheduler acts on: it runs at once. */
    bool m_wake GUARDED_BY(m_mutex){false};
    bool m_interrupted GUARDED_BY(m_mutex){false};
    /** In submission order. */
    std::deque<std::unique_ptr<JobRecord>> m_queued GUARDED_BY(m_mutex);
    /** In the order they started. */
    std::deque<std::unique_ptr<JobRecord>> m_running GUARDED_BY(m_mutex);
    /** In the order they ended, at most MAX_FINISHED_JOBS (D3). */
    std::deque<std::unique_ptr<JobRecord>> m_finished GUARDED_BY(m_mutex);
    /** Jobs queued so far. */
    uint64_t m_submissions GUARDED_BY(m_mutex){0};
    /** The previous pass's StartPolicy::Snapshot::last_poll and next_start (N3). */
    std::optional<NodeClock::time_point> m_last_poll GUARDED_BY(m_mutex);
    std::optional<NodeClock::time_point> m_next_start GUARDED_BY(m_mutex);
    FastRandomContext m_rng GUARDED_BY(m_mutex);
    /** Runs the jobs: max_concurrent workers, from construction until Stop(). */
    ThreadPool m_job_pool{"privbcast"};
    std::thread m_scheduler;

    void ThreadScheduler() EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);
    /** One pass of the scheduler: read the clocks, networking and the proxy and act on what
     *  StartPolicy decides. */
    void Poll() EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);
    void ThreadJob(JobRecord& record, privbcast::JobInputs inputs) EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);
    /** Start the job at the front of the queue, at these clock readings. */
    void StartJob(const Proxy& proxy, NodeClock::time_point now, MockableSteadyClock::time_point steady_now) EXCLUSIVE_LOCKS_REQUIRED(m_mutex);
    /** Cancel every running job not cancelled yet, and drop every queued one, with this error. */
    void CancelAll(const std::string& error) EXCLUSIVE_LOCKS_REQUIRED(m_mutex);
    /** Keep the record of a job that has ended, dropping its transaction (N9), and trim the
     *  finished ones, oldest first (D3). */
    void Finish(std::unique_ptr<JobRecord> record, State state, NodeClock::time_point now) EXCLUSIVE_LOCKS_REQUIRED(m_mutex);
    /** A queued job, or a running one that is not being aborted, has this wtxid (N5). */
    bool HasJobFor(const Wtxid& wtxid) const EXCLUSIVE_LOCKS_REQUIRED(m_mutex);
    /** A window's length, drawn in [START_SPACING_MIN, START_SPACING_MAX) (N3). */
    std::chrono::milliseconds DrawSpacing() EXCLUSIVE_LOCKS_REQUIRED(m_mutex);
    static std::string StateName(State state);

    /** Record the first arrival of a job's txid in the mempool (N7). */
    void TransactionAddedToMempool(const NewMempoolTransactionInfo& tx, uint64_t mempool_sequence) override EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);
};

} // namespace node

#endif // BITCOIN_NODE_PRIVBCAST_H
