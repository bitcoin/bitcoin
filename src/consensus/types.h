// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2013 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CONSENSUS_TYPES_H
#define BITCOIN_CONSENSUS_TYPES_H

#include "uint256.h"

struct CBlockIndexBase
{
    //! pointer to the hash of the block, if any. Memory is owned by this CBlockIndexBase
    const uint256* phashBlock;
    //! block header
    int32_t nVersion;
    uint256 hashMerkleRoot;
    uint32_t nTime;
    uint32_t nBits;
    uint32_t nNonce;
    //! height of the entry in the chain. The genesis block has height 0
    int nHeight;
};

typedef const CBlockIndexBase* (*PrevIndexGetter)(const CBlockIndexBase*);

#endif // BITCOIN_CONSENSUS_TYPES_H
