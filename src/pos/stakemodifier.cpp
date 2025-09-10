#include <pos/stakemodifier.h>

#include <chain.h>
#include <hash.h>
#include <mutex>

namespace {
uint256 g_stake_modifier;
int64_t g_modifier_time{0};
std::mutex g_mutex;
}

uint256 ComputeStakeModifier(const CBlockIndex* pindexPrev, const uint256& prev_modifier)
{
    HashWriter ss;
    ss << prev_modifier;
    if (pindexPrev) {
        ss << pindexPrev->GetBlockHash() << pindexPrev->nHeight << pindexPrev->nTime;
    }
    return ss.GetHash();
}

uint256 GetStakeModifier(const CBlockIndex* pindexPrev, unsigned int nTime, const Consensus::Params& params)
{
    std::lock_guard<std::mutex> lock(g_mutex);
    if (g_stake_modifier.IsNull() || static_cast<int64_t>(nTime) - g_modifier_time >= params.nStakeModifierInterval) {
        g_stake_modifier = ComputeStakeModifier(pindexPrev, g_stake_modifier);
        g_modifier_time = nTime;
    }
    return g_stake_modifier;
}
