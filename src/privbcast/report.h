// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_PRIVBCAST_REPORT_H
#define BITCOIN_PRIVBCAST_REPORT_H

#include <primitives/transaction_identifier.h>
#include <privbcast/assign.h>
#include <privbcast/attempt.h>
#include <privbcast/plan.h>

#include <univalue.h>

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace privbcast {

/** One attempt in the report. */
struct AttemptReport {
    /** The endpoint dialled, and where it came from. */
    Candidate candidate;
    Outcome outcome{Outcome::NotAnnounced};
    std::string reason;
    /** The opportunity's scheduled start, and the dial. */
    std::chrono::milliseconds scheduled_start{0};
    std::chrono::milliseconds started{0};
    /** The rest of its events. */
    AttemptTimes times;
    /** From the peer's VERSION. */
    std::optional<int> peer_version;
    std::optional<std::string> peer_user_agent;
    int extra_requests{0};
    uint64_t bytes_sent{0};
    uint64_t bytes_recv{0};
};

/** One slot in the report. */
struct SlotReport {
    /** Its index, stratum and times, from the plan. */
    SlotSchedule schedule;
    /** Opportunities without a candidate, and those missed (C5). */
    int empty_opportunities{0};
    int missed_opportunities{0};
    /** Stopped by cancellation. */
    bool interrupted{false};
    /** In the order they were dialled. */
    std::vector<AttemptReport> attempts;
};

/** What discovery found and the candidates it gave. */
struct DiscoveryReport : DiscoverySummary {
    /** When discovery ended (C3). */
    std::chrono::milliseconds duration{0};
};

struct Summary {
    /** Attempts dialled, and those that got that far. */
    int connections{0};
    int announcements_handed{0};
    int announcements_written{0};
    int tx_written{0};
    int pongs{0};
    /** Slots that ran to their end, cut short neither by cancellation nor by a failure of the job. */
    int slots_completed{0};
    /** The job was cancelled. */
    bool interrupted{false};
    /** Why the job failed, if it did (H2). */
    std::optional<std::string> error;
    /** When the report was made. */
    std::chrono::milliseconds duration{0};
};

/**
 * A job's report, field for field as Interface/Report has it. Every time is an offset from job
 * start on the plan clock (H1).
 */
struct Report {
    Txid txid;
    Wtxid wtxid;
    std::string chain;
    DiscoveryReport discovery;
    /** In slot order. */
    std::vector<SlotReport> slots;
    Summary summary;
};

/** The report's JSON object, with the field names and values of Interface/Report. A time is an
 *  integer number of milliseconds, or null for an event that did not happen. */
UniValue ToUniValue(const Report& report);
/** The report's discovery object. With `candidates`, what `discover` prints: the endpoints each
 *  seed was credited (seeds[].candidates) are added. */
UniValue ToUniValue(const DiscoveryReport& discovery, bool candidates);

/** The exit statuses of `bitcoin-privbcast send` (H2). A usage or input error exits with
 *  EXIT_JOB_FAILED's value too. */
inline constexpr int EXIT_ANNOUNCED{0};
inline constexpr int EXIT_JOB_FAILED{1};
inline constexpr int EXIT_NOT_ANNOUNCED{2};

/** The exit status of `send` for a job's report (H2): EXIT_ANNOUNCED if at least one INV was fully
 *  written, even if the job failed afterwards; otherwise EXIT_JOB_FAILED if it failed, and else
 *  EXIT_NOT_ANNOUNCED. */
int ExitStatus(const Report& report);

} // namespace privbcast

#endif // BITCOIN_PRIVBCAST_REPORT_H
