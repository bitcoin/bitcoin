// Copyright (c) 2017 The Particl Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef PARTICL_RCTINDEX_H
#define PARTICL_RCTINDEX_H

#include "primitives/transaction.h"

class CKeyImageSpent
{
// Stored in txdb, key is keyimage
public:
    CKeyImageSpent() {};

    CKeyImageSpent(uint256& txnHash_, uint32_t inputNo_)
        : txnHash(txnHash_), inputNo(inputNo_) {};

    COutPoint outpoint;
    uint256 txnHash;    // hash of spending transaction
    uint32_t inputNo;   // keyimage is for inputNo of txnHash

    ADD_SERIALIZE_METHODS;
    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action)
    {
        READWRITE(txnHash);
        READWRITE(inputNo);
    };
};

class CAnonOutput
{
// Stored in txdb, key is 64bit index
public:
    CAnonOutput() : nBlockHeight(0), nCompromised(0) {};
    CAnonOutput(CPubKey pubkey_, COutPoint &outpoint_, int nBlockHeight_, uint8_t nCompromised_)
        : pubkey(pubkey_), outpoint(outpoint_), nBlockHeight(nBlockHeight_), nCompromised(nCompromised_) {};

    CPubKey pubkey;
    COutPoint outpoint;
    int nBlockHeight;
    uint8_t nCompromised;   // TODO: mark if output can be identified (spent with ringsize 1)

    ADD_SERIALIZE_METHODS;
    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action)
    {
        READWRITE(pubkey);
        READWRITE(outpoint);
        READWRITE(nBlockHeight);
        READWRITE(nCompromised);
    };
};


#endif // PARTICL_RCTINDEX_H

