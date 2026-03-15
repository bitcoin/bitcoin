// Copyright (c) 2018-2023 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SYSCOIN_EVO_DETERMINISTICMNS_H
#define SYSCOIN_EVO_DETERMINISTICMNS_H

#include <evo/dmnstate.h>
#include <arith_uint256.h>
#include <consensus/params.h>
#include <crypto/common.h>
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
#include <unordered_set>
#include <utility>
#include <interfaces/chain.h>
class CBlock;
class UniValue;
class CBlockIndex;
class TxValidationState;
class ChainstateManager;
namespace llmq
{
    class CFinalCommitmentTxPayload;
    class CFinalCommitment;
} // namespace llmq

class CDeterministicMN
{
private:
    uint64_t internalId{std::numeric_limits<uint64_t>::max()};

public:
    uint256 proTxHash;
    COutPoint collateralOutpoint;
    uint16_t nOperatorReward{0};
    std::shared_ptr<const CDeterministicMNState> pdmnState;

    CDeterministicMN() = delete; // no default constructor, must specify internalId
    explicit CDeterministicMN(uint64_t _internalId) :
        internalId(_internalId)
    {
        // only non-initial values
        assert(_internalId != std::numeric_limits<uint64_t>::max());
    }

    template <typename Stream>
    CDeterministicMN(deserialize_type, Stream& s)
    {
        s >> *this;
    }

public:
    SERIALIZE_METHODS(CDeterministicMN, obj) {
        READWRITE(obj.proTxHash, VARINT(obj.internalId), obj.collateralOutpoint, obj.nOperatorReward, obj.pdmnState);
    }

    [[nodiscard]] uint64_t GetInternalId() const;

    [[nodiscard]] std::string ToString() const;
    void ToJson(interfaces::Chain& chain, UniValue& obj) const;
};
using CDeterministicMNCPtr = std::shared_ptr<const CDeterministicMN>;

