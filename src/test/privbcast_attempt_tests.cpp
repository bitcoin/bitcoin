// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <net_transport.h>
#include <netmessagemaker.h>
#include <primitives/transaction.h>
#include <privbcast/attempt.h>
#include <privbcast/params.h>
#include <protocol.h>
#include <random.h>
#include <script/script.h>
#include <span.h>
#include <streams.h>
#include <test/util/privbcast.h>
#include <test/util/setup_common.h>
#include <uint256.h>

#include <boost/test/unit_test.hpp>

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <limits>
#include <optional>
#include <span>
#include <string>
#include <utility>
#include <vector>

using namespace privbcast;
using namespace std::chrono_literals;
using std::chrono::milliseconds;

namespace {

/** The opportunity's scheduled start, for every attempt here. */
constexpr milliseconds START{18s};
/** Services of a peer that passes E2. */
constexpr uint64_t PEER_SERVICES{NODE_NETWORK | NODE_WITNESS};

using Types = std::vector<std::string>;

/** Package mode: a child of MakeParent()'s transaction, with a witness too. */
CTransactionRef MakeChild(const CTransaction& parent)
{
    CMutableTransaction tx;
    tx.vin.emplace_back(COutPoint{parent.GetHash(), 0});
    tx.vin[0].scriptWitness.stack.push_back({7, 8, 9});
    tx.vout.emplace_back(10'000, CScript{} << OP_TRUE);
    return MakeTransactionRef(std::move(tx));
}

CSerializedNetMsg Msg(std::string type, std::vector<uint8_t> payload = {})
{
    CSerializedNetMsg msg;
    msg.m_type = std::move(type);
    msg.data = std::move(payload);
    return msg;
}

CSerializedNetMsg Version(int version = 70016, uint64_t services = PEER_SERVICES, bool relay = true,
                          const std::string& user_agent = "/Satoshi:30.0.0/")
{
    return PeerVersion(version, services, relay, user_agent);
}

CSerializedNetMsg GetData(std::vector<CInv> request) { return NetMsg::Make(NetMsgType::GETDATA, request); }

CSerializedNetMsg Pong(uint64_t nonce) { return NetMsg::Make(NetMsgType::PONG, nonce); }

uint64_t Nonce(const std::vector<uint8_t>& ping)
{
    uint64_t nonce{0};
    DataStream{ping} >> nonce;
    return nonce;
}

/** A message the peer decoded. */
struct Message {
    std::string type;
    std::vector<uint8_t> payload;
};

/** The entries of an INV, GETDATA or NOTFOUND, as type and hash, which compare. */
using Entries = std::vector<std::pair<uint32_t, uint256>>;

Entries ToEntries(const std::vector<CInv>& inv)
{
    Entries entries;
    for (const CInv& entry : inv) entries.emplace_back(entry.type, entry.hash);
    return entries;
}

/** The entries of a NOTFOUND the peer received. */
Entries NotFound(const Message& msg)
{
    BOOST_REQUIRE_EQUAL(msg.type, NetMsgType::NOTFOUND);
    std::vector<CInv> inv;
    DataStream stream{msg.payload};
    stream >> inv;
    BOOST_CHECK(stream.empty());
    return ToEntries(inv);
}

/** The wtxid of the transaction, with its witness, in a TX the peer received. */
Wtxid Served(const Message& msg)
{
    BOOST_REQUIRE_EQUAL(msg.type, NetMsgType::TX);
    CMutableTransaction sent;
    DataStream stream{msg.payload};
    stream >> TX_WITH_WITNESS(sent);
    BOOST_CHECK(stream.empty());
    return CTransaction{sent}.GetWitnessHash();
}

/** The recipient's end of an attempt's connection: a BIP324 responder, as a node runs one. It
 *  decodes what the attempt writes and sends the messages a test scripts. */
class Peer
{
public:
    explicit Peer(FastRandomContext& rng) : m_transport(MakeResponder(rng)) {}

    /** Queue a message for the attempt. */
    void Send(CSerializedNetMsg msg) { m_queue.push_back(std::move(msg)); }

    /** Pass up to max of the peer's bytes to the attempt at now. Returns how many. */
    size_t Deliver(Attempt& attempt, milliseconds now, size_t max = std::numeric_limits<size_t>::max())
    {
        size_t total{0};
        while (total < max) {
            if (!m_queue.empty() && m_transport.SetMessageToSend(m_queue.front())) m_queue.pop_front();
            const auto& [bytes, _more, _type]{m_transport.GetBytesToSend(/*have_next_message=*/!m_queue.empty())};
            if (bytes.empty()) break;
            const std::vector<uint8_t> chunk{bytes.begin(), bytes.begin() + std::min(bytes.size(), max - total)};
            m_transport.MarkBytesSent(chunk.size());
            attempt.Received(chunk, now);
            total += chunk.size();
            m_delivered += chunk.size();
        }
        return total;
    }

    /** Pass all of the peer's queued messages to the attempt at now in one read, as a socket read
     *  that returns several messages at once. Returns how many bytes. */
    size_t DeliverInOneRead(Attempt& attempt, milliseconds now)
    {
        std::vector<uint8_t> read;
        while (true) {
            if (!m_queue.empty() && m_transport.SetMessageToSend(m_queue.front())) m_queue.pop_front();
            const auto& [bytes, _more, _type]{m_transport.GetBytesToSend(/*have_next_message=*/!m_queue.empty())};
            if (bytes.empty()) break;
            read.insert(read.end(), bytes.begin(), bytes.end());
            m_transport.MarkBytesSent(bytes.size());
        }
        if (!read.empty()) attempt.Received(read, now);
        m_delivered += read.size();
        return read.size();
    }

    /** Write up to max of the bytes the attempt offers, marking them sent at now, and decode them.
     *  Returns how many. */
    size_t Read(Attempt& attempt, milliseconds now, size_t max = std::numeric_limits<size_t>::max())
    {
        size_t total{0};
        while (total < max) {
            const std::span<const uint8_t> offered{attempt.BytesToSend()};
            if (offered.empty()) break;
            const std::vector<uint8_t> chunk{offered.begin(), offered.begin() + std::min(offered.size(), max - total)};
            attempt.MarkSent(chunk.size(), now);
            total += chunk.size();
            m_wire.insert(m_wire.end(), chunk.begin(), chunk.end());
            std::span<const uint8_t> rest{chunk};
            while (!rest.empty()) {
                BOOST_REQUIRE(m_transport.ReceivedBytes(rest));
                if (!m_transport.ReceivedMessageComplete()) continue;
                bool reject{false};
                CNetMessage msg{m_transport.GetReceivedMessage({}, reject)};
                BOOST_REQUIRE(!reject);
                const auto payload{MakeUCharSpan(msg.m_recv)};
                m_received.push_back({msg.m_type, {payload.begin(), payload.end()}});
            }
        }
        return total;
    }

    /** Write the message the attempt's transport holds, which goes out in full before the next. */
    size_t ReadMessage(Attempt& attempt, milliseconds now) { return Read(attempt, now, attempt.BytesToSend().size()); }

    /** Both ways, until neither side has anything left to write. */
    void Exchange(Attempt& attempt, milliseconds now)
    {
        while (Read(attempt, now) + Deliver(attempt, now) > 0) {}
    }

    const std::vector<Message>& Received() const { return m_received; }
    Types ReceivedTypes() const
    {
        Types types;
        for (const Message& msg : m_received) types.push_back(msg.type);
        return types;
    }
    /** Every byte the attempt wrote. */
    const std::vector<uint8_t>& Wire() const { return m_wire; }
    /** Every byte passed to the attempt. */
    uint64_t Delivered() const { return m_delivered; }

private:
    V2Transport m_transport;
    std::deque<CSerializedNetMsg> m_queue;
    std::vector<Message> m_received;
    std::vector<uint8_t> m_wire;
    uint64_t m_delivered{0};
};

/** Connect at `at` and run the handshake with a peer that passes E2: the INV is handed and written
 *  at `at`. */
void Announce(Attempt& attempt, Peer& peer, milliseconds at)
{
    attempt.Connected(at);
    peer.Exchange(attempt, at);
    peer.Send(Version());
    peer.Send(Msg(NetMsgType::WTXIDRELAY));
    peer.Send(Msg(NetMsgType::VERACK));
    peer.Exchange(attempt, at);
    BOOST_REQUIRE(attempt.Times().inv_handed == at);
    BOOST_REQUIRE(attempt.Times().inv_written == at);
}

const Types HANDSHAKE{NetMsgType::VERSION, NetMsgType::WTXIDRELAY, NetMsgType::VERACK};
const Types ANNOUNCED{NetMsgType::VERSION, NetMsgType::WTXIDRELAY, NetMsgType::VERACK, NetMsgType::INV};
const Types SERVED{NetMsgType::VERSION, NetMsgType::WTXIDRELAY, NetMsgType::VERACK, NetMsgType::INV,
                   NetMsgType::TX, NetMsgType::PING};

Types Then(Types types, const Types& more)
{
    types.insert(types.end(), more.begin(), more.end());
    return types;
}

struct AttemptSetup : public BasicTestingSetup {
    const CTransactionRef tx{MakeTx()};
    const uint256 wtxid{tx->GetWitnessHash().ToUint256()};
    /** Package mode. */
    const CTransactionRef parent{MakeParent()};
    const CTransactionRef child{MakeChild(*parent)};
    const uint256 parent_txid{parent->GetHash().ToUint256()};
    const uint256 parent_wtxid{parent->GetWitnessHash().ToUint256()};
    const uint256 child_txid{child->GetHash().ToUint256()};
    const uint256 child_wtxid{child->GetWitnessHash().ToUint256()};
};

} // namespace

BOOST_FIXTURE_TEST_SUITE(privbcast_attempt_tests, AttemptSetup)

BOOST_AUTO_TEST_CASE(peer_version)
{
    // E2: a low version, no NODE_WITNESS, no relay, or a VERSION that does not read in full (its user
    // agent over 256 bytes) ends the attempt; it sent only its VERSION.
    FastRandomContext rng{uint256{2}};
    struct Case {
        int version;
        uint64_t services;
        bool relay;
        std::string user_agent{"/Satoshi:30.0.0/"};
    };
    for (const Case& c : {Case{70015, PEER_SERVICES, true}, Case{70016, NODE_NETWORK, true}, Case{70016, PEER_SERVICES, false},
                          Case{70016, PEER_SERVICES, true, std::string(257, 'a')}}) {
        Attempt attempt{tx, START, Timing{}, rng, KeysFrom(rng), NodeId{0}};
        Peer peer{rng};
        attempt.Connected(START);
        peer.Exchange(attempt, START);
        peer.Send(Version(c.version, c.services, c.relay, c.user_agent));
        peer.Send(Msg(NetMsgType::WTXIDRELAY));
        peer.Send(Msg(NetMsgType::VERACK));
        peer.Exchange(attempt, START + 1s);
        BOOST_CHECK(attempt.Ended());
        BOOST_CHECK(attempt.GetOutcome() == Outcome::NotAnnounced);
        BOOST_CHECK(attempt.Times().ended == START + 1s);
        BOOST_CHECK(peer.ReceivedTypes() == Types{NetMsgType::VERSION});
        BOOST_CHECK(attempt.BytesToSend().empty());
    }

    // A VERSION cut before its relay flag does not read in full; bytes after the flag are ignored.
    for (const bool trailing : {false, true}) {
        Attempt attempt{tx, START, Timing{}, rng, KeysFrom(rng), NodeId{0}};
        Peer peer{rng};
        attempt.Connected(START);
        peer.Exchange(attempt, START);
        CSerializedNetMsg version{Version()};
        if (trailing) {
            version.data.push_back(0x00);
        } else {
            version.data.pop_back();
        }
        peer.Send(std::move(version));
        peer.Exchange(attempt, START + 1s);
        BOOST_CHECK_EQUAL(attempt.Ended(), !trailing);
        BOOST_CHECK(peer.ReceivedTypes() == (trailing ? HANDSHAKE : Types{NetMsgType::VERSION}));
    }

    // Only the peer's first VERSION counts: a second one, good or not, changes nothing, and the
    // handshake goes on to the INV.
    for (const bool good : {true, false}) {
        Attempt attempt{tx, START, Timing{}, rng, KeysFrom(rng), NodeId{0}};
        Peer peer{rng};
        attempt.Connected(START);
        peer.Exchange(attempt, START);
        peer.Send(Version());
        peer.Send(good ? Version() : Version(70015));
        peer.Exchange(attempt, START + 1s);
        BOOST_CHECK(!attempt.Ended());
        BOOST_CHECK(peer.ReceivedTypes() == HANDSHAKE);
        peer.Send(Msg(NetMsgType::WTXIDRELAY));
        peer.Send(Msg(NetMsgType::VERACK));
        peer.Exchange(attempt, START + 1s);
        BOOST_CHECK(peer.ReceivedTypes() == ANNOUNCED);
    }

    // The peer's VERACK without a WTXIDRELAY before it ends the attempt; a WTXIDRELAY before the
    // VERSION does not count.
    const std::vector<Types> scripts{
        {NetMsgType::VERSION, NetMsgType::VERACK},
        {NetMsgType::WTXIDRELAY, NetMsgType::VERSION, NetMsgType::VERACK},
    };
    for (const Types& script : scripts) {
        Attempt attempt{tx, START, Timing{}, rng, KeysFrom(rng), NodeId{0}};
        Peer peer{rng};
        attempt.Connected(START);
        peer.Exchange(attempt, START);
        for (const std::string& type : script) {
            peer.Send(type == NetMsgType::VERSION ? Version() : Msg(type));
            peer.Exchange(attempt, START + 1s);
        }
        BOOST_CHECK(attempt.Ended());
        BOOST_CHECK(attempt.GetOutcome() == Outcome::NotAnnounced);
        BOOST_CHECK(!attempt.Times().inv_handed);
        BOOST_CHECK(peer.ReceivedTypes() == HANDSHAKE);
    }
}

BOOST_AUTO_TEST_CASE(getdata_before_announcement)
{
    // E4: a GETDATA before the announcement point is ignored and not counted.
    FastRandomContext rng{uint256{4}};
    const CInv request{MSG_WTX, wtxid};
    {
        Attempt attempt{tx, START, Timing{}, rng, KeysFrom(rng), NodeId{0}};
        Peer peer{rng};
        attempt.Connected(START);
        peer.Exchange(attempt, START);
        peer.Send(GetData({request}));
        peer.Send(Version());
        peer.Send(GetData({request}));
        peer.Send(Msg(NetMsgType::WTXIDRELAY));
        peer.Send(GetData({request}));
        peer.Exchange(attempt, START + 1s);
        BOOST_CHECK_EQUAL(attempt.ExtraRequests(), 0);
        BOOST_CHECK(!attempt.Times().getdata);
        peer.Send(Msg(NetMsgType::VERACK));
        peer.Exchange(attempt, START + 2s);
        BOOST_CHECK(peer.ReceivedTypes() == ANNOUNCED);
        BOOST_CHECK_EQUAL(attempt.ExtraRequests(), 0);
        // They did not use up the one request that is served.
        peer.Send(GetData({request}));
        peer.Exchange(attempt, START + 3s);
        BOOST_CHECK(attempt.Times().getdata == START + 3s);
        BOOST_CHECK(peer.ReceivedTypes() == SERVED);
    }
    {
        // After the peer's VERACK, while the INV waits for the attempt's VERACK to be written.
        Attempt attempt{tx, START, Timing{}, rng, KeysFrom(rng), NodeId{0}};
        Peer peer{rng};
        attempt.Connected(START);
        peer.Exchange(attempt, START);
        peer.Send(Version());
        peer.Send(Msg(NetMsgType::WTXIDRELAY));
        peer.Send(Msg(NetMsgType::VERACK));
        peer.Send(GetData({request}));
        peer.Deliver(attempt, START + 1s);
        BOOST_CHECK(!attempt.Times().inv_handed);
        peer.Exchange(attempt, START + 2s);
        BOOST_CHECK(attempt.Times().inv_handed == START + 2s);
        BOOST_CHECK(peer.ReceivedTypes() == ANNOUNCED);
        BOOST_CHECK_EQUAL(attempt.ExtraRequests(), 0);
        BOOST_CHECK(!attempt.Times().getdata);
    }
}

BOOST_AUTO_TEST_CASE(getdata_while_inv_is_written)
{
    // E4: a GETDATA after the INV was handed, before its last byte went, is served after the INV.
    FastRandomContext rng{uint256{5}};
    Attempt attempt{tx, START, Timing{}, rng, KeysFrom(rng), NodeId{0}};
    Peer peer{rng};
    attempt.Connected(START);
    peer.Exchange(attempt, START);
    peer.Send(Version());
    peer.Send(Msg(NetMsgType::WTXIDRELAY));
    peer.Send(Msg(NetMsgType::VERACK));
    peer.Deliver(attempt, START + 1s);
    peer.ReadMessage(attempt, START + 1s); // WTXIDRELAY
    peer.ReadMessage(attempt, START + 1s); // VERACK
    BOOST_REQUIRE(attempt.Times().inv_handed == START + 1s);
    BOOST_CHECK_EQUAL(peer.Read(attempt, START + 1s, 1), 1U);
    BOOST_CHECK(!attempt.Times().inv_written);
    peer.Send(GetData({CInv{MSG_WTX, wtxid}}));
    peer.Deliver(attempt, START + 2s);
    BOOST_CHECK(attempt.Times().getdata == START + 2s);
    peer.Exchange(attempt, START + 3s);
    BOOST_CHECK(peer.ReceivedTypes() == SERVED);
    BOOST_CHECK(attempt.Times().inv_written == START + 3s);
    BOOST_CHECK(attempt.Times().tx_written == START + 3s);
}

BOOST_AUTO_TEST_CASE(served_then_malformed_getdata)
{
    // E5, E7: once served, an undecodable GETDATA is only counted; the attempt goes on to its PONG.
    FastRandomContext rng{uint256{17}};
    Attempt attempt{tx, START, Timing{}, rng, KeysFrom(rng), NodeId{0}};
    Peer peer{rng};
    Announce(attempt, peer, START);
    peer.Send(GetData({CInv{MSG_WTX, wtxid}}));
    peer.Deliver(attempt, START + 1s);
    BOOST_REQUIRE(attempt.Times().getdata == START + 1s);
    // A count of one entry, and no entry.
    peer.Send(Msg(NetMsgType::GETDATA, {0x01}));
    peer.Deliver(attempt, START + 1s);
    BOOST_CHECK(!attempt.Ended());
    BOOST_CHECK_EQUAL(attempt.ExtraRequests(), 1);
    BOOST_CHECK(!attempt.Times().tx_written);
    peer.Exchange(attempt, START + 2s);
    BOOST_REQUIRE(peer.ReceivedTypes() == SERVED);
    // A count cut short.
    peer.Send(Msg(NetMsgType::GETDATA, {0xfd, 0x01}));
    peer.Exchange(attempt, START + 3s);
    BOOST_CHECK(!attempt.Ended());
    BOOST_CHECK_EQUAL(attempt.ExtraRequests(), 2);
    peer.Send(Pong(Nonce(peer.Received().back().payload)));
    peer.Exchange(attempt, START + 4s);
    BOOST_CHECK(attempt.GetOutcome() == Outcome::PongReceived);
    BOOST_CHECK_EQUAL(attempt.ExtraRequests(), 2);
    BOOST_CHECK(peer.ReceivedTypes() == SERVED);
}

BOOST_AUTO_TEST_CASE(pong_nonce)
{
    // E6: only a PONG that carries the PING's nonce ends the attempt; one before the PING, or with
    // another nonce, is ignored.
    FastRandomContext rng{uint256{12}};
    Attempt attempt{tx, START, Timing{}, rng, KeysFrom(rng), NodeId{0}};
    Peer peer{rng};
    Announce(attempt, peer, START);
    peer.Send(Pong(0));
    peer.Send(GetData({CInv{MSG_WTX, wtxid}}));
    peer.Exchange(attempt, START + 1s);
    BOOST_REQUIRE(peer.ReceivedTypes() == SERVED);
    const uint64_t nonce{Nonce(peer.Received().back().payload)};
    peer.Send(Pong(nonce + 1));
    peer.Exchange(attempt, START + 2s);
    BOOST_CHECK(!attempt.Ended());
    BOOST_CHECK(!attempt.Times().pong);
    peer.Send(Pong(nonce));
    peer.Exchange(attempt, START + 3s);
    BOOST_CHECK(attempt.GetOutcome() == Outcome::PongReceived);
    BOOST_CHECK(attempt.Times().pong == START + 3s);
}

BOOST_AUTO_TEST_CASE(ignored_messages)
{
    // E7: messages of other kinds, or out of order, are ignored and answered with nothing.
    FastRandomContext rng{uint256{9}};
    Attempt attempt{tx, START, Timing{}, rng, KeysFrom(rng), NodeId{0}};
    Peer peer{rng};
    attempt.Connected(START);
    peer.Exchange(attempt, START);
    const auto noise{[&] {
        peer.Send(NetMsg::Make(NetMsgType::PING, uint64_t{7}));
        peer.Send(NetMsg::Make(NetMsgType::SENDTXRCNCL, uint32_t{1}, uint64_t{2}));
        peer.Send(Msg(NetMsgType::ADDR, {0x00}));
        peer.Send(Msg("unknowncmd", {1, 2, 3}));
    }};
    peer.Send(Msg(NetMsgType::VERACK));
    noise();
    peer.Exchange(attempt, START + 1s);
    BOOST_CHECK(!attempt.Ended());
    BOOST_CHECK(peer.ReceivedTypes() == Types{NetMsgType::VERSION});
    peer.Send(Version());
    noise();
    peer.Send(Msg(NetMsgType::WTXIDRELAY));
    noise();
    peer.Send(Msg(NetMsgType::VERACK));
    noise();
    peer.Exchange(attempt, START + 2s);
    BOOST_CHECK(attempt.Times().inv_handed == START + 2s);
    BOOST_CHECK(peer.ReceivedTypes() == ANNOUNCED);
    BOOST_CHECK(!attempt.Ended());
}

BOOST_AUTO_TEST_CASE(deadlines)
{
    // D2: a GETDATA read after the request window has ended is not processed, whether or not the
    // attempt saw the end first; if it did not, the GETDATA's bytes still count as read (D3).
    FastRandomContext rng{uint256{11}};
    for (const bool tick : {true, false}) {
        Attempt attempt{tx, START, Timing{}, rng, KeysFrom(rng), NodeId{0}};
        Peer peer{rng};
        const milliseconds announced{START + 1ms};
        Announce(attempt, peer, announced);
        const milliseconds deadline{announced + REQUEST_WINDOW};
        BOOST_CHECK(attempt.NextDeadline() == deadline);
        attempt.Tick(deadline - 1s);
        BOOST_CHECK(!attempt.Ended());
        const milliseconds late{deadline + 1s};
        if (tick) attempt.Tick(late);
        const uint64_t before{peer.Delivered()};
        BOOST_REQUIRE_EQUAL(attempt.BytesRecv(), before);
        peer.Send(GetData({CInv{MSG_WTX, wtxid}}));
        peer.Exchange(attempt, late);
        BOOST_REQUIRE(peer.Delivered() > before);
        BOOST_CHECK_EQUAL(attempt.BytesRecv(), tick ? before : peer.Delivered());
        BOOST_CHECK(attempt.Ended());
        BOOST_CHECK(attempt.GetOutcome() == Outcome::AnnouncedNotRequested);
        BOOST_CHECK(attempt.Times().ended == late);
        BOOST_CHECK(!attempt.Times().getdata);
        BOOST_CHECK_EQUAL(attempt.ExtraRequests(), 0);
        BOOST_CHECK(peer.ReceivedTypes() == ANNOUNCED);
    }
}

BOOST_AUTO_TEST_CASE(requested_not_written)
{
    // E6: at the request window's end, a TX not written is a failure after the announcement
    // point; a TX written without the PING is tx_written_no_pong.
    FastRandomContext rng{uint256{18}};
    const milliseconds deadline{START + REQUEST_WINDOW};
    for (const bool tx_written : {false, true}) {
        Attempt attempt{tx, START, Timing{}, rng, KeysFrom(rng), NodeId{0}};
        Peer peer{rng};
        Announce(attempt, peer, START);
        peer.Send(GetData({CInv{MSG_WTX, wtxid}}));
        peer.Deliver(attempt, START + 1s);
        BOOST_REQUIRE(attempt.Times().getdata == START + 1s);
        if (tx_written) peer.ReadMessage(attempt, START + 1s);
        // The TX, or the PING, written but for its last byte.
        peer.Read(attempt, START + 1s, attempt.BytesToSend().size() - 1);
        attempt.Tick(deadline - 1s);
        BOOST_CHECK(!attempt.Ended());
        attempt.Tick(deadline + 1s);
        BOOST_CHECK(attempt.Ended());
        BOOST_CHECK(attempt.Times().ended == deadline + 1s);
        BOOST_CHECK_EQUAL(attempt.Times().tx_written.has_value(), tx_written);
        BOOST_CHECK(!attempt.Times().ping_written);
        BOOST_CHECK(attempt.GetOutcome() == (tx_written ? Outcome::TxWrittenNoPong : Outcome::PostAnnouncementFailure));
        BOOST_CHECK(attempt.BytesToSend().empty());
    }
}

BOOST_AUTO_TEST_CASE(receive_cap)
{
    // D3: every byte read counts, the handshake's too, and a peer that sends past the cap is cut
    // off.
    FastRandomContext rng{uint256{12}};
    for (const bool announced : {false, true}) {
        Attempt attempt{tx, START, Timing{}, rng, KeysFrom(rng), NodeId{0}};
        Peer peer{rng};
        attempt.Connected(START);
        peer.Exchange(attempt, START);
        peer.Send(Version());
        peer.Send(Msg(NetMsgType::WTXIDRELAY));
        if (announced) peer.Send(Msg(NetMsgType::VERACK));
        peer.Exchange(attempt, START);
        BOOST_CHECK_EQUAL(attempt.Times().inv_handed.has_value(), announced);
        BOOST_CHECK_EQUAL(attempt.BytesRecv(), peer.Delivered());
        BOOST_CHECK_EQUAL(attempt.BytesSent(), peer.Wire().size());
        // Messages the attempt ignores: half the cap is read on, as much again ends the attempt.
        peer.Send(Msg("filler", std::vector<uint8_t>(MAX_RECV_BYTES / 2)));
        peer.Exchange(attempt, START + 1s);
        BOOST_CHECK(!attempt.Ended());
        peer.Send(Msg("filler", std::vector<uint8_t>(MAX_RECV_BYTES)));
        peer.Exchange(attempt, START + 2s);
        BOOST_CHECK(attempt.Ended());
        BOOST_CHECK(attempt.GetOutcome() == (announced ? Outcome::PostAnnouncementFailure : Outcome::NotAnnounced));
        BOOST_CHECK(attempt.Times().ended == START + 2s);
        BOOST_CHECK_GT(attempt.BytesRecv(), MAX_RECV_BYTES);
        BOOST_CHECK_EQUAL(attempt.BytesSent(), peer.Wire().size());
    }
}

BOOST_AUTO_TEST_CASE(written_at_last_byte)
{
    // C6, Interface/Report: inv_written, tx_written and ping_written are set when the last byte is
    // marked sent.
    FastRandomContext rng{uint256{13}};
    Attempt attempt{tx, START, Timing{}, rng, KeysFrom(rng), NodeId{0}};
    Peer peer{rng};
    attempt.Connected(START);
    peer.Exchange(attempt, START);
    peer.Send(Version());
    peer.Send(Msg(NetMsgType::WTXIDRELAY));
    peer.Send(Msg(NetMsgType::VERACK));
    peer.Deliver(attempt, START);
    peer.ReadMessage(attempt, START);
    peer.ReadMessage(attempt, START);
    BOOST_REQUIRE(attempt.Times().inv_handed == START);

    milliseconds now{START};
    const auto written{[&](const std::optional<milliseconds> AttemptTimes::*field) {
        const size_t size{attempt.BytesToSend().size()};
        BOOST_REQUIRE(size > 2);
        BOOST_CHECK_EQUAL(peer.Read(attempt, now += 1ms, 1), 1U);
        BOOST_CHECK(!(attempt.Times().*field));
        BOOST_CHECK_EQUAL(peer.Read(attempt, now += 1ms, size - 2), size - 2);
        BOOST_CHECK(!(attempt.Times().*field));
        BOOST_CHECK_EQUAL(peer.Read(attempt, now += 1ms, 1), 1U);
        BOOST_CHECK(attempt.Times().*field == now);
    }};
    written(&AttemptTimes::inv_written);
    peer.Send(GetData({CInv{MSG_WTX, wtxid}}));
    peer.Deliver(attempt, now);
    written(&AttemptTimes::tx_written);
    written(&AttemptTimes::ping_written);
    BOOST_CHECK(peer.ReceivedTypes() == SERVED);
}

BOOST_AUTO_TEST_CASE(keys_from_strong_rng)
{
    // A4: without a key source, as in production, the key comes from the strong RNG, not from rng.
    const uint256 seed{1};
    FastRandomContext rng_a{seed};
    FastRandomContext rng_b{seed};
    Attempt a{tx, START, Timing{}, rng_a, KeySource{}, NodeId{0}};
    Attempt b{tx, START, Timing{}, rng_b, KeySource{}, NodeId{0}};
    a.Connected(START);
    b.Connected(START);
    BOOST_REQUIRE(a.BytesToSend().size() >= 64 && b.BytesToSend().size() >= 64);
    BOOST_CHECK(!std::ranges::equal(a.BytesToSend().first(64), b.BytesToSend().first(64)));
}

BOOST_AUTO_TEST_CASE(nonces_per_attempt)
{
    // A4: two attempts of one job send different VERSION nonces and different PING nonces. A
    // VERSION of the profile differs only by its nonce, and a PING is its nonce.
    FastRandomContext rng{uint256{3}};
    std::vector<std::vector<uint8_t>> versions, pings;
    for (int i{0}; i < 2; ++i) {
        Attempt attempt{tx, START, Timing{}, rng, KeysFrom(rng), NodeId{0}};
        Peer peer{rng};
        Announce(attempt, peer, START);
        peer.Send(GetData({CInv{MSG_WTX, wtxid}}));
        peer.Exchange(attempt, START + 1s);
        BOOST_REQUIRE(peer.ReceivedTypes() == SERVED);
        versions.push_back(peer.Received().front().payload);
        pings.push_back(peer.Received().back().payload);
    }
    BOOST_CHECK(versions[0] != versions[1]);
    BOOST_CHECK(pings[0] != pings[1]);
}

BOOST_AUTO_TEST_CASE(package_parent_before_child)
{
    // F2 (b), F4: before the child is served, a request for the parent that names no child id
    // serves it once, with NOTFOUND for unknown entries, then the PING; after it, never the child.
    FastRandomContext rng{uint256{32}};
    const uint256 unknown{0xab};
    {
        Attempt attempt{child, START, Timing{}, rng, KeysFrom(rng), NodeId{0}, parent};
        Peer peer{rng};
        Announce(attempt, peer, START);
        // Ignored entirely, without NOTFOUND.
        const std::vector<std::vector<CInv>> ignored{
            {CInv{MSG_WTX, child_wtxid}, CInv{MSG_WITNESS_TX, parent_txid}},
            {CInv{MSG_WITNESS_TX, parent_txid}, CInv{MSG_WITNESS_TX, child_txid}},
            {CInv{MSG_WITNESS_TX, parent_txid}, CInv{MSG_TX, child_wtxid}},
            {CInv{MSG_WITNESS_TX, parent_txid}, CInv{MSG_BLOCK, child_txid}},
            {CInv{MSG_WITNESS_TX, child_txid}},
            {CInv{MSG_TX, parent_txid}},
            {CInv{MSG_WTX, parent_wtxid}},
            {CInv{MSG_WITNESS_TX, parent_wtxid}},
            {CInv{MSG_WITNESS_TX, unknown}, CInv{MSG_TX, unknown}},
        };
        int extra{0};
        for (const std::vector<CInv>& request : ignored) {
            peer.Send(GetData(request));
            peer.Exchange(attempt, START + 1s);
            BOOST_CHECK_EQUAL(attempt.ExtraRequests(), ++extra);
            BOOST_CHECK(peer.ReceivedTypes() == ANNOUNCED);
        }
        BOOST_CHECK(!attempt.Times().getdata);
        BOOST_CHECK(!attempt.Times().parent_getdata);
        peer.Send(GetData({CInv{MSG_WITNESS_TX, parent_txid}, CInv{MSG_WITNESS_TX, unknown}, CInv{MSG_WITNESS_TX, parent_txid},
                           CInv{MSG_TX, parent_txid}}));
        peer.Exchange(attempt, START + 2s);
        // The parent, then the PING (F2); F4 does not place the NOTFOUND.
        const Types served{peer.ReceivedTypes()};
        Types types{served};
        const auto notfound{std::ranges::find(types, std::string{NetMsgType::NOTFOUND})};
        BOOST_REQUIRE(notfound != types.end());
        BOOST_CHECK(NotFound(peer.Received()[notfound - types.begin()]) == (Entries{{MSG_WITNESS_TX, unknown}}));
        types.erase(notfound);
        BOOST_REQUIRE(types == Then(ANNOUNCED, {NetMsgType::TX, NetMsgType::PING}));
        BOOST_CHECK(Served(*std::ranges::find(peer.Received(), std::string{NetMsgType::TX}, &Message::type)) == parent->GetWitnessHash());
        const uint64_t nonce{Nonce(std::ranges::find(peer.Received(), std::string{NetMsgType::PING}, &Message::type)->payload)};
        BOOST_CHECK(attempt.Times().parent_getdata == START + 2s);
        BOOST_CHECK(attempt.Times().parent_tx_written == START + 2s);
        BOOST_CHECK(attempt.Times().ping_written == START + 2s);
        BOOST_CHECK_EQUAL(attempt.ExtraRequests(), extra);
        // The child is never served on this connection.
        peer.Send(GetData({CInv{MSG_WTX, child_wtxid}}));
        peer.Exchange(attempt, START + 3s);
        BOOST_CHECK_EQUAL(attempt.ExtraRequests(), extra + 1);
        BOOST_CHECK(peer.ReceivedTypes() == served);
        peer.Send(Pong(nonce));
        peer.Exchange(attempt, START + 4s);
        BOOST_CHECK(attempt.GetOutcome() == Outcome::PongReceived);
        BOOST_CHECK(!attempt.Times().getdata);
        BOOST_CHECK(!attempt.Times().tx_written);
        BOOST_CHECK(!attempt.Times().parent_hold_expired);
    }
    {
        // After the request that named both, the child alone, then the parent: served in turn.
        Attempt attempt{child, START, Timing{}, rng, KeysFrom(rng), NodeId{0}, parent};
        Peer peer{rng};
        Announce(attempt, peer, START);
        peer.Send(GetData({CInv{MSG_WTX, child_wtxid}, CInv{MSG_WITNESS_TX, parent_txid}}));
        peer.Send(GetData({CInv{MSG_WTX, child_wtxid}}));
        peer.Exchange(attempt, START + 1s);
        BOOST_CHECK_EQUAL(attempt.ExtraRequests(), 1);
        BOOST_REQUIRE(peer.ReceivedTypes() == Then(ANNOUNCED, {NetMsgType::TX}));
        BOOST_CHECK(Served(peer.Received()[4]) == child->GetWitnessHash());
        peer.Send(GetData({CInv{MSG_WITNESS_TX, parent_txid}}));
        peer.Exchange(attempt, START + 2s);
        BOOST_REQUIRE(peer.ReceivedTypes() == Then(ANNOUNCED, {NetMsgType::TX, NetMsgType::TX, NetMsgType::PING}));
        BOOST_CHECK(Served(peer.Received()[5]) == parent->GetWitnessHash());
        BOOST_CHECK_EQUAL(attempt.ExtraRequests(), 1);
    }
}

BOOST_AUTO_TEST_CASE(package_hold)
{
    // F2, F3: a parent request during the hold is served; once the hold has run out, one is not.
    FastRandomContext rng{uint256{33}};
    for (const bool in_time : {true, false}) {
        Attempt attempt{child, START, Timing{}, rng, KeysFrom(rng), NodeId{0}, parent};
        Peer peer{rng};
        Announce(attempt, peer, START);
        peer.Send(GetData({CInv{MSG_WTX, child_wtxid}}));
        peer.Exchange(attempt, START + 1ms);
        const milliseconds hold_end{START + 1ms + PARENT_HOLD};
        BOOST_REQUIRE(attempt.NextDeadline() == hold_end);
        const milliseconds at{in_time ? hold_end - 10s : hold_end + 1s};
        peer.Send(GetData({CInv{MSG_WITNESS_TX, parent_txid}}));
        peer.Exchange(attempt, at);
        BOOST_CHECK(attempt.Times().ping_written == at);
        if (in_time) {
            BOOST_CHECK(peer.ReceivedTypes() == Then(ANNOUNCED, {NetMsgType::TX, NetMsgType::TX, NetMsgType::PING}));
            BOOST_CHECK(attempt.Times().parent_getdata == at);
            BOOST_CHECK(!attempt.Times().parent_hold_expired);
            BOOST_CHECK_EQUAL(attempt.ExtraRequests(), 0);
        } else {
            BOOST_CHECK(peer.ReceivedTypes() == Then(ANNOUNCED, {NetMsgType::TX, NetMsgType::PING}));
            BOOST_CHECK(!attempt.Times().parent_getdata);
            BOOST_CHECK(attempt.Times().parent_hold_expired);
            BOOST_CHECK_EQUAL(attempt.ExtraRequests(), 1);
        }
        BOOST_CHECK(attempt.NextDeadline() == at + PONG_WAIT);
    }
}

BOOST_AUTO_TEST_CASE(package_request_at_cut)
{
    // F3, D2: a hold never runs into the request window's last PONG_WAIT. A child served before it is
    // held only until it; after a child request inside it the PING follows the TX at once, and a
    // parent request after it is not served, in the same read or the next.
    FastRandomContext rng{uint256{37}};
    const milliseconds cut{START + REQUEST_WINDOW - PONG_WAIT};
    {
        Attempt attempt{child, START, Timing{}, rng, KeysFrom(rng), NodeId{0}, parent};
        Peer peer{rng};
        Announce(attempt, peer, START);
        peer.Send(GetData({CInv{MSG_WTX, child_wtxid}}));
        peer.Exchange(attempt, cut - 10s);
        BOOST_REQUIRE(peer.ReceivedTypes() == Then(ANNOUNCED, {NetMsgType::TX}));
        attempt.Tick(cut - 1s);
        peer.Exchange(attempt, cut - 1s);
        BOOST_CHECK(peer.ReceivedTypes() == Then(ANNOUNCED, {NetMsgType::TX}));
        attempt.Tick(cut + 1s);
        peer.Exchange(attempt, cut + 1s);
        BOOST_CHECK(peer.ReceivedTypes() == Then(ANNOUNCED, {NetMsgType::TX, NetMsgType::PING}));
        BOOST_CHECK(attempt.Times().parent_hold_expired == cut + 1s);
    }
    const milliseconds at{cut + PONG_WAIT / 2};
    for (const bool one_read : {true, false}) {
        Attempt attempt{child, START, Timing{}, rng, KeysFrom(rng), NodeId{0}, parent};
        Peer peer{rng};
        Announce(attempt, peer, START);
        peer.Send(GetData({CInv{MSG_WTX, child_wtxid}}));
        peer.Send(GetData({CInv{MSG_WITNESS_TX, parent_txid}}));
        if (one_read) {
            peer.DeliverInOneRead(attempt, at);
        } else {
            peer.Deliver(attempt, at);
        }
        BOOST_CHECK(attempt.Times().getdata == at);
        BOOST_CHECK(!attempt.Times().parent_getdata);
        BOOST_CHECK_EQUAL(attempt.ExtraRequests(), 1);
        peer.Exchange(attempt, at);
        BOOST_REQUIRE(peer.ReceivedTypes() == Then(ANNOUNCED, {NetMsgType::TX, NetMsgType::PING}));
        BOOST_CHECK(Served(peer.Received()[4]) == child->GetWitnessHash());
        BOOST_CHECK(attempt.Times().tx_written == at);
        BOOST_CHECK(attempt.Times().ping_written == at);
        BOOST_CHECK(!attempt.Times().parent_tx_written);
        BOOST_CHECK(!attempt.Times().parent_hold_expired);
        BOOST_CHECK(attempt.NextDeadline() == at + PONG_WAIT);
        peer.Send(Pong(Nonce(peer.Received().back().payload)));
        peer.Exchange(attempt, at + 1s);
        BOOST_CHECK(attempt.GetOutcome() == Outcome::PongReceived);
    }
}

BOOST_AUTO_TEST_SUITE_END()
