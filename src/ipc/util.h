// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_IPC_UTIL_H
#define BITCOIN_IPC_UTIL_H

#include <tinyformat.h>
#include <util/strencodings.h>

#include <array>
#include <cstdint>
#include <functional>
#include <kj/async-io.h>
#include <kj/debug.h>
#include <mp/util.h>
#include <mp/version.h>
#include <sys/socket.h>

namespace mp {
// Definitions that can be deleted when libmultiprocess subtree is updated to
// v12. Having these allows Bitcoin Core changes to be decoupled from
// libmultiprocess changes so they don't have to be reviewed in a single PR.
#if MP_MAJOR_VERSION < 12
using ProcessId = int;
using SocketId = int;
constexpr SocketId SocketError{-1};

inline std::array<SocketId, 2> SocketPair()
{
    int pair[2];
    KJ_SYSCALL(socketpair(AF_UNIX, SOCK_STREAM, 0, pair));
    return {pair[0], pair[1]};
}

using Stream = SocketId;
inline Stream MakeStream(kj::AsyncIoContext& io_context, SocketId socket)
{
    return socket;
}

using ConnectInfo = std::string;
inline SocketId StartSpawned(const ConnectInfo& connect_info)
{
    auto socket = ToIntegral<SocketId>(connect_info);
    if (!socket) throw std::invalid_argument(strprintf("Invalid socket descriptor '%s'", connect_info));
    return *socket;
}

using ConnectInfoToArgsFn = std::function<std::vector<std::string>(const ConnectInfo&)>;
inline std::tuple<ProcessId, SocketId> SpawnProcess(ConnectInfoToArgsFn&& connect_info_to_args)
{
    ProcessId pid;
    SocketId socket = SpawnProcess(pid, [&](int fd) { return connect_info_to_args(strprintf("%d", fd)); });
    return {pid, socket};
}
#endif
} // namespace mp

#endif // BITCOIN_IPC_UTIL_H
