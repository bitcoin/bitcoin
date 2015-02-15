// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "consensus/consensus.h"

#include "arith_uint256.h"
#include "consensus/validation.h"
#include "primitives/block.h"
#include "script/interpreter.h"
#include "tinyformat.h"

#include <algorithm>  

static const unsigned int MEDIAN_TIME_SPAN = 11;

uint32_t GetNextWorkRequired(const CBlockIndexBase* pindexLast, const CBlockHeader *pblock, const Consensus::Params& params, PrevIndexGetter indexGetter)
{
    uint32_t nProofOfWorkLimit = UintToArith256(params.powLimit).GetCompact();

    // Genesis block
    if (pindexLast == NULL)
        return nProofOfWorkLimit;

    // Only change once per difficulty adjustment interval
    if ((pindexLast->nHeight+1) % params.DifficultyAdjustmentInterval() != 0)
    {
        if (params.fPowAllowMinDifficultyBlocks)
        {
            // Special difficulty rule for testnet:
            // If the new block's timestamp is more than 2* 10 minutes
            // then allow mining of a min-difficulty block.
            if ((int64_t)pblock->nTime > (int64_t)pindexLast->nTime + params.nPowTargetSpacing*2)
                return nProofOfWorkLimit;
            else
            {
                // Return the last non-special-min-difficulty-rules-block
                const CBlockIndexBase* pindex = pindexLast;
                while (indexGetter(pindex) && pindex->nHeight % params.DifficultyAdjustmentInterval() != 0 && pindex->nBits == nProofOfWorkLimit)
                    pindex = indexGetter(pindex);
                return pindex->nBits;
            }
        }
        return pindexLast->nBits;
    }

    // Go back by what we want to be 14 days worth of blocks
    const CBlockIndexBase* pindexFirst = pindexLast;
    for (int i = 0; pindexFirst && i < params.DifficultyAdjustmentInterval()-1; i++)
        pindexFirst = indexGetter(pindexFirst);
    assert(pindexFirst);

    return CalculateNextWorkRequired(pindexLast, (int64_t)pindexFirst->nTime, params);
}

uint32_t CalculateNextWorkRequired(const CBlockIndexBase* pindexLast, int64_t nFirstBlockTime, const Consensus::Params& params)
{
    // Limit adjustment step
    int64_t nActualTimespan = (int64_t)pindexLast->nTime - nFirstBlockTime;
    if (nActualTimespan < params.nPowTargetTimespan/4)
        nActualTimespan = params.nPowTargetTimespan/4;
    if (nActualTimespan > params.nPowTargetTimespan*4)
        nActualTimespan = params.nPowTargetTimespan*4;

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

bool CheckProofOfWork(uint256 hash, unsigned int nBits, const Consensus::Params& params)
{
    bool fNegative;
    bool fOverflow;
    arith_uint256 bnTarget;

    bnTarget.SetCompact(nBits, &fNegative, &fOverflow);

    // Check range
    if (fNegative || bnTarget == 0 || fOverflow || bnTarget > UintToArith256(params.powLimit))
        return false; // nBits below minimum work

    // Check proof of work matches claimed amount
    if (UintToArith256(hash) > bnTarget)
        return false; // hash doesn't match nBits

    return true;
}

int64_t GetMedianTimePast(const CBlockIndexBase* pindex, PrevIndexGetter indexGetter)
{
    int64_t pmedian[MEDIAN_TIME_SPAN];
    int64_t* pbegin = &pmedian[MEDIAN_TIME_SPAN];
    int64_t* pend = &pmedian[MEDIAN_TIME_SPAN];

    for (unsigned int i = 0; i < MEDIAN_TIME_SPAN && pindex; i++, pindex = indexGetter(pindex))
        *(--pbegin) = (int64_t)pindex->nTime;

    std::sort(pbegin, pend);
    return pbegin[(pend - pbegin)/2];
}

bool IsSuperMajority(int minVersion, const CBlockIndexBase* pstart, unsigned nRequired, const Consensus::Params& consensusParams, PrevIndexGetter indexGetter)
{
    unsigned int nFound = 0;
    for (int i = 0; i < consensusParams.nMajorityWindow && nFound < nRequired && pstart != NULL; i++)
    {
        if (pstart->nVersion >= minVersion)
            ++nFound;
        pstart = indexGetter(pstart);
    }
    return (nFound >= nRequired);
}

bool Consensus::CheckBlockHeader(const CBlockHeader& block, CValidationState& state, const Consensus::Params& consensusParams, int64_t nTime, bool fCheckPOW)
{
    // Check proof of work matches claimed amount
    if (fCheckPOW && !CheckProofOfWork(block.GetHash(), block.nBits, consensusParams))
        return state.DoS(50, false, REJECT_INVALID, "high-hash");

    // Check timestamp
    if (block.GetBlockTime() > nTime + 2 * 60 * 60)
        return state.Invalid(false, REJECT_INVALID, "time-too-new");

    return true;
}

unsigned Consensus::GetFlags(const CBlock& block, const Consensus::Params& consensusParams, CBlockIndexBase* pindex, PrevIndexGetter indexGetter)
{
    int64_t nBIP16SwitchTime = 1333238400;
    bool fStrictPayToScriptHash = ((int64_t)pindex->nTime >= nBIP16SwitchTime);
    unsigned int flags = fStrictPayToScriptHash ? SCRIPT_VERIFY_P2SH : SCRIPT_VERIFY_NONE;

    if (block.nVersion >= 3 && IsSuperMajority(3, indexGetter(pindex), consensusParams.nMajorityEnforceBlockUpgrade, consensusParams, indexGetter))
        flags |= SCRIPT_VERIFY_DERSIG;
    return flags;
}

bool Consensus::ContextualCheckBlockHeader(const CBlockHeader& block, CValidationState& state, const Consensus::Params& consensusParams, const CBlockIndexBase* pindexPrev, PrevIndexGetter indexGetter)
{
    // Check proof of work
    if (block.nBits != GetNextWorkRequired(pindexPrev, &block, consensusParams, indexGetter))
        return state.DoS(100, false, REJECT_INVALID, "bad-diffbits");

    // Check timestamp against prev
    if (block.GetBlockTime() <= GetMedianTimePast(pindexPrev, indexGetter))
        return state.Invalid(false, REJECT_INVALID, "time-too-old");

    // Reject block.nVersion=n blocks when 95% (75% on testnet) of the network has upgraded (last version=3):
    for (int i = 2; i <= 3; i++)
        if (block.nVersion < i && IsSuperMajority(i, pindexPrev, consensusParams.nMajorityRejectBlockOutdated, consensusParams, indexGetter))
            return state.Invalid(false, REJECT_OBSOLETE, strprintf("bad-version nVersion=%d", i-1));

    return true;
}

bool Consensus::VerifyBlockHeader(const CBlockHeader& block, CValidationState& state, const Consensus::Params& params, int64_t nTime, CBlockIndexBase* pindexPrev, PrevIndexGetter indexGetter)
{
    if (!Consensus::CheckBlockHeader(block, state, params, nTime, true))
        return false;
    if (!Consensus::ContextualCheckBlockHeader(block, state, params, pindexPrev, indexGetter))
        return false;
    return true;
}
