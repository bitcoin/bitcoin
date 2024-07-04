// Copyright (c) 2024 The Navcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <blsct/pos/pos.h>
#include <primitives/block.h>

namespace blsct {
// ppcoin: find last block index up to pindex
const CBlockIndex* GetLastBlockIndex(const CBlockIndex* pindex, bool fProofOfStake)
{
    while (pindex && pindex->pprev && (pindex->IsProofOfStake() != fProofOfStake))
        pindex = pindex->pprev;
    return pindex;
}

unsigned int GetNextTargetRequired(const CBlockIndex* pindexLast, const CBlockHeader* pblock, const Consensus::Params& params)
{
    assert(pindexLast != nullptr);
    // unsigned int nProofOfStakeLimit = UintToArith256(params.posLimit).GetCompact();

    // Only change once per difficulty adjustment interval
    if ((pindexLast->nHeight + 1) % params.DifficultyAdjustmentIntervalPos() != 0) {
        // if (params.fPowAllowMinDifficultyBlocks) {
        //     // Special difficulty rule for testnet:
        //     // If the new block's timestamp is more than 2* 10 minutes
        //     // then allow mining of a min-difficulty block.
        //     if (pblock->GetBlockTime() > pindexLast->GetBlockTime() + params.nPosTargetSpacing * 2)
        //         return nProofOfStakeLimit;
        //     else {
        //         // Return the last non-special-min-difficulty-rules-block
        //         const CBlockIndex* pindex = pindexLast;
        //         while (pindex->pprev && pindex->nHeight % params.DifficultyAdjustmentIntervalPos() != 0 && pindex->nBits == nProofOfStakeLimit)
        //             pindex = pindex->pprev;
        //         return pindex->nBits;
        //     }
        // }
        return pindexLast->nBits;
    }

    // Go back by what we want to be 15 minutes worth of blocks
    int nHeightFirst = pindexLast->nHeight - (params.DifficultyAdjustmentIntervalPos() - 1);
    assert(nHeightFirst >= 0);
    const CBlockIndex* pindexFirst = pindexLast->GetAncestor(nHeightFirst);
    assert(pindexFirst);

    return CalculateNextTargetRequired(pindexLast, pindexFirst->GetBlockTime(), params);
}

unsigned int CalculateNextTargetRequired(const CBlockIndex* pindexLast, int64_t nFirstBlockTime, const Consensus::Params& params)
{
    if (params.fPosNoRetargeting)
        return pindexLast->nBits;

    // Limit adjustment step
    int64_t nActualTimespan = pindexLast->GetBlockTime() - nFirstBlockTime;
    if (nActualTimespan < params.nPosTargetTimespan / 4)
        nActualTimespan = params.nPosTargetTimespan / 4;
    if (nActualTimespan > params.nPosTargetTimespan * 4)
        nActualTimespan = params.nPosTargetTimespan * 4;

    // Retarget
    const arith_uint256 bnPosLimit = UintToArith256(params.posLimit);
    arith_uint256 bnNew;
    bnNew.SetCompact(pindexLast->nBits);
    bnNew *= nActualTimespan;
    bnNew /= params.nPosTargetTimespan;

    if (bnNew > bnPosLimit)
        bnNew = bnPosLimit;

    return bnNew.GetCompact();
}

// Get the last stake modifier and its generation time from a given block
bool GetLastStakeModifier(const CBlockIndex* pindex, uint64_t& nStakeModifier, int64_t& nModifierTime)
{
    AssertLockHeld(::cs_main);
    if (!pindex)
        return false; // error("GetLastStakeModifier: null pindex");
    while (pindex && pindex->pprev && !pindex->GeneratedStakeModifier())
        pindex = pindex->pprev;
    if (!pindex->GeneratedStakeModifier()) {
        nStakeModifier = 1;
        nModifierTime = pindex->GetBlockTime();
        return true;
    }
    nStakeModifier = pindex->nStakeModifier;
    nModifierTime = pindex->GetBlockTime();
    return true;
}

// Get selection interval section (in seconds)
int64_t GetStakeModifierSelectionIntervalSection(int nSection, const Consensus::Params& params)
{
    assert(nSection >= 0 && nSection < 64);
    return (params.nModifierInterval * 63 / (63 + ((63 - nSection) * (MODIFIER_INTERVAL_RATIO - 1))));
}

// Get stake modifier selection interval (in seconds)
int64_t GetStakeModifierSelectionInterval(const Consensus::Params& params)
{
    int64_t nSelectionInterval = 0;
    for (int nSection = 0; nSection < 64; nSection++)
        nSelectionInterval += GetStakeModifierSelectionIntervalSection(nSection, params);
    return nSelectionInterval;
}

std::vector<unsigned char> CalculateSetMemProofRandomness(const CBlockIndex* pindexPrev)
{
    HashWriter ss{};

    ss << pindexPrev->GetBlockHash() << pindexPrev->nStakeModifier;

    auto hash = ss.GetHash();

    return std::vector<unsigned char>(hash.begin(), hash.end());
}


blsct::Message
CalculateSetMemProofGeneratorSeed(const CBlockIndex* pindexPrev)
{
    HashWriter ss{};

    ss << pindexPrev->nHeight << pindexPrev->nStakeModifier;

    auto hash = ss.GetHash();

    return std::vector<unsigned char>(hash.begin(), hash.end());
}

uint256 CalculateKernelHash(const CBlockIndex* pindexPrev, const CBlock& block)
{
    return CalculateKernelHash(pindexPrev->nTime, pindexPrev->nStakeModifier, block.posProof.setMemProof.phi, block.nTime);
}
} // namespace blsct