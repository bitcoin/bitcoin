// Copyright (c) 2023 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <node/abort.h>

#include <logging.h>
#include <node/interface_ui.h>
#include <util/signalinterrupt.h>
#include <util/translation.h>
#include <warnings.h>

#include <atomic>
#include <cstdlib>
#include <string>

namespace node {

void AbortNode(util::SignalInterrupt* shutdown, std::atomic<int>& exit_status, const bilingual_str& message)
{
    SetMiscWarning(message);
    InitError(_("A fatal internal error occurred, see debug.log for details: ") + message);
    exit_status.store(EXIT_FAILURE);
    if (shutdown && !(*shutdown)()) {
        LogError("Failed to send shutdown signal\n");
    };
}
} // namespace node
