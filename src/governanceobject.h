// Copyright (c) 2014-2017 The Dash Core developers
// Copyright (c) 2017-2018 The Syscoin Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SYSCOIN_GOVERNANCEOBJECT_H
#define SYSCOIN_GOVERNANCEOBJECT_H

//#define ENABLE_SYS_DEBUG

#include "cachemultimap.h"
#include "governanceexceptions.h"
#include "governancevote.h"
#include "governancevotedb.h"
#include "key.h"
#include "net.h"
#include "sync.h"
#include <util/strencodings.h>

#include <univalue.h>
#include "version.h"
class CGovernanceManager;
class CGovernanceTriggerManager;
class CGovernanceObject;
class CGovernanceVote;

static const int MAX_GOVERNANCE_OBJECT_DATA_SIZE = 16 * 1024;
static const int MIN_GOVERNANCE_PEER_PROTO_VERSION = MIN_PEER_PROTO_VERSION;
static const int GOVERNANCE_FILTER_PROTO_VERSION = MIN_PEER_PROTO_VERSION;

static const double GOVERNANCE_FILTER_FP_RATE = 0.001;

static const int GOVERNANCE_OBJECT_UNKNOWN = 0;
static const int GOVERNANCE_OBJECT_PROPOSAL = 1;
static const int GOVERNANCE_OBJECT_TRIGGER = 2;
static const int GOVERNANCE_OBJECT_WATCHDOG = 3;

static const CAmount GOVERNANCE_PROPOSAL_FEE_TX = (250.0*COIN);

static const int64_t GOVERNANCE_FEE_CONFIRMATIONS = 6;
static const int64_t GOVERNANCE_MIN_RELAY_FEE_CONFIRMATIONS = 1;
static const int64_t GOVERNANCE_UPDATE_MIN = 60*60;
static const int64_t GOVERNANCE_DELETION_DELAY = 10*60;
static const int64_t GOVERNANCE_ORPHAN_EXPIRATION_TIME = 10*60;

// FOR SEEN MAP ARRAYS - GOVERNANCE OBJECTS AND VOTES
static const int SEEN_OBJECT_IS_VALID = 0;
static const int SEEN_OBJECT_ERROR_INVALID = 1;
static const int SEEN_OBJECT_ERROR_IMMATURE = 2;
static const int SEEN_OBJECT_EXECUTED = 3; //used for triggers
static const int SEEN_OBJECT_UNKNOWN = 4; // the default

typedef std::pair<CGovernanceVote, int64_t> vote_time_pair_t;

inline bool operator<(const vote_time_pair_t& p1, const vote_time_pair_t& p2)
{
    return (p1.first < p2.first);
}

struct vote_instance_t {

    vote_outcome_enum_t eOutcome;
    int64_t nTime;
    int64_t nCreationTime;

    vote_instance_t(vote_outcome_enum_t eOutcomeIn = VOTE_OUTCOME_NONE, int64_t nTimeIn = 0, int64_t nCreationTimeIn = 0)
        : eOutcome(eOutcomeIn),
          nTime(nTimeIn),
          nCreationTime(nCreationTimeIn)
    {}

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action)
    {
        int nOutcome = int(eOutcome);
        READWRITE(nOutcome);
        READWRITE(nTime);
        READWRITE(nCreationTime);
        if(ser_action.ForRead()) {
            eOutcome = vote_outcome_enum_t(nOutcome);
        }
    }
};

typedef std::map<int,vote_instance_t> vote_instance_m_t;

typedef vote_instance_m_t::iterator vote_instance_m_it;

typedef vote_instance_m_t::const_iterator vote_instance_m_cit;

struct vote_rec_t {
    vote_instance_m_t mapInstances;

    ADD_SERIALIZE_METHODS;

     template <typename Stream, typename Operation>
     inline void SerializationOp(Stream& s, Operation ser_action)
     {
         READWRITE(mapInstances);
     }
};

/**
* Governance Object
*
*/

class CGovernanceObject
{
    friend class CGovernanceManager;
    friend class CGovernanceTriggerManager;
    friend class CSuperblock;

public: // Types
    typedef std::map<COutPoint, vote_rec_t> vote_m_t;

    typedef vote_m_t::iterator vote_m_it;

    typedef vote_m_t::const_iterator vote_m_cit;

    typedef CacheMultiMap<COutPoint, vote_time_pair_t> vote_cmm_t;

private:
    /// critical section to protect the inner data structures
    mutable CCriticalSection cs;

    /// Object typecode
    int nObjectType;

    /// parent object, 0 is root
    uint256 nHashParent;

    /// object revision in the system
    int nRevision;

    /// time this object was created
    int64_t nTime;

    /// time this object was marked for deletion
    int64_t nDeletionTime;
    
