// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <compat/compat.h>
#include <netaddress.h>
#include <netbase.h>
#include <privbcast/assign.h>
#include <privbcast/discovery.h>
#include <privbcast/params.h>
#include <privbcast/plan.h>
#include <privbcast/stream.h>
#include <random.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/util/net.h>
#include <tinyformat.h>
#include <util/sock.h>

#include <algorithm>
#include <array>
#include <cassert>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <deque>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

using namespace privbcast;
using namespace std::chrono_literals;
using std::chrono::milliseconds;

namespace {

/** The I/O clock at job start. It moves with the plan clock, so no stage of an exchange runs out
 *  before the window does. */
constexpr SteadyMs STEADY{1h};

/** An IPv4 address from its value in host byte order. */
CNetAddr IPv4(uint32_t ip)
{
    in_addr addr;
    addr.s_addr = htonl(ip);
    return CNetAddr{addr};
}

/** What the proxy does with one query's stream. */
struct Script {
    enum class Connect {
        /** The connect completes at once. */
        Now,
        /** CreateSock fails, or the connect fails at once: the query is skipped. */
        NoSocket,
        Refused,
        /** The connect is in progress until the stream's turn, and then succeeds or fails. */
        Later,
        LaterFails,
    };
    enum class Reply { Address, Name, Error, Close, Silent };

    Connect connect{Connect::Now};
    /** What the proxy does at the stream's turn: answer the request with an address, a name or an
     *  error, close the stream, or stay silent. */
    Reply reply{Reply::Silent};
    /** The bound address of an Address reply: an IPv4 address, or else an IPv6 one. */
    std::vector<uint8_t> address;

    bool Opens() const { return connect != Connect::NoSocket && connect != Connect::Refused; }
    /** The query ends at its turn: with an answer, or failed. */
    bool Ends() const { return Opens() && (connect == Connect::LaterFails || reply != Reply::Silent); }
    /** The query is answered at its turn, with this address or a name. */
    bool Answers() const { return Opens() && connect != Connect::LaterFails && (reply == Reply::Address || reply == Reply::Name); }
    /** The address of an Address reply, as the client reads it. */
    CNetAddr Answer() const
    {
        if (address.size() == 4) {
            in_addr ipv4;
            std::memcpy(&ipv4, address.data(), sizeof(ipv4));
            return CNetAddr{ipv4};
        }
        in6_addr ipv6;
        std::memcpy(&ipv6, address.data(), sizeof(ipv6));
        return CNetAddr{ipv6};
    }
};

/** The proxy behind the sockets discovery creates through CreateSock, each stream as its script
 *  says. */
class FakeProxy
{
public:
    explicit FakeProxy(const std::vector<Script>& scripts) : m_scripts{scripts} {}

    std::unique_ptr<Sock> Create()
    {
        // Discovery creates one socket per query, a seed at a time, its query sets in order.
        assert(m_streams.size() < m_scripts.size());
        Stream& stream{m_streams.emplace_back()};
        stream.script = &m_scripts[m_streams.size() - 1];
        switch (stream.script->connect) {
        case Script::Connect::Now: break;
        case Script::Connect::NoSocket: return nullptr;
        case Script::Connect::Refused: stream.connection->connect_error = ECONNREFUSED; break;
        case Script::Connect::Later:
        case Script::Connect::LaterFails: stream.connection->connect_error = WSAEINPROGRESS; break;
        } // no default case, so the compiler can warn about missing cases
        return std::make_unique<ConnectingSock>(stream.pipes, stream.connection);
    }

    /** Read what each stream's client wrote, and answer the greeting, the authentication and, once
     *  the stream's turn has come, the request. */
    void Serve()
    {
        for (Stream& stream : m_streams) {
            const std::vector<uint8_t> answer{stream.proxy.Received(ReadAll(stream.pipes->send))};
            if (!answer.empty()) stream.pipes->recv.PushBytes(answer.data(), answer.size());
            if (!stream.turn || stream.replied || !stream.proxy.GetRequest()) continue;
            stream.replied = true;
            std::vector<uint8_t> reply;
            switch (stream.script->reply) {
            case Script::Reply::Address:
                reply = {0x05, 0x00, 0x00, static_cast<uint8_t>(stream.script->address.size() == 4 ? 0x01 : 0x04)};
                reply.insert(reply.end(), stream.script->address.begin(), stream.script->address.end());
                reply.insert(reply.end(), {0x00, 0x00});
                break;
            case Script::Reply::Name: reply = Socks5Responder::ReplyName("seed.example"); break;
            case Script::Reply::Error: reply = Socks5Responder::ReplyError(0x04); break;
            case Script::Reply::Close:
            case Script::Reply::Silent: break;
            } // no default case, so the compiler can warn about missing cases
            stream.pipes->recv.PushBytes(reply.data(), reply.size());
        }
    }

