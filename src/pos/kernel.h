// Copyright (c) 2012-2013 The PPCoin developers
// Copyright (c) 2014 The BlackCoin developers
// Copyright (c) 2017 The Particl Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef PPCOIN_KERNEL_H
#define PPCOIN_KERNEL_H

#include "validation.h"


// Compute the hash modifier for proof-of-stake
uint256 ComputeStakeModifierV2(const CBlockIndex *pindexPrev, const uint256 &kernel);

/**
 * Check whether stake kernel meets hash target
 * Sets hashProofOfStake on success return
 */
bool CheckStakeKernelHash(const CBlockIndex *pindexPrev,
    uint32_t nBits, uint32_t nBlockFromTime,
    CAmount prevOutAmount, const COutPoint &prevout, uint32_t nTimeTx,
    uint256 &hashProofOfStake, uint256 &targetProofOfStake,
    bool fPrintProofOfStake=false);


/**
 * Check kernel hash target and coinstake signature
 * Sets hashProofOfStake on success return
 */
bool CheckProofOfStake(const CBlockIndex *pindexPrev, const CTransaction &tx, int64_t nTime, unsigned int nBits, uint256 &hashProofOfStake, uint256 &targetProofOfStake);

/**
 * Check whether the coinstake timestamp meets protocol
 */
bool CheckCoinStakeTimestamp(int nHeight, int64_t nTimeBlock);

/**
 * Wrapper around CheckStakeKernelHash()
 * Also checks existence of kernel input and min age
 * Convenient for searching a kernel
 */
bool CheckKernel(const CBlockIndex *pindexPrev, unsigned int nBits, int64_t nTime, const COutPoint &prevout, int64_t* pBlockTime = nullptr);

#endif // PPCOIN_KERNEL_H
