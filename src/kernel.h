// Copyright (c) 2012-2020 The Peercoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef PEERCOIN_KERNEL_H
#define PEERCOIN_KERNEL_H

#include <primitives/transaction.h> // CTransaction(Ref)

class CBlockIndex;
class CValidationState;
class CBlockHeader;
class CBlock;


// MODIFIER_INTERVAL_RATIO:
// ratio of group interval length between the last group and the first group
static const int MODIFIER_INTERVAL_RATIO = 3;

// Protocol switch time of v0.3 kernel protocol
extern unsigned int nProtocolV03SwitchTime;
extern unsigned int nProtocolV03TestSwitchTime;

// Whether a given coinstake is subject to new v0.3 protocol
bool IsProtocolV03(unsigned int nTimeCoinStake);
// Whether a given block is subject to new v0.4 protocol
bool IsProtocolV04(unsigned int nTimeBlock);
// Whether a given transaction is subject to new v0.5 protocol
bool IsProtocolV05(unsigned int nTimeTx);
// Whether a given block is subject to new v0.6 protocol
// Test against previous block index! (always available)
bool IsProtocolV06(const CBlockIndex *pindexPrev);
// Whether a given transaction is subject to new v0.7 protocol
bool IsProtocolV07(unsigned int nTimeTx);
// Whether a given block is subject to new BIPs from bitcoin 0.16.x
bool IsBTC16BIPsEnabled(uint32_t nTimeTx);

// Compute the hash modifier for proof-of-stake
bool ComputeNextStakeModifier(const CBlockIndex* pindexCurrent, uint64_t& nStakeModifier, bool& fGeneratedStakeModifier);

// Check whether stake kernel meets hash target
// Sets hashProofOfStake on success return
bool CheckStakeKernelHash(unsigned int nBits, CBlockIndex* pindexPrev, const CBlockHeader& blockFrom, unsigned int nTxPrevOffset, const CTransactionRef& txPrev, const COutPoint& prevout, unsigned int nTimeTx, uint256& hashProofOfStake, bool fPrintProofOfStake=false);

// Check kernel hash target and coinstake signature
// Sets hashProofOfStake on success return
bool CheckProofOfStake(CValidationState &state, CBlockIndex* pindexPrev, const CTransactionRef &tx, unsigned int nBits, uint256& hashProofOfStake);

// Check whether the coinstake timestamp meets protocol
bool CheckCoinStakeTimestamp(int64_t nTimeBlock, int64_t nTimeTx);

// Get stake modifier checksum
unsigned int GetStakeModifierChecksum(const CBlockIndex* pindex);

// Check stake modifier hard checkpoints
bool CheckStakeModifierCheckpoints(int nHeight, unsigned int nStakeModifierChecksum);

bool IsSuperMajority(int minVersion, const CBlockIndex* pstart, unsigned int nRequired, unsigned int nToCheck);

// peercoin: entropy bit for stake modifier if chosen by modifier
unsigned int GetStakeEntropyBit(const CBlock& block);

#endif // PEERCOIN_KERNEL_H
