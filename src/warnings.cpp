// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <sync.h>
#include <util/system.h>
#include <warnings.h>
#include <hash.h>

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

std::string GetWarnings(const std::string& strFor)
{
    std::string strStatusBar;
    std::string strGUI;
    const std::string uiAlertSeparator = "<hr />";

    LOCK(g_warnings_mutex);

    if (!CLIENT_VERSION_IS_RELEASE) {
        strStatusBar = "This is a pre-release test build - use at your own risk - do not use for mining or merchant applications";
        strGUI = _("This is a pre-release test build - use at your own risk - do not use for mining or merchant applications");
    }

    // Misc warnings like out of disk space and clock is wrong
    if (strMiscWarning != "")
    {
        strStatusBar = strMiscWarning;
        strGUI += (strGUI.empty() ? "" : uiAlertSeparator) + strMiscWarning;
    }

    if (fLargeWorkForkFound)
    {
        strStatusBar = "Warning: The network does not appear to fully agree! Some miners appear to be experiencing issues.";
        strGUI += (strGUI.empty() ? "" : uiAlertSeparator) + _("Warning: The network does not appear to fully agree! Some miners appear to be experiencing issues.");
    }
    else if (fLargeWorkInvalidChainFound)
    {
        strStatusBar = "Warning: We do not appear to fully agree with our peers! You may need to upgrade, or other nodes may need to upgrade.";
        strGUI += (strGUI.empty() ? "" : uiAlertSeparator) + _("Warning: We do not appear to fully agree with our peers! You may need to upgrade, or other nodes may need to upgrade.");
    }

    if (strFor == "gui")
        return strGUI;
    else if (strFor == "statusbar")
        return strStatusBar;
    assert(!"GetWarnings(): invalid parameter");
    return "error";
}
