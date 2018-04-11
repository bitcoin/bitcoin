// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <sync.h>
#include <clientversion.h>
#include <util.h>
#include <warnings.h>

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

std::string GetWarnings(const std::string& strFor)
{
    std::string strStatusBar;
    std::string strRPC;
    std::string strGUI;
    const std::string uiAlertSeperator = "<hr />";

    LOCK(cs_warnings);

    if (!CLIENT_VERSION_IS_RELEASE) {
        const char *PRE_RELEASE_MSG =
            "This is a pre-release test build - use at your own risk - do not use for "
            "mining or merchant applications.";
        strStatusBar = PRE_RELEASE_MSG;
        strGUI = _(PRE_RELEASE_MSG);
    }

    if (gArgs.GetBoolArg("-testsafemode", DEFAULT_TESTSAFEMODE))
        strStatusBar = strRPC = strGUI = "testsafemode enabled";

    // Misc warnings like out of disk space and clock is wrong
    if (strMiscWarning != "")
    {
        strStatusBar = strMiscWarning;
        strGUI += (strGUI.empty() ? "" : uiAlertSeperator) + strMiscWarning;
    }

    if (fLargeWorkForkFound)
    {
        const char *LARGE_WORK_FORK_MSG =
            "Warning: The network does not appear to fully agree! Some miners appear to be "
            "experiencing issues.";
        strStatusBar = strRPC = LARGE_WORK_FORK_MSG;
        strGUI += (strGUI.empty() ? "" : uiAlertSeperator) + _(LARGE_WORK_FORK_MSG);
    }
    else if (fLargeWorkInvalidChainFound)
    {
        const char *INVALID_CHAIN_MSG =
            "Warning: We do not appear to fully agree with our peers! You may need to upgrade, "
            "or other nodes may need to upgrade.";
        strStatusBar = strRPC = INVALID_CHAIN_MSG;
        strGUI += (strGUI.empty() ? "" : uiAlertSeperator) + _(INVALID_CHAIN_MSG);
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
