// Copyright (c) 2018-2020 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SYSCOIN_EVO_DETERMINISTICMNS_H
#define SYSCOIN_EVO_DETERMINISTICMNS_H

#include <arith_uint256.h>
#include <evo/evodb.h>
#include <evo/providertx.h>
#include <saltedhasher.h>
#include <sync.h>
#include <threadsafety.h>

#include <immer/map.hpp>

#include <unordered_map>
#include <unordered_set>
#include <script/standard.h>
class CBlock;
class CBlockIndex;
class BlockValidationState;
class CSimplifiedMNListDiff;
namespace llmq
{
    class CFinalCommitment;
    class CFinalCommitmentTxPayload;
} // namespace llmq

class CDeterministicMNState
{
private:
    int nPoSeBanHeight{-1};

    friend class CDeterministicMNStateDiff;

public:
    int nRegisteredHeight{-1};
    int nCollateralHeight{-1};
    int nLastPaidHeight{0};
    int nPoSePenalty{0};
    int nPoSeRevivedHeight{-1};
    uint16_t nRevocationReason{CProUpRevTx::REASON_NOT_SPECIFIED};

    // the block hash X blocks after registration, used in quorum calculations
    uint256 confirmedHash;
    // sha256(proTxHash, confirmedHash) to speed up quorum calculations
    // please note that this is NOT a double-sha256 hash
    uint256 confirmedHashWithProRegTxHash;

    CKeyID keyIDOwner;
    CBLSLazyPublicKey pubKeyOperator;
    CKeyID keyIDVoting;
    CService addr;
    CScript scriptPayout;
    CScript scriptOperatorPayout;

public:
    CDeterministicMNState() = default;
    explicit CDeterministicMNState(const CProRegTx& proTx)
    {
        keyIDOwner = proTx.keyIDOwner;
        pubKeyOperator.Set(proTx.pubKeyOperator);
        keyIDVoting = proTx.keyIDVoting;
        addr = proTx.addr;
        scriptPayout = proTx.scriptPayout;
    }
    template <typename Stream>
    CDeterministicMNState(deserialize_type, Stream& s)
    {
        s >> *this;
    }

    SERIALIZE_METHODS(CDeterministicMNState, obj)
    {
         READWRITE(obj.nRegisteredHeight, obj.nLastPaidHeight, obj.nPoSePenalty, obj.nPoSeRevivedHeight, obj.nPoSeBanHeight,
         obj.nRevocationReason, obj.confirmedHash, obj.confirmedHashWithProRegTxHash, obj.keyIDOwner, obj.pubKeyOperator, obj.keyIDVoting,
         obj.addr, obj.scriptPayout, obj.scriptOperatorPayout, obj.nCollateralHeight);
    }

    void ResetOperatorFields()
    {
        pubKeyOperator.Set(CBLSPublicKey());
        addr = CService();
        scriptOperatorPayout = CScript();
        nRevocationReason = CProUpRevTx::REASON_NOT_SPECIFIED;
    }
    void BanIfNotBanned(int height)
    {
        if (!IsBanned()) {
            nPoSeBanHeight = height;
        }
    }
    int GetBannedHeight() const
    {
        return nPoSeBanHeight;
    }
    bool IsBanned() const
    {
        return nPoSeBanHeight != -1;
    }
    void Revive(int nRevivedHeight)
    {
        nPoSePenalty = 0;
        nPoSeBanHeight = -1;
        nPoSeRevivedHeight = nRevivedHeight;
    }
    void UpdateConfirmedHash(const uint256& _proTxHash, const uint256& _confirmedHash)
    {
        confirmedHash = _confirmedHash;
        CSHA256 h;
        h.Write(_proTxHash.begin(), _proTxHash.size());
        h.Write(_confirmedHash.begin(), _confirmedHash.size());
        h.Finalize(confirmedHashWithProRegTxHash.begin());
    }

public:
    std::string ToString() const;
    void ToJson(UniValue& obj) const;
};
typedef std::shared_ptr<CDeterministicMNState> CDeterministicMNStatePtr;
typedef std::shared_ptr<const CDeterministicMNState> CDeterministicMNStateCPtr;

