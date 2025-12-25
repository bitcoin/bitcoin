// Copyright (c) 2009-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/fuzz/util/net.h>

#include <compat/compat.h>
#include <netaddress.h>
#include <node/protocol_version.h>
#include <protocol.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/util.h>
#include <test/util/net.h>
#include <util/sock.h>
#include <util/time.h>

#include <array>
#include <cassert>
#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <ranges>
#include <thread>
#include <vector>

class CNode;

CNetAddr ConsumeNetAddr(FuzzedDataProvider& fuzzed_data_provider, FastRandomContext* rand) noexcept
{
    struct NetAux {
        Network net;
        CNetAddr::BIP155Network bip155;
        size_t len;
    };

    static constexpr std::array<NetAux, 6> nets{
        NetAux{.net = Network::NET_IPV4, .bip155 = CNetAddr::BIP155Network::IPV4, .len = ADDR_IPV4_SIZE},
        NetAux{.net = Network::NET_IPV6, .bip155 = CNetAddr::BIP155Network::IPV6, .len = ADDR_IPV6_SIZE},
        NetAux{.net = Network::NET_ONION, .bip155 = CNetAddr::BIP155Network::TORV3, .len = ADDR_TORV3_SIZE},
        NetAux{.net = Network::NET_I2P, .bip155 = CNetAddr::BIP155Network::I2P, .len = ADDR_I2P_SIZE},
        NetAux{.net = Network::NET_CJDNS, .bip155 = CNetAddr::BIP155Network::CJDNS, .len = ADDR_CJDNS_SIZE},
        NetAux{.net = Network::NET_INTERNAL, .bip155 = CNetAddr::BIP155Network{0}, .len = 0},
    };

    const size_t nets_index{rand == nullptr
        ? fuzzed_data_provider.ConsumeIntegralInRange<size_t>(0, nets.size() - 1)
        : static_cast<size_t>(rand->randrange(nets.size()))};

    const auto& aux = nets[nets_index];

    CNetAddr addr;

    if (aux.net == Network::NET_INTERNAL) {
        if (rand == nullptr) {
            addr.SetInternal(fuzzed_data_provider.ConsumeBytesAsString(32));
        } else {
            const auto v = rand->randbytes(32);
            addr.SetInternal(std::string{v.begin(), v.end()});
        }
        return addr;
    }

    DataStream s;

    s << static_cast<uint8_t>(aux.bip155);

    std::vector<uint8_t> addr_bytes;
    if (rand == nullptr) {
        addr_bytes = fuzzed_data_provider.ConsumeBytes<uint8_t>(aux.len);
        addr_bytes.resize(aux.len);
    } else {
        addr_bytes = rand->randbytes(aux.len);
    }
    if (aux.net == NET_IPV6 && addr_bytes[0] == CJDNS_PREFIX) { // Avoid generating IPv6 addresses that look like CJDNS.
        addr_bytes[0] = 0x55; // Just an arbitrary number, anything != CJDNS_PREFIX would do.
    }
    if (aux.net == NET_CJDNS) { // Avoid generating CJDNS addresses that don't start with CJDNS_PREFIX because those are !IsValid().
        addr_bytes[0] = CJDNS_PREFIX;
    }
    s << addr_bytes;

    s >> CAddress::V2_NETWORK(addr);

    return addr;
}

CAddress ConsumeAddress(FuzzedDataProvider& fuzzed_data_provider) noexcept
{
    return {ConsumeService(fuzzed_data_provider), ConsumeWeakEnum(fuzzed_data_provider, ALL_SERVICE_FLAGS), NodeSeconds{std::chrono::seconds{fuzzed_data_provider.ConsumeIntegral<uint32_t>()}}};
}

template <typename P>
P ConsumeDeserializationParams(FuzzedDataProvider& fuzzed_data_provider) noexcept
{
    constexpr std::array ADDR_ENCODINGS{
        CNetAddr::Encoding::V1,
        CNetAddr::Encoding::V2,
    };
    constexpr std::array ADDR_FORMATS{
        CAddress::Format::Disk,
        CAddress::Format::Network,
    };
    if constexpr (std::is_same_v<P, CNetAddr::SerParams>) {
        return P{PickValue(fuzzed_data_provider, ADDR_ENCODINGS)};
    }
    if constexpr (std::is_same_v<P, CAddress::SerParams>) {
        return P{{PickValue(fuzzed_data_provider, ADDR_ENCODINGS)}, PickValue(fuzzed_data_provider, ADDR_FORMATS)};
    }
}
template CNetAddr::SerParams ConsumeDeserializationParams(FuzzedDataProvider&) noexcept;
template CAddress::SerParams ConsumeDeserializationParams(FuzzedDataProvider&) noexcept;

