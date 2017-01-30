// Copyright (c) 2016-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "header_verify.h"

#include "params.h"
#include "pow.h"
#include "primitives/block.h"
#include "script/generic.hpp"
#include "script/interpreter.h"
#include "validation.h"

#define BLOCK_SIGN_SCRIPT_FLAGS SCRIPT_VERIFY_NONE // TODO signblocks: complete

bool CheckProof(const Consensus::Params& consensusParams, const CBlockHeader& block)
{
    if (consensusParams.fSignBlockChain) {
        if (!block.proof.script ||
            !GenericVerifyScript(*block.proof.script, consensusParams.blocksignScript, BLOCK_SIGN_SCRIPT_FLAGS, block)) {
            return false;
        }
    } else if (!CheckProofOfWork(block.GetHash(), block.proof.pow.nBits, consensusParams))
        return false;

    return true;
}

bool MaybeGenerateProof(const Consensus::Params& consensusParams, CBlockHeader* pblock, const CKeyStore* pkeystore, uint64_t& nTries)
{
    if (consensusParams.fSignBlockChain) {
        if (!pkeystore)
            return false;
        SignatureData solution(*pblock->proof.script);
        nTries = 0;
        if (!GenericSignScript(*pkeystore, *pblock, consensusParams.blocksignScript, solution))
            return false;
        pblock->proof.script = new CScript(solution.scriptSig);
        return CheckProof(consensusParams, *pblock);
    }

    const int nInnerLoopCount = 0x10000;
    while (nTries > 0 && pblock->proof.pow.nNonce < nInnerLoopCount && !CheckProofOfWork(pblock->GetHash(), pblock->proof.pow.nBits, consensusParams)) {
        ++pblock->proof.pow.nNonce;
        --nTries;
    }
    return CheckProofOfWork(pblock->GetHash(), pblock->proof.pow.nBits, consensusParams);
}

bool GenerateProof(const Consensus::Params& consensusParams, CBlockHeader* pblock, const CKeyStore* pkeystore)
{
    uint64_t nTries = 10000;
    return MaybeGenerateProof(consensusParams, pblock, pkeystore, nTries);
}

void ResetProof(const Consensus::Params& consensusParams, CBlockHeader* pblock)
{
    if (consensusParams.fSignBlockChain) 
        *pblock->proof.script = CScript();
    else
        pblock->proof.pow.nNonce = 0;
}

bool CheckChallenge(const Consensus::Params& consensusParams, CValidationState& state, const CBlockHeader *pblock, const CBlockIndex* pindexPrev)
{
    if (consensusParams.fSignBlockChain)
        return true; // The scriptPubKey for blocksigning is constant in consensusParams
    else if (pblock->proof.pow.nBits != GetNextWorkRequired(pindexPrev, pblock, consensusParams))
        return state.DoS(100, false, REJECT_INVALID, "bad-diffbits", false, "incorrect proof of work");

    return true;
}

void ResetChallenge(const Consensus::Params& consensusParams, CBlockHeader* pblock, const CBlockIndex* pindexPrev)
{
    // The scriptPubKey for blocksigning is constant in consensusParams
    if (!consensusParams.fSignBlockChain)
        pblock->proof.pow.nBits = GetNextWorkRequired(pindexPrev, pblock, consensusParams);
}
