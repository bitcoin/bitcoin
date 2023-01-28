// Copyright (c) 2009-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/fuzz/util/net.h>

#include <compat/compat.h>
#include <netaddress.h>
#include <protocol.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/util.h>
#include <test/util/net.h>
#include <util/sock.h>
#include <util/time.h>
#include <version.h>

#include <array>
#include <cassert>
#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <thread>
#include <vector>

class CNode;

CNetAddr ConsumeNetAddr(FuzzedDataProvider& fuzzed_data_provider) noexcept
{
    const Network network = fuzzed_data_provider.PickValueInArray({Network::NET_IPV4, Network::NET_IPV6, Network::NET_INTERNAL, Network::NET_ONION});
    CNetAddr net_addr;
    if (network == Network::NET_IPV4) {
        in_addr v4_addr = {};
        v4_addr.s_addr = fuzzed_data_provider.ConsumeIntegral<uint32_t>();
        net_addr = CNetAddr{v4_addr};
    } else if (network == Network::NET_IPV6) {
        if (fuzzed_data_provider.remaining_bytes() >= 16) {
            in6_addr v6_addr = {};
            memcpy(v6_addr.s6_addr, fuzzed_data_provider.ConsumeBytes<uint8_t>(16).data(), 16);
            net_addr = CNetAddr{v6_addr, fuzzed_data_provider.ConsumeIntegral<uint32_t>()};
        }
    } else if (network == Network::NET_INTERNAL) {
        net_addr.SetInternal(fuzzed_data_provider.ConsumeBytesAsString(32));
    } else if (network == Network::NET_ONION) {
        auto pub_key{fuzzed_data_provider.ConsumeBytes<uint8_t>(ADDR_TORV3_SIZE)};
        pub_key.resize(ADDR_TORV3_SIZE);
        const bool ok{net_addr.SetSpecial(OnionToString(pub_key))};
        assert(ok);
    }
    return net_addr;
}

CAddress ConsumeAddress(FuzzedDataProvider& fuzzed_data_provider) noexcept
{
    return {ConsumeService(fuzzed_data_provider), ConsumeWeakEnum(fuzzed_data_provider, ALL_SERVICE_FLAGS), NodeSeconds{std::chrono::seconds{fuzzed_data_provider.ConsumeIntegral<uint32_t>()}}};
}

FuzzedSock::FuzzedSock(FuzzedDataProvider& fuzzed_data_provider)
    : m_fuzzed_data_provider{fuzzed_data_provider}, m_selectable{fuzzed_data_provider.ConsumeBool()}
{
    m_socket = fuzzed_data_provider.ConsumeIntegralInRange<SOCKET>(INVALID_SOCKET - 1, INVALID_SOCKET);
}

FuzzedSock::~FuzzedSock()
{
    // Sock::~Sock() will be called after FuzzedSock::~FuzzedSock() and it will call
    // close(m_socket) if m_socket is not INVALID_SOCKET.
    // Avoid closing an arbitrary file descriptor (m_socket is just a random very high number which
    // theoretically may concide with a real opened file descriptor).
    m_socket = INVALID_SOCKET;
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
    if (len == 0 || m_fuzzed_data_provider.ConsumeBool()) {
        const ssize_t r = m_fuzzed_data_provider.ConsumeBool() ? 0 : -1;
        if (r == -1) {
            SetFuzzedErrNo(m_fuzzed_data_provider, recv_errnos);
        }
        return r;
    }
    std::vector<uint8_t> random_bytes;
    bool pad_to_len_bytes{m_fuzzed_data_provider.ConsumeBool()};
    if (m_peek_data.has_value()) {
        // `MSG_PEEK` was used in the preceding `Recv()` call, return `m_peek_data`.
        random_bytes.assign({m_peek_data.value()});
        if ((flags & MSG_PEEK) == 0) {
            m_peek_data.reset();
        }
        pad_to_len_bytes = false;
    } else if ((flags & MSG_PEEK) != 0) {
        // New call with `MSG_PEEK`.
        random_bytes = m_fuzzed_data_provider.ConsumeBytes<uint8_t>(1);
        if (!random_bytes.empty()) {
            m_peek_data = random_bytes[0];
            pad_to_len_bytes = false;
        }
    } else {
        random_bytes = m_fuzzed_data_provider.ConsumeBytes<uint8_t>(
            m_fuzzed_data_provider.ConsumeIntegralInRange<size_t>(0, len));
    }
    if (random_bytes.empty()) {
        const ssize_t r = m_fuzzed_data_provider.ConsumeBool() ? 0 : -1;
        if (r == -1) {
            SetFuzzedErrNo(m_fuzzed_data_provider, recv_errnos);
        }
        return r;
    }
    std::memcpy(buf, random_bytes.data(), random_bytes.size());
    if (pad_to_len_bytes) {
        if (len > random_bytes.size()) {
            std::memset((char*)buf + random_bytes.size(), 0, len - random_bytes.size());
        }
        return len;
    }
    if (m_fuzzed_data_provider.ConsumeBool() && std::getenv("FUZZED_SOCKET_FAKE_LATENCY") != nullptr) {
        std::this_thread::sleep_for(std::chrono::milliseconds{2});
    }
    return random_bytes.size();
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
    *name_len = m_fuzzed_data_provider.ConsumeData(name, *name_len);
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
        *occurred = m_fuzzed_data_provider.ConsumeBool() ? requested : 0;
    }
    return true;
}

bool FuzzedSock::WaitMany(std::chrono::milliseconds timeout, EventsPerSock& events_per_sock) const
{
    for (auto& [sock, events] : events_per_sock) {
        (void)sock;
        events.occurred = m_fuzzed_data_provider.ConsumeBool() ? events.requested : 0;
    }
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
    connman.Handshake(node,
                      /*successfully_connected=*/fuzzed_data_provider.ConsumeBool(),
                      /*remote_services=*/ConsumeWeakEnum(fuzzed_data_provider, ALL_SERVICE_FLAGS),
                      /*local_services=*/ConsumeWeakEnum(fuzzed_data_provider, ALL_SERVICE_FLAGS),
                      /*version=*/fuzzed_data_provider.ConsumeIntegralInRange<int32_t>(MIN_PEER_PROTO_VERSION, std::numeric_limits<int32_t>::max()),
                      /*relay_txs=*/fuzzed_data_provider.ConsumeBool());
}
