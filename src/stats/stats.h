// Copyright (c) 2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_STATS_STATS_H
#define BITCOIN_STATS_STATS_H

#include <sync.h>

#include <atomic>
#include <stdlib.h>
#include <vector>

#include <boost/signals2/signal.hpp>

struct CStatsMempoolSample {
    uint32_t m_time_delta;    //use 32bit time delta to save memory
    int64_t m_tx_count;       //transaction count
    int64_t m_dyn_mem_usage;  //dynamic mempool usage
    int64_t m_min_fee_per_k;  //min fee per Kb
};

typedef std::vector<struct CStatsMempoolSample> mempoolSamples_t;

// simple mempool stats container
class CStatsMempool
{
public:
    uint64_t m_start_time;      //start time of the container
    mempoolSamples_t m_samples;
    uint64_t m_cleanup_counter; //internal counter to trogger cleanups

    CStatsMempool()
    {
        m_start_time = 0;
        m_cleanup_counter = 0;
    }
};

// Class that manages various types of statistics and its memory consumption
class CStats
{
private:
    static size_t maxStatsMemory;                  //maximum amount of memory to use for the stats

    static CStats* m_shared_instance;
    mutable RecursiveMutex cs_stats;

    CStatsMempool m_mempool_stats; //mempool stats container

public:
    static const size_t DEFAULT_MAX_STATS_MEMORY; //default maximum of memory to use
    static const bool DEFAULT_STATISTICS_ENABLED; //default value for enabling statistics

    static std::atomic<bool> m_stats_enabled; //if enabled, stats will be collected
    static CStats* DefaultStats(); //shared instance

    /* signals */
    boost::signals2::signal<void(void)> MempoolStatsDidChange; //mempool stats update signal

    /* add a mempool stats sample */
    void addMempoolSample(int64_t txcount, int64_t dynUsage, int64_t currentMinRelayFee);

    /* get all mempool samples in range */
    mempoolSamples_t mempoolGetValuesInRange(uint64_t& fromTime, uint64_t& toTime);

    /* set the target for the maximum memory consumption (in bytes) */
    void setMaxMemoryUsageTarget(size_t maxMem);

    /* register the statistics module help strings */
    static void AddStatsOptions();

    /* access the parameters and map it to the internal model */
    static bool parameterInteraction();
};

#endif // BITCOIN_STATS_STATS_H
