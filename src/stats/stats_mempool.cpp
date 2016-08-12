// Copyright (c) 2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "stats/stats_mempool.h"

#include "memusage.h"
#include "utiltime.h"

#include "util.h"


const static unsigned int precisionIntervals[] = {
    2, // == every 2 secs == 1800 samples per hour
    60, // == every minute = 1440 samples per day
    1800 // == every half-hour = ~2'160 per Month
};

const static unsigned int MIN_SAMPLES = 5;
const static unsigned int MAX_SAMPLES = 5000;


const static unsigned int fallbackMaxSamplesPerPrecision = 1000;

std::atomic<size_t> CStatsMempool::cacheMempoolSize;
std::atomic<size_t> CStatsMempool::cacheMempoolDynamicMemoryUsage;
std::atomic<CAmount> CStatsMempool::cacheMempoolMinRelayFee;

CStatsMempool::CStatsMempool(unsigned int collectIntervalIn) : collectInterval(collectIntervalIn)
{
    startTime = 0;
    intervalCounter = 0;

    // setup the samples per precision vector
    for (unsigned int interval : precisionIntervals) {
        (void)(interval);
        vSamplesPerPrecision.emplace_back();

        // use the fallback max in case max memory will not be set
        vMaxSamplesPerPrecision.push_back(fallbackMaxSamplesPerPrecision);

        // add starttime 0 to each level
        vTimeLastSample.push_back(0);
    }
}

std::vector<unsigned int> CStatsMempool::getPrecisionGroupsAndIntervals() {
    return {std::begin(precisionIntervals), std::end(precisionIntervals)};
}

bool CStatsMempool::addMempoolSamples(const size_t maxStatsMemory)
{
    bool statsChanged = false;
    uint64_t now = GetTime();
    {
        LOCK(cs_mempool_stats);

        // set the mempool stats start time if this is the first sample
        if (startTime == 0)
            startTime = now;

        unsigned int biggestInterval = 0;
        for (unsigned int i = 0; i < sizeof(precisionIntervals) / sizeof(precisionIntervals[0]); i++) {
            // check if it's time to collect a samples for the given precision level
            uint16_t timeDelta = 0;
            if (intervalCounter % (precisionIntervals[i] / (collectInterval / 1000)) == 0) {
                if (vTimeLastSample[i] == 0) {
                    // first sample, calc delta to starttime
                    timeDelta = now - startTime;
                } else {
                    timeDelta = now - vTimeLastSample[i];
                }
                vSamplesPerPrecision[i].push_back({timeDelta, CStatsMempool::cacheMempoolSize, CStatsMempool::cacheMempoolDynamicMemoryUsage, CStatsMempool::cacheMempoolMinRelayFee});
                statsChanged = true;

                // check if we need to remove items at the beginning
                if (vSamplesPerPrecision[i].size() > vMaxSamplesPerPrecision[i]) {
                    // increase starttime by the removed deltas
                    for (unsigned int j = (vSamplesPerPrecision[i].size() - vMaxSamplesPerPrecision[i]); j > 0; j--) {
                        startTime += vSamplesPerPrecision[i][j].timeDelta;
                    }
                    // remove element(s) at vector front
                    vSamplesPerPrecision[i].erase(vSamplesPerPrecision[i].begin(), vSamplesPerPrecision[i].begin() + (vSamplesPerPrecision[i].size() - vMaxSamplesPerPrecision[i]));

                    // release memory
                    vSamplesPerPrecision[i].shrink_to_fit();
                }

                vTimeLastSample[i] = now;
            }
            biggestInterval = precisionIntervals[i];
        }

        intervalCounter++;

        if (intervalCounter > biggestInterval) {
            intervalCounter = 1;
        }
    }
    return statsChanged;
}

void CStatsMempool::setMaxMemoryUsageTarget(size_t maxMem)
{
    // calculate the memory requirement of a single sample
    size_t sampleSize = memusage::MallocUsage(sizeof(CStatsMempoolSample));

    // calculate how many samples would fit in the target
    size_t maxAmountOfSamples = maxMem / sampleSize;

    // distribute the max samples equal between precision levels
    unsigned int samplesPerPrecision = maxAmountOfSamples / sizeof(precisionIntervals) / sizeof(precisionIntervals[0]);
    samplesPerPrecision = std::max(MIN_SAMPLES, samplesPerPrecision);
    samplesPerPrecision = std::min(MAX_SAMPLES, samplesPerPrecision);
    for (unsigned int i = 0; i < sizeof(precisionIntervals) / sizeof(precisionIntervals[0]); i++) {
        vMaxSamplesPerPrecision[i] = samplesPerPrecision;
    }
}

MempoolSamplesVector CStatsMempool::getSamplesForPrecision(unsigned int precision, uint64_t& fromTime)
{
    LOCK(cs_mempool_stats);

    if (precision >= vSamplesPerPrecision.size()) {
        return MempoolSamplesVector();
    }

    fromTime = startTime;
    return vSamplesPerPrecision[precision];
}
