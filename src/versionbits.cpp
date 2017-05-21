// Copyright (c) 2016-2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "versionbits.h"
#include "consensus/params.h"

// bip135 begin fill out entire table
struct BIP9DeploymentInfo VersionBitsDeploymentInfo[Consensus::MAX_VERSION_BITS_DEPLOYMENTS] = {
    {
        /*.name =*/ (char *) "csv",
        /*.gbt_force =*/ true,
    },
    {
        /*.name =*/ (char *) "segwit",
        /*.gbt_force =*/ true,
    },
    {
        /*.name =*/ (char *) "",          // unallocated bit 2
        /*.gbt_force =*/ false,
    },
    {
        /*.name =*/ (char *) "",          // unallocated bit 3
        /*.gbt_force =*/ false,
    },
    {
        /*.name =*/ (char *) "",          // unallocated bit 4
        /*.gbt_force =*/ false,
    },
    {
        /*.name =*/ (char *) "",          // unallocated bit 5
        /*.gbt_force =*/ false,
    },
    {
        /*.name =*/ (char *) "",          // unallocated bit 6
        /*.gbt_force =*/ false,
    },
    {
        /*.name =*/ (char *) "",          // unallocated bit 7
        /*.gbt_force =*/ false,
    },
    {
        /*.name =*/ (char *) "",          // unallocated bit 8
        /*.gbt_force =*/ false,
    },
    {
        /*.name =*/ (char *) "",          // unallocated bit 9
        /*.gbt_force =*/ false,
    },
    {
        /*.name =*/ (char *) "",          // unallocated bit 10
        /*.gbt_force =*/ false,
    },
    {
        /*.name =*/ (char *) "",          // unallocated bit 11
        /*.gbt_force =*/ false,
    },
    {
        /*.name =*/ (char *) "",          // unallocated bit 12
        /*.gbt_force =*/ false,
    },
    {
        /*.name =*/ (char *) "",          // unallocated bit 13
        /*.gbt_force =*/ false,
    },
    {
        /*.name =*/ (char *) "",          // unallocated bit 14
        /*.gbt_force =*/ false,
    },
    {
        /*.name =*/ (char *) "",          // unallocated bit 15
        /*.gbt_force =*/ false,
    },
    {
        /*.name =*/ (char *) "",          // unallocated bit 16
        /*.gbt_force =*/ false,
    },
    {
        /*.name =*/ (char *) "",          // unallocated bit 17
        /*.gbt_force =*/ false,
    },
    {
        /*.name =*/ (char *) "",          // unallocated bit 18
        /*.gbt_force =*/ false,
    },
    {
        /*.name =*/ (char *) "",          // unallocated bit 19
        /*.gbt_force =*/ false,
    },
    {
        /*.name =*/ (char *) "",          // unallocated bit 20
        /*.gbt_force =*/ false,
    },
    {
        /*.name =*/ (char *) "",          // unallocated bit 21
        /*.gbt_force =*/ false,
    },
    {
        /*.name =*/ (char *) "",          // unallocated bit 22
        /*.gbt_force =*/ false,
    },
    {
        /*.name =*/ (char *) "",          // unallocated bit 23
        /*.gbt_force =*/ false,
    },
    {
        /*.name =*/ (char *) "",          // unallocated bit 24
        /*.gbt_force =*/ false,
    },
    {
        /*.name =*/ (char *) "",          // unallocated bit 25
        /*.gbt_force =*/ false,
    },
    {
        /*.name =*/ (char *) "",          // unallocated bit 26
        /*.gbt_force =*/ false,
    },
    {
        /*.name =*/ (char *) "",          // unallocated bit 27
        /*.gbt_force =*/ false,
    },
    {
        /*.name =*/ (char *) "testdummy", // unallocated bit 28
        /*.gbt_force =*/ false,
    }
};
// bip135 end


