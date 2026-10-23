// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_PRIVBCAST_SOCKS5_H
#define BITCOIN_PRIVBCAST_SOCKS5_H

#include <netaddress.h>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

class FastRandomContext;

namespace privbcast {

/** Whether a SOCKS5 request can carry host as its destination: one to 255 bytes, its length
 *  being one byte, none of them NUL (RFC 1928). */
bool Socks5CanCarry(std::string_view host);

/** The username and password a stream presents to the proxy, which puts streams with different
 *  credentials on different circuits (B2). */
struct Socks5Credentials {
    std::string username;
    std::string password;

    /** Fresh random credentials. */
    static Socks5Credentials Draw(FastRandomContext& rng);
};

/**
 * The client side of one SOCKS5 exchange (RFC 1928) with username and password authentication
 * (RFC 1929), driven by bytes: the caller writes what BytesToSend() returns and passes what the
 * proxy sends to Received(). It owns no socket and reads no clock.
 *
 * The greeting offers username and password authentication only, with the credentials the caller
 * gives or, by default, fresh ones, so that Tor puts each attempt and each query set on its own
 * circuit (B2). A proxy that selects any other method fails the exchange. Each reply is accepted only once the message it answers has
 * been written in full.
 */
class Socks5Client
{
public:
    enum class Command : uint8_t {
        /** Open a stream to the destination. */
        Connect = 0x01,
        /** Tor's extension: resolve a name to one address, returned in the reply. */
        Resolve = 0xF0,
    };

    /**
     * @param[in] host  A domain name, sent as DOMAINNAME, or a numeric IPv4 or IPv6 address, sent
     *                  as IPV4 or IPV6. A host that cannot be sent (Socks5CanCarry()) fails the
     *                  exchange before anything is sent.
     * @param[in] credentials  Presented to the proxy.
     */
    Socks5Client(Command cmd, const std::string& host, uint16_t port, Socks5Credentials credentials);
    /** As above, with fresh credentials drawn from rng. */
    Socks5Client(Command cmd, const std::string& host, uint16_t port, FastRandomContext& rng);

    /** What to write next. Empty while waiting for the proxy, once done and once failed. */
    std::span<const uint8_t> BytesToSend() const;
    /** The first n bytes of BytesToSend() were written. */
    void MarkSent(size_t n);
    /** Process bytes from the proxy. Returns how many were consumed: all of them until the exchange
     *  ends, none after it. Bytes past the final reply are the caller's. */
    size_t Received(std::span<const uint8_t> bytes);

    /** The final reply arrived and reported success. */
    bool Done() const { return m_stage == Stage::Done; }
    bool Failed() const { return m_stage == Stage::Failed; }
    /** Why the exchange failed: for a refused request, the reply code and its meaning. */
    const std::string& Reason() const { return m_reason; }
    /** For RESOLVE once done: the IPv4 or IPv6 address in the reply. Empty if the reply carried a
     *  name instead. */
    std::optional<CNetAddr> Answer() const { return m_answer; }

    const std::string& Username() const { return m_credentials.username; }
    const std::string& Password() const { return m_credentials.password; }

private:
    /** The reply awaited next, or the end of the exchange. */
    enum class Stage : uint8_t { Method, Auth, Reply, Done, Failed };

    Command m_cmd;
    Socks5Credentials m_credentials;
    /** The CONNECT or RESOLVE request, sent once authenticated. */
    std::vector<uint8_t> m_request;

    Stage m_stage{Stage::Method};
    /** The message being written, and how much of it was. */
    std::vector<uint8_t> m_send;
    size_t m_sent{0};
    /** The reply received so far, and its length once known. */
    std::vector<uint8_t> m_reply;
    size_t m_reply_size{0};

    std::string m_reason;
    std::optional<CNetAddr> m_answer;

    /** Check the last byte of m_reply, and move on when the reply is complete. */
    void Parse();
    /** Await the reply to msg, which is written next. */
    void Next(Stage stage, std::vector<uint8_t> msg);
    void Fail(std::string reason);
};

} // namespace privbcast

#endif // BITCOIN_PRIVBCAST_SOCKS5_H
