// Copyright (c) 2021-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_WALLET_TRANSACTION_H
#define BITCOIN_WALLET_TRANSACTION_H

#include <attributes.h>
#include <consensus/amount.h>
#include <primitives/transaction.h>
#include <tinyformat.h>
#include <uint256.h>
#include <util/overloaded.h>
#include <util/strencodings.h>
#include <util/string.h>
#include <wallet/types.h>

#include <bitset>
#include <cstdint>
#include <map>
#include <utility>
#include <variant>
#include <vector>

namespace interfaces {
class Chain;
} // namespace interfaces

namespace wallet {
//! State of transaction confirmed in a block.
struct TxStateConfirmed {
    uint256 confirmed_block_hash;
    int confirmed_block_height;
    int position_in_block;

    explicit TxStateConfirmed(const uint256& block_hash, int height, int index) : confirmed_block_hash(block_hash), confirmed_block_height(height), position_in_block(index) {}
    std::string toString() const { return strprintf("Confirmed (block=%s, height=%i, index=%i)", confirmed_block_hash.ToString(), confirmed_block_height, position_in_block); }
};

//! State of transaction added to mempool.
struct TxStateInMempool {
    std::string toString() const { return strprintf("InMempool"); }
};

//! State of rejected transaction that conflicts with a confirmed block.
struct TxStateBlockConflicted {
    uint256 conflicting_block_hash;
    int conflicting_block_height;

    explicit TxStateBlockConflicted(const uint256& block_hash, int height) : conflicting_block_hash(block_hash), conflicting_block_height(height) {}
    std::string toString() const { return strprintf("BlockConflicted (block=%s, height=%i)", conflicting_block_hash.ToString(), conflicting_block_height); }
};

//! State of transaction not confirmed or conflicting with a known block and
//! not in the mempool. May conflict with the mempool, or with an unknown block,
//! or be abandoned, never broadcast, or rejected from the mempool for another
//! reason.
struct TxStateInactive {
    bool abandoned;

    explicit TxStateInactive(bool abandoned = false) : abandoned(abandoned) {}
    std::string toString() const { return strprintf("Inactive (abandoned=%i)", abandoned); }
};

//! State of transaction loaded in an unrecognized state with unexpected hash or
//! index values. Treated as inactive (with serialized hash and index values
//! preserved) by default, but may enter another state if transaction is added
//! to the mempool, or confirmed, or abandoned, or found conflicting.
struct TxStateUnrecognized {
    uint256 block_hash;
    int index;

