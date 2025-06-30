// Copyright (c) 2018-2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_EVO_DETERMINISTICMNS_H
#define BITCOIN_EVO_DETERMINISTICMNS_H

#include <evo/dmnstate.h>

#include <arith_uint256.h>
#include <clientversion.h>
#include <consensus/params.h>
#include <crypto/common.h>
#include <evo/dmn_types.h>
#include <evo/providertx.h>
#include <gsl/pointers.h>
#include <saltedhasher.h>
#include <scheduler.h>
#include <sync.h>

#include <immer/map.hpp>

#include <atomic>
#include <limits>
#include <numeric>
#include <unordered_map>
#include <utility>

class CBlock;
class CBlockIndex;
class CCoinsViewCache;
class CEvoDB;
class CSimplifiedMNList;
class CSimplifiedMNListEntry;
class TxValidationState;

extern RecursiveMutex cs_main;

class CDeterministicMN
{
private:
    uint64_t internalId{std::numeric_limits<uint64_t>::max()};

public:
    static constexpr uint16_t MN_VERSION_FORMAT = 2;
    static constexpr uint16_t MN_CURRENT_FORMAT = MN_VERSION_FORMAT;

    uint256 proTxHash;
    COutPoint collateralOutpoint;
    uint16_t nOperatorReward{0};
    MnType nType{MnType::Regular};
    std::shared_ptr<const CDeterministicMNState> pdmnState;

    CDeterministicMN() = delete; // no default constructor, must specify internalId
    explicit CDeterministicMN(uint64_t _internalId, MnType mnType = MnType::Regular) :
        internalId(_internalId),
        nType(mnType)
    {
        // only non-initial values
        assert(_internalId != std::numeric_limits<uint64_t>::max());
    }
    template <typename Stream>
    CDeterministicMN(deserialize_type, Stream& s) { s >> *this; }

    SERIALIZE_METHODS(CDeterministicMN, obj)
    {
        READWRITE(obj.proTxHash);
        READWRITE(VARINT(obj.internalId));
        READWRITE(obj.collateralOutpoint);
        READWRITE(obj.nOperatorReward);
        READWRITE(obj.pdmnState);
        // We can't know if we are serialising for the Disk or for the Network here (s.GetType() is not accessible)
        // Therefore if s.GetVersion() == CLIENT_VERSION -> Then we know we are serialising for the Disk
        // Otherwise, we can safely check with protocol versioning logic so we won't break old clients
        if (s.GetVersion() == CLIENT_VERSION || s.GetVersion() >= DMN_TYPE_PROTO_VERSION) {
            READWRITE(obj.nType);
        } else {
            SER_READ(obj, obj.nType = MnType::Regular);
        }
    }

    [[nodiscard]] uint64_t GetInternalId() const;

    [[nodiscard]] CSimplifiedMNListEntry to_sml_entry() const;
    [[nodiscard]] std::string ToString() const;
    [[nodiscard]] UniValue ToJson() const;
};
using CDeterministicMNCPtr = std::shared_ptr<const CDeterministicMN>;

class CDeterministicMNListDiff;

template <typename Stream, typename K, typename T, typename Hash, typename Equal>
void SerializeImmerMap(Stream& os, const immer::map<K, T, Hash, Equal>& m)
{
    WriteCompactSize(os, m.size());
    for (typename immer::map<K, T, Hash, Equal>::const_iterator mi = m.begin(); mi != m.end(); ++mi)
        Serialize(os, (*mi));
}

template <typename Stream, typename K, typename T, typename Hash, typename Equal>
void UnserializeImmerMap(Stream& is, immer::map<K, T, Hash, Equal>& m)
{
    m = immer::map<K, T, Hash, Equal>();
    unsigned int nSize = ReadCompactSize(is);
    for (unsigned int i = 0; i < nSize; i++) {
        std::pair<K, T> item;
        Unserialize(is, item);
        m = m.set(item.first, item.second);
    }
}

// For some reason the compiler is not able to choose the correct Serialize/Deserialize methods without a specialized
// version of SerReadWrite. It otherwise always chooses the version that calls a.Serialize()
template<typename Stream, typename K, typename T, typename Hash, typename Equal>
inline void SerReadWrite(Stream& s, const immer::map<K, T, Hash, Equal>& m, CSerActionSerialize ser_action)
{
    ::SerializeImmerMap(s, m);
}

