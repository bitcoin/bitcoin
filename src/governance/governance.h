// Copyright (c) 2014-2025 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_GOVERNANCE_GOVERNANCE_H
#define BITCOIN_GOVERNANCE_GOVERNANCE_H

#include <cachemap.h>
#include <cachemultimap.h>
#include <governance/signing.h>

#include <protocol.h>
#include <sync.h>

#include <chrono>
#include <limits>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <string_view>
#include <vector>

class CBloomFilter;
class CBlockIndex;
class CChain;
class CConnman;
class ChainstateManager;
template<typename T>
class CFlatDB;
class CInv;
class CNode;
struct RPCResult;

class CDeterministicMNList;
class CDeterministicMNManager;
class CGovernanceException;
class CGovernanceManager;
class CGovernanceObject;
class CGovernanceVote;
class CMasternodeMetaMan;
class CMasternodeSync;
class CSporkManager;
class CSuperblock;

class UniValue;

using CSuperblock_sptr = std::shared_ptr<CSuperblock>;
using vote_time_pair_t = std::pair<CGovernanceVote, int64_t>;

static constexpr int RATE_BUFFER_SIZE = 5;
static constexpr bool DEFAULT_GOVERNANCE_ENABLE{true};

extern RecursiveMutex cs_main; // NOLINT(readability-redundant-declaration)

class CRateCheckBuffer
{
private:
    std::vector<int64_t> vecTimestamps;

    int nDataStart{0};
    int nDataEnd{0};
    bool fBufferEmpty{true};

public:
    CRateCheckBuffer() :
        vecTimestamps(RATE_BUFFER_SIZE)
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

    using txout_m_t = std::map<COutPoint, last_object_rec>;
    using vote_cmm_t = CacheMultiMap<uint256, vote_time_pair_t>;

protected:
    static constexpr int MAX_CACHE_SIZE = 1000000;
    static const std::string SERIALIZATION_VERSION_STRING;

protected:
    // critical section to protect the inner data structures
    mutable Mutex cs_store;

    // keep track of the scanning errors
    std::map<uint256, std::shared_ptr<CGovernanceObject>> mapObjects GUARDED_BY(cs_store);
    // mapErasedGovernanceObjects contains key-value pairs, where
    //   key   - governance object's hash
    //   value - expiration time for deleted objects
    std::map<uint256, int64_t> mapErasedGovernanceObjects GUARDED_BY(cs_store);
    CacheMap<uint256, CGovernanceVote> cmapInvalidVotes GUARDED_BY(cs_store);
    vote_cmm_t cmmapOrphanVotes GUARDED_BY(cs_store);
    txout_m_t mapLastMasternodeObject GUARDED_BY(cs_store);
    // used to check for changed voting keys
    std::shared_ptr<CDeterministicMNList> lastMNListForVotingKeys GUARDED_BY(cs_store);

public:
    GovernanceStore();
    ~GovernanceStore() = default;

    template<typename Stream>
    void Serialize(Stream &s) const EXCLUSIVE_LOCKS_REQUIRED(!cs_store)
    {
        LOCK(cs_store);
        s   << SERIALIZATION_VERSION_STRING
            << mapErasedGovernanceObjects
            << cmapInvalidVotes
            << cmmapOrphanVotes
            << mapObjects
            << mapLastMasternodeObject
            << *lastMNListForVotingKeys;
    }

    template<typename Stream>
    void Unserialize(Stream &s) EXCLUSIVE_LOCKS_REQUIRED(!cs_store)
    {
        Clear();

        LOCK(cs_store);
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

    void Clear()
        EXCLUSIVE_LOCKS_REQUIRED(!cs_store);

    std::string ToString() const
        EXCLUSIVE_LOCKS_REQUIRED(!cs_store);
};

//
// Governance Manager : Contains all proposals for the budget
//
class CGovernanceManager : public GovernanceStore, public GovernanceSignerParent
{
    friend class CGovernanceObject;

private:
    using db_type = CFlatDB<GovernanceStore>;
    using object_ref_cm_t = CacheMap<uint256, std::shared_ptr<CGovernanceObject>>;

private:
    const std::unique_ptr<db_type> m_db;
    bool is_valid{false};

