// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <pow.h>

#include <arith_uint256.h>
#include <chain.h>
#include <primitives/block.h>
#include <uint256.h>

#include <btv_const.h>

unsigned int GetNextWorkRequired(const CBlockIndex* pindexLast, const CBlockHeader *pblock, const Consensus::Params& params)
{
    assert(pindexLast != nullptr);
    int nCurrentHeight = pindexLast->nHeight + 1;
    unsigned int nProofOfWorkLimit = (nCurrentHeight >= BTV_BRANCH_HEIGHT) ? UintToArith256(uint256S(BTV_BRANCH_POW_LIMIT)).GetCompact() : UintToArith256(params.powLimit).GetCompact();

    if ((nCurrentHeight >= BTV_BRANCH_HEIGHT) && (nCurrentHeight <= BTV_BRANCH_HEIGHT_WINDOW)) return nProofOfWorkLimit;
    if (params.fPowNoRetargeting) return pindexLast->nBits;
    
    // Only change once per difficulty adjustment interval
    if ((pindexLast->nHeight+1) % params.DifficultyAdjustmentInterval() != 0)
    {
        if (params.fPowAllowMinDifficultyBlocks)
        {
            // Special difficulty rule for testnet:
            // If the new block's timestamp is more than 2* 10 minutes
            // then allow mining of a min-difficulty block.
            int64_t nPowTargetSpacing = (nCurrentHeight > BTV_BRANCH_HEIGHT) ? params.nBtvPowTargetSpacing : params.nPowTargetSpacing;
            if (pblock->GetBlockTime() > pindexLast->GetBlockTime() + nPowTargetSpacing*2)
                return nProofOfWorkLimit;
            else
            {
                // Return the last non-special-min-difficulty-rules-block
                const CBlockIndex* pindex = pindexLast;
                while (pindex->pprev && pindex->nHeight % params.DifficultyAdjustmentInterval() != 0 && pindex->nBits == nProofOfWorkLimit)
                    pindex = pindex->pprev;
                return pindex->nBits;
            }
        }
        return pindexLast->nBits;
    }

    // Go back by what we want to be 14 days worth of blocks
    int nHeightFirst = pindexLast->nHeight - (params.DifficultyAdjustmentInterval()-1);
    assert(nHeightFirst >= 0);
    const CBlockIndex* pindexFirst = pindexLast->GetAncestor(nHeightFirst);
    assert(pindexFirst);

    return CalculateNextWorkRequired(pindexLast, pindexFirst->GetBlockTime(), params);
}

unsigned int CalculateNextWorkRequired(const CBlockIndex* pindexLast, int64_t nFirstBlockTime, const Consensus::Params& params)
{
    if (params.fPowNoRetargeting)
        return pindexLast->nBits;

    int64_t nPowTargetTimespan = (pindexLast->nHeight >= BTV_BRANCH_HEIGHT) ? params.nBtvPowTargetTimespan : params.nPowTargetTimespan;

    // Limit adjustment step
    int64_t nActualTimespan = pindexLast->GetBlockTime() - nFirstBlockTime;
    if (nActualTimespan < nPowTargetTimespan/4)
        nActualTimespan = nPowTargetTimespan/4;
    if (nActualTimespan > nPowTargetTimespan*4)
        nActualTimespan = nPowTargetTimespan*4;

    // Retarget
    const arith_uint256 bnPowLimit = ((pindexLast->nHeight + 1) >= BTV_BRANCH_HEIGHT) ? UintToArith256(uint256S(BTV_BRANCH_POW_LIMIT)) : UintToArith256(params.powLimit);
    arith_uint256 bnNew;
    bnNew.SetCompact(pindexLast->nBits);
    bnNew *= nActualTimespan;
    bnNew /= nPowTargetTimespan;

    if (bnNew > bnPowLimit)
        bnNew = bnPowLimit;

    return bnNew.GetCompact();
}

bool CheckProofOfWork(uint256 hash, unsigned int nBits, const Consensus::Params& params, bool bBranched)
{
    bool fNegative;
    bool fOverflow;
    arith_uint256 bnTarget;

    bnTarget.SetCompact(nBits, &fNegative, &fOverflow);

    const arith_uint256 bnPowLimit = bBranched ? UintToArith256(uint256S(BTV_BRANCH_POW_LIMIT)) : UintToArith256(params.powLimit);
    // Check range
    if (fNegative || bnTarget == 0 || fOverflow || bnTarget > bnPowLimit)
        return false;

    // Check proof of work matches claimed amount
    if (UintToArith256(hash) > bnTarget)
        return false;

    return true;
}
