// Copyright (c) 2016-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_VERSIONBITS_H
#define BITCOIN_VERSIONBITS_H

#include <chain.h>
#include <sync.h>

#include <array>
#include <map>
#include <optional>
#include <vector>

class CChainParams;

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
    LOCKED_IN, // For at least one retarget period after the first retarget period with STARTED blocks of which at least threshold have the associated bit set in nVersion, until min_activation_height is reached.
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
    uint32_t period{0};
    /** Number of blocks with the version bit set required to activate the softfork */
    uint32_t threshold{0};
    /** Number of blocks elapsed since the beginning of the current period */
    uint32_t elapsed{0};
    /** Number of blocks with the version bit set since the beginning of the current period */
    uint32_t count{0};
    /** False if there are not enough blocks left in this period to pass activation threshold */
    bool possible{false};
};

/** Detailed status of an enabled BIP9 deployment */
struct BIP9Info {
    int since{0};
    std::string current_state{};
    std::string next_state{};
    std::optional<BIP9Stats> stats;
    std::vector<bool> signalling_blocks;
    std::optional<int> active_since;
};

struct BIP9GBTStatus {
    struct Info {
        int bit;
        uint32_t mask;
        bool gbt_force;
    };
    std::map<std::string, const Info> signalling, locked_in, active;
};

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

/** BIP 9 allows multiple softforks to be deployed in parallel. We cache
 *  per-period state for every one of them. */
class VersionBitsCache
{
private:
    Mutex m_mutex;
    std::array<ThresholdConditionCache,VERSIONBITS_NUM_BITS> m_warning_caches GUARDED_BY(m_mutex);
    std::array<ThresholdConditionCache,Consensus::MAX_VERSION_BITS_DEPLOYMENTS> m_caches GUARDED_BY(m_mutex);

public:
    BIP9Info Info(const CBlockIndex& block_index, const Consensus::Params& params, Consensus::DeploymentPos id) EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);

    BIP9GBTStatus GBTStatus(const CBlockIndex& block_index, const Consensus::Params& params) EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);

    /** Get the BIP9 state for a given deployment for the block after pindexPrev. */
    bool IsActiveAfter(const CBlockIndex* pindexPrev, const Consensus::Params& params, Consensus::DeploymentPos pos) EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);

    /** Determine what nVersion a new block should use */
    int32_t ComputeBlockVersion(const CBlockIndex* pindexPrev, const Consensus::Params& params) EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);

    std::vector<std::pair<int,bool>> CheckUnknownActivations(const CBlockIndex* pindex, const CChainParams& chainparams) EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);

    void Clear() EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);
};

#endif // BITCOIN_VERSIONBITS_H
