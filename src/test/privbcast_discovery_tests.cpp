// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bitcoin-build-config.h> // IWYU pragma: keep

#include <compat/compat.h>
#include <netaddress.h>
#include <netbase.h>
#include <privbcast/assign.h>
#include <privbcast/discovery.h>
#include <privbcast/params.h>
#include <privbcast/plan.h>
#include <privbcast/stream.h>
#include <random.h>
#include <test/util/net.h>
#include <test/util/setup_common.h>
#include <uint256.h>
#include <util/sock.h>

#include <boost/test/unit_test.hpp>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <deque>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#ifdef HAVE_SOCKADDR_UN
#include <sys/un.h>
#endif

using namespace privbcast;
using namespace std::chrono_literals;
using std::chrono::milliseconds;

namespace {

constexpr uint16_t DEFAULT_PORT{18444};
/** The I/O clock at job start. It is passed to discovery like the plan clock. */
constexpr SteadyMs STEADY{1h};
const std::vector<std::string> SEEDS{"a.seed.example", "b.seed.example", "c.seed.example"};

/** A numeric IPv4 address, without the DNS hook, which fails these tests. */
CService Service(const std::string& text, uint16_t port)
{
    in_addr ipv4;
    BOOST_REQUIRE(inet_pton(AF_INET, text.c_str(), &ipv4) == 1);
    return CService{ipv4, port};
}

const CService PROXY{Service("127.0.0.1", 9050)};

/** The query sets a seed got a usable answer through. */
size_t Usable(const SeedAnswers& seed)
{
    return std::ranges::count_if(seed.usable, [](const auto& answer) { return answer.has_value(); });
}

/** A socket discovery created, and the proxy's side of it. */
struct FakeStream {
    int domain{AF_UNSPEC};
    std::shared_ptr<DynSock::Pipes> pipes{std::make_shared<DynSock::Pipes>()};
    std::shared_ptr<ConnectingSock::Connection> connection;
    Socks5Responder proxy;
    /** Discovery closed the socket. */
    bool closed{false};
};

/**
 * The proxy, behind the sockets discovery creates through CreateSock. When discovery waits on
 * them, it answers each greeting and authentication; the test answers the requests.
 */
class FakeProxy
{
public:
    /** Every socket created, in order. */
    std::deque<FakeStream> streams;
    /** How the next sockets connect, in order. Once empty, a socket connects at once. */
    std::deque<ConnectingSock::Connection> connections;

    std::unique_ptr<Sock> Create(int domain);

    /** Read what discovery wrote, and answer the greetings and authentications. */
    void Serve()
    {
        for (FakeStream& stream : streams) {
            const std::vector<uint8_t> answer{stream.proxy.Received(ReadAll(stream.pipes->send))};
            if (!answer.empty()) stream.pipes->recv.PushBytes(answer.data(), answer.size());
        }
    }

    /** Answer the request of stream i. */
    void Reply(size_t i, const std::vector<uint8_t>& reply)
    {
        BOOST_REQUIRE(streams.at(i).proxy.GetRequest());
        streams[i].pipes->recv.PushBytes(reply.data(), reply.size());
    }
};

std::unique_ptr<Sock> FakeProxy::Create(int domain)
{
    FakeStream& stream{streams.emplace_back()};
    stream.domain = domain;
    stream.connection = std::make_shared<ConnectingSock::Connection>();
    if (!connections.empty()) {
        *stream.connection = connections.front();
        connections.pop_front();
    }
    // The proxy answers when the code under test waits on the socket.
    auto sock{std::make_unique<ConnectingSock>(stream.pipes, stream.connection)};
    sock->on_wait = [this] { Serve(); };
    sock->on_close = [&stream] { stream.closed = true; };
    return sock;
}

/** What the job's loop does for discovery, five times: the deadlines, a wait that does not block,
 *  and the events. */
void Run(Discovery& discovery, milliseconds now, SteadyMs steady_now)
{
    for (int round{0}; round < 5; ++round) {
        discovery.Tick(now, steady_now);
        const std::vector<ProxyStream*> active{discovery.ActiveStreams()};
        Sock::EventsPerSock events;
        for (ProxyStream* stream : active) events.emplace(stream->GetSock(), Sock::Events{stream->WantedEvents()});
        if (events.empty()) return;
        BOOST_REQUIRE(events.begin()->first->WaitMany(0ms, events));
        for (ProxyStream* stream : active) {
            const auto it{events.find(stream->GetSock())};
            discovery.OnStreamEvents(*stream, it == events.end() ? Sock::Event{0} : it->second.occurred, now, steady_now);
        }
    }
}

struct DiscoverySetup : public BasicTestingSetup {
    FakeProxy proxy;
    FastRandomContext rng{uint256{1}};
    const Plan plan{DrawPlan(rng, Timing{}, SEEDS)};
    decltype(CreateSock) create_sock{CreateSock};
    DNSLookupFn dns_lookup{g_dns_lookup};

