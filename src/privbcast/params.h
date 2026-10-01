// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_PRIVBCAST_PARAMS_H
#define BITCOIN_PRIVBCAST_PARAMS_H

#include <node/protocol_version.h>
#include <protocol.h>
#include <util/check.h>
#include <util/time.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <string_view>

/**
 * The values of private broadcast jobs: the Parameters section of
 * doc/design/private-broadcast.md, the same for every job of a release, followed by the
 * values the specification leaves open. Durations are unscaled: a job's own durations, those of
 * its plan and its I/O timeouts, go through Timing::Scale.
 */
namespace privbcast {

/** A closed interval of durations, [min, max]. */
struct DurationRange {
    std::chrono::milliseconds min;
    std::chrono::milliseconds max;
};

/** Query sets, each carrying one RESOLVE query of every DNS seed, all launched at job start: the
 *  queries per DNS seed (D1). */
inline constexpr int QUERIES_PER_SEED{4};
/** Job start to delivery start. Discovery ends by then (C3). */
inline constexpr std::chrono::seconds DISCOVERY_WINDOW{12};

inline constexpr size_t SLOTS{6};
/** A first attempt and three backups. */
inline constexpr size_t OPPORTUNITIES_PER_SLOT{4};

/** Where a slot's first opportunity lies: at delivery start, or in the mid or late range. */
enum class Stratum { Prompt, Mid, Late };
/** The slots' strata, fixed per release. */
inline constexpr std::array<Stratum, SLOTS> SLOT_LAYOUT{
    Stratum::Prompt, Stratum::Prompt, Stratum::Prompt, Stratum::Mid, Stratum::Late, Stratum::Late,
};
/** The DNS seeds the mainnet release lists, at least (R3): the fewest of which all but one, with
 *  QUERIES_PER_SEED answers each, fill every opportunity and number at least SLOTS, so that no two
 *  first attempts come from one seed (R5b). */
inline constexpr size_t MIN_DNS_SEEDS{7};
static_assert((MIN_DNS_SEEDS - 1) * QUERIES_PER_SEED >= SLOTS * OPPORTUNITIES_PER_SLOT, "all but one seed fill every opportunity");
static_assert(MIN_DNS_SEEDS - 1 >= SLOTS, "all but one seed give every first attempt a seed of its own");
/** The mid slot's first opportunity, after delivery start. */
inline constexpr DurationRange MID_SLOT_RANGE{35s, 180s};
/** The late slots' first opportunities, after delivery start. */
inline constexpr DurationRange LATE_SLOT_RANGE{185s, 240s};
/** The late slots' first opportunities are at least this far apart. */
inline constexpr std::chrono::seconds LATE_SLOT_SEPARATION{5};
/** A backup's scheduled start, after the previous opportunity's. */
inline constexpr DurationRange BACKUP_RANGE{50s, 60s};
/** An opportunity not dialled within this of its scheduled start is missed (C5). */
inline constexpr std::chrono::seconds START_GRACE{5};

/** Scheduled start to the announcement point. */
inline constexpr std::chrono::seconds HANDSHAKE_BUDGET{45};
static_assert(BACKUP_RANGE.min == HANDSHAKE_BUDGET + START_GRACE, "a failure before the announcement point is known by a backup's time");
/** Announcement point to the peer's request. The TX and the PING are written by its end (E6). */
inline constexpr std::chrono::seconds REQUEST_WINDOW{75};
/** The wait for the PONG after the PING is written (E6). */
inline constexpr std::chrono::seconds PONG_WAIT{10};
/** A connection that has received more than this after the proxy connected it is ended (D3). */
inline constexpr uint64_t MAX_RECV_BYTES{128 * 1024};

/** An attempt ends by its scheduled start plus this (D2). */
inline constexpr std::chrono::seconds ATTEMPT_MAX{HANDSHAKE_BUDGET + REQUEST_WINDOW + PONG_WAIT};
/** A slot ends by its first opportunity's scheduled start plus this (D2). */
inline constexpr std::chrono::milliseconds SLOT_MAX{(OPPORTUNITIES_PER_SLOT - 1) * BACKUP_RANGE.max + ATTEMPT_MAX};
/** A job's last slot ends by job start plus this (D2). */
inline constexpr std::chrono::milliseconds SCHEDULED_BOUND{DISCOVERY_WINDOW + LATE_SLOT_RANGE.max + SLOT_MAX};
// The values the specification's Parameters section derives for them.
static_assert(ATTEMPT_MAX == 130s);
static_assert(SLOT_MAX == 310s);
static_assert(SCHEDULED_BOUND == 562s);

/**
 * The job's VERSION (E1): this version and these services, time 0, addr_recv null with services
 * 0, addr_from null with these services, a random nonce, this user agent, start height 0 and
 * relay false.
 */
inline constexpr int PROFILE_VERSION{70017};
static_assert(PROFILE_VERSION == PROTOCOL_VERSION, "the profile's version tracks the release");
inline constexpr ServiceFlags PROFILE_SERVICES{NODE_WITNESS};
inline constexpr std::string_view PROFILE_USER_AGENT{"/pynode:0.0.1/"};
/** The lowest version a peer's VERSION may show for the job to announce (E2): BIP339. */
inline constexpr int MIN_PEER_PROTOCOL_VERSION{70016};

/** The node stops a job still running this long after it started, on a steady clock (D2). */
inline constexpr std::chrono::minutes JOB_CAP{10};
static_assert(SCHEDULED_BOUND <= JOB_CAP, "the cap stops only a job that outlives its schedule");
/** The node draws the spacing of job starts in [START_SPACING_MIN, START_SPACING_MAX) (N3). */
inline constexpr std::chrono::seconds START_SPACING_MIN{35};
inline constexpr std::chrono::seconds START_SPACING_MAX{55};
static_assert(START_SPACING_MIN > DISCOVERY_WINDOW, "discoveries never overlap without a clock step (N4)");
/** Jobs the node queues, and finished jobs whose reports it keeps, at most (D3). */
inline constexpr size_t MAX_QUEUED_JOBS{10'000};
inline constexpr size_t MAX_FINISHED_JOBS{100};
/** The largest regtest -privatebroadcasttimedivisor (U1). */
inline constexpr int MAX_TIME_DIVISOR{1000};

// Not normative: constrained only by C3, C5, D2, N3 and N6.

/** Each stage of a proxy stream: the connect to the proxy and each step of the SOCKS5 exchange. */
inline constexpr std::chrono::seconds SOCKS_STAGE_TIMEOUT{20};
/** How late after job start a RESOLVE query may still be launched. */
inline constexpr std::chrono::seconds QUERY_GRACE{START_GRACE};
/** The longest a job's loop waits before reading its clocks again. Not scaled. */
inline constexpr std::chrono::milliseconds LOOP_WAIT{100};

/**
 * The time divisor of a job: the regtest -privatebroadcasttimedivisor, 1 otherwise (U1). It
 * divides every duration of the plan and every I/O timeout.
 */
class Timing
{
public:
    Timing() = default;
    explicit Timing(int divisor) : m_divisor{std::clamp(divisor, 1, MAX_TIME_DIVISOR)}
    {
        Assume(m_divisor == divisor);
    }

    int Divisor() const { return m_divisor; }
    /** The duration under this timing, at millisecond resolution. */
    std::chrono::milliseconds Scale(std::chrono::milliseconds duration) const { return duration / m_divisor; }

    bool operator==(const Timing&) const = default;

private:
    int m_divisor{1};
};

} // namespace privbcast

#endif // BITCOIN_PRIVBCAST_PARAMS_H
