// Copyright (c) 2016-2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <versionbits.h>
#include <consensus/params.h>

#include <limits>

static int calculateStartHeight(const CBlockIndex* pindexPrev, ThresholdState state, const int nPeriod, const ThresholdConditionCache& cache) {
    int nStartHeight{std::numeric_limits<int>::max()};

    // we are interested only in state STARTED
    // For state DEFINED: it is not started yet, nothing to do
    // For states LOCKED_IN, FAILED, ACTIVE: it is too late, nothing to do
    while (state == ThresholdState::STARTED) {
        nStartHeight = std::min(pindexPrev->nHeight + 1, nStartHeight);

        // we can walk back here because the only way for STARTED state to exist
        // in cache already is to be calculated in previous runs via "walk forward"
        // loop below starting from DEFINED state.
        pindexPrev = pindexPrev->GetAncestor(pindexPrev->nHeight - nPeriod);
        auto cache_it = cache.find(pindexPrev);
        assert(cache_it != cache.end());

        state = cache_it->second;
    }

    return nStartHeight;
}

ThresholdState AbstractThresholdConditionChecker::GetStateFor(const CBlockIndex* pindexPrev, const Consensus::Params& params, ThresholdConditionCache& cache) const
{
    int nPeriod = Period(params);
    int64_t nTimeStart = BeginTime(params);
    int masternodeStartHeight = SignalHeight(pindexPrev, params);
    int64_t nTimeTimeout = EndTime(params);

    // Check if this deployment is always active.
    if (nTimeStart == Consensus::BIP9Deployment::ALWAYS_ACTIVE) {
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
        if (pindexPrev->GetMedianTimePast() < nTimeStart || pindexPrev->nHeight < masternodeStartHeight) {
            // Optimization: don't recompute down further, as we know every earlier block will be before the start time
            cache[pindexPrev] = ThresholdState::DEFINED;
            break;
        }
        vToCompute.push_back(pindexPrev);
        pindexPrev = pindexPrev->GetAncestor(pindexPrev->nHeight - nPeriod);
    }

    // At this point, cache[pindexPrev] is known
    assert(cache.count(pindexPrev));
    ThresholdState state = cache[pindexPrev];

    int nStartHeight = calculateStartHeight(pindexPrev, state, nPeriod, cache);

    // Now walk forward and compute the state of descendants of pindexPrev
    while (!vToCompute.empty()) {
        ThresholdState stateNext = state;
        pindexPrev = vToCompute.back();
        vToCompute.pop_back();

        switch (state) {
            case ThresholdState::DEFINED: {
                if (pindexPrev->GetMedianTimePast() >= nTimeTimeout) {
                    stateNext = ThresholdState::FAILED;
                } else if (pindexPrev->GetMedianTimePast() >= nTimeStart && pindexPrev->nHeight >= masternodeStartHeight) {
                    stateNext = ThresholdState::STARTED;
                    nStartHeight = pindexPrev->nHeight + 1;
                }
                break;
            }
            case ThresholdState::STARTED: {
                if (pindexPrev->GetMedianTimePast() >= nTimeTimeout) {
                    stateNext = ThresholdState::FAILED;
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
                assert(nStartHeight > 0 && nStartHeight < std::numeric_limits<int>::max());
                int nAttempt = (pindexCount->nHeight + 1 - nStartHeight) / nPeriod;
                if (count >= Threshold(params, nAttempt)) {
                    stateNext = ThresholdState::LOCKED_IN;
                }
                break;
            }
            case ThresholdState::LOCKED_IN: {
                // Always progresses into ACTIVE.
                stateNext = ThresholdState::ACTIVE;
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

BIP9Stats AbstractThresholdConditionChecker::GetStateStatisticsFor(const CBlockIndex* pindex, const Consensus::Params& params, ThresholdConditionCache& cache) const
{
    BIP9Stats stats = {};

    stats.period = Period(params);
    stats.threshold = Threshold(params, 0);

    if (pindex == nullptr)
        return stats;

    // Find beginning of period
    const CBlockIndex* pindexEndOfPrevPeriod = pindex->GetAncestor(pindex->nHeight - ((pindex->nHeight + 1) % stats.period));
    stats.elapsed = pindex->nHeight - pindexEndOfPrevPeriod->nHeight;

    // Re-calculate current threshold
    int nAttempt{0};
    const ThresholdState state = GetStateFor(pindexEndOfPrevPeriod, params, cache);
    if (state == ThresholdState::STARTED) {
        int nStartHeight = GetStateSinceHeightFor(pindexEndOfPrevPeriod, params, cache);
        nAttempt = (pindexEndOfPrevPeriod->nHeight + 1 - nStartHeight)/stats.period;
    }
    stats.threshold = Threshold(params, nAttempt);

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
    int64_t start_time = BeginTime(params);
    if (start_time == Consensus::BIP9Deployment::ALWAYS_ACTIVE) {
        return 0;
    }

    const ThresholdState initialState = GetStateFor(pindexPrev, params, cache);

    // BIP 9 about state DEFINED: "The genesis block is by definition in this state for each deployment."
    if (initialState == ThresholdState::DEFINED) {
        return 0;
    }

    const int nPeriod = Period(params);

    // A block's state is always the same as that of the first of its period, so it is computed based on a pindexPrev whose height equals a multiple of nPeriod - 1.
    // To ease understanding of the following height calculation, it helps to remember that
    // right now pindexPrev points to the block prior to the block that we are computing for, thus:
    // if we are computing for the last block of a period, then pindexPrev points to the second to last block of the period, and
    // if we are computing for the first block of a period, then pindexPrev points to the last block of the previous period.
    // The parent of the genesis block is represented by nullptr.
    pindexPrev = pindexPrev->GetAncestor(pindexPrev->nHeight - ((pindexPrev->nHeight + 1) % nPeriod));

    const CBlockIndex* previousPeriodParent = pindexPrev->GetAncestor(pindexPrev->nHeight - nPeriod);

    while (previousPeriodParent != nullptr && GetStateFor(previousPeriodParent, params, cache) == initialState) {
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
    int64_t BeginTime(const Consensus::Params& params) const override { return params.vDeployments[id].nStartTime; }
    int SignalHeight(const CBlockIndex* const pindexPrev, const Consensus::Params& params) const override {
        const auto& deployment = params.vDeployments[id];
        if (!deployment.useEHF) {
            return 0;
        }
        // ehfManager should be initialized before first usage of VersionBitsConditionChecker
        const auto ehfManagerPtr = AbstractEHFManager::getInstance();
        const auto signals = ehfManagerPtr->GetSignalsStage(pindexPrev);
        const auto it = signals.find(deployment.bit);
        if (it == signals.end()) {
            return std::numeric_limits<int>::max();
        }

        return it->second;
    }
    int64_t EndTime(const Consensus::Params& params) const override { return params.vDeployments[id].nTimeout; }
    int Period(const Consensus::Params& params) const override { return params.vDeployments[id].nWindowSize ? params.vDeployments[id].nWindowSize : params.nMinerConfirmationWindow; }
    int Threshold(const Consensus::Params& params, int nAttempt) const override
    {
        if (params.vDeployments[id].nThresholdStart == 0) {
            return params.nRuleChangeActivationThreshold;
        }
        if (params.vDeployments[id].nThresholdMin == 0 || params.vDeployments[id].nFalloffCoeff == 0) {
            return params.vDeployments[id].nThresholdStart;
        }
        int64_t nThresholdCalc = params.vDeployments[id].nThresholdStart - nAttempt * nAttempt * Period(params) / 100 / params.vDeployments[id].nFalloffCoeff;
        return std::max(params.vDeployments[id].nThresholdMin, nThresholdCalc);
    }

    bool Condition(const CBlockIndex* pindex, const Consensus::Params& params) const override
    {
        return (((pindex->nVersion & VERSIONBITS_TOP_MASK) == VERSIONBITS_TOP_BITS) && (pindex->nVersion & Mask(params)) != 0);
    }

public:
    explicit VersionBitsConditionChecker(Consensus::DeploymentPos id_) : id(id_) {}
    uint32_t Mask(const Consensus::Params& params) const { return ((uint32_t)1) << params.vDeployments[id].bit; }
};

} // namespace

ThresholdState VersionBitsCache::State(const CBlockIndex* pindexPrev, const Consensus::Params& params, Consensus::DeploymentPos pos)
{
    LOCK(m_mutex);
    return VersionBitsConditionChecker(pos).GetStateFor(pindexPrev, params, m_caches[pos]);
}

BIP9Stats VersionBitsCache::Statistics(const CBlockIndex* pindexPrev, const Consensus::Params& params, Consensus::DeploymentPos pos)
{
    LOCK(m_mutex);
    return VersionBitsConditionChecker(pos).GetStateStatisticsFor(pindexPrev, params, m_caches[pos]);
}

int VersionBitsCache::StateSinceHeight(const CBlockIndex* pindexPrev, const Consensus::Params& params, Consensus::DeploymentPos pos)
{
    LOCK(m_mutex);
    return VersionBitsConditionChecker(pos).GetStateSinceHeightFor(pindexPrev, params, m_caches[pos]);
}

uint32_t VersionBitsCache::Mask(const Consensus::Params& params, Consensus::DeploymentPos pos)
{
    return VersionBitsConditionChecker(pos).Mask(params);
}

int32_t VersionBitsCache::ComputeBlockVersion(const CBlockIndex* pindexPrev, const Consensus::Params& params)
{
    LOCK(m_mutex);
    int32_t nVersion = VERSIONBITS_TOP_BITS;

    for (int i = 0; i < (int)Consensus::MAX_VERSION_BITS_DEPLOYMENTS; i++) {
        Consensus::DeploymentPos pos = static_cast<Consensus::DeploymentPos>(i);
        ThresholdState state = VersionBitsConditionChecker(pos).GetStateFor(pindexPrev, params, m_caches[pos]);
        if (state == ThresholdState::LOCKED_IN || state == ThresholdState::STARTED) {
            nVersion |= Mask(params, pos);
        }
    }

    return nVersion;
}

void VersionBitsCache::Clear()
{
    LOCK(m_mutex);
    for (unsigned int d = 0; d < Consensus::MAX_VERSION_BITS_DEPLOYMENTS; d++) {
        m_caches[d].clear();
    }
}
AbstractEHFManager* AbstractEHFManager::globalInstance{nullptr};
