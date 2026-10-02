// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <privbcast/attempt.h>

#include <key.h>
#include <net_transport.h>
#include <netaddress.h>
#include <netmessagemaker.h>
#include <primitives/transaction.h>
#include <privbcast/params.h>
#include <protocol.h>
#include <random.h>
#include <serialize.h>
#include <span.h>
#include <streams.h>
#include <tinyformat.h>
#include <uint256.h>
#include <util/check.h>
#include <util/time.h>

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <ios>
#include <optional>
#include <span>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace privbcast {
namespace {

/** The longest user agent a VERSION may carry (E2). */
constexpr size_t MAX_USER_AGENT_LENGTH{256};
/** addr_recv or addr_from in a VERSION: services, address and port. */
constexpr size_t VERSION_ADDRESS_SIZE{8 + 16 + 2};

V2Transport MakeTransport(NodeId id, const KeySource& keys)
{
    if (!keys) return V2Transport{id, /*initiating=*/true};
    TransportKeys drawn{keys()};
    return V2Transport{id, /*initiating=*/true, drawn.key, MakeByteSpan(drawn.ellswift_entropy), std::move(drawn.garbage)};
}

/** Whether the entry names tx: its txid or its wtxid, whatever the entry's type. */
bool Names(const CInv& inv, const CTransaction& tx)
{
    return inv.hash == tx.GetHash().ToUint256() || inv.hash == tx.GetWitnessHash().ToUint256();
}

/** Whether the entry asks for the parent as F2 serves it: MSG_WITNESS_TX with its txid. */
bool AsksForParent(const CInv& inv, const CTransaction& parent)
{
    return inv.type == MSG_WITNESS_TX && inv.hash == parent.GetHash().ToUint256();
}

} // namespace

Attempt::Attempt(CTransactionRef tx, std::chrono::milliseconds scheduled_start, const Timing& timing,
                 FastRandomContext& rng, const KeySource& keys, NodeId id, CTransactionRef parent)
    : m_tx{std::move(tx)},
      m_parent{std::move(parent)},
      m_scheduled_start{scheduled_start},
      m_timing{timing},
      m_transport(MakeTransport(id, keys)), // V2Transport cannot be moved: initialized from the prvalue
      m_version_nonce{rng.rand64()},
      m_ping_nonce{rng.rand64()}
{
}

void Attempt::Connected(std::chrono::milliseconds now, std::span<const uint8_t> first)
{
    if (Ended() || m_times.connected) return;
    // Recorded at a deadline too: the connection, and the bytes read with it (D3).
    m_times.connected = now;
    const std::span<const uint8_t> within{CountReceived(first)};
    Tick(now);
    if (Ended()) return;
    // The profile (E1).
    Queue(NetMsg::Make(NetMsgType::VERSION,
                       PROFILE_VERSION,
                       uint64_t{PROFILE_SERVICES},
                       int64_t{0}, // time
                       uint64_t{NODE_NONE}, CNetAddr::V1(CService{}),
                       uint64_t{PROFILE_SERVICES}, CNetAddr::V1(CService{}),
                       m_version_nonce,
                       std::string{PROFILE_USER_AGENT},
                       int32_t{0}, // start height
                       /*relay=*/false),
          now);
    Feed(within, now);
}

void Attempt::Received(std::span<const uint8_t> bytes, std::chrono::milliseconds now)
{
    if (Ended() || bytes.empty() || !Assume(m_times.connected.has_value())) return;
    // Bytes read count at a deadline too (D3); the deadline decides only whether they are
    // processed (D2).
    const std::span<const uint8_t> within{CountReceived(bytes)};
    Tick(now);
    Feed(within, now);
}

std::span<const uint8_t> Attempt::CountReceived(std::span<const uint8_t> bytes)
{
    const uint64_t room{m_bytes_recv < MAX_RECV_BYTES ? MAX_RECV_BYTES - m_bytes_recv : 0};
    m_bytes_recv += bytes.size();
    return bytes.first(std::min<uint64_t>(bytes.size(), room));
}

