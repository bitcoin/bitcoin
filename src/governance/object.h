// Copyright (c) 2014-2025 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_GOVERNANCE_OBJECT_H
#define BITCOIN_GOVERNANCE_OBJECT_H

#include <governance/common.h>
#include <governance/exceptions.h>
#include <governance/vote.h>
#include <governance/votedb.h>
#include <sync.h>

#include <span.h>

#include <univalue.h>

class CBLSPublicKey;
class CDeterministicMNList;
class CGovernanceManager;
class CGovernanceObject;
class CGovernanceVote;
class ChainstateManager;
class CMasternodeMetaMan;
struct RPCResult;

extern RecursiveMutex cs_main; // NOLINT(readability-redundant-declaration)

static constexpr double GOVERNANCE_FILTER_FP_RATE = 0.001;
static constexpr CAmount GOVERNANCE_PROPOSAL_FEE_TX = (1 * COIN);
static constexpr int64_t GOVERNANCE_FEE_CONFIRMATIONS = 6;
static constexpr int64_t GOVERNANCE_MIN_RELAY_FEE_CONFIRMATIONS = 1;
static constexpr int64_t GOVERNANCE_UPDATE_MIN = 60 * 60;

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

public:
    /// critical section to protect the inner data structures
    mutable Mutex cs;

private:
    Governance::Object m_obj;

    /// time this object was marked for deletion
    int64_t nDeletionTime GUARDED_BY(cs){0};

    /// is valid by blockchain
    bool fCachedLocalValidity{false};
    std::string strLocalValidityError;

    // VARIOUS FLAGS FOR OBJECT / SET VIA MASTERNODE VOTING

    /// true == minimum network support has been reached for this object to be funded (doesn't mean it will for sure though)
    bool fCachedFunding{false};

    /// true == minimum network has been reached flagging this object as a valid and understood governance object (e.g, the serialized data is correct format, etc)
    bool fCachedValid{true};

    /// true == minimum network support has been reached saying this object should be deleted from the system entirely
    bool fCachedDelete{false};

    /** true == minimum network support has been reached flagging this object as endorsed by an elected representative body
     * (e.g. business review board / technical review board /etc)
     */
    bool fCachedEndorsed{false};

    /// object was updated and cached values should be updated soon
    bool fDirtyCache{true};

    /// Object is no longer of interest
    bool fExpired GUARDED_BY(cs){false};

    /// Failed to parse object data
    bool fUnparsable{false};

    vote_m_t mapCurrentMNVotes GUARDED_BY(cs);

    CGovernanceObjectVoteFile fileVotes GUARDED_BY(cs);