template<typename Stream, typename K, typename T, typename Hash, typename Equal>
inline void SerReadWrite(Stream& s, immer::map<K, T, Hash, Equal>& obj, CSerActionUnserialize ser_action)
{
    ::UnserializeImmerMap(s, obj);
}


class CDeterministicMNList
{
private:
    struct ImmerHasher
    {
        size_t operator()(const uint256& hash) const { return ReadLE64(hash.begin()); }
    };

public:
    using MnMap = immer::map<uint256, CDeterministicMNCPtr, ImmerHasher>;
    using MnInternalIdMap = immer::map<uint64_t, uint256>;
    using MnUniquePropertyMap = immer::map<uint256, std::pair<uint256, uint32_t>, ImmerHasher>;

private:
    uint256 blockHash;
    int nHeight{-1};
    uint32_t nTotalRegisteredCount{0};
    MnMap mnMap;
    MnInternalIdMap mnInternalIdMap;

    // map of unique properties like address and keys
    // we keep track of this as checking for duplicates would otherwise be painfully slow
    MnUniquePropertyMap mnUniquePropertyMap;

    // This SML could be null
    // This cache is used to improve performance and meant to be reused
    // for multiple CDeterministicMNList until mnMap is actually changed.
    // Calls of AddMN, RemoveMN and (in some cases) UpdateMN reset this cache;
    // it happens also for indirect calls such as ApplyDiff
    mutable std::shared_ptr<const CSimplifiedMNList> m_cached_sml;

public:
    CDeterministicMNList() = default;
    explicit CDeterministicMNList(const uint256& _blockHash, int _height, uint32_t _totalRegisteredCount) :
        blockHash(_blockHash),
        nHeight(_height),
        nTotalRegisteredCount(_totalRegisteredCount)
    {
        assert(nHeight >= 0);
    }

    template <typename Stream, typename Operation>
    inline void SerializationOpBase(Stream& s, Operation ser_action)
    {
        READWRITE(blockHash);
        READWRITE(nHeight);
        READWRITE(nTotalRegisteredCount);
    }

    template<typename Stream>
    void Serialize(Stream& s) const
    {
        const_cast<CDeterministicMNList*>(this)->SerializationOpBase(s, CSerActionSerialize());

        // Serialize the map as a vector
        WriteCompactSize(s, mnMap.size());
        for (const auto& [_, dmn] : mnMap) {
            s << *dmn;
        }
    }

    template <typename Stream>
    void Unserialize(Stream& s)
    {
        Clear();

        SerializationOpBase(s, CSerActionUnserialize());

        for (size_t to_read = ReadCompactSize(s); to_read > 0; --to_read) {
            AddMN(std::make_shared<CDeterministicMN>(deserialize, s), /*fBumpTotalCount=*/false);
        }
    }

    void Clear()
    {
        blockHash = uint256{};
        nHeight = -1;
        nTotalRegisteredCount = 0;
        mnMap = MnMap();
        mnUniquePropertyMap = MnUniquePropertyMap();
        mnInternalIdMap = MnInternalIdMap();
        m_cached_sml = nullptr;
    }

    [[nodiscard]] size_t GetAllMNsCount() const
    {
        return mnMap.size();
    }

    [[nodiscard]] size_t GetValidMNsCount() const
    {
        return ranges::count_if(mnMap, [](const auto& p) { return !p.second->pdmnState->IsBanned(); });
    }

    [[nodiscard]] size_t GetAllEvoCount() const
    {
        return ranges::count_if(mnMap, [](const auto& p) { return p.second->nType == MnType::Evo; });
    }

    [[nodiscard]] size_t GetValidEvoCount() const
    {
        return ranges::count_if(mnMap, [](const auto& p) {
            return p.second->nType == MnType::Evo && !p.second->pdmnState->IsBanned();
        });
    }

    [[nodiscard]] size_t GetValidWeightedMNsCount() const
    {
        return std::accumulate(mnMap.begin(), mnMap.end(), 0, [](auto res, const auto& p) {
            if (p.second->pdmnState->IsBanned()) return res;
            return res + GetMnType(p.second->nType).voting_weight;
        });
    }