void Attempt::Feed(std::span<const uint8_t> bytes, std::chrono::milliseconds now)
{
    while (!bytes.empty() && !Ended()) {
        if (!m_transport.ReceivedBytes(bytes)) return Fail(now, "transport error");
        if (!m_transport.ReceivedMessageComplete()) continue;
        bool reject{false};
        CNetMessage msg{m_transport.GetReceivedMessage(NodeClock::time_point{}, reject)};
        // A message of a type the transport cannot decode is dropped.
        if (!reject) Process(msg.m_type, msg.m_recv, now);
    }
    if (m_bytes_recv > MAX_RECV_BYTES) Fail(now, "receive cap");
}

std::span<const uint8_t> Attempt::BytesToSend() const
{
    if (Ended() || !m_times.connected) return {};
    return std::get<0>(m_transport.GetBytesToSend(/*have_next_message=*/false));
}

void Attempt::MarkSent(size_t n, std::chrono::milliseconds now)
{
    const size_t offered{BytesToSend().size()};
    if (!Assume(n <= offered)) n = offered;
    if (n == 0) return;
    m_transport.MarkBytesSent(n);
    m_bytes_sent += n;
    if (!BytesToSend().empty()) return Tick(now);
    // The transport has written all it held. A message whose last byte went is written, at a
    // deadline too (C6, Interface/Report).
    if (const Event written{m_in_transport.value_or(nullptr)}) m_times.*written = now;
    m_in_transport.reset();
    // Then the deadlines: once one has passed, nothing more goes to the transport (D2).
    Tick(now);
    HandOff(now);
}

void Attempt::Tick(std::chrono::milliseconds now)
{
    if (Ended()) return;
    const std::chrono::milliseconds deadline{EndDeadline()};
    if (now < deadline) return CheckParentHold(now);
    std::string reason{"no PONG in time"};
    if (!m_times.inv_handed) {
        reason = "not announced by the handshake deadline";
    } else if (!m_times.getdata && !m_times.parent_getdata) {
        reason = "no request in the request window";
    } else if (m_times.getdata && !m_times.tx_written) {
        reason = "TX not written in the request window";
    } else if (m_times.parent_getdata && !m_times.parent_tx_written) {
        reason = "the parent's TX not written in the request window";
    } else if (!m_times.ping_written || *m_times.ping_written >= deadline) {
        reason = "PING not written in the request window";
    }
    End(now, StateOutcome(), std::move(reason));
}

void Attempt::ConnectionFailed(std::chrono::milliseconds now, std::string reason)
{
    Tick(now);
    Fail(now, std::move(reason));
}

void Attempt::Interrupt(std::chrono::milliseconds now, std::string reason)
{
    Tick(now);
    End(now, StateOutcome(), std::move(reason));
}

std::optional<std::chrono::milliseconds> Attempt::NextDeadline() const
{
    if (Ended()) return std::nullopt;
    // The hold ends before the request window's last PONG_WAIT, so before the attempt can.
    return WaitingForParent() ? ParentDeadline() : EndDeadline();
}

std::chrono::milliseconds Attempt::EndDeadline() const
{
    // The handshake budget, then the request window, by whose end the TX and the PING are written
    // (E6), then the PONG wait. A PING written at the window's end opens no PONG wait.
    std::chrono::milliseconds phase_end{m_scheduled_start + m_timing.Scale(HANDSHAKE_BUDGET)};
    if (m_times.inv_handed) phase_end = *m_times.inv_handed + m_timing.Scale(REQUEST_WINDOW);
    if (m_times.ping_written && *m_times.ping_written < phase_end) {
        phase_end = *m_times.ping_written + m_timing.Scale(PONG_WAIT);
    }
    return std::min(phase_end, m_scheduled_start + m_timing.Scale(ATTEMPT_MAX));
}

std::chrono::milliseconds Attempt::ParentDeadline() const
{
    // The PING goes out by the start of the request window's last PONG_WAIT, so that the PONG wait
    // fits in the window. Until the child's TX is written, that is the only bound.
    const std::chrono::milliseconds last{*Assert(m_times.inv_handed) + m_timing.Scale(REQUEST_WINDOW) - m_timing.Scale(PONG_WAIT)};
    if (!m_times.tx_written) return last;
    return std::min(*m_times.tx_written + m_timing.Scale(PARENT_HOLD), last);
}

