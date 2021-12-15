// Copyright (c) 2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <interfaces/chain.h>
#include <interfaces/echo.h>
#include <interfaces/init.h>
#include <interfaces/node.h>
#include <interfaces/wallet.h>

namespace interfaces {
std::unique_ptr<Node> Init::makeNode() { return {}; }
std::unique_ptr<Chain> Init::makeChain() { return {}; }
std::unique_ptr<WalletClient> Init::makeWalletClient(Chain& chain) { return {}; }
std::unique_ptr<Echo> Init::makeEcho() { return {}; }
Ipc* Init::ipc() { return nullptr; }
} // namespace interfaces
