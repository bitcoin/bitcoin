// Copyright (c) 2018 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef DASH_DETERMINISTICMNS_H
#define DASH_DETERMINISTICMNS_H

#include "arith_uint256.h"
#include "bls/bls.h"
#include "dbwrapper.h"
#include "evodb.h"
#include "providertx.h"
#include "simplifiedmns.h"
#include "sync.h"

#include "immer/map.hpp"
#include "immer/map_transient.hpp"

#include <map>

class CBlock;
class CBlockIndex;
class CValidationState;

namespace llmq
{
    class CFinalCommitment;
}

class CDeterministicMNState
{
public:
    int nRegisteredHeight{-1};
    int nLastPaidHeight{0};
    int nPoSePenalty{0};
    int nPoSeRevivedHeight{-1};
    int nPoSeBanHeight{-1};
    uint16_t nRevocationReason{CProUpRevTx::REASON_NOT_SPECIFIED};

    // the block hash X blocks after registration, used in quorum calculations
    uint256 confirmedHash;
    // sha256(proTxHash, confirmedHash) to speed up quorum calculations
    // please note that this is NOT a double-sha256 hash
    uint256 confirmedHashWithProRegTxHash;

    CKeyID keyIDOwner;
    CBLSPublicKey pubKeyOperator;
    CKeyID keyIDVoting;
    CService addr;
    CScript scriptPayout;
    CScript scriptOperatorPayout;

public:
    CDeterministicMNState() {}
    CDeterministicMNState(const CProRegTx& proTx)
    {
        keyIDOwner = proTx.keyIDOwner;
        pubKeyOperator = proTx.pubKeyOperator;
        keyIDVoting = proTx.keyIDVoting;
        addr = proTx.addr;
        scriptPayout = proTx.scriptPayout;
    }
    template <typename Stream>
    CDeterministicMNState(deserialize_type, Stream& s)
    {
        s >> *this;
    }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action)
    {
        READWRITE(nRegisteredHeight);
        READWRITE(nLastPaidHeight);
        READWRITE(nPoSePenalty);
        READWRITE(nPoSeRevivedHeight);
        READWRITE(nPoSeBanHeight);
        READWRITE(nRevocationReason);
        READWRITE(confirmedHash);
        READWRITE(confirmedHashWithProRegTxHash);
        READWRITE(keyIDOwner);
        READWRITE(pubKeyOperator);
        READWRITE(keyIDVoting);
        READWRITE(addr);
        READWRITE(*(CScriptBase*)(&scriptPayout));
        READWRITE(*(CScriptBase*)(&scriptOperatorPayout));
    }

    void ResetOperatorFields()
    {
        pubKeyOperator = CBLSPublicKey();
        addr = CService();
        scriptOperatorPayout = CScript();
        nRevocationReason = CProUpRevTx::REASON_NOT_SPECIFIED;
    }
    void BanIfNotBanned(int height)
    {
        if (nPoSeBanHeight == -1) {
            nPoSeBanHeight = height;
        }
    }
    void UpdateConfirmedHash(const uint256& _proTxHash, const uint256& _confirmedHash)
    {
        confirmedHash = _confirmedHash;
        CSHA256 h;
        h.Write(_proTxHash.begin(), _proTxHash.size());
        h.Write(_confirmedHash.begin(), _confirmedHash.size());
        h.Finalize(confirmedHashWithProRegTxHash.begin());
    }

    bool operator==(const CDeterministicMNState& rhs) const
    {
        return nRegisteredHeight == rhs.nRegisteredHeight &&
               nLastPaidHeight == rhs.nLastPaidHeight &&
               nPoSePenalty == rhs.nPoSePenalty &&
               nPoSeRevivedHeight == rhs.nPoSeRevivedHeight &&
               nPoSeBanHeight == rhs.nPoSeBanHeight &&
               nRevocationReason == rhs.nRevocationReason &&
               confirmedHash == rhs.confirmedHash &&
               confirmedHashWithProRegTxHash == rhs.confirmedHashWithProRegTxHash &&
               keyIDOwner == rhs.keyIDOwner &&
               pubKeyOperator == rhs.pubKeyOperator &&
               keyIDVoting == rhs.keyIDVoting &&
               addr == rhs.addr &&
               scriptPayout == rhs.scriptPayout &&
               scriptOperatorPayout == rhs.scriptOperatorPayout;
    }

    bool operator!=(const CDeterministicMNState& rhs) const
    {
        return !(rhs == *this);
    }

