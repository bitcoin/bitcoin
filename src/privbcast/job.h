// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_PRIVBCAST_JOB_H
#define BITCOIN_PRIVBCAST_JOB_H

#include <net.h>
#include <netbase.h>
#include <primitives/transaction.h>
#include <privbcast/assign.h>
#include <privbcast/attempt.h>
#include <privbcast/discovery.h>
#include <privbcast/params.h>
#include <privbcast/plan.h>
#include <privbcast/report.h>
#include <privbcast/stream.h>
#include <random.h>
#include <util/sock.h>
#include <util/time.h>

#include <array>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace privbcast {

/** What a job works from, besides the clocks and fresh randomness (A1). */
/** The chain's release seed material (A1, R1). */
struct SeedMaterial {
    std::vector<std::string> dns_seeds;
    uint16_t default_port{0};
    /** The chain's name, for the report. */
    std::string chain;

    bool operator==(const SeedMaterial&) const = default;
};

struct JobInputs {
    /** The transaction to broadcast. */
    CTransactionRef tx;
    /** The SOCKS5 proxy that every stream goes through (B1). */
    Proxy proxy;
    SeedMaterial seeds;
    Timing timing;
    /** Set by the owner to cancel the job (C8). May be null. */
    const std::atomic<bool>* cancel{nullptr};
    /** Passed to every attempt. Empty in production (A4). */
    KeySource keys{};
    /** Job start on the plan clock, as the owner read it when it started the job. Empty: the job
     *  reads the clock when it is made. */
    std::optional<NodeClock::time_point> start{};
};

/** How far a running job has got (Interface/Node: progress). */
struct Progress {
    /** Discovery has ended. */
    bool discovery_done{false};
    /** Opportunities that were empty or missed, counted once their scheduled time has passed, and
     *  those whose attempt has ended. */
    int opportunities_ended{0};
    /** Attempts that have ended, and those of them whose INV was fully written. */
    int connections{0};
    int announcements_written{0};

    bool operator==(const Progress&) const = default;
};

/**
 * One private broadcast of one transaction: discovery, the slots' opportunities as the plan
 * schedules them, and the report (doc/design/private-broadcast.md).
 *
 * The plan is drawn at construction, before any socket exists (C1), and job start is read then
 * unless the inputs give it. Discovery starts with the first Step(); delivery starts at the end of
 * the discovery window, with the candidates fixed when discovery ended (C2, C3). The slots run side
 * by side on one non-blocking event loop (C7), each dialling its opportunities at their scheduled
 * times (C4-C6). The job ends with its last slot (D2).
 *
 * An exception thrown while the loop acts on one attempt or one discovery query ends only that
 * attempt or query, as a failure of its stream would (C7). One thrown anywhere else in a step fails
 * the job: it stops as a cancelled one does, and its report gives the error (H2).
 *
 * Its only effects are on sockets made through CreateSock to the proxy (A3, B1). The plan clock
 * is NodeClock: the schedule, the deadlines and the report's times, which are offsets from the
 * reading taken at job start (H1). The I/O clock, MockableSteadyClock, times the proxy streams.
 */
class Job
{
public:
    /** @param[in] rng  The plan, the proxy credentials and the VERSION and PING nonces are drawn
     *                  from it; the BIP324 keys and garbage come from the inputs' key source.
     *                  Null: a fresh context seeded from the OS (A4). */
    explicit Job(JobInputs inputs, std::unique_ptr<FastRandomContext> rng = nullptr);

    /** Run the job to its end, on the caller's thread, and return its report. Each change of
     *  Snapshot() is passed to progress, if set. */
    Report Run(const std::function<void(const Progress&)>& progress = {});
    /** One iteration of the loop: the deadlines and the dials that are due, a wait on the sockets
     *  of at most max_wait and no longer than the next deadline, and the I/O. True once the job
     *  has ended, its report made. Only a failure to make the report throws. */
    bool Step(std::chrono::milliseconds max_wait);
    /** The report, once Step() returned true. */
    Report TakeReport();
    /** As of the last Step(). */
    Progress Snapshot() const { return m_progress; }
    const Plan& GetPlan() const { return m_plan; }

private:
    /** A dialled opportunity. */
    struct Running {
        size_t opportunity{0};
        Candidate candidate;
        /** The dial. */
        std::chrono::milliseconds started{0};
        std::unique_ptr<Attempt> attempt;
        ProxyStream stream;
    };