// bip135 begin
// major adaptations to generalize and support new grace period parameters
ThresholdState AbstractThresholdConditionChecker::GetStateFor(const CBlockIndex* pindexPrev, const Consensus::Params& params, ThresholdConditionCache& cache) const
{
    int nPeriod = Period(params);
    int nThreshold = Threshold(params);
    int64_t nTimeStart = BeginTime(params);
    int64_t nTimeTimeout = EndTime(params);
    int nMinLockedBlocks = MinLockedBlocks(params);
    int64_t nMinLockedTime = MinLockedTime(params);
    int64_t nActualLockinTime = 0;
    int nActualLockinBlock = 0;

    if (nPeriod == 0)
    {
        // we cannot do anything further -this deployment is not really defined.
        return THRESHOLD_DEFINED;
    }

    // A block's state is always the same as that of the first of its period, so it is computed based on a pindexPrev whose height equals a multiple of nPeriod - 1.
    if (pindexPrev != NULL) {
        assert(nPeriod);
        pindexPrev = pindexPrev->GetAncestor(pindexPrev->nHeight - ((pindexPrev->nHeight + 1) % nPeriod));
    }

    // Walk backwards in steps of nPeriod to find a pindexPrev which was DEFINED
    std::vector<const CBlockIndex*> vToCompute;
    bool backAtDefined = (cache.count(pindexPrev) && cache[pindexPrev] == THRESHOLD_DEFINED);
    while (!backAtDefined) {
        if (pindexPrev == NULL) {
            // The genesis block is by definition defined.
            cache[pindexPrev] = THRESHOLD_DEFINED;
            break;
        }
        if (pindexPrev->GetMedianTimePast() < nTimeStart) {
            // Optimizaton: don't recompute down further, as we know every
            // earlier block will be before the start time
            cache[pindexPrev] = THRESHOLD_DEFINED;
            break;
        }

        // push the pindex for later forward walking
        vToCompute.push_back(pindexPrev);
        // go back one more period
        pindexPrev = pindexPrev->GetAncestor(pindexPrev->nHeight - nPeriod);

        if (cache.count(pindexPrev) > 0 && cache[pindexPrev] == THRESHOLD_DEFINED) {
            backAtDefined = true;
        }
    }

    // At this point, cache[pindexPrev] is known
    assert(cache.count(pindexPrev));

    // initialize starting state for forward walk
    ThresholdState state = cache[pindexPrev];
    assert(state == THRESHOLD_DEFINED);
    // Now walk forward and compute the state of descendants of pindexPrev
    while (!vToCompute.empty()) {
        ThresholdState stateNext = state;
        pindexPrev = vToCompute.back();
        vToCompute.pop_back();

        switch (state) {
            case THRESHOLD_DEFINED: {
                if (pindexPrev->GetMedianTimePast() >= nTimeTimeout) {
                    stateNext = THRESHOLD_FAILED;
                } else if (pindexPrev->GetMedianTimePast() >= nTimeStart) {
                    stateNext = THRESHOLD_STARTED;
                }
                break;
            }
            case THRESHOLD_STARTED: {
                if (pindexPrev->GetMedianTimePast() >= nTimeTimeout) {
                    stateNext = THRESHOLD_FAILED;
                    break;
                }
                // We need to count
                const CBlockIndex* pindexCount = pindexPrev;
                int count = 0;
                for (int i = 0; i < nPeriod; i++) {
                    if (Condition(pindexCount, params)) {
                        count++;
                    }
                    pindexCount = pindexCount->pprev;
                }
                if (count >= nThreshold) {
                    stateNext = THRESHOLD_LOCKED_IN;
                    // bip135: make a note of lock-in time & height
                    // this will be used for assessing grace period conditions.
                    nActualLockinBlock = pindexPrev->nHeight;
                    nActualLockinTime = pindexPrev->GetMedianTimePast();
                }
                break;
            }
            case THRESHOLD_LOCKED_IN: {
                // bip135: Progress to ACTIVE only once all grace conditions are met.
                if (pindexPrev->GetMedianTimePast() >= nActualLockinTime + nMinLockedTime
                        && pindexPrev->nHeight >= nActualLockinBlock + nMinLockedBlocks) {
                    stateNext = THRESHOLD_ACTIVE;
                }
                else {
                    // bip135: if grace not yet met, remain in LOCKED_IN
                    stateNext = THRESHOLD_LOCKED_IN;
                }
                break;
            }
            case THRESHOLD_FAILED:
            case THRESHOLD_ACTIVE: {
                // Nothing happens, these are terminal states.
                break;
            }
        }
        cache[pindexPrev] = state = stateNext;
    }

    return state;
}
// bip135 end

// return the numerical statistics of blocks signalling the specified BIP9 condition in this current period
ForkStats AbstractThresholdConditionChecker::GetStateStatisticsFor(const CBlockIndex* pindex, const Consensus::Params& params) const
{
    ForkStats stats;

    stats.period = Period(params);
    stats.threshold = Threshold(params);

    if (pindex == NULL)
        return stats;

    // Find beginning of period
    const CBlockIndex* pindexEndOfPrevPeriod = pindex->GetAncestor(pindex->nHeight - ((pindex->nHeight + 1) % stats.period));
    stats.elapsed = pindex->nHeight - pindexEndOfPrevPeriod->nHeight;

    // Count from current block to beginning of period
    int count = 0;
    const CBlockIndex* currentIndex = pindex;
    while (pindexEndOfPrevPeriod->nHeight != currentIndex->nHeight){
        if (Condition(currentIndex, params))
            count++;
        currentIndex = currentIndex->pprev;
    }

    stats.count = count;
    stats.possible = (stats.period - stats.threshold ) >= (stats.elapsed - count);

    return stats;
}

