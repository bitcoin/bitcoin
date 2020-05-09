// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <rpc/server.h>

#include <logging.h>
#include <rpc/interfaces.h>
#include <rpc/request.h>
#include <rpc/util.h>
#include <util/system.h>
#include <sync.h>

#include <boost/signals2/signal.hpp>

#include <atomic>
#include <memory> // for unique_ptr

extern CRPCTable tableRPC;
extern std::atomic<bool> g_rpc_running;
extern RecursiveMutex cs_rpcWarmup;
extern std::map<std::string, std::unique_ptr<RPCTimerBase> > deadlineTimers;

static struct CRPCSignals
{
    boost::signals2::signal<void ()> Started;
    boost::signals2::signal<void ()> Stopped;
} g_rpcSignals;

void RPCServer::OnStarted(std::function<void ()> slot)
{
    g_rpcSignals.Started.connect(slot);
}

void RPCServer::OnStopped(std::function<void ()> slot)
{
    g_rpcSignals.Stopped.connect(slot);
}

void StartRPC()
{
    LogPrint(BCLog::RPC, "Starting RPC\n");
    g_rpc_running = true;
    g_rpcSignals.Started();
}

void InterruptRPC()
{
    LogPrint(BCLog::RPC, "Interrupting RPC\n");
    // Interrupt e.g. running longpolls
    g_rpc_running = false;
}

void StopRPC()
{
    LogPrint(BCLog::RPC, "Stopping RPC\n");
    deadlineTimers.clear();
    DeleteAuthCookie();
    g_rpcSignals.Stopped();
}