    DiscoverySetup()
    {
        // B1: no name is ever looked up locally.
        g_dns_lookup = [](const std::string& name, bool) -> std::vector<CNetAddr> {
            BOOST_ERROR("DNS lookup of " << name);
            return {};
        };
        CreateSock = [this](int domain, int, int) { return proxy.Create(domain); };
    }
    ~DiscoverySetup()
    {
        CreateSock = create_sock;
        g_dns_lookup = dns_lookup;
    }
};

} // namespace

BOOST_FIXTURE_TEST_SUITE(privbcast_discovery_tests, DiscoverySetup)

#ifdef HAVE_SOCKADDR_UN
BOOST_AUTO_TEST_CASE(unix_socket_proxy)
{
    // B1: a proxy given as a path is connected to as a unix socket.
    constexpr uint8_t RESOLVE{0xF0};
    const std::string path{"/run/tor/socks"};
    Discovery discovery{plan, Proxy{ADDR_PREFIX_UNIX + path}, DEFAULT_PORT, rng};
    discovery.Start(0ms, STEADY);
    BOOST_REQUIRE_EQUAL(proxy.streams.size(), QUERIES_PER_SEED * SEEDS.size());
    for (const FakeStream& stream : proxy.streams) {
        BOOST_CHECK_EQUAL(stream.domain, AF_UNIX);
        sockaddr_un addr{};
        BOOST_REQUIRE_EQUAL(stream.connection->address.size(), sizeof(addr));
        std::memcpy(&addr, stream.connection->address.data(), sizeof(addr));
        BOOST_CHECK_EQUAL(addr.sun_family, AF_UNIX);
        BOOST_CHECK_EQUAL(std::string{addr.sun_path}, path);
    }
    Run(discovery, 0ms, STEADY);
    for (const FakeStream& stream : proxy.streams) {
        BOOST_REQUIRE(stream.proxy.GetRequest());
        BOOST_CHECK_EQUAL(stream.proxy.GetRequest()->command, RESOLVE);
    }
}
#endif

BOOST_AUTO_TEST_CASE(query_sets)
{
    // B2, D1: QUERIES_PER_SEED query sets, each with credentials of its own and one query of every
    // seed on a stream of its own. An answer counts for the set it came through, whatever the order
    // the answers arrive in (R5b).
    Discovery discovery{plan, Proxy{PROXY}, DEFAULT_PORT, rng};
    discovery.Start(0ms, STEADY);
    Run(discovery, 0ms, STEADY);
    BOOST_REQUIRE_EQUAL(proxy.streams.size(), QUERIES_PER_SEED * SEEDS.size());
    std::map<std::pair<std::string, std::string>, std::vector<std::string>> sets;
    for (const FakeStream& stream : proxy.streams) {
        const auto& request{stream.proxy.GetRequest()};
        BOOST_REQUIRE(request);
        BOOST_CHECK(!request->username.empty() && !request->password.empty());
        sets[{request->username, request->password}].push_back(request->host);
    }
    BOOST_REQUIRE_EQUAL(sets.size(), size_t{QUERIES_PER_SEED});
    for (auto& [_, names] : sets) {
        std::ranges::sort(names);
        BOOST_CHECK(names == SEEDS);
    }
    // The streams are created a seed at a time, in the plan's order, its query sets in order. The
    // last stream is answered first.
    for (size_t left{proxy.streams.size()}; left > 0; --left) {
        const size_t i{left - 1};
        proxy.Reply(i, Socks5Responder::ReplyAddress(strprintf("1.1.%d.%d", i / QUERIES_PER_SEED + 1, i % QUERIES_PER_SEED + 1)));
        Run(discovery, 1s, STEADY + 1s);
    }
    BOOST_REQUIRE(discovery.Done());
    const DiscoveryResult result{discovery.Result()};
    for (size_t s{0}; s < SEEDS.size(); ++s) {
        BOOST_CHECK_EQUAL(result.seeds[s].name, plan.seed_order[s]);
        for (size_t set{0}; set < QUERIES_PER_SEED; ++set) {
            const auto username{proxy.streams[s * QUERIES_PER_SEED + set].proxy.GetRequest()->username};
            BOOST_CHECK_EQUAL(username, proxy.streams[set].proxy.GetRequest()->username);
            BOOST_CHECK(result.seeds[s].usable[set] == Service(strprintf("1.1.%d.%d", s + 1, set + 1), DEFAULT_PORT));
        }
    }
}

