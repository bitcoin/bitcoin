// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <pow.h>

#include <arith_uint256.h>
#include <chain.h>
#include <consensus/params.h>
#include <primitives/block.h>
#include <uint256.h>
#include <validation.h>
#include <chainparams.h>



// MBC Edit

unsigned int GetNextWorkRequired(const CBlockIndex* pindexLast, const CBlockHeader *pblock, const Consensus::Params& params)
{
    assert(pindexLast != nullptr);

    // MBC Edit
    if (IsHardForkEnabled(pindexLast->nHeight + 1, params))
    {
        if ((pindexLast->nHeight + 1) < (params.MBCHeight + params.MBCPowLimitWindow)) {
           return UintToArith256(params.powLimitMBCStart).GetCompact();
        } else {
           return GetNextWorkRequiredByDAA(pindexLast, pblock, params);
        }
    }


    unsigned int nProofOfWorkLimit = UintToArith256(params.powLimit).GetCompact();

    // Only change once per difficulty adjustment interval
    if ((pindexLast->nHeight+1) % params.DifficultyAdjustmentInterval()/5 != 0)
    {
        if (params.fPowAllowMinDifficultyBlocks)
        {
            // Special difficulty rule for testnet:
            // If the new block's timestamp is more than 2* 10 minutes
            // then allow mining of a min-difficulty block.
            if (pblock->GetBlockTime() > pindexLast->GetBlockTime() + params.nPowTargetSpacing*2)
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

    // Limit adjustment step
    int64_t nActualTimespan = pindexLast->GetBlockTime() - nFirstBlockTime;
    if (nActualTimespan < params.nPowTargetTimespan/4)
        nActualTimespan = params.nPowTargetTimespan/4;
    if (nActualTimespan > params.nPowTargetTimespan*4)
        nActualTimespan = params.nPowTargetTimespan*4;

    // Retarget
    const arith_uint256 bnPowLimit = UintToArith256(params.powLimit);
    arith_uint256 bnNew;
    bnNew.SetCompact(pindexLast->nBits);
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



/**
 * Compute a target base on the work done between two blocks and the time required to produce that work.
 */
static arith_uint256 ComputeTarget(const CBlockIndex* pindexFirst,
                                   const CBlockIndex* pindexLast,
                                   const Consensus::Params& params) {
    assert(pindexLast->nHeight > pindexFirst->nHeight);
    arith_uint256 work = pindexLast->nChainWork - pindexFirst->nChainWork;
    work *= params.nPowTargetSpacing;

    int64_t nActualTimespan = int64_t(pindexLast->nTime) - int64_t(pindexFirst->nTime);

    if (nActualTimespan > 1440 * params.nPowTargetSpacing) { // 2 days
        nActualTimespan = 1440 * params.nPowTargetSpacing;
    } else if (nActualTimespan < 360 * params.nPowTargetSpacing) {  // 0.5 days
        nActualTimespan = 360 * params.nPowTargetSpacing;
    }

    /**
     * We need to compute T = (2^256 / W) - 1 but 2^256 doesn't fit in 256 bits.
     * By expressing 1 as W / W, we get (2^256 - W) / W, and we can compute
     * 2^256 - W as the complement of W. 2^256 - w = (-w)
     */
    work /= nActualTimespan;

    return (-work) /work;

}

/**
 * To lower the impact of skewed timstamps, we choose the median of the 3 top
 * most blocks as a starting point
 */

static const CBlockIndex* GetSuitableBlock(const CBlockIndex* pindexLast) {
    const CBlockIndex* nBlocks[3];
    nBlocks[2] = pindexLast;
    nBlocks[1] = pindexLast->pprev;
    nBlocks[0] = nBlocks[1]->pprev;

    if (nBlocks[0]->nTime > nBlocks[2]->nTime) {
        std::swap(nBlocks[0], nBlocks[2]);
    }
    if (nBlocks[0]->nTime > nBlocks[1]->nTime) {
        std::swap(nBlocks[0], nBlocks[1]);
    }
    if (nBlocks[1]->nTime > nBlocks[2]->nTime) {
        std::swap(nBlocks[1], nBlocks[2]);
    }

    return nBlocks[1];
}

/**
 * Compute the next required proof of work using a dynamic difficulty adjustment of the
 * estimated hashrate per block
 *
 */
uint32_t GetNextWorkRequiredByDAA(const CBlockIndex* pindexLast, const CBlockHeader* pblock, const Consensus::Params& params) {
    assert(pindexLast != nullptr);

    // Special difficulty rule for testnet:
    uint32_t nProofOfWorkLimit = UintToArith256(params.powLimitMBCStart).GetCompact();
    if (params.fPowAllowMinDifficultyBlocks) {
        // If the new block's timestamp is more than 2 * (nPowTargetSpaceing / 60) minutes
        // then allow mining of a min-difficulty block.
        if (pblock->GetBlockTime() > pindexLast->GetBlockTime() + params.nPowTargetSpacing * 2) {
            return nProofOfWorkLimit;
        }
    }
    // Get the last suitable block to lower the impact of skewed timestamps
    const CBlockIndex* pindexLastSuitable = GetSuitableBlock(pindexLast);
    assert(pindexLastSuitable);

    // Get the first suitable block of the difficulty interval (1 day)
    const uint32_t nHeight = pindexLast->nHeight;
    uint32_t nHeightFirstSuitable = nHeight - 720;
    const CBlockIndex* pindexFirstSuitable = GetSuitableBlock(pindexLast->GetAncestor(nHeightFirstSuitable));
    assert(pindexFirstSuitable);

    // Compute the target based on time and work done
    arith_uint256 nextTarget = ComputeTarget(pindexFirstSuitable, pindexLastSuitable, params);
    arith_uint256 powLimit = UintToArith256(params.powLimitMBCStart);
    if (nextTarget > powLimit) {
        nextTarget = powLimit;
    }
    return nextTarget.GetCompact();
}
