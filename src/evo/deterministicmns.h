// Copyright (c) 2018-2022 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_EVO_DETERMINISTICMNS_H
#define BITCOIN_EVO_DETERMINISTICMNS_H

#include <evo/dmnstate.h>

#include <arith_uint256.h>
#include <consensus/params.h>
#include <crypto/common.h>
#include <evo/dmn_types.h>
#include <evo/evodb.h>
#include <evo/providertx.h>
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
class CChainState;
class CConnman;
class TxValidationState;

extern RecursiveMutex cs_main;

namespace llmq
{
    class CFinalCommitment;
} // namespace llmq

class CDeterministicMN
{
private:
    uint64_t internalId{std::numeric_limits<uint64_t>::max()};

public:
    static constexpr uint16_t MN_OLD_FORMAT = 0;
    static constexpr uint16_t MN_TYPE_FORMAT = 1;
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
    CDeterministicMN(deserialize_type, Stream& s, const uint8_t format_version)
    {
        SerializationOp(s, CSerActionUnserialize(), format_version);
    }

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, const uint8_t format_version)
    {
        READWRITE(proTxHash);
        READWRITE(VARINT(internalId));
        READWRITE(collateralOutpoint);
        READWRITE(nOperatorReward);
        // We need to read CDeterministicMNState using the old format only when called with MN_OLD_FORMAT or MN_TYPE_FORMAT on Unserialize()
        // Serialisation (writing) will be done always using new format
        if (ser_action.ForRead() && format_version == MN_OLD_FORMAT) {
            CDeterministicMNState_Oldformat old_state;
            READWRITE(old_state);
            pdmnState = std::make_shared<const CDeterministicMNState>(old_state);
        } else if (ser_action.ForRead() && format_version == MN_TYPE_FORMAT) {
            CDeterministicMNState_mntype_format old_state;
            READWRITE(old_state);
            pdmnState = std::make_shared<const CDeterministicMNState>(old_state);
        } else {
            READWRITE(pdmnState);
        }
        // We need to read/write nType if:
        // format_version is set to MN_TYPE_FORMAT (For writing (serialisation) it is always the case) Needed for the MNLISTDIFF Migration in evoDB
        // We can't know if we are serialising for the Disk or for the Network here (s.GetType() is not accessible)
        // Therefore if s.GetVersion() == CLIENT_VERSION -> Then we know we are serialising for the Disk
        // Otherwise, we can safely check with protocol versioning logic so we won't break old clients
        if (format_version >= MN_TYPE_FORMAT && (s.GetVersion() == CLIENT_VERSION || s.GetVersion() >= DMN_TYPE_PROTO_VERSION)) {
            READWRITE(nType);
        } else {
            nType = MnType::Regular;
        }
    }

    template<typename Stream>
    void Serialize(Stream& s) const
    {
        const_cast<CDeterministicMN*>(this)->SerializationOp(s, CSerActionSerialize(), MN_CURRENT_FORMAT);
    }

    template <typename Stream>
    void Unserialize(Stream& s, const uint8_t format_version = MN_CURRENT_FORMAT)
    {
        SerializationOp(s, CSerActionUnserialize(), format_version);
    }

    [[nodiscard]] uint64_t GetInternalId() const;

    [[nodiscard]] std::string ToString() const;
    void ToJson(UniValue& obj) const;
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
        for (const auto& p : mnMap) {
            s << *p.second;
        }
    }

    template<typename Stream>
    void Unserialize(Stream& s, const uint8_t format_version = CDeterministicMN::MN_CURRENT_FORMAT) {
        mnMap = MnMap();
        mnUniquePropertyMap = MnUniquePropertyMap();
        mnInternalIdMap = MnInternalIdMap();

        SerializationOpBase(s, CSerActionUnserialize());

        bool evodb_migration = (format_version == CDeterministicMN::MN_OLD_FORMAT || format_version == CDeterministicMN::MN_TYPE_FORMAT);
        size_t cnt = ReadCompactSize(s);
        for (size_t i = 0; i < cnt; i++) {
            if (evodb_migration) {
                const auto dmn = std::make_shared<CDeterministicMN>(deserialize, s, format_version);
                mnMap = mnMap.set(dmn->proTxHash, dmn);
            } else {
                AddMN(std::make_shared<CDeterministicMN>(deserialize, s, format_version), false);
            }
        }
    }

    [[nodiscard]] size_t GetAllMNsCount() const
    {
        return mnMap.size();
    }

    [[nodiscard]] size_t GetValidMNsCount() const
    {
        return ranges::count_if(mnMap, [this](const auto& p){ return IsMNValid(*p.second); });
    }

    [[nodiscard]] size_t GetAllEvoCount() const
    {
        return ranges::count_if(mnMap, [this](const auto& p) { return p.second->nType == MnType::Evo; });
    }

    [[nodiscard]] size_t GetValidEvoCount() const
    {
        return ranges::count_if(mnMap, [this](const auto& p) { return p.second->nType == MnType::Evo && IsMNValid(*p.second); });
    }

    [[nodiscard]] size_t GetValidWeightedMNsCount() const
    {
        return std::accumulate(mnMap.begin(), mnMap.end(), 0, [this](auto res, const auto& p) {
                                                                if (!IsMNValid(*p.second)) return res;
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
            if (!onlyValid || IsMNValid(*p.second)) {
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
            if (!onlyValid || IsMNValid(*p.second)) {
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
    static bool IsMNValid(const CDeterministicMN& dmn);
    static bool IsMNPoSeBanned(const CDeterministicMN& dmn);

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
    [[nodiscard]] CDeterministicMNCPtr GetMNPayee(const CBlockIndex* pIndex) const;

    /**
     * Calculates the projected MN payees for the next *count* blocks. The result is not guaranteed to be correct
     * as PoSe banning might occur later
     * @param nCount the number of payees to return. "nCount = max()"" means "all", use it to avoid calling GetValidWeightedMNsCount twice.
     * @return
     */
    [[nodiscard]] std::vector<CDeterministicMNCPtr> GetProjectedMNPayees(const CBlockIndex* const pindex, int nCount = std::numeric_limits<int>::max()) const;
    [[nodiscard]] std::vector<CDeterministicMNCPtr> GetProjectedMNPayeesAtChainTip(int nCount = std::numeric_limits<int>::max()) const;

    /**
     * Calculate a quorum based on the modifier. The resulting list is deterministically sorted by score
     * @param maxSize
     * @param modifier
     * @return
     */
    [[nodiscard]] std::vector<CDeterministicMNCPtr> CalculateQuorum(size_t maxSize, const uint256& modifier, const bool onlyEvoNodes = false) const;
    [[nodiscard]] std::vector<std::pair<arith_uint256, CDeterministicMNCPtr>> CalculateScores(const uint256& modifier, const bool onlyEvoNodes) const;

    /**
     * Calculates the maximum penalty which is allowed at the height of this MN list. It is dynamic and might change
     * for every block.
     * @return
     */
    [[nodiscard]] int CalcMaxPoSePenalty() const;

    /**
     * Returns a the given percentage from the max penalty for this MN list. Always use this method to calculate the
     * value later passed to PoSePunish. The percentage should be high enough to take per-block penalty decreasing for MNs
     * into account. This means, if you want to accept 2 failures per payment cycle, you should choose a percentage that
     * is higher then 50%, e.g. 66%.
     * @param percent
     * @return
     */
    [[nodiscard]] int CalcPenalty(int percent) const;

    /**
     * Punishes a MN for misbehavior. If the resulting penalty score of the MN reaches the max penalty, it is banned.
     * Penalty scores are only increased when the MN is not already banned, which means that after banning the penalty
     * might appear lower then the current max penalty, while the MN is still banned.
     * @param proTxHash
     * @param penalty
     */
    void PoSePunish(const uint256& proTxHash, int penalty, bool debugLogs);

    /**
     * Decrease penalty score of MN by 1.
     * Only allowed on non-banned MNs.
     * @param proTxHash
     */
    void PoSeDecrease(const uint256& proTxHash);

    [[nodiscard]] CDeterministicMNListDiff BuildDiff(const CDeterministicMNList& to) const;
    [[nodiscard]] CDeterministicMNList ApplyDiff(const CBlockIndex* pindex, const CDeterministicMNListDiff& diff) const;

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
        if constexpr (std::is_same<T, CBLSPublicKey>()) {
            assert(false);
        }
        return ::SerializeHash(v);
    }
    template <typename T>
    [[nodiscard]] bool AddUniqueProperty(const CDeterministicMN& dmn, const T& v)
    {
        static const T nullValue;
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
        static const T nullValue;
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
        static const T nullValue;

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
        for (const auto& p : updatedMNs) {
            WriteVarInt<Stream, VarIntMode::DEFAULT, uint64_t>(s, p.first);
            s << p.second;
        }
        WriteCompactSize(s, removedMns.size());
        for (const auto& p : removedMns) {
            WriteVarInt<Stream, VarIntMode::DEFAULT, uint64_t>(s, p);
        }
    }

    template <typename Stream>
    void Unserialize(Stream& s, const uint8_t format_version = CDeterministicMN::MN_CURRENT_FORMAT)
    {
        updatedMNs.clear();
        removedMns.clear();

        size_t tmp;
        uint64_t tmp2;
        tmp = ReadCompactSize(s);
        for (size_t i = 0; i < tmp; i++) {
            CDeterministicMN mn(0);
            mn.Unserialize(s, format_version);
            auto dmn = std::make_shared<CDeterministicMN>(mn);
            addedMNs.push_back(dmn);
        }
        tmp = ReadCompactSize(s);
        for (size_t i = 0; i < tmp; i++) {
            CDeterministicMNStateDiff diff;
            // CDeterministicMNState hold new fields {nConsecutivePayments, platformNodeID, platformP2PPort, platformHTTPPort} but no migration is needed here since:
            // CDeterministicMNStateDiff is always serialised using a bitmask.
            // Because the new field have a new bit guide value then we are good to continue
            tmp2 = ReadVarInt<Stream, VarIntMode::DEFAULT, uint64_t>(s);
            s >> diff;
            updatedMNs.emplace(tmp2, std::move(diff));
        }
        tmp = ReadCompactSize(s);
        for (size_t i = 0; i < tmp; i++) {
            tmp2 = ReadVarInt<Stream, VarIntMode::DEFAULT, uint64_t>(s);
            removedMns.emplace(tmp2);
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

class CDeterministicMNManager
{
    static constexpr int DISK_SNAPSHOT_PERIOD = 576; // once per day
    // keep cache for enough disk snapshots to have all active quourms covered
    static constexpr int DISK_SNAPSHOTS = llmq_max_blocks() / DISK_SNAPSHOT_PERIOD + 1;
    static constexpr int LIST_DIFFS_CACHE_SIZE = DISK_SNAPSHOT_PERIOD * DISK_SNAPSHOTS;

public:
    Mutex cs;

private:
    Mutex cs_cleanup;
    // We have performed CleanupCache() on this height.
    int did_cleanup GUARDED_BY(cs_cleanup) {0};

    // Main thread has indicated we should perform cleanup up to this height
    std::atomic<int> to_cleanup {0};

    CChainState& m_chainstate;
    CConnman& connman;
    CEvoDB& m_evoDb;

    std::unordered_map<uint256, CDeterministicMNList, StaticSaltedHasher> mnListsCache GUARDED_BY(cs);
    std::unordered_map<uint256, CDeterministicMNListDiff, StaticSaltedHasher> mnListDiffsCache GUARDED_BY(cs);
    const CBlockIndex* tipIndex GUARDED_BY(cs) {nullptr};
    const CBlockIndex* m_initial_snapshot_index GUARDED_BY(cs) {nullptr};

public:
    explicit CDeterministicMNManager(CChainState& chainstate, CConnman& _connman, CEvoDB& evoDb) :
        m_chainstate(chainstate), connman(_connman), m_evoDb(evoDb) {}
    ~CDeterministicMNManager() = default;

    bool ProcessBlock(const CBlock& block, const CBlockIndex* pindex, BlockValidationState& state,
                      const CCoinsViewCache& view, bool fJustCheck) EXCLUSIVE_LOCKS_REQUIRED(cs_main) LOCKS_EXCLUDED(cs);
    bool UndoBlock(const CBlockIndex* pindex) LOCKS_EXCLUDED(cs);

    void UpdatedBlockTip(const CBlockIndex* pindex) LOCKS_EXCLUDED(cs);

    // the returned list will not contain the correct block hash (we can't know it yet as the coinbase TX is not updated yet)
    bool BuildNewListFromBlock(const CBlock& block, const CBlockIndex* pindexPrev, BlockValidationState& state, const CCoinsViewCache& view,
                               CDeterministicMNList& mnListRet, bool debugLogs) EXCLUSIVE_LOCKS_REQUIRED(cs);
    static void HandleQuorumCommitment(const llmq::CFinalCommitment& qc, const CBlockIndex* pQuorumBaseBlockIndex, CDeterministicMNList& mnList, bool debugLogs);
    static void DecreasePoSePenalties(CDeterministicMNList& mnList);

    CDeterministicMNList GetListForBlock(const CBlockIndex* pindex) LOCKS_EXCLUDED(cs) {
        LOCK(cs);
        return GetListForBlockInternal(pindex);
    };
    CDeterministicMNList GetListAtChainTip() LOCKS_EXCLUDED(cs);

    // Test if given TX is a ProRegTx which also contains the collateral at index n
    static bool IsProTxWithCollateral(const CTransactionRef& tx, uint32_t n);

    bool IsDIP3Enforced(int nHeight = -1) LOCKS_EXCLUDED(cs);

    bool MigrateDBIfNeeded();
    bool MigrateDBIfNeeded2();

    void DoMaintenance() LOCKS_EXCLUDED(cs);

private:
    void CleanupCache(int nHeight) EXCLUSIVE_LOCKS_REQUIRED(cs);
    CDeterministicMNList GetListForBlockInternal(const CBlockIndex* pindex) EXCLUSIVE_LOCKS_REQUIRED(cs);
};

bool CheckProRegTx(const CTransaction& tx, const CBlockIndex* pindexPrev, TxValidationState& state, const CCoinsViewCache& view, bool check_sigs);
bool CheckProUpServTx(const CTransaction& tx, const CBlockIndex* pindexPrev, TxValidationState& state, bool check_sigs);
bool CheckProUpRegTx(const CTransaction& tx, const CBlockIndex* pindexPrev, TxValidationState& state, const CCoinsViewCache& view, bool check_sigs);
bool CheckProUpRevTx(const CTransaction& tx, const CBlockIndex* pindexPrev, TxValidationState& state, bool check_sigs);

extern std::unique_ptr<CDeterministicMNManager> deterministicMNManager;

#endif // BITCOIN_EVO_DETERMINISTICMNS_H
