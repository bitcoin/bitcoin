// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "alert.h"
#include <clientversion.h>
#include <util.h>
#include <warnings.h>

#include <checkpointsync.h>

CCriticalSection cs_warnings;
std::string strMiscWarning;
std::string strMintWarning;
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
    int nPriority = 0;
    std::string strStatusBar;
    std::string strRPC;
    std::string strGUI;
    const std::string uiAlertSeperator = "<hr />";

    LOCK(cs_warnings);

    if (!CLIENT_VERSION_IS_RELEASE) {
        strStatusBar = "This is a pre-release test build - use at your own risk - do not use for mining or merchant applications";
        strGUI = _("This is a pre-release test build - use at your own risk - do not use for mining or merchant applications");
    }

    if (gArgs.GetBoolArg("-testsafemode", DEFAULT_TESTSAFEMODE))
        strStatusBar = strRPC = strGUI = "testsafemode enabled";

    // peercoin: wallet lock warning for minting
    if (strMintWarning != "")
    {
        nPriority = 0;
        strStatusBar = strRPC = strGUI = strMintWarning;
        strGUI += (strGUI.empty() ? "" : uiAlertSeperator) + _(strMintWarning.c_str());
    }

#ifdef ENABLE_CHECKPOINTS
    // peercoin: checkpoint warning
    // should not enter safe mode for longer invalid chain
    if (strCheckpointWarning != "")
    {
        nPriority = 900;
        strStatusBar = strRPC = strGUI = strCheckpointWarning;
        strGUI += (strGUI.empty() ? "" : uiAlertSeperator) + _(strMintWarning.c_str());
    }
#endif

    // Misc warnings like out of disk space and clock is wrong
    if (strMiscWarning != "")
    {
        nPriority = 1000;
        strStatusBar = strMiscWarning;
        strGUI += (strGUI.empty() ? "" : uiAlertSeperator) + strMiscWarning;
    }

    if (fLargeWorkForkFound)
    {
        nPriority = 2000;
        strStatusBar = strRPC = "Warning: The network does not appear to fully agree! Some miners appear to be experiencing issues.";
        strGUI += (strGUI.empty() ? "" : uiAlertSeperator) + _("Warning: The network does not appear to fully agree! Some miners appear to be experiencing issues.");
    }
    else if (fLargeWorkInvalidChainFound)
    {
        nPriority = 2000;
        strStatusBar = strRPC = "Warning: We do not appear to fully agree with our peers! You may need to upgrade, or other nodes may need to upgrade.";
        strGUI += (strGUI.empty() ? "" : uiAlertSeperator) + _("Warning: We do not appear to fully agree with our peers! You may need to upgrade, or other nodes may need to upgrade.");
    }
#ifdef ENABLE_CHECKPOINTS
    // peercoin: detect invalid checkpoint
    if (hashInvalidCheckpoint != uint256())
    {
        nPriority = 3000;
        strStatusBar = strRPC = "WARNING: Inconsistent checkpoint found! Stop enforcing checkpoints and notify developers to resolve the issue.";
        strGUI += (strGUI.empty() ? "" : uiAlertSeperator) + _("WARNING: Invalid checkpoint found! Displayed transactions may not be correct! You may need to upgrade, or notify developers of the issue.");
    }
#endif
    // Alerts
    {
        LOCK(cs_mapAlerts);
        for (const auto& item : mapAlerts)
        {
            const CAlert& alert = item.second;
            if (alert.AppliesToMe() && alert.nPriority > nPriority)
            {
                nPriority = alert.nPriority;
                strStatusBar = strGUI = alert.strStatusBar;
            }
        }
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
