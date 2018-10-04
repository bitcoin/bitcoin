// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "pow.h"

#include "arith_uint256.h"
#include "chain.h"
#include "chainparams.h"
#include "primitives/block.h"
#include "uint256.h"
#include "util.h"

#include <math.h>

unsigned int static KimotoGravityWell(const CBlockIndex* pindexLast) {
    const CBlockIndex *BlockLastSolved = pindexLast;
    const CBlockIndex *BlockReading = pindexLast;
    uint64_t PastBlocksMass = 0;
    int64_t PastRateActualSeconds = 0;
    int64_t PastRateTargetSeconds = 0;
    double PastRateAdjustmentRatio = double(1);
    arith_uint256 PastDifficultyAverage;
    arith_uint256 PastDifficultyAveragePrev;
    double EventHorizonDeviation;
    double EventHorizonDeviationFast;
    double EventHorizonDeviationSlow;

    uint64_t pastSecondsMin = Params().TargetTimespan() * 0.025;
    uint64_t pastSecondsMax = Params().TargetTimespan() * 7;
    uint64_t PastBlocksMin = pastSecondsMin / Params().TargetSpacing();
    uint64_t PastBlocksMax = pastSecondsMax / Params().TargetSpacing();

    if (BlockLastSolved == NULL || BlockLastSolved->nHeight == 0 || (uint64_t)BlockLastSolved->nHeight < PastBlocksMin) { return Params().ProofOfWorkLimit().GetCompact(); }

    for (unsigned int i = 1; BlockReading && BlockReading->nHeight > 0; i++) {
        if (PastBlocksMax > 0 && i > PastBlocksMax) { break; }
        PastBlocksMass++;

        PastDifficultyAverage.SetCompact(BlockReading->nBits);
        if (i > 1) {
            // handle negative uint256
            if(PastDifficultyAverage >= PastDifficultyAveragePrev)
                PastDifficultyAverage = ((PastDifficultyAverage - PastDifficultyAveragePrev) / i) + PastDifficultyAveragePrev;
            else
                PastDifficultyAverage = PastDifficultyAveragePrev - ((PastDifficultyAveragePrev - PastDifficultyAverage) / i);
        }
        PastDifficultyAveragePrev = PastDifficultyAverage;

        PastRateActualSeconds = BlockLastSolved->GetBlockTime() - BlockReading->GetBlockTime();
        PastRateTargetSeconds = Params().TargetSpacing() * PastBlocksMass;
        PastRateAdjustmentRatio = double(1);
        if (PastRateActualSeconds < 0) { PastRateActualSeconds = 0; }
        if (PastRateActualSeconds != 0 && PastRateTargetSeconds != 0) {
            PastRateAdjustmentRatio = double(PastRateTargetSeconds) / double(PastRateActualSeconds);
        }
        EventHorizonDeviation = 1 + (0.7084 * pow((double(PastBlocksMass)/double(28.2)), -1.228));
        EventHorizonDeviationFast = EventHorizonDeviation;
        EventHorizonDeviationSlow = 1 / EventHorizonDeviation;

        if (PastBlocksMass >= PastBlocksMin) {
                if ((PastRateAdjustmentRatio <= EventHorizonDeviationSlow) || (PastRateAdjustmentRatio >= EventHorizonDeviationFast))
                { assert(BlockReading); break; }
        }
        if (BlockReading->pprev == NULL) { assert(BlockReading); break; }
        BlockReading = BlockReading->pprev;
    }

    arith_uint256 bnNew(PastDifficultyAverage);
    if (PastRateActualSeconds != 0 && PastRateTargetSeconds != 0) {
        bnNew *= PastRateActualSeconds;
        bnNew /= PastRateTargetSeconds;
    }

    if (bnNew > Params().ProofOfWorkLimit()) {
        bnNew = Params().ProofOfWorkLimit();
    }

    return bnNew.GetCompact();
}

unsigned int static DarkGravityWave(const CBlockIndex* pindexLast) {
    /* current difficulty formula, crown - DarkGravity v3, written by Evan Duffield - evan@crown.tech */
    const CBlockIndex *BlockLastSolved = pindexLast;
    const CBlockIndex *BlockReading = pindexLast;
    int64_t nActualTimespan = 0;
    int64_t LastBlockTime = 0;
    int64_t PastBlocksMin = 24;
    int64_t PastBlocksMax = 24;
    int64_t CountBlocks = 0;
    arith_uint256 PastDifficultyAverage;
    arith_uint256 PastDifficultyAveragePrev;

    if (BlockLastSolved == NULL || BlockLastSolved->nHeight == 0 || BlockLastSolved->nHeight < PastBlocksMin) {
        return Params().ProofOfWorkLimit().GetCompact();
    }

    for (unsigned int i = 1; BlockReading && BlockReading->nHeight > 0; i++) {
        if (PastBlocksMax > 0 && i > PastBlocksMax) { break; }
        CountBlocks++;

        if(CountBlocks <= PastBlocksMin) {
            if (CountBlocks == 1) { PastDifficultyAverage.SetCompact(BlockReading->nBits); }
            else { PastDifficultyAverage = ((PastDifficultyAveragePrev * CountBlocks) + (arith_uint256().SetCompact(BlockReading->nBits))) / (CountBlocks + 1); }
            PastDifficultyAveragePrev = PastDifficultyAverage;
        }

        if(LastBlockTime > 0){
            int64_t Diff = (LastBlockTime - BlockReading->GetBlockTime());
            nActualTimespan += Diff;
        }
        LastBlockTime = BlockReading->GetBlockTime();

        if (BlockReading->pprev == NULL) { assert(BlockReading); break; }
        BlockReading = BlockReading->pprev;
    }

    arith_uint256 bnNew(PastDifficultyAverage);

    int64_t _nTargetTimespan = CountBlocks * Params().TargetSpacing();

    if (nActualTimespan < _nTargetTimespan/3)
        nActualTimespan = _nTargetTimespan/3;
    if (nActualTimespan > _nTargetTimespan*3)
        nActualTimespan = _nTargetTimespan*3;

    // Retarget
    bnNew *= nActualTimespan;
    bnNew /= _nTargetTimespan;

    if (bnNew > Params().ProofOfWorkLimit()){
        bnNew = Params().ProofOfWorkLimit();
    }

    return bnNew.GetCompact();
}

