// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Copyright (c) 2014-2017 The Syscoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "pow.h"

#include "arith_uint256.h"
#include "chain.h"
#include "chainparams.h"
#include "primitives/block.h"
#include "uint256.h"
#include "util.h"

#include <math.h>
// SYSCOIN
#include "validation.h"
unsigned int GetNextWorkRequired(const CBlockIndex* pindexLast, const CBlockHeader *pblock, const Consensus::Params& params)
{
	return CalculateNextWorkRequired(pindexLast, 0, params);
}
// SYSCOIN DGW diff algo
unsigned int CalculateNextWorkRequired(const CBlockIndex* pindexLast, int64_t nFirstBlockTime, const Consensus::Params& params)
{

	/* current difficulty formula, syscoin - DarkGravity v3, written by Evan Duffield - evan@dashpay.io */
	// Genesis block
	const arith_uint256 nProofOfWorkLimit = UintToArith256(params.powLimit);
	if (params.fPowNoRetargeting)
		return pindexLast->nBits;
	const CBlockIndex *BlockLastSolved = pindexLast;
	const CBlockIndex *BlockReading = pindexLast;
	int64_t nActualTimespan = 0;
	int64_t LastBlockTime = 0;
	int64_t PastBlocksMin = 24;
	int64_t PastBlocksMax = 24;
	int64_t CountBlocks = 0;
	arith_uint256 PastDifficultyAverage;
	arith_uint256 PastDifficultyAveragePrev;

	// SYSCOIN 300 needed for snapshot unit test
	if (BlockLastSolved == NULL || BlockLastSolved->nHeight <= 400 ) {
		return UintToArith256(Params(CBaseChainParams::REGTEST).GetConsensus().powLimit).GetCompact();
	}

	for (unsigned int i = 1; BlockReading && BlockReading->nHeight > 0; i++) {
		if (PastBlocksMax > 0 && i > PastBlocksMax) { break; }
		CountBlocks++;

		if (CountBlocks <= PastBlocksMin) {
			if (CountBlocks == 1) { PastDifficultyAverage.SetCompact(BlockReading->nBits); }
			else { PastDifficultyAverage = ((PastDifficultyAveragePrev * CountBlocks) + (arith_uint256().SetCompact(BlockReading->nBits))) / (CountBlocks + 1); }
			PastDifficultyAveragePrev = PastDifficultyAverage;
		}

		if (LastBlockTime > 0) {
			int64_t Diff = (LastBlockTime - BlockReading->GetBlockTime());
			nActualTimespan += Diff;
		}
		LastBlockTime = BlockReading->GetBlockTime();

		if (BlockReading->pprev == NULL) { assert(BlockReading); break; }
		BlockReading = BlockReading->pprev;
	}

	arith_uint256 bnNew(PastDifficultyAverage);

	int64_t _nTargetTimespan = CountBlocks * params.nPowTargetSpacing;

	if (nActualTimespan < _nTargetTimespan / 3)
		nActualTimespan = _nTargetTimespan / 3;
	if (nActualTimespan > _nTargetTimespan * 3)
		nActualTimespan = _nTargetTimespan * 3;

	// Retarget
	bnNew *= nActualTimespan;
	bnNew /= _nTargetTimespan;

	if (bnNew > nProofOfWorkLimit) {
		bnNew = nProofOfWorkLimit;
	}

	return bnNew.GetCompact();
}

bool CheckProofOfWork(uint256 hash, unsigned int nBits, const Consensus::Params& params)
{
    bool fNegative;
    bool fOverflow;
    arith_uint256 bnTarget;

    bnTarget.SetCompact(nBits, &fNegative, &fOverflow);
	// SYSCOIN
	arith_uint256 nProofOfWorkLimit = UintToArith256(params.powLimit);
	if(chainActive.Height() <= 400)
		nProofOfWorkLimit = UintToArith256(Params(CBaseChainParams::REGTEST).GetConsensus().powLimit);
    // Check range
    if (fNegative || bnTarget == 0 || fOverflow || bnTarget > nProofOfWorkLimit)
        return error("CheckProofOfWork(): nBits below minimum work");

    // Check proof of work matches claimed amount
    if (UintToArith256(hash) > bnTarget)
        return error("CheckProofOfWork(): hash doesn't match nBits");

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

int64_t GetBlockProofEquivalentTime(const CBlockIndex& to, const CBlockIndex& from, const CBlockIndex& tip, const Consensus::Params& params)
{
    arith_uint256 r;
    int sign = 1;
    if (to.nChainWork > from.nChainWork) {
        r = to.nChainWork - from.nChainWork;
    } else {
        r = from.nChainWork - to.nChainWork;
        sign = -1;
    }
    r = r * arith_uint256(params.nPowTargetSpacing) / GetBlockProof(tip);
    if (r.bits() > 63) {
        return sign * std::numeric_limits<int64_t>::max();
    }
    return sign * r.GetLow64();
}