    /// transaction collateral for the fee
    uint256 nCollateralHash;
        
    /// Data field - can be used for anything
    std::vector<unsigned char> vchData;
    /// Masternode info for signed objects
    COutPoint masternodeOutpoint;
    std::vector<unsigned char> vchSig;

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
     * (e.g. business review board / technecial review board /etc)
     */
    bool fCachedEndorsed;

    /// object was updated and cached values should be updated soon
    bool fDirtyCache;

    /// Object is no longer of interest
    bool fExpired;

    /// Failed to parse object data
    bool fUnparsable;

    vote_m_t mapCurrentMNVotes;

    /// Limited map of votes orphaned by MN
    vote_cmm_t cmmapOrphanVotes;

    CGovernanceObjectVoteFile fileVotes;

public:
    CGovernanceObject();

    CGovernanceObject(const uint256& nHashParentIn, int nRevisionIn, int64_t nHeight, const uint256& nCollateralHashIn, const std::string& strDataHexIn);

    CGovernanceObject(const CGovernanceObject& other);

    // Public Getter methods

     int64_t GetCreationTime() const {
        return nTime;
    }

    int64_t GetDeletionTime() const {
        return nDeletionTime;
    }

    int GetObjectType() const {
        return nObjectType;
    }
    const uint256 GetCollateralHash() const {
        return nCollateralHash;
    }     

    const COutPoint& GetMasternodeOutpoint() const {
        return masternodeOutpoint;
    }

    bool IsSetCachedFunding() const {
        return fCachedFunding;
    }

    bool IsSetCachedValid() const {
        return fCachedValid;
    }

    bool IsSetCachedDelete() const {
        return fCachedDelete;
    }

    bool IsSetCachedEndorsed() const {
        return fCachedEndorsed;
    }

    bool IsSetDirtyCache() const {
        return fDirtyCache;
    }

    bool IsSetExpired() const {
        return fExpired;
    }

    void InvalidateVoteCache() {
        fDirtyCache = true;
    }

    const CGovernanceObjectVoteFile& GetVoteFile() const {
        return fileVotes;
    }

    // Signature related functions

    void SetMasternodeOutpoint(const COutPoint& outpoint);
    bool Sign(const CKey& keyMasternode, const CPubKey& pubKeyMasternode);
    bool CheckSignature(const CPubKey& pubKeyMasternode) const;

    std::string GetSignatureMessage() const;
    uint256 GetSignatureHash() const;

    // CORE OBJECT FUNCTIONS

    bool IsValidLocally(std::string& strError, bool fCheckCollateral) const;

    bool IsValidLocally(std::string& strError, bool& fMissingMasternode, bool& fMissingConfirmations, bool fCheckCollateral) const;

    /// Check the collateral transaction for the budget proposal/finalized budget
    bool IsCollateralValid(std::string& strError, bool& fMissingConfirmations) const;

    void UpdateLocalValidity();

    void UpdateSentinelVariables();

    CAmount GetMinCollateralFee() const;

    UniValue GetJSONObject();

    void Relay(CConnman& connman);

    uint256 GetHash() const;

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

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action)
    {
        // SERIALIZE DATA FOR SAVING/LOADING OR NETWORK FUNCTIONS
        READWRITE(nHashParent);
        READWRITE(nRevision);
        READWRITE(nTime);
        READWRITE(nCollateralHash);
        // using new format directly
        READWRITE(vchData);
        
        READWRITE(nObjectType);
        
        // using new format directly
        READWRITE(masternodeOutpoint);
        
        if (!(s.GetType() & SER_GETHASH)) {
            READWRITE(vchSig);
        }
        if(s.GetType() & SER_DISK) {
            // Only include these for the disk file format
            LogPrint(BCLog::GOBJECT, "CGovernanceObject::SerializationOp Reading/writing votes from/to disk\n");
            READWRITE(nDeletionTime);
            READWRITE(fExpired);
            READWRITE(mapCurrentMNVotes);
            READWRITE(fileVotes);
            LogPrint(BCLog::GOBJECT, "CGovernanceObject::SerializationOp hash = %s, vote count = %d\n", GetHash().ToString(), fileVotes.GetVoteCount());
        }

        // AFTER DESERIALIZATION OCCURS, CACHED VARIABLES MUST BE CALCULATED MANUALLY
    }

private:
    // FUNCTIONS FOR DEALING WITH DATA STRING
    void LoadData();
    void GetData(UniValue& objResult);

    bool ProcessVote(CNode* pfrom,
                     const CGovernanceVote& vote,
                     CGovernanceException& exception,
                     CConnman& connman);

    /// Called when MN's which have voted on this object have been removed
    void ClearMasternodeVotes();

    void CheckOrphanVotes(CConnman& connman);

};


#endif // SYSCOIN_GOVERNANCEOBJECT_H
