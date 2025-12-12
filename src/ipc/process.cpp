// Copyright (c) 2021-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <ipc/process.h>
#include <ipc/protocol.h>
#include <logging.h>
#include <mp/util.h>
#include <tinyformat.h>
#include <util/fs.h>
#include <util/strencodings.h>
#include <util/syserror.h>

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <utility>
#include <vector>

#ifdef WIN32
#include <afunix.h>
#else
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#define closesocket close
#endif

using util::RemovePrefixView;

namespace ipc {
namespace {
class ProcessImpl : public Process
{
public:
    mp::ProcessId spawn(mp::SocketId socket, const std::string& new_exe_name, const fs::path& argv0_path) override
    {
        return mp::SpawnProcess(socket, [&](mp::ConnectInfo info) {
            fs::path path = argv0_path;
            path.remove_filename();
            path /= fs::PathFromString(new_exe_name);
            return std::vector<std::string>{fs::PathToString(path), "-ipcfd", std::move(info)};
        });
    }
    int waitSpawned(mp::ProcessId pid) override { return mp::WaitProcess(pid); }
    bool checkSpawned(int argc, char* argv[], mp::SocketId& socket) override
    {
        // If this process was not started with a single -ipcfd argument, it is
        // not a process spawned by the spawn() call above, so return false and
        // do not try to serve requests.
        if (argc != 3 || strcmp(argv[1], "-ipcfd") != 0) {
            return false;
        }
        // If a single -ipcfd argument was provided, return true and get the
        // file descriptor so Protocol::serve() can be called to handle
        // requests from the parent process. The -ipcfd argument is not valid
        // in combination with other arguments because the parent process
        // should be able to control the child process through the IPC protocol
        // without passing information out of band.
        try {
           socket = mp::StartSpawned(argv[2]);
        } catch (const std::exception& e) {
           throw std::runtime_error(strprintf("Invalid -ipcfd number '%s' (%s)", argv[2], e.what()));
        }
        return true;
    }
    mp::SocketId connect(const fs::path& data_dir,
                const std::string& dest_exe_name,
                std::string& address) override;
    mp::SocketId bind(const fs::path& data_dir, const std::string& exe_name, std::string& address) override;
};

static bool ParseAddress(std::string& address,
                  const fs::path& data_dir,
                  const std::string& dest_exe_name,
                  struct sockaddr_un& addr,
                  std::string& error)
{
    if (address == "unix" || address.starts_with("unix:")) {
        fs::path path;
        if (address.size() <= 5) {
            path = data_dir / fs::PathFromString(strprintf("%s.sock", RemovePrefixView(dest_exe_name, "bitcoin-")));
        } else {
            path = data_dir / fs::PathFromString(address.substr(5));
        }
        std::string path_str = fs::PathToString(path);
        address = strprintf("unix:%s", path_str);
        if (path_str.size() >= sizeof(addr.sun_path)) {
            error = strprintf("Unix address path %s exceeded maximum socket path length", fs::quoted(fs::PathToString(path)));
            return false;
        }
        memset(&addr, 0, sizeof(addr));
        addr.sun_family = AF_UNIX;
        strncpy(addr.sun_path, path_str.c_str(), sizeof(addr.sun_path)-1);
        return true;
    }

    error = strprintf("Unrecognized address '%s'", address);
    return false;
}

mp::SocketId ProcessImpl::connect(const fs::path& data_dir,
                         const std::string& dest_exe_name,
                         std::string& address)
{
    struct sockaddr_un addr;
    std::string error;
    if (!ParseAddress(address, data_dir, dest_exe_name, addr, error)) {
        throw std::invalid_argument(error);
    }

    mp::SocketId fd;
    if ((fd = ::socket(addr.sun_family, SOCK_STREAM, 0)) == mp::SocketError) {
        throw std::system_error(errno, std::system_category());
    }
    if (::connect(fd, (struct sockaddr*)&addr, sizeof(addr)) == 0) {
        return fd;
    }
    int connect_error = errno;
    if (::closesocket(fd) != 0) {
        LogWarning("Error closing file descriptor %i '%s': %s", fd, address, SysErrorString(errno));
    }
    throw std::system_error(connect_error, std::system_category());
}

mp::SocketId ProcessImpl::bind(const fs::path& data_dir, const std::string& exe_name, std::string& address)
{
    struct sockaddr_un addr;
    std::string error;
    if (!ParseAddress(address, data_dir, exe_name, addr, error)) {
        throw std::invalid_argument(error);
    }

    if (addr.sun_family == AF_UNIX) {
        fs::path path = addr.sun_path;
        if (path.has_parent_path()) fs::create_directories(path.parent_path());
        if (fs::symlink_status(path).type() == fs::file_type::socket
#ifdef WIN32
            // On windows, sockets show up as regular files with size 0
            || (fs::is_regular_file(path) && fs::file_size(path) == 0)
#endif
        ) {
            fs::remove(path);
        }
    }

    mp::SocketId fd;
    if ((fd = ::socket(addr.sun_family, SOCK_STREAM, 0)) == mp::SocketError) {
        throw std::system_error(errno, std::system_category());
    }

    if (::bind(fd, (struct sockaddr*)&addr, sizeof(addr)) == 0) {
        return fd;
    }
    int bind_error = errno;
    if (::closesocket(fd) != 0) {
        LogWarning("Error closing file descriptor %i: %s", fd, SysErrorString(errno));
    }
    throw std::system_error(bind_error, std::system_category());
}
} // namespace

std::unique_ptr<Process> MakeProcess() { return std::make_unique<ProcessImpl>(); }
} // namespace ipc
