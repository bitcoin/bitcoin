#ifndef BITCOIN_POS_STAKEMODIFIER_MANAGER_H
#define BITCOIN_POS_STAKEMODIFIER_MANAGER_H

#include <consensus/params.h>
#include <stdint.h>
#include <map>
#include <mutex>
#include <optional>
#include <uint256.h>

class CBlockIndex;

/**
 * Manages the rolling stake modifier used by PoS blocks. The modifier is
 * recomputed at fixed intervals as defined by consensus parameters.
 */
class StakeModifierManager {
public:
    StakeModifierManager();

    /** Return the modifier for a block if known. */
    std::optional<uint256> GetModifier(const uint256& block_hash) const;

    /** Return the current stake modifier. */
    uint256 GetCurrentModifier() const;

    /** Update the modifier on block connect. */
    void UpdateOnConnect(const CBlockIndex* pindex,
                         const Consensus::Params& params);

    /** Remove the modifier on block disconnect. */
    void RemoveOnDisconnect(const CBlockIndex* pindex);

private:
    struct ModifierEntry {
        uint256 modifier;
        int64_t last_update{0};
    };

    std::map<uint256, ModifierEntry> m_modifiers;
    uint256 m_current_modifier;
    uint256 m_current_block_hash;
    std::mutex m_mutex;
};

/** Access the global stake modifier manager instance. */
StakeModifierManager& GetStakeModifierManager();

#endif // BITCOIN_POS_STAKEMODIFIER_MANAGER_H
