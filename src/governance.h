// Copyright (c) 2014-2017 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef GOVERNANCE_H
#define GOVERNANCE_H

//#define ENABLE_DASH_DEBUG

#include "bloom.h"
#include "cachemap.h"
#include "cachemultimap.h"
#include "chain.h"
#include "governance-exceptions.h"
#include "governance-object.h"
#include "governance-vote.h"
#include "net.h"
#include "sync.h"
#include "timedata.h"
#include "util.h"

class CGovernanceManager;
class CGovernanceTriggerManager;
class CGovernanceObject;
class CGovernanceVote;

extern CGovernanceManager governance;

struct ExpirationInfo {
    ExpirationInfo(int64_t _nExpirationTime, int _idFrom) : nExpirationTime(_nExpirationTime), idFrom(_idFrom) {}

    int64_t nExpirationTime;
    NodeId idFrom;
};

typedef std::pair<CGovernanceObject, ExpirationInfo> object_info_pair_t;

static const int RATE_BUFFER_SIZE = 5;

class CRateCheckBuffer {
private:
    std::vector<int64_t> vecTimestamps;

    int nDataStart;

    int nDataEnd;

    bool fBufferEmpty;

public:
    CRateCheckBuffer()
        : vecTimestamps(RATE_BUFFER_SIZE),
          nDataStart(0),
          nDataEnd(0),
          fBufferEmpty(true)
        {}

    void AddTimestamp(int64_t nTimestamp)
    {
        if((nDataEnd == nDataStart) && !fBufferEmpty) {
            // Buffer full, discard 1st element
            nDataStart = (nDataStart + 1) % RATE_BUFFER_SIZE;
        }
        vecTimestamps[nDataEnd] = nTimestamp;
        nDataEnd = (nDataEnd + 1) % RATE_BUFFER_SIZE;
        fBufferEmpty = false;
    }

    int64_t GetMinTimestamp()
    {
        int nIndex = nDataStart;
        int64_t nMin = numeric_limits<int64_t>::max();
        if(fBufferEmpty) {
            return nMin;
        }
        do {
            if(vecTimestamps[nIndex] < nMin) {
                nMin = vecTimestamps[nIndex];
            }
            nIndex = (nIndex + 1) % RATE_BUFFER_SIZE;
        } while(nIndex != nDataEnd);
        return nMin;
    }

    int64_t GetMaxTimestamp()
    {
        int nIndex = nDataStart;
        int64_t nMax = 0;
        if(fBufferEmpty) {
            return nMax;
        }
        do {
            if(vecTimestamps[nIndex] > nMax) {
                nMax = vecTimestamps[nIndex];
            }
            nIndex = (nIndex + 1) % RATE_BUFFER_SIZE;
        } while(nIndex != nDataEnd);
        return nMax;
    }

    int GetCount()
    {
        int nCount = 0;
        if(fBufferEmpty) {
            return 0;
        }
        if(nDataEnd > nDataStart) {
            nCount = nDataEnd - nDataStart;
        }
        else {
            nCount = RATE_BUFFER_SIZE - nDataStart + nDataEnd;
        }

        return nCount;
    }

    double GetRate()
    {
        int nCount = GetCount();
        if(nCount < RATE_BUFFER_SIZE) {
            return 0.0;
        }
        int64_t nMin = GetMinTimestamp();
        int64_t nMax = GetMaxTimestamp();
        if(nMin == nMax) {
            // multiple objects with the same timestamp => infinite rate
            return 1.0e10;
        }
        return double(nCount) / double(nMax - nMin);
    }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion)
    {
        READWRITE(vecTimestamps);
        READWRITE(nDataStart);
        READWRITE(nDataEnd);
        READWRITE(fBufferEmpty);
    }
};

//
// Governance Manager : Contains all proposals for the budget
//
class CGovernanceManager
{
    friend class CGovernanceObject;

public: // Types
    struct last_object_rec {
        last_object_rec(bool fStatusOKIn = true)
            : triggerBuffer(),
              watchdogBuffer(),
              fStatusOK(fStatusOKIn)
            {}

        ADD_SERIALIZE_METHODS;

        template <typename Stream, typename Operation>
        inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion)
        {
            READWRITE(triggerBuffer);
            READWRITE(watchdogBuffer);
            READWRITE(fStatusOK);
        }

