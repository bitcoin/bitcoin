// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <compat/compat.h>
#include <net_transport.h>
#include <netaddress.h>
#include <netbase.h>
#include <netmessagemaker.h>
#include <primitives/transaction.h>
#include <privbcast/assign.h>
#include <privbcast/job.h>
#include <privbcast/params.h>
#include <privbcast/plan.h>
#include <privbcast/report.h>
#include <protocol.h>
#include <random.h>
#include <serialize.h>
#include <streams.h>
#include <test/util/net.h>
#include <test/util/privbcast.h>
#include <test/util/setup_common.h>
#include <test/util/time.h>
#include <uint256.h>
#include <univalue.h>
#include <util/fs.h>
#include <util/sock.h>
#include <util/time.h>

#include <boost/test/unit_test.hpp>

#include <algorithm>
#include <array>
#include <atomic>
#include <cassert>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <deque>
#include <functional>
#include <map>
#include <memory>
#include <numeric>
#include <optional>
#include <set>
#include <span>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

using namespace privbcast;
using namespace std::chrono_literals;
using std::chrono::milliseconds;

namespace {

/** Job start, on the plan clock. */
constexpr NodeSeconds T0{1'700'000'000s};
constexpr uint16_t PORT{18444};
constexpr uint8_t CONNECT{0x01};
constexpr uint8_t RESOLVE{0xF0};
/** What every recipient's VERSION shows, unless a test says otherwise. */
constexpr int PEER_VERSION{70016};
const std::string PEER_USER_AGENT{"/Satoshi:30.0.0/"};

/** A numeric IPv4 address, without the DNS hook, which fails these tests. */
CService Service(const std::string& text)
{
    in_addr ipv4;
    const int parsed{inet_pton(AF_INET, text.c_str(), &ipv4)};
    assert(parsed == 1);
    return CService{ipv4, PORT};
}

const CService PROXY{[] {
    in_addr loopback;
    loopback.s_addr = htonl(INADDR_LOOPBACK);
    return CService{loopback, 9050};
}()};

/** Three DNS seeds, each with three addresses (its fourth answer repeats its first): nine
 *  candidates. */
const std::vector<std::string> SEEDS{"a.seed.", "b.seed.", "c.seed."};
const std::map<std::string, std::vector<std::string>> ANSWERS{
    {"a.seed.", {"8.0.0.1", "8.0.0.2", "8.0.0.3", "8.0.0.1"}},
    {"b.seed.", {"8.0.1.1", "8.0.1.2", "8.0.1.3", "8.0.1.1"}},
    {"c.seed.", {"8.0.2.1", "8.0.2.2", "8.0.2.3", "8.0.2.1"}},
};

/** What discovery finds when every query gets its scripted answer: the plan's seeds, in its order. */
DiscoveryResult Result(const Plan& plan, const std::map<std::string, std::vector<std::string>>& answers)
{
    DiscoveryResult result;
    for (const std::string& name : plan.seed_order) {
        SeedAnswers& seed{result.seeds.emplace_back()};
        seed.name = name;
        seed.queries = QUERIES_PER_SEED;
        const auto it{answers.find(name)};
        if (it == answers.end()) continue;
        seed.answers = static_cast<int>(it->second.size());
        // The proxy answers a seed's queries in the order they are sent, a query set at a time.
        for (size_t set{0}; set < it->second.size(); ++set) seed.usable.at(set) = Service(it->second[set]);
    }
    return result;
}

std::unique_ptr<FastRandomContext> Rng(uint8_t seed) { return std::make_unique<FastRandomContext>(uint256{seed}); }

/** A job that draws from a context seeded with seed, its attempts' BIP324 keys and garbage
 *  included, so that its bytes are reproducible. */
Job MakeJob(JobInputs inputs, uint8_t seed)
{
    auto rng{Rng(seed)};
    inputs.keys = KeysFrom(*rng);
    return Job{std::move(inputs), std::move(rng)};
}

enum class Behaviour {
    /** Handshakes, requests the transaction by wtxid and answers the PING. */
    Honest,
    /** Handshakes and never requests the transaction, as a peer that has it already. */
    NoRequest,
    /** Requests the transaction and never answers the PING. */
    NoPong,
    /** As Honest, and once the transaction has arrived, asks for its parent as a node lacking it
     *  does: by txid, as MSG_WITNESS_TX. */
    AsksParent,
    /** Answers the job's VERSION only LATE_BY after it, then as Honest. */
    Late,
    /** Closes the connection on the INV. */
    Drops,
    /** Never sends a byte. */
    Silent,
    /** Closes the connection as soon as the proxy has connected it. */
    Closes,
    /** The proxy refuses the CONNECT. */
    Refused,
    /** The proxy never answers the CONNECT. */
    Stalled,
};
constexpr std::chrono::seconds LATE_BY{30};

/** A recipient behind the proxy: a BIP324 responder, as a node runs one. */
class Recipient
{
public:
    Recipient(Behaviour behaviour, std::string user_agent, FastRandomContext& rng, const Wtxid& wtxid, const Txid& parent_txid)
        : m_behaviour{behaviour}, m_user_agent{std::move(user_agent)}, m_transport(MakeResponder(rng)), m_wtxid{wtxid}, m_parent_txid{parent_txid} {}

    /** What the job wrote. */
    void Received(std::span<const uint8_t> bytes)
    {
        if (m_behaviour == Behaviour::Silent) return;
        while (!bytes.empty() && !m_closing) {
            BOOST_REQUIRE(m_transport.ReceivedBytes(bytes));
            if (!m_transport.ReceivedMessageComplete()) continue;
            bool reject{false};
            CNetMessage msg{m_transport.GetReceivedMessage({}, reject)};
            BOOST_REQUIRE(!reject);
            Process(msg.m_type, msg.m_recv);
        }
    }

    /** What the recipient writes now. */
    std::vector<uint8_t> ToSend()
    {
        if (m_answer_at && NodeClock::now() >= *m_answer_at) {
            m_answer_at.reset();
            Handshake();
        }
        std::vector<uint8_t> out;
        while (!m_closing) {
            if (!m_queue.empty() && m_transport.SetMessageToSend(m_queue.front())) m_queue.pop_front();
            const auto& [bytes, _more, _type]{m_transport.GetBytesToSend(/*have_next_message=*/!m_queue.empty())};
            if (bytes.empty()) break;
            out.insert(out.end(), bytes.begin(), bytes.end());
            m_transport.MarkBytesSent(bytes.size());
        }
        return out;
    }

    /** Queue a message the test scripts, sent at the recipient's next turn. */
    void Send(CSerializedNetMsg msg) { m_queue.push_back(std::move(msg)); }
    /** The recipient closes the connection. */
    bool Closing() const { return m_closing; }
    /** The types of the messages the job sent, in order. */
    const std::vector<std::string>& Messages() const { return m_messages; }

private:
    const Behaviour m_behaviour;
    const std::string m_user_agent;
    V2Transport m_transport;
    const Wtxid m_wtxid;
    const Txid m_parent_txid;
    std::deque<CSerializedNetMsg> m_queue;
    std::vector<std::string> m_messages;
    std::optional<NodeClock::time_point> m_answer_at;
    bool m_asked_parent{false};
    bool m_closing{false};

    void Handshake()
    {
        m_queue.push_back(PeerVersion(PEER_VERSION, NODE_NETWORK | NODE_WITNESS, /*relay=*/true, m_user_agent));
        m_queue.push_back(NetMsg::Make(NetMsgType::WTXIDRELAY));
        m_queue.push_back(NetMsg::Make(NetMsgType::VERACK));
    }

    void Process(const std::string& type, DataStream& payload)
    {
        m_messages.push_back(type);
        if (type == NetMsgType::VERSION) {
            if (m_behaviour == Behaviour::Late) {
                m_answer_at = NodeClock::now() + LATE_BY;
            } else {
                Handshake();
            }
        } else if (type == NetMsgType::INV) {
            if (m_behaviour == Behaviour::Drops) {
                m_closing = true;
            } else if (m_behaviour != Behaviour::NoRequest) {
                m_queue.push_back(NetMsg::Make(NetMsgType::GETDATA, std::vector<CInv>{CInv{MSG_WTX, m_wtxid.ToUint256()}}));
            }
        } else if (type == NetMsgType::TX && m_behaviour == Behaviour::AsksParent && !m_asked_parent) {
            m_asked_parent = true;
            m_queue.push_back(NetMsg::Make(NetMsgType::GETDATA, std::vector<CInv>{CInv{MSG_WITNESS_TX, m_parent_txid.ToUint256()}}));
        } else if (type == NetMsgType::PING && m_behaviour != Behaviour::NoPong) {
            uint64_t nonce{0};
            payload >> nonce;
            m_queue.push_back(NetMsg::Make(NetMsgType::PONG, nonce));
        }
    }
};

/** One socket the job created: the proxy's side of it, and the recipient behind it. */
struct FakeStream {
    int domain{AF_UNSPEC};
    std::shared_ptr<DynSock::Pipes> pipes{std::make_shared<DynSock::Pipes>()};
    std::shared_ptr<ConnectingSock::Connection> connection{std::make_shared<ConnectingSock::Connection>()};
    Socks5Responder socks;
    /** For a RESOLVE: the scripted answer, an address or empty for a failure, until it is sent. */
    std::optional<std::string> answer;
    std::unique_ptr<Recipient> recipient;
    /** The plan clock when the job created the socket, and when it closed it. */
    NodeClock::time_point created;
    std::optional<NodeClock::time_point> closed;
    /** Run once, when the job next writes to or reads from this socket. */
    std::function<void()> on_io;

    bool Is(uint8_t command) const { return socks.GetRequest() && socks.GetRequest()->command == command; }
};

/**
 * The proxy, playing Tor: RESOLVE queries get scripted answers, and a CONNECT reaches a scripted
 * recipient. Its side of every socket runs when the job waits on its sockets.
 */
class FakeTor
{
public:
    /** By seed name, in the order its queries arrive. An empty string is a failure reply. */
    std::map<std::string, std::vector<std::string>> resolve;
    /** By the host a CONNECT names; default_behaviour for the others. */
    std::map<std::string, Behaviour> behaviours;
    Behaviour default_behaviour{Behaviour::Honest};
    /** By the host a CONNECT names; PEER_USER_AGENT for the others. */
    std::map<std::string, std::string> user_agents;
    /** How the next sockets connect to the proxy, in order. Once empty, at once. */
    std::deque<ConnectingSock::Connection> connections;
    /** Hold back the RESOLVE answers until Release(). */
    bool hold{false};
    Wtxid wtxid;
    /** What an AsksParent recipient asks for. */
    Txid parent_txid;
    /** Run once, when the job next writes to or reads from one of its sockets: time that passes
     *  inside a step. */
    std::function<void()> on_io;

    /** Every socket the job created, in order. */
    std::deque<FakeStream> streams;
    /** Bytes moved either way, to tell when the network has gone quiet. */
    uint64_t moved{0};

    std::unique_ptr<Sock> Create(int domain);

    /** Forget the sockets of a job that has gone. */
    void Reset()
    {
        streams.clear();
        m_queries.clear();
        moved = 0;
    }

    /** The proxy's and the recipients' turn: read what the job wrote and answer it. */
    void Serve()
    {
        for (FakeStream& stream : streams) {
            if (stream.closed) continue;
            const std::vector<uint8_t> written{ReadAll(stream.pipes->send)};
            moved += written.size();
            if (!stream.socks.GetRequest()) {
                Push(stream, stream.socks.Received(written));
                if (stream.socks.GetRequest()) Request(stream);
            } else if (stream.recipient) {
                stream.recipient->Received(written);
            }
            if (stream.recipient) {
                Push(stream, stream.recipient->ToSend());
                if (stream.recipient->Closing()) Close(stream);
            }
        }
    }

    /** Send the held answer of stream i. */
    void Release(size_t i) { Answer(streams.at(i)); }

    /** The job writes to or reads from the stream's socket. */
    void Io(FakeStream& stream)
    {
        if (stream.on_io) std::exchange(stream.on_io, nullptr)();
        if (on_io) std::exchange(on_io, nullptr)();
    }

    /** The CONNECT streams to an endpoint. */
    std::vector<const FakeStream*> Dialled(const CService& endpoint) const
    {
        std::vector<const FakeStream*> found;
        for (const FakeStream& stream : streams) {
            if (stream.Is(CONNECT) && stream.socks.GetRequest()->host == endpoint.ToStringAddr()) found.push_back(&stream);
        }
        return found;
    }

    /** The streams of attempts: a CONNECT, or no request yet once delivery has started. */
    std::vector<const FakeStream*> Attempts() const
    {
        std::vector<const FakeStream*> found;
        for (const FakeStream& stream : streams) {
            if (stream.socks.GetRequest() ? stream.Is(CONNECT) : stream.created >= T0 + DISCOVERY_WINDOW) found.push_back(&stream);
        }
        return found;
    }

private:
    FastRandomContext m_rng{uint256{42}};
    std::map<std::string, size_t> m_queries;

    void Push(FakeStream& stream, const std::vector<uint8_t>& bytes)
    {
        if (bytes.empty()) return;
        stream.pipes->recv.PushBytes(bytes.data(), bytes.size());
        moved += bytes.size();
    }

    void Answer(FakeStream& stream)
    {
        BOOST_REQUIRE(stream.answer);
        Push(stream, stream.answer->empty() ? Socks5Responder::ReplyError(0x04) : Socks5Responder::ReplyAddress(*stream.answer));
        stream.answer.reset();
    }

    void Close(FakeStream& stream) { stream.pipes->recv.Eof(); }

    void Request(FakeStream& stream)
    {
        const Socks5Responder::Request& request{*stream.socks.GetRequest()};
        if (request.command == RESOLVE) {
            const std::vector<std::string>& answers{resolve[request.host]};
            const size_t n{m_queries[request.host]++};
            stream.answer = n < answers.size() ? answers[n] : "";
            if (!hold) Answer(stream);
            return;
        }
        const auto it{behaviours.find(request.host)};
        const Behaviour behaviour{it == behaviours.end() ? default_behaviour : it->second};
        if (behaviour == Behaviour::Refused) return Push(stream, Socks5Responder::ReplyError(0x05));
        if (behaviour == Behaviour::Stalled) return;
        Push(stream, Socks5Responder::ReplyAddress("0.0.0.0"));
        if (behaviour == Behaviour::Closes) return Close(stream);
        const auto agent{user_agents.find(request.host)};
        stream.recipient = std::make_unique<Recipient>(behaviour, agent == user_agents.end() ? PEER_USER_AGENT : agent->second, m_rng, wtxid, parent_txid);
    }
};

std::unique_ptr<Sock> FakeTor::Create(int domain)
{
    FakeStream& stream{streams.emplace_back()};
    stream.domain = domain;
    stream.created = NodeClock::now();
    if (!connections.empty()) {
        *stream.connection = connections.front();
        connections.pop_front();
    }
    // The proxy and the recipients answer when the job waits on the socket.
    auto sock{std::make_unique<ConnectingSock>(stream.pipes, stream.connection)};
    sock->on_wait = [this] { Serve(); };
    sock->on_io = [this, &stream] { Io(stream); };
    sock->on_close = [&stream] { stream.closed = NodeClock::now(); };
    return sock;
}

/** The endpoint's host as a CONNECT names it. */
std::string Host(const CService& endpoint) { return endpoint.ToStringAddr(); }

/** The candidates the plan gives these answers: the job's assignment once discovery has them. */
Assignment Expected(const Plan& plan, const std::map<std::string, std::vector<std::string>>& answers = ANSWERS)
{
    return Assign(plan, Result(plan, answers));
}

/** The attempt was dialled at its opportunity's time: not before, and within the second after it,
 *  as the tests move the clock a second at a time. */
bool OnTime(const AttemptReport& attempt)
{
    return attempt.started >= attempt.scheduled_start && attempt.started - attempt.scheduled_start < 1s;
}

/** A slot's dials: endpoint, scheduled start and the dial, in order. */
using Dials = std::vector<std::tuple<std::string, milliseconds, milliseconds>>;
Dials DialsOf(const SlotReport& slot)
{
    Dials dials;
    for (const AttemptReport& attempt : slot.attempts) {
        dials.emplace_back(attempt.candidate.endpoint.ToStringAddrPort(), attempt.scheduled_start, attempt.started);
    }
    return dials;
}

/** A slot's attempts as the report gives them, less the bytes counted, which depend on what the
 *  recipients and the attempts dialled before drew from their randomness. */
std::vector<std::string> EntriesOf(const Report& report, size_t slot)
{
    const UniValue json{ToUniValue(report)};
    std::vector<std::string> entries;
    for (const UniValue& attempt : json["slots"][slot]["attempts"].getValues()) {
        UniValue entry{UniValue::VOBJ};
        for (const std::string& key : attempt.getKeys()) {
            if (!key.starts_with("bytes_")) entry.pushKV(key, attempt[key]);
        }
        entries.push_back(entry.write());
    }
    return entries;
}

struct JobSetup : public BasicTestingSetup {
    FakeNodeClock clock{T0};
    FakeSteadyClock steady;
    FakeTor tor;
    std::atomic<bool> cancel{false};
    const CTransactionRef tx{MakeTx()};
    decltype(CreateSock) create_sock{CreateSock};
    DNSLookupFn dns_lookup{g_dns_lookup};

    JobSetup()
    {
        // B1: no name is ever looked up locally.
        g_dns_lookup = [](const std::string& name, bool) -> std::vector<CNetAddr> {
            BOOST_ERROR("DNS lookup of " << name);
            return {};
        };
        CreateSock = [this](int domain, int, int) { return tor.Create(domain); };
        tor.wtxid = tx->GetWitnessHash();
        tor.resolve = ANSWERS;
    }
    ~JobSetup()
    {
        CreateSock = create_sock;
        g_dns_lookup = dns_lookup;
    }

    JobInputs Inputs(const std::vector<std::string>& seeds = SEEDS)
    {
        JobInputs inputs;
        inputs.tx = tx;
        inputs.proxy = Proxy{PROXY};
        inputs.seeds = {.dns_seeds = seeds, .default_port = PORT, .chain = "regtest"};
        inputs.cancel = &cancel;
        return inputs;
    }

    /** The plan clock's offset from T0. */
    milliseconds Now() const { return std::chrono::floor<milliseconds>(NodeClock::now() - T0); }

    /** Step the job without moving the clocks until the network is quiet. True once the job ended. */
    bool Settle(Job& job)
    {
        for (int step{0}, quiet{0}; quiet < 3; ++step) {
            BOOST_REQUIRE(step < 1000);
            const uint64_t moved{tor.moved};
            const size_t sockets{tor.streams.size()};
            if (job.Step(0ms)) return true;
            quiet = tor.moved == moved && tor.streams.size() == sockets ? quiet + 1 : 0;
        }
        return false;
    }

    /** Move both clocks on a second and settle the job. True once it ended. */
    bool Pass(Job& job)
    {
        clock += 1s;
        steady += 1s;
        return Settle(job);
    }

    /** Pass time until the plan clock reaches `at` from T0, or the job ends. */
    bool PassUntil(Job& job, milliseconds at)
    {
        while (Now() < at) {
            if (Pass(job)) return true;
        }
        return false;
    }

    Report RunToEnd(Job& job)
    {
        bool ended{Settle(job)};
        for (int second{0}; !ended && second < 1000; ++second) ended = Pass(job);
        BOOST_REQUIRE(ended);
        return job.TakeReport();
    }

    /** Start again at T0, with the default scripts. */
    void Restart()
    {
        tor.Reset();
        tor.resolve = ANSWERS;
        tor.behaviours.clear();
        tor.default_behaviour = Behaviour::Honest;
        tor.user_agents.clear();
        tor.connections.clear();
        tor.hold = false;
        clock.set(T0);
        cancel = false;
    }
};

/** Where a socket connected to. */
CService Destination(const FakeStream& stream)
{
    sockaddr_storage addr{};
    BOOST_REQUIRE(!stream.connection->address.empty() && stream.connection->address.size() <= sizeof(addr));
    std::memcpy(&addr, stream.connection->address.data(), stream.connection->address.size());
    CService service;
    BOOST_REQUIRE(service.SetSockAddr(reinterpret_cast<const sockaddr*>(&addr), stream.connection->address.size()));
    return service;
}

/** Restores the current directory. */
class CurrentPath
{
public:
    explicit CurrentPath(const fs::path& path) : m_previous{fs::current_path()} { fs::current_path(path); }
    ~CurrentPath() { fs::current_path(m_previous); }

private:
    const fs::path m_previous;
};

/** An object's keys. */
std::set<std::string> Keys(const UniValue& obj)
{
    BOOST_REQUIRE(obj.isObject());
    return {obj.getKeys().begin(), obj.getKeys().end()};
}

/** The keys of an attempt and of the summary in a report (Interface/Report). */
const std::set<std::string> ATTEMPT_KEYS{"endpoint", "provenance", "outcome", "reason", "scheduled_start_ms",
                                         "started_ms", "connected_ms", "peer_version", "peer_user_agent", "inv_handed_ms",
                                         "inv_written_ms", "getdata_ms", "tx_written_ms", "ping_written_ms", "pong_ms",
                                         "ended_ms", "extra_requests", "bytes_sent", "bytes_recv"};
const std::set<std::string> SUMMARY_KEYS{"connections", "announcements_handed", "announcements_written", "tx_written", "pongs",
                                         "slots_completed", "interrupted", "error", "duration_ms"};

/** H3: the text holds none of the transaction's bytes, with or without its witness. */
void CheckNoTxBytes(const std::string& text, const CTransaction& tx)
{
    DataStream with_witness, without_witness;
    with_witness << TX_WITH_WITNESS(tx);
    without_witness << TX_NO_WITNESS(tx);
    BOOST_CHECK(text.find(HexStr(with_witness)) == std::string::npos);
    BOOST_CHECK(text.find(HexStr(without_witness)) == std::string::npos);
}

} // namespace

BOOST_FIXTURE_TEST_SUITE(privbcast_job_tests, JobSetup)

BOOST_AUTO_TEST_CASE(plan_independent_of_peers)
{
    // C1: the same randomness gives the same plan, whatever the seeds answer.
    std::vector<Plan> plans;
    for (const bool answering : {true, false}) {
        Restart();
        if (!answering) {
            tor.resolve.clear();
            tor.default_behaviour = Behaviour::Refused;
        }
        Job job = MakeJob(Inputs(), 2);
        const Report report{RunToEnd(job)};
        BOOST_CHECK_EQUAL(report.discovery.exit_path_candidates, answering ? 9 : 0);
        plans.push_back(job.GetPlan());
    }
    BOOST_CHECK(plans[0] == plans[1]);
}

BOOST_AUTO_TEST_CASE(assignment_from_result)
{
    // C2, R5b: the same answers arriving in the opposite order give the same assignment. Every
    // CONNECT is refused, so that every opportunity with a candidate is dialled.
    std::vector<std::vector<Dials>> runs;
    for (const bool reverse : {false, true}) {
        Restart();
        tor.hold = true;
        tor.default_behaviour = Behaviour::Refused;
        Job job = MakeJob(Inputs(), 3);
        BOOST_REQUIRE(!Settle(job));
        std::vector<size_t> order(tor.streams.size());
        std::iota(order.begin(), order.end(), size_t{0});
        if (reverse) std::reverse(order.begin(), order.end());
        // All within the discovery window.
        for (const size_t i : order) {
            tor.Release(i);
            BOOST_REQUIRE(!Settle(job));
        }
        BOOST_REQUIRE(job.Snapshot().discovery_done);
        const Report report{RunToEnd(job)};
        std::vector<Dials>& run{runs.emplace_back()};
        for (const SlotReport& slot : report.slots) run.push_back(DialsOf(slot));

        // The dials are the plan's assignment of the answers, each at its opportunity's time.
        const Assignment expected{Expected(job.GetPlan())};
        for (size_t s{0}; s < SLOTS; ++s) {
            std::vector<std::pair<std::string, milliseconds>> assigned, dialled;
            for (size_t k{0}; k < OPPORTUNITIES_PER_SLOT; ++k) {
                const auto& candidate{expected.opportunities[s][k]};
                if (candidate) assigned.emplace_back(candidate->endpoint.ToStringAddrPort(), job.GetPlan().slots[s].scheduled[k]);
            }
            for (const AttemptReport& attempt : report.slots[s].attempts) {
                dialled.emplace_back(attempt.candidate.endpoint.ToStringAddrPort(), attempt.scheduled_start);
                BOOST_CHECK(OnTime(attempt));
            }
            BOOST_CHECK(dialled == assigned);
        }
    }
    BOOST_CHECK(runs[0] == runs[1]);
}

BOOST_AUTO_TEST_CASE(recipients_move_only_their_slot)
{
    // C4: a silent, a late and a closing recipient change no other slot; a slot whose attempt fails
    // before the announcement goes on with its next candidate. Unrequested, the job dials the same.
    const Assignment expected{Expected(DrawPlan(*Rng(4), Timing{}, SEEDS))};
    const auto first{[&](size_t slot) { return Host(expected.opportunities[slot][0]->endpoint); }};
    std::map<std::string, Report> reports;
    for (const std::string run : {"honest", "deviant", "unrequested"}) {
        Restart();
        if (run == "deviant") {
            tor.behaviours = {{first(0), Behaviour::Silent}, {first(1), Behaviour::Late}, {first(2), Behaviour::Closes}};
        } else if (run == "unrequested") {
            tor.default_behaviour = Behaviour::NoRequest;
        }
        Job job = MakeJob(Inputs(), 4);
        reports.emplace(run, RunToEnd(job));
    }
    const Report& honest{reports.at("honest")};
    const Report& deviant{reports.at("deviant")};
    const Report& unrequested{reports.at("unrequested")};
    for (size_t s{0}; s < SLOTS; ++s) {
        BOOST_CHECK(DialsOf(honest.slots[s]) == DialsOf(unrequested.slots[s]));
        BOOST_REQUIRE_EQUAL(honest.slots[s].attempts.size(), 1U);
        if (s != 0 && s != 2) BOOST_CHECK(DialsOf(honest.slots[s]) == DialsOf(deviant.slots[s]));
    }
    // The late recipient was announced to late, in its own slot only.
    BOOST_CHECK(deviant.slots[1].attempts[0].times.inv_handed == DISCOVERY_WINDOW + LATE_BY);
    BOOST_CHECK(deviant.slots[1].attempts[0].outcome == Outcome::PongReceived);
    // The silent and the closing recipients' slots dialled their next candidates at their times.
    for (const size_t s : {0, 2}) {
        const SlotReport& slot{deviant.slots[s]};
        BOOST_REQUIRE_EQUAL(slot.attempts.size(), 2U);
        BOOST_CHECK(DialsOf(slot)[0] == DialsOf(honest.slots[s])[0]);
        BOOST_CHECK(slot.attempts[0].outcome == Outcome::NotAnnounced);
        BOOST_CHECK(slot.attempts[1].candidate == *expected.opportunities[s][1]);
        BOOST_CHECK(slot.attempts[1].scheduled_start == slot.schedule.scheduled[1]);
        BOOST_CHECK(OnTime(slot.attempts[1]));
    }
    // The silent one held its attempt until the handshake deadline.
    const std::optional<milliseconds> silent_ended{deviant.slots[0].attempts[0].times.ended};
    BOOST_CHECK(silent_ended && *silent_ended >= DISCOVERY_WINDOW + HANDSHAKE_BUDGET && *silent_ended <= DISCOVERY_WINDOW + HANDSHAKE_BUDGET + 1s);
    BOOST_CHECK(deviant.slots[2].attempts[0].times.ended == DISCOVERY_WINDOW);
}

BOOST_AUTO_TEST_CASE(dialled_within_grace)
{
    // C5: an opportunity the loop reaches late, within START_GRACE, is dialled when reached.
    Job job = MakeJob(Inputs(), 5);
    BOOST_REQUIRE(!Settle(job));
    BOOST_REQUIRE(job.Snapshot().discovery_done);
    const std::chrono::seconds late{START_GRACE / 2};
    clock += DISCOVERY_WINDOW + late;
    steady += DISCOVERY_WINDOW + late;
    const Report report{RunToEnd(job)};
    for (size_t s{0}; s < SLOTS; ++s) {
        if (report.slots[s].schedule.stratum != Stratum::Prompt) continue;
        BOOST_CHECK_EQUAL(report.slots[s].missed_opportunities, 0);
        BOOST_REQUIRE(!report.slots[s].attempts.empty());
        BOOST_CHECK(report.slots[s].attempts[0].started == DISCOVERY_WINDOW + late);
    }
}

BOOST_AUTO_TEST_CASE(missed_after_grace)
{
    // C5: an opportunity reached well past START_GRACE late is missed, never dialled; the next runs
    // on time.
    tor.default_behaviour = Behaviour::Refused;
    Job job = MakeJob(Inputs(), 5);
    const Assignment expected{Expected(job.GetPlan())};
    BOOST_REQUIRE(!Settle(job));
    clock += DISCOVERY_WINDOW + 2 * START_GRACE;
    steady += DISCOVERY_WINDOW + 2 * START_GRACE;
    const size_t sockets{tor.streams.size()};
    BOOST_REQUIRE(!Settle(job));
    BOOST_CHECK_EQUAL(tor.streams.size(), sockets);
    const Report report{RunToEnd(job)};
    for (size_t s{0}; s < SLOTS; ++s) {
        const SlotReport& slot{report.slots[s]};
        if (slot.schedule.stratum != Stratum::Prompt) {
            BOOST_CHECK_EQUAL(slot.missed_opportunities, 0);
            continue;
        }
        BOOST_CHECK_EQUAL(slot.missed_opportunities, 1);
        BOOST_CHECK(tor.Dialled(expected.opportunities[s][0]->endpoint).empty());
        BOOST_REQUIRE(expected.opportunities[s][1]);
        BOOST_REQUIRE(!slot.attempts.empty());
        BOOST_CHECK(slot.attempts[0].candidate == *expected.opportunities[s][1]);
        BOOST_CHECK(slot.attempts[0].scheduled_start == slot.schedule.scheduled[1]);
        BOOST_CHECK(OnTime(slot.attempts[0]));
    }
}

BOOST_AUTO_TEST_CASE(failure_inside_an_attempt)
{
    // C7: a throw while reading from one attempt ends only that attempt, with its text as reason:
    // before the announcement point its slot dials the next opportunity on time (C4), after it
    // nothing more (C6). The other slots run as without it, and the job does not fail.
    for (const Behaviour behaviour : {Behaviour::Late, Behaviour::NoRequest}) {
        const bool announced{behaviour == Behaviour::NoRequest};
        std::vector<Report> reports;
        Assignment expected;
        for (const bool fail : {false, true}) {
            Restart();
            Job job = MakeJob(Inputs(), 31);
            expected = Expected(job.GetPlan());
            BOOST_REQUIRE(expected.opportunities[0][0] && expected.opportunities[0][1]);
            const CService& endpoint{expected.opportunities[0][0]->endpoint};
            tor.behaviours[Host(endpoint)] = behaviour;
            BOOST_REQUIRE(!PassUntil(job, DISCOVERY_WINDOW + 1s));
            if (fail) {
                const auto dialled{tor.Dialled(endpoint)};
                BOOST_REQUIRE_EQUAL(dialled.size(), 1U);
                BOOST_REQUIRE(dialled[0]->recipient && !dialled[0]->closed);
                dialled[0]->connection->recv_throws = "injected";
                if (announced) dialled[0]->recipient->Send(NetMsg::Make(NetMsgType::PING, uint64_t{1}));
            }
            reports.push_back(RunToEnd(job));
        }
        const Report& report{reports[1]};
        const SlotReport& slot{report.slots[0]};
        BOOST_REQUIRE_EQUAL(slot.attempts.size(), announced ? 1U : 2U);
        const AttemptReport& failed{slot.attempts[0]};
        BOOST_CHECK(failed.outcome == (announced ? Outcome::PostAnnouncementFailure : Outcome::NotAnnounced));
        BOOST_CHECK_EQUAL(failed.reason, "injected");
        BOOST_CHECK(failed.times.connected == DISCOVERY_WINDOW);
        BOOST_CHECK(failed.times.inv_handed == (announced ? std::optional{DISCOVERY_WINDOW} : std::nullopt));
        BOOST_REQUIRE(failed.times.ended);
        BOOST_CHECK(*failed.times.ended == (announced ? DISCOVERY_WINDOW + 1s : DISCOVERY_WINDOW + LATE_BY));
        BOOST_CHECK(tor.Dialled(expected.opportunities[0][0]->endpoint)[0]->closed == T0 + *failed.times.ended);
        if (announced) {
            BOOST_CHECK(tor.Dialled(expected.opportunities[0][1]->endpoint).empty());
        } else {
            BOOST_CHECK(slot.attempts[1].candidate == *expected.opportunities[0][1]);
            BOOST_CHECK(slot.attempts[1].scheduled_start == slot.schedule.scheduled[1]);
            BOOST_CHECK(OnTime(slot.attempts[1]));
            BOOST_CHECK(slot.attempts[1].outcome == Outcome::PongReceived);
        }
        for (size_t s{1}; s < SLOTS; ++s) {
            BOOST_CHECK(!report.slots[s].attempts.empty());
            BOOST_CHECK(EntriesOf(report, s) == EntriesOf(reports[0], s));
        }
        BOOST_CHECK(!report.summary.error);
        BOOST_CHECK_EQUAL(report.summary.slots_completed, int(SLOTS));
    }
}

BOOST_AUTO_TEST_CASE(failure_of_the_job)
{
    // C7, H2: a throw in the wait on the sockets fails the job; it stops as a cancelled one does:
    // each attempt in flight ends by its state, with the error as reason, and the report gives it.
    // No slot is interrupted, only ended ones are completed, and the tool exits 1, or 0 if an INV
    // was written.
    for (const bool announced : {false, true}) {
        Restart();
        tor.default_behaviour = Behaviour::Refused;
        Job job = MakeJob(Inputs(), 33);
        const Plan& plan{job.GetPlan()};
        const Assignment expected{Expected(plan)};
        // The first late slot's first recipient never answers; every other connection is refused,
        // but in the variant slot 1's first, whose recipient never asks for the transaction.
        const size_t late{plan.slots[4].scheduled[0] < plan.slots[5].scheduled[0] ? size_t{4} : size_t{5}};
        BOOST_REQUIRE(expected.opportunities[late][0]);
        const CService& silent{expected.opportunities[late][0]->endpoint};
        tor.behaviours[Host(silent)] = Behaviour::Silent;
        if (announced) tor.behaviours[Host(expected.opportunities[1][0]->endpoint)] = Behaviour::NoRequest;
        BOOST_REQUIRE(!PassUntil(job, plan.slots[late].scheduled[0] + 1s));
        const auto dialled{tor.Dialled(silent)};
        BOOST_REQUIRE_EQUAL(dialled.size(), 1U);
        BOOST_REQUIRE(!dialled[0]->closed);
        dialled[0]->connection->wait_throws = "injected";
        BOOST_REQUIRE(job.Step(0ms));

        const Report report{job.TakeReport()};
        BOOST_CHECK_EQUAL(report.summary.error.value_or(""), "injected");
        BOOST_CHECK(!report.summary.interrupted);
        BOOST_CHECK(report.summary.duration == Now());
        BOOST_REQUIRE_EQUAL(report.slots[late].attempts.size(), 1U);
        const AttemptReport& in_flight{report.slots[late].attempts[0]};
        BOOST_CHECK(in_flight.candidate.endpoint == silent);
        BOOST_CHECK(in_flight.outcome == Outcome::NotAnnounced);
        BOOST_CHECK_EQUAL(in_flight.reason, "injected");
        BOOST_CHECK(in_flight.times.ended == Now());
        for (const FakeStream& stream : tor.streams) BOOST_CHECK(stream.closed);
        // Each slot has ended by its last opportunity's time, as no attempt but the one in flight
        // gets far: the prompt ones, and perhaps the mid one, had ended.
        int ended{0};
        for (size_t s{0}; s < SLOTS; ++s) {
            BOOST_CHECK(!report.slots[s].interrupted);
            if (plan.slots[s].scheduled.back() <= Now()) ++ended;
        }
        BOOST_CHECK_GE(ended, 3);
        BOOST_CHECK_EQUAL(report.summary.slots_completed, ended);
        BOOST_CHECK_EQUAL(report.summary.announcements_written, announced ? 1 : 0);
        BOOST_CHECK_EQUAL(ExitStatus(report), announced ? EXIT_ANNOUNCED : EXIT_JOB_FAILED);
    }

    // The job fails while a prompt attempt waits for a request: the attempt ends as cancellation
    // would end it, announced_not_requested, with the error as reason, and the tool exits 0.
    Restart();
    tor.default_behaviour = Behaviour::Refused;
    Job job = MakeJob(Inputs(), 33);
    const Assignment expected{Expected(job.GetPlan())};
    BOOST_REQUIRE(expected.opportunities[1][0]);
    const CService& endpoint{expected.opportunities[1][0]->endpoint};
    tor.behaviours[Host(endpoint)] = Behaviour::NoRequest;
    BOOST_REQUIRE(!PassUntil(job, DISCOVERY_WINDOW + 1s));
    const auto dialled{tor.Dialled(endpoint)};
    BOOST_REQUIRE_EQUAL(dialled.size(), 1U);
    BOOST_REQUIRE(!dialled[0]->closed);
    dialled[0]->connection->wait_throws = "injected";
    BOOST_REQUIRE(job.Step(0ms));

    const Report report{job.TakeReport()};
    BOOST_CHECK_EQUAL(report.summary.error.value_or(""), "injected");
    BOOST_REQUIRE_EQUAL(report.slots[1].attempts.size(), 1U);
    const AttemptReport& in_flight{report.slots[1].attempts[0]};
    BOOST_CHECK(in_flight.times.inv_written == DISCOVERY_WINDOW);
    BOOST_CHECK(in_flight.outcome == Outcome::AnnouncedNotRequested);
    BOOST_CHECK_EQUAL(in_flight.reason, "injected");
    BOOST_CHECK(in_flight.times.ended == Now());
    BOOST_CHECK_EQUAL(ExitStatus(report), EXIT_ANNOUNCED);
}

BOOST_AUTO_TEST_CASE(exit_status)
{
    // H2: 0 once an INV was fully written, failed or not; otherwise 1 if the job failed, else 2.
    for (const bool announced : {true, false}) {
        for (const bool failed : {true, false}) {
            Report report;
            report.summary.announcements_written = announced ? 1 : 0;
            if (failed) report.summary.error = "failed";
            BOOST_CHECK_EQUAL(ExitStatus(report), announced ? 0 : (failed ? 1 : 2));
        }
    }
}

BOOST_AUTO_TEST_CASE(cancel_blocked_exchange)
{
    // C8: cancelled with one stream blocked in the SOCKS5 exchange and one in its connect, the job
    // ends at the next step: every stream closed, running slots interrupted.
    Job job = MakeJob(Inputs(), 8);
    const Assignment expected{Expected(job.GetPlan())};
    tor.behaviours[Host(expected.opportunities[0][0]->endpoint)] = Behaviour::Stalled;
    ConnectingSock::Connection in_progress;
    in_progress.connect_error = WSAEINPROGRESS;
    BOOST_REQUIRE(!PassUntil(job, DISCOVERY_WINDOW - 1s));
    tor.connections = {ConnectingSock::Connection{}, in_progress};
    BOOST_REQUIRE(!Pass(job));
    const auto blocked{tor.Dialled(expected.opportunities[0][0]->endpoint)};
    BOOST_REQUIRE_EQUAL(blocked.size(), 1U);
    BOOST_CHECK(!blocked[0]->closed);
    cancel = true;
    BOOST_CHECK(job.Step(0ms));
    for (const FakeStream& stream : tor.streams) BOOST_CHECK(stream.closed);

    const Report report{job.TakeReport()};
    BOOST_CHECK(report.summary.interrupted);
    BOOST_CHECK(report.summary.duration == DISCOVERY_WINDOW);
    BOOST_CHECK_EQUAL(report.summary.connections, 3);
    // Slots 0 and 1 were interrupted mid-attempt; slot 2 had finished.
    for (const size_t s : {0, 1}) {
        BOOST_CHECK(report.slots[s].interrupted);
        BOOST_REQUIRE_EQUAL(report.slots[s].attempts.size(), 1U);
        BOOST_CHECK(report.slots[s].attempts[0].outcome == Outcome::NotAnnounced);
    }
    BOOST_CHECK(!report.slots[2].interrupted);
    for (size_t s{3}; s < SLOTS; ++s) {
        BOOST_CHECK(report.slots[s].interrupted);
        BOOST_CHECK(report.slots[s].attempts.empty());
    }
    BOOST_CHECK_EQUAL(report.summary.slots_completed, 1);
}

BOOST_AUTO_TEST_CASE(one_socket_per_slot)
{
    // D1: a slot holds one socket at a time, even when its attempt ends in the step that dials its
    // next opportunity, here as the clock reaches the backup's time during a write. So slot 0's first
    // attempt has closed its socket when its backup creates one.
    Job job = MakeJob(Inputs(), 21);
    const Plan& plan{job.GetPlan()};
    const Assignment expected{Expected(plan)};
    BOOST_REQUIRE(expected.opportunities[0][0] && expected.opportunities[0][1]);
    const std::string first{Host(expected.opportunities[0][0]->endpoint)};
    const std::string backup{Host(expected.opportunities[0][1]->endpoint)};
    const std::chrono::seconds due{std::chrono::ceil<std::chrono::seconds>(plan.slots[0].scheduled[1])};
    BOOST_REQUIRE(plan.slots[3].scheduled[0] > due);
    // Whether the first attempt's socket was open as each socket was created.
    std::vector<bool> first_open;
    CreateSock = [&](int domain, int, int) {
        first_open.push_back(std::ranges::any_of(tor.streams, [&](const FakeStream& s) {
            return !s.closed && s.Is(CONNECT) && s.socks.GetRequest()->host == first;
        }));
        return tor.Create(domain);
    };
    BOOST_REQUIRE(!PassUntil(job, DISCOVERY_WINDOW - 1s));
    tor.on_io = [&] {
        clock += due - DISCOVERY_WINDOW;
        steady += due - DISCOVERY_WINDOW;
    };
    BOOST_REQUIRE(!Pass(job));
    BOOST_REQUIRE(Now() == due);
    const Report report{RunToEnd(job)};
    // The first attempt ended at the backup's time, and the backup was dialled then.
    const SlotReport& slot{report.slots[0]};
    BOOST_REQUIRE_EQUAL(slot.attempts.size(), 2U);
    BOOST_CHECK(slot.attempts[0].outcome == Outcome::NotAnnounced);
    BOOST_CHECK(slot.attempts[0].times.ended == due);
    BOOST_CHECK(slot.attempts[1].started == due);
    BOOST_CHECK(slot.attempts[1].outcome == Outcome::PongReceived);
    const auto backup_stream{std::ranges::find_if(tor.streams, [&](const FakeStream& s) {
        return s.Is(CONNECT) && s.socks.GetRequest()->host == backup;
    })};
    BOOST_REQUIRE(backup_stream != tor.streams.end());
    BOOST_CHECK(!first_open.at(backup_stream - tor.streams.begin()));
}

BOOST_AUTO_TEST_CASE(clock_set_back)
{
    // D2: the deadlines are on the plan clock, so a clock set back stretches the waits that remain.
    tor.default_behaviour = Behaviour::NoRequest;
    Job job = MakeJob(Inputs(), 11);
    BOOST_REQUIRE(!PassUntil(job, 50s));
    clock -= 20s;
    // The prompt attempts announced at delivery start; their request windows end on the plan clock.
    const milliseconds window_end{DISCOVERY_WINDOW + REQUEST_WINDOW};
    while (Now() + 1s < window_end) BOOST_REQUIRE(!Pass(job));
    BOOST_CHECK_EQUAL(job.Snapshot().connections, 0);
    BOOST_REQUIRE(!Pass(job));
    BOOST_REQUIRE(!Pass(job));
    BOOST_CHECK_EQUAL(job.Snapshot().connections, 3);
    const Report report{RunToEnd(job)};
    for (size_t s{0}; s < SLOTS; ++s) {
        if (report.slots[s].schedule.stratum != Stratum::Prompt) continue;
        const std::optional<milliseconds> ended{report.slots[s].attempts[0].times.ended};
        BOOST_CHECK(ended && *ended >= window_end && *ended <= window_end + 1s);
    }
}

BOOST_AUTO_TEST_CASE(time_passes_within_step)
{
    // C5, Interface/Report: time that passes during a step's I/O counts at once: the TX and the
    // PING written after a slow read get the later time, and a backup whose grace ran out meanwhile
    // is missed.
    Job job = MakeJob(Inputs(), 19);
    const Plan& plan{job.GetPlan()};
    const Assignment expected{Expected(plan)};
    const CService& backup{expected.opportunities[0][1]->endpoint};
    const CService& requesting{expected.opportunities[1][0]->endpoint};
    tor.behaviours = {{Host(expected.opportunities[0][0]->endpoint), Behaviour::Refused},
                      {Host(requesting), Behaviour::NoRequest}};
    const milliseconds due{plan.slots[0].scheduled[1]};
    BOOST_REQUIRE(!PassUntil(job, due - 1s));
    FakeStream* stream{nullptr};
    for (FakeStream& s : tor.streams) {
        if (s.Is(CONNECT) && s.socks.GetRequest()->host == Host(requesting)) stream = &s;
    }
    BOOST_REQUIRE(stream && stream->recipient && !stream->closed);
    stream->recipient->Send(NetMsg::Make(NetMsgType::GETDATA, std::vector<CInv>{CInv{MSG_WTX, tx->GetWitnessHash().ToUint256()}}));
    const milliseconds before{Now()};
    tor.on_io = [&] {
        clock += START_GRACE + 2s;
        steady += START_GRACE + 2s;
    };
    BOOST_REQUIRE(!job.Step(0ms));
    const milliseconds after{Now()};
    BOOST_REQUIRE(after == before + START_GRACE + 2s);
    // Cancelled at once, the job reports what that step did.
    cancel = true;
    BOOST_REQUIRE(job.Step(0ms));
    const Report report{job.TakeReport()};
    BOOST_REQUIRE_EQUAL(report.slots[1].attempts.size(), 1U);
    const AttemptTimes& times{report.slots[1].attempts[0].times};
    BOOST_CHECK(times.getdata == before);
    BOOST_CHECK(times.tx_written == after);
    BOOST_CHECK(times.ping_written == after);
    const SlotReport& slot{report.slots[0]};
    BOOST_CHECK_EQUAL(slot.missed_opportunities, 1);
    BOOST_CHECK_EQUAL(slot.attempts.size(), 1U);
    BOOST_CHECK(tor.Dialled(backup).empty());
}

BOOST_AUTO_TEST_CASE(stream_ends_at_dial)
{
    // C4: a stream that fails as it is dialled ends its attempt at the dial, though the step then
    // takes a second, and the slot dials its next opportunity on time.
    Job job = MakeJob(Inputs(), 23);
    const Assignment expected{Expected(job.GetPlan())};
    BOOST_REQUIRE(!PassUntil(job, DISCOVERY_WINDOW - 1s));
    // Slot 0 dials first, then slot 1, whose greeting is the next write.
    ConnectingSock::Connection failing;
    failing.send_error = ECONNRESET;
    tor.connections = {failing};
    tor.on_io = [&] {
        tor.on_io = [&] {
            clock += 1s;
            steady += 1s;
        };
    };
    BOOST_REQUIRE(!Pass(job));
    const Report report{RunToEnd(job)};
    const SlotReport& slot{report.slots[0]};
    BOOST_REQUIRE_EQUAL(slot.attempts.size(), 2U);
    const AttemptReport& failed{slot.attempts[0]};
    BOOST_CHECK(failed.candidate == *expected.opportunities[0][0]);
    BOOST_CHECK(failed.outcome == Outcome::NotAnnounced);
    BOOST_CHECK(failed.started == DISCOVERY_WINDOW);
    BOOST_CHECK(failed.times.ended == DISCOVERY_WINDOW);
    BOOST_CHECK(!failed.times.connected);
    BOOST_CHECK_EQUAL(failed.bytes_sent, 0U);
    BOOST_CHECK(slot.attempts[1].candidate == *expected.opportunities[0][1]);
    BOOST_CHECK(OnTime(slot.attempts[1]));
    BOOST_CHECK(slot.attempts[1].outcome == Outcome::PongReceived);
}

BOOST_AUTO_TEST_CASE(first_bytes_with_the_reply)
{
    // D3: the peer's first bytes, read with the proxy's reply, reach the attempt and count: it
    // records the connection at that read, then runs to its handshake deadline.
    Job job = MakeJob(Inputs(), 25);
    const Assignment expected{Expected(job.GetPlan())};
    const CService& stalled{expected.opportunities[0][0]->endpoint};
    tor.behaviours[Host(stalled)] = Behaviour::Stalled;
    BOOST_REQUIRE(!PassUntil(job, DISCOVERY_WINDOW - 1s));
    BOOST_REQUIRE(!Pass(job));
    FakeStream* stream{nullptr};
    for (FakeStream& s : tor.streams) {
        if (s.Is(CONNECT) && s.socks.GetRequest()->host == Host(stalled)) stream = &s;
    }
    BOOST_REQUIRE(stream && !stream->closed);
    const std::vector<uint8_t> first(10, 0x42);
    std::vector<uint8_t> answer{Socks5Responder::ReplyAddress("0.0.0.0")};
    answer.insert(answer.end(), first.begin(), first.end());
    stream->pipes->recv.PushBytes(answer.data(), answer.size());
    const milliseconds read_at{Now()};
    BOOST_REQUIRE(!job.Step(0ms));
    BOOST_CHECK(!stream->closed);
    const Report report{RunToEnd(job)};
    BOOST_REQUIRE(!report.slots[0].attempts.empty());
    const AttemptReport& attempt{report.slots[0].attempts[0]};
    BOOST_CHECK(attempt.outcome == Outcome::NotAnnounced);
    BOOST_CHECK(attempt.times.connected == read_at);
    const milliseconds deadline{attempt.scheduled_start + HANDSHAKE_BUDGET};
    BOOST_CHECK(attempt.times.ended >= deadline && attempt.times.ended <= deadline + 1s);
    BOOST_CHECK_EQUAL(attempt.bytes_recv, first.size());
    BOOST_CHECK_GT(attempt.bytes_sent, 0U);
}

BOOST_AUTO_TEST_CASE(slot_ends_after_its_opportunities)
{
    // D2, C6: a slot that announced ends with that attempt; any other once each opportunity has
    // ended, an empty one at its time. Slot 0, refused at delivery start with three empty
    // opportunities, runs to the fourth's time: a cancellation before then interrupts it.
    const std::map<std::string, std::vector<std::string>> answers{{"a.seed.", {"8.0.0.1", "8.0.0.2"}}};
    for (int variant{0}; variant < 3; ++variant) {
        Restart();
        tor.resolve = answers;
        Job job = MakeJob(Inputs({"a.seed."}), 13);
        const Assignment expected{Expected(job.GetPlan(), answers)};
        BOOST_REQUIRE(expected.opportunities[0][0] && expected.opportunities[1][0]);
        tor.behaviours[Host(expected.opportunities[0][0]->endpoint)] = Behaviour::Refused;
        const milliseconds end{job.GetPlan().slots[0].scheduled.back()};
        const milliseconds at_end{std::chrono::ceil<std::chrono::seconds>(end)};
        // Just after delivery start, at the last whole second before slot 0's end, and at its end.
        const milliseconds cancel_at{variant == 0 ? DISCOVERY_WINDOW + 1s : variant == 1 ? at_end - 1s : at_end};
        BOOST_REQUIRE(!PassUntil(job, cancel_at));
        cancel = true;
        BOOST_REQUIRE(job.Step(0ms));
        const Report report{job.TakeReport()};
        const SlotReport& refused{report.slots[0]};
        BOOST_REQUIRE_EQUAL(refused.attempts.size(), 1U);
        BOOST_CHECK(refused.attempts[0].outcome == Outcome::NotAnnounced);
        BOOST_CHECK(refused.attempts[0].times.ended == DISCOVERY_WINDOW);
        BOOST_CHECK_EQUAL(refused.empty_opportunities, OPPORTUNITIES_PER_SLOT - 1);
        BOOST_CHECK_EQUAL(refused.interrupted, cancel_at < end);
        const SlotReport& announced{report.slots[1]};
        BOOST_REQUIRE_EQUAL(announced.attempts.size(), 1U);
        BOOST_CHECK(announced.attempts[0].outcome == Outcome::PongReceived);
        BOOST_CHECK(!announced.interrupted);
        int empty{0};
        for (const SlotReport& slot : report.slots) empty += slot.empty_opportunities;
        BOOST_CHECK_EQUAL(empty, int(SLOTS * OPPORTUNITIES_PER_SLOT) - 2);
    }
}

BOOST_AUTO_TEST_CASE(progress)
{
    // Interface/Node: Snapshot() gives progress as defined there, at every step.
    tor.resolve = {{"a.seed.", {"8.0.0.1", "8.0.0.2", "8.0.0.3"}}};
    tor.default_behaviour = Behaviour::NoRequest;
    tor.hold = true;
    Job job = MakeJob(Inputs({"a.seed."}), 14);
    const Plan& plan{job.GetPlan()};
    const Assignment expected{Expected(plan, tor.resolve)};
    BOOST_CHECK(job.Snapshot() == Progress{});
    BOOST_REQUIRE(!Settle(job));
    BOOST_CHECK(job.Snapshot() == Progress{});
    for (size_t i{0}; i < tor.streams.size(); ++i) tor.Release(i);
    bool ended{Settle(job)};
    while (!ended) {
        const Progress progress{job.Snapshot()};
        BOOST_CHECK(progress.discovery_done);
        int empty_ended{0};
        for (size_t s{0}; s < SLOTS; ++s) {
            for (size_t k{0}; k < OPPORTUNITIES_PER_SLOT; ++k) {
                if (!expected.opportunities[s][k] && plan.slots[s].scheduled[k] <= Now()) ++empty_ended;
            }
        }
        int attempts_ended{0};
        for (const FakeStream* stream : tor.Attempts()) {
            if (stream->closed) ++attempts_ended;
        }
        BOOST_CHECK_EQUAL(progress.opportunities_ended, empty_ended + attempts_ended);
        BOOST_CHECK_EQUAL(progress.connections, attempts_ended);
        BOOST_CHECK_EQUAL(progress.announcements_written, attempts_ended);
        ended = Pass(job);
    }
    // Three candidates, each announced to and held until its request window ended.
    BOOST_CHECK_EQUAL(job.TakeReport().summary.announcements_written, 3);
}

BOOST_AUTO_TEST_CASE(side_effects)
{
    // A3, B1: a job's only effects are sockets to the proxy made through CreateSock; no files.
    const fs::path cwd{m_path_root / "cwd"};
    fs::create_directories(cwd);
    Report report;
    {
        const CurrentPath current{cwd};
        tor.default_behaviour = Behaviour::NoPong;
        Job job = MakeJob(Inputs(), 15);
        report = RunToEnd(job);
    }
    BOOST_CHECK(fs::is_empty(cwd));
    BOOST_CHECK_EQUAL(tor.streams.size(), QUERIES_PER_SEED * SEEDS.size() + report.summary.connections);
    for (const FakeStream& stream : tor.streams) {
        BOOST_CHECK_EQUAL(stream.domain, AF_INET);
        BOOST_CHECK(Destination(stream) == PROXY);
        BOOST_CHECK(stream.closed);
    }
}

BOOST_AUTO_TEST_CASE(fresh_randomness)
{
    // A4: by default each job draws a fresh plan and fresh credentials from an OS-seeded context:
    // each of the two jobs has QUERIES_PER_SEED query sets, no two sharing credentials (B2).
    Job one{Inputs()};
    Job two{Inputs()};
    BOOST_CHECK(one.GetPlan() != two.GetPlan());
    BOOST_REQUIRE(!Settle(one));
    BOOST_REQUIRE(!Settle(two));
    BOOST_REQUIRE_EQUAL(tor.streams.size(), 2 * QUERIES_PER_SEED * SEEDS.size());
    std::set<std::string> usernames, passwords;
    for (const FakeStream& stream : tor.streams) {
        BOOST_REQUIRE(stream.socks.GetRequest());
        usernames.insert(stream.socks.GetRequest()->username);
        passwords.insert(stream.socks.GetRequest()->password);
    }
    BOOST_CHECK_EQUAL(usernames.size(), 2U * QUERIES_PER_SEED);
    BOOST_CHECK_EQUAL(passwords.size(), 2U * QUERIES_PER_SEED);
}

BOOST_AUTO_TEST_CASE(report_interface)
{
    // Interface/Report: exactly its fields and values; H1: no time of day, only offsets from job
    // start; H3: every endpoint dialled, with the peer's version and user agent, no tx bytes.
    Job job = MakeJob(Inputs(), 17);
    const Assignment expected{Expected(job.GetPlan())};
    const auto host{[&](size_t slot, size_t k) { return Host(expected.opportunities[slot][k]->endpoint); }};
    tor.behaviours = {{host(0, 0), Behaviour::Refused}, {host(1, 0), Behaviour::NoRequest},
                      {host(3, 0), Behaviour::Silent}, {host(4, 0), Behaviour::NoPong},
                      {host(5, 0), Behaviour::Drops}};
    // A user agent with bytes that are not printable.
    tor.user_agents[host(2, 0)] = "/odd\xff\n/";
    const Report report{RunToEnd(job)};
    const UniValue json{ToUniValue(report)};
    const std::string text{json.write()};

    BOOST_CHECK(Keys(json) == (std::set<std::string>{"txid", "wtxid", "chain", "discovery", "slots", "summary"}));
    BOOST_CHECK_EQUAL(json["txid"].get_str(), tx->GetHash().GetHex());
    BOOST_CHECK_EQUAL(json["wtxid"].get_str(), tx->GetWitnessHash().GetHex());
    BOOST_CHECK_EQUAL(json["chain"].get_str(), "regtest");

    const UniValue& discovery{json["discovery"]};
    BOOST_CHECK(Keys(discovery) == (std::set<std::string>{"duration_ms", "seeds", "duplicates", "rejected", "exit_path_candidates"}));
    BOOST_REQUIRE_EQUAL(discovery["seeds"].size(), SEEDS.size());
    int accepted{0};
    for (const UniValue& seed : discovery["seeds"].getValues()) {
        BOOST_CHECK(Keys(seed) == (std::set<std::string>{"name", "queries", "skipped", "answers", "accepted"}));
        BOOST_CHECK_EQUAL(seed["queries"].getInt<int>() + seed["skipped"].getInt<int>(), QUERIES_PER_SEED);
        BOOST_CHECK_EQUAL(seed["answers"].getInt<int>(), 4);
        BOOST_CHECK_EQUAL(seed["accepted"].getInt<int>(), 3);
        accepted += seed["accepted"].getInt<int>();
    }
    BOOST_CHECK_EQUAL(discovery["duplicates"].getInt<int>(), 3);
    BOOST_CHECK_EQUAL(discovery["rejected"].getInt<int>(), 0);
    BOOST_CHECK_EQUAL(discovery["exit_path_candidates"].getInt<int>(), accepted);

    const std::set<std::string> slot_keys{"slot", "stratum", "scheduled_ms", "scheduled_end_ms", "empty_opportunities",
                                          "missed_opportunities", "interrupted", "attempts"};
    const std::set<std::string> outcomes{"not_announced", "announced_not_requested", "tx_written_no_pong", "pong_received",
                                         "post_announcement_failure"};
    std::set<std::string> seen_outcomes;
    BOOST_REQUIRE_EQUAL(json["slots"].size(), SLOTS);
    for (size_t s{0}; s < SLOTS; ++s) {
        const UniValue& slot{json["slots"][s]};
        BOOST_CHECK(Keys(slot) == slot_keys);
        BOOST_CHECK_EQUAL(slot["slot"].getInt<int>(), int(s));
        const std::string stratum{slot["stratum"].get_str()};
        BOOST_CHECK(stratum == (s < 3 ? "prompt" : s == 3 ? "mid" : "late"));
        BOOST_CHECK_EQUAL(slot["scheduled_ms"].size(), OPPORTUNITIES_PER_SLOT);
        BOOST_CHECK(slot["interrupted"].isBool());
        for (const UniValue& attempt : slot["attempts"].getValues()) {
            BOOST_CHECK(Keys(attempt) == ATTEMPT_KEYS);
            const std::string outcome{attempt["outcome"].get_str()};
            BOOST_CHECK(outcomes.contains(outcome));
            seen_outcomes.insert(outcome);
            BOOST_CHECK_EQUAL(std::ranges::count(SEEDS, attempt["provenance"].get_str()), 1);
            // H3: the endpoint, and the peer's VERSION as it sent it.
            const std::string endpoint{attempt["endpoint"].get_str()};
            BOOST_CHECK(endpoint.ends_with(strprintf(":%d", PORT)));
            const std::string sent_host{endpoint.substr(0, endpoint.rfind(':'))};
            const auto behaviour{tor.behaviours.find(sent_host)};
            const bool handshaken{behaviour == tor.behaviours.end() || (behaviour->second != Behaviour::Refused && behaviour->second != Behaviour::Silent)};
            if (handshaken) {
                BOOST_CHECK_EQUAL(attempt["peer_version"].getInt<int>(), PEER_VERSION);
                BOOST_CHECK_EQUAL(attempt["peer_user_agent"].get_str(), sent_host == host(2, 0) ? "/odd/" : PEER_USER_AGENT);
            } else {
                BOOST_CHECK(attempt["peer_version"].isNull());
                BOOST_CHECK(attempt["peer_user_agent"].isNull());
            }
        }
    }
    BOOST_CHECK(seen_outcomes == outcomes);

    BOOST_CHECK(Keys(json["summary"]) == SUMMARY_KEYS);
    BOOST_CHECK_EQUAL(json["summary"]["slots_completed"].getInt<int>(), int(SLOTS));
    BOOST_CHECK(!json["summary"]["interrupted"].get_bool());
    BOOST_CHECK(json["summary"]["error"].isNull());

    // H1: no field is named for a time of day, every time is an offset from job start, and no
    // number is anywhere near one.
    const std::function<void(const UniValue&)> check_times{[&](const UniValue& value) {
        if (value.isObject()) {
            for (const std::string& key : value.getKeys()) {
                BOOST_CHECK(!key.starts_with("time"));
                const UniValue& field{value[key]};
                if (key.ends_with("_ms")) {
                    for (const UniValue& time : field.isArray() ? field.getValues() : std::vector<UniValue>{field}) {
                        if (time.isNull()) continue;
                        BOOST_CHECK(time.getInt<int64_t>() >= 0);
                        BOOST_CHECK(time.getInt<int64_t>() <= count_milliseconds(SCHEDULED_BOUND));
                    }
                }
                check_times(field);
            }
        } else if (value.isArray()) {
            for (const UniValue& element : value.getValues()) check_times(element);
        } else if (value.isNum()) {
            BOOST_CHECK(value.getInt<int64_t>() < 1'000'000'000);
        }
    }};
    check_times(json);

    // The text is valid JSON, and holds no transaction bytes.
    UniValue parsed;
    BOOST_CHECK(parsed.read(text));
    CheckNoTxBytes(text, *tx);
}

BOOST_AUTO_TEST_CASE(package_report)
{
    // F1-F3: recipients get the parent only by asking after the child; for the others the PING
    // waits out the hold. The report adds the parent's ids, its times and parents_served.
    const CTransactionRef parent{MakeParent()};
    JobInputs inputs{Inputs()};
    inputs.parent = parent;
    tor.parent_txid = parent->GetHash();
    Job job = MakeJob(std::move(inputs), 19);
    const Assignment expected{Expected(job.GetPlan())};
    const std::set<std::string> asking{Host(expected.opportunities[0][0]->endpoint), Host(expected.opportunities[2][0]->endpoint)};
    for (const std::string& host : asking) tor.behaviours[host] = Behaviour::AsksParent;
    const Report report{RunToEnd(job)};
    const UniValue json{ToUniValue(report)};
    const std::string text{json.write()};

    BOOST_CHECK(Keys(json) == (std::set<std::string>{"txid", "wtxid", "parent_txid", "parent_wtxid", "chain", "discovery", "slots", "summary"}));
    BOOST_CHECK_EQUAL(json["txid"].get_str(), tx->GetHash().GetHex());
    BOOST_CHECK_EQUAL(json["parent_txid"].get_str(), parent->GetHash().GetHex());
    BOOST_CHECK_EQUAL(json["parent_wtxid"].get_str(), parent->GetWitnessHash().GetHex());
    std::set<std::string> attempt_keys{ATTEMPT_KEYS};
    attempt_keys.insert({"parent_getdata_ms", "parent_tx_written_ms", "parent_hold_expired_ms"});
    const int64_t hold{count_milliseconds(PARENT_HOLD)};
    int attempts{0};
    for (const UniValue& slot : json["slots"].getValues()) {
        for (const UniValue& attempt : slot["attempts"].getValues()) {
            ++attempts;
            BOOST_CHECK(Keys(attempt) == attempt_keys);
            BOOST_CHECK_EQUAL(attempt["outcome"].get_str(), "pong_received");
            const std::string endpoint{attempt["endpoint"].get_str()};
            const int64_t tx_written{attempt["tx_written_ms"].getInt<int64_t>()};
            const int64_t ping_written{attempt["ping_written_ms"].getInt<int64_t>()};
            if (asking.contains(endpoint.substr(0, endpoint.rfind(':')))) {
                BOOST_CHECK(attempt["parent_getdata_ms"].getInt<int64_t>() >= attempt["getdata_ms"].getInt<int64_t>());
                BOOST_CHECK(attempt["parent_tx_written_ms"].getInt<int64_t>() >= tx_written);
                BOOST_CHECK(attempt["parent_hold_expired_ms"].isNull());
                BOOST_CHECK(ping_written < tx_written + hold);
            } else {
                BOOST_CHECK(attempt["parent_getdata_ms"].isNull());
                BOOST_CHECK(attempt["parent_tx_written_ms"].isNull());
                // The hold ends in the first step at or after PARENT_HOLD; the job steps each second.
                const int64_t expired{attempt["parent_hold_expired_ms"].getInt<int64_t>()};
                BOOST_CHECK(expired >= tx_written + hold && expired <= tx_written + hold + 1000);
                BOOST_CHECK_EQUAL(ping_written, expired);
            }
        }
    }
    BOOST_CHECK_EQUAL(attempts, int(SLOTS));
    std::set<std::string> summary_keys{SUMMARY_KEYS};
    summary_keys.insert("parents_served");
    BOOST_CHECK(Keys(json["summary"]) == summary_keys);
    BOOST_CHECK_EQUAL(json["summary"]["parents_served"].getInt<int>(), 2);
    BOOST_CHECK_EQUAL(json["summary"]["pongs"].getInt<int>(), int(SLOTS));

    // One announcement each, and the parent only to those who asked for it.
    BOOST_CHECK_EQUAL(tor.Attempts().size(), SLOTS);
    for (const FakeStream* stream : tor.Attempts()) {
        BOOST_REQUIRE(stream->recipient);
        const std::vector<std::string>& sent{stream->recipient->Messages()};
        BOOST_CHECK_EQUAL(std::ranges::count(sent, std::string{NetMsgType::INV}), 1);
        BOOST_CHECK_EQUAL(std::ranges::count(sent, std::string{NetMsgType::TX}), asking.contains(stream->socks.GetRequest()->host) ? 2 : 1);
    }
    // H3: neither the transaction's nor the parent's serialization appears in the report.
    for (const CTransactionRef& sent : {tx, parent}) CheckNoTxBytes(text, *sent);
}

BOOST_AUTO_TEST_CASE(discovery_only)
{
    // RunDiscoveryOnly() needs no transaction, makes a job's queries, dials no one, and returns
    // what `discover` prints: each seed's candidates.
    JobInputs inputs{Inputs()};
    inputs.tx = nullptr;
    Job job = MakeJob(std::move(inputs), 18);
    const DiscoveryReport discovery{job.RunDiscoveryOnly().discovery};
    BOOST_CHECK_EQUAL(tor.streams.size(), QUERIES_PER_SEED * SEEDS.size());
    for (const FakeStream& stream : tor.streams) {
        BOOST_CHECK(stream.Is(RESOLVE));
        BOOST_CHECK(stream.closed);
    }
    const UniValue json{ToUniValue(discovery, /*candidates=*/true)};
    BOOST_CHECK_EQUAL(json["exit_path_candidates"].getInt<int>(), 9);
    std::set<std::string> candidates;
    for (const UniValue& seed : json["seeds"].getValues()) {
        BOOST_CHECK_EQUAL(seed["candidates"].size(), 3U);
        for (const UniValue& candidate : seed["candidates"].getValues()) candidates.insert(candidate.get_str());
    }
    std::set<std::string> answered;
    for (const auto& [_, addresses] : ANSWERS) {
        for (const std::string& address : addresses) answered.insert(Service(address).ToStringAddrPort());
    }
    BOOST_CHECK(candidates == answered);
    // The job's own report does not show them.
    const UniValue plain{ToUniValue(discovery, /*candidates=*/false)};
    for (const UniValue& seed : plain["seeds"].getValues()) BOOST_CHECK(!seed.exists("candidates"));
}

BOOST_AUTO_TEST_SUITE_END()
