// Copyright (c) 2014-2016 The Dash Core developers

// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef GOVERNANCE_VOTE_H
#define GOVERNANCE_VOTE_H

//todo: which of these do we need?
#include "main.h"
//#include "sync.h"
//#include "net.h"
#include "key.h"
#include "util.h"
#include "base58.h"
#include "masternode.h"
//#include "governance.h"
//#include "governance-types.h"
//#include "governance-classes.h"
#include <boost/lexical_cast.hpp>

using namespace std;

extern CCriticalSection cs_budget;

class CGovernanceNode;

//
// CGovernanceVote - Allow a masternode node to vote and broadcast throughout the network
//

class CGovernanceVote
{   // **** Objects and memory ****************************************************
    
public:
    int nGovernanceType; //GovernanceObjectType
    bool fValid; //if the vote is currently valid / counted
    bool fSynced; //if we've sent this to our peers
    CTxIn vin;
    uint256 nParentHash; //Object hash which is the parent (proposal, setting, contract, final budget)
    int nVote;
    int64_t nTime;
    std::vector<unsigned char> vchSig;

    CGovernanceNode* pParent;
    
    // **** Initialization ********************************************************

    CGovernanceVote();
    CGovernanceVote(CGovernanceNode& pParentNodeIn, CTxIn vin, uint256 nParentHashIn, int nVoteIn);

    // **** Update ****************************************************************

    bool Sign(CKey& keyMasternode, CPubKey& pubKeyMasternode);
    void SetParent(CGovernanceNode* pGovObjectParent);
    
    // **** Statistics / Information **********************************************

    GovernanceObjectType GetType();
    int64_t GetValidStartTimestamp();
    int64_t GetValidEndTimestamp();

    bool IsValid(bool fSignatureCheck, std::string& strReason);

    std::string GetVoteString() {
        std::string ret = "ABSTAIN";
        if(nVote == VOTE_YES) ret = "YES";
        if(nVote == VOTE_NO) ret = "NO";
        return ret;
    }

    uint256 GetHash(){
        CHashWriter ss(SER_GETHASH, PROTOCOL_VERSION);
        ss << nGovernanceType;
        ss << vin;
        ss << nParentHash;
        ss << nVote;
        ss << nTime;
        ss << nGovernanceType;
        return ss.GetHash();
    }

    void Relay();

    // **** Serializer ************************************************************

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(vin);
        READWRITE(nParentHash);
        READWRITE(nVote);
        READWRITE(nTime);
        READWRITE(vchSig);

        // TODO : For testnet version bump
        READWRITE(nGovernanceType);
    }

};


#endif