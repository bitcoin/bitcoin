// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <shutdown.h>

#include <atomic>
#include <veriblock/context.hpp>

static std::atomic<bool> fRequestShutdown(false);

void StartShutdown()
{
    fRequestShutdown = true;
    altintegration::RequestShutdown();
}
void AbortShutdown()
{
    fRequestShutdown = false;
}
bool ShutdownRequested()
{
    return fRequestShutdown;
}