FuzzedSock::FuzzedSock(FuzzedDataProvider& fuzzed_data_provider)
    : Sock{fuzzed_data_provider.ConsumeIntegralInRange<SOCKET>(INVALID_SOCKET - 1, INVALID_SOCKET)},
      m_fuzzed_data_provider{fuzzed_data_provider},
      m_selectable{fuzzed_data_provider.ConsumeBool()},
      m_time{MockableSteadyClock::INITIAL_MOCK_TIME}
{
    ElapseTime(std::chrono::seconds(0)); // start mocking the steady clock.
}

FuzzedSock::~FuzzedSock()
{
    // Sock::~Sock() will be called after FuzzedSock::~FuzzedSock() and it will call
    // close(m_socket) if m_socket is not INVALID_SOCKET.
    // Avoid closing an arbitrary file descriptor (m_socket is just a random very high number which
    // theoretically may concide with a real opened file descriptor).
    m_socket = INVALID_SOCKET;
}

void FuzzedSock::ElapseTime(std::chrono::milliseconds duration) const
{
    m_time += duration;
    MockableSteadyClock::SetMockTime(m_time);
}

FuzzedSock& FuzzedSock::operator=(Sock&& other)
{
    assert(false && "Move of Sock into FuzzedSock not allowed.");
    return *this;
}

ssize_t FuzzedSock::Send(const void* data, size_t len, int flags) const
{
    constexpr std::array send_errnos{
        EACCES,
        EAGAIN,
        EALREADY,
        EBADF,
        ECONNRESET,
        EDESTADDRREQ,
        EFAULT,
        EINTR,
        EINVAL,
        EISCONN,
        EMSGSIZE,
        ENOBUFS,
        ENOMEM,
        ENOTCONN,
        ENOTSOCK,
        EOPNOTSUPP,
        EPIPE,
        EWOULDBLOCK,
    };
    if (m_fuzzed_data_provider.ConsumeBool()) {
        return len;
    }
    const ssize_t r = m_fuzzed_data_provider.ConsumeIntegralInRange<ssize_t>(-1, len);
    if (r == -1) {
        SetFuzzedErrNo(m_fuzzed_data_provider, send_errnos);
    }
    return r;
}

ssize_t FuzzedSock::Recv(void* buf, size_t len, int flags) const
{
    // Have a permanent error at recv_errnos[0] because when the fuzzed data is exhausted
    // SetFuzzedErrNo() will always return the first element and we want to avoid Recv()
    // returning -1 and setting errno to EAGAIN repeatedly.
    constexpr std::array recv_errnos{
        ECONNREFUSED,
        EAGAIN,
        EBADF,
        EFAULT,
        EINTR,
        EINVAL,
        ENOMEM,
        ENOTCONN,
        ENOTSOCK,
        EWOULDBLOCK,
    };
    assert(buf != nullptr || len == 0);

    // Do the latency before any of the "return" statements.
    if (m_fuzzed_data_provider.ConsumeBool() && std::getenv("FUZZED_SOCKET_FAKE_LATENCY") != nullptr) {
        std::this_thread::sleep_for(std::chrono::milliseconds{2});
    }

    if (len == 0 || m_fuzzed_data_provider.ConsumeBool()) {
        const ssize_t r = m_fuzzed_data_provider.ConsumeBool() ? 0 : -1;
        if (r == -1) {
            SetFuzzedErrNo(m_fuzzed_data_provider, recv_errnos);
        }
        return r;
    }

    size_t copied_so_far{0};

    if (!m_peek_data.empty()) {
        // `MSG_PEEK` was used in the preceding `Recv()` call, copy the first bytes from `m_peek_data`.
        const size_t copy_len{std::min(len, m_peek_data.size())};
        std::memcpy(buf, m_peek_data.data(), copy_len);
        copied_so_far += copy_len;
        if ((flags & MSG_PEEK) == 0) {
            m_peek_data.erase(m_peek_data.begin(), m_peek_data.begin() + copy_len);
        }
    }

    if (copied_so_far == len) {
        return copied_so_far;
    }

    auto new_data = ConsumeRandomLengthByteVector(m_fuzzed_data_provider, len - copied_so_far);
    if (new_data.empty()) return copied_so_far;

    std::memcpy(reinterpret_cast<uint8_t*>(buf) + copied_so_far, new_data.data(), new_data.size());
    copied_so_far += new_data.size();

    if ((flags & MSG_PEEK) != 0) {
        m_peek_data.insert(m_peek_data.end(), new_data.begin(), new_data.end());
    }

    if (copied_so_far == len || m_fuzzed_data_provider.ConsumeBool()) {
        return copied_so_far;
    }

    // Pad to len bytes.
    std::memset(reinterpret_cast<uint8_t*>(buf) + copied_so_far, 0x0, len - copied_so_far);

    return len;
}

