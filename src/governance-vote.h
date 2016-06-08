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

class CGovernanceVote;

#define VOTE_OUTCOME_NONE     0
#define VOTE_OUTCOME_YES      1
#define VOTE_OUTCOME_NO       2
#define VOTE_OUTCOME_ABSTAIN  3
// INTENTION OF MASTERNODES REGARDING ITEM

#define VOTE_SIGNAL_NONE                0 // SIGNAL VARIOUS THINGS TO HAPPEN:
#define VOTE_SIGNAL_FUNDING             1 //   -- fund this object for it's stated amount
#define VOTE_SIGNAL_VALID               2 //   -- this object checks out in sentinel engine
#define VOTE_SIGNAL_DELETE              3 //   -- this object should be deleted from memory entirely
#define VOTE_SIGNAL_ENDORSED            4 //   -- officially endorsed by the network somehow (delegation)
#define VOTE_SIGNAL_NOOP1               5 // FOR FURTHER EXPANSION
#define VOTE_SIGNAL_NOOP2               6 // 
#define VOTE_SIGNAL_NOOP3               7 // 
#define VOTE_SIGNAL_NOOP4               8 // 
#define VOTE_SIGNAL_NOOP5               9 // 
#define VOTE_SIGNAL_NOOP6               10 // 
#define VOTE_SIGNAL_NOOP7               11 // 
#define VOTE_SIGNAL_NOOP8               12 // 
#define VOTE_SIGNAL_NOOP9               13 // 
#define VOTE_SIGNAL_NOOP10              14 // 
#define VOTE_SIGNAL_NOOP11              15 // 
#define VOTE_SIGNAL_CUSTOM_START        16 // SENTINEL CUSTOM ACTIONS 
#define VOTE_SIGNAL_CUSTOM_END          35 //        16-35

//
// CGovernanceVote - Allow a masternode node to vote and broadcast throughout the network
//

class CGovernanceVote
{
public:
    bool fValid; //if the vote is currently valid / counted
    bool fSynced; //if we've sent this to our peers
    int nVoteSignal; // see VOTE_ACTIONS above
    CTxIn vinMasternode;
    uint256 nParentHash;
    int nVoteOutcome; // see VOTE_OUTCOMES above
    int64_t nTime;
    std::vector<unsigned char> vchSig;

    CGovernanceVote();
    CGovernanceVote(CTxIn vinMasternodeIn, uint256 nParentHashIn, int nVoteSignalIn, int nVoteOutcomeIn);

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

    /**
    *   GetHash()
    *
    *   GET UNIQUE HASH WITH DETERMINISTIC VALUE OF THIS SPECIFIC VOTE
    */

    uint256 GetHash() const
    {
        CHashWriter ss(SER_GETHASH, PROTOCOL_VERSION);
        ss << vinMasternode;
        ss << nParentHash;
        ss << nVoteSignal;
        ss << nVoteOutcome;
        ss << nTime;
        return ss.GetHash();
    }

    /**
    *   GetTypeHash()
    *
    *   GET HASH WITH DETERMINISTIC VALUE OF MASTERNODE-VIN/PARENT-HASH/VOTE-SIGNAL
    *
    *   This hash collides with previous masternode votes when they update their votes on governance objects. 
    *   With 12.1 there's various types of votes (funding, valid, delete, etc), so this is the deterministic hash 
    *   that will collide with the previous vote and allow the system to update.
    *   
    *   --
    *
    *   We do not include an outcome, because that can change when a masternode updates their vote from yes to no
    *   on funding a specific project for example. 
    *   We do not include a time because it will be updated each time the vote is updated, changing the hash
    */
    uint256 GetTypeHash() const
    {       
        // CALCULATE HOW TO STORE VOTE IN governance.mapVotes

        CHashWriter ss(SER_GETHASH, PROTOCOL_VERSION);
        ss << vinMasternode;
        ss << nParentHash;
        ss << nVoteSignal;
        //  -- no outcome 
        //  -- timeless
        return ss.GetHash();
    }

    uint256 GetParentHash(){
        return nParentHash;
    }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(vinMasternode);
        READWRITE(nParentHash);
        READWRITE(nVoteOutcome);
        READWRITE(nVoteSignal);
        READWRITE(nTime);
        READWRITE(vchSig);
    }

};


/** 
* 12.1.1 - CGovernanceVoteManager
* -------------------------------
*

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
