// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "sync.h"
#include "clientversion.h"
#include "util.h"
#include "warnings.h"

CCriticalSection cs_warnings;
std::string strMiscWarning;
bool fLargeWorkForkFound = false;
bool fLargeWorkInvalidChainFound = false;

void SetMiscWarning(const std::string& strWarning)
{
    LOCK(cs_warnings);
    strMiscWarning = strWarning;
}

void SetfLargeWorkForkFound(bool flag)
{
    LOCK(cs_warnings);
    fLargeWorkForkFound = flag;
}

bool GetfLargeWorkForkFound()
{
    LOCK(cs_warnings);
    return fLargeWorkForkFound;
}

void SetfLargeWorkInvalidChainFound(bool flag)
{
    LOCK(cs_warnings);
    fLargeWorkInvalidChainFound = flag;
}

bool GetfLargeWorkInvalidChainFound()
{
    LOCK(cs_warnings);
    return fLargeWorkInvalidChainFound;
}

std::string GetWarnings(const std::string& strFor)
{
    std::string strStatusBar;
    std::string strRPC;
    std::string strGUI;
    const std::string uiAlertSeperator = "<hr />";

    LOCK(cs_warnings);

    if (!CLIENT_VERSION_IS_RELEASE) {
        strStatusBar = "This is a pre-release test build - use at your own risk - do not use for mining or merchant applications";
        strGUI = _("This is a pre-release test build - use at your own risk - do not use for mining or merchant applications");
    }

    if (GetBoolArg("-testsafemode", DEFAULT_TESTSAFEMODE))
        strStatusBar = strRPC = strGUI = "testsafemode enabled";

    // Misc warnings like out of disk space and clock is wrong
    if (strMiscWarning != "")
    {
        strStatusBar = strMiscWarning;
        strGUI += (strGUI.empty() ? "" : uiAlertSeperator) + strMiscWarning;
    }

    if (fLargeWorkForkFound)
    {
        strStatusBar = strRPC = "Warning: The network does not appear to fully agree! Some miners appear to be experiencing issues. Outgoing and incoming payment may not be reliable! See debug.log for details.";
        strGUI += (strGUI.empty() ? "" : uiAlertSeperator) + _("Warning: The network does not appear to fully agree! Some miners appear to be experiencing issues. Outgoing and incoming payments may not be reliable! See debug.log for details.");
    }
    if (fLargeWorkInvalidChainFound)
    {
        strStatusBar = strRPC = "Warning: Found invalid chain at least 6 blocks longer than our best chain! We do not appear to fully agree with our peers! Outgoing and incoming payment may not be reliable! See debug.log for details.";
        strGUI += (strGUI.empty() ? "" : uiAlertSeperator) + _("Warning: Found invalid chain at least 6 blocks longer than our best chain! We do not appear to fully agree with our peers! Outgoing and incoming payments may not be reliable! Be wary when being suggested to upgrade! See debug.log for details.");
    }

    if (strFor == "gui")
        return strGUI;
    else if (strFor == "statusbar")
        return strStatusBar;
    else if (strFor == "rpc")
        return strRPC;
    assert(!"GetWarnings(): invalid parameter");
    return "error";
}
