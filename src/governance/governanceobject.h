// Copyright (c) 2014-2023 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SYSCOIN_GOVERNANCE_GOVERNANCEOBJECT_H
#define SYSCOIN_GOVERNANCE_GOVERNANCEOBJECT_H

#include <governance/governancecommon.h>
#include <governance/governanceexceptions.h>
#include <governance/governancevote.h>
#include <governance/governancevotedb.h>
#include <sync.h>

#include <univalue.h>
#include <kernel/cs_main.h>

class CActiveMasternodeManager;
class CBLSPublicKey;
class CDeterministicMNList;
class CGovernanceManager;
class CGovernanceObject;
class CGovernanceVote;
class CMasternodeMetaMan;
class CMasternodeSync;
class CNode;
class PeerManager;
class ChainstateManager;


static constexpr double GOVERNANCE_FILTER_FP_RATE = 0.001;


static constexpr CAmount GOVERNANCE_PROPOSAL_FEE_TX = (150.0 * COIN);

static constexpr int64_t GOVERNANCE_FEE_CONFIRMATIONS = 6;
static constexpr int64_t GOVERNANCE_MIN_RELAY_FEE_CONFIRMATIONS = 1;
static constexpr int64_t GOVERNANCE_UPDATE_MIN = 60 * 60;
static constexpr int64_t GOVERNANCE_DELETION_DELAY = 10 * 60;
static constexpr int64_t GOVERNANCE_ORPHAN_EXPIRATION_TIME = 10 * 60;
static constexpr int64_t GOVERNANCE_FUDGE_WINDOW = 60 * 60 * 2;

// FOR SEEN MAP ARRAYS - GOVERNANCE OBJECTS AND VOTES
enum class SeenObjectStatus {
    Valid = 0,
    ErrorInvalid,
    Executed,
    Unknown
};

using vote_time_pair_t = std::pair<CGovernanceVote, int64_t>;

inline bool operator<(const vote_time_pair_t& p1, const vote_time_pair_t& p2)
{
    return (p1.first < p2.first);
}

struct vote_instance_t {
    vote_outcome_enum_t eOutcome;
    int64_t nTime;
    int64_t nCreationTime;

    explicit vote_instance_t(vote_outcome_enum_t eOutcomeIn = VOTE_OUTCOME_NONE, int64_t nTimeIn = 0, int64_t nCreationTimeIn = 0) :
        eOutcome(eOutcomeIn),
        nTime(nTimeIn),
        nCreationTime(nCreationTimeIn)
    {
    }

    SERIALIZE_METHODS(vote_instance_t, obj)
    {
        int nOutcome;
        SER_WRITE(obj, nOutcome = int(obj.eOutcome));
        READWRITE(nOutcome, obj.nTime, obj.nCreationTime);
        SER_READ(obj, obj.eOutcome = vote_outcome_enum_t(nOutcome));
    }
};

using vote_instance_m_t = std::map<int, vote_instance_t>;

struct vote_rec_t {
    vote_instance_m_t mapInstances;

    SERIALIZE_METHODS(vote_rec_t, obj)
    {
        READWRITE(obj.mapInstances);
    }
};

/**
* Governance Object
*
*/

class CGovernanceObject
{
public: // Types
    using vote_m_t = std::map<COutPoint, vote_rec_t>;

private:
    /// critical section to protect the inner data structures
    mutable RecursiveMutex cs;

    Governance::Object m_obj;

    /// time this object was marked for deletion
    int64_t nDeletionTime;


    /// is valid by blockchain
    bool fCachedLocalValidity;
    std::string strLocalValidityError;

    // VARIOUS FLAGS FOR OBJECT / SET VIA MASTERNODE VOTING

    /// true == minimum network support has been reached for this object to be funded (doesn't mean it will for sure though)
    bool fCachedFunding;

    /// true == minimum network has been reached flagging this object as a valid and understood governance object (e.g, the serialized data is correct format, etc)
    bool fCachedValid;

    /// true == minimum network support has been reached saying this object should be deleted from the system entirely
    bool fCachedDelete;

    /** true == minimum network support has been reached flagging this object as endorsed by an elected representative body
     * (e.g. business review board / technical review board /etc)
     */
    bool fCachedEndorsed;

    /// object was updated and cached values should be updated soon
    bool fDirtyCache;

    /// Object is no longer of interest
    bool fExpired;

    /// Failed to parse object data
    bool fUnparsable;

    vote_m_t mapCurrentMNVotes;

    CGovernanceObjectVoteFile fileVotes;

public:
    CGovernanceObject();

    CGovernanceObject(const uint256& nHashParentIn, int nRevisionIn, int64_t nTime, const uint256& nCollateralHashIn, const std::string& strDataHexIn);

    CGovernanceObject(const CGovernanceObject& other);

    // Public Getter methods

    const Governance::Object& Object() const
    {
        return m_obj;
    }

