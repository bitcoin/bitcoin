// Copyright (c) 2016-2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_VERSIONBITS_H
#define BITCOIN_VERSIONBITS_H

#include <chain.h>
#include <gsl/pointers.h>
#include <sync.h>

#include <map>

/** What block version to use for new blocks (pre versionbits) */
static const int32_t VERSIONBITS_LAST_OLD_BLOCK_VERSION = 4;
/** What bits to set in version for versionbits blocks */
static const int32_t VERSIONBITS_TOP_BITS = 0x20000000UL;
/** What bitmask determines whether versionbits is in use */
static const int32_t VERSIONBITS_TOP_MASK = 0xE0000000UL;
/** Total bits available for versionbits */
static const int32_t VERSIONBITS_NUM_BITS = 29;

/** BIP 9 defines a finite-state-machine to deploy a softfork in multiple stages.
 *  State transitions happen during retarget period if conditions are met
 *  In case of reorg, transitions can go backward. Without transition, state is
 *  inherited between periods. All blocks of a period share the same state.
 */
enum class ThresholdState {
    DEFINED,   // First state that each softfork starts out as. The genesis block is by definition in this state for each deployment.
    STARTED,   // For blocks past the starttime.
    LOCKED_IN, // For one retarget period after the first retarget period with STARTED blocks of which at least threshold have the associated bit set in nVersion.
    ACTIVE,    // For all blocks after the LOCKED_IN retarget period (final state)
    FAILED,    // For all blocks once the first retarget period after the timeout time is hit, if LOCKED_IN wasn't already reached (final state)
};

// A map that gives the state for blocks whose height is a multiple of Period().
// The map is indexed by the block's parent, however, so all keys in the map
// will either be nullptr or a block with (height + 1) % Period() == 0.
typedef std::map<const CBlockIndex*, ThresholdState> ThresholdConditionCache;

/** Display status of an in-progress BIP9 softfork */
struct BIP9Stats {
    /** Length of blocks of the BIP9 signalling period */
    int period;
    /** Number of blocks with the version bit set required to activate the softfork */
    int threshold;
    /** Number of blocks elapsed since the beginning of the current period */
    int elapsed;
    /** Number of blocks with the version bit set since the beginning of the current period */
    int count;
    /** False if there are not enough blocks left in this period to pass activation threshold */
    bool possible;
};

/**
 * Abstract class that implements BIP9-style threshold logic, and caches results.
 */
class AbstractThresholdConditionChecker {
protected:
    virtual bool Condition(const CBlockIndex* pindex, const Consensus::Params& params) const =0;
    virtual int64_t BeginTime(const Consensus::Params& params) const =0;
    virtual int SignalHeight(const CBlockIndex* pindexPrev, const Consensus::Params& params) const = 0;
    virtual int64_t EndTime(const Consensus::Params& params) const =0;
    virtual int Period(const Consensus::Params& params) const =0;
    virtual int Threshold(const Consensus::Params& params, int nAttempt) const =0;

public:
    /** Returns the numerical statistics of an in-progress BIP9 softfork in the current period */
    BIP9Stats GetStateStatisticsFor(const CBlockIndex* pindex, const Consensus::Params& params, ThresholdConditionCache& cache) const;
    /** Returns the state for pindex A based on parent pindexPrev B. Applies any state transition if conditions are present.
     *  Caches state from first block of period. */
    ThresholdState GetStateFor(const CBlockIndex* pindexPrev, const Consensus::Params& params, ThresholdConditionCache& cache) const;
    /** Returns the height since when the ThresholdState has started for pindex A based on parent pindexPrev B, all blocks of a period share the same */
    int GetStateSinceHeightFor(const CBlockIndex* pindexPrev, const Consensus::Params& params, ThresholdConditionCache& cache) const;
};

/** BIP 9 allows multiple softforks to be deployed in parallel. We cache
 *  per-period state for every one of them. */
class VersionBitsCache
{
private:
    Mutex m_mutex;
    ThresholdConditionCache m_caches[Consensus::MAX_VERSION_BITS_DEPLOYMENTS] GUARDED_BY(m_mutex);

public:
    /** Get the numerical statistics for a given deployment for the signalling period that includes the block after pindexPrev. */
    BIP9Stats Statistics(const CBlockIndex* pindexPrev, const Consensus::Params& params, Consensus::DeploymentPos pos);

    static uint32_t Mask(const Consensus::Params& params, Consensus::DeploymentPos pos);

    /** Get the BIP9 state for a given deployment for the block after pindexPrev. */
    ThresholdState State(const CBlockIndex* pindexPrev, const Consensus::Params& params, Consensus::DeploymentPos pos);

    /** Get the block height at which the BIP9 deployment switched into the state for the block after pindexPrev. */
    int StateSinceHeight(const CBlockIndex* pindexPrev, const Consensus::Params& params, Consensus::DeploymentPos pos);

    /** Determine what nVersion a new block should use
     */
    int32_t ComputeBlockVersion(const CBlockIndex* pindexPrev, const Consensus::Params& params);

    void Clear();
};

class AbstractEHFManager
{
public:
    using Signals = std::unordered_map<uint8_t, int>;

    /**
     * getInstance() is used in versionbit because it is non-trivial
     * to get access to NodeContext from all usages of VersionBits* methods
     * For simplification of interface this methods static/global variable is used
     * to get access to EHF data
     */
public:
    [[nodiscard]] static gsl::not_null<AbstractEHFManager*> getInstance() {
        return globalInstance;
    };

    /**
     * `GetSignalsStage' prepares signals for new block.
     * The results are diffent with GetFromCache results due to one more
     * stage of processing: signals that would be expired in next block
     * are excluded from results.
     * This member function is not const because it calls non-const GetFromCache()
     */
    virtual Signals GetSignalsStage(const CBlockIndex* const pindexPrev) = 0;


protected:
    static AbstractEHFManager* globalInstance;
};

#endif // BITCOIN_VERSIONBITS_H
