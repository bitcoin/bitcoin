// Copyright (c) 2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "consensus.h"

#include "interfaces.h"
#include "pow.h"
#include "primitives/block.h"
#include "tinyformat.h"
#include "validation.h"

bool CheckBlockHeader(const CBlockHeader& block, CValidationState& state, const Consensus::Params& consensusParams, bool fCheckPOW)
{
    // Check proof of work matches claimed amount
    if (fCheckPOW && !CheckProofOfWork(block.GetHash(), block.nBits, consensusParams))
        return state.DoS(50, false, REJECT_INVALID, "high-hash", false, "proof of work failed");

    return true;
}

bool ContextualCheckHeader(const CBlockHeader& block, CValidationState& state, const Consensus::Params& consensusParams, const void* indexObject, const BlockIndexInterface& iBlockIndex, int64_t nAdjustedTime)
{
    const int nHeight = indexObject == NULL ? 0 : iBlockIndex.GetHeight(indexObject) + 1;
    // Check proof of work
    if (block.nBits != PowGetNextWorkRequired(indexObject, iBlockIndex, &block, consensusParams))
        return state.DoS(100, false, REJECT_INVALID, "bad-diffbits", false, "incorrect proof of work");

    // Check timestamp against prev
    if (block.GetBlockTime() <= iBlockIndex.GetMedianTime(indexObject))
        return state.Invalid(false, REJECT_INVALID, "time-too-old", "block's timestamp is too early");

    // Check timestamp
    if (block.GetBlockTime() > nAdjustedTime + 2 * 60 * 60)
        return state.Invalid(false, REJECT_INVALID, "time-too-new", "block timestamp too far in the future");

    // Reject outdated version blocks when 95% (75% on testnet) of the network has upgraded:
    // check for version 2, 3 and 4 upgrades
    if((block.nVersion < 2 && nHeight >= consensusParams.BIP34Height) ||
       (block.nVersion < 3 && nHeight >= consensusParams.BIP66Height) ||
       (block.nVersion < 4 && nHeight >= consensusParams.BIP65Height))
            return state.Invalid(false, REJECT_OBSOLETE, strprintf("bad-version(0x%08x)", block.nVersion),
                                 strprintf("rejected nVersion=0x%08x block", block.nVersion));

    return true;
}

bool VerifyHeader(const CBlockHeader& block, CValidationState& state, const Consensus::Params& consensusParams, const void* indexObject, const BlockIndexInterface& iBlockIndex, int64_t nAdjustedTime)
{
    if (!CheckBlockHeader(block, state, consensusParams, true))
        return false;

    if (!ContextualCheckHeader(block, state, consensusParams, indexObject, iBlockIndex, nAdjustedTime))
        return false;

    return true;
}
