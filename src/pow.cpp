// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "pow.h"

#include "arith_uint256.h"
#include "chain.h"
#include "primitives/block.h"
#include "uint256.h"
#include "util.h"
#include "utilstrencodings.h"

#include <gmp.h>
#include <gmpxx.h>

unsigned int GetNextWorkRequired(const CBlockIndex* pindexLast, const CBlockHeader *pblock, const Consensus::Params& params)
{
    assert(pindexLast != NULL);
    unsigned int nProofOfWorkLimit = UintToArith256(params.powLimit).GetCompact();

    // Only change once per difficulty adjustment interval
    if ((pindexLast->nHeight+1) % params.DifficultyAdjustmentInterval(pindexLast->nHeight+1) != 0)
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
                while (pindex->pprev && pindex->nHeight % params.DifficultyAdjustmentInterval(pindex->nHeight) != 0 && pindex->nBits == nProofOfWorkLimit)
                    pindex = pindex->pprev;
                return pindex->nBits;
            }
        }
        return pindexLast->nBits;
    }

    // Go back by what we want to be 14 days worth of blocks
    int nHeightFirst = pindexLast->nHeight - (params.DifficultyAdjustmentInterval(pindexLast->nHeight+1)-1);
    assert(nHeightFirst >= 0);
    const CBlockIndex* pindexFirst = pindexLast->GetAncestor(nHeightFirst);
    assert(pindexFirst);

    return CalculateNextWorkRequired(pindexLast, pindexFirst->GetBlockTime(), params);
}

mpz_class inline i64_to_mpz(int64_t nValue)
{
    return mpz_class(i64tostr(nValue));
}

int64_t inline mpz_to_i64(const mpz_class &zValue)
{
    static mpz_class MPZ_MAX_I64( "9223372036854775807");
    static mpz_class MPZ_MIN_I64("-9223372036854775808");
    if (zValue < MPZ_MIN_I64 || zValue > MPZ_MAX_I64)
        throw std::runtime_error("mpz_to_i64 : input exceeds range of int64_t type");
    int64_t result = 0;
    mpz_class tmp(zValue);
    bool sign = tmp < 0;
    if ( sign ) tmp = -tmp;
    result = atoi64(tmp.get_str());
    return (sign ? -result : result);
}

mpz_class inline mpz_from_uint256(const uint256& bn)
{
    mpz_class z;
    mpz_import(z.get_mpz_t(), 32, -1, 1, -1, 0, bn.begin());
    return z;
}

uint256 inline uint256_from_mpz(const mpz_class& z)
{
    uint256 bn;
    mpz_export(bn.begin(), NULL, -1, 32, -1, 0, z.get_mpz_t());
    return bn;
}

unsigned int CalculateNextWorkRequired(const CBlockIndex* pindexLast, int64_t nFirstBlockTime, const Consensus::Params& params)
{
    if (params.fPowNoRetargeting)
        return pindexLast->nBits;

    const int kWindowSize = 36;
    static mpq_class kOne = mpq_class(1);
    static mpq_class kGain = mpq_class(1, 8);       // 0.125
    static mpq_class kLimiterUp = mpq_class(11, 8); // 1.375
    static mpq_class kLimiterDown = mpq_class(8, 11);
    static mpq_class kTargetInterval = mpq_class(i64_to_mpz(params.nPowTargetSpacing));
    static int32_t kFilterCoeff[kWindowSize] =
        {  140111793,  105764386,  109100300,   79262945,   11957247,  -86965742,
          -200607279, -301199508, -354940962, -328658726, -197005272,   49124369,
           398931360,  820161125, 1263269526, 1669153929, 1979408502, 2147483647,
          2147483647, 1979408502, 1669153929, 1263269526,  820161125,  398931360,
            49124369, -197005272, -328658726, -354940962, -301199508, -200607279,
           -86965742,   11957247,   79262945,  109100300,  105764386,  140111793 };
    static mpz_class kNormFactor = mpz_class("14608703280");

    const bool fUseFilter = (pindexLast->nHeight >= (params.nFIRDiffFilterThreshold-1));

    LogPrintf("GetNextWorkRequired RETARGET\n");

    mpq_class dAdjustmentFactor;

    if (fUseFilter)
    {
        int64_t vTimeDelta[kWindowSize];

        size_t idx = 0;
        const CBlockIndex *pitr = pindexLast;
        for ( ; idx!=kWindowSize && pitr && pitr->pprev; ++idx, pitr=pitr->pprev )
            vTimeDelta[idx] = (int32_t)(pitr->GetBlockTime() - pitr->pprev->GetBlockTime());
        for ( ; idx!=kWindowSize; ++idx )
            vTimeDelta[idx] = (int32_t)params.nPowTargetSpacing;

        int64_t vFilteredTime = 0;
        for ( idx=0; idx<kWindowSize; ++idx )
            vFilteredTime += (int64_t)kFilterCoeff[idx] * (int64_t)vTimeDelta[idx];
        mpq_class dFilteredInterval(i64_to_mpz(vFilteredTime), kNormFactor);

        dAdjustmentFactor = kOne - kGain * (dFilteredInterval - kTargetInterval) / kTargetInterval;
        if (dAdjustmentFactor > kLimiterUp)
            dAdjustmentFactor = kLimiterUp;
        if (dAdjustmentFactor < kLimiterDown)
            dAdjustmentFactor = kLimiterDown;
    } else {
        // Limit adjustment step
        int64_t nActualTimespan = pindexLast->GetBlockTime() - nFirstBlockTime;
        LogPrintf("  nActualTimespan = %d  before bounds\n", nActualTimespan);
        if (nActualTimespan < params.nPowOriginalTargetTimespan/4)
            nActualTimespan = params.nPowOriginalTargetTimespan/4;
        if (nActualTimespan > params.nPowOriginalTargetTimespan*4)
            nActualTimespan = params.nPowOriginalTargetTimespan*4;

        dAdjustmentFactor = mpq_class(i64_to_mpz(params.nPowOriginalTargetTimespan),
                                      i64_to_mpz(nActualTimespan));
    }

    // Retarget
    arith_uint256 bnOld;
    bnOld.SetCompact(pindexLast->nBits);

    mpz_class zNew;
    zNew  = mpz_from_uint256(ArithToUint256(bnOld));
    zNew *= dAdjustmentFactor.get_den();
    zNew /= dAdjustmentFactor.get_num();

    arith_uint256 bnNew;
    bnNew = UintToArith256(uint256_from_mpz(zNew));

    const arith_uint256 bnPowLimit = UintToArith256(params.powLimit);
    if (bnNew > bnPowLimit)
        bnNew = bnPowLimit;

    LogPrintf("dAdjustmentFactor = %g\n", dAdjustmentFactor.get_d());
    LogPrintf("Before: %08x  %s\n", pindexLast->nBits, ArithToUint256(bnOld).ToString());
    LogPrintf("After:  %08x  %s\n", bnNew.GetCompact(), ArithToUint256(bnNew).ToString());

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
