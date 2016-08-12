// Copyright (c) 2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_STATS_MEMPOOL_H
#define BITCOIN_STATS_MEMPOOL_H

#include <amount.h>
#include "scheduler.h"
#include <sync.h>

#include <atomic>
#include <stdlib.h>
#include <vector>

struct CStatsMempoolSample {

    /* time delta to last item as 16bit unsigned integer
     * allows a max delta of ~18h */
    uint16_t timeDelta;

    size_t txCount;     //!<transaction count
    size_t dynMemUsage; //!<dynamic mempool usage
    CAmount minFeePerK;  //!<min fee per K
};

typedef std::vector<struct CStatsMempoolSample> MempoolSamplesVector;

class CStatsMempool
{
private:

    /* caches */
    static std::atomic<size_t> cacheMempoolSize;
    static std::atomic<size_t> cacheMempoolDynamicMemoryUsage;
    static std::atomic<CAmount> cacheMempoolMinRelayFee;
    mutable CCriticalSection cs_mempool_stats; //!<protects the sample/max/time vectors

    std::atomic<uint64_t> startTime; //start time
    std::vector<MempoolSamplesVector> vSamplesPerPrecision; //!<multiple samples vector per precision level
    std::vector<size_t> vMaxSamplesPerPrecision; //!<vector with the maximum amount of samples per precision level
    std::vector<size_t> vTimeLastSample; //!<vector with the maximum amount of samples per precision level
    size_t intervalCounter; //!<internal counter for cleanups

    const unsigned int collectInterval; //timedelta in milliseconds

    /* adds a mempool stats samples by accessing the lock free cache values
     * returns true if at least one sample was collected */
    bool addMempoolSamples(const size_t maxStatsMemory);

    /* set the target for the maximum memory consumption (in bytes) */
    void setMaxMemoryUsageTarget(size_t maxMem);

    friend class CStats;
public:

    CStatsMempool(unsigned int collectIntervalIn);

    static void setCache(size_t cacheMempoolSizeIn, size_t cacheMempoolDynamicMemoryUsageIn, CAmount cacheMempoolMinRelayFeeIn) {
        cacheMempoolSize = cacheMempoolSizeIn;
        cacheMempoolDynamicMemoryUsage = cacheMempoolDynamicMemoryUsageIn;
        cacheMempoolMinRelayFee = cacheMempoolMinRelayFeeIn;
    }

    /* get mempool precision groups and its time interval-target */
    std::vector<unsigned int> getPrecisionGroupsAndIntervals();

    /* get mempool samples from a given precision group index(!)
     * this returns a COPY */
    MempoolSamplesVector getSamplesForPrecision(unsigned int precision, uint64_t& fromTime);
};

#endif // BITCOIN_STATS_MEMPOOL_H