public:
    std::string ToString() const;
    void ToJson(UniValue& obj) const;
};
typedef std::shared_ptr<CDeterministicMNState> CDeterministicMNStatePtr;
typedef std::shared_ptr<const CDeterministicMNState> CDeterministicMNStateCPtr;

class CDeterministicMN
{
public:
    CDeterministicMN() {}
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
    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action)
    {
        READWRITE(proTxHash);
        READWRITE(collateralOutpoint);
        READWRITE(nOperatorReward);
        READWRITE(pdmnState);
    }

public:
    std::string ToString() const;
    void ToJson(UniValue& obj) const;
};
typedef std::shared_ptr<CDeterministicMN> CDeterministicMNPtr;
typedef std::shared_ptr<const CDeterministicMN> CDeterministicMNCPtr;

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

class CDeterministicMNList
{
public:
    typedef immer::map<uint256, CDeterministicMNCPtr> MnMap;
    typedef immer::map<uint256, std::pair<uint256, uint32_t> > MnUniquePropertyMap;

private:
    uint256 blockHash;
    int nHeight{-1};
    MnMap mnMap;

    // map of unique properties like address and keys
    // we keep track of this as checking for duplicates would otherwise be painfully slow
    // the entries in the map are ref counted as some properties might appear multiple times per MN (e.g. operator/owner keys)
    MnUniquePropertyMap mnUniquePropertyMap;

public:
    CDeterministicMNList() {}
    explicit CDeterministicMNList(const uint256& _blockHash, int _height) :
        blockHash(_blockHash),
        nHeight(_height)
    {
    }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action)
    {
        READWRITE(blockHash);
        READWRITE(nHeight);
        if (ser_action.ForRead()) {
            UnserializeImmerMap(s, mnMap);
            UnserializeImmerMap(s, mnUniquePropertyMap);
        } else {
            SerializeImmerMap(s, mnMap);
            SerializeImmerMap(s, mnUniquePropertyMap);
        }
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

    bool IsMNValid(const uint256& proTxHash) const;
    bool IsMNPoSeBanned(const uint256& proTxHash) const;
    bool IsMNValid(const CDeterministicMNCPtr& dmn) const;
    bool IsMNPoSeBanned(const CDeterministicMNCPtr& dmn) const;

    bool HasMN(const uint256& proTxHash) const
    {
        return GetMN(proTxHash) != nullptr;
    }
    CDeterministicMNCPtr GetMN(const uint256& proTxHash) const;
    CDeterministicMNCPtr GetValidMN(const uint256& proTxHash) const;
    CDeterministicMNCPtr GetMNByOperatorKey(const CBLSPublicKey& pubKey);
    CDeterministicMNCPtr GetMNByCollateral(const COutPoint& collateralOutpoint) const;
    CDeterministicMNCPtr GetMNPayee() const;

    /**
     * Calculates the projected MN payees for the next *count* blocks. The result is not guaranteed to be correct
     * as PoSe banning might occur later
     * @param count
     * @return
     */
    std::vector<CDeterministicMNCPtr> GetProjectedMNPayees(int nCount) const;

    /**
     * Calculate a quorum based on the modifier. The resulting list is deterministically sorted by score
     * @param maxSize
     * @param modifier
     * @return
     */
    std::vector<CDeterministicMNCPtr> CalculateQuorum(size_t maxSize, const uint256& modifier) const;
    std::vector<std::pair<arith_uint256, CDeterministicMNCPtr>> CalculateScores(const uint256& modifier) const;

    /**
     * Calculates the maximum penalty which is allowed at the height of this MN list. It is dynamic and might change
     * for every block.
     * @return
     */
    int CalcMaxPoSePenalty() const;

    /**
     * Returns a the given percentage from the max penalty for this MN list. Always use this method to calculate the
     * value later passed to PoSePunish. The percentage should be high enough to take per-block penalty decreasing for MNs
     * into account. This means, if you want to accept 2 failures per payment cycle, you should choose a percentage that
     * is higher then 50%, e.g. 66%.
     * @param percent
     * @return
     */
    int CalcPenalty(int percent) const;

    /**
     * Punishes a MN for misbehavior. If the resulting penalty score of the MN reaches the max penalty, it is banned.
     * Penalty scores are only increased when the MN is not already banned, which means that after banning the penalty
     * might appear lower then the current max penalty, while the MN is still banned.
     * @param proTxHash
     * @param penalty
     */
    void PoSePunish(const uint256& proTxHash, int penalty);

    /**
     * Decrease penalty score of MN by 1.
     * Only allowed on non-banned MNs.
     * @param proTxHash
     */
    void PoSeDecrease(const uint256& proTxHash);

    CDeterministicMNListDiff BuildDiff(const CDeterministicMNList& to) const;
    CSimplifiedMNListDiff BuildSimplifiedDiff(const CDeterministicMNList& to) const;
    CDeterministicMNList ApplyDiff(const CDeterministicMNListDiff& diff) const;

    void AddMN(const CDeterministicMNCPtr& dmn);
    void UpdateMN(const uint256& proTxHash, const CDeterministicMNStateCPtr& pdmnState);
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
        DeleteUniqueProperty(dmn, oldValue);
        AddUniqueProperty(dmn, newValue);
    }
};

