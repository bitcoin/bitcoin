// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Copyright (c) 2015-2017 The Bitcoin Unlimited developers
// Copyright (c) 2017 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "pow.h"

#include "arith_uint256.h"
#include "chain.h"
#include "primitives/block.h"
#include "uint256.h"
#include "util.h"

#ifndef BITCOIN_CASH
uint32_t GetNextWorkRequired(const CBlockIndex *pindexLast, const CBlockHeader *pblock, const Consensus::Params &params)
{
    unsigned int nProofOfWorkLimit = UintToArith256(params.powLimit).GetCompact();

    // Genesis block
    if (pindexLast == nullptr)
        return nProofOfWorkLimit;

    // Only change once per difficulty adjustment interval
    uint32_t nHeight = pindexLast->nHeight + 1;
    if (nHeight % params.DifficultyAdjustmentInterval() == 0)
    {
        // Go back by what we want to be 14 days worth of blocks
        assert(nHeight >= params.DifficultyAdjustmentInterval());
        uint32_t nHeightFirst = nHeight - params.DifficultyAdjustmentInterval();
        const CBlockIndex *pindexFirst = pindexLast->GetAncestor(nHeightFirst);
        assert(pindexFirst);

        return CalculateNextWorkRequired(pindexLast, pindexFirst->GetBlockTime(), params);
    }

    if (params.fPowAllowMinDifficultyBlocks)
    {
        // Special difficulty rule for testnet:
        // If the new block's timestamp is more than 2* 10 minutes then allow
        // mining of a min-difficulty block.
        if (pblock->GetBlockTime() > pindexLast->GetBlockTime() + 2 * params.nPowTargetSpacing)
        {
            return nProofOfWorkLimit;
        }

        // Return the last non-special-min-difficulty-rules-block
        const CBlockIndex *pindex = pindexLast;
        while (pindex->pprev && pindex->nHeight % params.DifficultyAdjustmentInterval() != 0 &&
               pindex->nBits == nProofOfWorkLimit)
        {
            pindex = pindex->pprev;
        }

        return pindex->nBits;
    }

    // Do not go lower than the POW limit.
    uint32_t nBits = pindexLast->nBits;
    if (nBits == nProofOfWorkLimit)
    {
        return nProofOfWorkLimit;
    }

    // If producing the last 6 block took less than 12h, we keep the same difficulty.
    const CBlockIndex *pindex6 = pindexLast->GetAncestor(nHeight - 7);
    assert(pindex6);
    int64_t mtp6blocks = pindexLast->GetMedianTimePast() - pindex6->GetMedianTimePast();
    if (mtp6blocks < 12 * 3600)
    {
        return nBits;
    }

    // If producing the last 6 block took more than 12h, increase the difficulty
    // target by 25% (which reduces the difficulty by 20%). This ensure the
    // chain do not get stuck in case we lose hashrate abruptly.
    arith_uint256 nPow;
    nPow.SetCompact(nBits);
    nPow += (nPow >> 2);

    // Make sure we do not go below allowed values.
    const arith_uint256 bnPowLimit = UintToArith256(params.powLimit);
    if (nPow > bnPowLimit)
        nPow = bnPowLimit;

    return nPow.GetCompact();
}

#else
/**
 * Compute the next required proof of work using the legacy Bitcoin difficulty
 * adjustement + Emergency Difficulty Adjustement (EDA).
 */
