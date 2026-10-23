// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bitcoin-build-config.h> // IWYU pragma: keep

#include <privbcast/stream.h>

#include <compat/compat.h>
#include <netbase.h>
#include <privbcast/params.h>
#include <privbcast/socks5.h>
#include <tinyformat.h>
#include <util/check.h>
#include <util/sock.h>

#include <array>
#include <cassert>
#include <cstring>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <utility>
#include <vector>

#ifdef HAVE_SOCKADDR_UN
#include <sys/un.h>
#endif

namespace privbcast {
namespace {

/** The most read from the proxy at a time during the exchange. Its final reply has at most 262
 *  bytes; anything after it is the destination's. */
constexpr size_t REPLY_READ_SIZE{512};

/** Where the proxy listens: a TCP endpoint or a unix socket path, never a name (B1). */
struct ProxyAddress {
    sockaddr_storage addr{};
    socklen_t len{sizeof(addr)};
    int protocol{IPPROTO_TCP};
};

std::optional<ProxyAddress> GetProxyAddress(const Proxy& proxy)
{
    ProxyAddress address;
    if (!proxy.m_is_unix_socket) {
        if (!proxy.proxy.GetSockAddr(reinterpret_cast<sockaddr*>(&address.addr), &address.len)) return std::nullopt;
        return address;
    }
#ifdef HAVE_SOCKADDR_UN
    if (!IsUnixSocketPath(proxy.m_unix_socket_path)) return std::nullopt;
    const std::string path{proxy.m_unix_socket_path.substr(ADDR_PREFIX_UNIX.size())};
    sockaddr_un addr_un{};
    static_assert(sizeof(addr_un) <= sizeof(address.addr));
    addr_un.sun_family = AF_UNIX;
    // IsUnixSocketPath() leaves room for the terminating NUL.
    std::memcpy(addr_un.sun_path, path.data(), path.size());
    std::memcpy(&address.addr, &addr_un, sizeof(addr_un));
    address.len = sizeof(addr_un);
    address.protocol = 0;
    return address;
#else
    return std::nullopt;
#endif
}

} // namespace

ProxyStream::ProxyStream(const Proxy& proxy, Socks5Client socks, const Timing& timing, SteadyMs now)
    : m_proxy{proxy},
      m_socks{std::move(socks)},
      m_stage_timeout{timing.Scale(SOCKS_STAGE_TIMEOUT)},
      m_stage_start{now}
{
}

bool ProxyStream::Open()
{
    if (!Assume(m_phase == Phase::Connecting && !m_sock)) return false;
    if (m_socks.Failed()) {
        Fail(m_socks.Reason());
        return false;
    }
    const std::optional<ProxyAddress> address{GetProxyAddress(m_proxy)};
    if (!address) {
        Fail(strprintf("the proxy %s is neither an address nor a unix socket path", m_proxy.ToString()));
        return false;
    }
    std::unique_ptr<Sock> sock{CreateSock(address->addr.ss_family, SOCK_STREAM, address->protocol)};
    if (!sock) {
        Fail("cannot create a socket");
        return false;
    }
    m_sock = std::move(sock);
    if (m_sock->Connect(reinterpret_cast<const sockaddr*>(&address->addr), address->len) == SOCKET_ERROR) {
        const int err{WSAGetLastError()};
        // In progress. WSAEINVAL is what some legacy versions of winsock report.
        if (err == WSAEINPROGRESS || err == WSAEWOULDBLOCK || err == WSAEINVAL) return true;
        Fail(strprintf("connect to the proxy failed: %s", NetworkErrorString(err)));
        return false;
    }
    StartExchange(m_stage_start);
    return true;
}

Sock::Event ProxyStream::WantedEvents() const
{
    switch (m_phase) {
    case Phase::Connecting:
        return Sock::SendEvent;
    case Phase::Socks:
        return m_socks.BytesToSend().empty() ? Sock::RecvEvent : Sock::Event{Sock::RecvEvent | Sock::SendEvent};
    case Phase::Open:
        return Sock::RecvEvent;
    case Phase::Closed:
        return 0;
    } // no default case, so the compiler can warn about missing cases
    assert(false);
}

void ProxyStream::OnEvents(Sock::Event occurred, SteadyMs now)
{
    if (m_phase != Phase::Connecting && m_phase != Phase::Socks) return;
    if (now >= m_stage_start + m_stage_timeout) {
        return Fail(m_phase == Phase::Connecting ? "timed out connecting to the proxy" : "timed out waiting for the proxy");
    }
    if (m_phase == Phase::Connecting) {
        if (!(occurred & (Sock::SendEvent | Sock::ErrorEvent))) return;
        int error{0};
        socklen_t len{sizeof(error)};
        if (m_sock->GetSockOpt(SOL_SOCKET, SO_ERROR, &error, &len) == SOCKET_ERROR) {
            return Fail(strprintf("getsockopt() failed: %s", NetworkErrorString(WSAGetLastError())));
        }
        if (error != 0) return Fail(strprintf("connect to the proxy failed: %s", NetworkErrorString(error)));
        return StartExchange(now);
    }
    if (occurred & (Sock::RecvEvent | Sock::ErrorEvent)) ReadReply(now);
    if (m_phase == Phase::Socks) WriteRequest();
}

std::optional<SteadyMs> ProxyStream::NextDeadline() const
{
    if (m_phase != Phase::Connecting && m_phase != Phase::Socks) return std::nullopt;
    return m_stage_start + m_stage_timeout;
}

std::vector<uint8_t> ProxyStream::TakeLeftover()
{
    return std::exchange(m_leftover, {});
}

size_t ProxyStream::Send(std::span<const uint8_t> bytes)
{
    if (!Assume(m_phase == Phase::Open)) return 0;
    return Write(bytes);
}

size_t ProxyStream::Recv(std::span<uint8_t> buf)
{
    if (!Assume(m_phase == Phase::Open) || buf.empty()) return 0;
    const ssize_t n{m_sock->Recv(buf.data(), buf.size(), MSG_DONTWAIT)};
    if (n > 0) return static_cast<size_t>(n);
    if (n == 0) {
        m_peer_closed = true;
        Fail("closed by the peer");
        return 0;
    }
    const int err{WSAGetLastError()};
    if (IOErrorIsPermanent(err)) Fail(strprintf("recv() failed: %s", NetworkErrorString(err)));
    return 0;
}

void ProxyStream::Close()
{
    m_sock.reset();
    m_phase = Phase::Closed;
}

void ProxyStream::StartExchange(SteadyMs now)
{
    m_phase = Phase::Socks;
    m_stage_start = now;
    WriteRequest();
}

void ProxyStream::ReadReply(SteadyMs now)
{
    std::array<uint8_t, REPLY_READ_SIZE> buf;
    const ssize_t n{m_sock->Recv(buf.data(), buf.size(), MSG_DONTWAIT)};
    if (n == 0) return Fail("the proxy closed the connection");
    if (n < 0) {
        const int err{WSAGetLastError()};
        if (IOErrorIsPermanent(err)) Fail(strprintf("recv() failed: %s", NetworkErrorString(err)));
        return;
    }
    const std::span<const uint8_t> bytes{buf.data(), static_cast<size_t>(n)};
    const size_t used{m_socks.Received(bytes)};
    if (m_socks.Failed()) return Fail(m_socks.Reason());
    if (m_socks.Done()) {
        m_leftover.assign(bytes.begin() + used, bytes.end());
        m_phase = Phase::Open;
        return;
    }
    // A reply was complete and the client wrote it all, or Received() would have failed, so a
    // message to send means that the next step of the exchange has begun.
    if (!m_socks.BytesToSend().empty()) m_stage_start = now;
}

void ProxyStream::WriteRequest()
{
    const std::span<const uint8_t> bytes{m_socks.BytesToSend()};
    if (bytes.empty()) return;
    m_socks.MarkSent(Write(bytes));
}

size_t ProxyStream::Write(std::span<const uint8_t> bytes)
{
    const ssize_t n{m_sock->Send(bytes.data(), bytes.size(), MSG_NOSIGNAL | MSG_DONTWAIT)};
    if (n >= 0) return static_cast<size_t>(n);
    const int err{WSAGetLastError()};
    if (IOErrorIsPermanent(err)) Fail(strprintf("send() failed: %s", NetworkErrorString(err)));
    return 0;
}

void ProxyStream::Fail(std::string reason)
{
    m_reason = std::move(reason);
    m_sock.reset();
    m_phase = Phase::Closed;
}

} // namespace privbcast