    /**
     * Execute a callback on all masternodes in the mnList. This will pass a reference
     * of each masternode to the callback function. This should be preferred over ForEachMNShared.
     * @param onlyValid Run on all masternodes, or only "valid" (not banned) masternodes
     * @param cb callback to execute
     */
    template <typename Callback>
    void ForEachMN(bool onlyValid, Callback&& cb) const
    {
        for (const auto& p : mnMap) {
            if (!onlyValid || !p.second->pdmnState->IsBanned()) {
                cb(*p.second);
            }
        }
    }

    /**
     * Prefer ForEachMN. Execute a callback on all masternodes in the mnList.
     * This will pass a non-null shared_ptr of each masternode to the callback function.
     * Use this function only when a shared_ptr is needed in order to take shared ownership.
     * @param onlyValid Run on all masternodes, or only "valid" (not banned) masternodes
     * @param cb callback to execute
     */
    template <typename Callback>
    void ForEachMNShared(bool onlyValid, Callback&& cb) const
    {
        for (const auto& p : mnMap) {
            if (!onlyValid || !p.second->pdmnState->IsBanned()) {
                cb(p.second);
            }
        }
    }

    [[nodiscard]] const uint256& GetBlockHash() const
    {
        return blockHash;
    }
    void SetBlockHash(const uint256& _blockHash)
    {
        blockHash = _blockHash;
    }
    [[nodiscard]] int GetHeight() const
    {
        assert(nHeight >= 0);
        return nHeight;
    }
    void SetHeight(int _height)
    {
        assert(_height >= 0);
        nHeight = _height;
    }
    [[nodiscard]] uint32_t GetTotalRegisteredCount() const
    {
        return nTotalRegisteredCount;
    }

    [[nodiscard]] bool IsMNValid(const uint256& proTxHash) const;
    [[nodiscard]] bool IsMNPoSeBanned(const uint256& proTxHash) const;

    [[nodiscard]] bool HasMN(const uint256& proTxHash) const
    {
        return GetMN(proTxHash) != nullptr;
    }
    [[nodiscard]] bool HasMNByCollateral(const COutPoint& collateralOutpoint) const
    {
        return GetMNByCollateral(collateralOutpoint) != nullptr;
    }
    [[nodiscard]] bool HasValidMNByCollateral(const COutPoint& collateralOutpoint) const
    {
        return GetValidMNByCollateral(collateralOutpoint) != nullptr;
    }
    [[nodiscard]] CDeterministicMNCPtr GetMN(const uint256& proTxHash) const;
    [[nodiscard]] CDeterministicMNCPtr GetValidMN(const uint256& proTxHash) const;
    [[nodiscard]] CDeterministicMNCPtr GetMNByOperatorKey(const CBLSPublicKey& pubKey) const;
    [[nodiscard]] CDeterministicMNCPtr GetMNByCollateral(const COutPoint& collateralOutpoint) const;
    [[nodiscard]] CDeterministicMNCPtr GetValidMNByCollateral(const COutPoint& collateralOutpoint) const;
    [[nodiscard]] CDeterministicMNCPtr GetMNByService(const CService& service) const;
    [[nodiscard]] CDeterministicMNCPtr GetMNByInternalId(uint64_t internalId) const;
    [[nodiscard]] CDeterministicMNCPtr GetMNPayee(gsl::not_null<const CBlockIndex*> pindexPrev) const;

    /**
     * Calculates the projected MN payees for the next *count* blocks. The result is not guaranteed to be correct
     * as PoSe banning might occur later
     * @param nCount the number of payees to return. "nCount = max()"" means "all", use it to avoid calling GetValidWeightedMNsCount twice.
     */
    [[nodiscard]] std::vector<CDeterministicMNCPtr> GetProjectedMNPayees(gsl::not_null<const CBlockIndex* const> pindexPrev, int nCount = std::numeric_limits<int>::max()) const;

    /**
     * Calculates CSimplifiedMNList for current list and cache it
     */
    gsl::not_null<std::shared_ptr<const CSimplifiedMNList>> to_sml() const;

