// Copyright (c) 2014-2016 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef GOVERNANCE_H
#define GOVERNANCE_H

//#define ENABLE_DASH_DEBUG

#include "util.h"
#include "main.h"
#include "sync.h"
#include "net.h"
#include "key.h"
#include "util.h"
#include "base58.h"
#include "masternode.h"
#include "governance-exceptions.h"
#include "governance-vote.h"
#include "governance-votedb.h"
#include "masternodeman.h"
#include <boost/lexical_cast.hpp>
#include "init.h"
#include <univalue.h>
#include "utilstrencodings.h"
#include "cachemap.h"
#include "cachemultimap.h"

#include <stdio.h>
#include <string.h>

class CGovernanceManager;
class CGovernanceTriggerManager;
class CGovernanceObject;
class CGovernanceVote;

static const int MAX_GOVERNANCE_OBJECT_DATA_SIZE = 16 * 1024;
static const int MIN_GOVERNANCE_PEER_PROTO_VERSION = 70202;

static const int GOVERNANCE_OBJECT_UNKNOWN = 0;
static const int GOVERNANCE_OBJECT_PROPOSAL = 1;
static const int GOVERNANCE_OBJECT_TRIGGER = 2;
static const int GOVERNANCE_OBJECT_WATCHDOG = 3;

static const CAmount GOVERNANCE_PROPOSAL_FEE_TX = (0.33*COIN);

static const int64_t GOVERNANCE_FEE_CONFIRMATIONS = 6;
static const int64_t GOVERNANCE_UPDATE_MIN = 60*60;
static const int64_t GOVERNANCE_DELETION_DELAY = 10*60;


// FOR SEEN MAP ARRAYS - GOVERNANCE OBJECTS AND VOTES
static const int SEEN_OBJECT_IS_VALID = 0;
static const int SEEN_OBJECT_ERROR_INVALID = 1;
static const int SEEN_OBJECT_ERROR_IMMATURE = 2;
static const int SEEN_OBJECT_EXECUTED = 3; //used for triggers
static const int SEEN_OBJECT_UNKNOWN = 4; // the default

extern std::map<uint256, int64_t> mapAskedForGovernanceObject;
extern CGovernanceManager governance;

//
// Governance Manager : Contains all proposals for the budget
//
class CGovernanceManager
{
    friend class CGovernanceObject;

public: // Types

    typedef std::map<uint256, CGovernanceObject> object_m_t;

    typedef object_m_t::iterator object_m_it;

    typedef object_m_t::const_iterator object_m_cit;

    typedef CacheMap<uint256, CGovernanceObject*> object_ref_cache_t;

    typedef std::map<uint256, int> count_m_t;

    typedef count_m_t::iterator count_m_it;

    typedef count_m_t::const_iterator count_m_cit;

    typedef std::map<uint256, CGovernanceVote> vote_m_t;

    typedef vote_m_t::iterator vote_m_it;

    typedef vote_m_t::const_iterator vote_m_cit;

    typedef CacheMap<uint256, CGovernanceVote> vote_cache_t;

    typedef CacheMultiMap<uint256, CGovernanceVote> vote_mcache_t;

    typedef object_m_t::size_type size_type;

    typedef std::map<COutPoint, int> txout_m_t;

    typedef txout_m_t::iterator txout_m_it;

    typedef txout_m_t::const_iterator txout_m_cit;

    typedef std::set<uint256> hash_s_t;

    typedef hash_s_t::iterator hash_s_it;

    typedef hash_s_t::const_iterator hash_s_cit;

private:
    static const int MAX_CACHE_SIZE = 1000000;

    static const std::string SERIALIZATION_VERSION_STRING;

    // Keep track of current block index
    const CBlockIndex *pCurrentBlockIndex;

    int64_t nTimeLastDiff;
    int nCachedBlockHeight;

    // keep track of the scanning errors
    object_m_t mapObjects;

    count_m_t mapSeenGovernanceObjects;

    object_m_t mapMasternodeOrphanObjects;

    object_ref_cache_t mapVoteToObject;

    vote_cache_t mapInvalidVotes;

    vote_mcache_t mapOrphanVotes;

    txout_m_t mapLastMasternodeTrigger;

    hash_s_t setRequestedObjects;

    hash_s_t setRequestedVotes;

    bool fRateChecksEnabled;

public:
    // critical section to protect the inner data structures
    mutable CCriticalSection cs;

    CGovernanceManager();

    virtual ~CGovernanceManager() {}

    void ClearSeen()
    {
        LOCK(cs);
        mapSeenGovernanceObjects.clear();
    }

    int CountProposalInventoryItems()
    {
        // TODO What is this for ?
        return mapSeenGovernanceObjects.size();
        //return mapSeenGovernanceObjects.size() + mapSeenVotes.size();
    }

