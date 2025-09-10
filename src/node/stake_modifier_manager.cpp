#include <node/stake_modifier_manager.h>

#include <algorithm>
#include <chain.h>
#include <pos/stakemodifier.h>
#include <validation.h>
#include <vector>

namespace node {

std::optional<uint256> StakeModifierManager::GetStakeModifier(const uint256& block_hash)
{
    LOCK(cs_main);
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_cache.find(block_hash);
    if (it != m_cache.end()) return it->second;

    const CBlockIndex* index = m_chainman.m_blockman.LookupBlockIndex(block_hash);
    if (index == nullptr) return std::nullopt;

    // Walk back until a cached modifier is found.
    std::vector<const CBlockIndex*> ancestors;
    uint256 modifier{};
    for (const CBlockIndex* p = index->pprev; p; p = p->pprev) {
        auto it2 = m_cache.find(p->GetBlockHash());
        if (it2 != m_cache.end()) {
            modifier = it2->second;
            break;
        }
        ancestors.push_back(p);
    }
    std::reverse(ancestors.begin(), ancestors.end());
    for (const CBlockIndex* ancestor : ancestors) {
        modifier = ComputeStakeModifier(ancestor, modifier);
        m_cache.emplace(ancestor->GetBlockHash(), modifier);
    }
    modifier = ComputeStakeModifier(index->pprev, modifier);
    m_cache.emplace(block_hash, modifier);
    return modifier;
}

bool StakeModifierManager::ProcessStakeModifier(const uint256& block_hash, const uint256& modifier)
{
    LOCK(cs_main);
    std::lock_guard<std::mutex> lock(m_mutex);
    const CBlockIndex* index = m_chainman.m_blockman.LookupBlockIndex(block_hash);
    if (index == nullptr) return false;

    // Determine expected modifier using cached data.
    std::vector<const CBlockIndex*> ancestors;
    uint256 prev_mod{};
    for (const CBlockIndex* p = index->pprev; p; p = p->pprev) {
        auto it = m_cache.find(p->GetBlockHash());
        if (it != m_cache.end()) {
            prev_mod = it->second;
            break;
        }
        ancestors.push_back(p);
    }
    std::reverse(ancestors.begin(), ancestors.end());
    for (const CBlockIndex* ancestor : ancestors) {
        prev_mod = ComputeStakeModifier(ancestor, prev_mod);
        m_cache[ancestor->GetBlockHash()] = prev_mod;
    }
    uint256 expected = ComputeStakeModifier(index->pprev, prev_mod);
    if (expected != modifier) return false;

    m_cache[block_hash] = modifier;
    return true;
}

} // namespace node
