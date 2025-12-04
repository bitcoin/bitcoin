// Copyright (c) 2026 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NODE_MINING_ARGS_H
#define BITCOIN_NODE_MINING_ARGS_H

#include <util/result.h>

class ArgsManager;

namespace node {

[[nodiscard]] util::Result<void> ApplyArgsManOptions(const ArgsManager& args);

} // namespace node

#endif // BITCOIN_NODE_MINING_ARGS_H