class CDeterministicMNStateDiff
{
public:
    enum Field : uint32_t {
        Field_nRegisteredHeight                 = 0x0001,
        Field_nLastPaidHeight                   = 0x0002,
        Field_nPoSePenalty                      = 0x0004,
        Field_nPoSeRevivedHeight                = 0x0008,
        Field_nPoSeBanHeight                    = 0x0010,
        Field_nRevocationReason                 = 0x0020,
        Field_confirmedHash                     = 0x0040,
        Field_confirmedHashWithProRegTxHash     = 0x0080,
        Field_keyIDOwner                        = 0x0100,
        Field_pubKeyOperator                    = 0x0200,
        Field_keyIDVoting                       = 0x0400,
        Field_addr                              = 0x0800,
        Field_scriptPayout                      = 0x1000,
        Field_scriptOperatorPayout              = 0x2000,
        Field_nCollateralHeight                 = 0x4000,
    };

#define DMN_STATE_DIFF_ALL_FIELDS \
    DMN_STATE_DIFF_LINE(nRegisteredHeight) \
    DMN_STATE_DIFF_LINE(nLastPaidHeight) \
    DMN_STATE_DIFF_LINE(nPoSePenalty) \
    DMN_STATE_DIFF_LINE(nPoSeRevivedHeight) \
    DMN_STATE_DIFF_LINE(nPoSeBanHeight) \
    DMN_STATE_DIFF_LINE(nRevocationReason) \
    DMN_STATE_DIFF_LINE(confirmedHash) \
    DMN_STATE_DIFF_LINE(confirmedHashWithProRegTxHash) \
    DMN_STATE_DIFF_LINE(keyIDOwner) \
    DMN_STATE_DIFF_LINE(pubKeyOperator) \
    DMN_STATE_DIFF_LINE(keyIDVoting) \
    DMN_STATE_DIFF_LINE(addr) \
    DMN_STATE_DIFF_LINE(scriptPayout) \
    DMN_STATE_DIFF_LINE(scriptOperatorPayout) \
    DMN_STATE_DIFF_LINE(nCollateralHeight)

public:
    uint32_t fields{0};
    // we reuse the state class, but only the members as noted by fields are valid
    CDeterministicMNState state;

public:
    CDeterministicMNStateDiff() = default;
    CDeterministicMNStateDiff(const CDeterministicMNState& a, const CDeterministicMNState& b)
    {
#define DMN_STATE_DIFF_LINE(f) if (a.f != b.f) { state.f = b.f; fields |= Field_##f; }
        DMN_STATE_DIFF_ALL_FIELDS
#undef DMN_STATE_DIFF_LINE
    }
    SERIALIZE_METHODS(CDeterministicMNStateDiff, obj) {
        READWRITE(VARINT(obj.fields));
        #define DMN_STATE_DIFF_LINE(f) if (obj.fields & Field_##f) READWRITE(obj.state.f);
        DMN_STATE_DIFF_ALL_FIELDS
        #undef DMN_STATE_DIFF_LINE
    }
    
    void ApplyToState(CDeterministicMNState& target) const
    {
#define DMN_STATE_DIFF_LINE(f) if (fields & Field_##f) target.f = state.f;
        DMN_STATE_DIFF_ALL_FIELDS
#undef DMN_STATE_DIFF_LINE
    }
};

class CDeterministicMN
{
private:
    uint64_t internalId{std::numeric_limits<uint64_t>::max()};

public:
    CDeterministicMN() = delete; // no default constructor, must specify internalId
    explicit CDeterministicMN(uint64_t _internalId) : internalId(_internalId)
    {
        // only non-initial values
        assert(_internalId != std::numeric_limits<uint64_t>::max());
    }

    template <typename Stream>
    CDeterministicMN(deserialize_type, Stream& s)
    {
        s >> *this;
    }