unsigned int GetNextWorkRequired(const CBlockIndex* pindexLast, const CBlockHeader *pblock)
{
    unsigned int retarget = DIFF_DGW;

    if (pindexLast->nHeight + 1 >= 1059780) retarget = DIFF_DGW;
    else retarget = DIFF_BTC;

    if (Params().NetworkID() == CBaseChainParams::TESTNET && pindexLast->nHeight >= 10000)
        retarget = DIFF_DGW;

    // Default Bitcoin style retargeting
    if (retarget == DIFF_BTC)
    {
        unsigned int nProofOfWorkLimit = Params().ProofOfWorkLimit().GetCompact();

        // Genesis block
        if (pindexLast == NULL)
            return nProofOfWorkLimit;

        // Only change once per interval
        if ((pindexLast->nHeight+1) % Params().Interval() != 0)
        {
            if (Params().AllowMinDifficultyBlocks())
            {
                // Special difficulty rule for testnet:
                // If the new block's timestamp is more than 2* 2.5 minutes
                // then allow mining of a min-difficulty block.
                if (pblock->GetBlockTime() > pindexLast->GetBlockTime() + Params().TargetSpacing()*2)
                    return nProofOfWorkLimit;
                else
                {
                    // Return the last non-special-min-difficulty-rules-block
                    const CBlockIndex* pindex = pindexLast;
                    while (pindex->pprev && pindex->nHeight % Params().Interval() != 0 && pindex->nBits == nProofOfWorkLimit)
                        pindex = pindex->pprev;
                    return pindex->nBits;
                }
            }
            return pindexLast->nBits;
        }

        // Go back by what we want to be 1 day worth of blocks
        const CBlockIndex* pindexFirst = pindexLast;
        for (int i = 0; pindexFirst && i < Params().Interval()-1; i++)
            pindexFirst = pindexFirst->pprev;
        assert(pindexFirst);

        // Limit adjustment step
        int64_t nActualTimespan = pindexLast->GetBlockTime() - pindexFirst->GetBlockTime();
        LogPrintf("  nActualTimespan = %d  before bounds\n", nActualTimespan);
        if (nActualTimespan < Params().TargetTimespan()/4)
            nActualTimespan = Params().TargetTimespan()/4;
        if (nActualTimespan > Params().TargetTimespan()*4)
            nActualTimespan = Params().TargetTimespan()*4;

        // Retarget
        arith_uint256 bnNew;
        arith_uint256 bnOld;
        bnNew.SetCompact(pindexLast->nBits);
        bnOld = bnNew;
        bnNew *= nActualTimespan;
        bnNew /= Params().TargetTimespan();

        if (bnNew > Params().ProofOfWorkLimit())
            bnNew = Params().ProofOfWorkLimit();

        /// debug print
        LogPrintf("GetNextWorkRequired RETARGET at %d\n", pindexLast->nHeight + 1);
        LogPrintf("Params().TargetTimespan() = %d    nActualTimespan = %d\n", Params().TargetTimespan(), nActualTimespan);
        LogPrintf("Before: %08x  %s\n", pindexLast->nBits, bnOld.ToString());
        LogPrintf("After:  %08x  %s\n", bnNew.GetCompact(), bnNew.ToString());

        return bnNew.GetCompact();

    }

    // Retarget using Kimoto Gravity Wave
    else if (retarget == DIFF_KGW)
    {
        return KimotoGravityWell(pindexLast);
    }

    // Retarget using Dark Gravity Wave 3
    else if (retarget == DIFF_DGW)
    {
        return DarkGravityWave(pindexLast);
    }

    return DarkGravityWave(pindexLast);
}

bool CheckProofOfWork(uint256 hash, unsigned int nBits)
{
    bool fNegative;
    bool fOverflow;
    arith_uint256 bnTarget;

    bnTarget.SetCompact(nBits, &fNegative, &fOverflow);

    // Check range
    if (fNegative || bnTarget == 0 || fOverflow || bnTarget > Params().ProofOfWorkLimit())
        return error("CheckProofOfWork() : nBits below minimum work");

    // Check proof of work matches claimed amount
    if (UintToArith256(hash) > bnTarget)
        return error("CheckProofOfWork() : hash doesn't match nBits");

    return true;
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