    /**
     * Calculates the maximum penalty which is allowed at the height of this MN list. It is dynamic and might change
     * for every block.
     */
    [[nodiscard]] int CalcMaxPoSePenalty() const;

    /**
     * Returns a the given percentage from the max penalty for this MN list. Always use this method to calculate the
     * value later passed to PoSePunish. The percentage should be high enough to take per-block penalty decreasing for MNs
     * into account. This means, if you want to accept 2 failures per payment cycle, you should choose a percentage that
     * is higher then 50%, e.g. 66%.
     */
    [[nodiscard]] int CalcPenalty(int percent) const;

    /**
     * Punishes a MN for misbehavior. If the resulting penalty score of the MN reaches the max penalty, it is banned.
     * Penalty scores are only increased when the MN is not already banned, which means that after banning the penalty
     * might appear lower then the current max penalty, while the MN is still banned.
     */
    void PoSePunish(const uint256& proTxHash, int penalty, bool debugLogs);

    void DecreaseScores();
    /**
     * Decrease penalty score of MN by 1.
     * Only allowed on non-banned MNs.
     */
    void PoSeDecrease(const CDeterministicMN& dmn);

    [[nodiscard]] CDeterministicMNListDiff BuildDiff(const CDeterministicMNList& to) const;
    /**
     * Apply Diff modifies current object.
     * It is more efficient than creating a copy due to heavy copy constructor.
     * Calculating for old block may require up to {DISK_SNAPSHOT_PERIOD} object copy & destroy.
     */
    void ApplyDiff(gsl::not_null<const CBlockIndex*> pindex, const CDeterministicMNListDiff& diff);

    void AddMN(const CDeterministicMNCPtr& dmn, bool fBumpTotalCount = true);
    void UpdateMN(const CDeterministicMN& oldDmn, const std::shared_ptr<const CDeterministicMNState>& pdmnState);
    void UpdateMN(const uint256& proTxHash, const std::shared_ptr<const CDeterministicMNState>& pdmnState);
    void UpdateMN(const CDeterministicMN& oldDmn, const CDeterministicMNStateDiff& stateDiff);
    void RemoveMN(const uint256& proTxHash);

    template <typename T>
    [[nodiscard]] bool HasUniqueProperty(const T& v) const
    {
        return mnUniquePropertyMap.count(GetUniquePropertyHash(v)) != 0;
    }
    template <typename T>
    [[nodiscard]] CDeterministicMNCPtr GetUniquePropertyMN(const T& v) const
    {
        auto p = mnUniquePropertyMap.find(GetUniquePropertyHash(v));
        if (!p) {
            return nullptr;
        }
        return GetMN(p->first);
    }

private:
    template <typename T>
    [[nodiscard]] uint256 GetUniquePropertyHash(const T& v) const
    {
#define DMNL_NO_TEMPLATE(name) \
    static_assert(!std::is_same_v<std::decay_t<T>, name>, "GetUniquePropertyHash cannot be templated against " #name)
        DMNL_NO_TEMPLATE(CBLSPublicKey);
        DMNL_NO_TEMPLATE(MnNetInfo);
        DMNL_NO_TEMPLATE(NetInfoEntry);
        DMNL_NO_TEMPLATE(NetInfoInterface);
        DMNL_NO_TEMPLATE(std::shared_ptr<NetInfoInterface>);
#undef DMNL_NO_TEMPLATE
        return ::SerializeHash(v);
    }
    template <typename T>
    [[nodiscard]] bool AddUniqueProperty(const CDeterministicMN& dmn, const T& v)
    {
        static const T nullValue{};
        if (v == nullValue) {
            return false;
        }

        auto hash = GetUniquePropertyHash(v);
        auto oldEntry = mnUniquePropertyMap.find(hash);
        if (oldEntry != nullptr && oldEntry->first != dmn.proTxHash) {
            return false;
        }
        std::pair<uint256, uint32_t> newEntry(dmn.proTxHash, 1);
        if (oldEntry != nullptr) {
            newEntry.second = oldEntry->second + 1;
        }
        mnUniquePropertyMap = mnUniquePropertyMap.set(hash, newEntry);
        return true;
    }
    template <typename T>
    [[nodiscard]] bool DeleteUniqueProperty(const CDeterministicMN& dmn, const T& oldValue)
    {
        static const T nullValue{};
        if (oldValue == nullValue) {
            return false;
        }

        auto oldHash = GetUniquePropertyHash(oldValue);
        auto p = mnUniquePropertyMap.find(oldHash);
        if (p == nullptr || p->first != dmn.proTxHash) {
            return false;
        }
        if (p->second == 1) {
            mnUniquePropertyMap = mnUniquePropertyMap.erase(oldHash);
        } else {
            mnUniquePropertyMap = mnUniquePropertyMap.set(oldHash, std::make_pair(dmn.proTxHash, p->second - 1));
        }
        return true;
    }
    template <typename T>
    [[nodiscard]] bool UpdateUniqueProperty(const CDeterministicMN& dmn, const T& oldValue, const T& newValue)
    {
        if (oldValue == newValue) {
            return true;
        }
        static const T nullValue{};

        if (oldValue != nullValue && !DeleteUniqueProperty(dmn, oldValue)) {
            return false;
        }

        if (newValue != nullValue && !AddUniqueProperty(dmn, newValue)) {
            return false;
        }
        return true;
    }

    friend bool operator==(const CDeterministicMNList& a, const CDeterministicMNList& b)
    {
        return  a.blockHash == b.blockHash &&
                a.nHeight == b.nHeight &&
                a.nTotalRegisteredCount == b.nTotalRegisteredCount &&
                a.mnMap == b.mnMap &&
                a.mnInternalIdMap == b.mnInternalIdMap &&
                a.mnUniquePropertyMap == b.mnUniquePropertyMap;
    }
};

class CDeterministicMNListDiff
{
public:
    int nHeight{-1}; //memory only

