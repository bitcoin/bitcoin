// Copyright (c) 2023 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <node/abort.h>

#include <logging.h>
#include <node/interface_ui.h>
#include <shutdown.h>
#include <util/translation.h>
#include <warnings.h>

#include <atomic>
#include <cstdlib>
#include <string>

namespace node {

void AbortNode(std::atomic<int>& exit_status, const std::string& debug_message, const bilingual_str& user_message, bool shutdown)
{
    SetMiscWarning(Untranslated(debug_message));
    LogPrintf("*** %s\n", debug_message);
    InitError(user_message.empty() ? _("A fatal internal error occurred, see debug.log for details") : user_message);
    exit_status.store(EXIT_FAILURE);
    if (shutdown) StartShutdown();
}
} // namespace node