void Attempt::CheckParentHold(std::chrono::milliseconds now)
{
    // The hold begins with the last byte of the child's TX: when the window's last PONG_WAIT
    // begins before that, no hold has expired, and the PING just follows the TX.
    if (!WaitingForParent()) return;
    const std::chrono::milliseconds end{ParentDeadline()};
    if (now < end) return;
    if (m_times.tx_written && *m_times.tx_written < end) m_times.parent_hold_expired = now;
    QueuePing(now);
}

Outcome Attempt::StateOutcome() const
{
    if (!m_times.inv_handed) return Outcome::NotAnnounced;
    if (!m_times.getdata) return Outcome::AnnouncedNotRequested;
    if (!m_times.tx_written) return Outcome::PostAnnouncementFailure;
    return Outcome::TxWrittenNoPong;
}

void Attempt::End(std::chrono::milliseconds now, Outcome outcome, std::string reason)
{
    if (Ended()) return;
    m_outcome = outcome;
    m_reason = std::move(reason);
    m_times.ended = now;
    m_queue.clear();
}

void Attempt::Fail(std::chrono::milliseconds now, std::string reason)
{
    End(now, m_times.inv_handed ? Outcome::PostAnnouncementFailure : Outcome::NotAnnounced, std::move(reason));
}

void Attempt::Queue(CSerializedNetMsg msg, std::chrono::milliseconds now, Event written)
{
    m_queue.push_back({std::move(msg), written});
    HandOff(now);
}

void Attempt::QueuePing(std::chrono::milliseconds now)
{
    m_ping_queued = true;
    Queue(NetMsg::Make(NetMsgType::PING, m_ping_nonce), now, &AttemptTimes::ping_written);
}

void Attempt::HandOff(std::chrono::milliseconds now)
{
    if (Ended() || m_in_transport || m_queue.empty()) return;
    const bool inv{m_queue.front().msg.m_type == NetMsgType::INV};
    if (!m_transport.SetMessageToSend(m_queue.front().msg)) return;
    m_in_transport = m_queue.front().written;
    m_queue.pop_front();
    // The announcement point: the request window starts.
    if (inv) m_times.inv_handed = now;
}

void Attempt::Process(const std::string& type, DataStream& payload, std::chrono::milliseconds now)
{
    // A message the attempt does not act on in its state is ignored (E7).
    if (!m_peer_version) {
        if (type == NetMsgType::VERSION) ProcessVersion(payload, now);
    } else if (!m_peer_verack) {
        // WTXIDRELAY counts only between the peer's VERSION and its VERACK (BIP339).
        if (type == NetMsgType::WTXIDRELAY) {
            m_peer_wtxidrelay = true;
        } else if (type == NetMsgType::VERACK) {
            m_peer_verack = true;
            if (!m_peer_wtxidrelay) return End(now, Outcome::NotAnnounced, "no WTXIDRELAY before the peer's VERACK");
            // One INV with one entry, MSG_WTX for the wtxid (E3); in package mode the child's (F1).
            Queue(NetMsg::Make(NetMsgType::INV, std::vector<CInv>{CInv{MSG_WTX, m_tx->GetWitnessHash().ToUint256()}}), now,
                  &AttemptTimes::inv_written);
        }
    } else if (m_times.inv_handed) {
        // Before the announcement point, a GETDATA is ignored and not counted (E4). Once the PING
        // is queued, another is counted without being parsed: nothing more is served (E6, F2).
        if (type == NetMsgType::GETDATA && m_ping_queued) {
            ++m_extra_requests;
        } else if (type == NetMsgType::GETDATA) {
            ProcessGetData(payload, now);
        } else if (type == NetMsgType::PONG && m_times.ping_written) {
            ProcessPong(payload, now);
        }
    }
}