    CMasternodeMetaMan& m_mn_metaman;
    const ChainstateManager& m_chainman;
    const std::unique_ptr<CDeterministicMNManager>& m_dmnman;
    CMasternodeSync& m_mn_sync;

    int64_t nTimeLastDiff{0};
    // keep track of current block height
    int nCachedBlockHeight{0};
    object_ref_cm_t cmapVoteToObject;
    std::map<uint256, std::shared_ptr<CGovernanceObject>> mapPostponedObjects;
    std::set<uint256> setAdditionalRelayObjects;
    std::map<uint256, std::chrono::seconds> m_requested_hash_time;
    bool fRateChecksEnabled{true};
    std::map<uint256, std::shared_ptr<CSuperblock>> mapTrigger;

    mutable Mutex cs_relay;
    std::vector<CInv> m_relay_invs GUARDED_BY(cs_relay);

public:
    CGovernanceManager() = delete;
    CGovernanceManager(const CGovernanceManager&) = delete;
    CGovernanceManager& operator=(const CGovernanceManager&) = delete;
    explicit CGovernanceManager(CMasternodeMetaMan& mn_metaman,
                                const ChainstateManager& chainman,
                                const std::unique_ptr<CDeterministicMNManager>& dmnman, CMasternodeSync& mn_sync);
    ~CGovernanceManager();

    // Basic initialization and querying
    bool IsValid() const override { return is_valid; }
    bool LoadCache(bool load_cache)
        EXCLUSIVE_LOCKS_REQUIRED(!cs_store);
    [[nodiscard]] static RPCResult GetJsonHelp(const std::string& key, bool optional);
    std::string ToString() const
        EXCLUSIVE_LOCKS_REQUIRED(!cs_store);
    [[nodiscard]] UniValue ToJson() const
        EXCLUSIVE_LOCKS_REQUIRED(!cs_store);
    void Clear()
        EXCLUSIVE_LOCKS_REQUIRED(!cs_store);

    // CGovernanceObject
    bool AreRateChecksEnabled() const { return fRateChecksEnabled; }

    // Getters/Setters
    int GetCachedBlockHeight() const override { return nCachedBlockHeight; }
    int64_t GetLastDiffTime() const { return nTimeLastDiff; }
    std::vector<CGovernanceVote> GetCurrentVotes(const uint256& nParentHash, const COutPoint& mnCollateralOutpointFilter) const
        EXCLUSIVE_LOCKS_REQUIRED(!cs_store);
    void GetAllNewerThan(std::vector<CGovernanceObject>& objs, int64_t nMoreThanTime,
                         bool include_postponed = false) const
        EXCLUSIVE_LOCKS_REQUIRED(!cs_store);
    void UpdateLastDiffTime(int64_t nTimeIn) { nTimeLastDiff = nTimeIn; }

    // Networking
    /**
     * This is called by AlreadyHave in net_processing.cpp as part of the inventory
     * retrieval process.  Returns true if we want to retrieve the object, otherwise
     * false. (Note logic is inverted in AlreadyHave).
     */
    bool ConfirmInventoryRequest(const CInv& inv)
        EXCLUSIVE_LOCKS_REQUIRED(!cs_store);
    bool ProcessVoteAndRelay(const CGovernanceVote& vote, CGovernanceException& exception, CConnman& connman) override
        EXCLUSIVE_LOCKS_REQUIRED(!cs_store, !cs_relay);
    void RelayObject(const CGovernanceObject& obj)
        EXCLUSIVE_LOCKS_REQUIRED(!cs_relay);
    void RelayVote(const CGovernanceVote& vote)
        EXCLUSIVE_LOCKS_REQUIRED(!cs_relay);

    // Notification interface trigger
    void UpdatedBlockTip(const CBlockIndex* pindex)
        EXCLUSIVE_LOCKS_REQUIRED(!cs_store, !cs_relay);

