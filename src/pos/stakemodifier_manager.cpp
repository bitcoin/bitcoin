#include <pos/stakemodifier_manager.h>

#include <pos/stakemodifier.h>

// Global instance
static StakeModifierManager g_manager;

StakeModifierManager& GetStakeModifierManager() { return g_manager; }

StakeModifierManager::StakeModifierManager() : m_modifier{} {}

uint256 StakeModifierManager::GetModifier()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_modifier;
}

void StakeModifierManager::ComputeNextModifier(const CBlockIndex* pindexPrev,
                                               unsigned int nTime,
                                               const Consensus::Params& params)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_modifier.IsNull() || static_cast<int64_t>(nTime) - m_last_update >= params.nStakeModifierInterval) {
        m_modifier = ComputeStakeModifier(pindexPrev, m_modifier);
        m_last_update = nTime;
    }
}