    std::vector<CDeterministicMNCPtr> addedMNs;
    // keys are all relating to the internalId of MNs
    std::unordered_map<uint64_t, CDeterministicMNStateDiff> updatedMNs;
    std::set<uint64_t> removedMns;

    template<typename Stream>
    void Serialize(Stream& s) const
    {
        s << addedMNs;

        WriteCompactSize(s, updatedMNs.size());
        for (const auto& [internalId, pdmnState] : updatedMNs) {
            WriteVarInt<Stream, VarIntMode::DEFAULT, uint64_t>(s, internalId);
            s << pdmnState;
        }

        WriteCompactSize(s, removedMns.size());
        for (const auto& internalId : removedMns) {
            WriteVarInt<Stream, VarIntMode::DEFAULT, uint64_t>(s, internalId);
        }
    }

    template <typename Stream>
    void Unserialize(Stream& s)
    {
        updatedMNs.clear();
        removedMns.clear();

        for (size_t to_read = ReadCompactSize(s); to_read > 0; --to_read) {
            addedMNs.push_back(std::make_shared<CDeterministicMN>(deserialize, s));
        }

        for (size_t to_read = ReadCompactSize(s); to_read > 0; --to_read) {
            uint64_t internalId = ReadVarInt<Stream, VarIntMode::DEFAULT, uint64_t>(s);
            // CDeterministicMNState can have newer fields but doesn't need migration logic here as CDeterministicMNStateDiff
            // is always serialised using a bitmask and new fields have a new bit guide value, so we are good to continue.
            updatedMNs.emplace(internalId, CDeterministicMNStateDiff(deserialize, s));
        }

        for (size_t to_read = ReadCompactSize(s); to_read > 0; --to_read) {
            uint64_t internalId = ReadVarInt<Stream, VarIntMode::DEFAULT, uint64_t>(s);
            removedMns.emplace(internalId);
        }
    }