void Attempt::ProcessVersion(DataStream& payload, std::chrono::milliseconds now)
{
    int32_t version{0};
    uint64_t services{0};
    std::string user_agent;
    bool relay{false};
    try {
        int64_t time{0};
        uint64_t nonce{0};
        int32_t start_height{0};
        payload >> version >> services >> time;
        payload.ignore(2 * VERSION_ADDRESS_SIZE);
        payload >> nonce >> LIMITED_STRING(user_agent, MAX_USER_AGENT_LENGTH) >> start_height >> relay;
    } catch (const std::ios_base::failure&) {
        return Fail(now, "malformed VERSION");
    }
    m_peer_version = version;
    m_peer_user_agent = user_agent;
    // E2: after any other VERSION, nothing more is sent.
    if (version < MIN_PEER_PROTOCOL_VERSION) {
        return End(now, Outcome::NotAnnounced, strprintf("peer version %d is below %d", version, MIN_PEER_PROTOCOL_VERSION));
    }
    if (!(services & NODE_WITNESS)) return End(now, Outcome::NotAnnounced, "peer does not offer NODE_WITNESS");
    if (!relay) return End(now, Outcome::NotAnnounced, "peer does not relay transactions");
    Queue(NetMsg::Make(NetMsgType::WTXIDRELAY), now);
    Queue(NetMsg::Make(NetMsgType::VERACK), now);
}

void Attempt::ProcessGetData(DataStream& payload, std::chrono::milliseconds now)
{
    std::vector<CInv> request;
    try {
        payload >> request;
    } catch (const std::ios_base::failure&) {
        // It gets no reply, and ends the attempt (E7).
        ++m_extra_requests;
        return Fail(now, "malformed GETDATA");
    }
    // Package mode, once the child was served: the parent, NOTFOUND or nothing (F2 (a), F4).
    if (WaitingForParent()) {
        if (!AnswerForParent(request, now)) ++m_extra_requests;
        return;
    }
    // Served once, only for a single-entry MSG_WTX naming the wtxid (E5); TX, then a PING (E6),
    // which in package mode follows the parent phase (F3).
    if (request.size() == 1 && request[0].IsMsgWtx() && request[0].hash == m_tx->GetWitnessHash().ToUint256()) {
        m_times.getdata = now;
        Queue(NetMsg::Make(NetMsgType::TX, TX_WITH_WITNESS(*m_tx)), now, &AttemptTimes::tx_written);
        if (!m_parent) {
            QueuePing(now);
        } else {
            // The parent phase opens, its deadline in force at once: at or after it, the PING
            // follows the TX, and a parent request in the same read is not served (F3).
            CheckParentHold(now);
        }
        return;
    }
    // Before the child, the parent is served only for a request that names neither of the
    // child's ids (F2 (b)). Any other request is ignored entirely.
    if (m_parent && std::ranges::any_of(request, [&](const CInv& inv) { return AsksForParent(inv, *m_parent); }) &&
        std::ranges::none_of(request, [&](const CInv& inv) { return Names(inv, *m_tx); })) {
        AnswerForParent(request, now);
        return;
    }
    ++m_extra_requests;
}

bool Attempt::AnswerForParent(const std::vector<CInv>& request, std::chrono::milliseconds now)
{
    bool parent{false};
    std::vector<CInv> notfound;
    for (const CInv& inv : request) {
        if (!parent && AsksForParent(inv, *m_parent)) {
            parent = true;
        } else if (inv.IsGenTxMsg() && !Names(inv, *m_tx) && !Names(inv, *m_parent)) {
            // A transaction the job does not have (F4).
            notfound.push_back(inv);
        }
    }
    if (parent) {
        m_times.parent_getdata = now;
        Queue(NetMsg::Make(NetMsgType::TX, TX_WITH_WITNESS(*m_parent)), now, &AttemptTimes::parent_tx_written);
    }
    if (!notfound.empty()) Queue(NetMsg::Make(NetMsgType::NOTFOUND, notfound), now);
    // After the parent, the PING: nothing more is served (F2).
    if (parent) QueuePing(now);
    return parent || !notfound.empty();
}

void Attempt::ProcessPong(DataStream& payload, std::chrono::milliseconds now)
{
    uint64_t nonce{0};
    try {
        payload >> nonce;
    } catch (const std::ios_base::failure&) {
        return Fail(now, "malformed PONG");
    }
    // A PONG with another nonce changes nothing (E6).
    if (nonce != m_ping_nonce) return;
    m_times.pong = now;
    End(now, Outcome::PongReceived, "PONG received");
}

} // namespace privbcast
