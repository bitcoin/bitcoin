// Copyright (c) 2023 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <node/abort.h>

#include <logging.h>
#include <node/interface_ui.h>
#include <node/warnings.h>
#include <util/fs.h>
#include <util/signalinterrupt.h>
#include <util/translation.h>

#include <atomic>
#include <cstdlib>

namespace node {

void AbortNode(util::SignalInterrupt* shutdown, std::atomic<int>& exit_status, const bilingual_str& message, node::Warnings* warnings)
{
    if (warnings) warnings->Set(Warning::FATAL_INTERNAL_ERROR, message);
    InitError(strprintf(_("A fatal internal error occurred, see %s for details: %s"), fs::PathToString(LogInstance().m_file_path.filename()), message));
    exit_status.store(EXIT_FAILURE);
    if (shutdown && !(*shutdown)()) {
        LogError("Failed to send shutdown signal\n");
    };
}
} // namespace node
