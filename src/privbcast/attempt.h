// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_PRIVBCAST_ATTEMPT_H
#define BITCOIN_PRIVBCAST_ATTEMPT_H

#include <key.h>
#include <net_transport.h>
#include <primitives/transaction.h>
#include <privbcast/params.h>
#include <uint256.h>

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <functional>
#include <optional>
#include <span>
#include <string>
#include <vector>

class DataStream;
class FastRandomContext;

namespace privbcast {

/** How an attempt ended (Report: outcome). */
enum class Outcome {
    /** Ended before the announcement point. */
    NotAnnounced,
    AnnouncedNotRequested,
    TxWrittenNoPong,
    PongReceived,
    /** The connection failed after the announcement point. */
    PostAnnouncementFailure,
};

/** An attempt's BIP324 key, the entropy of its ElligatorSwift encoding, and the garbage. */
struct TransportKeys {
    CKey key;
    uint256 ellswift_entropy;
    std::vector<uint8_t> garbage;
};

/** Where an attempt's TransportKeys come from. When it is empty, as in production, V2Transport
 *  draws them from the strong RNG. Tests inject one that draws from a seeded generator, so that an
 *  attempt's bytes are reproducible. */
using KeySource = std::function<TransportKeys()>;

/** When an attempt's events happened, as offsets from job start on the plan clock. Empty for an
 *  event that did not happen. */
struct AttemptTimes {
    /** The proxy connected the stream. */
    std::optional<std::chrono::milliseconds> connected;
    /** The transport accepted the INV: the announcement point. */
    std::optional<std::chrono::milliseconds> inv_handed;
    /** The last byte of the INV was written. */
    std::optional<std::chrono::milliseconds> inv_written;
    /** The GETDATA that was served arrived. */
    std::optional<std::chrono::milliseconds> getdata;
    /** The last byte of the TX was written. */
    std::optional<std::chrono::milliseconds> tx_written;
    /** The last byte of the PING was written. */
    std::optional<std::chrono::milliseconds> ping_written;
    /** The PONG carrying the PING's nonce arrived. */
    std::optional<std::chrono::milliseconds> pong;
    /** The attempt ended. */
    std::optional<std::chrono::milliseconds> ended;

    bool operator==(const AttemptTimes&) const = default;
};

/**
 * One connection to a recipient, from the moment the proxy connected it: a BIP324 session as
 * initiator, the handshake, the announcement, serving the transaction and the PING (E1-E8). It owns
 * no socket and reads no clock: the caller passes the bytes read, writes what BytesToSend() offers,
 * and passes the time, an offset from job start, with every call.
 *
 * Every byte it offers is a function of the transaction, the randomness drawn at construction,
 * what the peer sent, when it arrived against the attempt's deadlines, and Interrupt (A2).
 * Nothing is acted on at or after a deadline (D2): each call records what its I/O did, then ends
 * the attempt if a deadline has passed. Once ended, it offers no bytes, not even the rest of a
 * partly written message (E7).
 */
class Attempt
{
public:
    /**
     * @param[in] tx               The transaction to announce and serve.
     * @param[in] scheduled_start  The opportunity's scheduled start. The deadlines count from it.
     * @param[in] timing           Scales the deadlines.
     * @param[in] rng              The VERSION and PING nonces are drawn from it here (A4).
     * @param[in] keys             The BIP324 key, entropy and garbage are taken from it here (A4).
     * @param[in] id               Names the connection in the transport's debug log.
     */
    Attempt(CTransactionRef tx, std::chrono::milliseconds scheduled_start, const Timing& timing,
            FastRandomContext& rng, const KeySource& keys, NodeId id);

