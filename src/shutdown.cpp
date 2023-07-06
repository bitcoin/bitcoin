// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <shutdown.h>

#include <kernel/context.h>
#include <logging.h>
#include <util/check.h>
#include <util/signalinterrupt.h>

#include <assert.h>
#include <system_error>

void StartShutdown()
{
    try {
        Assert(kernel::g_context)->interrupt();
    } catch (const std::system_error&) {
        LogPrintf("Sending shutdown token failed\n");
        assert(0);
    }
}

void AbortShutdown()
{
    Assert(kernel::g_context)->interrupt.reset();
}

bool ShutdownRequested()
{
    return bool{Assert(kernel::g_context)->interrupt};
}

void WaitForShutdown()
{
    try {
        Assert(kernel::g_context)->interrupt.wait();
    } catch (const std::system_error&) {
        LogPrintf("Reading shutdown token failed\n");
        assert(0);
    }
}