    uint256 proTxHash;
    COutPoint collateralOutpoint;
    uint16_t nOperatorReward;
    CDeterministicMNStateCPtr pdmnState;

public:
    SERIALIZE_METHODS(CDeterministicMN, obj) {
        READWRITE(obj.proTxHash, VARINT(obj.internalId), obj.collateralOutpoint, obj.nOperatorReward, obj.pdmnState);
    }

    uint64_t GetInternalId() const;

    std::string ToString() const;
    void ToJson(UniValue& obj) const;
};
typedef std::shared_ptr<const CDeterministicMN> CDeterministicMNCPtr;

class CDeterministicMNListDiff;
class CDeterministicMNList
{
public:
    typedef immer::map<uint256, CDeterministicMNCPtr> MnMap;
    typedef immer::map<uint64_t, uint256> MnInternalIdMap;
    typedef immer::map<uint256, std::pair<uint256, uint32_t> > MnUniquePropertyMap;

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
    }

public:
    size_t GetAllMNsCount() const
    {
        return mnMap.size();
    }

    size_t GetValidMNsCount() const
    {
        size_t count = 0;
        for (const auto& p : mnMap) {
            if (IsMNValid(p.second)) {
                count++;
            }
        }
        return count;
    }

    template <typename Callback>
    void ForEachMN(bool onlyValid, Callback&& cb) const
    {
        for (const auto& p : mnMap) {
            if (!onlyValid || IsMNValid(p.second)) {
                cb(p.second);
            }
        }
    }