    /** B2: two streams that got as far as their request share credentials just when they carry
     *  queries of the same query set. */
    void CheckCredentials() const
    {
        for (size_t i{0}; i < m_streams.size(); ++i) {
            for (size_t j{0}; j < i; ++j) {
                const auto& a{m_streams[i].proxy.GetRequest()};
                const auto& b{m_streams[j].proxy.GetRequest()};
                if (!a || !b) continue;
                assert((i % QUERIES_PER_SEED == j % QUERIES_PER_SEED) == (a->username == b->username && a->password == b->password));
            }
        }
    }

    /** Stream i's turn: its connect completes, if it was in progress, and the proxy answers its
     *  request, or closes the stream, or stays silent. */
    void Turn(size_t i)
    {
        if (i >= m_streams.size()) return;
        Stream& stream{m_streams[i]};
        stream.turn = true;
        stream.connection->in_progress = false;
        if (stream.script->connect == Script::Connect::LaterFails) stream.connection->so_error = ECONNREFUSED;
        if (stream.script->reply == Script::Reply::Close) stream.pipes->recv.Eof();
    }

private:
    struct Stream {
        const Script* script{nullptr};
        std::shared_ptr<DynSock::Pipes> pipes{std::make_shared<DynSock::Pipes>()};
        std::shared_ptr<ConnectingSock::Connection> connection{std::make_shared<ConnectingSock::Connection>()};
        Socks5Responder proxy;
        bool turn{false};
        bool replied{false};
    };