int AbstractThresholdConditionChecker::GetStateSinceHeightFor(const CBlockIndex* pindexPrev, const Consensus::Params& params, ThresholdConditionCache& cache) const
{
    const ThresholdState initialState = GetStateFor(pindexPrev, params, cache);

    // BIP 9 about state DEFINED: "The genesis block is by definition in this state for each deployment."
    if (initialState == THRESHOLD_DEFINED) {
        return 0;
    }

    const int nPeriod = Period(params);

    // A block's state is always the same as that of the first of its period, so it is computed based on a pindexPrev whose height equals a multiple of nPeriod - 1.
    // To ease understanding of the following height calculation, it helps to remember that
    // right now pindexPrev points to the block prior to the block that we are computing for, thus:
    // if we are computing for the last block of a period, then pindexPrev points to the second to last block of the period, and
    // if we are computing for the first block of a period, then pindexPrev points to the last block of the previous period.
    // The parent of the genesis block is represented by NULL.
    pindexPrev = pindexPrev->GetAncestor(pindexPrev->nHeight - ((pindexPrev->nHeight + 1) % nPeriod));

    const CBlockIndex* previousPeriodParent = pindexPrev->GetAncestor(pindexPrev->nHeight - nPeriod);

    while (previousPeriodParent != NULL && GetStateFor(previousPeriodParent, params, cache) == initialState) {
        pindexPrev = previousPeriodParent;
        previousPeriodParent = pindexPrev->GetAncestor(pindexPrev->nHeight - nPeriod);
    }

    // Adjust the result because right now we point to the parent block.
    return pindexPrev->nHeight + 1;
}

namespace
{
/**
 * Class to implement versionbits logic.
 */
class VersionBitsConditionChecker : public AbstractThresholdConditionChecker {
private:
    const Consensus::DeploymentPos id;

protected:
    int64_t BeginTime(const Consensus::Params& params) const { return params.vDeployments[id].nStartTime; }
    int64_t EndTime(const Consensus::Params& params) const { return params.vDeployments[id].nTimeout; }
    int Period(const Consensus::Params& params) const { return params.vDeployments[id].windowsize; }
    int Threshold(const Consensus::Params& params) const { return params.vDeployments[id].threshold; }
    int MinLockedBlocks(const Consensus::Params& params) const { return params.vDeployments[id].minlockedblocks; }
    int64_t MinLockedTime(const Consensus::Params& params) const { return params.vDeployments[id].minlockedtime; }

    bool Condition(const CBlockIndex* pindex, const Consensus::Params& params) const
    {
        return (((pindex->nVersion & VERSIONBITS_TOP_MASK) == VERSIONBITS_TOP_BITS) && (pindex->nVersion & Mask(params)) != 0);
    }

public:
    VersionBitsConditionChecker(Consensus::DeploymentPos id_) : id(id_) {}
    uint32_t Mask(const Consensus::Params& params) const { return ((uint32_t)1) << params.vDeployments[id].bit; }
};

}

ThresholdState VersionBitsState(const CBlockIndex* pindexPrev, const Consensus::Params& params, Consensus::DeploymentPos pos, VersionBitsCache& cache)
{
    return VersionBitsConditionChecker(pos).GetStateFor(pindexPrev, params, cache.caches[pos]);
}

ForkStats VersionBitsStatistics(const CBlockIndex* pindexPrev, const Consensus::Params& params, Consensus::DeploymentPos pos)
{
    return VersionBitsConditionChecker(pos).GetStateStatisticsFor(pindexPrev, params);
}

int VersionBitsStateSinceHeight(const CBlockIndex* pindexPrev, const Consensus::Params& params, Consensus::DeploymentPos pos, VersionBitsCache& cache)
{
    return VersionBitsConditionChecker(pos).GetStateSinceHeightFor(pindexPrev, params, cache.caches[pos]);
}

uint32_t VersionBitsMask(const Consensus::Params& params, Consensus::DeploymentPos pos)
{
    return VersionBitsConditionChecker(pos).Mask(params);
}

void VersionBitsCache::Clear()
{
    for (unsigned int d = 0; d < Consensus::MAX_VERSION_BITS_DEPLOYMENTS; d++) {
        caches[d].clear();
    }
}