        CRateCheckBuffer triggerBuffer;
        CRateCheckBuffer watchdogBuffer;
        bool fStatusOK;
    };


    typedef std::map<uint256, CGovernanceObject> object_m_t;

    typedef object_m_t::iterator object_m_it;

    typedef object_m_t::const_iterator object_m_cit;

    typedef CacheMap<uint256, CGovernanceObject*> object_ref_cache_t;

    typedef std::map<uint256, CGovernanceVote> vote_m_t;

    typedef vote_m_t::iterator vote_m_it;

    typedef vote_m_t::const_iterator vote_m_cit;

    typedef CacheMap<uint256, CGovernanceVote> vote_cache_t;

    typedef CacheMultiMap<uint256, vote_time_pair_t> vote_mcache_t;

    typedef object_m_t::size_type size_type;

    typedef std::map<COutPoint, last_object_rec > txout_m_t;

    typedef txout_m_t::iterator txout_m_it;

    typedef txout_m_t::const_iterator txout_m_cit;

    typedef std::map<COutPoint, int> txout_int_m_t;

    typedef std::set<uint256> hash_s_t;

    typedef hash_s_t::iterator hash_s_it;

    typedef hash_s_t::const_iterator hash_s_cit;

    typedef std::map<uint256, object_info_pair_t> object_info_m_t;

    typedef object_info_m_t::iterator object_info_m_it;

    typedef object_info_m_t::const_iterator object_info_m_cit;

    typedef std::map<uint256, int64_t> hash_time_m_t;

    typedef hash_time_m_t::iterator hash_time_m_it;

    typedef hash_time_m_t::const_iterator hash_time_m_cit;

private:
    static const int MAX_CACHE_SIZE = 1000000;

    static const std::string SERIALIZATION_VERSION_STRING;

    static const int MAX_TIME_FUTURE_DEVIATION;
    static const int RELIABLE_PROPAGATION_TIME;

    int64_t nTimeLastDiff;

    // keep track of current block height
    int nCachedBlockHeight;

    // keep track of the scanning errors
    object_m_t mapObjects;

    // mapErasedGovernanceObjects contains key-value pairs, where
    //   key   - governance object's hash
    //   value - expiration time for deleted objects
    hash_time_m_t mapErasedGovernanceObjects;

    object_info_m_t mapMasternodeOrphanObjects;
    txout_int_m_t mapMasternodeOrphanCounter;

    object_m_t mapPostponedObjects;
    hash_s_t setAdditionalRelayObjects;

    hash_time_m_t mapWatchdogObjects;

    uint256 nHashWatchdogCurrent;

    int64_t nTimeWatchdogCurrent;

    object_ref_cache_t mapVoteToObject;

    vote_cache_t mapInvalidVotes;

    vote_mcache_t mapOrphanVotes;

    txout_m_t mapLastMasternodeObject;

    hash_s_t setRequestedObjects;

    hash_s_t setRequestedVotes;

    bool fRateChecksEnabled;

    class ScopedLockBool
    {
        bool& ref;
        bool fPrevValue;

    public:
        ScopedLockBool(CCriticalSection& _cs, bool& _ref, bool _value) : ref(_ref)
        {
            AssertLockHeld(_cs);
            fPrevValue = ref;
            ref = _value;
        }

        ~ScopedLockBool()
        {
            ref = fPrevValue;
        }
    };

