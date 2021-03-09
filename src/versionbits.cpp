// Copyright (c) 2016-2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <versionbits.h>
#include <consensus/params.h>

ThresholdState ThresholdConditionChecker::GetStateFor(const CBlockIndex* pindexPrev, ThresholdConditionCache& cache) const
{
    int nPeriod = m_dep.m_period;
    int nThreshold = m_dep.threshold;
    int64_t height_start = m_dep.startheight;
    int64_t height_timeout = m_dep.timeoutheight;
    int64_t height_active_min = m_dep.m_min_activation_height;

    // Check if this deployment is never active.
    if (height_start == Consensus::VBitsDeployment::NEVER_ACTIVE && height_timeout == Consensus::VBitsDeployment::NEVER_ACTIVE) {
        return ThresholdState::DEFINED;
    }

    // Check if this deployment is always active.
    if (height_start == Consensus::VBitsDeployment::ALWAYS_ACTIVE) {
        return ThresholdState::ACTIVE;
    }

    // A block's state is always the same as that of the first of its period, so it is computed based on a pindexPrev whose height equals a multiple of nPeriod - 1.
    if (pindexPrev != nullptr) {
        pindexPrev = pindexPrev->GetAncestor(pindexPrev->nHeight - ((pindexPrev->nHeight + 1) % nPeriod));
    }

    // Walk backwards in steps of nPeriod to find a pindexPrev whose information is known
    std::vector<const CBlockIndex*> vToCompute;
    while (cache.count(pindexPrev) == 0) {
        if (pindexPrev == nullptr) {
            // The genesis block is by definition defined.
            cache[pindexPrev] = ThresholdState::DEFINED;
            break;
        }

        // We track state by previous-block, so the height we should be comparing is +1
        if (pindexPrev->nHeight + 1 < height_start) {
            // Optimization: don't recompute down further, as we know every earlier block will be before the start height
            cache[pindexPrev] = ThresholdState::DEFINED;
            break;
        }
        vToCompute.push_back(pindexPrev);
        pindexPrev = pindexPrev->GetAncestor(pindexPrev->nHeight - nPeriod);
    }

    // At this point, cache[pindexPrev] is known
    assert(cache.count(pindexPrev));
    ThresholdState state = cache[pindexPrev];

    // Now walk forward and compute the state of descendants of pindexPrev
    while (!vToCompute.empty()) {
        ThresholdState stateNext = state;
        pindexPrev = vToCompute.back();
        vToCompute.pop_back();

        // We track state by previous-block, so the height we should be comparing is +1
        const int64_t height = pindexPrev->nHeight + 1;

        switch (state) {
            case ThresholdState::DEFINED: {
                if (height >= height_timeout) {
                    stateNext = ThresholdState::FAILED;
                } else if (height >= height_start) {
                    stateNext = ThresholdState::STARTED;
                }
                break;
            }
            case ThresholdState::STARTED: {
                // We need to count
                const CBlockIndex* pindexCount = pindexPrev;
                int count = 0;
                for (int i = 0; i < nPeriod; i++) {
                    if (Condition(pindexCount)) {
                        count++;
                    }
                    pindexCount = pindexCount->pprev;
                }
                if (count >= nThreshold) {
                    stateNext = ThresholdState::LOCKED_IN;
                } else if (height >= height_timeout) {
                    stateNext = ThresholdState::FAILED;
                }
                break;
            }
            case ThresholdState::LOCKED_IN: {
                // Only progress into ACTIVE if minimum activation height has been reached
                if (height >= height_active_min) {
                    stateNext = ThresholdState::ACTIVE;
                }
                break;
            }
            case ThresholdState::FAILED:
            case ThresholdState::ACTIVE: {
                // Nothing happens, these are terminal states.
                break;
            }
        }
        cache[pindexPrev] = state = stateNext;
    }

    return state;
}

