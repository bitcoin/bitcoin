// Copyright (c) 2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <common/interfaces/chain.h>
#include <common/interfaces/echo.h>
#include <common/interfaces/init.h>
#include <common/interfaces/node.h>
#include <common/interfaces/wallet.h>

namespace interfaces {
std::unique_ptr<Node> Init::makeNode() { return {}; }
std::unique_ptr<Chain> Init::makeChain() { return {}; }
std::unique_ptr<WalletLoader> Init::makeWalletLoader(Chain& chain) { return {}; }
std::unique_ptr<Echo> Init::makeEcho() { return {}; }
Ipc* Init::ipc() { return nullptr; }
} // namespace interfaces
