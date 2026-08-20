// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef MP_TEST_UNIXLISTENER_H
#define MP_TEST_UNIXLISTENER_H

#include <kj/test.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <string>

namespace mp {
namespace test {

//! Owns a temporary Unix-domain listening socket. Tests call
//! MakeConnectedSocket() to create client socket FDs and release() to transfer
//! ownership of the listening FD.
class UnixListener
{
public:
    UnixListener()
    {
        std::string dir_template = (std::filesystem::temp_directory_path() / "mptest-listener-XXXXXX").string();
        char* dir = mkdtemp(dir_template.data());
        KJ_REQUIRE(dir != nullptr);
        m_dir = dir;
        m_path = m_dir + "/socket";

        m_fd = socket(AF_UNIX, SOCK_STREAM, 0);
        KJ_REQUIRE(m_fd >= 0);

        sockaddr_un addr{};
        addr.sun_family = AF_UNIX;
        KJ_REQUIRE(m_path.size() < sizeof(addr.sun_path));
        std::strncpy(addr.sun_path, m_path.c_str(), sizeof(addr.sun_path) - 1);
        KJ_REQUIRE(bind(m_fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == 0);
        KJ_REQUIRE(listen(m_fd, SOMAXCONN) == 0);
    }

    ~UnixListener()
    {
        if (m_fd >= 0) close(m_fd);
        if (!m_path.empty()) unlink(m_path.c_str());
        if (!m_dir.empty()) rmdir(m_dir.c_str());
    }

    int release()
    {
        assert(m_fd >= 0);
        int fd = m_fd;
        m_fd = -1;
        return fd;
    }

    int MakeConnectedSocket() const
    {
        int fd = socket(AF_UNIX, SOCK_STREAM, 0);
        KJ_REQUIRE(fd >= 0);

        sockaddr_un addr{};
        addr.sun_family = AF_UNIX;
        KJ_REQUIRE(m_path.size() < sizeof(addr.sun_path));
        std::strncpy(addr.sun_path, m_path.c_str(), sizeof(addr.sun_path) - 1);
        KJ_REQUIRE(connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == 0);
        return fd;
    }

private:
    int m_fd{-1};
    std::string m_dir;
    std::string m_path;
};

} // namespace test
} // namespace mp

#endif // MP_TEST_UNIXLISTENER_H
