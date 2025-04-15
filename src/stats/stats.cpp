// Copyright (c) 2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <stats/stats.h>

#include <memusage.h>
#include <util/time.h>

#include <cmath>

static const uint32_t SAMPLE_MIN_DELTA_IN_SEC = 2;
static const int CLEANUP_SAMPLES_THRESHOLD = 100;
size_t CStats::maxStatsMemory = 0;
const size_t CStats::DEFAULT_MAX_STATS_MEMORY = 10 * 1024 * 1024; //10 MB
const bool CStats::DEFAULT_STATISTICS_ENABLED = false;
std::atomic<bool> CStats::m_stats_enabled(false); //disable stats by default

CStats* CStats::m_shared_instance{nullptr};

CStats* CStats::DefaultStats()
{
    if (!m_shared_instance)
        m_shared_instance = new CStats();

    return m_shared_instance;
}

void CStats::addMempoolSample(int64_t txcount, int64_t dynUsage, int64_t currentMinRelayFee)
{
    if (!m_stats_enabled)
        return;

    uint64_t now = GetTime();
    {
        LOCK(cs_stats);

        // set the mempool stats start time if this is the first sample
        if (m_mempool_stats.m_start_time == 0)
            m_mempool_stats.m_start_time = now;

        // ensure the minimum time delta between samples
        if (m_mempool_stats.m_samples.size() && m_mempool_stats.m_start_time + m_mempool_stats.m_samples.back().m_time_delta + SAMPLE_MIN_DELTA_IN_SEC >= now) {
            return;
        }

        // calculate the current time delta and add a sample
        uint32_t timeDelta = now - m_mempool_stats.m_start_time; //truncate to uint32_t should be sufficient
        m_mempool_stats.m_samples.push_back({timeDelta, txcount, dynUsage, currentMinRelayFee});
        m_mempool_stats.m_cleanup_counter++;

        // check if we should cleanup the container
        if (m_mempool_stats.m_cleanup_counter >= CLEANUP_SAMPLES_THRESHOLD) {
            //check memory usage
            if (memusage::DynamicUsage(m_mempool_stats.m_samples) > maxStatsMemory && m_mempool_stats.m_samples.size() > 1) {
                // only shrink if the vector.capacity() is > the target for performance reasons
                m_mempool_stats.m_samples.shrink_to_fit();
                const size_t memUsage = memusage::DynamicUsage(m_mempool_stats.m_samples);
                // calculate the amount of samples we need to remove
                size_t itemsToRemove = (memUsage - maxStatsMemory + sizeof(m_mempool_stats.m_samples[0]) - 1) / sizeof(m_mempool_stats.m_samples[0]);

                // sanity check; always keep the most recent sample we just added
                if (m_mempool_stats.m_samples.size() <= itemsToRemove) {
                    itemsToRemove = m_mempool_stats.m_samples.size() - 1;
                }
                m_mempool_stats.m_samples.erase(m_mempool_stats.m_samples.begin(), m_mempool_stats.m_samples.begin() + itemsToRemove);
            }
            // shrink vector
            m_mempool_stats.m_samples.shrink_to_fit();
            m_mempool_stats.m_cleanup_counter = 0;
        }

        // fire signal
        MempoolStatsDidChange();
    }
}

mempoolSamples_t CStats::mempoolGetValuesInRange(uint64_t& fromTime, uint64_t& toTime)
{
    if (!m_stats_enabled)
        return mempoolSamples_t();

    LOCK(cs_stats);

    // if empty, return directly
    if (!m_mempool_stats.m_samples.size())
        return m_mempool_stats.m_samples;


    if (!(fromTime == 0 && toTime == 0) && (fromTime > m_mempool_stats.m_start_time + m_mempool_stats.m_samples.front().m_time_delta || toTime < m_mempool_stats.m_start_time + m_mempool_stats.m_samples.back().m_time_delta)) {
        mempoolSamples_t::iterator fromSample = m_mempool_stats.m_samples.begin();
        mempoolSamples_t::iterator toSample = std::prev(m_mempool_stats.m_samples.end());

        // create subset of samples
        bool fromSet = false;
        for (mempoolSamples_t::iterator it = m_mempool_stats.m_samples.begin(); it != m_mempool_stats.m_samples.end(); ++it) {
            if (m_mempool_stats.m_start_time + (*it).m_time_delta >= fromTime && !fromSet) {
                fromSample = it;
                fromSet = true;
            }
            else if (m_mempool_stats.m_start_time + (*it).m_time_delta > toTime) {
                toSample = std::prev(it);
                break;
            }
        }

        mempoolSamples_t subset(fromSample, toSample + 1);

        // set the fromTime and toTime pass-by-ref parameters
        fromTime = m_mempool_stats.m_start_time + (*fromSample).m_time_delta;
        toTime = m_mempool_stats.m_start_time + (*toSample).m_time_delta;

        // return subset
        return subset;
    }

    // return all available samples
    fromTime = m_mempool_stats.m_start_time + m_mempool_stats.m_samples.front().m_time_delta;
    toTime = m_mempool_stats.m_start_time + m_mempool_stats.m_samples.back().m_time_delta;
    return m_mempool_stats.m_samples;
}

void CStats::setMaxMemoryUsageTarget(size_t maxMem)
{
    m_stats_enabled = (maxMem > 0);

    LOCK(cs_stats);
    maxStatsMemory = maxMem;
}
