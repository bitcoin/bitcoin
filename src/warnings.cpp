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

std::string GetWarnings(bool htmlMultiline)
{
    std::string strPlaintext;
    std::string strHTML;
    const std::string uiAlertSeperator = "<hr />";

    LOCK(cs_warnings);

    if (!CLIENT_VERSION_IS_RELEASE) {
        strPlaintext = "This is a pre-release test build - use at your own risk - do not use for mining or merchant applications";
        strHTML = _("This is a pre-release test build - use at your own risk - do not use for mining or merchant applications");
    }

    if (gArgs.GetBoolArg("-testsafemode", DEFAULT_TESTSAFEMODE))
        strPlaintext = strHTML = "testsafemode enabled";

    // Misc warnings like out of disk space and clock is wrong
    if (strMiscWarning != "")
    {
        strPlaintext = strMiscWarning;
        strHTML += (strHTML.empty() ? "" : uiAlertSeperator) + strMiscWarning;
    }

    if (fLargeWorkForkFound)
    {
        strPlaintext = "Warning: The network does not appear to fully agree! Some miners appear to be experiencing issues.";
        strHTML += (strHTML.empty() ? "" : uiAlertSeperator) + _("Warning: The network does not appear to fully agree! Some miners appear to be experiencing issues.");
    }
    else if (fLargeWorkInvalidChainFound)
    {
        strPlaintext = "Warning: We do not appear to fully agree with our peers! You may need to upgrade, or other nodes may need to upgrade.";
        strHTML += (strHTML.empty() ? "" : uiAlertSeperator) + _("Warning: We do not appear to fully agree with our peers! You may need to upgrade, or other nodes may need to upgrade.");
    }

    return (htmlMultiline ? strHTML : strPlaintext);
}
