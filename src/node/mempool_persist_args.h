// Copyright (c) 2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NODE_MEMPOOL_PERSIST_ARGS_H
#define BITCOIN_NODE_MEMPOOL_PERSIST_ARGS_H

#include <fs.h>

class ArgsManager;

namespace node {

bool ShouldPersistMempool(const ArgsManager& argsman);
fs::path MempoolPath(const ArgsManager& argsman);

} // namespace node

#endif // BITCOIN_NODE_MEMPOOL_PERSIST_ARGS_H
