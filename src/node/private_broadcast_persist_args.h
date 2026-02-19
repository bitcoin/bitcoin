// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NODE_PRIVATE_BROADCAST_PERSIST_ARGS_H
#define BITCOIN_NODE_PRIVATE_BROADCAST_PERSIST_ARGS_H

#include <util/fs.h>

class ArgsManager;

namespace node {

/** Location of the private broadcast persistence file. */
fs::path PrivateBroadcastPath(const ArgsManager& argsman);

} // namespace node

#endif // BITCOIN_NODE_PRIVATE_BROADCAST_PERSIST_ARGS_H