class CDeterministicMNListDiff
{
public:
    uint256 prevBlockHash;
    uint256 blockHash;
    int nHeight{-1};
    std::map<uint256, CDeterministicMNCPtr> addedMNs;
    std::map<uint256, CDeterministicMNStateCPtr> updatedMNs;
    std::set<uint256> removedMns;

public:
    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action)
    {
        READWRITE(prevBlockHash);
        READWRITE(blockHash);
        READWRITE(nHeight);
        READWRITE(addedMNs);
        READWRITE(updatedMNs);
        READWRITE(removedMns);
    }

public:
    bool HasChanges() const
    {
        return !addedMNs.empty() || !updatedMNs.empty() || !removedMns.empty();
    }
};

class CDeterministicMNManager
{
    static const int SNAPSHOT_LIST_PERIOD = 576; // once per day
    static const int LISTS_CACHE_SIZE = 576;

public:
    CCriticalSection cs;

private:
    CEvoDB& evoDb;

    std::map<uint256, CDeterministicMNList> mnListsCache;
    int tipHeight{-1};
    uint256 tipBlockHash;

public:
    CDeterministicMNManager(CEvoDB& _evoDb);

    bool ProcessBlock(const CBlock& block, const CBlockIndex* pindexPrev, CValidationState& state);
    bool UndoBlock(const CBlock& block, const CBlockIndex* pindex);

    void UpdatedBlockTip(const CBlockIndex* pindex);

    // the returned list will not contain the correct block hash (we can't know it yet as the coinbase TX is not updated yet)
    bool BuildNewListFromBlock(const CBlock& block, const CBlockIndex* pindexPrev, CValidationState& state, CDeterministicMNList& mnListRet);
    void HandleQuorumCommitment(llmq::CFinalCommitment& qc, CDeterministicMNList& mnList);
    void DecreasePoSePenalties(CDeterministicMNList& mnList);

    CDeterministicMNList GetListForBlock(const uint256& blockHash);
    CDeterministicMNList GetListAtChainTip();

    // TODO remove after removal of old non-deterministic lists
    bool HasValidMNCollateralAtChainTip(const COutPoint& outpoint);
    bool HasMNCollateralAtChainTip(const COutPoint& outpoint);

    // Test if given TX is a ProRegTx which also contains the collateral at index n
    bool IsProTxWithCollateral(const CTransactionRef& tx, uint32_t n);

    bool IsDeterministicMNsSporkActive(int nHeight = -1);

private:
    int64_t GetSpork15Value();
    void CleanupCache(int nHeight);
};

extern CDeterministicMNManager* deterministicMNManager;

#endif //DASH_DETERMINISTICMNS_H
