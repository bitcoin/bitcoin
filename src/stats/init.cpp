// Copyright (c) 2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <stats/stats.h>

#include <common/args.h>
#include <tinyformat.h>

void CStats::AddStatsOptions()
{
    // FIXME: autodetect which default is in use
    // FIXME: ensure we get the correct for multiprocess builds
    gArgs.AddArg("-statsenable", "Enable statistics (default: 1 for GUI, 0 otherwise)", ArgsManager::ALLOW_ANY, OptionsCategory::STATS);
    gArgs.AddArg("-statsmaxmemorytarget=<n>", strprintf("Set the memory limit target for statistics in bytes (default: %u)", DEFAULT_MAX_STATS_MEMORY), ArgsManager::ALLOW_ANY, OptionsCategory::STATS);
}

bool CStats::parameterInteraction()
{
    if (gArgs.GetBoolArg("-statsenable", DEFAULT_STATISTICS_ENABLED))
        DefaultStats()->setMaxMemoryUsageTarget(gArgs.GetIntArg("-statsmaxmemorytarget", DEFAULT_MAX_STATS_MEMORY));

    return true;
}
