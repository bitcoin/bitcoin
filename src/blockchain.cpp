// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "blockchain.h"
#include "consensus/validation.h"
#include "chainparams.h"
#include "pow.h"
#include "primitives/block.h"
#include "timedata.h"

bool CBlockchain::CheckBlockHeader(const CBlockHeader& block, CValidationState& state, bool fCheckPOW) {
    // Check proof of work matches claimed amount
    if (fCheckPOW && !CheckProofOfWork(block.GetHash(), block.nBits, Params().GetConsensus()))
        return state.DoS(50, false, REJECT_INVALID, "high-hash", false, "proof of work failed");

    // Check timestamp
    if (block.GetBlockTime() > GetAdjustedTime() + 2 * 60 * 60)
        return state.Invalid(false, REJECT_INVALID, "time-too-new", "block timestamp too far in the future");

    return true;
}