    /** The proxy connected the stream, and `first`, the peer's bytes that came with its reply,
     *  were read. The VERSION is queued (E1), then `first` is handled as Received() handles bytes. */
    void Connected(std::chrono::milliseconds now, std::span<const uint8_t> first = {});
    /** Bytes read from the connection. They count toward the receive cap (D3), at a deadline
     *  too; those within it feed the transport, and the messages they complete are handled. */
    void Received(std::span<const uint8_t> bytes, std::chrono::milliseconds now);
    /** What to write next. Empty before Connected and once ended. */
    std::span<const uint8_t> BytesToSend() const;
    /** The first n bytes of BytesToSend() were written. When the last byte of the INV, the TX or the
     *  PING goes, its time is recorded, at a deadline too (C6, Interface/Report); then, unless a
     *  deadline has passed, the next message is handed to the transport. */
    void MarkSent(size_t n, std::chrono::milliseconds now);
    /** End the attempt if a deadline has passed. The caller runs it before reading the connection
     *  and waits until NextDeadline(). */
    void Tick(std::chrono::milliseconds now);
    /** The connection failed: the peer closed it, the socket failed, or acting on the attempt threw
     *  (C7). End as a transport error does, with `reason`. */
    void ConnectionFailed(std::chrono::milliseconds now, std::string reason);
    /** The job was cancelled (C8) or failed: end now, whatever the state, with `reason`. */
    void Interrupt(std::chrono::milliseconds now, std::string reason);

    bool Ended() const { return m_outcome.has_value(); }
    /** How the attempt ended. Before it has ended: how an interruption would end it now. */
    Outcome GetOutcome() const { return m_outcome.value_or(StateOutcome()); }
    /** Why it ended, for people. Empty while it runs. */
    const std::string& Reason() const { return m_reason; }
    const AttemptTimes& Times() const { return m_times; }
    /** From the peer's VERSION, as sent. */
    std::optional<int> PeerVersion() const { return m_peer_version; }
    std::optional<std::string> PeerUserAgent() const { return m_peer_user_agent; }
    /** GETDATAs received after the announcement point that were not served. */
    int ExtraRequests() const { return m_extra_requests; }
    /** Everything written to and read from the connection. */
    uint64_t BytesSent() const { return m_bytes_sent; }
    uint64_t BytesRecv() const { return m_bytes_recv; }
    /** The next deadline, or empty once ended. */
    std::optional<std::chrono::milliseconds> NextDeadline() const;

private:
    const CTransactionRef m_tx;
    const std::chrono::milliseconds m_scheduled_start;
    const Timing m_timing;
    /** BIP324 as initiator, never v1 (B4). Constructed first, so that a key source drawing from rng
     *  draws before the nonces. */
    V2Transport m_transport;
    const uint64_t m_version_nonce;
    const uint64_t m_ping_nonce;

    /** Messages not yet handed to the transport, which takes one at a time. */
    std::deque<CSerializedNetMsg> m_queue;
    /** The type of the message the transport holds, until its last byte is written. */
    std::string m_in_transport;

    std::optional<int> m_peer_version;
    std::optional<std::string> m_peer_user_agent;
    bool m_peer_wtxidrelay{false};
    bool m_peer_verack{false};

    AttemptTimes m_times;
    std::optional<Outcome> m_outcome;
    std::string m_reason;
    int m_extra_requests{0};
    uint64_t m_bytes_sent{0};
    uint64_t m_bytes_recv{0};

    /** What the attempt has achieved: the outcome of a deadline or an interruption now. A request
     *  whose TX is not fully written is a failure after the announcement point. */
    Outcome StateOutcome() const;
    void End(std::chrono::milliseconds now, Outcome outcome, std::string reason);
    /** End on a failure of the connection or of the peer: a close, a socket or transport error,
     *  the receive cap, or a malformed message the attempt would act on. */
    void Fail(std::chrono::milliseconds now, std::string reason);
    /** Count bytes read (D3). Returns those within the receive cap. */
    std::span<const uint8_t> CountReceived(std::span<const uint8_t> bytes);
    /** Feed counted bytes to the transport and handle the messages they complete; then end the
     *  attempt if the receive cap was passed. */
    void Feed(std::span<const uint8_t> bytes, std::chrono::milliseconds now);
    void Queue(CSerializedNetMsg msg, std::chrono::milliseconds now);
    /** Hand the next queued message to the transport if it takes one. */
    void HandOff(std::chrono::milliseconds now);
    void Process(const std::string& type, DataStream& payload, std::chrono::milliseconds now);
    void ProcessVersion(DataStream& payload, std::chrono::milliseconds now);
    void ProcessGetData(DataStream& payload, std::chrono::milliseconds now);
    void ProcessPong(DataStream& payload, std::chrono::milliseconds now);
};

} // namespace privbcast

#endif // BITCOIN_PRIVBCAST_ATTEMPT_H