    // Signer interface
    bool MasternodeRateCheck(const CGovernanceObject& govobj, bool fUpdateFailStatus = false) override
        EXCLUSIVE_LOCKS_REQUIRED(!cs_store);
    std::shared_ptr<CGovernanceObject> FindGovernanceObjectByDataHash(const uint256& nDataHash) override
        EXCLUSIVE_LOCKS_REQUIRED(!cs_store);
    std::vector<std::shared_ptr<const CGovernanceObject>> GetApprovedProposals(const CDeterministicMNList& tip_mn_list) override
        EXCLUSIVE_LOCKS_REQUIRED(!cs_store);
    void AddGovernanceObject(CGovernanceObject& govobj, const CNode* pfrom = nullptr) override
        EXCLUSIVE_LOCKS_REQUIRED(!cs_store, !cs_relay);

    // Superblocks
    bool GetSuperblockPayments(const CDeterministicMNList& tip_mn_list, int nBlockHeight,
                               std::vector<CTxOut>& voutSuperblockRet)
        EXCLUSIVE_LOCKS_REQUIRED(!cs_store);
    bool IsSuperblockTriggered(const CDeterministicMNList& tip_mn_list, int nBlockHeight)
        EXCLUSIVE_LOCKS_REQUIRED(!cs_store);
    bool IsValidSuperblock(const CChain& active_chain, const CDeterministicMNList& tip_mn_list,
                           const CTransaction& txNew, int nBlockHeight, CAmount blockReward)
        EXCLUSIVE_LOCKS_REQUIRED(!cs_store);

    // Thread-safe accessors
    bool GetBestSuperblock(const CDeterministicMNList& tip_mn_list, CSuperblock_sptr& pSuperblockRet,
                           int nBlockHeight) override
        EXCLUSIVE_LOCKS_REQUIRED(!cs_store);
    bool HaveObjectForHash(const uint256& nHash) const
        EXCLUSIVE_LOCKS_REQUIRED(!cs_store);
    bool HaveVoteForHash(const uint256& nHash) const
        EXCLUSIVE_LOCKS_REQUIRED(!cs_store);
    bool SerializeObjectForHash(const uint256& nHash, CDataStream& ss) const
        EXCLUSIVE_LOCKS_REQUIRED(!cs_store);
    bool SerializeVoteForHash(const uint256& nHash, CDataStream& ss) const
        EXCLUSIVE_LOCKS_REQUIRED(!cs_store);
    std::shared_ptr<CGovernanceObject> FindGovernanceObject(const uint256& nHash) override
        EXCLUSIVE_LOCKS_REQUIRED(!cs_store);
    int GetVoteCount() const
        EXCLUSIVE_LOCKS_REQUIRED(!cs_store);
    std::vector<std::shared_ptr<CSuperblock>> GetActiveTriggers() const override
        EXCLUSIVE_LOCKS_REQUIRED(!cs_store);
    void AddPostponedObject(const CGovernanceObject& govobj)
        EXCLUSIVE_LOCKS_REQUIRED(!cs_store);

    std::shared_ptr<const CGovernanceObject> FindConstGovernanceObject(const uint256& nHash) const
        EXCLUSIVE_LOCKS_REQUIRED(!cs_store);

    // Used by NetGovernance
    std::vector<CInv> FetchRelayInventory() EXCLUSIVE_LOCKS_REQUIRED(!cs_relay);
    void CheckAndRemove() EXCLUSIVE_LOCKS_REQUIRED(!cs_store);
    /** Get hashes of governance objects for which we have orphan votes. Also cleans up expired orphans. */
    [[nodiscard]] std::vector<uint256> GetOrphanVoteObjectHashes() EXCLUSIVE_LOCKS_REQUIRED(!cs_store);
    std::pair<std::vector<uint256>, std::vector<uint256>> FetchGovernanceObjectVotes(
        size_t peers_per_hash_max, int64_t now, std::map<uint256, std::map<CService, int64_t>>& map_asked_recently) const
        EXCLUSIVE_LOCKS_REQUIRED(!cs_store);
    /** Build bloom filter of existing votes for a governance object (for sync requests) */
    [[nodiscard]] CBloomFilter GetVoteBloomFilter(const uint256& nHash) const EXCLUSIVE_LOCKS_REQUIRED(!cs_store);
    /** Returns inventory items for all syncable (non-deleted, non-expired) governance objects */
    [[nodiscard]] std::vector<CInv> GetSyncableObjectInvs() const EXCLUSIVE_LOCKS_REQUIRED(!cs_store);
    /** Returns inventory items for syncable votes on a specific object, filtered by bloom filter */
    [[nodiscard]] std::vector<CInv> GetSyncableVoteInvs(const uint256& nProp, const CBloomFilter& filter) const
        EXCLUSIVE_LOCKS_REQUIRED(!cs_store);
    /// Called to indicate a requested object or vote has been received
    bool AcceptMessage(const uint256& nHash) EXCLUSIVE_LOCKS_REQUIRED(!cs_store);
    bool ProcessObject(const CNode& peer, const uint256& hash, CGovernanceObject& govobj)
        EXCLUSIVE_LOCKS_REQUIRED(::cs_main, !cs_store, !cs_relay);

