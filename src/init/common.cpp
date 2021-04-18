// Copyright (c) 2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#if defined(HAVE_CONFIG_H)
#include <config/bitcoin-config.h>
#endif

#include <compat/sanity.h>
#include <crypto/sha256.h>
#include <key.h>
#include <logging.h>
#include <node/ui_interface.h>
#include <pubkey.h>
#include <random.h>
#include <util/system.h>
#include <util/time.h>
#include <util/translation.h>

#include <memory>

static std::unique_ptr<ECCVerifyHandle> globalVerifyHandle;

namespace init {
void SetGlobals()
{
    std::string sha256_algo = SHA256AutoDetect();
    LogPrintf("Using the '%s' SHA256 implementation\n", sha256_algo);
    RandomInit();
    ECC_Start();
    globalVerifyHandle.reset(new ECCVerifyHandle());
}

void UnsetGlobals()
{
    globalVerifyHandle.reset();
    ECC_Stop();
}

bool SanityChecks()
{
    if (!ECC_InitSanityCheck()) {
        return InitError(Untranslated("Elliptic curve cryptography sanity check failure. Aborting."));
    }

    if (!glibcxx_sanity_test())
        return false;

    if (!Random_SanityCheck()) {
        return InitError(Untranslated("OS cryptographic RNG sanity check failure. Aborting."));
    }

    if (!ChronoSanityCheck()) {
        return InitError(Untranslated("Clock epoch mismatch. Aborting."));
    }

    return true;
}

void AddLoggingArgs(ArgsManager& argsman)
{
    argsman.AddArg("-debuglogfile=<file>", strprintf("Specify location of debug log file. Relative paths will be prefixed by a net-specific datadir location. (-nodebuglogfile to disable; default: %s)", DEFAULT_DEBUGLOGFILE), ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-debug=<category>", "Output debugging information (default: -nodebug, supplying <category> is optional). "
        "If <category> is not supplied or if <category> = 1, output all debugging information. <category> can be: " + LogInstance().LogCategoriesString() + ". This option can be specified multiple times to output multiple categories.",
        ArgsManager::ALLOW_ANY, OptionsCategory::DEBUG_TEST);
    argsman.AddArg("-debugexclude=<category>", strprintf("Exclude debugging information for a category. Can be used in conjunction with -debug=1 to output debug logs for all categories except the specified category. This option can be specified multiple times to exclude multiple categories."), ArgsManager::ALLOW_ANY, OptionsCategory::DEBUG_TEST);
    argsman.AddArg("-logips", strprintf("Include IP addresses in debug output (default: %u)", DEFAULT_LOGIPS), ArgsManager::ALLOW_ANY, OptionsCategory::DEBUG_TEST);
    argsman.AddArg("-logtimestamps", strprintf("Prepend debug output with timestamp (default: %u)", DEFAULT_LOGTIMESTAMPS), ArgsManager::ALLOW_ANY, OptionsCategory::DEBUG_TEST);
#ifdef HAVE_THREAD_LOCAL
    argsman.AddArg("-logthreadnames", strprintf("Prepend debug output with name of the originating thread (only available on platforms supporting thread_local) (default: %u)", DEFAULT_LOGTHREADNAMES), ArgsManager::ALLOW_ANY, OptionsCategory::DEBUG_TEST);
#else
    argsman.AddHiddenArgs({"-logthreadnames"});
#endif
    argsman.AddArg("-logsourcelocations", strprintf("Prepend debug output with name of the originating source location (source file, line number and function name) (default: %u)", DEFAULT_LOGSOURCELOCATIONS), ArgsManager::ALLOW_ANY, OptionsCategory::DEBUG_TEST);
    argsman.AddArg("-logtimemicros", strprintf("Add microsecond precision to debug timestamps (default: %u)", DEFAULT_LOGTIMEMICROS), ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::DEBUG_TEST);
    argsman.AddArg("-printtoconsole", "Send trace/debug info to console (default: 1 when no -daemon. To disable logging to file, set -nodebuglogfile)", ArgsManager::ALLOW_ANY, OptionsCategory::DEBUG_TEST);
    argsman.AddArg("-shrinkdebugfile", "Shrink debug.log file on client startup (default: 1 when no -debug)", ArgsManager::ALLOW_ANY, OptionsCategory::DEBUG_TEST);
}
} // namespace init