    TxStateUnrecognized(const uint256& block_hash, int index) : block_hash(block_hash), index(index) {}
    std::string toString() const { return strprintf("Unrecognized (block=%s, index=%i)", block_hash.ToString(), index); }
};

//! All possible CWalletTx states
using TxState = std::variant<TxStateConfirmed, TxStateInMempool, TxStateBlockConflicted, TxStateInactive, TxStateUnrecognized>;

//! Subset of states transaction sync logic is implemented to handle.
using SyncTxState = std::variant<TxStateConfirmed, TxStateInMempool, TxStateInactive>;

//! Try to interpret deserialized TxStateUnrecognized data as a recognized state.
static inline TxState TxStateInterpretSerialized(TxStateUnrecognized data)
{
    if (data.block_hash == uint256::ZERO) {
        if (data.index == 0) return TxStateInactive{};
    } else if (data.block_hash == uint256::ONE) {
        if (data.index == -1) return TxStateInactive{/*abandoned=*/true};
    } else if (data.index >= 0) {
        return TxStateConfirmed{data.block_hash, /*height=*/-1, data.index};
    } else if (data.index == -1) {
        return TxStateBlockConflicted{data.block_hash, /*height=*/-1};
    }
    return data;
}

//! Get TxState serialized block hash. Inverse of TxStateInterpretSerialized.
static inline uint256 TxStateSerializedBlockHash(const TxState& state)
{
    return std::visit(util::Overloaded{
        [](const TxStateInactive& inactive) { return inactive.abandoned ? uint256::ONE : uint256::ZERO; },
        [](const TxStateInMempool& in_mempool) { return uint256::ZERO; },
        [](const TxStateConfirmed& confirmed) { return confirmed.confirmed_block_hash; },
        [](const TxStateBlockConflicted& conflicted) { return conflicted.conflicting_block_hash; },
        [](const TxStateUnrecognized& unrecognized) { return unrecognized.block_hash; }
    }, state);
}

//! Get TxState serialized block index. Inverse of TxStateInterpretSerialized.
static inline int TxStateSerializedIndex(const TxState& state)
{
    return std::visit(util::Overloaded{
        [](const TxStateInactive& inactive) { return inactive.abandoned ? -1 : 0; },
        [](const TxStateInMempool& in_mempool) { return 0; },
        [](const TxStateConfirmed& confirmed) { return confirmed.position_in_block; },
        [](const TxStateBlockConflicted& conflicted) { return -1; },
        [](const TxStateUnrecognized& unrecognized) { return unrecognized.index; }
    }, state);
}

//! Return TxState or SyncTxState as a string for logging or debugging.
template<typename T>
std::string TxStateString(const T& state)
{
    return std::visit([](const auto& s) { return s.toString(); }, state);
}

/**
 * Cachable amount subdivided into watchonly and spendable parts.
 */
struct CachableAmount
{
    // NO and ALL are never (supposed to be) cached
    std::bitset<ISMINE_ENUM_ELEMENTS> m_cached;
    CAmount m_value[ISMINE_ENUM_ELEMENTS];
    inline void Reset()
    {
        m_cached.reset();
    }
    void Set(isminefilter filter, CAmount value)
    {
        m_cached.set(filter);
        m_value[filter] = value;
    }
};


typedef std::map<std::string, std::string> mapValue_t;


/** Legacy class used for deserializing vtxPrev for backwards compatibility.
 * vtxPrev was removed in commit 93a18a3650292afbb441a47d1fa1b94aeb0164e3,
 * but old wallet.dat files may still contain vtxPrev vectors of CMerkleTxs.
 * These need to get deserialized for field alignment when deserializing
 * a CWalletTx, but the deserialized values are discarded.**/
class CMerkleTx
{
public:
    template<typename Stream>
    void Unserialize(Stream& s)
    {
        CTransactionRef tx;
        uint256 hashBlock;
        std::vector<uint256> vMerkleBranch;
        int nIndex;

        s >> TX_WITH_WITNESS(tx) >> hashBlock >> vMerkleBranch >> nIndex;
    }
};

/**
 * A transaction with a bunch of additional info that only the owner cares about.
 * It includes any unrecorded transactions needed to link it back to the block chain.
 */
class CWalletTx
{
public:
    /**
     * Key/value map with information about the transaction.
     *
     * The following keys can be read and written through the map and are
     * serialized in the wallet database:
     *
     *     "comment", "to"   - comment strings provided to sendtoaddress,
     *                         and sendmany wallet RPCs
     *     "replaces_txid"   - txid (as HexStr) of transaction replaced by
     *                         bumpfee on transaction created by bumpfee
     *     "replaced_by_txid" - txid (as HexStr) of transaction created by
     *                         bumpfee on transaction replaced by bumpfee
     *     "from", "message" - obsolete fields that could be set in UI prior to
     *                         2011 (removed in commit 4d9b223)
     *
     * The following keys are serialized in the wallet database, but shouldn't
     * be read or written through the map (they will be temporarily added and
     * removed from the map during serialization):
     *
     *     "fromaccount"     - serialized strFromAccount value
     *     "n"               - serialized nOrderPos value
     *     "timesmart"       - serialized nTimeSmart value
     *     "spent"           - serialized vfSpent value that existed prior to
     *                         2014 (removed in commit 93a18a3)
     */
    mapValue_t mapValue;
    std::vector<std::pair<std::string, std::string> > vOrderForm;
    unsigned int nTimeReceived; //!< time received by this node
    /**
     * Stable timestamp that never changes, and reflects the order a transaction
     * was added to the wallet. Timestamp is based on the block time for a
     * transaction added as part of a block, or else the time when the
     * transaction was received if it wasn't part of a block, with the timestamp
     * adjusted in both cases so timestamp order matches the order transactions
     * were added to the wallet. More details can be found in
     * CWallet::ComputeTimeSmart().
     */
    unsigned int nTimeSmart;
    int64_t nOrderPos; //!< position in ordered transaction list
    std::multimap<int64_t, CWalletTx*>::const_iterator m_it_wtxOrdered;

    // memory only
    enum AmountType { DEBIT, CREDIT, IMMATURE_CREDIT, AVAILABLE_CREDIT, AMOUNTTYPE_ENUM_ELEMENTS };
    mutable CachableAmount m_amounts[AMOUNTTYPE_ENUM_ELEMENTS];
    /**
     * This flag is true if all m_amounts caches are empty. This is particularly
     * useful in places where MarkDirty is conditionally called and the
     * condition can be expensive and thus can be skipped if the flag is true.
     * See MarkDestinationsDirty.
     */
    mutable bool m_is_cache_empty{true};
    mutable bool fChangeCached;
    mutable CAmount nChangeCached;

    CWalletTx(CTransactionRef tx, const TxState& state) : tx(std::move(tx)), m_state(state)
    {
        Init();
    }

    void Init()
    {
        mapValue.clear();
        vOrderForm.clear();
        nTimeReceived = 0;
        nTimeSmart = 0;
        fChangeCached = false;
        nChangeCached = 0;
        nOrderPos = -1;
    }

    CTransactionRef tx;
    TxState m_state;

    // Set of mempool transactions that conflict
    // directly with the transaction, or that conflict
    // with an ancestor transaction. This set will be
    // empty if state is InMempool or Confirmed, but
    // can be nonempty if state is Inactive or
    // BlockConflicted.
    std::set<Txid> mempool_conflicts;