    /**
     * This is called by AlreadyHave in main.cpp as part of the inventory
     * retrieval process.  Returns true if we want to retrieve the object, otherwise
     * false. (Note logic is inverted in AlreadyHave).
     */
    bool ConfirmInventoryRequest(const CInv& inv);

    void Sync(CNode* node, uint256 nProp);

    void ProcessMessage(CNode* pfrom, std::string& strCommand, CDataStream& vRecv);

    void NewBlock();

    CGovernanceObject *FindGovernanceObject(const uint256& nHash);

    std::vector<CGovernanceVote> GetMatchingVotes(const uint256& nParentHash);
    std::vector<CGovernanceObject*> GetAllNewerThan(int64_t nMoreThanTime);

    bool IsBudgetPaymentBlock(int nBlockHeight);
    bool AddGovernanceObject (CGovernanceObject& govobj);

    std::string GetRequiredPaymentsString(int nBlockHeight);

    void UpdateCachesAndClean();

    void CheckAndRemove() {UpdateCachesAndClean();}

    void Clear()
    {
        LOCK(cs);

        LogPrint("gobject", "Governance object manager was cleared\n");
        mapObjects.clear();
        mapSeenGovernanceObjects.clear();
        mapVoteToObject.Clear();
        mapInvalidVotes.Clear();
        mapOrphanVotes.Clear();
        mapLastMasternodeTrigger.clear();
    }

    std::string ToString() const;

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        LOCK(cs);
        std::string strVersion;
        if(ser_action.ForRead()) {
            READWRITE(strVersion);
        }
        else {
            strVersion = SERIALIZATION_VERSION_STRING;
            READWRITE(strVersion);
        }
        READWRITE(mapSeenGovernanceObjects);
        READWRITE(mapInvalidVotes);
        READWRITE(mapOrphanVotes);
        READWRITE(mapObjects);
        READWRITE(mapLastMasternodeTrigger);
        if(ser_action.ForRead() && (strVersion != SERIALIZATION_VERSION_STRING)) {
            Clear();
            return;
        }
        if(ser_action.ForRead()) {
            RebuildIndexes();
            AddCachedTriggers();
        }
    }

    void UpdatedBlockTip(const CBlockIndex *pindex);
    int64_t GetLastDiffTime() { return nTimeLastDiff; }
    void UpdateLastDiffTime(int64_t nTimeIn) { nTimeLastDiff = nTimeIn; }

    int GetCachedBlockHeight() { return nCachedBlockHeight; }

    // Accessors for thread-safe access to maps
    bool HaveObjectForHash(uint256 nHash);

    bool HaveVoteForHash(uint256 nHash);

    bool SerializeObjectForHash(uint256 nHash, CDataStream& ss);

    bool SerializeVoteForHash(uint256 nHash, CDataStream& ss);

    void AddSeenGovernanceObject(uint256 nHash, int status);

    void AddSeenVote(uint256 nHash, int status);

    bool MasternodeRateCheck(const CTxIn& vin, int nObjectType);

    bool ProcessVote(const CGovernanceVote& vote, CGovernanceException& exception) {
        return ProcessVote(NULL, vote, exception);
    }

    void CheckMasternodeOrphanVotes();

    void CheckMasternodeOrphanObjects();

    bool AreRateChecksEnabled() const {
        LOCK(cs);
        return fRateChecksEnabled;
    }

private:
    void RequestGovernanceObject(CNode* pfrom, const uint256& nHash);

    void AddInvalidVote(const CGovernanceVote& vote)
    {
        mapInvalidVotes.Insert(vote.GetHash(), vote);
    }

    void AddOrphanVote(const CGovernanceVote& vote)
    {
        mapOrphanVotes.Insert(vote.GetHash(), vote);
    }

    bool ProcessVote(CNode* pfrom, const CGovernanceVote& vote, CGovernanceException& exception);

    /// Called to indicate a requested object has been received
    bool AcceptObjectMessage(const uint256& nHash);

    /// Called to indicate a requested vote has been received
    bool AcceptVoteMessage(const uint256& nHash);

    static bool AcceptMessage(const uint256& nHash, hash_s_t& setHash);

    void CheckOrphanVotes(CNode* pfrom, CGovernanceObject& govobj, CGovernanceException& exception);

    void RebuildIndexes();

    /// Returns MN index, handling the case of index rebuilds
    int GetMasternodeIndex(const CTxIn& masternodeVin);

    void RebuildVoteMaps();

    void AddCachedTriggers();

};

struct vote_instance_t {

    vote_outcome_enum_t eOutcome;
    int64_t nTime;

    vote_instance_t(vote_outcome_enum_t eOutcomeIn = VOTE_OUTCOME_NONE, int64_t nTimeIn = 0)
        : eOutcome(eOutcomeIn),
          nTime(nTimeIn)
    {}

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion)
    {
        int nOutcome = int(eOutcome);
        READWRITE(nOutcome);
        READWRITE(nTime);
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
     inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion)
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

