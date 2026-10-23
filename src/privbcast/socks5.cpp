// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <privbcast/socks5.h>

#include <compat/compat.h>
#include <crypto/hex_base.h>
#include <netaddress.h>
#include <netbase.h>
#include <random.h>
#include <tinyformat.h>
#include <util/check.h>

#include <algorithm>
#include <cstring>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace privbcast {
namespace {

constexpr uint8_t SOCKS_VERSION{0x05};
/** RFC 1929 username and password authentication, the only method offered. */
constexpr uint8_t METHOD_USERNAME_PASSWORD{0x02};
constexpr uint8_t AUTH_VERSION{0x01};
constexpr uint8_t AUTH_SUCCESS{0x00};
constexpr uint8_t REPLY_SUCCEEDED{0x00};
constexpr uint8_t ATYP_IPV4{0x01};
constexpr uint8_t ATYP_DOMAINNAME{0x03};
constexpr uint8_t ATYP_IPV6{0x04};
/** Random bytes in each of the username and the password, which are sent hex encoded. */
constexpr size_t CREDENTIAL_BYTES{16};
/** VER, REP, RSV and ATYP, before the bound address. */
constexpr size_t REPLY_HEAD_SIZE{4};

template <typename T>
void AppendBytes(std::vector<uint8_t>& out, const T& object)
{
    const auto* begin{reinterpret_cast<const uint8_t*>(&object)};
    out.insert(out.end(), begin, begin + sizeof(object));
}

} // namespace

bool Socks5CanCarry(std::string_view host)
{
    return !host.empty() && host.size() <= 255 && host.find('\0') == std::string_view::npos;
}

Socks5Credentials Socks5Credentials::Draw(FastRandomContext& rng)
{
    return {HexStr(rng.randbytes(CREDENTIAL_BYTES)), HexStr(rng.randbytes(CREDENTIAL_BYTES))};
}

Socks5Client::Socks5Client(Command cmd, const std::string& host, uint16_t port, FastRandomContext& rng)
    : Socks5Client{cmd, host, port, Socks5Credentials::Draw(rng)}
{
}

Socks5Client::Socks5Client(Command cmd, const std::string& host, uint16_t port, Socks5Credentials credentials)
    : m_cmd{cmd},
      m_credentials{std::move(credentials)}
{
    if (!Socks5CanCarry(host)) {
        Fail("destination is not a name or address that SOCKS5 can carry");
        return;
    }
    // VER CMD RSV ATYP DST.ADDR DST.PORT (RFC 1928 section 4). inet_pton only parses; it never
    // looks a name up (B1).
    m_request = {SOCKS_VERSION, static_cast<uint8_t>(cmd), 0x00};
    in_addr ipv4{};
    in6_addr ipv6{};
    if (inet_pton(AF_INET, host.c_str(), &ipv4) == 1) {
        m_request.push_back(ATYP_IPV4);
        AppendBytes(m_request, ipv4);
    } else if (inet_pton(AF_INET6, host.c_str(), &ipv6) == 1) {
        m_request.push_back(ATYP_IPV6);
        AppendBytes(m_request, ipv6);
    } else {
        m_request.push_back(ATYP_DOMAINNAME);
        m_request.push_back(static_cast<uint8_t>(host.size()));
        m_request.insert(m_request.end(), host.begin(), host.end());
    }
    m_request.push_back(static_cast<uint8_t>(port >> 8));
    m_request.push_back(static_cast<uint8_t>(port & 0xFF));

    // VER NMETHODS METHODS (RFC 1928 section 3): username and password only (B2).
    m_send = {SOCKS_VERSION, 0x01, METHOD_USERNAME_PASSWORD};
}

std::span<const uint8_t> Socks5Client::BytesToSend() const
{
    return std::span{m_send}.subspan(m_sent);
}

void Socks5Client::MarkSent(size_t n)
{
    Assume(n <= m_send.size() - m_sent);
    m_sent += std::min(n, m_send.size() - m_sent);
}

size_t Socks5Client::Received(std::span<const uint8_t> bytes)
{
    size_t used{0};
    while (used < bytes.size() && (m_stage == Stage::Method || m_stage == Stage::Auth || m_stage == Stage::Reply)) {
        const uint8_t byte{bytes[used++]};
        if (m_sent < m_send.size()) {
            Fail("proxy replied before the request was written");
            break;
        }
        m_reply.push_back(byte);
        Parse();
    }
    return used;
}

