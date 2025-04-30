// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_IPC_UTIL_H
#define BITCOIN_IPC_UTIL_H

#include <cstdint>
#include <mp/util.h>
#include <mp/version.h>

namespace mp {
// Definitions that can be deleted when libmultiprocess subtree is updated to
// v14. Having these allows Bitcoin Core changes to be decoupled from
// libmultiprocess changes so they don't have to be reviewed in a single PR.
class EventLoop;

using Stream = SocketId;
inline Stream MakeStream(EventLoop&, SocketId socket)
{
    return socket;
}
} // namespace mp

#endif // BITCOIN_IPC_UTIL_H
