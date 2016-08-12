// Copyright (c) 2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_STATS_H
#define BITCOIN_STATS_H

#include "amount.h"
#include "scheduler.h"
#include "stats/stats_mempool.h"

#include <atomic>
#include <stdlib.h>

#include <boost/signals2/signal.hpp>

// Class that manages various types of statistics and its memory consumption
class CStats
{
private:
    static CStats* sharedInstance; //!< singleton instance
    std::atomic<bool> statsEnabled;

    //maximum amount of memory to use for the stats
    std::atomic<size_t> maxStatsMemory;

    /* collect callback through the scheduler task */
    void collectCallback();

public:

    static CStats* DefaultStats(); //shared instance

    CStats();
    ~CStats();

    /* get the statistics module help strings */
    static std::string getHelpString(bool showDebug);

    /* access the parameters and map it to the internal model */
    static bool parameterInteraction();

    /* dispatch the stats collector repetitive scheduler task */
    void startCollecting(CScheduler& scheduler);

    /* set the target for the maximum memory consumption (in bytes) */
    void setMaxMemoryUsageTarget(size_t maxMem);



    /* COLLECTOR INSTANCES
     * =================== */
    std::unique_ptr<CStatsMempool> mempoolCollector;


    /* SIGNALS
     * ======= */
    boost::signals2::signal<void(void)> MempoolStatsDidChange; //mempool signal
};

#endif // BITCOIN_STATS_H
