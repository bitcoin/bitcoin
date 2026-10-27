// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <privbcast/discovery.h>

#include <netaddress.h>
#include <netbase.h>
#include <privbcast/assign.h>
#include <privbcast/params.h>
#include <privbcast/plan.h>
#include <privbcast/socks5.h>
#include <privbcast/stream.h>
#include <util/check.h>
#include <util/log.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <exception>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace privbcast {

Discovery::Discovery(const Plan& plan, const Proxy& proxy, uint16_t default_port, FastRandomContext& rng)
    : m_timing{plan.timing},
      m_window_end{plan.timing.Scale(DISCOVERY_WINDOW)},
      m_proxy{proxy},
      m_default_port{default_port},
      m_rng{rng}
{
    for (const std::string& name : plan.seed_order) {
        SeedAnswers& seed{m_seeds.emplace_back()};
        seed.name = name;
        seed.skipped = QUERIES_PER_SEED;
    }
}

void Discovery::Start(std::chrono::milliseconds now, SteadyMs steady_now)
{
    if (!Assume(!m_end)) return;
    if (now < m_timing.Scale(QUERY_GRACE)) {
        // The credentials of each query set, shared by its queries and nothing else (B2).
        std::array<Socks5Credentials, QUERIES_PER_SEED> credentials;
        for (Socks5Credentials& set : credentials) set = Socks5Credentials::Draw(m_rng);
        m_queries.reserve(m_seeds.size() * QUERIES_PER_SEED);
        for (size_t i{0}; i < m_seeds.size(); ++i) {
            SeedAnswers& seed{m_seeds[i]};
            for (size_t set{0}; set < credentials.size(); ++set) {
                // An internal failure ends only this query (C7): one not launched yet is skipped.
                try {
                    // The bare name, one query per stream (B2).
                    ProxyStream stream{m_proxy, Socks5Client{Socks5Client::Command::Resolve, seed.name, 0, credentials[set]}, m_timing, steady_now};
                    if (!stream.Open()) {
                        LogDebug(BCLog::PROXY, "Cannot query %s through %s: %s\n", seed.name, m_proxy.ToString(), stream.Reason());
                        continue;
                    }
                    ++seed.queries;
                    --seed.skipped;
                    if (stream.GetPhase() == ProxyStream::Phase::Closed) {
                        LogDebug(BCLog::PROXY, "Query of %s through %s failed: %s\n", seed.name, m_proxy.ToString(), stream.Reason());
                    }
                    m_queries.push_back({i, set, std::move(stream)});
                } catch (const std::exception& e) {
                    LogDebug(BCLog::PRIVBROADCAST, "Query of %s failed: %s\n", seed.name, e.what());
                }
            }
        }
    }
    LogDebug(BCLog::PRIVBROADCAST, "Discovery: %d queries of %d DNS seeds started\n", m_queries.size(), m_seeds.size());
    // With no query under way, discovery is over (C3).
    if (AllEnded()) End(now);
}

std::vector<ProxyStream*> Discovery::ActiveStreams()
{
    std::vector<ProxyStream*> streams;
    for (Query& query : m_queries) {
        if (query.stream.GetPhase() != ProxyStream::Phase::Closed) streams.push_back(&query.stream);
    }
    return streams;
}

void Discovery::OnStreamEvents(ProxyStream& stream, Sock::Event occurred, std::chrono::milliseconds now, SteadyMs steady_now)
{
    // Nothing is acted on at or after the end of the window (C3, D2).
    if (!m_end && now >= m_window_end) End(m_window_end);
    if (m_end || stream.GetPhase() == ProxyStream::Phase::Closed) return;
    const auto query{std::ranges::find(m_queries, &stream, [](const Query& q) { return &q.stream; })};
    if (!Assume(query != m_queries.end())) return;
    Drive(*query, occurred, now, steady_now);
}

void Discovery::Tick(std::chrono::milliseconds now, SteadyMs steady_now)
{
    if (m_end) return;
    if (now >= m_window_end) return End(m_window_end);
    for (Query& query : m_queries) {
        if (query.stream.GetPhase() == ProxyStream::Phase::Closed) continue;
        // No event: only the stage's time is checked, and what is left to write written.
        Drive(query, 0, now, steady_now);
        if (m_end) return;
    }
}

void Discovery::Interrupt(std::chrono::milliseconds now)
{
    if (!m_end) End(now);
}

DiscoveryResult Discovery::Result() const
{
    Assume(m_end);
    return {m_seeds, m_end.value_or(std::chrono::milliseconds{0})};
}

void Discovery::Drive(Query& query, Sock::Event occurred, std::chrono::milliseconds now, SteadyMs steady_now)
{
    try {
        query.stream.OnEvents(occurred, steady_now);
        Check(query, now);
    } catch (const std::exception& e) {
        LogDebug(BCLog::PRIVBROADCAST, "Query of %s failed: %s\n", m_seeds[query.seed].name, e.what());
        query.stream.Close();
        if (AllEnded()) End(now);
    }
}

void Discovery::Check(Query& query, std::chrono::milliseconds now)
{
    SeedAnswers& seed{m_seeds[query.seed]};
    switch (query.stream.GetPhase()) {
    case ProxyStream::Phase::Connecting:
    case ProxyStream::Phase::Socks:
        return;
    case ProxyStream::Phase::Open: {
        // Only a public routable IPv4 or IPv6 address is usable (R2), whatever the node knows of it
        // (R6). The answer is counted once recorded, so that a query that fails on the way gets none.
        const std::optional<CNetAddr> answer{query.stream.Socks().Answer()};
        if (answer && (answer->IsIPv4() || answer->IsIPv6()) && answer->IsRoutable()) {
            seed.usable[query.query_set] = CService{*answer, m_default_port};
        } else {
            ++seed.rejected;
        }
        ++seed.answers;
        query.stream.Close();
        break;
    }
    case ProxyStream::Phase::Closed:
        LogDebug(BCLog::PROXY, "Query of %s through %s failed: %s\n", seed.name, m_proxy.ToString(), query.stream.Reason());
        break;
    } // no default case, so the compiler can warn about missing cases
    if (AllEnded()) End(now);
}

bool Discovery::AllEnded() const
{
    return std::ranges::all_of(m_queries, [](const Query& q) { return q.stream.GetPhase() == ProxyStream::Phase::Closed; });
}

void Discovery::End(std::chrono::milliseconds at)
{
    for (Query& query : m_queries) query.stream.Close();
    m_end = at;
    LogDebug(BCLog::PRIVBROADCAST, "Discovery ended at %d ms\n", at.count());
}

} // namespace privbcast