public: // Types
    typedef std::map<int, vote_rec_t> vote_m_t;

    typedef vote_m_t::iterator vote_m_it;

    typedef vote_m_t::const_iterator vote_m_cit;

    typedef CacheMultiMap<CTxIn, CGovernanceVote> vote_mcache_t;

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

    /// fee-tx
    uint256 nCollateralHash;

    /// Data field - can be used for anything
    std::string strData;

    /// Masternode info for signed objects
    CTxIn vinMasternode;
    std::vector<unsigned char> vchSig;

    /// is valid by blockchain
    bool fCachedLocalValidity;
    std::string strLocalValidityError;

    // VARIOUS FLAGS FOR OBJECT / SET VIA MASTERNODE VOTING

    /// true == minimum network support has been reached for this object to be funded (doesn't mean it will for sure though)
    bool fCachedFunding;

    /// true == minimum network has been reached flagging this object as a valid and understood goverance object (e.g, the serialized data is correct format, etc)
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
    vote_mcache_t mapOrphanVotes;

    CGovernanceObjectVoteFile fileVotes;

public:
    CGovernanceObject();

    CGovernanceObject(uint256 nHashParentIn, int nRevisionIn, int64_t nTime, uint256 nCollateralHashIn, std::string strDataIn);

    CGovernanceObject(const CGovernanceObject& other);

    void swap(CGovernanceObject& first, CGovernanceObject& second); // nothrow

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

    const uint256& GetCollateralHash() const {
        return nCollateralHash;
    }

    const CTxIn& GetMasternodeVin() const {
        return vinMasternode;
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

    CGovernanceObjectVoteFile& GetVoteFile() {
        return fileVotes;
    }

    // Signature related functions

    void SetMasternodeInfo(const CTxIn& vin);
    bool Sign(CKey& keyMasternode, CPubKey& pubKeyMasternode);
    bool CheckSignature(CPubKey& pubKeyMasternode);

    // CORE OBJECT FUNCTIONS

    bool IsValidLocally(const CBlockIndex* pindex, std::string& strError, bool fCheckCollateral);

    bool IsValidLocally(const CBlockIndex* pindex, std::string& strError, bool& fMissingMasternode, bool fCheckCollateral);

    /// Check the collateral transaction for the budget proposal/finalized budget
    bool IsCollateralValid(std::string& strError);

    void UpdateLocalValidity(const CBlockIndex *pCurrentBlockIndex);

    void UpdateSentinelVariables(const CBlockIndex *pCurrentBlockIndex);

    int GetObjectSubtype();

    CAmount GetMinCollateralFee();

    UniValue GetJSONObject();

    void Relay();

    uint256 GetHash();

    // GET VOTE COUNT FOR SIGNAL

    int CountMatchingVotes(vote_signal_enum_t eVoteSignalIn, vote_outcome_enum_t eVoteOutcomeIn) const;

    int GetAbsoluteYesCount(vote_signal_enum_t eVoteSignalIn) const;
    int GetAbsoluteNoCount(vote_signal_enum_t eVoteSignalIn) const;
    int GetYesCount(vote_signal_enum_t eVoteSignalIn) const;
    int GetNoCount(vote_signal_enum_t eVoteSignalIn) const;
    int GetAbstainCount(vote_signal_enum_t eVoteSignalIn) const;

    // FUNCTIONS FOR DEALING WITH DATA STRING

    std::string GetDataAsHex();
    std::string GetDataAsString();

    // SERIALIZER

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion)
    {
        // SERIALIZE DATA FOR SAVING/LOADING OR NETWORK FUNCTIONS

        READWRITE(nHashParent);
        READWRITE(nRevision);
        READWRITE(nTime);
        READWRITE(nCollateralHash);
        READWRITE(LIMITED_STRING(strData, MAX_GOVERNANCE_OBJECT_DATA_SIZE));
        READWRITE(nObjectType);
        READWRITE(vinMasternode);
        READWRITE(vchSig);
        if(nType & SER_DISK) {
            // Only include these for the disk file format
            LogPrint("gobject", "CGovernanceObject::SerializationOp Reading/writing votes from/to disk\n");
            READWRITE(mapCurrentMNVotes);
            READWRITE(fileVotes);
            LogPrint("gobject", "CGovernanceObject::SerializationOp hash = %s, vote count = %d\n", GetHash().ToString(), fileVotes.GetVoteCount());
        }

        // AFTER DESERIALIZATION OCCURS, CACHED VARIABLES MUST BE CALCULATED MANUALLY
    }

private:
    // FUNCTIONS FOR DEALING WITH DATA STRING
    void LoadData();
    void GetData(UniValue& objResult);

    bool ProcessVote(CNode* pfrom,
                     const CGovernanceVote& vote,
                     CGovernanceException& exception);

    void RebuildVoteMap();

    /// Called when MN's which have voted on this object have been removed
    void ClearMasternodeVotes();

    void CheckOrphanVotes();

};


#endif
