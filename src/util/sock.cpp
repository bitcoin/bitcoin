// Copyright (c) 2020-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <compat/compat.h>
#include <logging.h>
#include <threadinterrupt.h>
#include <tinyformat.h>
#include <util/sock.h>
#include <util/syserror.h>
#include <util/system.h>
#include <util/time.h>

#include <memory>
#include <stdexcept>
#include <string>

#ifdef USE_POLL
#include <poll.h>
#endif

static inline bool IOErrorIsPermanent(int err)
{
    return err != WSAEAGAIN && err != WSAEINTR && err != WSAEWOULDBLOCK && err != WSAEINPROGRESS;
}

Sock::Sock() : m_socket(INVALID_SOCKET) {}

Sock::Sock(SOCKET s) : m_socket(s) {}

Sock::Sock(Sock&& other)
{
    m_socket = other.m_socket;
    other.m_socket = INVALID_SOCKET;
}

Sock::~Sock() { Close(); }

Sock& Sock::operator=(Sock&& other)
{
    Close();
    m_socket = other.m_socket;
    other.m_socket = INVALID_SOCKET;
    return *this;
}

SOCKET Sock::Get() const { return m_socket; }

ssize_t Sock::Send(const void* data, size_t len, int flags) const
{
    return send(m_socket, static_cast<const char*>(data), len, flags);
}

ssize_t Sock::Recv(void* buf, size_t len, int flags) const
{
    return recv(m_socket, static_cast<char*>(buf), len, flags);
}

int Sock::Connect(const sockaddr* addr, socklen_t addr_len) const
{
    return connect(m_socket, addr, addr_len);
}

int Sock::Bind(const sockaddr* addr, socklen_t addr_len) const
{
    return bind(m_socket, addr, addr_len);
}

int Sock::Listen(int backlog) const
{
    return listen(m_socket, backlog);
}

std::unique_ptr<Sock> Sock::Accept(sockaddr* addr, socklen_t* addr_len) const
{
#ifdef WIN32
    static constexpr auto ERR = INVALID_SOCKET;
#else
    static constexpr auto ERR = SOCKET_ERROR;
#endif

    std::unique_ptr<Sock> sock;

    const auto socket = accept(m_socket, addr, addr_len);
    if (socket != ERR) {
        try {
            sock = std::make_unique<Sock>(socket);
        } catch (const std::exception&) {
#ifdef WIN32
            closesocket(socket);
#else
            close(socket);
#endif
        }
    }

    return sock;
}

int Sock::GetSockOpt(int level, int opt_name, void* opt_val, socklen_t* opt_len) const
{
    return getsockopt(m_socket, level, opt_name, static_cast<char*>(opt_val), opt_len);
}

int Sock::SetSockOpt(int level, int opt_name, const void* opt_val, socklen_t opt_len) const
{
    return setsockopt(m_socket, level, opt_name, static_cast<const char*>(opt_val), opt_len);
}

int Sock::GetSockName(sockaddr* name, socklen_t* name_len) const
{
    return getsockname(m_socket, name, name_len);
}

bool Sock::SetNonBlocking() const
{
#ifdef WIN32
    u_long on{1};
    if (ioctlsocket(m_socket, FIONBIO, &on) == SOCKET_ERROR) {
        return false;
    }
#else
    const int flags{fcntl(m_socket, F_GETFL, 0)};
    if (flags == SOCKET_ERROR) {
        return false;
    }
    if (fcntl(m_socket, F_SETFL, flags | O_NONBLOCK) == SOCKET_ERROR) {
        return false;
    }
#endif
    return true;
}

bool Sock::IsSelectable() const
{
#if defined(USE_POLL) || defined(WIN32)
    return true;
#else
    return m_socket < FD_SETSIZE;
#endif
}

bool Sock::Wait(std::chrono::milliseconds timeout, Event requested, Event* occurred) const
{
#ifdef USE_POLL
    pollfd fd;
    fd.fd = m_socket;
    fd.events = 0;
    if (requested & RECV) {
        fd.events |= POLLIN;
    }
    if (requested & SEND) {
        fd.events |= POLLOUT;
    }

    if (poll(&fd, 1, count_milliseconds(timeout)) == SOCKET_ERROR) {
        return false;
    }

    if (occurred != nullptr) {
        *occurred = 0;
        if (fd.revents & POLLIN) {
            *occurred |= RECV;
        }
        if (fd.revents & POLLOUT) {
            *occurred |= SEND;
        }
        if (fd.revents & (POLLERR | POLLHUP)) {
            *occurred |= ERR;
        }
    }

    return true;
#else
    if (!IsSelectable()) {
        return false;
    }

    fd_set fdset_recv;
    fd_set fdset_send;
    fd_set fdset_err;
    FD_ZERO(&fdset_recv);
    FD_ZERO(&fdset_send);
    FD_ZERO(&fdset_err);

    if (requested & RECV) {
        FD_SET(m_socket, &fdset_recv);
    }

    if (requested & SEND) {
        FD_SET(m_socket, &fdset_send);
    }

    FD_SET(m_socket, &fdset_err);

    timeval timeout_struct = MillisToTimeval(timeout);

    if (select(m_socket + 1, &fdset_recv, &fdset_send, &fdset_err, &timeout_struct) == SOCKET_ERROR) {
        return false;
    }

    if (occurred != nullptr) {
        *occurred = 0;
        if (FD_ISSET(m_socket, &fdset_recv)) {
            *occurred |= RECV;
        }
        if (FD_ISSET(m_socket, &fdset_send)) {
            *occurred |= SEND;
        }
        if (FD_ISSET(m_socket, &fdset_err)) {
            *occurred |= ERR;
        }
    }

    return true;
#endif /* USE_POLL */
}