    template<typename Stream>
    void Serialize(Stream& s) const
    {
        mapValue_t mapValueCopy = mapValue;

        mapValueCopy["fromaccount"] = "";
        if (nOrderPos != -1) {
            mapValueCopy["n"] = util::ToString(nOrderPos);
        }
        if (nTimeSmart) {
            mapValueCopy["timesmart"] = strprintf("%u", nTimeSmart);
        }

        std::vector<uint8_t> dummy_vector1; //!< Used to be vMerkleBranch
        std::vector<uint8_t> dummy_vector2; //!< Used to be vtxPrev
        bool dummy_bool = false; //!< Used to be fFromMe, and fSpent
        uint32_t dummy_int = 0; // Used to be fTimeReceivedIsTxTime
        uint256 serializedHash = TxStateSerializedBlockHash(m_state);
        int serializedIndex = TxStateSerializedIndex(m_state);
        s << TX_WITH_WITNESS(tx) << serializedHash << dummy_vector1 << serializedIndex << dummy_vector2 << mapValueCopy << vOrderForm << dummy_int << nTimeReceived << dummy_bool << dummy_bool;
    }

    template<typename Stream>
    void Unserialize(Stream& s)
    {
        Init();

        std::vector<uint256> dummy_vector1; //!< Used to be vMerkleBranch
        std::vector<CMerkleTx> dummy_vector2; //!< Used to be vtxPrev
        bool dummy_bool; //! Used to be fFromMe, and fSpent
        uint32_t dummy_int; // Used to be fTimeReceivedIsTxTime
        uint256 serialized_block_hash;
        int serializedIndex;
        s >> TX_WITH_WITNESS(tx) >> serialized_block_hash >> dummy_vector1 >> serializedIndex >> dummy_vector2 >> mapValue >> vOrderForm >> dummy_int >> nTimeReceived >> dummy_bool >> dummy_bool;

        m_state = TxStateInterpretSerialized({serialized_block_hash, serializedIndex});

        const auto it_op = mapValue.find("n");
        nOrderPos = (it_op != mapValue.end()) ? LocaleIndependentAtoi<int64_t>(it_op->second) : -1;
        const auto it_ts = mapValue.find("timesmart");
        nTimeSmart = (it_ts != mapValue.end()) ? static_cast<unsigned int>(LocaleIndependentAtoi<int64_t>(it_ts->second)) : 0;

        mapValue.erase("fromaccount");
        mapValue.erase("spent");
        mapValue.erase("n");
        mapValue.erase("timesmart");
    }

    void SetTx(CTransactionRef arg)
    {
        tx = std::move(arg);
    }

    //! make sure balances are recalculated
    void MarkDirty()
    {
        m_amounts[DEBIT].Reset();
        m_amounts[CREDIT].Reset();
        m_amounts[IMMATURE_CREDIT].Reset();
        m_amounts[AVAILABLE_CREDIT].Reset();
        fChangeCached = false;
        m_is_cache_empty = true;
    }

    /** True if only scriptSigs are different */
    bool IsEquivalentTo(const CWalletTx& tx) const;

    bool InMempool() const;

    int64_t GetTxTime() const;

    template<typename T> const T* state() const { return std::get_if<T>(&m_state); }
    template<typename T> T* state() { return std::get_if<T>(&m_state); }

    //! Update transaction state when attaching to a chain, filling in heights
    //! of conflicted and confirmed blocks
    void updateState(interfaces::Chain& chain);

    bool isAbandoned() const { return state<TxStateInactive>() && state<TxStateInactive>()->abandoned; }
    bool isMempoolConflicted() const { return !mempool_conflicts.empty(); }
    bool isBlockConflicted() const { return state<TxStateBlockConflicted>(); }
    bool isInactive() const { return state<TxStateInactive>(); }
    bool isUnconfirmed() const { return !isAbandoned() && !isBlockConflicted() && !isMempoolConflicted() && !isConfirmed(); }
    bool isConfirmed() const { return state<TxStateConfirmed>(); }
    const Txid& GetHash() const LIFETIMEBOUND { return tx->GetHash(); }
    const Wtxid& GetWitnessHash() const LIFETIMEBOUND { return tx->GetWitnessHash(); }
    bool IsCoinBase() const { return tx->IsCoinBase(); }

private:
    // Disable copying of CWalletTx objects to prevent bugs where instances get
    // copied in and out of the mapWallet map, and fields are updated in the
    // wrong copy.
    CWalletTx(const CWalletTx&) = default;
    CWalletTx& operator=(const CWalletTx&) = default;
public:
    // Instead have an explicit copy function
    void CopyFrom(const CWalletTx&);
};

struct WalletTxOrderComparator {
    bool operator()(const CWalletTx* a, const CWalletTx* b) const
    {
        return a->nOrderPos < b->nOrderPos;
    }
};
} // namespace wallet

#endif // BITCOIN_WALLET_TRANSACTION_H
