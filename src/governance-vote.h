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

#define VOTE_ABSTAIN  0
#define VOTE_YES      1
#define VOTE_NO       2

//
// CBudgetVote - Allow a masternode node to vote and broadcast throughout the network
//

class CBudgetVote
{
    //# ----
public:
    bool fValid; //if the vote is currently valid / counted
    bool fSynced; //if we've sent this to our peers
    int nVoteType; //fund, end, invalid, reports
    /* 
        fund=1 = shall we fund this?
        url=2= is the url correct?
        amount=3 is the amount correct?
        expiration is the expiration correct?
        address is the address correct?
        parent=4 to 10 is variable x correct?
        reports=5= Are all reports up to date?
        spam=6= Is this spam?
        name=7= Is the name correct? 

        certifications:

        business=7  Has it been certified by the business committee? (if exists)
        technical=8 Has it been certified by the technical committee?
        scientific=9 etc

    */
    CTxIn vin;
    uint256 nProposalHash;
    int nVote;
    int64_t nTime;
    std::vector<unsigned char> vchSig;

    CBudgetVote();
    CBudgetVote(CTxIn vin, uint256 nProposalHash, int nVoteIn);

    bool Sign(CKey& keyMasternode, CPubKey& pubKeyMasternode);
    bool IsValid(bool fSignatureCheck);
    void Relay();

    std::string GetVoteString() {
        std::string ret = "ABSTAIN";
        if(nVote == VOTE_YES) ret = "YES";
        if(nVote == VOTE_NO) ret = "NO";
        return ret;
    }

    uint256 GetHash(){
        CHashWriter ss(SER_GETHASH, PROTOCOL_VERSION);
        ss << vin;
        ss << nProposalHash;
        ss << nVote;
        ss << nTime;
        return ss.GetHash();
    }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(vin);
        READWRITE(nProposalHash);
        READWRITE(nVote);
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