    bool HasChanges() const
    {
        return !addedMNs.empty() || !updatedMNs.empty() || !removedMns.empty();
    }
};


constexpr int llmq_max_blocks() {
    int max_blocks{0};
    for (const auto& llmq : Consensus::available_llmqs) {
        int blocks = (llmq.useRotation ? 1 : llmq.signingActiveQuorumCount) * llmq.dkgInterval;
        max_blocks = std::max(max_blocks, blocks);
    }
    return max_blocks;
}

struct MNListUpdates
{
    CDeterministicMNList old_list;
    CDeterministicMNList new_list;
    CDeterministicMNListDiff diff;
};

class CDeterministicMNManager
{
    static constexpr int DISK_SNAPSHOT_PERIOD = 576; // once per day
    // keep cache for enough disk snapshots to have all active quourms covered
    static constexpr int DISK_SNAPSHOTS = llmq_max_blocks() / DISK_SNAPSHOT_PERIOD + 1;
    static constexpr int LIST_DIFFS_CACHE_SIZE = DISK_SNAPSHOT_PERIOD * DISK_SNAPSHOTS;

private:
    Mutex cs;
    Mutex cs_cleanup;
    // We have performed CleanupCache() on this height.
    int did_cleanup GUARDED_BY(cs_cleanup) {0};

    // Main thread has indicated we should perform cleanup up to this height
    std::atomic<int> to_cleanup {0};

    CEvoDB& m_evoDb;

    std::unordered_map<uint256, CDeterministicMNList, StaticSaltedHasher> mnListsCache GUARDED_BY(cs);
    std::unordered_map<uint256, CDeterministicMNListDiff, StaticSaltedHasher> mnListDiffsCache GUARDED_BY(cs);
    const CBlockIndex* tipIndex GUARDED_BY(cs) {nullptr};
    const CBlockIndex* m_initial_snapshot_index GUARDED_BY(cs) {nullptr};

public:
    explicit CDeterministicMNManager(CEvoDB& evoDb) :
        m_evoDb(evoDb)
    {
    }
    ~CDeterministicMNManager() = default;

    bool ProcessBlock(const CBlock& block, gsl::not_null<const CBlockIndex*> pindex, BlockValidationState& state,
                      const CDeterministicMNList& newList, std::optional<MNListUpdates>& updatesRet)
        EXCLUSIVE_LOCKS_REQUIRED(!cs, ::cs_main);
    bool UndoBlock(gsl::not_null<const CBlockIndex*> pindex, std::optional<MNListUpdates>& updatesRet) EXCLUSIVE_LOCKS_REQUIRED(!cs);

    void UpdatedBlockTip(gsl::not_null<const CBlockIndex*> pindex) EXCLUSIVE_LOCKS_REQUIRED(!cs);

    CDeterministicMNList GetListForBlock(gsl::not_null<const CBlockIndex*> pindex) EXCLUSIVE_LOCKS_REQUIRED(!cs) {
        LOCK(cs);
        return GetListForBlockInternal(pindex);
    };
    CDeterministicMNList GetListAtChainTip() EXCLUSIVE_LOCKS_REQUIRED(!cs);

    // Test if given TX is a ProRegTx which also contains the collateral at index n
    static bool IsProTxWithCollateral(const CTransactionRef& tx, uint32_t n);

    void DoMaintenance() EXCLUSIVE_LOCKS_REQUIRED(!cs);

private:
    void CleanupCache(int nHeight) EXCLUSIVE_LOCKS_REQUIRED(cs);
    CDeterministicMNList GetListForBlockInternal(gsl::not_null<const CBlockIndex*> pindex) EXCLUSIVE_LOCKS_REQUIRED(cs);
};

bool CheckProRegTx(CDeterministicMNManager& dmnman, const CTransaction& tx, gsl::not_null<const CBlockIndex*> pindexPrev, TxValidationState& state, const CCoinsViewCache& view, bool check_sigs);
bool CheckProUpServTx(CDeterministicMNManager& dmnman, const CTransaction& tx, gsl::not_null<const CBlockIndex*> pindexPrev, TxValidationState& state, bool check_sigs);
bool CheckProUpRegTx(CDeterministicMNManager& dmnman, const CTransaction& tx, gsl::not_null<const CBlockIndex*> pindexPrev, TxValidationState& state, const CCoinsViewCache& view, bool check_sigs);
bool CheckProUpRevTx(CDeterministicMNManager& dmnman, const CTransaction& tx, gsl::not_null<const CBlockIndex*> pindexPrev, TxValidationState& state, bool check_sigs);

#endif // BITCOIN_EVO_DETERMINISTICMNS_H
