// Copyright (c) 2016-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_VERSIONBITS_IMPL_H
#define BITCOIN_VERSIONBITS_IMPL_H

#include <chain.h>
#include <sync.h>
#include <versionbits.h>

/** BIP 9 defines a finite-state-machine to deploy a softfork in multiple stages.
 *  State transitions happen during retarget period if conditions are met
 *  In case of reorg, transitions can go backward. Without transition, state is
 *  inherited between periods. All blocks of a period share the same state.
 */
enum class ThresholdState : uint8_t {
    DEFINED,   // First state that each softfork starts out as. The genesis block is by definition in this state for each deployment.
    STARTED,   // For blocks past the starttime.
    LOCKED_IN, // For at least one retarget period after the first retarget period with STARTED blocks of which at least threshold have the associated bit set in nVersion, until min_activation_height is reached.
    ACTIVE,    // For all blocks after the LOCKED_IN retarget period (final state)
    FAILED,    // For all blocks once the first retarget period after the timeout time is hit, if LOCKED_IN wasn't already reached (final state)
};

/** Get a string with the state name */
std::string StateName(ThresholdState state);

/**
 * Abstract class that implements BIP9-style threshold logic, and caches results.
 */
class AbstractThresholdConditionChecker {
protected:
    virtual bool Condition(const CBlockIndex* pindex) const =0;
    virtual int64_t BeginTime() const =0;
    virtual int64_t EndTime() const =0;
    virtual int MinActivationHeight() const { return 0; }
    virtual int Period() const =0;
    virtual int Threshold() const =0;

public:
    virtual ~AbstractThresholdConditionChecker() = default;

    /** Returns the numerical statistics of an in-progress BIP9 softfork in the period including pindex
     * If provided, signalling_blocks is set to true/false based on whether each block in the period signalled
     */
    BIP9Stats GetStateStatisticsFor(const CBlockIndex* pindex, std::vector<bool>* signalling_blocks = nullptr) const;
    /** Returns the state for pindex A based on parent pindexPrev B. Applies any state transition if conditions are present.
     *  Caches state from first block of period. */
    ThresholdState GetStateFor(const CBlockIndex* pindexPrev, ThresholdConditionCache& cache) const;
    /** Returns the height since when the ThresholdState has started for pindex A based on parent pindexPrev B, all blocks of a period share the same */
    int GetStateSinceHeightFor(const CBlockIndex* pindexPrev, ThresholdConditionCache& cache) const;
};

/**
 * Class to implement versionbits logic.
 */
class VersionBitsConditionChecker : public AbstractThresholdConditionChecker {
private:
    const Consensus::BIP9Deployment& dep;

protected:
    int64_t BeginTime() const override { return dep.nStartTime; }
    int64_t EndTime() const override { return dep.nTimeout; }
    int MinActivationHeight() const override { return dep.min_activation_height; }
    int Period() const override { return dep.period; }
    int Threshold() const override { return dep.threshold; }

    bool Condition(const CBlockIndex* pindex) const override
    {
        return Condition(pindex->nVersion);
    }

public:
    explicit VersionBitsConditionChecker(const Consensus::BIP9Deployment& dep) : dep{dep} {}
    explicit VersionBitsConditionChecker(const Consensus::Params& params, Consensus::DeploymentPos id) : VersionBitsConditionChecker{params.vDeployments[id]} {}

    uint32_t Mask() const { return (uint32_t{1}) << dep.bit; }

    bool Condition(int32_t nVersion) const
    {
        return (((nVersion & VERSIONBITS_TOP_MASK) == VERSIONBITS_TOP_BITS) && (nVersion & Mask()) != 0);
    }
};

#endif // BITCOIN_VERSIONBITS_IMPL_H
