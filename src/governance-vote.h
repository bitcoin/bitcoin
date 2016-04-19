// Copyright (c) 2014-2016 The Dash Core developers

// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef GOVERANCE_VOTE_H
#define GOVERANCE_VOTE_H

#include "main.h"
#include "sync.h"
#include "net.h"
#include "key.h"
#include "util.h"
#include "base58.h"
#include "masternode.h"
#include <boost/lexical_cast.hpp>
#include "init.h"

using namespace std;

class CBudgetVote;

#define VOTE_OUTCOME_NONE     0
#define VOTE_OUTCOME_YES      1
#define VOTE_OUTCOME_NO       2
#define VOTE_OUTCOME_ABSTAIN  3
// INTENTION OF MASTERNODES REGARDING ITEM

#define VOTE_ACTION_NONE                0
#define VOTE_ACTION_FUNDING             1 //SIGNAL TO FUND GOVOBJ
#define VOTE_ACTION_VALID               2 //SIGNAL GOVOBJ IS VALID OR NOT
#define VOTE_ACTION_UPTODATE            3 //SIGNAL ALL REQUIRED INFORMATION IS UP-TO-DATE (PROJECTS/MILESTONES/REPORTS)
#define VOTE_ACTION_DELETE              4 //SIGNAL TO DELETE NODE AND CHILDREN FROM SYSTEM
#define VOTE_ACTION_CLEAR_REGISTERS     5 //SIGNAL TO CLEAR REGISTER DATA (DASHDRIVE or other outer-storage implementations)
#define VOTE_ACTION_ENDORSED            6 //SIGNAL GOVOBJ IS ENDORSED BY REVIEW COMMITTEES

//
// CBudgetVote - Allow a masternode node to vote and broadcast throughout the network
//

class CBudgetVote
{
    //# ----
public:
    bool fValid; //if the vote is currently valid / counted
    bool fSynced; //if we've sent this to our peers
    int nVoteType; // see VOTE_OUTCOMES above
    CTxIn vinMasternode;
    uint256 nParentHash;
    int nVoteOutcome;
    int64_t nTime;
    std::vector<unsigned char> vchSig;

    CBudgetVote();
    CBudgetVote(CTxIn vinMasternode, uint256 nParentHash, int nVoteTypeIn, int nVoteOutcomeIn);

    bool Sign(CKey& keyMasternode, CPubKey& pubKeyMasternode);
    bool IsValid(bool fSignatureCheck);
    void Relay();

    std::string GetVoteString() {
        std::string ret = "ERROR";
        if(nVoteOutcome == VOTE_OUTCOME_NONE)         ret = "NONE";
        else if(nVoteOutcome == VOTE_OUTCOME_ABSTAIN) ret = "ABSTAIN";
        else if(nVoteOutcome == VOTE_OUTCOME_YES)     ret = "YES";
        else if(nVoteOutcome == VOTE_OUTCOME_NO)      ret = "NO";
        return ret;
    }

    uint256 GetHash(){
        CHashWriter ss(SER_GETHASH, PROTOCOL_VERSION);
        ss << vinMasternode;
        ss << nParentHash;
        ss << nVoteType;
        ss << nVoteOutcome;
        ss << nTime;
        return ss.GetHash();
    }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(vinMasternode);
        READWRITE(nParentHash);
        READWRITE(nVoteOutcome);
        READWRITE(nVoteType);
        READWRITE(nTime);
        READWRITE(vchSig);
    }

};


/** 
* 12.1.1 - CGovernanceVoteManager
* -------------------------------
*

    Class Structure:

    //       parent hash       vote hash     vote
    std::map<uint256, std::map<uint256, CBudgetVote> > mapVotes;

    GetVote(name, yes_no):
        - caching function
        - mark last accessed votes
        - load serialized files from filesystem if needed
        - calc answer
        - return result
    
    CacheUnused():
        - Cache votes if lastused > 12h/24/48/etc

*/

#endif
