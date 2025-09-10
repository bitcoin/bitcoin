#include <pos/stakemodifier_manager.h>

#include <pos/stakemodifier.h>

// Global instance
static StakeModifierManager g_manager;

StakeModifierManager& GetStakeModifierManager() { return g_manager; }

StakeModifierManager::StakeModifierManager() : m_current_modifier{}, m_current_block_hash{} {}

std::optional<uint256> StakeModifierManager::GetModifier(const uint256& block_hash) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_modifiers.find(block_hash);
    if (it == m_modifiers.end()) return std::nullopt;
    return it->second.modifier;
}

uint256 StakeModifierManager::GetCurrentModifier() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_current_modifier;
}

void StakeModifierManager::UpdateOnConnect(const CBlockIndex* pindex,
                                           const Consensus::Params& params)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    ModifierEntry entry{};
    if (pindex->pprev) {
        auto it = m_modifiers.find(pindex->pprev->GetBlockHash());
        if (it != m_modifiers.end()) entry = it->second;
    }
    if (entry.modifier.IsNull() || static_cast<int64_t>(pindex->nTime) - entry.last_update >= params.nStakeModifierInterval) {
        entry.modifier = ComputeStakeModifier(pindex->pprev, entry.modifier);
        entry.last_update = pindex->nTime;
    }
    m_current_modifier = entry.modifier;
    m_current_block_hash = pindex->GetBlockHash();
    m_modifiers[m_current_block_hash] = entry;
}

void StakeModifierManager::RemoveOnDisconnect(const CBlockIndex* pindex)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    const uint256 hash = pindex->GetBlockHash();
    m_modifiers.erase(hash);
    if (m_current_block_hash == hash) {
        if (pindex->pprev) {
            m_current_block_hash = pindex->pprev->GetBlockHash();
            auto it = m_modifiers.find(m_current_block_hash);
            if (it != m_modifiers.end()) {
                m_current_modifier = it->second.modifier;
            } else {
                m_current_modifier.SetNull();
            }
        } else {
            m_current_block_hash.SetNull();
            m_current_modifier.SetNull();
        }
    }
}