public:
    const uint256& GetBlockHash() const
    {
        return blockHash;
    }
    void SetBlockHash(const uint256& _blockHash)
    {
        blockHash = _blockHash;
    }
    int GetHeight() const
    {
        return nHeight;
    }
    void SetHeight(int _height)
    {
        nHeight = _height;
    }
    uint32_t GetTotalRegisteredCount() const
    {
        return nTotalRegisteredCount;
    }

    bool IsMNValid(const uint256& proTxHash) const;
    bool IsMNPoSeBanned(const uint256& proTxHash) const;
    static bool IsMNValid(const CDeterministicMNCPtr& dmn);
    static bool IsMNPoSeBanned(const CDeterministicMNCPtr& dmn);

    bool HasMN(const uint256& proTxHash) const
    {
        return GetMN(proTxHash) != nullptr;
    }
    bool HasMNByCollateral(const COutPoint& collateralOutpoint) const
    {
        return GetMNByCollateral(collateralOutpoint) != nullptr;
    }
    bool HasValidMNByCollateral(const COutPoint& collateralOutpoint) const
    {
        return GetValidMNByCollateral(collateralOutpoint) != nullptr;
    }
    CDeterministicMNCPtr GetMN(const uint256& proTxHash) const;
    CDeterministicMNCPtr GetValidMN(const uint256& proTxHash) const;
    CDeterministicMNCPtr GetMNByOperatorKey(const CBLSPublicKey& pubKey);
    CDeterministicMNCPtr GetMNByCollateral(const COutPoint& collateralOutpoint) const;
    CDeterministicMNCPtr GetValidMNByCollateral(const COutPoint& collateralOutpoint) const;
    CDeterministicMNCPtr GetMNByService(const CService& service) const;
    CDeterministicMNCPtr GetMNByInternalId(uint64_t internalId) const;
    CDeterministicMNCPtr GetMNPayee() const;

    /**
     * Calculates the projected MN payees for the next *count* blocks. The result is not guaranteed to be correct
     * as PoSe banning might occur later
     * @param[in]   nCount max block count to find mn payees
     * @param[out]  result result vector of payees
     */
    void GetProjectedMNPayees(size_t nCount, std::vector<CDeterministicMNCPtr>& result) const;

    /**
     * Calculate a quorum based on the modifier. The resulting list is deterministically sorted by score
     * @param[in]   maxSize max size of quorums returned
     * @param[in]   modifier hash used as seed for quorum calculation
     * @param[out]  members return vector of MN's as candidates
     */
    void CalculateQuorum(size_t maxSize, const uint256& modifier, std::vector<CDeterministicMNCPtr> &members) const;
    void CalculateScores(const uint256& modifier, std::vector<std::pair<arith_uint256, CDeterministicMNCPtr>> &scores) const;

    /**
     * Calculates the maximum penalty which is allowed at the height of this MN list. It is dynamic and might change
     * for every block.
     */
    int CalcMaxPoSePenalty() const;

    /**
     * Returns a the given percentage from the max penalty for this MN list. Always use this method to calculate the
     * value later passed to PoSePunish. The percentage should be high enough to take per-block penalty decreasing for MNs
     * into account. This means, if you want to accept 2 failures per payment cycle, you should choose a percentage that
     * is higher then 50%, e.g. 66%.
     * @param[in]   percent penalty percentage
     */
    int CalcPenalty(int percent) const;

    /**
     * Punishes a MN for misbehavior. If the resulting penalty score of the MN reaches the max penalty, it is banned.
     * Penalty scores are only increased when the MN is not already banned, which means that after banning the penalty
     * might appear lower then the current max penalty, while the MN is still banned.
     * @param[in]   proTxHash MN hash to punish
     * @param[in]   penalty amount to punish
     * @param[in]   debugLogs log event or not
     */
    void PoSePunish(const uint256& proTxHash, int penalty, bool debugLogs);

    /**
     * Decrease penalty score of MN by 1.
     * Only allowed on non-banned MNs.
     * @param[in]   proTxHash MN hash to decrease punishment
     */
    void PoSeDecrease(const uint256& proTxHash);

    void BuildDiff(const CDeterministicMNList& to, CDeterministicMNListDiff& ret) const;
    CSimplifiedMNListDiff BuildSimplifiedDiff(const CDeterministicMNList& to) const;
    CDeterministicMNList ApplyDiff(const CBlockIndex* pindex, const CDeterministicMNListDiff& diff) const;

    void AddMN(const CDeterministicMNCPtr& dmn, bool fBumpTotalCount = true);
    void UpdateMN(const CDeterministicMNCPtr& oldDmn, const CDeterministicMNStateCPtr& pdmnState);
    void UpdateMN(const uint256& proTxHash, const CDeterministicMNStateCPtr& pdmnState);
    void UpdateMN(const CDeterministicMNCPtr& oldDmn, const CDeterministicMNStateDiff& stateDiff);
    void RemoveMN(const uint256& proTxHash);

    template <typename T>
    bool HasUniqueProperty(const T& v) const
    {
        return mnUniquePropertyMap.count(::SerializeHash(v)) != 0;
    }
    template <typename T>
    CDeterministicMNCPtr GetUniquePropertyMN(const T& v) const
    {
        auto p = mnUniquePropertyMap.find(::SerializeHash(v));
        if (!p) {
            return nullptr;
        }
        return GetMN(p->first);
    }

private:
    template <typename T>
    void AddUniqueProperty(const CDeterministicMNCPtr& dmn, const T& v)
    {
        static const T nullValue;
        assert(v != nullValue);

        auto hash = ::SerializeHash(v);
        auto oldEntry = mnUniquePropertyMap.find(hash);
        assert(!oldEntry || oldEntry->first == dmn->proTxHash);
        std::pair<uint256, uint32_t> newEntry(dmn->proTxHash, 1);
        if (oldEntry) {
            newEntry.second = oldEntry->second + 1;
        }
        mnUniquePropertyMap = mnUniquePropertyMap.set(hash, newEntry);
    }
    template <typename T>
    void DeleteUniqueProperty(const CDeterministicMNCPtr& dmn, const T& oldValue)
    {
        static const T nullValue;
        assert(oldValue != nullValue);

        auto oldHash = ::SerializeHash(oldValue);
        auto p = mnUniquePropertyMap.find(oldHash);
        assert(p && p->first == dmn->proTxHash);
        if (p->second == 1) {
            mnUniquePropertyMap = mnUniquePropertyMap.erase(oldHash);
        } else {
            mnUniquePropertyMap = mnUniquePropertyMap.set(oldHash, std::make_pair(dmn->proTxHash, p->second - 1));
        }
    }
    template <typename T>
    void UpdateUniqueProperty(const CDeterministicMNCPtr& dmn, const T& oldValue, const T& newValue)
    {
        if (oldValue == newValue) {
            return;
        }
        static const T nullValue;

        if (oldValue != nullValue) {
            DeleteUniqueProperty(dmn, oldValue);
        }

        if (newValue != nullValue) {
            AddUniqueProperty(dmn, newValue);
        }
    }
};