public:
    // critical section to protect the inner data structures
    mutable CCriticalSection cs;

    CGovernanceManager();

    virtual ~CGovernanceManager() {}

    /**
     * This is called by AlreadyHave in main.cpp as part of the inventory
     * retrieval process.  Returns true if we want to retrieve the object, otherwise
     * false. (Note logic is inverted in AlreadyHave).
     */
    bool ConfirmInventoryRequest(const CInv& inv);

    void Sync(CNode* node, const uint256& nProp, const CBloomFilter& filter, CConnman& connman);

    void ProcessMessage(CNode* pfrom, std::string& strCommand, CDataStream& vRecv, CConnman& connman);

    void DoMaintenance(CConnman& connman);

    CGovernanceObject *FindGovernanceObject(const uint256& nHash);

    std::vector<CGovernanceVote> GetMatchingVotes(const uint256& nParentHash);
    std::vector<CGovernanceVote> GetCurrentVotes(const uint256& nParentHash, const COutPoint& mnCollateralOutpointFilter);
    std::vector<CGovernanceObject*> GetAllNewerThan(int64_t nMoreThanTime);

    bool IsBudgetPaymentBlock(int nBlockHeight);
    void AddGovernanceObject(CGovernanceObject& govobj, CConnman& connman, CNode* pfrom = NULL);

    std::string GetRequiredPaymentsString(int nBlockHeight);

    void UpdateCachesAndClean();

    void CheckAndRemove() {UpdateCachesAndClean();}

    void Clear()
    {
        LOCK(cs);

        LogPrint("gobject", "Governance object manager was cleared\n");
        mapObjects.clear();
        mapErasedGovernanceObjects.clear();
        mapWatchdogObjects.clear();
        nHashWatchdogCurrent = uint256();
        nTimeWatchdogCurrent = 0;
        mapVoteToObject.Clear();
        mapInvalidVotes.Clear();
        mapOrphanVotes.Clear();
        mapLastMasternodeObject.clear();
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

        READWRITE(mapErasedGovernanceObjects);
        READWRITE(mapInvalidVotes);
        READWRITE(mapOrphanVotes);
        READWRITE(mapObjects);
        READWRITE(mapWatchdogObjects);
        READWRITE(nHashWatchdogCurrent);
        READWRITE(nTimeWatchdogCurrent);
        READWRITE(mapLastMasternodeObject);
        if(ser_action.ForRead() && (strVersion != SERIALIZATION_VERSION_STRING)) {
            Clear();
            return;
        }
    }

    void UpdatedBlockTip(const CBlockIndex *pindex, CConnman& connman);
    int64_t GetLastDiffTime() { return nTimeLastDiff; }
    void UpdateLastDiffTime(int64_t nTimeIn) { nTimeLastDiff = nTimeIn; }

    int GetCachedBlockHeight() { return nCachedBlockHeight; }

    // Accessors for thread-safe access to maps
    bool HaveObjectForHash(uint256 nHash);

    bool HaveVoteForHash(uint256 nHash);

    int GetVoteCount() const;

    bool SerializeObjectForHash(uint256 nHash, CDataStream& ss);

    bool SerializeVoteForHash(uint256 nHash, CDataStream& ss);

    void AddPostponedObject(const CGovernanceObject& govobj)
    {
        LOCK(cs);
        mapPostponedObjects.insert(std::make_pair(govobj.GetHash(), govobj));
    }

    void AddSeenGovernanceObject(uint256 nHash, int status);

    void AddSeenVote(uint256 nHash, int status);

    void MasternodeRateUpdate(const CGovernanceObject& govobj);

    bool MasternodeRateCheck(const CGovernanceObject& govobj, bool fUpdateFailStatus = false);

    bool MasternodeRateCheck(const CGovernanceObject& govobj, bool fUpdateFailStatus, bool fForce, bool& fRateCheckBypassed);

    bool ProcessVoteAndRelay(const CGovernanceVote& vote, CGovernanceException& exception, CConnman& connman) {
        bool fOK = ProcessVote(NULL, vote, exception, connman);
        if(fOK) {
            vote.Relay(connman);
        }
        return fOK;
    }

    void CheckMasternodeOrphanVotes(CConnman& connman);

    void CheckMasternodeOrphanObjects(CConnman& connman);

    void CheckPostponedObjects(CConnman& connman);

    bool AreRateChecksEnabled() const {
        LOCK(cs);
        return fRateChecksEnabled;
    }

    void InitOnLoad();

    int RequestGovernanceObjectVotes(CNode* pnode, CConnman& connman);
    int RequestGovernanceObjectVotes(const std::vector<CNode*>& vNodesCopy, CConnman& connman);

private:
    void RequestGovernanceObject(CNode* pfrom, const uint256& nHash, CConnman& connman, bool fUseFilter = false);

    void AddInvalidVote(const CGovernanceVote& vote)
    {
        mapInvalidVotes.Insert(vote.GetHash(), vote);
    }

    void AddOrphanVote(const CGovernanceVote& vote)
    {
        mapOrphanVotes.Insert(vote.GetHash(), vote_time_pair_t(vote, GetAdjustedTime() + GOVERNANCE_ORPHAN_EXPIRATION_TIME));
    }

    bool ProcessVote(CNode* pfrom, const CGovernanceVote& vote, CGovernanceException& exception, CConnman& connman);

    /// Called to indicate a requested object has been received
    bool AcceptObjectMessage(const uint256& nHash);

    /// Called to indicate a requested vote has been received
    bool AcceptVoteMessage(const uint256& nHash);

    static bool AcceptMessage(const uint256& nHash, hash_s_t& setHash);

    void CheckOrphanVotes(CGovernanceObject& govobj, CGovernanceException& exception, CConnman& connman);

    void RebuildIndexes();

    void AddCachedTriggers();

    bool UpdateCurrentWatchdog(CGovernanceObject& watchdogNew);

    void RequestOrphanObjects(CConnman& connman);

    void CleanOrphanObjects();

};

#endif
