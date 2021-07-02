// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <shutdown.h>

#include <atomic>

static std::atomic<bool> fRequestShutdown(false);
static std::atomic<bool> fRequestRestart(false);

void StartShutdown()
{
    fRequestShutdown = true;
}
void StartRestart()
{
    fRequestShutdown = fRequestRestart = true;
}
void AbortShutdown()
{
    fRequestShutdown = false;
}
bool ShutdownRequested()
{
    return fRequestShutdown;
}
bool RestartRequested()
{
    return fRequestRestart;
}