int FuzzedSock::Connect(const sockaddr*, socklen_t) const
{
    // Have a permanent error at connect_errnos[0] because when the fuzzed data is exhausted
    // SetFuzzedErrNo() will always return the first element and we want to avoid Connect()
    // returning -1 and setting errno to EAGAIN repeatedly.
    constexpr std::array connect_errnos{
        ECONNREFUSED,
        EAGAIN,
        ECONNRESET,
        EHOSTUNREACH,
        EINPROGRESS,
        EINTR,
        ENETUNREACH,
        ETIMEDOUT,
    };
    if (m_fuzzed_data_provider.ConsumeBool()) {
        SetFuzzedErrNo(m_fuzzed_data_provider, connect_errnos);
        return -1;
    }
    return 0;
}

int FuzzedSock::Bind(const sockaddr*, socklen_t) const
{
    // Have a permanent error at bind_errnos[0] because when the fuzzed data is exhausted
    // SetFuzzedErrNo() will always set the global errno to bind_errnos[0]. We want to
    // avoid this method returning -1 and setting errno to a temporary error (like EAGAIN)
    // repeatedly because proper code should retry on temporary errors, leading to an
    // infinite loop.
    constexpr std::array bind_errnos{
        EACCES,
        EADDRINUSE,
        EADDRNOTAVAIL,
        EAGAIN,
    };
    if (m_fuzzed_data_provider.ConsumeBool()) {
        SetFuzzedErrNo(m_fuzzed_data_provider, bind_errnos);
        return -1;
    }
    return 0;
}

int FuzzedSock::Listen(int) const
{
    // Have a permanent error at listen_errnos[0] because when the fuzzed data is exhausted
    // SetFuzzedErrNo() will always set the global errno to listen_errnos[0]. We want to
    // avoid this method returning -1 and setting errno to a temporary error (like EAGAIN)
    // repeatedly because proper code should retry on temporary errors, leading to an
    // infinite loop.
    constexpr std::array listen_errnos{
        EADDRINUSE,
        EINVAL,
        EOPNOTSUPP,
    };
    if (m_fuzzed_data_provider.ConsumeBool()) {
        SetFuzzedErrNo(m_fuzzed_data_provider, listen_errnos);
        return -1;
    }
    return 0;
}

std::unique_ptr<Sock> FuzzedSock::Accept(sockaddr* addr, socklen_t* addr_len) const
{
    constexpr std::array accept_errnos{
        ECONNABORTED,
        EINTR,
        ENOMEM,
    };
    if (m_fuzzed_data_provider.ConsumeBool()) {
        SetFuzzedErrNo(m_fuzzed_data_provider, accept_errnos);
        return std::unique_ptr<FuzzedSock>();
    }
    if (addr != nullptr) {
        // Set a fuzzed address in the output argument addr.
        memset(addr, 0x00, *addr_len);
        if (m_fuzzed_data_provider.ConsumeBool()) {
            // IPv4
            const socklen_t write_len = static_cast<socklen_t>(sizeof(sockaddr_in));
            if (*addr_len >= write_len) {
                *addr_len = write_len;
                auto addr4 = reinterpret_cast<sockaddr_in*>(addr);
                addr4->sin_family = AF_INET;
                const auto sin_addr_bytes{m_fuzzed_data_provider.ConsumeBytes<std::byte>(sizeof(addr4->sin_addr))};
                std::ranges::copy(sin_addr_bytes, reinterpret_cast<std::byte*>(&addr4->sin_addr));
                addr4->sin_port = m_fuzzed_data_provider.ConsumeIntegralInRange<uint16_t>(1, 65535);
            }
        } else {
            // IPv6
            const socklen_t write_len = static_cast<socklen_t>(sizeof(sockaddr_in6));
            if (*addr_len >= write_len) {
                *addr_len = write_len;
                auto addr6 = reinterpret_cast<sockaddr_in6*>(addr);
                addr6->sin6_family = AF_INET6;
                const auto sin_addr_bytes{m_fuzzed_data_provider.ConsumeBytes<std::byte>(sizeof(addr6->sin6_addr))};
                std::ranges::copy(sin_addr_bytes, reinterpret_cast<std::byte*>(&addr6->sin6_addr));
                addr6->sin6_port = m_fuzzed_data_provider.ConsumeIntegralInRange<uint16_t>(1, 65535);
            }
        }
    }
    return std::make_unique<FuzzedSock>(m_fuzzed_data_provider);
}

