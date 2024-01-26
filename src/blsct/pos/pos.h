// Copyright (c) 2024 The Navcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BLSCT_POS_H
#define BLSCT_POS_H

#include <chain.h>
#include <node/blockstorage.h>

#define MODIFIER_INTERVAL_RATIO 3

namespace blsct {
unsigned int GetNextTargetRequired(const CBlockIndex* pindexLast, const Consensus::Params& params);
bool ComputeNextStakeModifier(const CBlockIndex* pindexPrev, uint64_t& nStakeModifier, bool& fGeneratedStakeModifier, const Consensus::Params& params, const node::BlockManager& m_blockman);
std::vector<unsigned char> CalculateSetMemProofRandomness(const CBlockIndex& pindexPrev);
} // namespace blsct

#endif // BLSCT_POS_H