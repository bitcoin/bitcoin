// Copyright (c) 2014-2024 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_GOVERNANCE_VOTE_H
#define BITCOIN_GOVERNANCE_VOTE_H

#include <primitives/transaction.h>
#include <uint256.h>

class CActiveMasternodeManager;
class CBLSPublicKey;
class CDeterministicMNList;
class CGovernanceVote;
class CMasternodeSync;
class CKey;
class CKeyID;
class CWallet;
class PeerManager;

// INTENTION OF MASTERNODES REGARDING ITEM
enum vote_outcome_enum_t : int {
    VOTE_OUTCOME_NONE = 0,
    VOTE_OUTCOME_YES,
    VOTE_OUTCOME_NO,
    VOTE_OUTCOME_ABSTAIN,
    VOTE_OUTCOME_UNKNOWN
};
template<> struct is_serializable_enum<vote_outcome_enum_t> : std::true_type {};

// SIGNAL VARIOUS THINGS TO HAPPEN:
enum vote_signal_enum_t : int {
    VOTE_SIGNAL_NONE = 0,
    VOTE_SIGNAL_FUNDING,  //   -- fund this object for it's stated amount
    VOTE_SIGNAL_VALID,    //   -- this object checks out in sentinel engine
    VOTE_SIGNAL_DELETE,   //   -- this object should be deleted from memory entirely
    VOTE_SIGNAL_ENDORSED, //   -- officially endorsed by the network somehow (delegation)
    VOTE_SIGNAL_UNKNOWN
};
template<> struct is_serializable_enum<vote_signal_enum_t> : std::true_type {};

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
// CGovernanceVote - Allow a masternode to vote and broadcast throughout the network
//

class CGovernanceVote
{
    friend bool operator==(const CGovernanceVote& vote1, const CGovernanceVote& vote2);

    friend bool operator<(const CGovernanceVote& vote1, const CGovernanceVote& vote2);

private:
    COutPoint masternodeOutpoint;
    uint256 nParentHash;
    vote_outcome_enum_t nVoteOutcome{VOTE_OUTCOME_NONE};
    vote_signal_enum_t nVoteSignal{VOTE_SIGNAL_NONE};
    int64_t nTime{0};
    std::vector<unsigned char> vchSig;

    /** Memory only. */
    const uint256 hash{0};
    void UpdateHash() const;

public:
    CGovernanceVote() = default;
    CGovernanceVote(const COutPoint& outpointMasternodeIn, const uint256& nParentHashIn, vote_signal_enum_t eVoteSignalIn, vote_outcome_enum_t eVoteOutcomeIn);

    int64_t GetTimestamp() const { return nTime; }

    vote_signal_enum_t GetSignal() const { return nVoteSignal; }

    vote_outcome_enum_t GetOutcome() const { return nVoteOutcome; }

    const uint256& GetParentHash() const { return nParentHash; }

    void SetTime(int64_t nTimeIn)
    {
        nTime = nTimeIn;
        UpdateHash();
    }

    void SetSignature(const std::vector<unsigned char>& vchSigIn) { vchSig = vchSigIn; }

    bool Sign(const CKey& key, const CKeyID& keyID);
    bool CheckSignature(const CKeyID& keyID) const;
    bool Sign(const CActiveMasternodeManager& mn_activeman);
    bool CheckSignature(const CBLSPublicKey& pubKey) const;
    bool IsValid(const CDeterministicMNList& tip_mn_list, bool useVotingKey) const;
    std::string GetSignatureString() const;
    void Relay(PeerManager& peerman, const CMasternodeSync& mn_sync, const CDeterministicMNList& tip_mn_list) const;

    const COutPoint& GetMasternodeOutpoint() const { return masternodeOutpoint; }

    /**
    *   GetHash()
    *
    *   GET UNIQUE HASH WITH DETERMINISTIC VALUE OF THIS SPECIFIC VOTE
    */

    uint256 GetHash() const;
    uint256 GetSignatureHash() const;

    std::string ToString(const CDeterministicMNList& tip_mn_list) const;

    SERIALIZE_METHODS(CGovernanceVote, obj)
    {
        READWRITE(obj.masternodeOutpoint, obj.nParentHash, obj.nVoteOutcome, obj.nVoteSignal, obj.nTime);
        if (!(s.GetType() & SER_GETHASH)) {
            READWRITE(obj.vchSig);
        }
        SER_READ(obj, obj.UpdateHash());
    }
};

/**
 * Sign a governance vote using wallet signing methods
 * Handles different signing approaches for different networks
 */

#endif // BITCOIN_GOVERNANCE_VOTE_H