static uint32_t GetNextEDAWorkRequired(const CBlockIndex *pindexPrev,
    const CBlockHeader *pblock,
    const Consensus::Params &params)
{
    // Only change once per difficulty adjustment interval
    uint32_t nHeight = pindexPrev->nHeight + 1;
    if (nHeight % params.DifficultyAdjustmentInterval() == 0)
    {
        // Go back by what we want to be 14 days worth of blocks
        assert(nHeight >= params.DifficultyAdjustmentInterval());
        uint32_t nHeightFirst = nHeight - params.DifficultyAdjustmentInterval();
        const CBlockIndex *pindexFirst = pindexPrev->GetAncestor(nHeightFirst);
        assert(pindexFirst);

        return CalculateNextWorkRequired(pindexPrev, pindexFirst->GetBlockTime(), params);
    }

    const uint32_t nProofOfWorkLimit = UintToArith256(params.powLimit).GetCompact();

    if (params.fPowAllowMinDifficultyBlocks)
    {
        // Special difficulty rule for testnet:
        // If the new block's timestamp is more than 2* 10 minutes then allow
        // mining of a min-difficulty block.
        if (pblock->GetBlockTime() > pindexPrev->GetBlockTime() + 2 * params.nPowTargetSpacing)
        {
            return nProofOfWorkLimit;
        }

        // Return the last non-special-min-difficulty-rules-block
        const CBlockIndex *pindex = pindexPrev;
        while (pindex->pprev && pindex->nHeight % params.DifficultyAdjustmentInterval() != 0 &&
               pindex->nBits == nProofOfWorkLimit)
        {
            pindex = pindex->pprev;
        }

        return pindex->nBits;
    }

    // We can't go bellow the minimum, so early bail.
    uint32_t nBits = pindexPrev->nBits;
    if (nBits == nProofOfWorkLimit)
    {
        return nProofOfWorkLimit;
    }

    // If producing the last 6 block took less than 12h, we keep the same
    // difficulty.
    const CBlockIndex *pindex6 = pindexPrev->GetAncestor(nHeight - 7);
    assert(pindex6);
    int64_t mtp6blocks = pindexPrev->GetMedianTimePast() - pindex6->GetMedianTimePast();
    if (mtp6blocks < 12 * 3600)
    {
        return nBits;
    }

    // If producing the last 6 block took more than 12h, increase the difficulty
    // target by 1/4 (which reduces the difficulty by 20%). This ensure the
    // chain do not get stuck in case we lose hashrate abruptly.
    arith_uint256 nPow;
    nPow.SetCompact(nBits);
    nPow += (nPow >> 2);

    // Make sure we do not go bellow allowed values.
    const arith_uint256 bnPowLimit = UintToArith256(params.powLimit);
    if (nPow > bnPowLimit)
        nPow = bnPowLimit;

    return nPow.GetCompact();
}

uint32_t GetNextWorkRequired(const CBlockIndex *pindexPrev, const CBlockHeader *pblock, const Consensus::Params &params)
{
    // Genesis block
    if (pindexPrev == nullptr)
    {
        return UintToArith256(params.powLimit).GetCompact();
    }

    // Special rule for regtest: we never retarget.
    if (params.fPowNoRetargeting)
    {
        return pindexPrev->nBits;
    }

    if (pindexPrev->GetMedianTimePast() >= GetArg("-newdaaactivationtime", params.cashHardForkActivationTime))
    {
        return GetNextCashWorkRequired(pindexPrev, pblock, params);
    }

    return GetNextEDAWorkRequired(pindexPrev, pblock, params);
}
#endif

uint32_t CalculateNextWorkRequired(const CBlockIndex *pindexLast,
    int64_t nFirstBlockTime,
    const Consensus::Params &params)
{
    if (params.fPowNoRetargeting)
    {
        return pindexLast->nBits;
    }

    // Limit adjustment step
    int64_t nActualTimespan = pindexLast->GetBlockTime() - nFirstBlockTime;
    if (nActualTimespan < params.nPowTargetTimespan / 4)
        nActualTimespan = params.nPowTargetTimespan / 4;
    if (nActualTimespan > params.nPowTargetTimespan * 4)
        nActualTimespan = params.nPowTargetTimespan * 4;

    // Retarget
    const arith_uint256 bnPowLimit = UintToArith256(params.powLimit);
    arith_uint256 bnNew;
    arith_uint256 bnOld;
    bnNew.SetCompact(pindexLast->nBits);
    bnOld = bnNew;
    bnNew *= nActualTimespan;
    bnNew /= params.nPowTargetTimespan;

    if (bnNew > bnPowLimit)
        bnNew = bnPowLimit;

    return bnNew.GetCompact();
}

bool CheckProofOfWork(uint256 hash, unsigned int nBits, const Consensus::Params &params)
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

arith_uint256 GetBlockProof(const CBlockIndex &block)
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

int64_t GetBlockProofEquivalentTime(const CBlockIndex &to,
    const CBlockIndex &from,
    const CBlockIndex &tip,
    const Consensus::Params &params)
{
    arith_uint256 r;
    int sign = 1;
    if (to.nChainWork > from.nChainWork)
    {
        r = to.nChainWork - from.nChainWork;
    }
    else
    {
        r = from.nChainWork - to.nChainWork;
        sign = -1;
    }
    r = r * arith_uint256(params.nPowTargetSpacing) / GetBlockProof(tip);
    if (r.bits() > 63)
    {
        return sign * std::numeric_limits<int64_t>::max();
    }
    return sign * r.GetLow64();
}
#ifdef BITCOIN_CASH
/**
 * Compute the a target based on the work done between 2 blocks and the time
 * required to produce that work.
 */
