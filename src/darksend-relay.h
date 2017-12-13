
// Copyright (c) 2014-2017 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef DARKSEND_RELAY_H
#define DARKSEND_RELAY_H

#include "validation.h"
#include "activemasternode.h"
#include "masternodeman.h"


class CDarkSendRelay
{
public:
    CTxIn vinMasternode;
    vector<unsigned char> vchSig;
    vector<unsigned char> vchSig2;
    int nBlockHeight;
    int nRelayType;
    CTxIn in;
    CTxOut out;

    CDarkSendRelay();
    CDarkSendRelay(CTxIn& vinMasternodeIn, vector<unsigned char>& vchSigIn, int nBlockHeightIn, int nRelayTypeIn, CTxIn& in2, CTxOut& out2);
    
    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(vinMasternode);
        READWRITE(vchSig);
        READWRITE(vchSig2);
        READWRITE(nBlockHeight);
        READWRITE(nRelayType);
        READWRITE(in);
        READWRITE(out);
    }

    std::string ToString();

    bool Sign(std::string strSharedKey);
    bool VerifyMessage(std::string strSharedKey);
    void Relay();
    void RelayThroughNode(int nRank);
};



#endif