class CDeterministicMNListDiff
{
public:
    int nHeight{-1}; //memory only

    std::vector<CDeterministicMNCPtr> addedMNs;
    // keys are all relating to the internalId of MNs
    std::unordered_map<uint64_t, CDeterministicMNStateDiff> updatedMNs;
    std::unordered_set<uint64_t> removedMns;

public:
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

    template<typename Stream>
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
            tmp2 = ReadVarInt<Stream, VarIntMode::DEFAULT, uint64_t>(s);
            s >> diff;
            updatedMNs.try_emplace(tmp2, std::move(diff));
        }
        tmp = ReadCompactSize(s);
        for (size_t i = 0; i < tmp; i++) {
            tmp2 = ReadVarInt<Stream, VarIntMode::DEFAULT, uint64_t>(s);
            removedMns.emplace(tmp2);
        }
    }

public:
    bool HasChanges() const
    {
        return !addedMNs.empty() || !updatedMNs.empty() || !removedMns.empty();
    }
};

class CDeterministicMNManager
{
    static const int DISK_SNAPSHOT_PERIOD = 576; // once per day
    static const int DISK_SNAPSHOTS = 3; // keep cache for 3 disk snapshots to have 2 full days covered
    static const int LIST_DIFFS_CACHE_SIZE = DISK_SNAPSHOT_PERIOD * DISK_SNAPSHOTS;

public:
    mutable RecursiveMutex cs;

private:

    std::unordered_map<uint256, CDeterministicMNList, StaticSaltedHasher> mnListsCache;
    std::unordered_map<uint256, CDeterministicMNListDiff, StaticSaltedHasher> mnListDiffsCache;
    const CBlockIndex* tipIndex{};

public:
    explicit CDeterministicMNManager();

    bool ProcessBlock(const CBlock& block, const CBlockIndex* pindex, BlockValidationState& state, CCoinsViewCache& view, bool fJustCheck) EXCLUSIVE_LOCKS_REQUIRED(::cs_main);
    bool UndoBlock(const CBlock& block, const CBlockIndex* pindex) EXCLUSIVE_LOCKS_REQUIRED(::cs_main);

    void UpdatedBlockTip(const CBlockIndex* pindex);

    // the returned list will not contain the correct block hash (we can't know it yet as the coinbase TX is not updated yet)
    bool BuildNewListFromBlock(const CBlock& block, const CBlockIndex* pindexPrev, BlockValidationState& state, CCoinsViewCache& view, CDeterministicMNList& mnListRet, bool debugLogs, const llmq::CFinalCommitmentTxPayload *qcIn = nullptr) EXCLUSIVE_LOCKS_REQUIRED(cs_main, cs);
    static void HandleQuorumCommitment(const llmq::CFinalCommitment& qc, const CBlockIndex* pindexQuorum, CDeterministicMNList& mnList, bool debugLogs);
    static void DecreasePoSePenalties(CDeterministicMNList& mnList);

    void GetListForBlock(const CBlockIndex* pindex, CDeterministicMNList& result);
    void GetListAtChainTip(CDeterministicMNList& result);

    // Test if given TX is a ProRegTx which also contains the collateral at index n
    static bool IsProTxWithCollateral(const CTransactionRef& tx, uint32_t n);

    bool IsDIP3Enforced(int nHeight = -1);

private:
    void CleanupCache(int nHeight) EXCLUSIVE_LOCKS_REQUIRED(cs);
};

extern std::unique_ptr<CDeterministicMNManager> deterministicMNManager;

#endif //SYSCOIN_EVO_DETERMINISTICMNS_H
