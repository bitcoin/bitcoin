// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_PRIVBCAST_DISCOVERY_H
#define BITCOIN_PRIVBCAST_DISCOVERY_H

#include <netbase.h>
#include <privbcast/assign.h>
#include <privbcast/params.h>
#include <privbcast/plan.h>
#include <privbcast/stream.h>
#include <util/sock.h>

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

class FastRandomContext;

namespace privbcast {

/**
 * A job's discovery: QUERIES_PER_SEED query sets, each one RESOLVE query of every DNS seed's bare
 * name through the proxy, all launched at job start (D1, R1). Each query has a stream of its own,
 * with its set's credentials (B2).
 * A query whose stream cannot be started is skipped; one whose stream fails ends without an
 * answer. An answer counts only if it arrives before the discovery window ends, and is usable
 * only as a public routable IPv4 or IPv6 address, which gets the chain's default port (R2).
 * Discovery ends when every query has ended or when the window ends, whatever is pending (C3).
 *
 * Like the attempts, it reads no clock: the job's loop passes the plan clock's offset from job
 * start and the I/O clock with every call, and waits on ActiveStreams().
 */
class Discovery
{
public:
    /**
     * @param[in] plan          The seeds, in the plan's order, and the timing.
     * @param[in] default_port  The chain's, for the usable answers.
     * @param[in] rng           Each query set's credentials are drawn from it.
     */
    Discovery(const Plan& plan, const Proxy& proxy, uint16_t default_port, FastRandomContext& rng);

    /** Launch every query now, unless now is QUERY_GRACE or more after job start (D1). A query
     *  whose stream cannot start is skipped; one whose stream closes as it starts has ended. Called
     *  once, before anything else. */
    void Start(std::chrono::milliseconds now, SteadyMs steady_now);
    /** The streams still running, for the loop's wait. */
    std::vector<ProxyStream*> ActiveStreams();
    /** Events on one of ActiveStreams(). Past the window, discovery ends instead. */
    void OnStreamEvents(ProxyStream& stream, Sock::Event occurred, std::chrono::milliseconds now, SteadyMs steady_now);
    /** End discovery once the window has passed, and the queries whose stage ran out. */
    void Tick(std::chrono::milliseconds now, SteadyMs steady_now);
    /** End discovery now, abandoning what is pending (C8). */
    void Interrupt(std::chrono::milliseconds now);

    /** Every query has ended, or the window has passed. */
    bool Done() const { return m_end.has_value(); }
    /** The end of the discovery window, from job start. */
    std::chrono::milliseconds WindowEnd() const { return m_window_end; }
    /** What discovery found, each seed's answers by query set: when they arrived plays no part
     *  (R5b). Its duration is when discovery ended. */
    DiscoveryResult Result() const;

private:
    struct Query {
        /** Index into m_seeds. */
        size_t seed{0};
        size_t query_set{0};
        ProxyStream stream;
    };

    const Timing m_timing;
    const std::chrono::milliseconds m_window_end;
    const Proxy m_proxy;
    const uint16_t m_default_port;
    FastRandomContext& m_rng;

    std::vector<SeedAnswers> m_seeds;
    /** The queries launched. Not resized after Start(), so that ActiveStreams() stay valid. */
    std::vector<Query> m_queries;
    std::optional<std::chrono::milliseconds> m_end;

    /** Drive the query's stream with the events that occurred, if any, then Check() it. A failure
     *  inside the job ends only this query, as a failure of its stream would (C7). */
    void Drive(Query& query, Sock::Event occurred, std::chrono::milliseconds now, SteadyMs steady_now);
    /** Count the query's answer, or its failure, if its stream has finished, and end discovery if
     *  every query has ended. */
    void Check(Query& query, std::chrono::milliseconds now);
    /** Every query launched has ended, or none was. */
    bool AllEnded() const;
    /** Close every stream: discovery ended at `at`. */
    void End(std::chrono::milliseconds at);
};

} // namespace privbcast

#endif // BITCOIN_PRIVBCAST_DISCOVERY_H
