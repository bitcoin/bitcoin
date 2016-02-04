// Copyright (c) 2012-2013 The PPCoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef PPCOIN_KERNEL_H
#define PPCOIN_KERNEL_H

#include "main.h"
#include "core.h"

// To decrease granularity of timestamp
// Supposed to be 2^n-1
static const int STAKE_TIMESTAMP_MASK = 15;

// MODIFIER_INTERVAL: time to elapse before new modifier is computed
extern unsigned int nModifierInterval;

// MODIFIER_INTERVAL_RATIO:
// ratio of group interval length between the last group and the first group
static const int MODIFIER_INTERVAL_RATIO = 3;

// Compute the hash modifier for proof-of-stake
bool ComputeNextStakeModifier(const CBlockIndex* pindexPrev, uint64_t& nStakeModifier, bool& fGeneratedStakeModifier);

bool ComputeNextStakeModifierThin(const CBlockThinIndex* pindexPrev, uint64_t& nStakeModifier, bool& fGeneratedStakeModifier);

// Check whether stake kernel meets hash target
// Sets hashProofOfStake on success return
bool CheckStakeKernelHash(int nPrevHeight, CStakeModifier* pStakeMod, unsigned int nBits, const CBlock& blockFrom, unsigned int nTxPrevOffset, const CTransaction& txPrev, const COutPoint& prevout, unsigned int nTimeTx, uint256& hashProofOfStake, uint256& targetProofOfStake, bool fPrintProofOfStake);

// Check kernel hash target and coinstake signature
// Sets hashProofOfStake on success return
bool CheckProofOfStake(CBlockIndex* pindexPrev, const CTransaction& tx, unsigned int nBits, uint256& hashProofOfStake, uint256& targetProofOfStake);

// Check whether the coinstake timestamp meets protocol
bool CheckCoinStakeTimestamp(int nHeight, int64_t nTimeBlock, int64_t nTimeTx);

// Get time weight using supplied timestamps 
int64_t GetWeight(int64_t nIntervalBeginning, int64_t nIntervalEnd); // TODO: posv2 remove

// Wrapper around CheckStakeKernelHash()
// Also checks existence of kernel input and min age
// Convenient for searching a kernel
bool CheckKernel(CBlockIndex* pindexPrev, unsigned int nBits, int64_t nTime, const COutPoint& prevout, int64_t* pBlockTime = NULL);


#endif // PPCOIN_KERNEL_H
