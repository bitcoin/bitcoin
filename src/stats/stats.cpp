// Copyright (c) 2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "stats/stats.h"

#include "memusage.h"
#include "utiltime.h"

#include "util.h"


const size_t DEFAULT_MAX_STATS_MEMORY = 10 * 1024 * 1024; // 10 MB
const bool DEFAULT_STATISTICS_ENABLED = false;
const static unsigned int STATS_COLLECT_INTERVAL = 2000; // 2 secs

CStats* CStats::sharedInstance = NULL;

CStats* CStats::DefaultStats()
{
    if (!sharedInstance)
        sharedInstance = new CStats();

    return sharedInstance;
}

CStats::CStats() : statsEnabled(false), maxStatsMemory(0)
{
    /* initialize the mempool stats collector */
    mempoolCollector = std::unique_ptr<CStatsMempool>(new CStatsMempool(STATS_COLLECT_INTERVAL));
}

CStats::~CStats()
{

}

std::string CStats::getHelpString(bool showDebug)
{
    std::string strUsage = HelpMessageGroup(_("Statistic options:"));
    strUsage += HelpMessageOpt("-statsenable=", strprintf("Enable statistics (default: %u)", DEFAULT_STATISTICS_ENABLED));
    strUsage += HelpMessageOpt("-statsmaxmemorytarget=<n>", strprintf(_("Set the memory limit target for statistics in bytes (default: %u)"), DEFAULT_MAX_STATS_MEMORY));

    return strUsage;
}

bool CStats::parameterInteraction()
{
    if (gArgs.GetBoolArg("-statsenable", DEFAULT_STATISTICS_ENABLED))
        DefaultStats()->setMaxMemoryUsageTarget(gArgs.GetArg("-statsmaxmemorytarget", DEFAULT_MAX_STATS_MEMORY));

    return true;
}

void CStats::startCollecting(CScheduler& scheduler)
{
    if (statsEnabled) {
        // dispatch the scheduler task
        scheduler.scheduleEvery(std::bind(&CStats::collectCallback, this), STATS_COLLECT_INTERVAL);
    }
}

void CStats::collectCallback()
{
    if (!statsEnabled)
        return;

    if (mempoolCollector->addMempoolSamples(maxStatsMemory)) {
        // fire the signal if the stats did change
        MempoolStatsDidChange();
    }
}

void CStats::setMaxMemoryUsageTarget(size_t maxMem)
{
    statsEnabled = (maxMem > 0);
    maxStatsMemory = maxMem;

    // for now: give 100% to mempool stats
    mempoolCollector->setMaxMemoryUsageTarget(maxMem);
}