    const std::vector<Script>& m_scripts;
    /** Every socket created, in order. */
    std::deque<Stream> m_streams;
};

/** What the job's loop does for discovery at plan clock time now: the deadlines, a wait that does
 *  not block, and the events. */
void Round(Discovery& discovery, FakeProxy& proxy, milliseconds now)
{
    const SteadyMs steady_now{STEADY + now};
    discovery.Tick(now, steady_now);
    proxy.Serve();
    const std::vector<ProxyStream*> active{discovery.ActiveStreams()};
    Sock::EventsPerSock events;
    for (ProxyStream* stream : active) events.emplace(stream->GetSock(), Sock::Events{stream->WantedEvents()});
    if (!events.empty()) assert(events.begin()->first->WaitMany(0ms, events));
    for (ProxyStream* stream : active) {
        const auto it{events.find(stream->GetSock())};
        discovery.OnStreamEvents(*stream, it == events.end() ? Sock::Event{0} : it->second.occurred, now, steady_now);
    }
}

/** A discovery, started at `start`, whose streams take their turns in this order and at these
 *  times, run until its window has passed. */
DiscoveryResult Discover(const Plan& plan, uint16_t port, const std::vector<Script>& scripts, milliseconds start,
                         const std::vector<std::pair<milliseconds, size_t>>& turns)
{
    FakeProxy proxy{scripts};
    const auto create_sock{CreateSock};
    CreateSock = [&](int, int, int) { return proxy.Create(); };
    // The credentials, which change nothing else here.
    FastRandomContext rng{/*fDeterministic=*/true};
    Discovery discovery{plan, Proxy{CService{IPv4(INADDR_LOOPBACK), 9050}, /*tor_stream_isolation=*/true}, port, rng};
    discovery.Start(start, STEADY + start);
    // Each stream that connected at once gets as far as its request.
    for (int round{0}; round < 4; ++round) Round(discovery, proxy, start);
    for (const auto& [at, stream] : turns) {
        proxy.Turn(stream);
        for (int round{0}; round < 5; ++round) Round(discovery, proxy, at);
    }
    // The first step after the window's end has ended discovery (C3).
    Round(discovery, proxy, plan.timing.Scale(DISCOVERY_WINDOW) + 1ms);
    assert(discovery.Done() && discovery.ActiveStreams().empty());
    proxy.CheckCredentials();
    CreateSock = create_sock;
    return discovery.Result();
}

/** What two discoveries found is the same, whenever each ended. */
bool SameAnswers(const DiscoveryResult& a, const DiscoveryResult& b)
{
    return std::ranges::equal(a.seeds, b.seeds, [](const SeedAnswers& x, const SeedAnswers& y) {
        return x.name == y.name && x.queries == y.queries && x.skipped == y.skipped && x.answers == y.answers &&
               x.rejected == y.rejected && x.usable == y.usable;
    });
}

/** Assign()'s credit and draw rules (R4, R5b, R5c) and the assignment rules (R4, R5a, R5b, R5c) on
 *  its output. */
void CheckAssignment(const DiscoveryResult& result, const Assignment& assignment)
{
    // Each seed, in the plan's order, is credited with the distinct endpoints it returned that no
    // earlier seed did, each through the lowest query set that returned it, and each a candidate.
    assert(assignment.seeds.size() == result.seeds.size());
    std::set<CService> returned_before, candidates;
    std::map<std::string, std::map<CService, size_t>> credited_by_seed;
    int answers{0}, accepted{0};
    for (size_t i{0}; i < result.seeds.size(); ++i) {
        const SeedAnswers& seed{result.seeds[i]};
        const SeedSummary& summary{assignment.seeds[i]};
        assert(summary.name == seed.name && summary.queries == seed.queries && summary.skipped == seed.skipped && summary.answers == seed.answers);
        std::map<CService, size_t>& credited{credited_by_seed[seed.name]};
        for (size_t set{0}; set < seed.usable.size(); ++set) {
            if (seed.usable[set] && !returned_before.contains(*seed.usable[set])) credited.emplace(*seed.usable[set], set);
        }
        assert(summary.accepted == static_cast<int>(credited.size()));
        assert(summary.candidates.size() == credited.size());
        for (const CService& candidate : summary.candidates) {
            assert(credited.contains(candidate));
            assert(candidates.insert(candidate).second);
        }
        for (const std::optional<CService>& endpoint : seed.usable) {
            if (endpoint) returned_before.insert(*endpoint);
        }
        answers += seed.answers;
        accepted += summary.accepted;
    }
    // Every answer was accepted, a duplicate or rejected.
    assert(accepted + assignment.duplicates + assignment.rejected == answers);
    assert(assignment.exit_path_candidates == accepted);

    // R4: no endpoint in two opportunities, each a candidate of its seed, of the query set credited.
    std::set<CService> assigned;
    std::map<std::string, size_t> handed_out;
    size_t filled{0};
    for (size_t slot{0}; slot < SLOTS; ++slot) {
        for (const std::optional<Candidate>& candidate : assignment.opportunities[slot]) {
            if (!candidate) continue;
            ++filled;
            assert(assigned.insert(candidate->endpoint).second);
            const auto seed{credited_by_seed.find(candidate->provenance)};
            assert(seed != credited_by_seed.end());
            const auto credited{seed->second.find(candidate->endpoint)};
            assert(credited != seed->second.end() && credited->second == candidate->query_set);
            ++handed_out[candidate->provenance];
        }
    }
    // No candidate is left while an opportunity is.
    assert(filled == std::min(static_cast<size_t>(accepted), SLOTS * OPPORTUNITIES_PER_SLOT));

    // R5a: a candidate in a backup means every slot's first opportunity is filled.
    for (size_t layer{1}; layer < OPPORTUNITIES_PER_SLOT; ++layer) {
        for (size_t slot{0}; slot < SLOTS; ++slot) {
            if (!assignment.opportunities[slot][layer]) continue;
            for (size_t other{0}; other < SLOTS; ++other) assert(assignment.opportunities[other][0]);
        }
    }
    // R5b: a first attempt comes from a seed that has supplied none while such a seed has a
    // candidate. R5c: and through query set slot mod QUERIES_PER_SEED while a seed it may draw from
    // has a candidate from that set. A seed that has supplied no first attempt has handed out
    // nothing yet.
    std::set<std::string> supplied;
    std::set<CService> firsts;
    for (size_t slot{0}; slot < SLOTS && assignment.opportunities[slot][0]; ++slot) {
        const Candidate& first{*assignment.opportunities[slot][0]};
        const size_t wanted{slot % QUERIES_PER_SEED};
        bool fresh{false}, fresh_wanted{false}, wanted_left{false};
        for (const auto& [name, credited] : credited_by_seed) {
            for (const auto& [endpoint, set] : credited) {
                if (!supplied.contains(name)) {
                    fresh = true;
                    fresh_wanted |= set == wanted;
                }
                wanted_left |= set == wanted && !firsts.contains(endpoint);
            }
        }
        if (fresh) assert(!supplied.contains(first.provenance));
        if (fresh ? fresh_wanted : wanted_left) assert(first.query_set == wanted);
        supplied.insert(first.provenance);
        firsts.insert(first.endpoint);
    }
    // R5b: with at least SLOTS seeds that have candidates, the first attempts come from SLOTS seeds.
    if (std::ranges::count_if(credited_by_seed, [](const auto& seed) { return !seed.second.empty(); }) >= static_cast<std::ptrdiff_t>(SLOTS)) {
        assert(supplied.size() == SLOTS);
    }
    // R5b: a slot draws from a seed it has used only once no seed it has not used has a candidate
    // left. Every seed such a slot never drew from, then, has handed out every candidate it has.
    for (size_t slot{0}; slot < SLOTS; ++slot) {
        std::multiset<std::string> used;
        for (const std::optional<Candidate>& candidate : assignment.opportunities[slot]) {
            if (candidate) used.insert(candidate->provenance);
        }
        if (std::set<std::string>(used.begin(), used.end()).size() == used.size()) continue;
        for (const auto& [name, credited] : credited_by_seed) {
            if (!used.contains(name)) assert(handed_out[name] == credited.size());
        }
    }
}

} // namespace

