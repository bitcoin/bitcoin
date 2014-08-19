// Copyright (c) 2012-2013 The PPCoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef PPCOIN_KERNEL_H
#define PPCOIN_KERNEL_H

#include "main.h"
#include "wallet.h"

// ChainDB upgrade time
extern unsigned int nModifierUpgradeTime;

// MODIFIER_INTERVAL: time to elapse before new modifier is computed
extern unsigned int nModifierInterval;

extern bool fCoinsDataActual;

// MODIFIER_INTERVAL_RATIO:
// ratio of group interval length between the last group and the first group
static const int MODIFIER_INTERVAL_RATIO = 3;

// Whether the given block is subject to new modifier protocol
bool IsFixedModifierInterval(unsigned int nTimeBlock);

// Compute the hash modifier for proof-of-stake
bool ComputeNextStakeModifier(const CBlockIndex* pindexCurrent, uint64& nStakeModifier, bool& fGeneratedStakeModifier);

// The stake modifier used to hash for a stake kernel is chosen as the stake
// modifier about a selection interval later than the coin generating the kernel
bool GetKernelStakeModifier(uint256 hashBlockFrom, uint64& nStakeModifier);

// Check whether stake kernel meets hash target
// Sets hashProofOfStake on success return
bool CheckStakeKernelHash(unsigned int nBits, const CBlock& blockFrom, unsigned int nTxPrevOffset, const CTransaction& txPrev, const COutPoint& prevout, unsigned int nTimeTx, uint256& hashProofOfStake, uint256& targetProofOfStake, bool fPrintProofOfStake=false);

// Preloaded coins set
typedef set<pair<const CWalletTx*,unsigned int> > CoinsSet;

// Preloaded coins metadata (block header and stake modifier)
typedef map<uint256, pair<CTxIndex, pair<CBlock, uint64> > > MetaMap;

// Coins scanning options
typedef struct KernelSearchSettings {
    unsigned int nBits;           // Packed difficulty
    unsigned int nTime;           // Basic time
    unsigned int nOffset;         // Offset inside CoinsSet (isn't used yet)
    unsigned int nLimit;          // Coins to scan (isn't used yet)
    unsigned int nSearchInterval; // Number of seconds allowed to go into the past
} KernelSearchSettings;

// Scan given coins set for kernel solution
bool ScanForStakeKernelHash(CoinsSet &setCoins, MetaMap &mapMeta, KernelSearchSettings &settings, CoinsSet::value_type &kernelcoin, unsigned int &nTimeTx, unsigned int &nBlockTime);

// Check kernel hash target and coinstake signature
// Sets hashProofOfStake on success return
bool CheckProofOfStake(const CTransaction& tx, unsigned int nBits, uint256& hashProofOfStake, uint256& targetProofOfStake);

// Get stake modifier checksum
unsigned int GetStakeModifierChecksum(const CBlockIndex* pindex);

// Check stake modifier hard checkpoints
bool CheckStakeModifierCheckpoints(int nHeight, unsigned int nStakeModifierChecksum);

// Get time weight using supplied timestamps
int64 GetWeight(int64 nIntervalBeginning, int64 nIntervalEnd);

#endif // PPCOIN_KERNEL_H
