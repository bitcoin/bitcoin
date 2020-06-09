// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <warnings.h>

#include <sync.h>
#include <util/system.h>
#include <util/translation.h>

static Mutex g_warnings_mutex;
static std::string strMiscWarning GUARDED_BY(g_warnings_mutex);
static bool fLargeWorkForkFound GUARDED_BY(g_warnings_mutex) = false;
static bool fLargeWorkInvalidChainFound GUARDED_BY(g_warnings_mutex) = false;

void SetMiscWarning(const std::string& strWarning)
{
    LOCK(g_warnings_mutex);
    strMiscWarning = strWarning;
}

void SetfLargeWorkForkFound(bool flag)
{
    LOCK(g_warnings_mutex);
    fLargeWorkForkFound = flag;
}

bool GetfLargeWorkForkFound()
{
    LOCK(g_warnings_mutex);
    return fLargeWorkForkFound;
}

void SetfLargeWorkInvalidChainFound(bool flag)
{
    LOCK(g_warnings_mutex);
    fLargeWorkInvalidChainFound = flag;
}

std::string GetWarnings(bool verbose)
{
    std::string warnings_concise;
    std::string warnings_verbose;
    const std::string warning_separator = "<hr />";

    LOCK(g_warnings_mutex);

    // Pre-release build warning
    if (!CLIENT_VERSION_IS_RELEASE) {
        warnings_concise = "This is a pre-release test build - use at your own risk - do not use for mining or merchant applications";
        warnings_verbose = _("This is a pre-release test build - use at your own risk - do not use for mining or merchant applications").translated;
    }

    // Misc warnings like out of disk space and clock is wrong
    if (strMiscWarning != "") {
        warnings_concise = strMiscWarning;
        warnings_verbose += (warnings_verbose.empty() ? "" : warning_separator) + strMiscWarning;
    }

    if (fLargeWorkForkFound) {
        warnings_concise = "Warning: The network does not appear to fully agree! Some miners appear to be experiencing issues.";
        warnings_verbose += (warnings_verbose.empty() ? "" : warning_separator) + _("Warning: The network does not appear to fully agree! Some miners appear to be experiencing issues.").translated;
    } else if (fLargeWorkInvalidChainFound) {
        warnings_concise = "Warning: We do not appear to fully agree with our peers! You may need to upgrade, or other nodes may need to upgrade.";
        warnings_verbose += (warnings_verbose.empty() ? "" : warning_separator) + _("Warning: We do not appear to fully agree with our peers! You may need to upgrade, or other nodes may need to upgrade.").translated;
    }

    if (verbose) return warnings_verbose;
    else return warnings_concise;
}