FUZZ_TARGET(privbcast_discovery)
{
    FuzzedDataProvider provider{buffer.data(), buffer.size()};
    // The plan: DNS seed names, each maybe listed twice, with a time divisor.
    const Timing timing{provider.ConsumeIntegralInRange<int>(1, MAX_TIME_DIVISOR)};
    std::vector<std::string> dns_seeds;
    LIMITED_WHILE(provider.ConsumeBool(), 8) dns_seeds.push_back(strprintf("seed%d.example", provider.ConsumeIntegralInRange<int>(0, 9)));
    FastRandomContext rng{ConsumeUInt256(provider)};
    const Plan plan{DrawPlan(rng, timing, dns_seeds)};
    const uint16_t port{provider.ConsumeIntegral<uint16_t>()};

    // Answers come from a few addresses, so that seeds share some and repeat others: public and
    // private, IPv4 and IPv6, an IPv4 address mapped into IPv6, and fuzzed ones.
    std::vector<std::vector<uint8_t>> addresses{{1, 2, 3, 4}, {10, 0, 0, 1}, {127, 0, 0, 1}, {8, 8, 8, 8}};
    for (const char* text : {"2600::1", "fc00::1", "::ffff:1.2.3.4", "fd6b:88c0:8724::1"}) {
        std::vector<uint8_t> bytes(16);
        assert(inet_pton(AF_INET6, text, bytes.data()) == 1);
        addresses.push_back(bytes);
    }
    LIMITED_WHILE(provider.ConsumeBool(), 4) {
        std::vector<uint8_t> bytes{provider.ConsumeBytes<uint8_t>(provider.ConsumeBool() ? 4 : 16)};
        bytes.resize(bytes.size() <= 4 ? 4 : 16);
        addresses.push_back(bytes);
    }

    // Every query's script: an address of its own, or one of those, as its answer. Then the turns:
    // when each stream gets its answer, in the order the proxy gives them, at times in the window
    // and past it.
    const size_t queries{plan.seed_order.size() * QUERIES_PER_SEED};
    std::vector<Script> scripts(queries);
    for (size_t q{0}; q < queries; ++q) {
        Script& script{scripts[q]};
        script.connect = provider.PickValueInArray({Script::Connect::Now, Script::Connect::Now, Script::Connect::NoSocket, Script::Connect::Refused,
                                                    Script::Connect::Later, Script::Connect::LaterFails});
        script.reply = provider.PickValueInArray({Script::Reply::Address, Script::Reply::Address, Script::Reply::Name, Script::Reply::Error,
                                                  Script::Reply::Close, Script::Reply::Silent});
        // Only an address reply takes one from the input.
        const bool pick{script.reply == Script::Reply::Address && provider.ConsumeBool()};
        script.address = pick ? addresses[provider.ConsumeIntegralInRange<size_t>(0, addresses.size() - 1)] :
                                std::vector<uint8_t>{9, 9, static_cast<uint8_t>(q), 1};
    }
    const milliseconds window{timing.Scale(DISCOVERY_WINDOW)};
    const milliseconds start{provider.ConsumeBool() ? 0ms : milliseconds{provider.ConsumeIntegralInRange<int64_t>(0, timing.Scale(QUERY_GRACE).count() + 1)}};
    std::vector<size_t> order(queries);
    for (size_t i{0}; i < queries; ++i) order[i] = i;
    std::vector<std::pair<milliseconds, size_t>> turns;
    milliseconds at{start};
    for (size_t i{0}; i < queries; ++i) {
        std::swap(order[i], order[provider.ConsumeIntegralInRange<size_t>(i, queries - 1)]);
        // The turns span about one window, so that some answers come after it (C3).
        at += milliseconds{provider.ConsumeIntegralInRange<int64_t>(0, 2 * window.count() / static_cast<int64_t>(queries) + 1)};
        // Whether an answer at the window's end itself counts is not specified (the invariants hold
        // to the precision of a step): such a turn comes a millisecond later.
        if (at == window) at += 1ms;
        turns.emplace_back(at, order[i]);
    }

    const DiscoveryResult result{Discover(plan, port, scripts, start, turns)};

    // What discovery must have found: each seed, in the plan's order, with its queries launched at
    // job start unless the loop started too late (D1), its answers that came within the window
    // (C3), and of those the public routable IPv4 and IPv6 addresses, with the chain's port (R2).
    const bool launched{start < timing.Scale(QUERY_GRACE)};
    assert(result.seeds.size() == plan.seed_order.size());
    std::vector<int> opened(plan.seed_order.size());
    for (size_t q{0}; q < queries; ++q) {
        if (launched && scripts[q].Opens()) ++opened[q / QUERIES_PER_SEED];
    }
    for (size_t s{0}; s < plan.seed_order.size(); ++s) {
        const SeedAnswers& seed{result.seeds[s]};
        assert(seed.name == plan.seed_order[s]);
        assert(seed.queries == opened[s] && seed.skipped == QUERIES_PER_SEED - opened[s]);
        int answers{0}, rejected{0};
        std::array<std::optional<CService>, QUERIES_PER_SEED> usable;
        for (const auto& [time, q] : turns) {
            if (q / QUERIES_PER_SEED != s || !launched || time >= window || !scripts[q].Answers()) continue;
            ++answers;
            const CNetAddr answer{scripts[q].Answer()};
            if (scripts[q].reply == Script::Reply::Address && (answer.IsIPv4() || answer.IsIPv6()) && answer.IsRoutable()) {
                usable[q % QUERIES_PER_SEED] = CService{answer, port};
            } else {
                ++rejected;
            }
        }
        assert(seed.answers == answers && seed.rejected == rejected);
        for (const std::optional<CService>& endpoint : seed.usable) {
            if (endpoint) assert((endpoint->IsIPv4() || endpoint->IsIPv6()) && endpoint->IsRoutable() && endpoint->GetPort() == port);
        }
        // Each answer counts for the query set it came through.
        assert(seed.usable == usable);
    }
    // C3: discovery ended when its last query did, or else at the end of the window.
    milliseconds last_end{start};
    bool all_ended{true};
    for (size_t q{0}; q < queries && all_ended; ++q) {
        if (!launched || !scripts[q].Opens()) continue;
        const auto turn{std::ranges::find(turns, q, &std::pair<milliseconds, size_t>::second)};
        if (!scripts[q].Ends() || turn->first >= window) {
            all_ended = false;
        } else {
            last_end = std::max(last_end, turn->first);
        }
    }
    assert(result.duration == (all_ended ? last_end : window));

    const Assignment assignment{Assign(plan, result)};
    CheckAssignment(result, assignment);

    // R5b: the order in which the answers arrive within the window changes nothing. The same
    // answers, and failures, at the same times but in another order give the same result, though
    // discovery may end at another time: when its last query ended.
    const auto in_window{std::ranges::count_if(turns, [&](const auto& turn) { return turn.first < window; })};
    std::vector<std::pair<milliseconds, size_t>> permuted{turns};
    for (ptrdiff_t i{in_window - 1}; i > 0; --i) {
        std::swap(permuted[i].second, permuted[provider.ConsumeIntegralInRange<ptrdiff_t>(0, i)].second);
    }
    const DiscoveryResult again{Discover(plan, port, scripts, start, permuted)};
    assert(SameAnswers(again, result));
    assert(Assign(plan, again) == assignment);
    // C3: nor do the turns after the end of the window, whose answers come too late.
    const std::vector<std::pair<milliseconds, size_t>> in_time{turns.begin(), turns.begin() + in_window};
    assert(SameAnswers(Discover(plan, port, scripts, start, in_time), result));
}
