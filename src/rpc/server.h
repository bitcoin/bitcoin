// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_RPC_SERVER_H
#define BITCOIN_RPC_SERVER_H

#include <rpc/interfaces.h>

#include <functional>

namespace RPCServer
{
    void OnStarted(std::function<void ()> slot);
    void OnStopped(std::function<void ()> slot);
}

void StartRPC();
void InterruptRPC();
void StopRPC();

#endif // BITCOIN_RPC_SERVER_H
