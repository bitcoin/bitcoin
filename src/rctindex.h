// Copyright (c) 2017 The Particl Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef PARTICL_RCTINDEX_H
#define PARTICL_RCTINDEX_H

#include <primitives/transaction.h>

class CAnonOutput
{
// Stored in txdb, key is 64bit index
public:
    CAnonOutput() : nBlockHeight(0), nCompromised(0) {};
    CAnonOutput(CCmpPubKey pubkey_, secp256k1_pedersen_commitment commitment_, COutPoint &outpoint_, int nBlockHeight_, uint8_t nCompromised_)
        : pubkey(pubkey_), commitment(commitment_), outpoint(outpoint_), nBlockHeight(nBlockHeight_), nCompromised(nCompromised_) {};

    CCmpPubKey pubkey;
    secp256k1_pedersen_commitment commitment;
    COutPoint outpoint;
    int nBlockHeight;
    uint8_t nCompromised;   // TODO: mark if output can be identified (spent with ringsize 1)

    ADD_SERIALIZE_METHODS;
    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action)
    {
        READWRITE(pubkey);
        if (ser_action.ForRead())
            s.read((char*)&commitment.data[0], 33);
        else
            s.write((char*)&commitment.data[0], 33);

        READWRITE(outpoint);
        READWRITE(nBlockHeight);
        READWRITE(nCompromised);
    };
};


#endif // PARTICL_RCTINDEX_H

