// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_UTIL_SOCK_H
#define BITCOIN_UTIL_SOCK_H

#include <compat/compat.h>
#include <logging.h>
#include <util/time.h>

#include <algorithm>
#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <unordered_map>

class CThreadInterrupt;

/**
 * Maximum time to wait for I/O readiness.
 * It will take up until this time to break off in case of an interruption.
 */
static constexpr auto MAX_WAIT_FOR_IO = 1s;

inline bool IOErrorIsPermanent(int err)
{
    return err != WSAEAGAIN && err != WSAEINTR && err != WSAEWOULDBLOCK && err != WSAEINPROGRESS;
}

/**
 * RAII helper class that manages a socket and closes it automatically when it goes out of scope.
 */
class Sock
{
public:
    Sock() = delete;

    /**
     * Take ownership of an existent socket.
     */
    explicit Sock(SOCKET s);

    /**
     * Copy constructor, disabled because closing the same socket twice is undesirable.
     */
    Sock(const Sock&) = delete;

    /**
     * Move constructor, grab the socket from another object and close ours (if set).
     */
    Sock(Sock&& other);

    /**
     * Destructor, close the socket or do nothing if empty.
     */
    virtual ~Sock();

    /**
     * Copy assignment operator, disabled because closing the same socket twice is undesirable.
     */
    Sock& operator=(const Sock&) = delete;

    /**
     * Move assignment operator, grab the socket from another object and close ours (if set).
     */
    virtual Sock& operator=(Sock&& other);

    /**
     * send(2) wrapper. Equivalent to `send(m_socket, data, len, flags);`. Code that uses this
     * wrapper can be unit tested if this method is overridden by a mock Sock implementation.
     */
    [[nodiscard]] virtual ssize_t Send(const void* data, size_t len, int flags) const;

    /**
     * recv(2) wrapper. Equivalent to `recv(m_socket, buf, len, flags);`. Code that uses this
     * wrapper can be unit tested if this method is overridden by a mock Sock implementation.
     */
    [[nodiscard]] virtual ssize_t Recv(void* buf, size_t len, int flags) const;

    /**
     * connect(2) wrapper. Equivalent to `connect(m_socket, addr, addrlen)`. Code that uses this
     * wrapper can be unit tested if this method is overridden by a mock Sock implementation.
     */
    [[nodiscard]] virtual int Connect(const sockaddr* addr, socklen_t addr_len) const;

    /**
     * bind(2) wrapper. Equivalent to `bind(m_socket, addr, addr_len)`. Code that uses this
     * wrapper can be unit tested if this method is overridden by a mock Sock implementation.
     */
    [[nodiscard]] virtual int Bind(const sockaddr* addr, socklen_t addr_len) const;

    /**
     * listen(2) wrapper. Equivalent to `listen(m_socket, backlog)`. Code that uses this
     * wrapper can be unit tested if this method is overridden by a mock Sock implementation.
     */
    [[nodiscard]] virtual int Listen(int backlog) const;

    /**
     * accept(2) wrapper. Equivalent to `std::make_unique<Sock>(accept(m_socket, addr, addr_len))`.
     * Code that uses this wrapper can be unit tested if this method is overridden by a mock Sock
     * implementation.
     * The returned unique_ptr is empty if `accept()` failed in which case errno will be set.
     */
    [[nodiscard]] virtual std::unique_ptr<Sock> Accept(sockaddr* addr, socklen_t* addr_len) const;

    /**
     * getsockopt(2) wrapper. Equivalent to
     * `getsockopt(m_socket, level, opt_name, opt_val, opt_len)`. Code that uses this
     * wrapper can be unit tested if this method is overridden by a mock Sock implementation.
     */
    [[nodiscard]] virtual int GetSockOpt(int level,
                                         int opt_name,
                                         void* opt_val,
                                         socklen_t* opt_len) const;

#if defined(WIN32)
    /**
     * WSAIoctl wrapper
     * https://learn.microsoft.com/en-us/windows/win32/api/winsock2/nf-winsock2-wsaioctl
     */
    [[nodiscard]] int WSAIoctl(DWORD                                dwIoControlCode,
                               LPVOID                               lpvInBuffer,
                               DWORD                                cbInBuffer,
                               LPVOID                               lpvOutBuffer,
                               DWORD                                cbOutBuffer,
                               LPDWORD                              lpcbBytesReturned,
                               LPWSAOVERLAPPED                      lpOverlapped,
                               LPWSAOVERLAPPED_COMPLETION_ROUTINE   lpCompletionRoutine)
    {
        return ::WSAIoctl(m_socket, dwIoControlCode, lpvInBuffer, cbInBuffer,
                          lpvOutBuffer, cbOutBuffer, lpcbBytesReturned,
                          lpOverlapped, lpCompletionRoutine);
    }
#endif

