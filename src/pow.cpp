// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "pow.h"

#include "arith_uint256.h"
#include "chain.h"
#include "consensus/consensus.h"
#include "primitives/block.h"
#include "uint256.h"
#include "util.h"

uint32_t GetNextWorkRequiredLog(const CBlockIndex* pindexLast, const CBlockHeader *pblock, const Consensus::Params& params)
{
    uint32_t nextChallenge = GetNextWorkRequired(pindexLast, pblock, params);
    /// debug print
    LogPrintf("GetNextWorkRequired RETARGET\n");
    LogPrintf("pindexLast->nTime = %d\n", (int64_t)pindexLast->nTime);
    arith_uint256 bnNew, bnOld;
    bnNew.SetCompact(nextChallenge);
    bnOld.SetCompact(pindexLast->nBits);    
    LogPrintf("Before: %08x  %s\n", pindexLast->nBits, bnOld.ToString());
    LogPrintf("After:  %08x  %s\n", nextChallenge, bnNew.ToString());
    return nextChallenge;
}

arith_uint256 GetBlockProof(const CBlockIndex& block)
{
    arith_uint256 bnTarget;
    bool fNegative;
    bool fOverflow;
    bnTarget.SetCompact(block.nBits, &fNegative, &fOverflow);
    if (fNegative || fOverflow || bnTarget == 0)
        return 0;
    // We need to compute 2**256 / (bnTarget+1), but we can't represent 2**256
    // as it's too large for a arith_uint256. However, as 2**256 is at least as large
    // as bnTarget+1, it is equal to ((2**256 - bnTarget - 1) / (bnTarget+1)) + 1,
    // or ~bnTarget / (nTarget+1) + 1.
    return (~bnTarget / (bnTarget + 1)) + 1;
}
