// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_BLOCKCHAIN_H
#define BITCOIN_BLOCKCHAIN_H

#include "chainparams.h"
#include "consensus/validation.h"
#include "primitives/block.h"

class CBlockchain {
protected:
    static bool CheckBlockHeader(const CChainParams chainParams, const CBlockHeader& block, CValidationState& state, int64_t nAdjustedTime, bool fCheckPOW = true);
public:
    /** Context-independent validity checks */
    static bool CheckTransaction(const CTransaction& tx, CValidationState& state);
    static bool CheckBlockHeader(const CBlockHeader& block, CValidationState& state, bool fCheckPOW = true);
};

#endif // BITCOIN_BLOCKCHAIN_H