    /**
     * setsockopt(2) wrapper. Equivalent to
     * `setsockopt(m_socket, level, opt_name, opt_val, opt_len)`. Code that uses this
     * wrapper can be unit tested if this method is overridden by a mock Sock implementation.
     */
    [[nodiscard]] virtual int SetSockOpt(int level,
                                         int opt_name,
                                         const void* opt_val,
                                         socklen_t opt_len) const;

    /**
     * getsockname(2) wrapper. Equivalent to
     * `getsockname(m_socket, name, name_len)`. Code that uses this
     * wrapper can be unit tested if this method is overridden by a mock Sock implementation.
     */
    [[nodiscard]] virtual int GetSockName(sockaddr* name, socklen_t* name_len) const;

    /**
     * Set the non-blocking option on the socket.
     * @return true if set successfully
     */
    [[nodiscard]] virtual bool SetNonBlocking() const;

    /**
     * Check if the underlying socket can be used for `select(2)` (or the `Wait()` method).
     * @return true if selectable
     */
    [[nodiscard]] virtual bool IsSelectable() const;

    using Event = uint8_t;

    /**
     * If passed to `Wait()`, then it will wait for readiness to read from the socket.
     */
    static constexpr Event RecvEvent = 0b001;

    /**
     * If passed to `Wait()`, then it will wait for readiness to send to the socket.
     */
    static constexpr Event SendEvent = 0b010;

    /**
     * Ignored if passed to `Wait()`, but could be set in the occurred events if an
     * exceptional condition has occurred on the socket or if it has been disconnected.
     */
    static constexpr Event ErrorEvent = 0b100;

    /**
     * Wait for readiness for input (recv) or output (send).
     * @param[in] timeout Wait this much for at least one of the requested events to occur.
     * @param[in] requested Wait for those events, bitwise-or of `RecvEvent` and `SendEvent`.
     * @param[out] occurred If not nullptr and the function returns `true`, then this
     * indicates which of the requested events occurred (`ErrorEvent` will be added, even if
     * not requested, if an exceptional event occurs on the socket).
     * A timeout is indicated by return value of `true` and `occurred` being set to 0.
     * @return true on success (or timeout, if `occurred` of 0 is returned), false otherwise
     */
    [[nodiscard]] virtual bool Wait(std::chrono::milliseconds timeout,
                                    Event requested,
                                    Event* occurred = nullptr) const;

    /**
     * Auxiliary requested/occurred events to wait for in `WaitMany()`.
     */
    struct Events {
        explicit Events(Event req) : requested{req} {}
        Event requested;
        Event occurred{0};
    };

    struct HashSharedPtrSock {
        size_t operator()(const std::shared_ptr<const Sock>& s) const
        {
            return s ? s->m_socket : std::numeric_limits<SOCKET>::max();
        }
    };

    struct EqualSharedPtrSock {
        bool operator()(const std::shared_ptr<const Sock>& lhs,
                        const std::shared_ptr<const Sock>& rhs) const
        {
            if (lhs && rhs) {
                return lhs->m_socket == rhs->m_socket;
            }
            if (!lhs && !rhs) {
                return true;
            }
            return false;
        }
    };

    /**
     * On which socket to wait for what events in `WaitMany()`.
     * The `shared_ptr` is copied into the map to ensure that the `Sock` object
     * is not destroyed (its destructor would close the underlying socket).
     * If this happens shortly before or after we call `poll(2)` and a new
     * socket gets created under the same file descriptor number then the report
     * from `WaitMany()` will be bogus.
     */
    using EventsPerSock = std::unordered_map<std::shared_ptr<const Sock>, Events, HashSharedPtrSock, EqualSharedPtrSock>;

