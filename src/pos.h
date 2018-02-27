// Copyright (c) 2015-2016 The Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef POS_H
#define POS_H

#include "consensus/validation.h"

#include <stdint.h>
#include "util.h"
class CBlockIndex;
class CCoins;
class COutPoint;
class uint256;
class CTransaction;

extern const int STAKE_TIMESTAMP_MASK;
/** Compute the hash modifier for proof-of-stake */
uint256 ComputeStakeModifier(const CBlockIndex* pindexPrev, const uint256& kernel);

// Check whether the coinstake timestamp meets protocol
bool CheckCoinStakeTimestamp(int64_t nTimeBlock, int64_t nTimeTx);
bool CheckStakeKernelHash(const CBlockIndex* pindexPrev, unsigned int nBits, const CCoins* txPrev, const COutPoint& prevout, unsigned int nTimeTx);
bool CheckKernel(CBlockIndex* pindexPrev, unsigned int nBits, int64_t nTime, const COutPoint& prevout, int64_t* pBlockTime = NULL);
bool CheckProofOfStake(CBlockIndex* pindexPrev, const CTransaction& tx, unsigned int nBits, CValidationState &state);
bool VerifySignature(const CTransaction& txFrom, const CTransaction& txTo, unsigned int nIn, unsigned int flags, int nHashType);
#endif // POS_H