    CDeterministicMNManager& GetMNManager();
    /** Process a governance vote. Returns true on success.
     *  If the vote is for an unknown object (orphan), hashToRequest is set to the object hash. */
    bool ProcessVote(CNode* pfrom, const CGovernanceVote& vote, CGovernanceException& exception, uint256& hashToRequest)
        EXCLUSIVE_LOCKS_REQUIRED(!cs_store);


private:
    // Internal counterparts to "Thread-safe accessors"
    void AddPostponedObjectInternal(const CGovernanceObject& govobj)
        EXCLUSIVE_LOCKS_REQUIRED(cs_store);
    bool GetBestSuperblockInternal(const CDeterministicMNList& tip_mn_list, CSuperblock_sptr& pSuperblockRet,
                                   int nBlockHeight)
        EXCLUSIVE_LOCKS_REQUIRED(cs_store);
    std::shared_ptr<CGovernanceObject> FindGovernanceObjectInternal(const uint256& nHash)
        EXCLUSIVE_LOCKS_REQUIRED(cs_store);
    std::vector<std::shared_ptr<CSuperblock>> GetActiveTriggersInternal() const
        EXCLUSIVE_LOCKS_REQUIRED(cs_store);

    std::shared_ptr<const CGovernanceObject> FindConstGovernanceObjectInternal(const uint256& nHash) const
        EXCLUSIVE_LOCKS_REQUIRED(cs_store);

    // Internal counterpart to "Signer interface"
    void AddGovernanceObjectInternal(CGovernanceObject& govobj, const CNode* pfrom = nullptr)
        EXCLUSIVE_LOCKS_REQUIRED(::cs_main, cs_store, !cs_relay);

    // ...
    void MasternodeRateUpdate(const CGovernanceObject& govobj)
        EXCLUSIVE_LOCKS_REQUIRED(cs_store);

    bool MasternodeRateCheck(const CGovernanceObject& govobj, bool fUpdateFailStatus, bool fForce, bool& fRateCheckBypassed)
        EXCLUSIVE_LOCKS_REQUIRED(cs_store);

    void CheckPostponedObjects()
        EXCLUSIVE_LOCKS_REQUIRED(::cs_main, cs_store, !cs_relay);

    void InitOnLoad()
        EXCLUSIVE_LOCKS_REQUIRED(!cs_store);

    /*
     * Trigger Management (formerly CGovernanceTriggerManager)
     *   - Track governance objects which are triggers
     *   - After triggers are activated and executed, they can be removed
    */
    bool AddNewTrigger(uint256 nHash)
        EXCLUSIVE_LOCKS_REQUIRED(cs_store);
    void CleanAndRemoveTriggers()
        EXCLUSIVE_LOCKS_REQUIRED(cs_store);

    void ExecuteBestSuperblock(const CDeterministicMNList& tip_mn_list, int nBlockHeight)
        EXCLUSIVE_LOCKS_REQUIRED(cs_store);

    void CheckOrphanVotes(CGovernanceObject& govobj)
        EXCLUSIVE_LOCKS_REQUIRED(cs_store, !cs_relay);

    void RebuildIndexes()
        EXCLUSIVE_LOCKS_REQUIRED(cs_store);

    void AddCachedTriggers()
        EXCLUSIVE_LOCKS_REQUIRED(cs_store);

    void RemoveInvalidVotes()
        EXCLUSIVE_LOCKS_REQUIRED(cs_store);
};

bool AreSuperblocksEnabled(const CSporkManager& sporkman);

#endif // BITCOIN_GOVERNANCE_GOVERNANCE_H