    /**
     * Same as `Wait()`, but wait on many sockets within the same timeout.
     * @param[in] timeout Wait this long for at least one of the requested events to occur.
     * @param[in,out] events_per_sock Wait for the requested events on these sockets and set
     * `occurred` for the events that actually occurred.
     * @return true on success (or timeout, if all `what[].occurred` are returned as 0),
     * false otherwise
     */
    [[nodiscard]] virtual bool WaitMany(std::chrono::milliseconds timeout,
                                        EventsPerSock& events_per_sock) const;

    /* Higher level, convenience, methods. These may throw. */

    /**
     * Send the given data, retrying on transient errors.
     * @param[in] data Data to send.
     * @param[in] timeout Timeout for the entire operation.
     * @param[in] interrupt If this is signaled then the operation is canceled.
     * @throws std::runtime_error if the operation cannot be completed. In this case only some of
     * the data will be written to the socket.
     */
    virtual void SendComplete(std::span<const unsigned char> data,
                              std::chrono::milliseconds timeout,
                              CThreadInterrupt& interrupt) const;

    /**
     * Convenience method, equivalent to `SendComplete(MakeUCharSpan(data), timeout, interrupt)`.
     */
    virtual void SendComplete(std::span<const char> data,
                              std::chrono::milliseconds timeout,
                              CThreadInterrupt& interrupt) const;

    /**
     * Read from socket until a terminator character is encountered. Will never consume bytes past
     * the terminator from the socket.
     * @param[in] terminator Character up to which to read from the socket.
     * @param[in] timeout Timeout for the entire operation.
     * @param[in] interrupt If this is signaled then the operation is canceled.
     * @param[in] max_data The maximum amount of data (in bytes) to receive. If this many bytes
     * are received and there is still no terminator, then this method will throw an exception.
     * @return The data that has been read, without the terminating character.
     * @throws std::runtime_error if the operation cannot be completed. In this case some bytes may
     * have been consumed from the socket.
     */
    [[nodiscard]] virtual std::string RecvUntilTerminator(uint8_t terminator,
                                                          std::chrono::milliseconds timeout,
                                                          CThreadInterrupt& interrupt,
                                                          size_t max_data) const;

    /**
     * Check if still connected.
     * @param[out] errmsg The error string, if the socket has been disconnected.
     * @return true if connected
     */
    [[nodiscard]] virtual bool IsConnected(std::string& errmsg) const;

    /**
     * Check if the internal socket is equal to `s`. Use only in tests.
     */
    bool operator==(SOCKET s) const;

protected:
    /**
     * Contained socket. `INVALID_SOCKET` designates the object is empty.
     */
    SOCKET m_socket;

private:
    /**
     * Close `m_socket` if it is not `INVALID_SOCKET`.
     */
    void Close();
};

/** Return readable error string for a network error code */
std::string NetworkErrorString(int err);

/**
 * Wrap platform specific data structures that contain information about TCP
 * connections, tcp_info on /Linux|e.BSD/, tcp_connection_info on macos, and
 * TCP_INFO_V0 on Windows.
 */
class TCPInfo
{
public:
    bool m_valid{true};
#if defined(__linux__) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
    struct tcp_info m_tcp_info;
    socklen_t m_tcp_info_len{sizeof(m_tcp_info)};
#elif defined(__APPLE__)
    struct tcp_connection_info m_tcp_info;
    socklen_t m_tcp_info_len{sizeof(m_tcp_info)};
#elif defined(WIN32_TCPINFO_SUPPORTED)
    TCP_INFO_v0 m_tcp_info;
    DWORD m_tcp_info_len{sizeof(m_tcp_info)};
#endif

