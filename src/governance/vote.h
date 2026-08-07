// Copyright (c) 2014-2025 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_GOVERNANCE_VOTE_H
#define BITCOIN_GOVERNANCE_VOTE_H

#include <hash.h>
#include <primitives/transaction.h>
#include <serialize.h>
#include <uint256.h>
#include <util/string.h>

#include <optional>

class CBLSPublicKey;
class CDeterministicMNList;
class CGovernanceVote;
class CKeyID;

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

public:
    // Wire-valid signature encodings: compact ECDSA voting key (FUNDING)
    // or BLS operator key (other signals). Kept in sync via static_assert
    // against CPubKey::COMPACT_SIGNATURE_SIZE / CBLSSignature::SerSize in vote.cpp.
    static constexpr size_t COMPACT_SIG_SIZE = 65;
    static constexpr size_t BLS_SIG_SIZE = 96;

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

    /**
     * Result of the last CheckSignature call, so that re-verifying an unchanged
     * vote costs a hash instead of an ECDSA recovery or a BLS pairing.
     *
     * The fingerprint covers the key, the signature hash and the signature
     * bytes, so a hit means this exact verification already ran. Answering from
     * a bare "already checked" flag would be wrong two ways: the same vote gets
     * checked against different keys (voting vs operator, and operator keys
     * rotate), and a vote can be modified after verification, which would
     * otherwise keep its stale verdict.
     */
    class SignatureMemo
    {
        std::optional<uint256> m_fingerprint;
        bool m_valid{false};

    public:
        std::optional<bool> Lookup(const uint256& fingerprint) const
        {
            if (m_fingerprint != fingerprint) return std::nullopt;
            return m_valid;
        }

        void Store(const uint256& fingerprint, bool valid)
        {
            m_fingerprint = fingerprint;
            m_valid = valid;
        }
    };

    /**
     * Memory only, and not internally synchronised. Shared votes live in
     * CGovernanceObjectVoteFile, reachable only through
     * CGovernanceObject::GetVoteFile() under the object's cs; every other vote
     * has a single owner.
     */
    mutable SignatureMemo m_sig_memo;

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

    bool CheckSignature(const CKeyID& keyID) const;
    bool CheckSignature(const CBLSPublicKey& pubKey) const;
    bool IsValid(const CDeterministicMNList& tip_mn_list, bool useVotingKey) const;
    /**
     * Validation for a vote whose parent object is unknown, so the exact key
     * requirement cannot be derived yet. Only a funding vote can ever be
     * voting-key-signed (PROPOSAL + VOTE_SIGNAL_FUNDING -- see onlyVotingKeyAllowed
     * in CGovernanceObject::ProcessVote); every other signal requires the operator
     * key for every object type, and a funding vote on a non-proposal will be
     * re-checked against the operator-key requirement when its parent arrives.
     */
    bool IsValidForUnknownParent(const CDeterministicMNList& tip_mn_list) const;

    /** The memoised verdict for this key, or nullopt if the next CheckSignature
     *  with it would have to run the cryptography. */
    std::optional<bool> GetMemoisedVerdict(const CKeyID& keyID) const;
    std::optional<bool> GetMemoisedVerdict(const CBLSPublicKey& pubKey) const;
    std::string GetSignatureString() const
    {
        return masternodeOutpoint.ToStringShort() + "|" + nParentHash.ToString() + "|" +
                                 ::ToString(nVoteSignal) + "|" +
                                 ::ToString(nVoteOutcome) + "|" +
                                 ::ToString(nTime);
    }

    const COutPoint& GetMasternodeOutpoint() const { return masternodeOutpoint; }

    /**
    *   GetHash()
    *
    *   GET UNIQUE HASH WITH DETERMINISTIC VALUE OF THIS SPECIFIC VOTE
    */

    uint256 GetHash() const;
    uint256 GetSignatureHash() const
    {
        return SerializeHash(*this);
    }

    std::string ToString(const CDeterministicMNList& tip_mn_list) const;

    SERIALIZE_METHODS(CGovernanceVote, obj)
    {
        READWRITE(obj.masternodeOutpoint, obj.nParentHash, obj.nVoteOutcome, obj.nVoteSignal, obj.nTime);
        if (!(s.GetType() & SER_GETHASH)) {
            // Network reads: cap the signature vector before allocation and require
            // one of the two legitimate encodings. Other paths (disk, hash, write)
            // keep the unbounded default.
            if (ser_action.ForRead() && (s.GetType() & SER_NETWORK)) {
                READWRITE(LIMITED_VECTOR(obj.vchSig, BLS_SIG_SIZE));
                if (obj.vchSig.size() != COMPACT_SIG_SIZE && obj.vchSig.size() != BLS_SIG_SIZE) {
                    throw std::ios_base::failure("bad governance vote signature size");
                }
            } else {
                READWRITE(obj.vchSig);
            }
        }
        SER_READ(obj, obj.UpdateHash());
    }
};

/**
 * Sign a governance vote using wallet signing methods
 * Handles different signing approaches for different networks
 */

#endif // BITCOIN_GOVERNANCE_VOTE_H
