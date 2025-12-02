// Copyright (c) 2021-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_INTERFACES_INIT_H
#define BITCOIN_INTERFACES_INIT_H

#include <interfaces/chain.h>
#include <interfaces/echo.h>
#include <interfaces/mining.h>
#include <interfaces/node.h>
#include <interfaces/wallet.h>

#include <memory>

namespace node {
struct NodeContext;
} // namespace node

namespace interfaces {
class Ipc;

//! Initial interface created when a process is first started, and used to give
//! and get access to other interfaces (Node, Chain, Wallet, etc).
//!
//! There is a different Init interface implementation for each process
//! (bitcoin-gui, bitcoin-node, bitcoin-wallet, bitcoind, bitcoin-qt) and each
//! implementation can implement the make methods for interfaces it supports.
//! The default make methods all return null.
class Init
{
public:
    virtual ~Init() = default;
    virtual std::unique_ptr<Node> makeNode() { return nullptr; }
    virtual std::unique_ptr<Chain> makeChain() { return nullptr; }
    virtual std::unique_ptr<Mining> makeMining() { return nullptr; }
    virtual std::unique_ptr<WalletLoader> makeWalletLoader(Chain& chain) { return nullptr; }
    virtual std::unique_ptr<Echo> makeEcho() { return nullptr; }
    virtual Ipc* ipc() { return nullptr; }
    virtual bool canListenIpc() { return false; }
    virtual const char* exeName() { return nullptr; }
};

//! Return implementation of Init interface for the node process. If the argv
//! indicates that this is a child process spawned to handle requests from a
//! parent process, this blocks and handles requests, then returns null and a
//! status code to exit with. If this returns non-null, the caller can start up
//! normally and use the Init object to spawn and connect to other processes
//! while it is running.
std::unique_ptr<Init> MakeNodeInit(node::NodeContext& node, int argc, char* argv[], int& exit_status);

//! Return implementation of Init interface for the wallet process.
std::unique_ptr<Init> MakeWalletInit(int argc, char* argv[], int& exit_status);

//! Return implementation of Init interface for the gui process.
std::unique_ptr<Init> MakeGuiInit(int argc, char* argv[]);
} // namespace interfaces

#endif // BITCOIN_INTERFACES_INIT_H