void Socks5Client::Parse()
{
    const size_t pos{m_reply.size() - 1};
    const uint8_t byte{m_reply.back()};
    switch (m_stage) {
    case Stage::Method:
        // VER METHOD (RFC 1928 section 3)
        if (pos == 0 && byte != SOCKS_VERSION) return Fail(strprintf("malformed method selection: version %d", byte));
        if (pos == 1) {
            if (byte != METHOD_USERNAME_PASSWORD) {
                return Fail(strprintf("proxy selected method 0x%02x, not username and password authentication", byte));
            }
            // VER ULEN UNAME PLEN PASSWD (RFC 1929 section 2)
            const auto& [username, password]{m_credentials};
            std::vector<uint8_t> auth{AUTH_VERSION, static_cast<uint8_t>(username.size())};
            auth.insert(auth.end(), username.begin(), username.end());
            auth.push_back(static_cast<uint8_t>(password.size()));
            auth.insert(auth.end(), password.begin(), password.end());
            Next(Stage::Auth, std::move(auth));
        }
        return;
    case Stage::Auth:
        // VER STATUS (RFC 1929 section 2)
        if (pos == 0 && byte != AUTH_VERSION) return Fail(strprintf("malformed authentication reply: version %d", byte));
        if (pos == 1) {
            if (byte != AUTH_SUCCESS) return Fail(strprintf("proxy refused the credentials: status 0x%02x", byte));
            Next(Stage::Reply, std::move(m_request));
        }
        return;
    case Stage::Reply:
        // VER REP RSV ATYP BND.ADDR BND.PORT (RFC 1928 section 6)
        if (pos == 0 && byte != SOCKS_VERSION) return Fail(strprintf("malformed reply: version %d", byte));
        if (pos == 1 && byte != REPLY_SUCCEEDED) return Fail(strprintf("proxy reply 0x%02x: %s", byte, Socks5ErrorString(byte)));
        if (pos == 2 && byte != 0x00) return Fail("malformed reply: reserved byte set");
        if (pos == 3) {
            switch (byte) {
            case ATYP_IPV4: m_reply_size = REPLY_HEAD_SIZE + 4 + 2; break;
            case ATYP_IPV6: m_reply_size = REPLY_HEAD_SIZE + 16 + 2; break;
            case ATYP_DOMAINNAME: break; // the length comes next
            default: return Fail(strprintf("malformed reply: address type 0x%02x", byte));
            }
        }
        if (pos == REPLY_HEAD_SIZE && m_reply[3] == ATYP_DOMAINNAME) {
            if (byte == 0) return Fail("malformed reply: empty name");
            m_reply_size = REPLY_HEAD_SIZE + 1 + byte + 2;
        }
        if (pos >= REPLY_HEAD_SIZE && m_reply.size() == m_reply_size) {
            const uint8_t* bound_addr{m_reply.data() + REPLY_HEAD_SIZE};
            if (m_cmd == Command::Resolve && m_reply[3] == ATYP_IPV4) {
                in_addr ipv4;
                std::memcpy(&ipv4, bound_addr, sizeof(ipv4));
                m_answer = CNetAddr{ipv4};
            } else if (m_cmd == Command::Resolve && m_reply[3] == ATYP_IPV6) {
                in6_addr ipv6;
                std::memcpy(&ipv6, bound_addr, sizeof(ipv6));
                m_answer = CNetAddr{ipv6};
            }
            m_stage = Stage::Done;
        }
        return;
    case Stage::Done:
    case Stage::Failed:
        break;
    } // no default case, so the compiler can warn about missing cases
    Assume(false);
}

void Socks5Client::Next(Stage stage, std::vector<uint8_t> msg)
{
    m_stage = stage;
    m_send = std::move(msg);
    m_sent = 0;
    m_reply.clear();
}

void Socks5Client::Fail(std::string reason)
{
    m_stage = Stage::Failed;
    m_reason = std::move(reason);
    m_send.clear();
    m_sent = 0;
}

} // namespace privbcast