VBitsStats ThresholdConditionChecker::GetStateStatisticsFor(const CBlockIndex* pindexPrev) const
{
    VBitsStats stats = {};

    stats.period = m_dep.m_period;
    stats.threshold = m_dep.threshold;

    // We track state by previous-block, so the height we should be comparing is +1
    const int64_t height = pindexPrev ? pindexPrev->nHeight + 1 : 0;

    if (pindexPrev == nullptr || height < stats.period) {
        // genesis block or first retarget period is DEFINED, and will
        // not give a valid pindexEndOfPrevPeriod
        return stats;
    }

    // Find end of previous period -- may be pindexPrev itself
    const CBlockIndex* pindexEndOfPrevPeriod = pindexPrev->GetAncestor(pindexPrev->nHeight - (height % stats.period));

    stats.elapsed = pindexPrev->nHeight - pindexEndOfPrevPeriod->nHeight;

    // Count from current block to beginning of period
    int count = 0;
    const CBlockIndex* currentIndex = pindexPrev;
    while (pindexEndOfPrevPeriod->nHeight != currentIndex->nHeight){
        if (Condition(currentIndex))
            count++;
        currentIndex = currentIndex->pprev;
    }

    stats.count = count;
    stats.possible = (stats.period - stats.threshold) >= (stats.elapsed - count);

    return stats;
}

int ThresholdConditionChecker::GetStateSinceHeightFor(const CBlockIndex* pindexPrev, ThresholdConditionCache& cache) const
{
    int64_t height_start = m_dep.startheight;
    if (height_start == Consensus::VBitsDeployment::ALWAYS_ACTIVE) {
        return 0;
    }

    const ThresholdState initialState = GetStateFor(pindexPrev, cache);

    // BIP 9 about state DEFINED: "The genesis block is by definition in this state for each deployment."
    if (initialState == ThresholdState::DEFINED) {
        return 0;
    }

    const int nPeriod = m_dep.m_period;

    // A block's state is always the same as that of the first of its period, so it is computed based on a pindexPrev whose height equals a multiple of nPeriod - 1.
    // To ease understanding of the following height calculation, it helps to remember that
    // right now pindexPrev points to the block prior to the block that we are computing for, thus:
    // if we are computing for the last block of a period, then pindexPrev points to the second to last block of the period, and
    // if we are computing for the first block of a period, then pindexPrev points to the last block of the previous period.
    // The parent of the genesis block is represented by nullptr.
    pindexPrev = pindexPrev->GetAncestor(pindexPrev->nHeight - ((pindexPrev->nHeight + 1) % nPeriod));

    const CBlockIndex* previousPeriodParent = pindexPrev->GetAncestor(pindexPrev->nHeight - nPeriod);

    while (previousPeriodParent != nullptr && GetStateFor(previousPeriodParent, cache) == initialState) {
        pindexPrev = previousPeriodParent;
        previousPeriodParent = pindexPrev->GetAncestor(pindexPrev->nHeight - nPeriod);
    }

    // Adjust the result because right now we point to the parent block.
    return pindexPrev->nHeight + 1;
}

bool ThresholdConditionChecker::Condition(const CBlockIndex* pindex) const
{
    return (((pindex->nVersion & VERSIONBITS_TOP_MASK) == VERSIONBITS_TOP_BITS) && (pindex->nVersion & Mask()) != 0);
}

namespace {
/**
 * Class to implement versionbits logic.
 */
class VersionBitsConditionChecker : public ThresholdConditionChecker {
public:
    explicit VersionBitsConditionChecker(const Consensus::Params& params, Consensus::DeploymentPos id) : ThresholdConditionChecker(params.m_deployments.at(id)) { }
};
} // namespace

ThresholdState VersionBitsState(const CBlockIndex* pindexPrev, const Consensus::Params& params, Consensus::DeploymentPos pos, VersionBitsCache& cache)
{
    return VersionBitsConditionChecker(params, pos).GetStateFor(pindexPrev, cache.caches[pos]);
}

VBitsStats VersionBitsStatistics(const CBlockIndex* pindexPrev, const Consensus::Params& params, Consensus::DeploymentPos pos)
{
    return VersionBitsConditionChecker(params, pos).GetStateStatisticsFor(pindexPrev);
}

int VersionBitsStateSinceHeight(const CBlockIndex* pindexPrev, const Consensus::Params& params, Consensus::DeploymentPos pos, VersionBitsCache& cache)
{
    return VersionBitsConditionChecker(params, pos).GetStateSinceHeightFor(pindexPrev, cache.caches[pos]);
}

int32_t VersionBitsMask(const Consensus::Params& params, Consensus::DeploymentPos pos)
{
    return VersionBitsConditionChecker(params, pos).Mask();
}

void VersionBitsCache::Clear()
{
    caches.clear();
}
