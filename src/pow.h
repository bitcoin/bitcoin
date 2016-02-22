// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_POW_H
#define BITCOIN_POW_H

#include "consensus/params.h"

#include <stdint.h>

class CBlockHeader;
class CBlockIndex;
class uint256;
class arith_uint256;

// Define difficulty retarget algorithms
enum DiffMode {
    DIFF_DEFAULT = 0, // Default to invalid 0
    DIFF_BTC     = 1, // Retarget every x blocks (Bitcoin style)
    DIFF_KGW     = 2, // Retarget using Kimoto Gravity Well
    DIFF_DGW     = 3, // Retarget using Dark Gravity Wave v3
};
unsigned int GetNextWorkRequired(const CBlockIndex* pindexLast, const CBlockHeader *pblock, const Consensus::Params&);
unsigned int CalculateNextWorkRequired(const CBlockIndex* pindexLast, int64_t nFirstBlockTime, const Consensus::Params&);


/** Check whether a block hash satisfies the proof-of-work requirement specified by nBits */
bool CheckProofOfWork(uint256 hash, unsigned int nBits, const Consensus::Params&);
arith_uint256 GetBlockProof(const CBlockIndex& block);

/** Return the time it would take to redo the work difference between from and to, assuming the current hashrate corresponds to the difficulty at tip, in seconds. */
int64_t GetBlockProofEquivalentTime(const CBlockIndex& to, const CBlockIndex& from, const CBlockIndex& tip, const Consensus::Params&);

#endif // BITCOIN_POW_H