    int64_t GetCreationTime() const
    {
        return m_obj.time;
    }

    int64_t GetDeletionTime() const
    {
        return nDeletionTime;
    }

    int GetObjectType() const
    {
        return m_obj.type.GetValue();
    }

    const uint256& GetCollateralHash() const
    {
        return m_obj.collateralHash;
    }

    const COutPoint& GetMasternodeOutpoint() const
    {
        return m_obj.masternodeOutpoint;
    }

    bool IsSetCachedFunding() const
    {
        return fCachedFunding;
    }

    bool IsSetCachedValid() const
    {
        return fCachedValid;
    }

    bool IsSetCachedDelete() const
    {
        return fCachedDelete;
    }

    bool IsSetCachedEndorsed() const
    {
        return fCachedEndorsed;
    }

    bool IsSetDirtyCache() const
    {
        return fDirtyCache;
    }

    bool IsSetExpired() const
    {
        return fExpired;
    }

    void SetExpired()
    {
        fExpired = true;
    }

    const CGovernanceObjectVoteFile& GetVoteFile() const
    {
        return fileVotes;
    }

    // Signature related functions

    void SetMasternodeOutpoint(const COutPoint& outpoint);
    bool Sign();
    bool CheckSignature(const CBLSPublicKey& pubKey) const;

    uint256 GetSignatureHash() const;

    // CORE OBJECT FUNCTIONS

    bool IsValidLocally(ChainstateManager &chainman,const CDeterministicMNList& tip_mn_list, std::string& strError, bool fCheckCollateral) const;

    bool IsValidLocally(ChainstateManager &chainman,const CDeterministicMNList& tip_mn_list, std::string& strError, bool& fMissingConfirmations, bool fCheckCollateral) const;

    /// Check the collateral transaction for the budget proposal/finalized budget
    bool IsCollateralValid(ChainstateManager &chainman, std::string& strError, bool& fMissingConfirmations) const;

    void UpdateLocalValidity(ChainstateManager &chainman, const CDeterministicMNList& tip_mn_list);

    void UpdateSentinelVariables(const CDeterministicMNList& tip_mn_list);

    void PrepareDeletion(int64_t nDeletionTime_)
    {
        fCachedDelete = true;
        if (nDeletionTime == 0) {
            nDeletionTime = nDeletionTime_;
        }
    }

    CAmount GetMinCollateralFee() const;

    UniValue GetJSONObject() const;

    void Relay(PeerManager& peerman) const;

    uint256 GetHash() const;
    uint256 GetDataHash() const;

    // GET VOTE COUNT FOR SIGNAL

    int CountMatchingVotes(vote_signal_enum_t eVoteSignalIn, vote_outcome_enum_t eVoteOutcomeIn) const;

    int GetAbsoluteYesCount(vote_signal_enum_t eVoteSignalIn) const;
    int GetAbsoluteNoCount(vote_signal_enum_t eVoteSignalIn) const;
    int GetYesCount(vote_signal_enum_t eVoteSignalIn) const;
    int GetNoCount(vote_signal_enum_t eVoteSignalIn) const;
    int GetAbstainCount(vote_signal_enum_t eVoteSignalIn) const;

    bool GetCurrentMNVotes(const COutPoint& mnCollateralOutpoint, vote_rec_t& voteRecord) const;

    // FUNCTIONS FOR DEALING WITH DATA STRING

    std::string GetDataAsHexString() const;
    std::string GetDataAsPlainString() const;

    // SERIALIZER

    SERIALIZE_METHODS(CGovernanceObject, obj)
    {
        // SERIALIZE DATA FOR SAVING/LOADING OR NETWORK FUNCTIONS
        READWRITE(obj.m_obj);
        if (s.GetType() & SER_DISK) {
            // Only include these for the disk file format
            READWRITE(obj.nDeletionTime, obj.fExpired, obj.mapCurrentMNVotes, obj.fileVotes);
        }

        // AFTER DESERIALIZATION OCCURS, CACHED VARIABLES MUST BE CALCULATED MANUALLY
    }

    UniValue ToJson() const;

    // FUNCTIONS FOR DEALING WITH DATA STRING
    void LoadData();
    void GetData(UniValue& objResult) const;

    bool ProcessVote(const CDeterministicMNList& tip_mn_list,
                     const CGovernanceVote& vote, CGovernanceException& exception);

    /// Called when MN's which have voted on this object have been removed
    void ClearMasternodeVotes(const CDeterministicMNList& tip_mn_list);

    // Revalidate all votes from this MN and delete them if validation fails.
    // This is the case for DIP3 MNs that changed voting or operator keys and
    // also for MNs that were removed from the list completely.
    // Returns deleted vote hashes.
    std::set<uint256> RemoveInvalidVotes(const CDeterministicMNList& tip_mn_list, const COutPoint& mnOutpoint);
};


#endif // SYSCOIN_GOVERNANCE_GOVERNANCEOBJECT_H
