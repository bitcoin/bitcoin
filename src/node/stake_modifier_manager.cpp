#include <node/stake_modifier_manager.h>

#include <algorithm>
#include <chain.h>
#include <pos/stakemodifier.h>
#include <validation.h>
#include <vector>

namespace node {

std::optional<uint256> StakeModifierManager::GetStakeModifier(const uint256& block_hash) const
{
    LOCK(cs_main);
    const CBlockIndex* index = m_chainman.m_blockman.LookupBlockIndex(block_hash);
    if (index == nullptr) return std::nullopt;

    // Walk back to genesis collecting ancestors so we can compute the
    // modifier chain from the beginning.
    std::vector<const CBlockIndex*> ancestors;
    for (const CBlockIndex* p = index->pprev; p; p = p->pprev) {
        ancestors.push_back(p);
    }
    std::reverse(ancestors.begin(), ancestors.end());

    uint256 modifier{};
    for (const CBlockIndex* ancestor : ancestors) {
        modifier = ComputeStakeModifier(ancestor, modifier);
    }
    return modifier;
}

void StakeModifierManager::ProcessStakeModifier(const uint256&, const uint256&)
{
    // Stake modifiers are derived locally; no processing required for now.
}

} // namespace node
