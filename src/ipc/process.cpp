// Copyright (c) 2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <fs.h>
#include <ipc/process.h>
#include <ipc/protocol.h>
#include <logging.h>
#include <mp/util.h>
#include <tinyformat.h>
#include <util/strencodings.h>

#include <cstdint>
#include <errno.h>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <system_error>
#include <unistd.h>
#include <utility>
#include <vector>

namespace ipc {
namespace {
class ProcessImpl : public Process
{
public:
    int spawn(const std::string& new_exe_name, const fs::path& argv0_path, int& pid) override
    {
        return mp::SpawnProcess(pid, [&](int fd) {
            fs::path path = argv0_path;
            path.remove_filename();
            path.append(new_exe_name);
            return std::vector<std::string>{path.string(), "-ipcfd", strprintf("%i", fd)};
        });
    }
    int waitSpawned(int pid) override { return mp::WaitProcess(pid); }
    bool checkSpawned(int argc, char* argv[], int& fd) override
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
        if (!ParseInt32(argv[2], &fd)) {
            throw std::runtime_error(strprintf("Invalid -ipcfd number '%s'", argv[2]));
        }
        return true;
    }
    int connect(const fs::path& data_dir,
                const std::string& dest_exe_name,
                std::string& address,
                std::string& error) override;
    int bind(const fs::path& data_dir, const std::string& exe_name, std::string& address, std::string& error) override;
};

bool ParseAddress(std::string& address,
                  const fs::path& data_dir,
                  const std::string& dest_exe_name,
                  struct sockaddr_un& addr,
                  std::string& error)
{
    if (address.compare(0, 4, "unix") == 0 && (address.size() == 4 || address[4] == ':')) {
        fs::path socket_dir = data_dir / "sockets";
        fs::path path;
        if (address.size() <= 5) {
            path = socket_dir / strprintf("%s.sock", dest_exe_name);
        } else {
            path = fs::absolute(address.substr(5), socket_dir);
        }
        std::string path_str = path.string();
        if (path_str.size() >= sizeof(addr.sun_path)) {
            error = strprintf("Address '%s' path '%s' exceeded maximum socket path length", address, path);
            return false;
        }
        memset(&addr, 0, sizeof(addr));
        addr.sun_family = AF_UNIX;
        strncpy(addr.sun_path, path.string().c_str(), sizeof(addr.sun_path));
        address = strprintf("unix:%s", path_str);
        return true;
    }

    error = strprintf("Unrecognized address '%s'", address);
    return false;
}

int ProcessImpl::connect(const fs::path& data_dir,
                         const std::string& dest_exe_name,
                         std::string& address,
                         std::string& error)
{
    struct sockaddr_un addr;
    if (!ParseAddress(address, data_dir, dest_exe_name, addr, error)) {
        return -1;
    }

    int fd;
    if ((fd = ::socket(addr.sun_family, SOCK_STREAM, 0)) == -1) {
        throw std::system_error(errno, std::system_category());
    }
    if (::connect(fd, (struct sockaddr*)&addr, sizeof(addr)) == 0) {
        return fd;
    }
    int connect_error = errno;
    if (::close(fd) != 0) {
        LogPrintf("Error closing file descriptor %i '%s': %s\n", fd, address, strerror(errno));
    }
    if (connect_error == ECONNREFUSED || connect_error == ENOENT) {
        error = strprintf("Error connecting to '%s': %s\n", address, strerror(connect_error));
        return -1;
    }
    LogPrintf("Error connecting to '%s': %s\n", address, strerror(connect_error));
    throw std::system_error(connect_error, std::system_category());
}

int ProcessImpl::bind(const fs::path& data_dir, const std::string& exe_name, std::string& address, std::string& error)
{
    struct sockaddr_un addr;
    if (!ParseAddress(address, data_dir, exe_name, addr, error)) {
        return -1;
    }

    if (addr.sun_family == AF_UNIX) {
        fs::path path = addr.sun_path;
        fs::create_directories(path.parent_path());
        if (fs::symlink_status(path).type() == fs::socket_file) {
            fs::remove(path);
        }
    }

    int fd;
    if ((fd = ::socket(addr.sun_family, SOCK_STREAM, 0)) == -1) {
        throw std::system_error(errno, std::system_category());
    }

    if (::bind(fd, (struct sockaddr*)&addr, sizeof(addr)) == 0) {
        return fd;
    }
    int bind_error = errno;
    if (::close(fd) != 0) {
        LogPrintf("Error closing file descriptor %i: %s\n", fd, strerror(errno));
    }
    throw std::system_error(bind_error, std::system_category());
}
} // namespace

std::unique_ptr<Process> MakeProcess() { return std::make_unique<ProcessImpl>(); }
} // namespace ipc
