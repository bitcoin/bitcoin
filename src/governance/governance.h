// Copyright (c) 2014-2024 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_GOVERNANCE_GOVERNANCE_H
#define BITCOIN_GOVERNANCE_GOVERNANCE_H

#include <governance/classes.h>
#include <governance/object.h>

#include <cachemap.h>
#include <cachemultimap.h>
#include <protocol.h>
#include <util/check.h>

#include <optional>
#include <string_view>

class CBloomFilter;
class CBlockIndex;
class CConnman;
template<typename T>
class CFlatDB;
class CInv;
class PeerManager;

class CDeterministicMNList;
class CDeterministicMNManager;
class CGovernanceManager;
class CGovernanceObject;
class CGovernanceVote;
class CMasternodeMetaMan;
class CMasternodeSync;
class CNetFulfilledRequestManager;
class CSporkManager;

static constexpr int RATE_BUFFER_SIZE = 5;
static constexpr bool DEFAULT_GOVERNANCE_ENABLE{true};

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

    int64_t GetMinTimestamp() const
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

    int64_t GetMaxTimestamp() const
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

    double GetRate() const
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

class GovernanceStore
{
protected:
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

    using object_ref_cm_t = CacheMap<uint256, CGovernanceObject*>;
    using txout_m_t = std::map<COutPoint, last_object_rec>;
    using vote_cmm_t = CacheMultiMap<uint256, vote_time_pair_t>;

protected:
    static constexpr int MAX_CACHE_SIZE = 1000000;
    static const std::string SERIALIZATION_VERSION_STRING;

public:
    // critical section to protect the inner data structures
    mutable RecursiveMutex cs;

protected:
    // keep track of the scanning errors
    std::map<uint256, CGovernanceObject> mapObjects GUARDED_BY(cs);
    // mapErasedGovernanceObjects contains key-value pairs, where
    //   key   - governance object's hash
    //   value - expiration time for deleted objects
    std::map<uint256, int64_t> mapErasedGovernanceObjects;
    object_ref_cm_t cmapVoteToObject;
    CacheMap<uint256, CGovernanceVote> cmapInvalidVotes;
    vote_cmm_t cmmapOrphanVotes;
    txout_m_t mapLastMasternodeObject;
    // used to check for changed voting keys
    std::shared_ptr<CDeterministicMNList> lastMNListForVotingKeys;

public:
    GovernanceStore();
    ~GovernanceStore() = default;

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
        Clear();

        LOCK(cs);
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

    void Clear();

    std::string ToString() const;
};

//
// Governance Manager : Contains all proposals for the budget
//
class CGovernanceManager : public GovernanceStore
{
    friend class CGovernanceObject;

private:
    using db_type = CFlatDB<GovernanceStore>;

private:
    static const int MAX_TIME_FUTURE_DEVIATION;
    static const int RELIABLE_PROPAGATION_TIME;

private:
    const std::unique_ptr<db_type> m_db;
    bool is_valid{false};

    CMasternodeMetaMan& m_mn_metaman;
    CNetFulfilledRequestManager& m_netfulfilledman;
    const ChainstateManager& m_chainman;
    const std::unique_ptr<CDeterministicMNManager>& m_dmnman;
    CMasternodeSync& m_mn_sync;

    int64_t nTimeLastDiff;
    // keep track of current block height
    int nCachedBlockHeight;
    std::map<uint256, CGovernanceObject> mapPostponedObjects;
    std::set<uint256> setAdditionalRelayObjects;
    std::map<uint256, std::chrono::seconds> m_requested_hash_time;
    bool fRateChecksEnabled;
    std::optional<uint256> votedFundingYesTriggerHash;
    std::map<uint256, std::shared_ptr<CSuperblock>> mapTrigger;

public:
    explicit CGovernanceManager(CMasternodeMetaMan& mn_metaman, CNetFulfilledRequestManager& netfulfilledman,
                                const ChainstateManager& chainman,
                                const std::unique_ptr<CDeterministicMNManager>& dmnman, CMasternodeSync& mn_sync);
    ~CGovernanceManager();

    bool LoadCache(bool load_cache);

    bool IsValid() const { return is_valid; }

    /**
     * This is called by AlreadyHave in net_processing.cpp as part of the inventory
     * retrieval process.  Returns true if we want to retrieve the object, otherwise
     * false. (Note logic is inverted in AlreadyHave).
     */
    bool ConfirmInventoryRequest(const CInv& inv);

    [[nodiscard]] MessageProcessingResult SyncSingleObjVotes(CNode& peer, const uint256& nProp, const CBloomFilter& filter, CConnman& connman);
    [[nodiscard]] MessageProcessingResult SyncObjects(CNode& peer, CConnman& connman) const;

    [[nodiscard]] MessageProcessingResult ProcessMessage(CNode& peer, CConnman& connman, PeerManager& peerman, std::string_view msg_type, CDataStream& vRecv);

private:
    void ResetVotedFundingTrigger();

public:
    void DoMaintenance(CConnman& connman);