    struct Slot {
        /** Each opportunity has ended: empty or missed once its scheduled time passed, or its
         *  attempt has ended (Interface/Node: progress). */
        std::array<bool, OPPORTUNITIES_PER_SLOT> opportunity_ended{};
        /** The next opportunity to dial or to miss. */
        size_t next{0};
        int missed{0};
        std::optional<Running> running;
        /** The attempts that have ended, in the order they were dialled. */
        std::vector<AttemptReport> attempts;
        /** An attempt reached the announcement point: nothing else in the slot runs (C6). */
        bool announced{false};
        bool ended{false};
        /** It ran to its end, not cut short by cancellation or by a failure of the job. */
        bool completed{false};
        bool interrupted{false};
    };

    const JobInputs m_inputs;
    const std::unique_ptr<FastRandomContext> m_rng;
    /** Job start, on the plan clock. */
    const NodeClock::time_point m_start;
    const Plan m_plan;
    Discovery m_discovery;
    /** When discovery ended. */
    std::chrono::milliseconds m_discovery_duration{0};
    /** Fixed when discovery ends (C2). */
    std::optional<Assignment> m_assignment;
    std::array<Slot, SLOTS> m_slots;

    bool m_started{false};
    bool m_interrupted{false};
    /** Why the job failed, if it did. */
    std::optional<std::string> m_error;
    /** Names the next attempt's connection in the transport's log. */
    NodeId m_next_id{0};
    /** What a read from a recipient goes into. */
    std::vector<uint8_t> m_buffer;
    std::optional<Report> m_report;

    Progress m_progress;

    /** The plan clock, as an offset from job start. */
    std::chrono::milliseconds PlanNow() const;
    bool Cancelled() const;
    /** Step() but for the failure of the job and the report: when the job ended, if it has. */
    std::optional<std::chrono::milliseconds> Iterate(std::chrono::milliseconds max_wait);
    /** Discovery and every slot, as of now: deadlines, then dials and slot ends. */
    void Advance(std::chrono::milliseconds now, SteadyMs steady_now);
    void AdvanceSlot(size_t index, std::chrono::milliseconds now, SteadyMs steady_now);
    void EndDiscovery();
    void Dial(size_t index, size_t opportunity, std::chrono::milliseconds now, SteadyMs steady_now);
    /** Record the slot's attempt, which has ended, and close its stream. */
    void Reap(size_t index);
    /** Drive the slot's attempt with the events of its stream. */
    void OnAttemptEvents(size_t index, Sock::Event occurred, SteadyMs steady_now);
    /** Write what the slot's attempt has to send, as much as its socket takes, each write marked
     *  with the plan clock's time when it completed. */
    void Write(size_t index);
    /** The attempt's stream has closed: the peer closed it, or the proxy or the socket failed. */
    void StreamClosed(Running& run, std::chrono::milliseconds now);
    /** Acting on the attempt threw `what` (C7): close its stream and end it as a failed stream
     *  would. */
    void AttemptFailed(Running& run, std::chrono::milliseconds now, const std::string& what);
    /** How long the loop may wait: until the next deadline or due opportunity, at most max_wait. */
    std::chrono::milliseconds NextWait(std::chrono::milliseconds now, SteadyMs steady_now, std::chrono::milliseconds max_wait);
    bool Over() const;
    /** How the log names the job: by its transaction, if it has one. */
    std::string LogName() const;
    /** Stop everything now (C8). */
    void Cancel(std::chrono::milliseconds now);
    /** The job failed with `what`, outside any one attempt or query: stop everything now. */
    void Fail(std::chrono::milliseconds now, const std::string& what);
    /** End discovery and every slot still running, each running attempt by its state as a
     *  cancellation ends it, with the job's error, if any, as its reason. */
    void Stop(std::chrono::milliseconds now);
    /** Make the report. */
    void Finish(std::chrono::milliseconds now);
    void UpdateProgress();
};

} // namespace privbcast

#endif // BITCOIN_PRIVBCAST_JOB_H
