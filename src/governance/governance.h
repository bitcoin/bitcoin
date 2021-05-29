// Copyright (c) 2014-2021 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_GOVERNANCE_GOVERNANCE_H
#define BITCOIN_GOVERNANCE_GOVERNANCE_H

#include <cachemap.h>
#include <cachemultimap.h>
#include <governance/governance-object.h>

class CBloomFilter;
class CBlockIndex;
class CInv;

class CGovernanceManager;
class CGovernanceTriggerManager;
class CGovernanceObject;
class CGovernanceVote;

extern CGovernanceManager governance;

static const int RATE_BUFFER_SIZE = 5;

class CDeterministicMNList;
typedef std::shared_ptr<CDeterministicMNList> CDeterministicMNListPtr;

class CRateCheckBuffer
{
private:
    std::vector<int64_t> vecTimestamps;

    int nDataStart;

    int nDataEnd;

    bool fBufferEmpty;

public:
    CRateCheckBuffer() :
        vecTimestamps(RATE_BUFFER_SIZE),
        nDataStart(0),
        nDataEnd(0),
        fBufferEmpty(true)
    {
    }

    void AddTimestamp(int64_t nTimestamp)
    {
        if ((nDataEnd == nDataStart) && !fBufferEmpty) {
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
        int64_t nMin = std::numeric_limits<int64_t>::max();
        if (fBufferEmpty) {
            return nMin;
        }
        do {
            if (vecTimestamps[nIndex] < nMin) {
                nMin = vecTimestamps[nIndex];
            }
            nIndex = (nIndex + 1) % RATE_BUFFER_SIZE;
        } while (nIndex != nDataEnd);
        return nMin;
    }

    int64_t GetMaxTimestamp()
    {
        int nIndex = nDataStart;
        int64_t nMax = 0;
        if (fBufferEmpty) {
            return nMax;
        }
        do {
            if (vecTimestamps[nIndex] > nMax) {
                nMax = vecTimestamps[nIndex];
            }
            nIndex = (nIndex + 1) % RATE_BUFFER_SIZE;
        } while (nIndex != nDataEnd);
        return nMax;
    }

    int GetCount() const
    {
        if (fBufferEmpty) {
            return 0;
        }
        if (nDataEnd > nDataStart) {
            return nDataEnd - nDataStart;
        }
        return RATE_BUFFER_SIZE - nDataStart + nDataEnd;
    }

    double GetRate()
    {
        int nCount = GetCount();
        if (nCount < RATE_BUFFER_SIZE) {
            return 0.0;
        }
        int64_t nMin = GetMinTimestamp();
        int64_t nMax = GetMaxTimestamp();
        if (nMin == nMax) {
            // multiple objects with the same timestamp => infinite rate
            return 1.0e10;
        }
        return double(nCount) / double(nMax - nMin);
    }

    SERIALIZE_METHODS(CRateCheckBuffer, obj)
    {
        READWRITE(obj.vecTimestamps, obj.nDataStart, obj.nDataEnd, obj.fBufferEmpty);
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
        explicit last_object_rec(bool fStatusOKIn = true) :
            triggerBuffer(),
            fStatusOK(fStatusOKIn)
        {
        }

        SERIALIZE_METHODS(last_object_rec, obj)
        {
            READWRITE(obj.triggerBuffer, obj.fStatusOK);
        }

        CRateCheckBuffer triggerBuffer;
        bool fStatusOK;
    };


    typedef CacheMap<uint256, CGovernanceObject*> object_ref_cm_t;

    typedef CacheMultiMap<uint256, vote_time_pair_t> vote_cmm_t;

    typedef std::map<COutPoint, last_object_rec> txout_m_t;

    typedef std::set<uint256> hash_s_t;

private:
    static const int MAX_CACHE_SIZE = 1000000;

    static const std::string SERIALIZATION_VERSION_STRING;

    static const int MAX_TIME_FUTURE_DEVIATION;
    static const int RELIABLE_PROPAGATION_TIME;

    int64_t nTimeLastDiff;

    // keep track of current block height
    int nCachedBlockHeight;

    // keep track of the scanning errors
    std::map<uint256, CGovernanceObject> mapObjects;

    // mapErasedGovernanceObjects contains key-value pairs, where
    //   key   - governance object's hash
    //   value - expiration time for deleted objects
    std::map<uint256, int64_t> mapErasedGovernanceObjects;

    std::map<uint256, CGovernanceObject> mapPostponedObjects;
    hash_s_t setAdditionalRelayObjects;

    object_ref_cm_t cmapVoteToObject;

    CacheMap<uint256, CGovernanceVote> cmapInvalidVotes;

    vote_cmm_t cmmapOrphanVotes;

    txout_m_t mapLastMasternodeObject;

    hash_s_t setRequestedObjects;

    hash_s_t setRequestedVotes;

    bool fRateChecksEnabled;

    // used to check for changed voting keys
    CDeterministicMNListPtr lastMNListForVotingKeys;

    class ScopedLockBool
    {
        bool& ref;
        bool fPrevValue;

    public:
        ScopedLockBool(CCriticalSection& _cs, bool& _ref, bool _value) :
            ref(_ref)
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

    virtual ~CGovernanceManager() = default;

    /**
     * This is called by AlreadyHave in net_processing.cpp as part of the inventory
     * retrieval process.  Returns true if we want to retrieve the object, otherwise
     * false. (Note logic is inverted in AlreadyHave).
     */
    bool ConfirmInventoryRequest(const CInv& inv);

    void SyncSingleObjVotes(CNode* pnode, const uint256& nProp, const CBloomFilter& filter, CConnman& connman);
    void SyncObjects(CNode* pnode, CConnman& connman) const;

    void ProcessMessage(CNode* pfrom, const std::string& strCommand, CDataStream& vRecv, CConnman& connman, bool enable_bip61);

    void DoMaintenance(CConnman& connman);

    CGovernanceObject* FindGovernanceObject(const uint256& nHash);

    // These commands are only used in RPC
    std::vector<CGovernanceVote> GetCurrentVotes(const uint256& nParentHash, const COutPoint& mnCollateralOutpointFilter) const;
    std::vector<const CGovernanceObject*> GetAllNewerThan(int64_t nMoreThanTime) const;

    void AddGovernanceObject(CGovernanceObject& govobj, CConnman& connman, CNode* pfrom = nullptr);

    void UpdateCachesAndClean();

    void CheckAndRemove() { UpdateCachesAndClean(); }

    void Clear()
    {
        LOCK(cs);

        LogPrint(BCLog::GOBJECT, "Governance object manager was cleared\n");
        mapObjects.clear();
        mapErasedGovernanceObjects.clear();
        cmapVoteToObject.Clear();
        cmapInvalidVotes.Clear();
        cmmapOrphanVotes.Clear();
        mapLastMasternodeObject.clear();
    }

    std::string ToString() const;
    UniValue ToJson() const;

    template<typename Stream>
    void Serialize(Stream &s) const
    {
        LOCK(cs);
        s   << SERIALIZATION_VERSION_STRING
            << mapErasedGovernanceObjects
            << cmapInvalidVotes
            << cmmapOrphanVotes
            << mapObjects
            << mapLastMasternodeObject
            << *lastMNListForVotingKeys;
    }

    template<typename Stream>
    void Unserialize(Stream &s)
    {
        LOCK(cs);
        Clear();

        std::string strVersion;
        s >> strVersion;
        if (strVersion != SERIALIZATION_VERSION_STRING) {
            return;
        }

        s   >> mapErasedGovernanceObjects
            >> cmapInvalidVotes
            >> cmmapOrphanVotes
            >> mapObjects
            >> mapLastMasternodeObject
            >> *lastMNListForVotingKeys;
    }

    void UpdatedBlockTip(const CBlockIndex* pindex, CConnman& connman);
    int64_t GetLastDiffTime() const { return nTimeLastDiff; }
    void UpdateLastDiffTime(int64_t nTimeIn) { nTimeLastDiff = nTimeIn; }

    int GetCachedBlockHeight() const { return nCachedBlockHeight; }

    // Accessors for thread-safe access to maps
    bool HaveObjectForHash(const uint256& nHash) const;

    bool HaveVoteForHash(const uint256& nHash) const;

    int GetVoteCount() const;

    bool SerializeObjectForHash(const uint256& nHash, CDataStream& ss) const;

    bool SerializeVoteForHash(const uint256& nHash, CDataStream& ss) const;

    void AddPostponedObject(const CGovernanceObject& govobj)
    {
        LOCK(cs);
        mapPostponedObjects.insert(std::make_pair(govobj.GetHash(), govobj));
    }

    void MasternodeRateUpdate(const CGovernanceObject& govobj);

    bool MasternodeRateCheck(const CGovernanceObject& govobj, bool fUpdateFailStatus = false);

    bool MasternodeRateCheck(const CGovernanceObject& govobj, bool fUpdateFailStatus, bool fForce, bool& fRateCheckBypassed);

    bool ProcessVoteAndRelay(const CGovernanceVote& vote, CGovernanceException& exception, CConnman& connman)
    {
        bool fOK = ProcessVote(nullptr, vote, exception, connman);
        if (fOK) {
            vote.Relay(connman);
        }
        return fOK;
    }

    void CheckPostponedObjects(CConnman& connman);

    bool AreRateChecksEnabled() const
    {
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
        cmapInvalidVotes.Insert(vote.GetHash(), vote);
    }

    bool ProcessVote(CNode* pfrom, const CGovernanceVote& vote, CGovernanceException& exception, CConnman& connman);

    /// Called to indicate a requested object has been received
    bool AcceptObjectMessage(const uint256& nHash);

    /// Called to indicate a requested vote has been received
    bool AcceptVoteMessage(const uint256& nHash);

    static bool AcceptMessage(const uint256& nHash, hash_s_t& setHash);

    void CheckOrphanVotes(CGovernanceObject& govobj, CConnman& connman);

    void RebuildIndexes();

    void AddCachedTriggers();

    void RequestOrphanObjects(CConnman& connman);

    void CleanOrphanObjects();

    void RemoveInvalidVotes();

};

bool AreSuperblocksEnabled();

#endif // BITCOIN_GOVERNANCE_GOVERNANCE_H
