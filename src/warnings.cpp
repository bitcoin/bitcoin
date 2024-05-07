// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <config/bitcoin-config.h> // IWYU pragma: keep

#include <warnings.h>

#include <common/system.h>
#include <sync.h>
#include <util/translation.h>

#include <optional>
#include <vector>

static GlobalMutex g_warnings_mutex;
static bilingual_str g_misc_warnings GUARDED_BY(g_warnings_mutex);
static bool fLargeWorkInvalidChainFound GUARDED_BY(g_warnings_mutex) = false;
static std::optional<bilingual_str> g_timeoffset_warning GUARDED_BY(g_warnings_mutex){};

void SetMiscWarning(const bilingual_str& warning)
{
    LOCK(g_warnings_mutex);
    g_misc_warnings = warning;
}

void SetfLargeWorkInvalidChainFound(bool flag)
{
    LOCK(g_warnings_mutex);
    fLargeWorkInvalidChainFound = flag;
}

void SetMedianTimeOffsetWarning(std::optional<bilingual_str> warning)
{
    LOCK(g_warnings_mutex);
    g_timeoffset_warning = warning;
}

std::vector<bilingual_str> GetWarnings()
{
    std::vector<bilingual_str> warnings;

    LOCK(g_warnings_mutex);

    // Pre-release build warning
    if (!CLIENT_VERSION_IS_RELEASE) {
        warnings.emplace_back(_("This is a pre-release test build - use at your own risk - do not use for mining or merchant applications"));
    }

    // Misc warnings like out of disk space and clock is wrong
    if (!g_misc_warnings.empty()) {
        warnings.emplace_back(g_misc_warnings);
    }

    if (fLargeWorkInvalidChainFound) {
        warnings.emplace_back(_("Warning: We do not appear to fully agree with our peers! You may need to upgrade, or other nodes may need to upgrade."));
    }

    if (g_timeoffset_warning) {
        warnings.emplace_back(g_timeoffset_warning.value());
    }

    return warnings;
}
