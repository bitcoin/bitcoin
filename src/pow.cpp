// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <pow.h>

#include <arith_uint256.h>
#include <chain.h>
#include <primitives/block.h>
#include <uint256.h>

#include <util.h>

unsigned int GetNextWorkRequiredBTC(const CBlockIndex* pindexLast, const CBlockHeader *pblock, const Consensus::Params& params)
{
    assert(pindexLast != nullptr);
    unsigned int nProofOfWorkLimit = UintToArith256(params.powLimit).GetCompact();

    // Only change once per difficulty adjustment interval
    if ((pindexLast->nHeight+1) % params.DifficultyAdjustmentInterval() != 0)
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

unsigned int static KimotoGravityWell(const CBlockIndex* pindexLast, const CBlockHeader *pblock, const Consensus::Params& params, uint64_t TargetBlocksSpacingSeconds, uint64_t PastBlocksMin, uint64_t PastBlocksMax) {
    const CBlockIndex  *BlockLastSolved             = pindexLast;
    const CBlockIndex  *BlockReading                = pindexLast;
    const CBlockHeader *BlockCreating               = pblock;
    BlockCreating               = BlockCreating;
    uint64_t              PastBlocksMass              = 0;
    int64_t               PastRateActualSeconds       = 0;
    int64_t               PastRateTargetSeconds       = 0;
    int64_t               LatestBlockTime             = BlockLastSolved->GetBlockTime();
    double              PastRateAdjustmentRatio     = double(1);
    arith_uint256       PastDifficultyAverage;
    arith_uint256       PastDifficultyAveragePrev;
    double              EventHorizonDeviation;
    double              EventHorizonDeviationFast;
    double              EventHorizonDeviationSlow;
    arith_uint256 bnProofOfWorkLimit = UintToArith256(params.powLimit);

    if (BlockLastSolved == NULL || BlockLastSolved->nHeight == 0 || (uint64_t)BlockLastSolved->nHeight < PastBlocksMin) { return bnProofOfWorkLimit.GetCompact(); }

    for (unsigned int i = 1; BlockReading && BlockReading->nHeight > 0; i++) {
        if (PastBlocksMax > 0 && i > PastBlocksMax) { break; }
        PastBlocksMass++;

        PastDifficultyAverage.SetCompact(BlockReading->nBits);
        if (i > 1) {
            // Dash: handle negative arith_uint256
            if(PastDifficultyAverage >= PastDifficultyAveragePrev)
                PastDifficultyAverage = ((PastDifficultyAverage - PastDifficultyAveragePrev) / i) + PastDifficultyAveragePrev;
            else
                PastDifficultyAverage = PastDifficultyAveragePrev - ((PastDifficultyAveragePrev - PastDifficultyAverage) / i);
        }

        PastDifficultyAveragePrev = PastDifficultyAverage;

        if (LatestBlockTime < BlockReading->GetBlockTime()) { LatestBlockTime = BlockReading->GetBlockTime(); }
        PastRateActualSeconds           = LatestBlockTime - BlockReading->GetBlockTime();
        PastRateTargetSeconds           = TargetBlocksSpacingSeconds * PastBlocksMass;
        PastRateAdjustmentRatio         = double(1);
        if (PastRateActualSeconds < 1) { PastRateActualSeconds = 1; }
        if (PastRateActualSeconds != 0 && PastRateTargetSeconds != 0) {
            PastRateAdjustmentRatio         = double(PastRateTargetSeconds) / double(PastRateActualSeconds);
        }
        EventHorizonDeviation           = 1 + (0.7084 * pow((double(PastBlocksMass)/double(9)), -1.228));
        EventHorizonDeviationFast       = EventHorizonDeviation;
        EventHorizonDeviationSlow       = 1 / EventHorizonDeviation;

        if (PastBlocksMass >= PastBlocksMin) {
            if ((PastRateAdjustmentRatio <= EventHorizonDeviationSlow) || (PastRateAdjustmentRatio >= EventHorizonDeviationFast)) { assert(BlockReading); break; }
        }
        if (BlockReading->pprev == NULL) { assert(BlockReading); break; }
        BlockReading = BlockReading->pprev;
    }

    arith_uint256 bnNew(PastDifficultyAverage);
    if (PastRateActualSeconds != 0 && PastRateTargetSeconds != 0) {
        bnNew *= (uint64_t)PastRateActualSeconds;
        bnNew /= (uint64_t)PastRateTargetSeconds;
    }
    if (bnNew > bnProofOfWorkLimit) { bnNew = bnProofOfWorkLimit; }

    return bnNew.GetCompact();
}

unsigned int static GetNextWorkRequiredKGW(const CBlockIndex* pindexLast, const CBlockHeader *pblock, const Consensus::Params& params)
{
    assert(pindexLast != nullptr);
    static const int64_t BlocksTargetSpacing        = 10 * 60; // 10 minutes
    unsigned int        TimeDaySeconds              = 60 * 60 * 24;
    int64_t             PastSecondsMin              = TimeDaySeconds * 0.0625;
    int64_t             PastSecondsMax              = TimeDaySeconds * 1.75;
    uint64_t            PastBlocksMin               = PastSecondsMin / BlocksTargetSpacing;
    uint64_t            PastBlocksMax               = PastSecondsMax / BlocksTargetSpacing;

    return KimotoGravityWell(pindexLast, pblock, params, BlocksTargetSpacing, PastBlocksMin, PastBlocksMax);
}

unsigned int GetNextWorkRequiredDSV1(const CBlockIndex* pindexLast, const CBlockHeader *pblock, const Consensus::Params& params)
{
    static const int64_t nTargetTimespan = 10*60 ; // dobbscoin: 10 minutes
    static const int64_t nTargetSpacing = 10*60;
    static const int64_t nInterval = nTargetTimespan / nTargetSpacing;

    unsigned int nProofOfWorkLimit = UintToArith256(params.powLimit).GetCompact();

    int64_t retargetTimespan = nTargetTimespan;
    int64_t retargetInterval = nInterval;

    // Only change once per interval
    if ((pindexLast->nHeight+1) % retargetInterval != 0)
    {
        if (params.fPowAllowMinDifficultyBlocks)
        {
            // Special difficulty rule for testnet:
            // If the new block's timestamp is more than 2* nTargetSpacing minutes
            // then allow mining of a min-difficulty block.
            if (pblock->nTime > pindexLast->nTime + nTargetSpacing * 30) //Params().TargetSpacing()*30)
                return nProofOfWorkLimit;
            else
            {
                // Return the last non-special-min-difficulty-rules-block
                const CBlockIndex* pindex = pindexLast;
                while (pindex->pprev && pindex->nHeight % retargetInterval != 0 && pindex->nBits == nProofOfWorkLimit)
                    pindex = pindex->pprev;
                return pindex->nBits;
            }
        }
        return pindexLast->nBits;
    }

    // Fractalcoin: This fixes an issue where a 51% attack can change difficulty at will.
    // Go back the full period unless it's the first retarget after genesis. Code courtesy of Art Forz
    int blockstogoback = retargetInterval-1;
    if ((pindexLast->nHeight+1) != retargetInterval)
        blockstogoback = retargetInterval;

    // Go back by what we want to be 14 days worth of blocks
    const CBlockIndex* pindexFirst = pindexLast;
    for (int i = 0; pindexFirst && i < blockstogoback; i++)
        pindexFirst = pindexFirst->pprev;
    assert(pindexFirst);

    // Limit adjustment step
    int64_t nActualTimespan = pindexLast->GetBlockTime() - pindexFirst->GetBlockTime();

    //DigiShield implementation - thanks to RealSolid & WDC for this code
    // amplitude filter - thanks to daft27 for this code
    nActualTimespan = retargetTimespan + (nActualTimespan - retargetTimespan)/8;

    if (nActualTimespan < (retargetTimespan - (retargetTimespan/4)) ) nActualTimespan = (retargetTimespan - (retargetTimespan/4));
    if (nActualTimespan > (retargetTimespan + (retargetTimespan/2)) ) nActualTimespan = (retargetTimespan + (retargetTimespan/2));

    // Retarget
    arith_uint256 bnNew;
    arith_uint256 bnOld;
    bnNew.SetCompact(pindexLast->nBits);
    bnOld = bnNew;
    //scale up for millisecond granularity
    bnNew *= nActualTimespan;
    //slingshield effectively works by making the target block time longer temporarily
    bnNew /= (retargetTimespan);

    if (bnNew > UintToArith256(params.powLimit))
        bnNew = UintToArith256(params.powLimit);

    return bnNew.GetCompact();
}

unsigned int GetNextWorkRequiredDSV2(const CBlockIndex* pindexLast, const CBlockHeader *pblock, const Consensus::Params& params)
{
    static const int64_t nTargetTimespan = 2*60 ; // dobbscoin: 2 minutes
    static const int64_t nTargetSpacing = 2*60;
    static const int64_t nInterval = nTargetTimespan / nTargetSpacing;

    unsigned int nProofOfWorkLimit = UintToArith256(params.powLimit).GetCompact();

    int64_t retargetTimespan = nTargetTimespan;
    int64_t retargetInterval = nInterval;

    // Only change once per interval
    if ((pindexLast->nHeight+1) % retargetInterval != 0)
    {
        if (params.fPowAllowMinDifficultyBlocks)
        {
            // Special difficulty rule for testnet:
            // If the new block's timestamp is more than 2* nTargetSpacing minutes
            // then allow mining of a min-difficulty block.
            if (pblock->nTime > pindexLast->nTime + nTargetSpacing * 30) //Params().TargetSpacing()*30)
                return nProofOfWorkLimit;
            else
            {
                // Return the last non-special-min-difficulty-rules-block
                const CBlockIndex* pindex = pindexLast;
                while (pindex->pprev && pindex->nHeight % retargetInterval != 0 && pindex->nBits == nProofOfWorkLimit)
                    pindex = pindex->pprev;
                return pindex->nBits;
            }
        }
        return pindexLast->nBits;
    }

    // Fractalcoin: This fixes an issue where a 51% attack can change difficulty at will.
    // Go back the full period unless it's the first retarget after genesis. Code courtesy of Art Forz
    int blockstogoback = retargetInterval-1;
    if ((pindexLast->nHeight+1) != retargetInterval)
        blockstogoback = retargetInterval;

    // Go back by what we want to be 14 days worth of blocks
    const CBlockIndex* pindexFirst = pindexLast;
    for (int i = 0; pindexFirst && i < blockstogoback; i++)
        pindexFirst = pindexFirst->pprev;
    assert(pindexFirst);

    // Limit adjustment step
    int64_t nActualTimespan = pindexLast->GetBlockTime() - pindexFirst->GetBlockTime();

    //DigiShield implementation - thanks to RealSolid & WDC for this code
    // amplitude filter - thanks to daft27 for this code
    nActualTimespan = retargetTimespan + (nActualTimespan - retargetTimespan)/8;

    if (nActualTimespan < (retargetTimespan - (retargetTimespan/4)) ) nActualTimespan = (retargetTimespan - (retargetTimespan/4));
    if (nActualTimespan > (retargetTimespan + (retargetTimespan/2)) ) nActualTimespan = (retargetTimespan + (retargetTimespan/2));

    // Retarget
    arith_uint256 bnNew;
    arith_uint256 bnOld;
    bnNew.SetCompact(pindexLast->nBits);
    bnOld = bnNew;
    //scale up for millisecond granularity
    bnNew *= nActualTimespan;
    //slingshield effectively works by making the target block time longer temporarily
    bnNew /= (retargetTimespan);

    if (bnNew > UintToArith256(params.powLimit))
        bnNew = UintToArith256(params.powLimit);

    return bnNew.GetCompact();
}

unsigned int GetNextWorkRequired(const CBlockIndex* pindexLast, const CBlockHeader *pblock, const Consensus::Params& params)
{
    int DiffMode = 1;
    if (params.fPowAllowMinDifficultyBlocks) {
        if (pindexLast->nHeight+1 >= 50) { DiffMode = 2; }
        if (pindexLast->nHeight+1 >= 100) { DiffMode = 3; }
        if(pindexLast->nHeight+1 >= 150) { DiffMode = 4; }
    }
    else
    {
        if (pindexLast->nHeight+1 >= 13579) { DiffMode = 2; } // KGW @ Block 13579
        if(pindexLast->nHeight+1 >= 31597) { DiffMode = 3; } //switch to dobbshield @ block 31597
        if(pindexLast->nHeight+1 >= 68425) {DiffMode = 4; } //switch to 2 minute blocks with dobbshield
    }
    if      (DiffMode == 1) { return GetNextWorkRequiredBTC(pindexLast, pblock, params); }
    else if (DiffMode == 2) { return GetNextWorkRequiredKGW(pindexLast, pblock, params); }
    else if (DiffMode == 3) { return GetNextWorkRequiredDSV1(pindexLast, pblock, params); }
    else if (DiffMode == 4) { return GetNextWorkRequiredDSV2(pindexLast, pblock, params); }
    return GetNextWorkRequiredDSV1(pindexLast, pblock, params); //never reached..
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

    //Dobbscoin: We need to step over genesis block hash checking, as it returns an invalid result.  Thank you, Coingen!
    if (hash == uint256S("0xb83bb8028229439a4e645a56d48ee31b884efdb0e395c8538a10a462166563f3"))
    {
        return true;
    }

    bnTarget.SetCompact(nBits, &fNegative, &fOverflow);

    // Check range
    if (fNegative || bnTarget == 0 || fOverflow || bnTarget > UintToArith256(params.powLimit))
        return false;

    // Check proof of work matches claimed amount
    if (UintToArith256(hash) > bnTarget)
        return false;

    return true;
}
