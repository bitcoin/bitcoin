// Copyright (c) 2022 The Tortoisecoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef TORTOISECOIN_NODE_COINS_VIEW_ARGS_H
#define TORTOISECOIN_NODE_COINS_VIEW_ARGS_H

class ArgsManager;
struct CoinsViewOptions;

namespace node {
void ReadCoinsViewArgs(const ArgsManager& args, CoinsViewOptions& options);
} // namespace node

#endif // TORTOISECOIN_NODE_COINS_VIEW_ARGS_H
