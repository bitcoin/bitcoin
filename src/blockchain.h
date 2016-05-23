// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_BLOCKCHAIN_H
#define BITCOIN_BLOCKCHAIN_H

#include "consensus/validation.h"
#include "primitives/block.h"

class CBlockchain {
public:
    static bool CheckBlockHeader(const CBlockHeader& block, CValidationState& state, bool fCheckPOW = true);
};

#endif // BITCOIN_BLOCKCHAIN_H
