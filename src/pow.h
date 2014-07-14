// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_POW_H
#define BITCOIN_POW_H

#include "serialize.h"

#include <stdint.h>

class CBlockIndex;
class CBlockHeader;
class uint256;

/** Check the work is more than the minimum a received block needs, without knowing its direct parent */
bool CheckMinWork(unsigned int nBits, unsigned int nBase, int64_t deltaTime);

class CProof
{
public:
    uint32_t nTime;
    uint32_t nBits;
    uint32_t nNonce;

    CProof();
    bool CheckSolution(const uint256 hash) const;
    void UpdateTime(const CBlockIndex* pindexPrev);
    uint256 GetProofIncrement() const;
    int64_t GetBlockTime() const;
    bool IsNull() const;
    void SetNull();

    IMPLEMENT_SERIALIZE;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(nTime);
        READWRITE(nBits);
        READWRITE(nNonce);
    }

    CProof(unsigned int nTime, unsigned int nBits, unsigned int nNonce);
    unsigned int GetNextChallenge(const CBlockIndex* pindexLast) const;
};

#endif // BITCOIN_POW_H
