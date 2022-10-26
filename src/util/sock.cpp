// Copyright (c) 2020-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <compat.h>
#include <logging.h>
#include <tinyformat.h>
#include <util/sock.h>
#include <util/system.h>
#include <util/time.h>

#include <codecvt>
#include <cwchar>
#include <locale>
#include <string>

#ifdef USE_POLL
#include <poll.h>
#endif

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

bool Sock::Wait(std::chrono::milliseconds timeout, Event requested) const
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

    return poll(&fd, 1, count_milliseconds(timeout)) != SOCKET_ERROR;
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

    return select(m_socket + 1, &fdset_recv, &fdset_send, nullptr, &timeout_struct) != SOCKET_ERROR;
#endif /* USE_POLL */
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
