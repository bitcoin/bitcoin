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
#include <atomic>
#include <system_error>
// SYSCOIN
namespace {
std::atomic<bool> g_shutdown_requested{false};
} // namespace

void StartShutdown()
{
    g_shutdown_requested.store(true);
    if (!kernel::g_context) return;
    try {
        kernel::g_context->interrupt();
    } catch (const std::system_error&) {
        LogPrintf("Sending shutdown token failed\n");
        assert(0);
    }
}

void AbortShutdown()
{
    g_shutdown_requested.store(false);
    if (!kernel::g_context) return;
    kernel::g_context->interrupt.reset();
}

bool ShutdownRequested()
{
    if (g_shutdown_requested.load()) return true;
    if (!kernel::g_context) return false;
    return bool{kernel::g_context->interrupt};
}

void WaitForShutdown()
{
    if (!kernel::g_context) return;
    try {
        kernel::g_context->interrupt.wait();
    } catch (const std::system_error&) {
        LogPrintf("Reading shutdown token failed\n");
        assert(0);
    }
}
