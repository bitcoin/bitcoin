// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "pow.h"

#include "arith_uint256.h"
#include "chain.h"
#include "primitives/block.h"
#include "uint256.h"

unsigned int PowGetNextWorkRequired(const void* indexObject, const BlockIndexInterface& iBlockIndex, const CBlockHeader *pblock, const Consensus::Params& params)
{
    unsigned int nProofOfWorkLimit = UintToArith256(params.powLimit).GetCompact();

    // Genesis block
    if (indexObject == NULL)
        return nProofOfWorkLimit;

    // Only change once per difficulty adjustment interval
    if ((iBlockIndex.GetHeight(indexObject)+1) % params.DifficultyAdjustmentInterval() != 0)
    {
        if (params.fPowAllowMinDifficultyBlocks)
        {
            // Special difficulty rule for testnet:
            // If the new block's timestamp is more than 2* 10 minutes
            // then allow mining of a min-difficulty block.
            if (pblock->GetBlockTime() > iBlockIndex.GetTime(indexObject) + params.nPowTargetSpacing*2)
                return nProofOfWorkLimit;
            else
            {
                // Return the last non-special-min-difficulty-rules-block
                const void* pindex = indexObject;
                while (iBlockIndex.GetPrev(pindex) &&
                       iBlockIndex.GetHeight(pindex) % params.DifficultyAdjustmentInterval() != 0 && iBlockIndex.GetBits(pindex) == nProofOfWorkLimit)
                    pindex = iBlockIndex.GetPrev(pindex);
                return iBlockIndex.GetBits(pindex);
            }
        }
        return iBlockIndex.GetBits(indexObject);
    }

    // Go back by what we want to be 14 days worth of blocks
    int nHeightFirst = iBlockIndex.GetHeight(indexObject) - (params.DifficultyAdjustmentInterval()-1);
    assert(nHeightFirst >= 0);
    const void* pindexFirst = iBlockIndex.GetAncestor(indexObject, nHeightFirst);
    assert(pindexFirst);

    return PowCalculateNextWorkRequired(indexObject, iBlockIndex, iBlockIndex.GetTime(pindexFirst), params);
}

unsigned int PowCalculateNextWorkRequired(const void* indexObject, const BlockIndexInterface& iBlockIndex, int64_t nFirstBlockTime, const Consensus::Params& params)
{
    if (params.fPowNoRetargeting)
        return iBlockIndex.GetBits(indexObject);

    // Limit adjustment step
    int64_t nActualTimespan = iBlockIndex.GetTime(indexObject) - nFirstBlockTime;
    if (nActualTimespan < params.nPowTargetTimespan/4)
        nActualTimespan = params.nPowTargetTimespan/4;
    if (nActualTimespan > params.nPowTargetTimespan*4)
        nActualTimespan = params.nPowTargetTimespan*4;

    // Retarget
    const arith_uint256 bnPowLimit = UintToArith256(params.powLimit);
    arith_uint256 bnNew;
    bnNew.SetCompact(iBlockIndex.GetBits(indexObject));
    bnNew *= nActualTimespan;
    bnNew /= params.nPowTargetTimespan;

    if (bnNew > bnPowLimit)
        bnNew = bnPowLimit;

    return bnNew.GetCompact();
}

bool CheckProofOfWork(uint256 hash, unsigned int nBits, const Consensus::Params& params)
{
    bool fNegative;
    bool fOverflow;
    arith_uint256 bnTarget;

    bnTarget.SetCompact(nBits, &fNegative, &fOverflow);

    // Check range
    if (fNegative || bnTarget == 0 || fOverflow || bnTarget > UintToArith256(params.powLimit))
        return false;

    // Check proof of work matches claimed amount
    if (UintToArith256(hash) > bnTarget)
        return false;

    return true;
}
