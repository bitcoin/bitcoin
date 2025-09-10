#ifndef BITCOIN_NODE_STAKE_MODIFIER_MANAGER_H
#define BITCOIN_NODE_STAKE_MODIFIER_MANAGER_H

#include <optional>
#include <uint256.h>

class ChainstateManager;

namespace node {

/**
 * Simple manager for retrieving stake modifier data.
 */
class StakeModifierManager
{
private:
    ChainstateManager& m_chainman;

public:
    explicit StakeModifierManager(ChainstateManager& chainman) : m_chainman(chainman) {}

    /** Return the stake modifier for the given block hash if known. */
    std::optional<uint256> GetStakeModifier(const uint256& block_hash) const;

    /** Process a received stake modifier message (currently a no-op). */
    void ProcessStakeModifier(const uint256& block_hash, const uint256& modifier);
};

} // namespace node

#endif // BITCOIN_NODE_STAKE_MODIFIER_MANAGER_H
