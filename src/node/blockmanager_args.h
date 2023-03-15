
// Copyright (c) 2023 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SYSCOIN_NODE_BLOCKMANAGER_ARGS_H
#define SYSCOIN_NODE_BLOCKMANAGER_ARGS_H

#include <node/blockstorage.h>

#include <optional>

class ArgsManager;
struct bilingual_str;

namespace node {
std::optional<bilingual_str> ApplyArgsManOptions(const ArgsManager& args, BlockManager::Options& opts);
} // namespace node

#endif // SYSCOIN_NODE_BLOCKMANAGER_ARGS_H
