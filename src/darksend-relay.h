
// Copyright (c) 2014-2015 The Dash developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef DARKSEND_RELAY_H
#define DARKSEND_RELAY_H

<<<<<<< HEAD
#include "main.h"
#include "activemasternode.h"
#include "masternodeman.h"
=======
#include "core.h"
#include "main.h"
#include "activethrone.h"
#include "throneman.h"
>>>>>>> origin/dirty-merge-dash-0.11.0


class CDarkSendRelay
{
public:
<<<<<<< HEAD
    CTxIn vinMasternode;
=======
    CTxIn vinThrone;
>>>>>>> origin/dirty-merge-dash-0.11.0
    vector<unsigned char> vchSig;
    vector<unsigned char> vchSig2;
    int nBlockHeight;
    int nRelayType;
    CTxIn in;
    CTxOut out;

    CDarkSendRelay();
<<<<<<< HEAD
    CDarkSendRelay(CTxIn& vinMasternodeIn, vector<unsigned char>& vchSigIn, int nBlockHeightIn, int nRelayTypeIn, CTxIn& in2, CTxOut& out2);
    
    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(vinMasternode);
=======
    CDarkSendRelay(CTxIn& vinThroneIn, vector<unsigned char>& vchSigIn, int nBlockHeightIn, int nRelayTypeIn, CTxIn& in2, CTxOut& out2);
    
    IMPLEMENT_SERIALIZE
    (
    	READWRITE(vinThrone);
>>>>>>> origin/dirty-merge-dash-0.11.0
        READWRITE(vchSig);
        READWRITE(vchSig2);
        READWRITE(nBlockHeight);
        READWRITE(nRelayType);
        READWRITE(in);
        READWRITE(out);
<<<<<<< HEAD
    }
=======
    )
>>>>>>> origin/dirty-merge-dash-0.11.0

    std::string ToString();

    bool Sign(std::string strSharedKey);
    bool VerifyMessage(std::string strSharedKey);
    void Relay();
    void RelayThroughNode(int nRank);
};



#endif
