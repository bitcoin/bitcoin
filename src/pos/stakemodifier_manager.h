#ifndef BITCOIN_POS_STAKEMODIFIER_MANAGER_H
#define BITCOIN_POS_STAKEMODIFIER_MANAGER_H

#include <consensus/params.h>
#include <stdint.h>
#include <mutex>
#include <uint256.h>

class CBlockIndex;

/**
 * Manages the rolling stake modifier used by PoS blocks. The modifier is
 * recomputed at fixed intervals as defined by consensus parameters.
 */
class StakeModifierManager {
public:
    StakeModifierManager();

    /** Return the current stake modifier. */
    uint256 GetModifier();

    /** Update the modifier if the refresh interval has elapsed. */
    void ComputeNextModifier(const CBlockIndex* pindexPrev, unsigned int nTime,
                             const Consensus::Params& params);

private:
    uint256 m_modifier;
    int64_t m_last_update{0};
    std::mutex m_mutex;
};

/** Access the global stake modifier manager instance. */
StakeModifierManager& GetStakeModifierManager();

#endif // BITCOIN_POS_STAKEMODIFIER_MANAGER_H