void Sock::SendComplete(const std::string& data,
                        std::chrono::milliseconds timeout,
                        CThreadInterrupt& interrupt) const
{
    const auto deadline = GetTime<std::chrono::milliseconds>() + timeout;
    size_t sent{0};

    for (;;) {
        const ssize_t ret{Send(data.data() + sent, data.size() - sent, MSG_NOSIGNAL)};

        if (ret > 0) {
            sent += static_cast<size_t>(ret);
            if (sent == data.size()) {
                break;
            }
        } else {
            const int err{WSAGetLastError()};
            if (IOErrorIsPermanent(err)) {
                throw std::runtime_error(strprintf("send(): %s", NetworkErrorString(err)));
            }
        }

        const auto now = GetTime<std::chrono::milliseconds>();

        if (now >= deadline) {
            throw std::runtime_error(strprintf(
                "Send timeout (sent only %u of %u bytes before that)", sent, data.size()));
        }

        if (interrupt) {
            throw std::runtime_error(strprintf(
                "Send interrupted (sent only %u of %u bytes before that)", sent, data.size()));
        }

        // Wait for a short while (or the socket to become ready for sending) before retrying
        // if nothing was sent.
        const auto wait_time = std::min(deadline - now, std::chrono::milliseconds{MAX_WAIT_FOR_IO});
        (void)Wait(wait_time, SEND);
    }
}

std::string Sock::RecvUntilTerminator(uint8_t terminator,
                                      std::chrono::milliseconds timeout,
                                      CThreadInterrupt& interrupt,
                                      size_t max_data) const
{
    const auto deadline = GetTime<std::chrono::milliseconds>() + timeout;
    std::string data;
    bool terminator_found{false};

    // We must not consume any bytes past the terminator from the socket.
    // One option is to read one byte at a time and check if we have read a terminator.
    // However that is very slow. Instead, we peek at what is in the socket and only read
    // as many bytes as possible without crossing the terminator.
    // Reading 64 MiB of random data with 262526 terminator chars takes 37 seconds to read
    // one byte at a time VS 0.71 seconds with the "peek" solution below. Reading one byte
    // at a time is about 50 times slower.

    for (;;) {
        if (data.size() >= max_data) {
            throw std::runtime_error(
                strprintf("Received too many bytes without a terminator (%u)", data.size()));
        }

        char buf[512];

        const ssize_t peek_ret{Recv(buf, std::min(sizeof(buf), max_data - data.size()), MSG_PEEK)};

        switch (peek_ret) {
        case -1: {
            const int err{WSAGetLastError()};
            if (IOErrorIsPermanent(err)) {
                throw std::runtime_error(strprintf("recv(): %s", NetworkErrorString(err)));
            }
            break;
        }
        case 0:
            throw std::runtime_error("Connection unexpectedly closed by peer");
        default:
            auto end = buf + peek_ret;
            auto terminator_pos = std::find(buf, end, terminator);
            terminator_found = terminator_pos != end;

            const size_t try_len{terminator_found ? terminator_pos - buf + 1 :
                                                    static_cast<size_t>(peek_ret)};

            const ssize_t read_ret{Recv(buf, try_len, 0)};

            if (read_ret < 0 || static_cast<size_t>(read_ret) != try_len) {
                throw std::runtime_error(
                    strprintf("recv() returned %u bytes on attempt to read %u bytes but previous "
                              "peek claimed %u bytes are available",
                              read_ret, try_len, peek_ret));
            }

            // Don't include the terminator in the output.
            const size_t append_len{terminator_found ? try_len - 1 : try_len};

            data.append(buf, buf + append_len);

            if (terminator_found) {
                return data;
            }
        }

        const auto now = GetTime<std::chrono::milliseconds>();

        if (now >= deadline) {
            throw std::runtime_error(strprintf(
                "Receive timeout (received %u bytes without terminator before that)", data.size()));
        }

        if (interrupt) {
            throw std::runtime_error(strprintf(
                "Receive interrupted (received %u bytes without terminator before that)",
                data.size()));
        }

        // Wait for a short while (or the socket to become ready for reading) before retrying.
        const auto wait_time = std::min(deadline - now, std::chrono::milliseconds{MAX_WAIT_FOR_IO});
        (void)Wait(wait_time, RECV);
    }
}

bool Sock::IsConnected(std::string& errmsg) const
{
    if (m_socket == INVALID_SOCKET) {
        errmsg = "not connected";
        return false;
    }

    char c;
    switch (Recv(&c, sizeof(c), MSG_PEEK)) {
    case -1: {
        const int err = WSAGetLastError();
        if (IOErrorIsPermanent(err)) {
            errmsg = NetworkErrorString(err);
            return false;
        }
        return true;
    }
    case 0:
        errmsg = "closed";
        return false;
    default:
        return true;
    }
}

void Sock::Close()
{
    if (m_socket == INVALID_SOCKET) {
        return;
    }
#ifdef WIN32
    int ret = closesocket(m_socket);
#else
    int ret = close(m_socket);
#endif
    if (ret) {
        LogPrintf("Error closing socket %d: %s\n", m_socket, NetworkErrorString(WSAGetLastError()));
    }
    m_socket = INVALID_SOCKET;
}

std::string NetworkErrorString(int err)
{
#if defined(WIN32)
    return Win32ErrorString(err);
#else
    // On BSD sockets implementations, NetworkErrorString is the same as SysErrorString.
    return SysErrorString(err);
#endif
}
