// Copyright (c) 2024 The Navcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BLSCT_POS_H
#define BLSCT_POS_H

#include <blsct/pos/helpers.h>
#include <chain.h>

namespace node {
class BlockManager;
}

namespace blsct {
const CBlockIndex* GetLastBlockIndex(const CBlockIndex* pindex, bool fProofOfStake);
unsigned int GetNextTargetRequired(const CBlockIndex* pindexLast, const CBlockHeader* pblock, const Consensus::Params& params);
unsigned int CalculateNextTargetRequired(const CBlockIndex* pindexLast, int64_t nFirstBlockTime, const Consensus::Params& params);
bool GetLastStakeModifier(const CBlockIndex* pindex, uint64_t& nStakeModifier, int64_t& nModifierTime) EXCLUSIVE_LOCKS_REQUIRED(cs_main);
int64_t GetStakeModifierSelectionIntervalSection(int nSection, const Consensus::Params& params);
int64_t GetStakeModifierSelectionInterval(const Consensus::Params& params);
std::vector<unsigned char> CalculateSetMemProofRandomness(const CBlockIndex& pindexPrev);
blsct::Message CalculateSetMemProofGeneratorSeed(const CBlockIndex& pindexPrev);
uint256 CalculateKernelHash(const CBlockIndex& pindexPrev, const CBlock& block);
} // namespace blsct

#endif // BLSCT_POS_H