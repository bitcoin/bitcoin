#include <node/stake_modifier_manager.h>

#include <chain.h>
#include <pos/validator.h>
#include <validation.h>

namespace node {

std::optional<uint256> StakeModifierManager::GetStakeModifier(const uint256& block_hash) const
{
    LOCK(cs_main);
    const CBlockIndex* index = m_chainman.m_blockman.LookupBlockIndex(block_hash);
    if (index == nullptr) return std::nullopt;
    return pos::ComputeStakeModifier(*index);
}

void StakeModifierManager::ProcessStakeModifier(const uint256&, const uint256&)
{
    // Stake modifiers are derived locally; no processing required for now.
}

} // namespace node
