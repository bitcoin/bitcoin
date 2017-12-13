// Copyright (c) 2014-2017 The Dash Core developers

// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef GOVERNANCE_VOTE_H
#define GOVERNANCE_VOTE_H

#include "key.h"
#include "primitives/transaction.h"

#include <boost/lexical_cast.hpp>

using namespace std;

class CGovernanceVote;
class CConnman;

// INTENTION OF MASTERNODES REGARDING ITEM
enum vote_outcome_enum_t  {
    VOTE_OUTCOME_NONE      = 0,
    VOTE_OUTCOME_YES       = 1,
    VOTE_OUTCOME_NO        = 2,
    VOTE_OUTCOME_ABSTAIN   = 3
};


// SIGNAL VARIOUS THINGS TO HAPPEN:
enum vote_signal_enum_t  {
    VOTE_SIGNAL_NONE       = 0,
    VOTE_SIGNAL_FUNDING    = 1, //   -- fund this object for it's stated amount
    VOTE_SIGNAL_VALID      = 2, //   -- this object checks out in sentinel engine
    VOTE_SIGNAL_DELETE     = 3, //   -- this object should be deleted from memory entirely
    VOTE_SIGNAL_ENDORSED   = 4, //   -- officially endorsed by the network somehow (delegation)
    VOTE_SIGNAL_NOOP1      = 5, // FOR FURTHER EXPANSION
    VOTE_SIGNAL_NOOP2      = 6,
    VOTE_SIGNAL_NOOP3      = 7,
    VOTE_SIGNAL_NOOP4      = 8,
    VOTE_SIGNAL_NOOP5      = 9,
    VOTE_SIGNAL_NOOP6      = 10,
    VOTE_SIGNAL_NOOP7      = 11,
    VOTE_SIGNAL_NOOP8      = 12,
    VOTE_SIGNAL_NOOP9      = 13,
    VOTE_SIGNAL_NOOP10     = 14,
    VOTE_SIGNAL_NOOP11     = 15,
    VOTE_SIGNAL_CUSTOM1    = 16,  // SENTINEL CUSTOM ACTIONS
    VOTE_SIGNAL_CUSTOM2    = 17,  //        16-35
    VOTE_SIGNAL_CUSTOM3    = 18,
    VOTE_SIGNAL_CUSTOM4    = 19,
    VOTE_SIGNAL_CUSTOM5    = 20,
    VOTE_SIGNAL_CUSTOM6    = 21,
    VOTE_SIGNAL_CUSTOM7    = 22,
    VOTE_SIGNAL_CUSTOM8    = 23,
    VOTE_SIGNAL_CUSTOM9    = 24,
    VOTE_SIGNAL_CUSTOM10   = 25,
    VOTE_SIGNAL_CUSTOM11   = 26,
    VOTE_SIGNAL_CUSTOM12   = 27,
    VOTE_SIGNAL_CUSTOM13   = 28,
    VOTE_SIGNAL_CUSTOM14   = 29,
    VOTE_SIGNAL_CUSTOM15   = 30,
    VOTE_SIGNAL_CUSTOM16   = 31,
    VOTE_SIGNAL_CUSTOM17   = 32,
    VOTE_SIGNAL_CUSTOM18   = 33,
    VOTE_SIGNAL_CUSTOM19   = 34,
    VOTE_SIGNAL_CUSTOM20   = 35
};

static const int MAX_SUPPORTED_VOTE_SIGNAL = VOTE_SIGNAL_ENDORSED;

/**
* Governance Voting
*
*   Static class for accessing governance data
*/

class CGovernanceVoting
{
public:
    static vote_outcome_enum_t ConvertVoteOutcome(std::string strVoteOutcome);
    static vote_signal_enum_t ConvertVoteSignal(std::string strVoteSignal);
    static std::string ConvertOutcomeToString(vote_outcome_enum_t nOutcome);
    static std::string ConvertSignalToString(vote_signal_enum_t nSignal);
};

//
// CGovernanceVote - Allow a masternode node to vote and broadcast throughout the network
//

class CGovernanceVote
{
    friend bool operator==(const CGovernanceVote& vote1, const CGovernanceVote& vote2);

    friend bool operator<(const CGovernanceVote& vote1, const CGovernanceVote& vote2);

private:
    bool fValid; //if the vote is currently valid / counted
    bool fSynced; //if we've sent this to our peers
    int nVoteSignal; // see VOTE_ACTIONS above
    CTxIn vinMasternode;
    uint256 nParentHash;
    int nVoteOutcome; // see VOTE_OUTCOMES above
    int64_t nTime;
    std::vector<unsigned char> vchSig;

public:
    CGovernanceVote();
    CGovernanceVote(COutPoint outpointMasternodeIn, uint256 nParentHashIn, vote_signal_enum_t eVoteSignalIn, vote_outcome_enum_t eVoteOutcomeIn);

    bool IsValid() const { return fValid; }

    bool IsSynced() const { return fSynced; }

    int64_t GetTimestamp() const { return nTime; }

    vote_signal_enum_t GetSignal() const  { return vote_signal_enum_t(nVoteSignal); }

    vote_outcome_enum_t GetOutcome() const  { return vote_outcome_enum_t(nVoteOutcome); }

    const uint256& GetParentHash() const { return nParentHash; }

    void SetTime(int64_t nTimeIn) { nTime = nTimeIn; }

    void SetSignature(const std::vector<unsigned char>& vchSigIn) { vchSig = vchSigIn; }

    bool Sign(CKey& keyMasternode, CPubKey& pubKeyMasternode);
    bool IsValid(bool fSignatureCheck) const;
    void Relay(CConnman& connman) const;

    std::string GetVoteString() const {
        return CGovernanceVoting::ConvertOutcomeToString(GetOutcome());
    }

    const COutPoint& GetMasternodeOutpoint() const { return vinMasternode.prevout; }

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

    std::string ToString() const
    {
        std::ostringstream ostr;
        ostr << vinMasternode.ToString() << ":"
             << nTime << ":"
             << CGovernanceVoting::ConvertOutcomeToString(GetOutcome()) << ":"
             << CGovernanceVoting::ConvertSignalToString(GetSignal());
        return ostr.str();
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
