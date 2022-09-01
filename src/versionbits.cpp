// Copyright (c) 2016-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <versionbits.h>

#include <kernel/chainparams.h>
#include <consensus/params.h>
#include <deploymentinfo.h>
#include <util/check.h>

bool AbstractThresholdConditionChecker::BINANA(int& year, int& number, int& revision) const
{
    const int32_t activate = ActivateVersion();
    const int32_t abandon = AbandonVersion();

    if ((activate & ~VERSIONBITS_TOP_MASK) != (abandon & ~VERSIONBITS_TOP_MASK)) {
        return false;
    }
    if ((activate & 0x18000000) != 0) return false;
    if ((activate & VERSIONBITS_TOP_MASK) != VERSIONBITS_TOP_ACTIVE) return false;
    if ((abandon & VERSIONBITS_TOP_MASK) != VERSIONBITS_TOP_ABANDON) return false;

    year = ((activate & 0x07c00000) >> 22) + 2016;
    number = (activate & 0x003fff00) >> 8;
    revision = (activate & 0x000000ff);

    return true;
}

ThresholdState AbstractThresholdConditionChecker::GetStateFor(const CBlockIndex* pindexPrev, ThresholdConditionCache& cache) const
{
    int nPeriod = Period();
    int64_t nTimeStart = BeginTime();
    int64_t nTimeTimeout = EndTime();
    const int32_t activate = ActivateVersion();
    const int32_t abandon = AbandonVersion();

    // Check if this deployment is always active.
    if (nTimeStart == Consensus::HereticalDeployment::ALWAYS_ACTIVE) {
        return ThresholdState::ACTIVE;
    }

    // Check if this deployment is never active.
    if (nTimeStart == Consensus::HereticalDeployment::NEVER_ACTIVE) {
        return ThresholdState::ABANDONED;
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
        if (pindexPrev->GetMedianTimePast() < nTimeStart && pindexPrev->GetMedianTimePast() < nTimeTimeout) {
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

        bool finished = pindexPrev->GetMedianTimePast() >= nTimeTimeout;
        switch (state) {
            case ThresholdState::DEFINED:
                if (finished) {
                    stateNext = ThresholdState::ABANDONED;
                } else if (pindexPrev->GetMedianTimePast() >= nTimeStart) {
                    stateNext = ThresholdState::STARTED;
                }
                break;
            case ThresholdState::STARTED: {
                if (finished) {
                    stateNext = ThresholdState::ABANDONED;
                    break;
                }
                // We need to look for the signal
                const CBlockIndex* pindexCheck = pindexPrev;
                bool sig_active = false;
                bool sig_abandon = false;
                for (int i = 0; i < nPeriod; ++i) {
                    if (pindexCheck->nVersion == abandon) {
                        sig_abandon = true;
                    } else if (pindexCheck->nVersion == activate) {
                        sig_active = true;
                    }
                    pindexCheck = pindexCheck->pprev;
                }
                if (sig_abandon) {
                    stateNext = ThresholdState::ABANDONED;
                } else if (sig_active) {
                    stateNext = ThresholdState::LOCKED_IN;
                }
                break;
            }
            case ThresholdState::LOCKED_IN:
                stateNext = ThresholdState::ACTIVE;
                [[fallthrough]];
            case ThresholdState::ACTIVE: {
                if (finished) {
                    stateNext = ThresholdState::DEACTIVATING;
                    break;
                }
                const CBlockIndex* pindexCheck = pindexPrev;
                for (int i = 0; i < nPeriod; ++i) {
                    if (pindexCheck->nVersion == abandon) {
                        stateNext = ThresholdState::DEACTIVATING;
                        break;
                    }
                    pindexCheck = pindexCheck->pprev;
                }
                break;
            }
            case ThresholdState::DEACTIVATING:
                stateNext = ThresholdState::ABANDONED;
                break;
            case ThresholdState::ABANDONED:
                // Nothing happens, terminal state
                break;
        }
        cache[pindexPrev] = state = stateNext;
    }

    return state;
}

int AbstractThresholdConditionChecker::GetStateSinceHeightFor(const CBlockIndex* pindexPrev, ThresholdConditionCache& cache) const
{
    int64_t start_time = BeginTime();
    if (start_time == Consensus::HereticalDeployment::ALWAYS_ACTIVE || start_time == Consensus::HereticalDeployment::NEVER_ACTIVE) {
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

std::vector<SignalInfo> AbstractThresholdConditionChecker::GetSignalInfo(const CBlockIndex* pindex) const
{
    std::vector<SignalInfo> result;

    int year = 0, number = 0, revision = 0;
    const bool check_other_versions = BINANA(year, number, revision);

    const int32_t activate = ActivateVersion();
    const int32_t abandon = AbandonVersion();
    const int period = Period();

    while (pindex != nullptr) {
        if (pindex->nVersion == activate) {
            result.push_back({ .height = pindex->nHeight, .revision = -1, .activate = true });
        } else if (pindex->nVersion == abandon) {
            result.push_back({ .height = pindex->nHeight, .revision = -1, .activate = false });
        } else if (check_other_versions) {
            if ((pindex->nVersion & 0x07FFFF00l) == (activate & 0x07FFFF00l)) {
                SignalInfo s;
                s.height = pindex->nHeight;
                s.revision = static_cast<uint8_t>(pindex->nVersion & 0xFF);
                if ((pindex->nVersion & 0xF8000000l) == VERSIONBITS_TOP_ACTIVE) {
                    s.activate = true;
                    result.push_back(s);
                } else if ((pindex->nVersion & 0xF8000000l) == VERSIONBITS_TOP_ABANDON) {
                    s.activate = false;
                    result.push_back(s);
                }
            }
        }
        if (pindex->nHeight % period == 0) break;
        pindex = pindex->pprev;
    }
    return result;
}

namespace
{
/**
 * Class to implement versionbits logic.
 */
class VersionBitsConditionChecker : public AbstractThresholdConditionChecker {
private:
    const Consensus::Params& params;
    const Consensus::DeploymentPos id;

protected:
    int64_t BeginTime() const override { return params.vDeployments[id].nStartTime; }
    int64_t EndTime() const override { return params.vDeployments[id].nTimeout; }
    int Period() const override { return params.nMinerConfirmationWindow; }
    int32_t ActivateVersion() const override { return params.vDeployments[id].signal_activate; }
    int32_t AbandonVersion() const override { return params.vDeployments[id].signal_abandon; }

public:
    explicit VersionBitsConditionChecker(const Consensus::Params& params, Consensus::DeploymentPos id_) : params{params}, id(id_) {}
};

} // namespace

ThresholdState VersionBitsCache::State(const CBlockIndex* pindexPrev, const Consensus::Params& params, Consensus::DeploymentPos pos)
{
    LOCK(m_mutex);
    return VersionBitsConditionChecker(params, pos).GetStateFor(pindexPrev, m_caches[pos]);
}

int VersionBitsCache::StateSinceHeight(const CBlockIndex* pindexPrev, const Consensus::Params& params, Consensus::DeploymentPos pos)
{
    LOCK(m_mutex);
    return VersionBitsConditionChecker(params, pos).GetStateSinceHeightFor(pindexPrev, m_caches[pos]);
}

std::vector<SignalInfo> VersionBitsCache::GetSignalInfo(const CBlockIndex* pindex, const Consensus::Params& params, Consensus::DeploymentPos pos) const
{
    return VersionBitsConditionChecker(params, pos).GetSignalInfo(pindex);
}

int32_t VersionBitsCache::ComputeBlockVersion(const CBlockIndex* pindexPrev, const Consensus::Params& params)
{
    int32_t nVersion = VERSIONBITS_TOP_BITS;
    return nVersion;
}

bool VersionBitsCache::BINANA(int& year, int& number, int& revision, const Consensus::Params& params, Consensus::DeploymentPos pos) const
{
    return VersionBitsConditionChecker(params, pos).BINANA(year, number, revision);
}

void VersionBitsCache::Clear()
{
    LOCK(m_mutex);
    for (unsigned int d = 0; d < Consensus::MAX_VERSION_BITS_DEPLOYMENTS; d++) {
        m_caches[d].clear();
    }
}
