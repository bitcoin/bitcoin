// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef MP_TEST_SOCKETLISTENER_H
#define MP_TEST_SOCKETLISTENER_H

#include <mp/util.h>

#include <cassert>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <kj/debug.h>
#include <string>
#include <system_error>
#include <variant>

#ifdef WIN32
#include <afunix.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/un.h>
#endif

#ifdef WIN32
// Ensure WSAStartup is called before any SocketListener operation. Winsock
// requires WSAStartup before any socket call; the mp library calls it inside
// StartSpawned(), but test files that create sockets directly never reach that
// code path. WSACleanup is intentionally omitted: the OS reclaims Winsock
// state on exit. TODO: check the return value and fail fast on error.
namespace {
struct WsaInit {
    WsaInit() { WSADATA data; WSAStartup(MAKEWORD(2, 2), &data); }
} g_wsa_init;
} // namespace
#endif

namespace mp {
namespace test {

//! Owns a temporary listening socket used by tests. Tests call
//! MakeConnectedSocket() to create client socket FDs and release() to transfer
//! ownership of the listening FD.
class SocketListener
{
public:
    SocketListener()
    {
        // Use TCP on Windows to work around Wine's incompatibility with
        // AF_UNIX: Wine does not support the AcceptEx extension used by KJ's
        // AF_UNIX listener
        // (https://gitlab.winehq.org/wine/wine/-/merge_requests/7650).
        // AF_UNIX sockets work fine on real Windows. It could make sense
        // later to test TCP connections on Unix as well.
#ifdef WIN32
        m_addr.emplace<sockaddr_in>();
#else
        m_addr.emplace<sockaddr_un>();
#endif
        std::visit([this](auto& addr) { Init(addr); }, m_addr);
    }

    ~SocketListener()
    {
        if (m_fd != SocketError) mp::CloseSocket(m_fd);
        if (auto* un = std::get_if<sockaddr_un>(&m_addr)) {
            std::error_code ec;
            if (un->sun_path[0]) std::filesystem::remove(un->sun_path, ec);
            if (!m_dir.empty()) std::filesystem::remove(m_dir, ec);
        }
    }

    SocketId release()
    {
        assert(m_fd != SocketError);
        SocketId fd = m_fd;
        m_fd = SocketError;
        return fd;
    }

    SocketId MakeConnectedSocket() const
    {
        return std::visit([](const auto& addr) { return Connect(addr); }, m_addr);
    }

private:
    void Init(sockaddr_in& addr)
    {
        m_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        KJ_REQUIRE(m_fd != SocketError);

        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        addr.sin_port = 0;
        KJ_REQUIRE(bind(m_fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == 0);
        KJ_REQUIRE(listen(m_fd, SOMAXCONN) == 0);

        socklen_t len = sizeof(addr);
        KJ_REQUIRE(getsockname(m_fd, reinterpret_cast<sockaddr*>(&addr), &len) == 0);
    }

    void Init(sockaddr_un& addr)
    {
        auto base = std::filesystem::temp_directory_path();
        auto now = std::chrono::steady_clock::now().time_since_epoch().count();
        for (unsigned attempt = 0; ; ++attempt) {
            auto path = base / ("mptest-listener-" + std::to_string(now) + std::to_string(attempt));
            if (std::filesystem::create_directory(path)) {
                m_dir = path.string();
                break;
            }
        }
        std::string path = m_dir + "/socket";

        m_fd = socket(AF_UNIX, SOCK_STREAM, 0);
        KJ_REQUIRE(m_fd != SocketError);

        addr.sun_family = AF_UNIX;
        KJ_REQUIRE(path.size() < sizeof(addr.sun_path));
        std::strncpy(addr.sun_path, path.c_str(), sizeof(addr.sun_path) - 1);
        KJ_REQUIRE(bind(m_fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == 0);
        KJ_REQUIRE(listen(m_fd, SOMAXCONN) == 0);
    }

    static SocketId Connect(const sockaddr_in& addr)
    {
        SocketId fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        KJ_REQUIRE(fd != SocketError);

        sockaddr_in a = addr;
        KJ_REQUIRE(connect(fd, reinterpret_cast<sockaddr*>(&a), sizeof(a)) == 0);
        return fd;
    }

    static SocketId Connect(const sockaddr_un& addr)
    {
        SocketId fd = socket(AF_UNIX, SOCK_STREAM, 0);
        KJ_REQUIRE(fd != SocketError);

        sockaddr_un a = addr;
        KJ_REQUIRE(connect(fd, reinterpret_cast<sockaddr*>(&a), sizeof(a)) == 0);
        return fd;
    }

    SocketId m_fd{SocketError};
    std::string m_dir;
    std::variant<sockaddr_in, sockaddr_un> m_addr;
};

} // namespace test
} // namespace mp

#endif // MP_TEST_SOCKETLISTENER_H
