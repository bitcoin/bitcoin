// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_PRIVBCAST_STREAM_H
#define BITCOIN_PRIVBCAST_STREAM_H

#include <netbase.h>
#include <privbcast/params.h>
#include <privbcast/socks5.h>
#include <util/sock.h>
#include <util/time.h>

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace privbcast {

/** A reading of the I/O clock, which times the connect to the proxy and the SOCKS5 exchange. */
using SteadyMs = std::chrono::time_point<MockableSteadyClock, std::chrono::milliseconds>;

/**
 * One stream through the SOCKS5 proxy, which the job's event loop drives without blocking: a
 * non-blocking connect to the proxy, over TCP or a unix socket, then the exchange of its
 * Socks5Client. Once the proxy has answered the request the stream is Open, and its socket
 * carries the destination's bytes, starting with any that arrived with the reply
 * (TakeLeftover()).
 *
 * The proxy is an address or a path, so nothing is resolved (B1). Each stage, the connect and
 * each step of the exchange, has SOCKS_STAGE_TIMEOUT on the I/O clock; a stage that runs out
 * closes the stream.
 */
class ProxyStream
{
public:
    enum class Phase { Connecting, Socks, Open, Closed };

    /** @param[in] now  The I/O clock: the connect, the first stage, starts now. */
    ProxyStream(const Proxy& proxy, Socks5Client socks, const Timing& timing, SteadyMs now);

    /** Create the socket through CreateSock and start the connect. False if the stream could not
     *  start; it is then Closed, and Reason() says why. A connect that completed at once has
     *  started the exchange too, which may have closed the stream already (GetPhase()). */
    bool Open();
    /** What to wait for: the connect's end, then the proxy's replies and room to write the
     *  exchange; once Open, the destination's bytes. */
    Sock::Event WantedEvents() const;
    /** Empty once Closed. */
    const std::shared_ptr<Sock>& GetSock() const { return m_sock; }
    /** Drive the connect and the exchange with the events that occurred, if any: check the stage's
     *  time first, then complete the connect, read the proxy's reply and write what the exchange
     *  has to send. Spurious events change nothing. */
    void OnEvents(Sock::Event occurred, SteadyMs now);
    /** When the current stage runs out. Empty once Open or Closed. */
    std::optional<SteadyMs> NextDeadline() const;

    Phase GetPhase() const { return m_phase; }
    /** Why the stream failed, for people. Empty if it did not. */
    const std::string& Reason() const { return m_reason; }
    /** The bytes that arrived after the proxy's final reply: the destination's first bytes. */
    std::vector<uint8_t> TakeLeftover();
    /** The SOCKS5 exchange: for a RESOLVE, its answer. */
    const Socks5Client& Socks() const { return m_socks; }

    /** Open: write the start of bytes. Returns how many the socket took, 0 if it takes none now.
     *  A socket error closes the stream. */
    size_t Send(std::span<const uint8_t> bytes);
    /** Open: read what has arrived into buf. Returns how many bytes, 0 if none has. The peer
     *  closing the stream, or a socket error, closes it. */
    size_t Recv(std::span<uint8_t> buf);
    /** The stream closed because the destination closed it once Open. */
    bool PeerClosed() const { return m_peer_closed; }

    /** End the stream without a failure: Closed, with no reason. */
    void Close();

private:
    Proxy m_proxy;
    Socks5Client m_socks;
    std::chrono::milliseconds m_stage_timeout;
    /** When the current stage began. */
    SteadyMs m_stage_start;

    Phase m_phase{Phase::Connecting};
    std::shared_ptr<Sock> m_sock;
    std::vector<uint8_t> m_leftover;
    std::string m_reason;
    bool m_peer_closed{false};

    /** The connect is done: start the exchange by writing the greeting. */
    void StartExchange(SteadyMs now);
    /** Read and process the proxy's reply, as much as has arrived. */
    void ReadReply(SteadyMs now);
    /** Write what the exchange has to send, as much as the socket takes. */
    void WriteRequest();
    /** Write the start of bytes, not empty. Returns how many the socket took. A socket error closes
     *  the stream. */
    size_t Write(std::span<const uint8_t> bytes);
    /** Close the stream, for this reason. */
    void Fail(std::string reason);
};

} // namespace privbcast

#endif // BITCOIN_PRIVBCAST_STREAM_H
