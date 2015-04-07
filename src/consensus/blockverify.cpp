// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "consensus/consensus.h"

#include "arith_uint256.h"
#include "chain.h"
#include "coins.h"
#include "consensus/validation.h"
#include "primitives/block.h"
#include "script/interpreter.h"
#include "tinyformat.h"
#include "version.h"

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

/**
 * Returns true if there are nRequired or more blocks of minVersion or above
 * in the last Consensus::Params::nMajorityWindow blocks, starting at pstart and going backwards.
 */
static bool IsSuperMajority(int minVersion, const CBlockIndexBase* pstart, unsigned nRequired, const Consensus::Params& consensusParams, PrevIndexGetter indexGetter)
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

bool Consensus::EnforceBIP30(const CBlock& block, CValidationState& state, const CBlockIndex* pindexPrev, const CCoinsViewCache& inputs)
{
    if (!(pindexPrev->nHeight==91842 && pindexPrev->GetBlockHash() == uint256S("0x00000000000a4d0a398161ffc163c503763b1f4360639393e0e4c8e300e0caec")) ||
        (pindexPrev->nHeight==91880 && pindexPrev->GetBlockHash() == uint256S("0x00000000000743f190a18c5577a3c2d2a1f610ae9601ac046a38084ccb7cd721")))
        for (unsigned int i = 0; i < block.vtx.size(); i++) {
            const CCoins* coins = inputs.AccessCoins(block.vtx[i].GetHash());
            if (coins && !coins->IsPruned())
                return state.DoS(100, false, REJECT_INVALID, "bad-txns-BIP30");
        }
    return true;
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

CAmount GetBlockSubsidy(int nHeight, const Consensus::Params& consensusParams)
{
    int halvings = nHeight / consensusParams.nSubsidyHalvingInterval;
    // Force block reward to zero when right shift is undefined.
    if (halvings >= 64)
        return 0;

    CAmount nSubsidy = 50 * COIN;
    // Subsidy is cut in half every 210,000 blocks which will occur approximately every 4 years.
    nSubsidy >>= halvings;
    return nSubsidy;
}

bool Consensus::CheckBlock(const CBlock& block, CValidationState& state, const Consensus::Params& consensusParams, int64_t nTime, bool fCheckPOW, bool fCheckMerkleRoot)
{
    // Check that the header is valid (particularly PoW).  This is mostly
    // redundant with the call in AcceptBlockHeader.
    if (!Consensus::CheckBlockHeader(block, state, consensusParams, nTime, fCheckPOW))
        return false;

    // Check the merkle root.
    if (fCheckMerkleRoot) {
        bool mutated;
        uint256 hashMerkleRoot2 = block.BuildMerkleTree(&mutated);
        if (block.hashMerkleRoot != hashMerkleRoot2)
            return state.DoS(100, false, REJECT_INVALID, "bad-txnmrklroot", true);

        // Check for merkle tree malleability (CVE-2012-2459): repeating sequences
        // of transactions in a block without affecting the merkle root of a block,
        // while still invalidating it.
        if (mutated)
            return state.DoS(100, false, REJECT_INVALID, "bad-txns-duplicate", true);
    }

    // All potential-corruption validation must be done before we do any
    // transaction validation, as otherwise we may mark the header as invalid
    // because we receive the wrong transactions for it.

    // Size limits
    if (block.vtx.empty() || block.vtx.size() > MAX_BLOCK_SIZE || ::GetSerializeSize(block, SER_NETWORK, PROTOCOL_VERSION) > MAX_BLOCK_SIZE)
        return state.DoS(100, false, REJECT_INVALID, "bad-blk-length");

    // First transaction must be coinbase, the rest must not be
    if (block.vtx.empty() || !block.vtx[0].IsCoinBase())
        return state.DoS(100, false, REJECT_INVALID, "bad-cb-missing");
    for (unsigned int i = 1; i < block.vtx.size(); i++)
        if (block.vtx[i].IsCoinBase())
            return state.DoS(100, false, REJECT_INVALID, "bad-cb-multiple");

    // Check transactions
    for (unsigned int i = 0; i < block.vtx.size(); i++)
        if (!Consensus::CheckTx(block.vtx[i], state))
            return false;

    unsigned int nSigOps = 0;
    for (unsigned int i = 0; i < block.vtx.size(); i++)
        nSigOps += GetLegacySigOpCount(block.vtx[i]);

    if (nSigOps > MAX_BLOCK_SIGOPS)
        return state.DoS(100, false, REJECT_INVALID, "bad-blk-sigops", true);

    return true;
}

bool Consensus::ContextualCheckBlock(const CBlock& block, CValidationState& state, const Consensus::Params& consensusParams, const CBlockIndexBase* pindexPrev, PrevIndexGetter indexGetter)
{
    const int nHeight = pindexPrev->nHeight + 1;
    // Check that all transactions are finalized
    for (unsigned int i = 1; i < block.vtx.size(); i++)
        if (!CheckFinalTx(block.vtx[i], nHeight, block.GetBlockTime()))
            return state.DoS(10, false, REJECT_INVALID, "bad-txns-nonfinal");

    // Enforce block.nVersion=2 rule that the coinbase starts with serialized block height
    // if 750 of the last 1,000 blocks are version 2 or greater (51/100 if testnet):
    if (block.nVersion >= 2 && IsSuperMajority(2, pindexPrev, consensusParams.nMajorityEnforceBlockUpgrade, consensusParams, indexGetter))
    {
        CScript expect = CScript() << nHeight;
        if (block.vtx[0].vin[0].scriptSig.size() < expect.size() ||
            !std::equal(expect.begin(), expect.end(), block.vtx[0].vin[0].scriptSig.begin())) {
            return state.DoS(100, false, REJECT_INVALID, "bad-cb-height");
        }
    }

    return true;
}