BOOST_AUTO_TEST_CASE(failure_inside_a_query)
{
    // C7: a throw while reading one query's reply ends only that query, started with no answer;
    // the others are answered and counted, and discovery ends with the last of them.
    Discovery discovery{plan, Proxy{PROXY}, DEFAULT_PORT, rng};
    discovery.Start(0ms, STEADY);
    Run(discovery, 0ms, STEADY);
    const size_t failing{0}, last{proxy.streams.size() - 1};
    proxy.streams[failing].connection->recv_throws = "injected";
    for (size_t i{0}; i < last; ++i) proxy.Reply(i, Socks5Responder::ReplyAddress(strprintf("1.1.1.%d", i + 1)));
    Run(discovery, 1s, STEADY + 1s);
    BOOST_CHECK(proxy.streams[failing].closed);
    BOOST_CHECK(!discovery.Done());
    BOOST_CHECK_EQUAL(discovery.ActiveStreams().size(), 1U);
    proxy.Reply(last, Socks5Responder::ReplyAddress("1.1.2.1"));
    Run(discovery, 2s, STEADY + 2s);
    BOOST_CHECK(discovery.Done());

    const DiscoveryResult result{discovery.Result()};
    BOOST_CHECK(result.duration == 2s);
    // The first seed's streams are the first ones.
    const SeedAnswers& first{result.seeds[0]};
    BOOST_CHECK_EQUAL(first.queries, QUERIES_PER_SEED);
    BOOST_CHECK_EQUAL(first.skipped, 0);
    BOOST_CHECK_EQUAL(first.answers, QUERIES_PER_SEED - 1);
    BOOST_CHECK_EQUAL(Usable(first), size_t{QUERIES_PER_SEED} - 1);
    BOOST_CHECK(!first.usable[0]);
    for (size_t i{1}; i < result.seeds.size(); ++i) {
        BOOST_CHECK_EQUAL(result.seeds[i].queries, QUERIES_PER_SEED);
        BOOST_CHECK_EQUAL(result.seeds[i].answers, QUERIES_PER_SEED);
        BOOST_CHECK_EQUAL(Usable(result.seeds[i]), size_t{QUERIES_PER_SEED});
    }
}

BOOST_AUTO_TEST_CASE(streams_end_at_launch)
{
    // C3: a query whose stream fails as it is launched has ended with no answer: with all ending
    // so, discovery is over at once; with one under way, when that one ends.
    ConnectingSock::Connection failing;
    failing.send_error = ECONNRESET;
    const size_t queries{QUERIES_PER_SEED * SEEDS.size()};
    for (const bool all : {true, false}) {
        proxy.streams.clear();
        proxy.connections.assign(all ? queries : queries - 1, failing);
        Discovery discovery{plan, Proxy{PROXY}, DEFAULT_PORT, rng};
        discovery.Start(0ms, STEADY);
        BOOST_REQUIRE_EQUAL(proxy.streams.size(), queries);
        BOOST_CHECK_EQUAL(discovery.Done(), all);
        BOOST_CHECK_EQUAL(discovery.ActiveStreams().size(), all ? 0U : 1U);
        if (!all) {
            Run(discovery, 0ms, STEADY);
            proxy.Reply(queries - 1, Socks5Responder::ReplyAddress("1.1.1.1"));
            Run(discovery, 1s, STEADY + 1s);
            BOOST_CHECK(discovery.Done());
        }
        for (const FakeStream& stream : proxy.streams) BOOST_CHECK(stream.closed);
        const DiscoveryResult result{discovery.Result()};
        BOOST_CHECK(result.duration == (all ? 0ms : 1s));
        int answers{0};
        for (const SeedAnswers& seed : result.seeds) {
            BOOST_CHECK_EQUAL(seed.queries, QUERIES_PER_SEED);
            BOOST_CHECK_EQUAL(seed.skipped, 0);
            answers += seed.answers;
        }
        BOOST_CHECK_EQUAL(answers, all ? 0 : 1);
    }
}

BOOST_AUTO_TEST_CASE(scaled_window)
{
    // The time divisor scales the window and the stages (Interface/bitcoin-privbcast: -timedivisor).
    const Plan scaled{DrawPlan(rng, Timing{10}, SEEDS)};
    Discovery discovery{scaled, Proxy{PROXY}, DEFAULT_PORT, rng};
    BOOST_CHECK(discovery.WindowEnd() == 1200ms);
    discovery.Start(0ms, STEADY);
    Run(discovery, 0ms, STEADY);
    for (FakeStream& stream : proxy.streams) BOOST_REQUIRE(stream.proxy.GetRequest());
    Run(discovery, 1s, STEADY + scaled.timing.Scale(SOCKS_STAGE_TIMEOUT) - 1ms);
    BOOST_CHECK(!discovery.Done());
    Run(discovery, 1s, STEADY + scaled.timing.Scale(SOCKS_STAGE_TIMEOUT));
    BOOST_CHECK(discovery.Done());
    BOOST_CHECK(discovery.Result().duration == 1s);
}

BOOST_AUTO_TEST_SUITE_END()
