// Copyright (c) 2016-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/params.h>
#include <deploymentinfo.h>
#include <kernel/chainparams.h>
#include <util/check.h>
#include <versionbits.h>
#include <versionbits_impl.h>

using enum ThresholdState;

std::string StateName(ThresholdState state)
{
    switch (state) {
    case DEFINED: return "defined";
    case STARTED: return "started";
    case LOCKED_IN: return "locked_in";
    case ACTIVE: return "active";
    case FAILED: return "failed";
    }
    return "invalid";
}

ThresholdState AbstractThresholdConditionChecker::GetStateFor(const CBlockIndex* pindexPrev, ThresholdConditionCache& cache) const
{
    int nPeriod = Period();
    int nThreshold = Threshold();
    int min_activation_height = MinActivationHeight();
    int64_t nTimeStart = BeginTime();
    int64_t nTimeTimeout = EndTime();

    // Check if this deployment is always active.
    if (nTimeStart == Consensus::BIP9Deployment::ALWAYS_ACTIVE) {
        return ThresholdState::ACTIVE;
    }

    // Check if this deployment is never active.
    if (nTimeStart == Consensus::BIP9Deployment::NEVER_ACTIVE) {
        return ThresholdState::FAILED;
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
        if (pindexPrev->GetMedianTimePast() < nTimeStart) {
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

    // Now walk forward and compute the state of descendants of pindexPrev
    while (!vToCompute.empty()) {
        ThresholdState stateNext = state;
        pindexPrev = vToCompute.back();
        vToCompute.pop_back();

        switch (state) {
            case ThresholdState::DEFINED: {
                if (pindexPrev->GetMedianTimePast() >= nTimeStart) {
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
                } else if (pindexPrev->GetMedianTimePast() >= nTimeTimeout) {
                    stateNext = ThresholdState::FAILED;
                }
                break;
            }
            case ThresholdState::LOCKED_IN: {
                // Progresses into ACTIVE provided activation height will have been reached.
                if (pindexPrev->nHeight + 1 >= min_activation_height) {
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

BIP9Stats AbstractThresholdConditionChecker::GetStateStatisticsFor(const CBlockIndex* pindex, std::vector<bool>* signalling_blocks) const
{
    BIP9Stats stats = {};

    stats.period = Period();
    stats.threshold = Threshold();

    if (pindex == nullptr) return stats;

    // Find how many blocks are in the current period
    int blocks_in_period = 1 + (pindex->nHeight % stats.period);

    // Reset signalling_blocks
    if (signalling_blocks) {
        signalling_blocks->assign(blocks_in_period, false);
    }

    // Count from current block to beginning of period
    int elapsed = 0;
    int count = 0;
    const CBlockIndex* currentIndex = pindex;
    do {
        ++elapsed;
        --blocks_in_period;
        if (Condition(currentIndex)) {
            ++count;
            if (signalling_blocks) signalling_blocks->at(blocks_in_period) = true;
        }
        currentIndex = currentIndex->pprev;
    } while(blocks_in_period > 0);

    stats.elapsed = elapsed;
    stats.count = count;
    stats.possible = (stats.period - stats.threshold ) >= (stats.elapsed - count);

    return stats;
}

int AbstractThresholdConditionChecker::GetStateSinceHeightFor(const CBlockIndex* pindexPrev, ThresholdConditionCache& cache) const
{
    int64_t start_time = BeginTime();
    if (start_time == Consensus::BIP9Deployment::ALWAYS_ACTIVE || start_time == Consensus::BIP9Deployment::NEVER_ACTIVE) {
        return 0;
    }

    const ThresholdState initialState = GetStateFor(pindexPrev, cache);

    // BIP 9 about state DEFINED: "The genesis block is by definition in this state for each deployment."
    if (initialState == ThresholdState::DEFINED) {
        return 0;
    }

    const int nPeriod = Period();

    // A block's state is always the same as that of the first of its period, so it is computed based on a pindexPrev whose height equals a multiple of nPeriod - 1.
    // To ease understanding of the following height calculation, it helps to remember that
    // right now pindexPrev points to the block prior to the block that we are computing for, thus:
    // if we are computing for the last block of a period, then pindexPrev points to the second to last block of the period, and
    // if we are computing for the first block of a period, then pindexPrev points to the last block of the previous period.
    // The parent of the genesis block is represented by nullptr.
    pindexPrev = Assert(pindexPrev->GetAncestor(pindexPrev->nHeight - ((pindexPrev->nHeight + 1) % nPeriod)));

    const CBlockIndex* previousPeriodParent = pindexPrev->GetAncestor(pindexPrev->nHeight - nPeriod);

    while (previousPeriodParent != nullptr && GetStateFor(previousPeriodParent, cache) == initialState) {
        pindexPrev = previousPeriodParent;
        previousPeriodParent = pindexPrev->GetAncestor(pindexPrev->nHeight - nPeriod);
    }

    // Adjust the result because right now we point to the parent block.
    return pindexPrev->nHeight + 1;
}

BIP9Info VersionBitsCache::Info(const CBlockIndex& block_index, const Consensus::Params& params, Consensus::DeploymentPos id)
{
    BIP9Info result;

    VersionBitsConditionChecker checker(params, id);

    ThresholdState current_state, next_state;

    {
        LOCK(m_mutex);
        current_state = checker.GetStateFor(block_index.pprev, m_caches[id]);
        next_state = checker.GetStateFor(&block_index, m_caches[id]);
        result.since = checker.GetStateSinceHeightFor(block_index.pprev, m_caches[id]);
    }

    result.current_state = StateName(current_state);
    result.next_state = StateName(next_state);

    const bool has_signal = (STARTED == current_state || LOCKED_IN == current_state);
    if (has_signal) {
        result.stats.emplace(checker.GetStateStatisticsFor(&block_index, &result.signalling_blocks));
        if (LOCKED_IN == current_state) {
            result.stats->threshold = 0;
            result.stats->possible = false;
        }
    }

    if (current_state == ACTIVE) {
        result.active_since = result.since;
    } else if (next_state == ACTIVE) {
        result.active_since = block_index.nHeight + 1;
    }

    return result;
}

BIP9GBTStatus VersionBitsCache::GBTStatus(const CBlockIndex& block_index, const Consensus::Params& params)
{
    BIP9GBTStatus result;

    LOCK(m_mutex);
    for (int i = 0; i < (int)Consensus::MAX_VERSION_BITS_DEPLOYMENTS; i++) {
        auto pos = static_cast<Consensus::DeploymentPos>(i);
        VersionBitsConditionChecker checker(params, pos);
        ThresholdState state = checker.GetStateFor(&block_index, m_caches[pos]);
        const VBDeploymentInfo& vbdepinfo = VersionBitsDeploymentInfo[pos];
        BIP9GBTStatus::Info gbtinfo{.bit=params.vDeployments[pos].bit, .mask=checker.Mask(), .gbt_optional_rule=vbdepinfo.gbt_optional_rule};

        switch (state) {
        case DEFINED:
        case FAILED:
            // Not exposed to GBT
            break;
        case STARTED:
            result.signalling.try_emplace(vbdepinfo.name, gbtinfo);
            break;
        case LOCKED_IN:
            result.locked_in.try_emplace(vbdepinfo.name, gbtinfo);
            break;
        case ACTIVE:
            result.active.try_emplace(vbdepinfo.name, gbtinfo);
            break;
        }
    }
    return result;
}

bool VersionBitsCache::IsActiveAfter(const CBlockIndex* pindexPrev, const Consensus::Params& params, Consensus::DeploymentPos pos)
{
    LOCK(m_mutex);
    return ThresholdState::ACTIVE == VersionBitsConditionChecker(params, pos).GetStateFor(pindexPrev, m_caches[pos]);
}

static int32_t ComputeBlockVersion(const CBlockIndex* pindexPrev, const Consensus::Params& params, std::array<ThresholdConditionCache, Consensus::MAX_VERSION_BITS_DEPLOYMENTS>& caches)
{
    int32_t nVersion = VERSIONBITS_TOP_BITS;

    for (int i = 0; i < (int)Consensus::MAX_VERSION_BITS_DEPLOYMENTS; i++) {
        Consensus::DeploymentPos pos = static_cast<Consensus::DeploymentPos>(i);
        VersionBitsConditionChecker checker(params, pos);
        ThresholdState state = checker.GetStateFor(pindexPrev, caches[pos]);
        if (state == ThresholdState::LOCKED_IN || state == ThresholdState::STARTED) {
            nVersion |= checker.Mask();
        }
    }

    return nVersion;
}

int32_t VersionBitsCache::ComputeBlockVersion(const CBlockIndex* pindexPrev, const Consensus::Params& params)
{
    LOCK(m_mutex);
    return ::ComputeBlockVersion(pindexPrev, params, m_caches);
}

void VersionBitsCache::Clear()
{
    LOCK(m_mutex);
    for (unsigned int d = 0; d < Consensus::MAX_VERSION_BITS_DEPLOYMENTS; d++) {
        m_caches[d].clear();
    }
}

namespace {
/**
 * Threshold condition checker that triggers when unknown versionbits are seen on the network.
 */
class WarningBitsConditionChecker : public AbstractThresholdConditionChecker
{
private:
    const Consensus::Params& m_params;
    std::array<ThresholdConditionCache, Consensus::MAX_VERSION_BITS_DEPLOYMENTS>& m_caches;
    int m_bit;
    int period{2016};
    int threshold{1815}; // 90% threshold used in BIP 341

public:
    explicit WarningBitsConditionChecker(const CChainParams& chainparams, std::array<ThresholdConditionCache, Consensus::MAX_VERSION_BITS_DEPLOYMENTS>& caches, int bit)
    : m_params{chainparams.GetConsensus()}, m_caches{caches}, m_bit(bit)
    {
        if (chainparams.IsTestChain()) {
            period = chainparams.GetConsensus().DifficultyAdjustmentInterval();
            threshold = period * 3 / 4; // 75% for test nets per BIP9 suggestion
        }
    }

    int64_t BeginTime() const override { return 0; }
    int64_t EndTime() const override { return std::numeric_limits<int64_t>::max(); }
    int Period() const override { return period; }
    int Threshold() const override { return threshold; }

    bool Condition(const CBlockIndex* pindex) const override
    {
        return pindex->nHeight >= m_params.MinBIP9WarningHeight &&
               ((pindex->nVersion & VERSIONBITS_TOP_MASK) == VERSIONBITS_TOP_BITS) &&
               ((pindex->nVersion >> m_bit) & 1) != 0 &&
               ((::ComputeBlockVersion(pindex->pprev, m_params, m_caches) >> m_bit) & 1) == 0;
    }
};
} // anonymous namespace

std::vector<std::pair<int, bool>> VersionBitsCache::CheckUnknownActivations(const CBlockIndex* pindex, const CChainParams& chainparams)
{
    LOCK(m_mutex);
    std::vector<std::pair<int, bool>> result;
    for (int bit = 0; bit < VERSIONBITS_NUM_BITS; ++bit) {
        WarningBitsConditionChecker checker(chainparams, m_caches, bit);
        ThresholdState state = checker.GetStateFor(pindex, m_warning_caches.at(bit));
        if (state == ACTIVE || state == LOCKED_IN) {
            result.emplace_back(bit, state == ACTIVE);
        }
    }
    return result;
}
