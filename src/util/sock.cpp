// Copyright (c) 2020-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <compat.h>
#include <logging.h>
#include <threadinterrupt.h>
#include <tinyformat.h>
#include <util/sock.h>
#include <util/system.h>
#include <util/time.h>

#include <codecvt>
#include <cwchar>
#include <locale>
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

Sock::~Sock() { Reset(); }

Sock& Sock::operator=(Sock&& other)
{
    Reset();
    m_socket = other.m_socket;
    other.m_socket = INVALID_SOCKET;
    return *this;
}

SOCKET Sock::Get() const { return m_socket; }

SOCKET Sock::Release()
{
    const SOCKET s = m_socket;
    m_socket = INVALID_SOCKET;
    return s;
}

void Sock::Reset() { CloseSocket(m_socket); }

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

int Sock::GetSockOpt(int level, int opt_name, void* opt_val, socklen_t* opt_len) const
{
    return getsockopt(m_socket, level, opt_name, static_cast<char*>(opt_val), opt_len);
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
    }

    return true;
#else
    if (!IsSelectableSocket(m_socket)) {
        return false;
    }

    fd_set fdset_recv;
    fd_set fdset_send;
    FD_ZERO(&fdset_recv);
    FD_ZERO(&fdset_send);

    if (requested & RECV) {
        FD_SET(m_socket, &fdset_recv);
    }

    if (requested & SEND) {
        FD_SET(m_socket, &fdset_send);
    }

    timeval timeout_struct = MillisToTimeval(timeout);

    if (select(m_socket + 1, &fdset_recv, &fdset_send, nullptr, &timeout_struct) == SOCKET_ERROR) {
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

#ifdef WIN32
std::string NetworkErrorString(int err)
{
    wchar_t buf[256];
    buf[0] = 0;
    if(FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_MAX_WIDTH_MASK,
            nullptr, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            buf, ARRAYSIZE(buf), nullptr))
    {
        return strprintf("%s (%d)", std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>,wchar_t>().to_bytes(buf), err);
    }
    else
    {
        return strprintf("Unknown error (%d)", err);
    }
}
#else
std::string NetworkErrorString(int err)
{
    char buf[256];
    buf[0] = 0;
    /* Too bad there are two incompatible implementations of the
     * thread-safe strerror. */
    const char *s;
#ifdef STRERROR_R_CHAR_P /* GNU variant can return a pointer outside the passed buffer */
    s = strerror_r(err, buf, sizeof(buf));
#else /* POSIX variant always returns message in buffer */
    s = buf;
    if (strerror_r(err, buf, sizeof(buf)))
        buf[0] = 0;
#endif
    return strprintf("%s (%d)", s, err);
}
#endif

bool CloseSocket(SOCKET& hSocket)
{
    if (hSocket == INVALID_SOCKET)
        return false;
#ifdef WIN32
    int ret = closesocket(hSocket);
#else
    int ret = close(hSocket);
#endif
    if (ret) {
        LogPrintf("Socket close failed: %d. Error: %s\n", hSocket, NetworkErrorString(WSAGetLastError()));
    }
    hSocket = INVALID_SOCKET;
    return ret != SOCKET_ERROR;
}
