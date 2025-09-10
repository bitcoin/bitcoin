#ifndef BITCOIN_NODE_STAKE_MODIFIER_MANAGER_H
#define BITCOIN_NODE_STAKE_MODIFIER_MANAGER_H

#include <map>
#include <mutex>
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
    mutable std::mutex m_mutex;
    std::map<uint256, uint256> m_cache;

public:
    explicit StakeModifierManager(ChainstateManager& chainman) : m_chainman(chainman) {}

    /** Return the stake modifier for the given block hash if known. */
    std::optional<uint256> GetStakeModifier(const uint256& block_hash);

    /** Validate and store a received stake modifier message. */
    bool ProcessStakeModifier(const uint256& block_hash, const uint256& modifier);
};

} // namespace node

#endif // BITCOIN_NODE_STAKE_MODIFIER_MANAGER_H