class CDeterministicMNListDiff;
class CDeterministicMNListNEVMAddressDiff;


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
    bool m_changed_nevm_address{false};
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

    template<typename Stream>
    void Serialize(Stream& s) const
    {
        s << blockHash;
        s << nHeight;
        s << nTotalRegisteredCount;
        // Serialize the map as a vector
        WriteCompactSize(s, mnMap.size());
        for (const auto& p : mnMap) {
            s << *p.second;
        }
    }

    template<typename Stream>
    void Unserialize(Stream& s) {
        mnMap = MnMap();
        mnUniquePropertyMap = MnUniquePropertyMap();
        mnInternalIdMap = MnInternalIdMap();
        s >> blockHash;
        s >> nHeight;
        s >> nTotalRegisteredCount;

        size_t cnt = ReadCompactSize(s);
        for (size_t i = 0; i < cnt; i++) {
            AddMN(std::make_shared<CDeterministicMN>(deserialize, s), false);
        }
    }
    void clear() {
        mnMap = MnMap();
        mnUniquePropertyMap = MnUniquePropertyMap();
        mnInternalIdMap = MnInternalIdMap();
        blockHash.SetNull();
        nHeight = -1;
        nTotalRegisteredCount = 0;
        m_changed_nevm_address = false;
    }
    [[nodiscard]] size_t GetAllMNsCount() const
    {
        return mnMap.size();
    }

    [[nodiscard]] size_t GetValidMNsCount() const
    {
        return ranges::count_if(mnMap, [](const auto& p){ return IsMNValid(*p.second); });
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
    [[nodiscard]] CDeterministicMNCPtr GetMNPayee() const;

    /**
     * Calculates the projected MN payees for the next *count* blocks. The result is not guaranteed to be correct
     * as PoSe banning might occur later
     * @param nCount the number of payees to return. "nCount = max()"" means "all", use it to avoid calling GetValidMNsCount twice.
     * @return
     */
    [[nodiscard]] std::vector<CDeterministicMNCPtr> GetProjectedMNPayees(int nCount = std::numeric_limits<int>::max()) const;

    /**
     * Calculate a quorum based on the modifier. The resulting list is deterministically sorted by score
     * @param maxSize
     * @param modifier
     * @return
     */
    [[nodiscard]] std::vector<CDeterministicMNCPtr> CalculateQuorum(size_t maxSize, const uint256& modifier) const;
    [[nodiscard]] std::vector<std::pair<arith_uint256, CDeterministicMNCPtr>> CalculateScores(const uint256& modifier) const;

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
    void PoSePunish(const uint256& proTxHash, int penalty);

    void DecreaseScores();
    /**
     * Decrease penalty score of MN by 1.
     * Only allowed on non-banned MNs.
     */
    void PoSeDecrease(const CDeterministicMN& dmn);

    void BuildDiff(const CDeterministicMNList& to, CDeterministicMNListDiff &diffRet, CDeterministicMNListNEVMAddressDiff &diffRetNEVMAddress) const;
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
        static_assert(!std::is_same<T, CBLSPublicKey>(), "GetUniquePropertyHash cannot be templated against CBLSPublicKey");
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


class CDeterministicMNListNEVMAddressDiff
{
public:

    std::vector<std::pair<std::vector<unsigned char>, uint32_t>> addedMNNEVM;
    std::vector<std::pair<std::vector<unsigned char>, std::pair<std::vector<unsigned char>, uint32_t>>> updatedMNNEVM;
    std::vector<std::vector<unsigned char>> removedMNNEVM;

    SERIALIZE_METHODS(CDeterministicMNListNEVMAddressDiff, obj) {
        READWRITE(obj.addedMNNEVM, obj.updatedMNNEVM, obj.removedMNNEVM);
    }
    std::string ToString() const;
};
// Temporary map to collect and deduplicate NEVM address changes.
// Keyed by masternode proTxHash.
enum class NEVMDiffType {
    None,
    Added,
    Updated,
    Removed
};

struct NEVMDiffEntry {
    NEVMDiffType type = NEVMDiffType::None;
    // For an update, both addresses are set;
    // For an add, only newAddress and collateral height are relevant;
    // For a removal, only oldAddress is needed.
    std::vector<unsigned char> oldAddress;
    std::vector<unsigned char> newAddress;
    uint32_t collateralHeight = 0;
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
    void Unserialize(Stream& s)
    {
        updatedMNs.clear();
        removedMns.clear();

        size_t tmp;
        uint64_t tmp2;
        s >> addedMNs;
        tmp = ReadCompactSize(s);
        for (size_t i = 0; i < tmp; i++) {
            CDeterministicMNStateDiff diff;
            // CDeterministicMNState holds a new field {nVersion} but no migration is needed here since:
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
class CDeterministicMNManager
{
    static constexpr int DISK_SNAPSHOT_PERIOD = 576; // once per day
    static constexpr int DISK_SNAPSHOTS = 3; // keep cache for 3 disk snapshots to have 2 full days covered
    static constexpr int LIST_CACHE_SIZE = DISK_SNAPSHOT_PERIOD * DISK_SNAPSHOTS;

private:
    Mutex cs;
    // Main thread has indicated we should perform cleanup up to this height
    std::atomic<int> to_cleanup {0};

    const CBlockIndex* tipIndex GUARDED_BY(cs) {nullptr};
public:
    struct EvoDBStats {
        int64_t approxPersistedEntries{0};
        uint64_t estimatedDiskSizeBytes{0};
        size_t cacheEntries{0};
        size_t eraseCacheEntries{0};
        std::string dbPath;
    };
    std::unique_ptr<CEvoDB<uint256, CDeterministicMNList, StaticSaltedHasher>> m_evoDb;
    explicit CDeterministicMNManager(const DBParams& db_params)
        : m_evoDb(std::make_unique<CEvoDB<uint256, CDeterministicMNList, StaticSaltedHasher>>(db_params, LIST_CACHE_SIZE)) {}
       
    ~CDeterministicMNManager() = default;

    bool ProcessBlock(const CBlock& block, const CBlockIndex* pindex, BlockValidationState& state,
                      const CCoinsViewCache& view, const llmq::CFinalCommitmentTxPayload &qcTx, CDeterministicMNListNEVMAddressDiff &diff, bool fJustCheck, bool ibd) EXCLUSIVE_LOCKS_REQUIRED(!cs, cs_main);
    bool UndoBlock(const CBlockIndex* pindex, CDeterministicMNListNEVMAddressDiff &inversedDiffNEVMAddress) EXCLUSIVE_LOCKS_REQUIRED(!cs, cs_main);

    // the returned list will not contain the correct block hash (we can't know it yet as the coinbase TX is not updated yet)
    bool BuildNewListFromBlock(const CBlock& block, const CBlockIndex* pindexPrev, BlockValidationState& state, const CCoinsViewCache& view,
                                CDeterministicMNList& mnListRet, CDeterministicMNList& mnOldListRet, const llmq::CFinalCommitmentTxPayload &qcTx) EXCLUSIVE_LOCKS_REQUIRED(!cs);
    void HandleQuorumCommitment(const llmq::CFinalCommitment& qc, const CBlockIndex* pQuorumBaseBlockIndex, CDeterministicMNList& mnList);
    static void DecreasePoSePenalties(CDeterministicMNList& mnList, const std::vector<CDeterministicMNCPtr> &toDecrease);

    const CDeterministicMNList GetListForBlock(const CBlockIndex* pindex) EXCLUSIVE_LOCKS_REQUIRED(!cs);
    void GetListForBlock(const CBlockIndex* pindex, CDeterministicMNList& list);
    const CDeterministicMNList GetListAtChainTip() EXCLUSIVE_LOCKS_REQUIRED(!cs);

    // Test if given TX is a ProRegTx which also contains the collateral at index n
    static bool IsProTxWithCollateral(const CTransactionRef& tx, uint32_t n);
    bool IsDIP3Enforced(int nHeight = -1) EXCLUSIVE_LOCKS_REQUIRED(!cs);
    bool FlushCacheToDisk(bool bForceFlush) EXCLUSIVE_LOCKS_REQUIRED(!cs);
    bool DoMaintenance(bool bForceFlush) EXCLUSIVE_LOCKS_REQUIRED(!cs);
    void UpdatedBlockTip(const CBlockIndex* pindex) EXCLUSIVE_LOCKS_REQUIRED(!cs);
    bool GetEvoDBStats(EvoDBStats& stats) EXCLUSIVE_LOCKS_REQUIRED(!cs);
private:
    const CDeterministicMNList GetListForBlockInternal(const CBlockIndex* pindex) EXCLUSIVE_LOCKS_REQUIRED(!cs);
};
extern int64_t DEFAULT_MAX_RECOVERED_SIGS_AGE; // keep them for a week
extern std::unique_ptr<CDeterministicMNManager> deterministicMNManager;
extern bool fMasternodeMode;
#endif // SYSCOIN_EVO_DETERMINISTICMNS_H
