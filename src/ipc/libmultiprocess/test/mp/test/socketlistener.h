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

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/un.h>

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
        // Currently only AF_UNIX sockets are supported. TCP (sockaddr_in) will
        // be added later.
        m_addr.emplace<sockaddr_un>();
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
    std::variant<sockaddr_un> m_addr;
};

} // namespace test
} // namespace mp

#endif // MP_TEST_SOCKETLISTENER_H
