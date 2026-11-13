// Copyright (c) 2022-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NODE_COINS_VIEW_ARGS_H
#define BITCOIN_NODE_COINS_VIEW_ARGS_H

#include <util/result.h>

class ArgsManager;
struct CoinsViewOptions;

namespace node {
[[nodiscard]] util::Result<void> ReadCoinsViewArgs(const ArgsManager& args, CoinsViewOptions& options);
} // namespace node

#endif // BITCOIN_NODE_COINS_VIEW_ARGS_H
