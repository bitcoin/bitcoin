// Copyright (c) 2016-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "header_verify.h"

#include "params.h"
#include "pow.h"
#include "primitives/block.h"
#include "validation.h"

bool CheckProof(const Consensus::Params& consensusParams, const CBlockHeader& block)
{
    if (!CheckProofOfWork(block.GetHash(), block.proof.pow.nBits, consensusParams))
        return false;

    return true;
}

bool MaybeGenerateProof(const Consensus::Params& consensusParams, CBlockHeader* pblock, uint64_t& nTries)
{
    const int nInnerLoopCount = 0x10000;
    while (nTries > 0 && pblock->proof.pow.nNonce < nInnerLoopCount && !CheckProofOfWork(pblock->GetHash(), pblock->proof.pow.nBits, consensusParams)) {
        ++pblock->proof.pow.nNonce;
        --nTries;
    }
    return CheckProofOfWork(pblock->GetHash(), pblock->proof.pow.nBits, consensusParams);
}

bool GenerateProof(const Consensus::Params& consensusParams, CBlockHeader* pblock)
{
    uint64_t nTries = 10000;
    return MaybeGenerateProof(consensusParams, pblock, nTries);
}

void ResetProof(const Consensus::Params& consensusParams, CBlockHeader* pblock)
{
    pblock->proof.pow.nNonce = 0;
}

bool CheckChallenge(const Consensus::Params& consensusParams, CValidationState& state, const CBlockHeader *pblock, const CBlockIndex* pindexPrev)
{
    if (pblock->proof.pow.nBits != GetNextWorkRequired(pindexPrev, pblock, consensusParams))
        return state.DoS(100, false, REJECT_INVALID, "bad-diffbits", false, "incorrect proof of work");

    return true;
}

void ResetChallenge(const Consensus::Params& consensusParams, CBlockHeader* pblock, const CBlockIndex* pindexPrev)
{
    pblock->proof.pow.nBits = GetNextWorkRequired(pindexPrev, pblock, consensusParams);
}
