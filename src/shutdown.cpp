// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <shutdown.h>

#if defined(HAVE_CONFIG_H)
#include <config/bitcoin-config.h>
#endif

#include <kernel/context.h>
#include <logging.h>
#include <node/interface_ui.h>
#include <util/check.h>
#include <util/signalinterrupt.h>
#include <warnings.h>

#include <atomic>
#include <cassert>

static std::atomic<int>* g_exit_status{nullptr};

bool AbortNode(const std::string& strMessage, bilingual_str user_message)
{
    SetMiscWarning(Untranslated(strMessage));
    LogPrintf("*** %s\n", strMessage);
    if (user_message.empty()) {
        user_message = _("A fatal internal error occurred, see debug.log for details");
    }
    InitError(user_message);
    Assert(g_exit_status)->store(EXIT_FAILURE);
    StartShutdown();
    return false;
}

void InitShutdownState(std::atomic<int>& exit_status)
{
    g_exit_status = &exit_status;
}

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