    const CGovernanceObject* FindConstGovernanceObject(const uint256& nHash) const EXCLUSIVE_LOCKS_REQUIRED(cs);
    CGovernanceObject* FindGovernanceObject(const uint256& nHash) EXCLUSIVE_LOCKS_REQUIRED(cs);
    CGovernanceObject* FindGovernanceObjectByDataHash(const uint256& nDataHash) EXCLUSIVE_LOCKS_REQUIRED(cs);
    void DeleteGovernanceObject(const uint256& nHash);

    // These commands are only used in RPC
    std::vector<CGovernanceVote> GetCurrentVotes(const uint256& nParentHash, const COutPoint& mnCollateralOutpointFilter) const;
    void GetAllNewerThan(std::vector<CGovernanceObject>& objs, int64_t nMoreThanTime) const;

    void AddGovernanceObject(CGovernanceObject& govobj, PeerManager& peerman, const CNode* pfrom = nullptr);

    void CheckAndRemove();

    UniValue ToJson() const;

    void UpdatedBlockTip(const CBlockIndex* pindex, CConnman& connman, PeerManager& peerman, const CActiveMasternodeManager* const mn_activeman);
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

    bool ProcessVoteAndRelay(const CGovernanceVote& vote, CGovernanceException& exception, CConnman& connman, PeerManager& peerman);

    void CheckPostponedObjects(PeerManager& peerman);

    bool AreRateChecksEnabled() const
    {
        LOCK(cs);
        return fRateChecksEnabled;
    }

    void InitOnLoad();

    int RequestGovernanceObjectVotes(CNode& peer, CConnman& connman, const PeerManager& peerman) const;
    int RequestGovernanceObjectVotes(const std::vector<CNode*>& vNodesCopy, CConnman& connman,
                                     const PeerManager& peerman) const;

    /*
     * Trigger Management (formerly CGovernanceTriggerManager)
     *   - Track governance objects which are triggers
     *   - After triggers are activated and executed, they can be removed
    */
    std::vector<std::shared_ptr<CSuperblock>> GetActiveTriggers() const EXCLUSIVE_LOCKS_REQUIRED(cs);
    bool AddNewTrigger(uint256 nHash) EXCLUSIVE_LOCKS_REQUIRED(cs);
    void CleanAndRemoveTriggers() EXCLUSIVE_LOCKS_REQUIRED(cs);

    // Superblocks related:

    /**
     *   Is Superblock Triggered
     *
     *   - Does this block have a non-executed and activated trigger?
     */
    bool IsSuperblockTriggered(const CDeterministicMNList& tip_mn_list, int nBlockHeight);

    /**
     *   Get Superblock Payments
     *
     *   - Returns payments for superblock
     */
    bool GetSuperblockPayments(const CDeterministicMNList& tip_mn_list, int nBlockHeight,
                               std::vector<CTxOut>& voutSuperblockRet);

    bool IsValidSuperblock(const CChain& active_chain, const CDeterministicMNList& tip_mn_list,
                           const CTransaction& txNew, int nBlockHeight, CAmount blockReward);

private:
    void ExecuteBestSuperblock(const CDeterministicMNList& tip_mn_list, int nBlockHeight);
    bool GetBestSuperblock(const CDeterministicMNList& tip_mn_list, CSuperblock_sptr& pSuperblockRet, int nBlockHeight)
        EXCLUSIVE_LOCKS_REQUIRED(cs);

    std::optional<const CSuperblock> CreateSuperblockCandidate(int nHeight) const;
    std::optional<const CGovernanceObject> CreateGovernanceTrigger(const std::optional<const CSuperblock>& sb_opt, PeerManager& peerman,
                                                                   const CActiveMasternodeManager& mn_activeman);
    void VoteGovernanceTriggers(const std::optional<const CGovernanceObject>& trigger_opt, CConnman& connman, PeerManager& peerman,
                                const CActiveMasternodeManager& mn_activeman);
    bool VoteFundingTrigger(const uint256& nHash, const vote_outcome_enum_t outcome, CConnman& connman, PeerManager& peerman,
                            const CActiveMasternodeManager& mn_activeman);
    bool HasAlreadyVotedFundingTrigger() const;

    void RequestGovernanceObject(CNode* pfrom, const uint256& nHash, CConnman& connman, bool fUseFilter = false) const;

    void AddInvalidVote(const CGovernanceVote& vote)
    {
        cmapInvalidVotes.Insert(vote.GetHash(), vote);
    }

    bool ProcessVote(CNode* pfrom, const CGovernanceVote& vote, CGovernanceException& exception, CConnman& connman);

    /// Called to indicate a requested object or vote has been received
    bool AcceptMessage(const uint256& nHash);

    void CheckOrphanVotes(CGovernanceObject& govobj, PeerManager& peerman);

    void RebuildIndexes();

    void AddCachedTriggers();

    void RequestOrphanObjects(CConnman& connman);

    void CleanOrphanObjects();

    void RemoveInvalidVotes();

};

bool AreSuperblocksEnabled(const CSporkManager& sporkman);

#endif // BITCOIN_GOVERNANCE_GOVERNANCE_H