int FuzzedSock::GetSockOpt(int level, int opt_name, void* opt_val, socklen_t* opt_len) const
{
    constexpr std::array getsockopt_errnos{
        ENOMEM,
        ENOBUFS,
    };
    if (m_fuzzed_data_provider.ConsumeBool()) {
        SetFuzzedErrNo(m_fuzzed_data_provider, getsockopt_errnos);
        return -1;
    }
    if (opt_val == nullptr) {
        return 0;
    }
    std::memcpy(opt_val,
                ConsumeFixedLengthByteVector(m_fuzzed_data_provider, *opt_len).data(),
                *opt_len);
    return 0;
}

int FuzzedSock::SetSockOpt(int, int, const void*, socklen_t) const
{
    constexpr std::array setsockopt_errnos{
        ENOMEM,
        ENOBUFS,
    };
    if (m_fuzzed_data_provider.ConsumeBool()) {
        SetFuzzedErrNo(m_fuzzed_data_provider, setsockopt_errnos);
        return -1;
    }
    return 0;
}

int FuzzedSock::GetSockName(sockaddr* name, socklen_t* name_len) const
{
    constexpr std::array getsockname_errnos{
        ECONNRESET,
        ENOBUFS,
    };
    if (m_fuzzed_data_provider.ConsumeBool()) {
        SetFuzzedErrNo(m_fuzzed_data_provider, getsockname_errnos);
        return -1;
    }
    assert(name_len);
    const auto bytes{ConsumeRandomLengthByteVector(m_fuzzed_data_provider, *name_len)};
    if (bytes.size() < (int)sizeof(sockaddr)) return -1;
    std::memcpy(name, bytes.data(), bytes.size());
    *name_len = bytes.size();
    return 0;
}

bool FuzzedSock::SetNonBlocking() const
{
    constexpr std::array setnonblocking_errnos{
        EBADF,
        EPERM,
    };
    if (m_fuzzed_data_provider.ConsumeBool()) {
        SetFuzzedErrNo(m_fuzzed_data_provider, setnonblocking_errnos);
        return false;
    }
    return true;
}

bool FuzzedSock::IsSelectable() const
{
    return m_selectable;
}

bool FuzzedSock::Wait(std::chrono::milliseconds timeout, Event requested, Event* occurred) const
{
    constexpr std::array wait_errnos{
        EBADF,
        EINTR,
        EINVAL,
    };
    if (m_fuzzed_data_provider.ConsumeBool()) {
        SetFuzzedErrNo(m_fuzzed_data_provider, wait_errnos);
        return false;
    }
    if (occurred != nullptr) {
        // We simulate the requested event as occurred when ConsumeBool()
        // returns false. This avoids simulating endless waiting if the
        // FuzzedDataProvider runs out of data.
        *occurred = m_fuzzed_data_provider.ConsumeBool() ? 0 : requested;
    }
    ElapseTime(timeout);
    return true;
}

bool FuzzedSock::WaitMany(std::chrono::milliseconds timeout, EventsPerSock& events_per_sock) const
{
    for (auto& [sock, events] : events_per_sock) {
        (void)sock;
        // We simulate the requested event as occurred when ConsumeBool()
        // returns false. This avoids simulating endless waiting if the
        // FuzzedDataProvider runs out of data.
        events.occurred = m_fuzzed_data_provider.ConsumeBool() ? 0 : events.requested;
    }
    ElapseTime(timeout);
    return true;
}

bool FuzzedSock::IsConnected(std::string& errmsg) const
{
    if (m_fuzzed_data_provider.ConsumeBool()) {
        return true;
    }
    errmsg = "disconnected at random by the fuzzer";
    return false;
}

void FillNode(FuzzedDataProvider& fuzzed_data_provider, ConnmanTestMsg& connman, CNode& node) noexcept
{
    auto successfully_connected = fuzzed_data_provider.ConsumeBool();
    auto remote_services = ConsumeWeakEnum(fuzzed_data_provider, ALL_SERVICE_FLAGS);
    auto local_services = ConsumeWeakEnum(fuzzed_data_provider, ALL_SERVICE_FLAGS);
    auto version = fuzzed_data_provider.ConsumeIntegralInRange<int32_t>(MIN_PEER_PROTO_VERSION, std::numeric_limits<int32_t>::max());
    auto relay_txs = fuzzed_data_provider.ConsumeBool();
    connman.Handshake(node, successfully_connected, remote_services, local_services, version, relay_txs);
}
