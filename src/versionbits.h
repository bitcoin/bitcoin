// Copyright (c) 2016-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_VERSIONBITS_H
#define BITCOIN_VERSIONBITS_H

#include <chain.h>
#include <sync.h>

#include <map>

/** What block version to use for new blocks (pre versionbits) */
static const int32_t VERSIONBITS_LAST_OLD_BLOCK_VERSION = 4;
/** What bits to set in version for versionbits blocks */
static const int32_t VERSIONBITS_TOP_BITS = 0x20000000UL;
/** What bitmask determines whether versionbits is in use */
static const int32_t VERSIONBITS_TOP_MASK = 0xE0000000UL;

/** BIP 9 defines a finite-state-machine to deploy a softfork in multiple stages.
 *  State transitions happen during retarget period if conditions are met
 *  In case of reorg, transitions can go backward. Without transition, state is
 *  inherited between periods. All blocks of a period share the same state.
 */
enum class ThresholdState {
    DEFINED,   // Inactive, waiting for begin time
    STARTED,   // Inactive, waiting for signal/timeout
    LOCKED_IN, // Activation signalled, will be active next period
    ACTIVE,    // Active; will deactivate on signal or timeout
    DEACTIVATING, // Still active, will be abandoned next period
    ABANDONED, // Not active, terminal state
};

// A map that gives the state for blocks whose height is a multiple of Period().
// The map is indexed by the block's parent, however, so all keys in the map
// will either be nullptr or a block with (height + 1) % Period() == 0.
typedef std::map<const CBlockIndex*, ThresholdState> ThresholdConditionCache;

struct SignalInfo
{
    int height;
    int revision; // -1 = this revision
    bool activate; // true = activate, false = abandon
};

/**
 * Abstract class that implements BIP9-style threshold logic, and caches results.
 */
class AbstractThresholdConditionChecker {
protected:
    virtual int64_t BeginTime() const =0;
    virtual int64_t EndTime() const =0;
    virtual int Period() const =0;
    virtual int32_t ActivateVersion() const =0;
    virtual int32_t AbandonVersion() const =0;

public:
    /** Returns the state for pindex A based on parent pindexPrev B. Applies any state transition if conditions are present.
     *  Caches state from first block of period. */
    ThresholdState GetStateFor(const CBlockIndex* pindexPrev, ThresholdConditionCache& cache) const;
    /** Returns the height since when the ThresholdState has started for pindex A based on parent pindexPrev B, all blocks of a period share the same */
    int GetStateSinceHeightFor(const CBlockIndex* pindexPrev, ThresholdConditionCache& cache) const;

    /** Report BINANA id, based on nVersion signalling standard */
    bool BINANA(int& year, int& number, int& revision) const;

    /** Returns signalling information */
    std::vector<SignalInfo> GetSignalInfo(const CBlockIndex* pindex) const;
};

/** BIP 9 allows multiple softforks to be deployed in parallel. We cache
 *  per-period state for every one of them. */
class VersionBitsCache
{
private:
    Mutex m_mutex;
    ThresholdConditionCache m_caches[Consensus::MAX_VERSION_BITS_DEPLOYMENTS] GUARDED_BY(m_mutex);

public:
    /** Get the BIP9 state for a given deployment for the block after pindexPrev. */
    ThresholdState State(const CBlockIndex* pindexPrev, const Consensus::Params& params, Consensus::DeploymentPos pos) EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);

    /** Get the block height at which the BIP9 deployment switched into the state for the block after pindexPrev. */
    int StateSinceHeight(const CBlockIndex* pindexPrev, const Consensus::Params& params, Consensus::DeploymentPos pos) EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);

    /** Determine what nVersion a new block should use
     */
    int32_t ComputeBlockVersion(const CBlockIndex* pindexPrev, const Consensus::Params& params) EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);

    /** Report BINANA details, based on nVersion signalling standard */
    bool BINANA(int& year, int& number, int& revision, const Consensus::Params& params, Consensus::DeploymentPos pos) const;

    /** Returns signalling information */
    std::vector<SignalInfo> GetSignalInfo(const CBlockIndex* pindex, const Consensus::Params& params, Consensus::DeploymentPos pos) const;

    void Clear() EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);
};

#endif // BITCOIN_VERSIONBITS_H
