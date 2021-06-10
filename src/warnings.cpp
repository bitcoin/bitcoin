// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <warnings.h>

#include <sync.h>
#include <util/system.h>
#include <util/translation.h>

static RecursiveMutex cs_warnings;
static std::string strMiscWarning GUARDED_BY(cs_warnings);
static bool fLargeWorkForkFound GUARDED_BY(cs_warnings) = false;
static bool fLargeWorkInvalidChainFound GUARDED_BY(cs_warnings) = false;

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

std::string GetWarnings(bool verbose)
{
    std::string warnings_concise;
    std::string warnings_verbose;
    const std::string warning_separator = "<hr>";

    LOCK(cs_warnings);

    // Pre-release build warning
    if (!CLIENT_VERSION_IS_RELEASE) {
        warnings_concise = "This is a reference implementation for Bitcoin based coins utilizing VeriBlock's Proof-of-Proof (PoP) technology. \nCoins associated with this blockchain have no value and are for testing purposes only.";
        warnings_verbose = _("This is a reference implementation for Bitcoin based coins utilizing VeriBlock's Proof-of-Proof (PoP) technology. \nCoins associated with this blockchain have no value and are for testing purposes only.").translated;
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