    TCPInfo(Sock &s) {
#if defined(__linux__) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
        m_valid = !s.GetSockOpt(IPPROTO_TCP, TCP_INFO, &m_tcp_info, &m_tcp_info_len);
#elif defined(__APPLE__)
        m_valid = !s.GetSockOpt(IPPROTO_TCP, TCP_CONNECTION_INFO, &m_tcp_info, &m_tcp_info_len);
#elif defined(WIN32_TCPINFO_SUPPORTED)
        DWORD version{0};

        // Windows 10 1703 is required for SIO_TCP_INFO, but this
        // will fail at runtime with WSAEOPNOTSUPP if the runtime
        // platform is too old.
        m_valid = !s.WSAIoctl(/*dwIoControlCode=*/SIO_TCP_INFO,
                              /*lpvInBuffer=*/&version,
                              /*cbInBuffer=*/sizeof(version),
                              /*lpvOutBuffer=*/&m_tcp_info,
                              /*cbOutBuffer=*/sizeof(m_tcp_info),
                              /*lpcpBytesReturned=*/&m_tcp_info_len,
                              /*lpvOverlapped=*/nullptr,
                              /*lpCompletionRoutine=*/nullptr);
#else
        m_valid = false;
        LogWarning("Error getting TCP Info, platform not supported!");
        return;
#endif
        if (!m_valid) {
            LogError("Error getting TCP Info: %s", NetworkErrorString(WSAGetLastError()));
        }
    }

    size_t GetTCPWindowSize()
    {
        if (!m_valid) {
            return 0;
        }

        uint32_t cwnd_bytes{};
        std::optional<uint32_t> peer_rwnd_bytes{std::nullopt};

// Linux: tcpi_snd_wnd introduced in 5.4 https://github.com/torvalds/linux/commit/8f7baad7f03543451af27f5380fc816b008aa1f2
// The logic around peer_rwnd_bytes being optional can be removed once the minimum supported kernel is 5.4 or greater.
#if defined(__linux__)
        // We can only create the runtime check if tcpi_snd_wnd is available at compile-time.
        #if defined(TCP_INFO_HAS_SEND_WND)
            // Offset + field length is the minimum struct size.
            size_t snd_wnd_reqd_size = offsetof(struct tcp_info, tcpi_snd_wnd) + sizeof(m_tcp_info.tcpi_snd_wnd);
            if (m_tcp_info_len >= snd_wnd_reqd_size) {
                peer_rwnd_bytes = m_tcp_info.tcpi_snd_wnd;
            }
        #endif
        // Unlike other platforms, on linux tcpi_snd_cwnd is reported in packets,
        // not bytes.
        uint64_t cwnd_bytes_temp = uint64_t(m_tcp_info.tcpi_snd_cwnd) * m_tcp_info.tcpi_snd_mss;
        if (cwnd_bytes_temp > UINT32_MAX) {
            // cwnd_bytes is u32, we overflowed.
            return 0;
        }
        cwnd_bytes = cwnd_bytes_temp;
/*
 * FreeBSD: Available since 6.0: https://cgit.freebsd.org/src/tree/sys/netinet/tcp.h?h=releng/6.0#n214 (commit: https://cgit.freebsd.org/src/commit/?id=b8af5dfa81f1fdca5ed77f8f339a5b522d10e714)
 * OSX: Available since 10.11: https://developer.apple.com/documentation/kernel/tcp_connection_info/1562043-tcpi_snd_wnd
 * NetBSD: Available since 10.2: https://github.com/NetBSD/src/blame/6cee2c5b88e06440257c4c8ad704f388954c9fb0/sys/netinet/tcp.h#L202
 * OpenBSD: Available since 7.2: https://cvsweb.openbsd.org/src/sys/netinet/tcp.h?rev=1.23&content-type=text/x-cvsweb-markup
 */
#elif defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__APPLE__)
        // congestion send window (# of segments) * mss (max segment size)
        cwnd_bytes = m_tcp_info.tcpi_snd_cwnd;
        peer_rwnd_bytes = m_tcp_info.tcpi_snd_wnd;
#elif defined(WIN32_TCPINFO_SUPPORTED)
        cwnd_bytes = m_tcp_info.Cwnd;
        peer_rwnd_bytes = m_tcp_info.SndWnd;
#endif
        if(peer_rwnd_bytes.has_value()) {
            return std::min(cwnd_bytes, peer_rwnd_bytes.value());
        } else {
            return cwnd_bytes;
        }
    }
};

#endif // BITCOIN_UTIL_SOCK_H
