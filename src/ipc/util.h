// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_IPC_UTIL_H
#define BITCOIN_IPC_UTIL_H

#include <array>
#include <cstdint>
#include <kj/debug.h>
#include <mp/util.h>
#include <mp/version.h>
#include <sys/socket.h>

namespace mp {
// Definitions that can be deleted when libmultiprocess subtree is updated to
// v14. Having these allows Bitcoin Core changes to be decoupled from
// libmultiprocess changes so they don't have to be reviewed in a single PR.
#if MP_MAJOR_VERSION < 14
class EventLoop;
using ProcessId = int;
using SocketId = int;
constexpr SocketId SocketError{-1};

using Stream = SocketId;
inline Stream MakeStream(EventLoop&, SocketId socket)
{
    return socket;
}

inline std::array<SocketId, 2> SocketPair()
{
    int pair[2];
    KJ_SYSCALL(socketpair(AF_UNIX, SOCK_STREAM, 0, pair));
    return {pair[0], pair[1]};
}
#endif
} // namespace mp

#endif // BITCOIN_IPC_UTIL_H
