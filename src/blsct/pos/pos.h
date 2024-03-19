// Copyright (c) 2024 The Navcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BLSCT_POS_H
#define BLSCT_POS_H

#include <blsct/pos/helpers.h>
#include <chain.h>
#include <node/blockstorage.h>

namespace blsct {
const CBlockIndex* GetLastBlockIndex(const CBlockIndex* pindex, bool fProofOfStake);
unsigned int GetNextTargetRequired(const CBlockIndex* pindexLast, const Consensus::Params& params);
static bool GetLastStakeModifier(const CBlockIndex* pindex, uint64_t& nStakeModifier, int64_t& nModifierTime);
static int64_t GetStakeModifierSelectionIntervalSection(int nSection, const Consensus::Params& params);
static int64_t GetStakeModifierSelectionInterval(const Consensus::Params& params);
static bool SelectBlockFromCandidates(std::vector<std::pair<int64_t, uint256>>& vSortedByTimestamp, std::map<uint256, const CBlockIndex*>& mapSelectedBlocks,
                                      int64_t nSelectionIntervalStop, uint64_t nStakeModifierPrev, const CBlockIndex** pindexSelected, const Consensus::Params& params, const node::BlockManager& m_blockman);
bool ComputeNextStakeModifier(const CBlockIndex* pindexPrev, uint64_t& nStakeModifier, bool& fGeneratedStakeModifier, const Consensus::Params& params, const node::BlockManager& m_blockman);
std::vector<unsigned char> CalculateSetMemProofRandomness(const CBlockIndex& pindexPrev);
blsct::Message CalculateSetMemProofGeneratorSeed(const CBlockIndex& pindexPrev);
uint256 CalculateKernelHash(const CBlockIndex& pindexPrev, const CBlock& block);
} // namespace blsct

#endif // BLSCT_POS_H