// Copyright (c) 2014-2017 The Dash Core developers

// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef GOVERNANCE_VOTE_H
#define GOVERNANCE_VOTE_H

#include "key.h"
#include "primitives/transaction.h"

#include <boost/lexical_cast.hpp>

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
    static vote_outcome_enum_t ConvertVoteOutcome(const std::string& strVoteOutcome);
    static vote_signal_enum_t ConvertVoteSignal(const std::string& strVoteSignal);
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
    COutPoint masternodeOutpoint;
    uint256 nParentHash;
    int nVoteOutcome; // see VOTE_OUTCOMES above
    int64_t nTime;
    std::vector<unsigned char> vchSig;

    /** Memory only. */
    const uint256 hash;
    void UpdateHash() const;

public:
    CGovernanceVote();
    CGovernanceVote(const COutPoint& outpointMasternodeIn, const uint256& nParentHashIn, vote_signal_enum_t eVoteSignalIn, vote_outcome_enum_t eVoteOutcomeIn);

    bool IsValid() const { return fValid; }

    bool IsSynced() const { return fSynced; }

    int64_t GetTimestamp() const { return nTime; }

    vote_signal_enum_t GetSignal() const  { return vote_signal_enum_t(nVoteSignal); }

    vote_outcome_enum_t GetOutcome() const  { return vote_outcome_enum_t(nVoteOutcome); }

    const uint256& GetParentHash() const { return nParentHash; }

    void SetTime(int64_t nTimeIn) { nTime = nTimeIn; UpdateHash(); }

    void SetSignature(const std::vector<unsigned char>& vchSigIn) { vchSig = vchSigIn; }

    bool Sign(const CKey& keyMasternode, const CPubKey& pubKeyMasternode);
    bool CheckSignature(const CPubKey& pubKeyMasternode) const;
    bool IsValid(bool fSignatureCheck) const;
    void Relay(CConnman& connman) const;

    std::string GetVoteString() const {
        return CGovernanceVoting::ConvertOutcomeToString(GetOutcome());
    }

    const COutPoint& GetMasternodeOutpoint() const { return masternodeOutpoint; }

    /**
    *   GetHash()
    *
    *   GET UNIQUE HASH WITH DETERMINISTIC VALUE OF THIS SPECIFIC VOTE
    */

    uint256 GetHash() const;
    uint256 GetSignatureHash() const;

    std::string ToString() const;

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        int nVersion = s.GetVersion();
        if (nVersion == 70208 && (s.GetType() & SER_NETWORK)) {
            // converting from/to old format
            CTxIn txin{};
            if (ser_action.ForRead()) {
                READWRITE(txin);
                masternodeOutpoint = txin.prevout;
            } else {
                txin = CTxIn(masternodeOutpoint);
                READWRITE(txin);
            }
        } else {
            // using new format directly
            READWRITE(masternodeOutpoint);
        }
        READWRITE(nParentHash);
        READWRITE(nVoteOutcome);
        READWRITE(nVoteSignal);
        READWRITE(nTime);
        if (!(s.GetType() & SER_GETHASH)) {
            READWRITE(vchSig);
        }
        if (ser_action.ForRead())
            UpdateHash();
    }

};

#endif