static arith_uint256 ComputeTarget(const CBlockIndex *pindexFirst,
    const CBlockIndex *pindexLast,
    const Consensus::Params &params)
{
    assert(pindexLast->nHeight > pindexFirst->nHeight);

    /**
     * From the total work done and the time it took to produce that much work,
     * we can deduce how much work we expect to be produced in the targeted time
     * between blocks.
     */
    arith_uint256 work = pindexLast->nChainWork - pindexFirst->nChainWork;
    work *= params.nPowTargetSpacing;

    // In order to avoid difficulty cliffs, we bound the amplitude of the
    // adjustement we are going to do.
    int64_t nActualTimespan = int64_t(pindexLast->nTime) - int64_t(pindexFirst->nTime);
    if (nActualTimespan > 288 * params.nPowTargetSpacing)
    {
        nActualTimespan = 288 * params.nPowTargetSpacing;
    }
    else if (nActualTimespan < 72 * params.nPowTargetSpacing)
    {
        nActualTimespan = 72 * params.nPowTargetSpacing;
    }

    work /= nActualTimespan;

    /**
     * We need to compute T = (2^256 / W) - 1 but 2^256 doesn't fit in 256 bits.
     * By expressing 1 as W / W, we get (2^256 - W) / W, and we can compute
     * 2^256 - W as the complement of W.
     */
    return (-work) / work;
}

/**
 * To reduce the impact of timestamp manipulation, we select the block we are
 * basing our computation on via a median of 3.
 */
static const CBlockIndex *GetSuitableBlock(const CBlockIndex *pindex)
{
    assert(pindex->nHeight >= 3);

    /**
     * In order to avoid a block is a very skewed timestamp to have too much
     * influence, we select the median of the 3 top most blocks as a starting
     * point.
     */
    const CBlockIndex *blocks[3];
    blocks[2] = pindex;
    blocks[1] = pindex->pprev;
    blocks[0] = blocks[1]->pprev;

    // Sorting network.
    if (blocks[0]->nTime > blocks[2]->nTime)
    {
        std::swap(blocks[0], blocks[2]);
    }

    if (blocks[0]->nTime > blocks[1]->nTime)
    {
        std::swap(blocks[0], blocks[1]);
    }

    if (blocks[1]->nTime > blocks[2]->nTime)
    {
        std::swap(blocks[1], blocks[2]);
    }

    // We should have our candidate in the middle now.
    return blocks[1];
}

/**
 * Compute the next required proof of work using a weighted average of the
 * estimated hashrate per block.
 *
 * Using a weighted average ensure that the timestamp parameter cancels out in
 * most of the calculation - except for the timestamp of the first and last
 * block. Because timestamps are the least trustworthy information we have as
 * input, this ensures the algorithm is more resistant to malicious inputs.
 */
uint32_t GetNextCashWorkRequired(const CBlockIndex *pindexPrev,
    const CBlockHeader *pblock,
    const Consensus::Params &params)
{
    // This cannot handle the genesis block and early blocks in general.
    assert(pindexPrev);

    // Special difficulty rule for testnet:
    // If the new block's timestamp is more than 2* 10 minutes then allow
    // mining of a min-difficulty block.
    if (params.fPowAllowMinDifficultyBlocks &&
        (pblock->GetBlockTime() > pindexPrev->GetBlockTime() + 2 * params.nPowTargetSpacing))
    {
        return UintToArith256(params.powLimit).GetCompact();
    }

    // Compute the difficulty based on the full adjustement interval.
    const uint32_t nHeight = pindexPrev->nHeight;
    assert(nHeight >= params.DifficultyAdjustmentInterval());

    // Get the last suitable block of the difficulty interval.
    const CBlockIndex *pindexLast = GetSuitableBlock(pindexPrev);
    assert(pindexLast);

    // Get the first suitable block of the difficulty interval.
    uint32_t nHeightFirst = nHeight - 144;
    const CBlockIndex *pindexFirst = GetSuitableBlock(pindexPrev->GetAncestor(nHeightFirst));
    assert(pindexFirst);

    // Compute the target based on time and work done during the interval.
    const arith_uint256 nextTarget = ComputeTarget(pindexFirst, pindexLast, params);

    const arith_uint256 powLimit = UintToArith256(params.powLimit);
    if (nextTarget > powLimit)
    {
        return powLimit.GetCompact();
    }

    return nextTarget.GetCompact();
}
#endif