public:
    CGovernanceObject();
    CGovernanceObject(const uint256& nHashParentIn, int nRevisionIn, int64_t nTime, const uint256& nCollateralHashIn, const std::string& strDataHexIn);
    CGovernanceObject(const CGovernanceObject& other);
    template <typename Stream>
    CGovernanceObject(deserialize_type, Stream& s) { s >> *this; }

    // Getters
    bool IsSetCachedFunding() const { return fCachedFunding; }
    bool IsSetCachedValid() const { return fCachedValid; }
    bool IsSetCachedDelete() const { return fCachedDelete; }
    bool IsSetCachedEndorsed() const { return fCachedEndorsed; }
    bool IsSetDirtyCache() const { return fDirtyCache; }
    bool IsSetExpired() const EXCLUSIVE_LOCKS_REQUIRED(!cs)
    {
        return WITH_LOCK(cs, return fExpired);
    }
    GovernanceObject GetObjectType() const { return m_obj.type; }
    int64_t GetCreationTime() const { return m_obj.time; }
    int64_t GetDeletionTime() const EXCLUSIVE_LOCKS_REQUIRED(!cs)
    {
        return WITH_LOCK(cs, return nDeletionTime);
    }

    const CGovernanceObjectVoteFile& GetVoteFile() const EXCLUSIVE_LOCKS_REQUIRED(cs)
    {
        AssertLockHeld(cs);
        return fileVotes;
    }
    const COutPoint& GetMasternodeOutpoint() const { return m_obj.masternodeOutpoint; }
    const Governance::Object& Object() const { return m_obj; }
    const uint256& GetCollateralHash() const { return m_obj.collateralHash; }

    // Setters
    void SetExpired() EXCLUSIVE_LOCKS_REQUIRED(!cs)
    {
        WITH_LOCK(cs, fExpired = true);
    }
    void SetMasternodeOutpoint(const COutPoint& outpoint);
    void SetSignature(Span<const uint8_t> sig);

    // Signature related functions
    bool CheckSignature(const CBLSPublicKey& pubKey) const;
    uint256 GetSignatureHash() const;

    // CORE OBJECT FUNCTIONS

    bool IsValidLocally(const CDeterministicMNList& tip_mn_list, const ChainstateManager& chainman, std::string& strError, bool fCheckCollateral) const
        EXCLUSIVE_LOCKS_REQUIRED(::cs_main);

    bool IsValidLocally(const CDeterministicMNList& tip_mn_list, const ChainstateManager& chainman, std::string& strError, bool& fMissingConfirmations, bool fCheckCollateral) const
        EXCLUSIVE_LOCKS_REQUIRED(::cs_main);

    /// Check the collateral transaction for the budget proposal/finalized budget
    bool IsCollateralValid(const ChainstateManager& chainman, std::string& strError, bool& fMissingConfirmations) const
        EXCLUSIVE_LOCKS_REQUIRED(::cs_main);

    void UpdateLocalValidity(const CDeterministicMNList& tip_mn_list, const ChainstateManager& chainman)
        EXCLUSIVE_LOCKS_REQUIRED(::cs_main);

    void UpdateSentinelVariables(const CDeterministicMNList& tip_mn_list)
        EXCLUSIVE_LOCKS_REQUIRED(!cs);

    void PrepareDeletion(int64_t nDeletionTime_) EXCLUSIVE_LOCKS_REQUIRED(!cs)
    {
        fCachedDelete = true;
        LOCK(cs);
        if (nDeletionTime == 0) {
            nDeletionTime = nDeletionTime_;
        }
    }

    CAmount GetMinCollateralFee() const;

    UniValue GetJSONObject() const;

    uint256 GetHash() const;
    uint256 GetDataHash() const;

    // GET VOTE COUNT FOR SIGNAL

    int CountMatchingVotes(const CDeterministicMNList& tip_mn_list, vote_signal_enum_t eVoteSignalIn, vote_outcome_enum_t eVoteOutcomeIn) const
        EXCLUSIVE_LOCKS_REQUIRED(!cs);

    int GetAbsoluteYesCount(const CDeterministicMNList& tip_mn_list, vote_signal_enum_t eVoteSignalIn) const
        EXCLUSIVE_LOCKS_REQUIRED(!cs);
    int GetAbsoluteNoCount(const CDeterministicMNList& tip_mn_list, vote_signal_enum_t eVoteSignalIn) const
        EXCLUSIVE_LOCKS_REQUIRED(!cs);
    int GetYesCount(const CDeterministicMNList& tip_mn_list, vote_signal_enum_t eVoteSignalIn) const
        EXCLUSIVE_LOCKS_REQUIRED(!cs);
    int GetNoCount(const CDeterministicMNList& tip_mn_list, vote_signal_enum_t eVoteSignalIn) const
        EXCLUSIVE_LOCKS_REQUIRED(!cs);
    int GetAbstainCount(const CDeterministicMNList& tip_mn_list, vote_signal_enum_t eVoteSignalIn) const
        EXCLUSIVE_LOCKS_REQUIRED(!cs);

    bool GetCurrentMNVotes(const COutPoint& mnCollateralOutpoint, vote_rec_t& voteRecord) const
        EXCLUSIVE_LOCKS_REQUIRED(!cs);

    // FUNCTIONS FOR DEALING WITH DATA STRING

    std::string GetDataAsHexString() const;
    std::string GetDataAsPlainString() const;

    // SERIALIZER

    template<typename Stream>
    void Serialize(Stream& s) const EXCLUSIVE_LOCKS_REQUIRED(!cs)
    {
        // SERIALIZE DATA FOR SAVING/LOADING OR NETWORK FUNCTIONS
        s << m_obj;
        if (s.GetType() & SER_DISK) {
            // Only include these for the disk file format
            LOCK(cs);
            s << nDeletionTime << fExpired << mapCurrentMNVotes << fileVotes;
        }
    }

    template<typename Stream>
    void Unserialize(Stream& s) EXCLUSIVE_LOCKS_REQUIRED(!cs)
    {
        s >> m_obj;
        if (s.GetType() & SER_DISK) {
            // Only include these for the disk file format
            LOCK(cs);
            s >> nDeletionTime >> fExpired >> mapCurrentMNVotes >> fileVotes;
        }
        // AFTER DESERIALIZATION OCCURS, CACHED VARIABLES MUST BE CALCULATED MANUALLY
    }

    // JSON emitters/help
    [[nodiscard]] static RPCResult GetInnerJsonHelp(const std::string& key, bool optional);
    [[nodiscard]] UniValue GetInnerJson() const;

    [[nodiscard]] static RPCResult GetStateJsonHelp(const std::string& key, bool optional, const std::string& local_valid_key);
    [[nodiscard]] UniValue GetStateJson(const ChainstateManager& chainman, const CDeterministicMNList& tip_mn_list, const std::string& local_valid_key) const
        EXCLUSIVE_LOCKS_REQUIRED(!cs);

    [[nodiscard]] static RPCResult GetVotesJsonHelp(const std::string& key, bool optional);
    [[nodiscard]] UniValue GetVotesJson(const CDeterministicMNList& tip_mn_list, vote_signal_enum_t signal) const
        EXCLUSIVE_LOCKS_REQUIRED(!cs);

    // FUNCTIONS FOR DEALING WITH DATA STRING
    void LoadData();
    void GetData(UniValue& objResult) const;

    bool ProcessVote(CMasternodeMetaMan& mn_metaman, CGovernanceManager& govman, const CDeterministicMNList& tip_mn_list,
                     const CGovernanceVote& vote, CGovernanceException& exception)
        EXCLUSIVE_LOCKS_REQUIRED(!cs);

    /// Called when MN's which have voted on this object have been removed
    void ClearMasternodeVotes(const CDeterministicMNList& tip_mn_list)
        EXCLUSIVE_LOCKS_REQUIRED(!cs);

    // Revalidate all votes from this MN and delete them if validation fails.
    // This is the case for DIP3 MNs that changed voting or operator keys and
    // also for MNs that were removed from the list completely.
    // Returns deleted vote hashes.
    std::set<uint256> RemoveInvalidVotes(const CDeterministicMNList& tip_mn_list, const COutPoint& mnOutpoint)
        EXCLUSIVE_LOCKS_REQUIRED(!cs);
};

#endif // BITCOIN_GOVERNANCE_OBJECT_H
