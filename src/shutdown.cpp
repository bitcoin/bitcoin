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
    if (!Assert(kernel::g_context)->interrupt()) {
        LogPrintf("Sending shutdown token failed\n");
        assert(0);
    }
}

void AbortShutdown()
{
    if (!Assert(kernel::g_context)->interrupt.reset()) {
        LogPrintf("Reading shutdown token failed\n");
        assert(0);
    }
}

bool ShutdownRequested()
{
    return bool{Assert(kernel::g_context)->interrupt};
}

void WaitForShutdown()
{
    if (!Assert(kernel::g_context)->interrupt.wait()) {
        LogPrintf("Reading shutdown token failed\n");
        assert(0);
    }
}
