// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SYSCOIN_TXMEMPOOL_H
#define SYSCOIN_TXMEMPOOL_H

#include <atomic>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <kernel/mempool_limits.h>
#include <kernel/mempool_options.h>

#include <coins.h>
#include <consensus/amount.h>
#include <indirectmap.h>
#include <kernel/cs_main.h>
#include <kernel/mempool_entry.h>
#include <policy/feerate.h>
#include <policy/packages.h>
#include <primitives/transaction.h>
#include <random.h>
#include <sync.h>
#include <util/epochguard.h>
#include <util/hasher.h>
#include <util/result.h>

#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>
// SYSCOIN
#include <netaddress.h>
#include <boost/multi_index_container.hpp>

class CBlockIndex;
class CChain;
class CBLSPublicKey;
class Chainstate;

/** Fake height value used in Coin to signify they are only in the memory pool (since 0.8) */
static const uint32_t MEMPOOL_HEIGHT = 0x7FFFFFFF;

/**
 * Test whether the LockPoints height and time are still valid on the current chain
 */
bool TestLockPointValidity(CChain& active_chain, const LockPoints& lp) EXCLUSIVE_LOCKS_REQUIRED(cs_main);

// extracts a transaction hash from CTxMemPoolEntry or CTransactionRef
struct mempoolentry_txid
{
    typedef uint256 result_type;
    result_type operator() (const CTxMemPoolEntry &entry) const
    {
        return entry.GetTx().GetHash();
    }

    result_type operator() (const CTransactionRef& tx) const
    {
        return tx->GetHash();
    }
};

// extracts a transaction witness-hash from CTxMemPoolEntry or CTransactionRef
struct mempoolentry_wtxid
{
    typedef uint256 result_type;
    result_type operator() (const CTxMemPoolEntry &entry) const
    {
        return entry.GetTx().GetWitnessHash();
    }

    result_type operator() (const CTransactionRef& tx) const
    {
        return tx->GetWitnessHash();
    }
};


/** \class CompareTxMemPoolEntryByDescendantScore
 *
 *  Sort an entry by max(score/size of entry's tx, score/size with all descendants).
 */
class CompareTxMemPoolEntryByDescendantScore
{
public:
    bool operator()(const CTxMemPoolEntry& a, const CTxMemPoolEntry& b) const
    {
        double a_mod_fee, a_size, b_mod_fee, b_size;

        GetModFeeAndSize(a, a_mod_fee, a_size);
        GetModFeeAndSize(b, b_mod_fee, b_size);

        // Avoid division by rewriting (a/b > c/d) as (a*d > c*b).
        double f1 = a_mod_fee * b_size;
        double f2 = a_size * b_mod_fee;

        if (f1 == f2) {
            return a.GetTime() >= b.GetTime();
        }
        return f1 < f2;
    }

    // Return the fee/size we're using for sorting this entry.
    void GetModFeeAndSize(const CTxMemPoolEntry &a, double &mod_fee, double &size) const
    {
        // Compare feerate with descendants to feerate of the transaction, and
        // return the fee/size for the max.
        double f1 = (double)a.GetModifiedFee() * a.GetSizeWithDescendants();
        double f2 = (double)a.GetModFeesWithDescendants() * a.GetTxSize();

        if (f2 > f1) {
            mod_fee = a.GetModFeesWithDescendants();
            size = a.GetSizeWithDescendants();
        } else {
            mod_fee = a.GetModifiedFee();
            size = a.GetTxSize();
        }
    }
};

/** \class CompareTxMemPoolEntryByScore
 *
 *  Sort by feerate of entry (fee/size) in descending order
 *  This is only used for transaction relay, so we use GetFee()
 *  instead of GetModifiedFee() to avoid leaking prioritization
 *  information via the sort order.
 */
class CompareTxMemPoolEntryByScore
{
public:
    bool operator()(const CTxMemPoolEntry& a, const CTxMemPoolEntry& b) const
    {
        double f1 = (double)a.GetFee() * b.GetTxSize();
        double f2 = (double)b.GetFee() * a.GetTxSize();
        if (f1 == f2) {
            return b.GetTx().GetHash() < a.GetTx().GetHash();
        }
        return f1 > f2;
    }
};

class CompareTxMemPoolEntryByEntryTime
{
public:
    bool operator()(const CTxMemPoolEntry& a, const CTxMemPoolEntry& b) const
    {
        return a.GetTime() < b.GetTime();
    }
};

/** \class CompareTxMemPoolEntryByAncestorScore
 *
 *  Sort an entry by min(score/size of entry's tx, score/size with all ancestors).
 */
class CompareTxMemPoolEntryByAncestorFee
{
public:
    template<typename T>
    bool operator()(const T& a, const T& b) const
    {
        double a_mod_fee, a_size, b_mod_fee, b_size;

        GetModFeeAndSize(a, a_mod_fee, a_size);
        GetModFeeAndSize(b, b_mod_fee, b_size);

        // Avoid division by rewriting (a/b > c/d) as (a*d > c*b).
        double f1 = a_mod_fee * b_size;
        double f2 = a_size * b_mod_fee;

        if (f1 == f2) {
            return a.GetTx().GetHash() < b.GetTx().GetHash();
        }
        return f1 > f2;
    }

    // Return the fee/size we're using for sorting this entry.
    template <typename T>
    void GetModFeeAndSize(const T &a, double &mod_fee, double &size) const
    {
        // Compare feerate with ancestors to feerate of the transaction, and
        // return the fee/size for the min.
        double f1 = (double)a.GetModifiedFee() * a.GetSizeWithAncestors();
        double f2 = (double)a.GetModFeesWithAncestors() * a.GetTxSize();

        if (f1 > f2) {
            mod_fee = a.GetModFeesWithAncestors();
            size = a.GetSizeWithAncestors();
        } else {
            mod_fee = a.GetModifiedFee();
            size = a.GetTxSize();
        }
    }
};

// Multi_index tag names
struct descendant_score {};
struct entry_time {};
struct ancestor_score {};
struct index_by_wtxid {};

class CBlockPolicyEstimator;

/**
 * Information about a mempool transaction.
 */
struct TxMempoolInfo
{
    /** The transaction itself */
    CTransactionRef tx;

    /** Time the transaction entered the mempool. */
    std::chrono::seconds m_time;

    /** Fee of the transaction. */
    CAmount fee;

    /** Virtual size of the transaction. */
    size_t vsize;

    /** The fee delta. */
    int64_t nFeeDelta;
};

/** Reason why a transaction was removed from the mempool,
 * this is passed to the notification signal.
 */
enum class MemPoolRemovalReason {
    EXPIRY,      //!< Expired from mempool
    SIZELIMIT,   //!< Removed in size limiting
    REORG,       //!< Removed for reorganization
    BLOCK,       //!< Removed for block
    CONFLICT,    //!< Removed for conflict with in-block transaction
    REPLACED,    //!< Removed for replacement
};

std::string RemovalReasonToString(const MemPoolRemovalReason& r) noexcept;

/**
 * CTxMemPool stores valid-according-to-the-current-best-chain transactions
 * that may be included in the next block.
 *
 * Transactions are added when they are seen on the network (or created by the
 * local node), but not all transactions seen are added to the pool. For
 * example, the following new transactions will not be added to the mempool:
 * - a transaction which doesn't meet the minimum fee requirements.
 * - a new transaction that double-spends an input of a transaction already in
 * the pool where the new transaction does not meet the Replace-By-Fee
 * requirements as defined in doc/policy/mempool-replacements.md.
 * - a non-standard transaction.
 *
 * CTxMemPool::mapTx, and CTxMemPoolEntry bookkeeping:
 *
 * mapTx is a boost::multi_index that sorts the mempool on 5 criteria:
 * - transaction hash (txid)
 * - witness-transaction hash (wtxid)
 * - descendant feerate [we use max(feerate of tx, feerate of tx with all descendants)]
 * - time in mempool
 * - ancestor feerate [we use min(feerate of tx, feerate of tx with all unconfirmed ancestors)]
 *
 * Note: the term "descendant" refers to in-mempool transactions that depend on
 * this one, while "ancestor" refers to in-mempool transactions that a given
 * transaction depends on.
 *
 * In order for the feerate sort to remain correct, we must update transactions
 * in the mempool when new descendants arrive.  To facilitate this, we track
 * the set of in-mempool direct parents and direct children in mapLinks.  Within
 * each CTxMemPoolEntry, we track the size and fees of all descendants.
 *
 * Usually when a new transaction is added to the mempool, it has no in-mempool
 * children (because any such children would be an orphan).  So in
 * addUnchecked(), we:
 * - update a new entry's setMemPoolParents to include all in-mempool parents
 * - update the new entry's direct parents to include the new tx as a child
 * - update all ancestors of the transaction to include the new tx's size/fee
 *
 * When a transaction is removed from the mempool, we must:
 * - update all in-mempool parents to not track the tx in setMemPoolChildren
 * - update all ancestors to not include the tx's size/fees in descendant state
 * - update all in-mempool children to not include it as a parent
 *
 * These happen in UpdateForRemoveFromMempool().  (Note that when removing a
 * transaction along with its descendants, we must calculate that set of
 * transactions to be removed before doing the removal, or else the mempool can
 * be in an inconsistent state where it's impossible to walk the ancestors of
 * a transaction.)
 *
 * In the event of a reorg, the assumption that a newly added tx has no
 * in-mempool children is false.  In particular, the mempool is in an
 * inconsistent state while new transactions are being added, because there may
 * be descendant transactions of a tx coming from a disconnected block that are
 * unreachable from just looking at transactions in the mempool (the linking
 * transactions may also be in the disconnected block, waiting to be added).
 * Because of this, there's not much benefit in trying to search for in-mempool
 * children in addUnchecked().  Instead, in the special case of transactions
 * being added from a disconnected block, we require the caller to clean up the
 * state, to account for in-mempool, out-of-block descendants for all the
 * in-block transactions by calling UpdateTransactionsFromBlock().  Note that
 * until this is called, the mempool state is not consistent, and in particular
 * mapLinks may not be correct (and therefore functions like
 * CalculateMemPoolAncestors() and CalculateDescendants() that rely
 * on them to walk the mempool are not generally safe to use).
 *
 * Computational limits:
 *
 * Updating all in-mempool ancestors of a newly added transaction can be slow,
 * if no bound exists on how many in-mempool ancestors there may be.
 * CalculateMemPoolAncestors() takes configurable limits that are designed to
 * prevent these calculations from being too CPU intensive.
 *
 */
class CTxMemPool
{
protected:
    const int m_check_ratio; //!< Value n means that 1 times in n we check.
    std::atomic<unsigned int> nTransactionsUpdated{0}; //!< Used by getblocktemplate to trigger CreateNewBlock() invocation
    CBlockPolicyEstimator* const minerPolicyEstimator;

    uint64_t totalTxSize GUARDED_BY(cs){0};      //!< sum of all mempool tx's virtual sizes. Differs from serialized tx size since witness data is discounted. Defined in BIP 141.
    CAmount m_total_fee GUARDED_BY(cs){0};       //!< sum of all mempool tx's fees (NOT modified fee)
    uint64_t cachedInnerUsage GUARDED_BY(cs){0}; //!< sum of dynamic memory usage of all the map elements (NOT the maps themselves)

    mutable int64_t lastRollingFeeUpdate GUARDED_BY(cs){GetTime()};
    mutable bool blockSinceLastRollingFeeBump GUARDED_BY(cs){false};
    mutable double rollingMinimumFeeRate GUARDED_BY(cs){0}; //!< minimum fee to get into the pool, decreases exponentially
    mutable Epoch m_epoch GUARDED_BY(cs){};

    // In-memory counter for external mempool tracking purposes.
    // This number is incremented once every time a transaction
    // is added or removed from the mempool for any reason.
    mutable uint64_t m_sequence_number GUARDED_BY(cs){1};

    void trackPackageRemoved(const CFeeRate& rate) EXCLUSIVE_LOCKS_REQUIRED(cs);

    bool m_load_tried GUARDED_BY(cs){false};

    CFeeRate GetMinFee(size_t sizelimit) const;

public:

    static const int ROLLING_FEE_HALFLIFE = 60 * 60 * 12; // public only for testing

    typedef boost::multi_index_container<
        CTxMemPoolEntry,
        boost::multi_index::indexed_by<
            // sorted by txid
            boost::multi_index::hashed_unique<mempoolentry_txid, SaltedTxidHasher>,
            // sorted by wtxid
            boost::multi_index::hashed_unique<
                boost::multi_index::tag<index_by_wtxid>,
                mempoolentry_wtxid,
                SaltedTxidHasher
            >,
            // sorted by fee rate
            boost::multi_index::ordered_non_unique<
                boost::multi_index::tag<descendant_score>,
                boost::multi_index::identity<CTxMemPoolEntry>,
                CompareTxMemPoolEntryByDescendantScore
            >,
            // sorted by entry time
            boost::multi_index::ordered_non_unique<
                boost::multi_index::tag<entry_time>,
                boost::multi_index::identity<CTxMemPoolEntry>,
                CompareTxMemPoolEntryByEntryTime
            >,
            // sorted by fee rate with ancestors
            boost::multi_index::ordered_non_unique<
                boost::multi_index::tag<ancestor_score>,
                boost::multi_index::identity<CTxMemPoolEntry>,
                CompareTxMemPoolEntryByAncestorFee
            >
        >
    > indexed_transaction_set;

    /**
     * This mutex needs to be locked when accessing `mapTx` or other members
     * that are guarded by it.
     *
     * @par Consistency guarantees
     *
     * By design, it is guaranteed that:
     *
     * 1. Locking both `cs_main` and `mempool.cs` will give a view of mempool
     *    that is consistent with current chain tip (`ActiveChain()` and
     *    `CoinsTip()`) and is fully populated. Fully populated means that if the
     *    current active chain is missing transactions that were present in a
     *    previously active chain, all the missing transactions will have been
     *    re-added to the mempool and should be present if they meet size and
     *    consistency constraints.
     *
     * 2. Locking `mempool.cs` without `cs_main` will give a view of a mempool
     *    consistent with some chain that was active since `cs_main` was last
     *    locked, and that is fully populated as described above. It is ok for
     *    code that only needs to query or remove transactions from the mempool
     *    to lock just `mempool.cs` without `cs_main`.
     *
     * To provide these guarantees, it is necessary to lock both `cs_main` and
     * `mempool.cs` whenever adding transactions to the mempool and whenever
     * changing the chain tip. It's necessary to keep both mutexes locked until
     * the mempool is consistent with the new chain tip and fully populated.
     *
     * @par Consistency bug
     *
     * The second guarantee above is not currently enforced, but
     * https://github.com/syscoin/syscoin/pull/14193 will fix it. No known code
     * in syscoin currently depends on second guarantee, but it is important to
     * fix for third party code that needs be able to frequently poll the
     * mempool without locking `cs_main` and without encountering missing
     * transactions during reorgs.
     */
    mutable RecursiveMutex cs;
    indexed_transaction_set mapTx GUARDED_BY(cs);

    using txiter = indexed_transaction_set::nth_index<0>::type::const_iterator;
    std::vector<std::pair<uint256, txiter>> vTxHashes GUARDED_BY(cs); //!< All tx witness hashes/entries in mapTx, in random order

    typedef std::set<txiter, CompareIteratorByHash> setEntries;

    using Limits = kernel::MemPoolLimits;

    uint64_t CalculateDescendantMaximum(txiter entry) const EXCLUSIVE_LOCKS_REQUIRED(cs);
private:
    typedef std::map<txiter, setEntries, CompareIteratorByHash> cacheMap;


    void UpdateParent(txiter entry, txiter parent, bool add) EXCLUSIVE_LOCKS_REQUIRED(cs);
    void UpdateChild(txiter entry, txiter child, bool add) EXCLUSIVE_LOCKS_REQUIRED(cs);

    std::vector<indexed_transaction_set::const_iterator> GetSortedDepthAndScore() const EXCLUSIVE_LOCKS_REQUIRED(cs);

    /**
     * Track locally submitted transactions to periodically retry initial broadcast.
     */
    std::set<uint256> m_unbroadcast_txids GUARDED_BY(cs);
    // SYSCOIN
    std::multimap<uint256, uint256> mapProTxRefs; // proTxHash -> transaction (all TXs that refer to an existing proTx)
    std::map<CService, uint256> mapProTxAddresses;
    std::map<CKeyID, uint256> mapProTxPubKeyIDs;
    std::map<uint256, uint256> mapProTxBlsPubKeyHashes;
    std::map<COutPoint, uint256> mapProTxCollaterals;


    /**
     * Helper function to calculate all in-mempool ancestors of staged_ancestors and apply ancestor
     * and descendant limits (including staged_ancestors themselves, entry_size and entry_count).
     *
     * @param[in]   entry_size          Virtual size to include in the limits.
     * @param[in]   entry_count         How many entries to include in the limits.
     * @param[in]   staged_ancestors    Should contain entries in the mempool.
     * @param[in]   limits              Maximum number and size of ancestors and descendants
     *
     * @return all in-mempool ancestors, or an error if any ancestor or descendant limits were hit
     */
    util::Result<setEntries> CalculateAncestorsAndCheckLimits(size_t entry_size,
                                                              size_t entry_count,
                                                              CTxMemPoolEntry::Parents &staged_ancestors,
                                                              const Limits& limits
                                                              ) const EXCLUSIVE_LOCKS_REQUIRED(cs);

public:
    indirectmap<COutPoint, const CTransaction*> mapNextTx GUARDED_BY(cs);
    std::map<uint256, CAmount> mapDeltas GUARDED_BY(cs);

    using Options = kernel::MemPoolOptions;

    const int64_t m_max_size_bytes;
    const std::chrono::seconds m_expiry;
    const CFeeRate m_incremental_relay_feerate;
    const CFeeRate m_min_relay_feerate;
    const CFeeRate m_dust_relay_feerate;
    const bool m_permit_bare_multisig;
    const std::optional<unsigned> m_max_datacarrier_bytes;
    const bool m_require_standard;
    const bool m_full_rbf;

    const Limits m_limits;

    /** Create a new CTxMemPool.
     * Sanity checks will be off by default for performance, because otherwise
     * accepting transactions becomes O(N^2) where N is the number of transactions
     * in the pool.
     */
    explicit CTxMemPool(const Options& opts);

    /**
     * If sanity-checking is turned on, check makes sure the pool is
     * consistent (does not contain two transactions that spend the same inputs,
     * all inputs are in the mapNextTx array). If sanity-checking is turned off,
     * check does nothing.
     */
    void check(const CCoinsViewCache& active_coins_tip, int64_t spendheight) const EXCLUSIVE_LOCKS_REQUIRED(::cs_main);

    // addUnchecked must updated state for all ancestors of a given transaction,
    // to track size/count of descendant transactions.  First version of
    // addUnchecked can be used to have it call CalculateMemPoolAncestors(), and
    // then invoke the second version.
    // Note that addUnchecked is ONLY called from ATMP outside of tests
    // and any other callers may break wallet's in-mempool tracking (due to
    // lack of CValidationInterface::TransactionAddedToMempool callbacks).
    void addUnchecked(const CTxMemPoolEntry& entry, bool validFeeEstimate = true) EXCLUSIVE_LOCKS_REQUIRED(cs, cs_main);
    void addUnchecked(const CTxMemPoolEntry& entry, setEntries& setAncestors, bool validFeeEstimate = true) EXCLUSIVE_LOCKS_REQUIRED(cs, cs_main);
    void removeRecursive(const CTransaction& tx, MemPoolRemovalReason reason) EXCLUSIVE_LOCKS_REQUIRED(cs);
    // SYSCOIN
    void removeProTxPubKeyConflicts(const CTransaction &tx, const CKeyID &keyId) EXCLUSIVE_LOCKS_REQUIRED(cs, cs_main);
    void removeProTxPubKeyConflicts(const CTransaction &tx, const CBLSPublicKey &pubKey) EXCLUSIVE_LOCKS_REQUIRED(cs, cs_main);
    void removeProTxCollateralConflicts(const CTransaction &tx, const COutPoint &collateralOutpoint) EXCLUSIVE_LOCKS_REQUIRED(cs, cs_main);
    void removeProTxSpentCollateralConflicts(const CTransaction &tx) EXCLUSIVE_LOCKS_REQUIRED(cs, cs_main);
    void removeProTxKeyChangedConflicts(const CTransaction &tx, const uint256& proTxHash, const uint256& newKeyHash) EXCLUSIVE_LOCKS_REQUIRED(cs, cs_main);
    void removeProTxConflicts(const CTransaction &tx) EXCLUSIVE_LOCKS_REQUIRED(cs, cs_main);
    bool existsConflicts(const CTransaction& tx) const EXCLUSIVE_LOCKS_REQUIRED(cs, cs_main);
    bool isSyscoinConflictIsFirstSeen(const CTransaction &tx) const EXCLUSIVE_LOCKS_REQUIRED(cs);
    void removeZDAGConflicts(const CTransaction& tx) EXCLUSIVE_LOCKS_REQUIRED(cs, cs_main);
    /** After reorg, filter the entries that would no longer be valid in the next block, and update
     * the entries' cached LockPoints if needed.  The mempool does not have any knowledge of
     * consensus rules. It just appplies the callable function and removes the ones for which it
     * returns true.
     * @param[in]   filter_final_and_mature   Predicate that checks the relevant validation rules
     *                                        and updates an entry's LockPoints.
     * */
    void removeForReorg(CChain& chain, std::function<bool(txiter)> filter_final_and_mature) EXCLUSIVE_LOCKS_REQUIRED(cs, cs_main);
    // SYSCOIN
    void removeConflicts(const CTransaction& tx) EXCLUSIVE_LOCKS_REQUIRED(cs, cs_main);
    void removeForBlock(const std::vector<CTransactionRef>& vtx, unsigned int nBlockHeight) EXCLUSIVE_LOCKS_REQUIRED(cs, cs_main);

    bool CompareDepthAndScore(const uint256& hasha, const uint256& hashb, bool wtxid=false);
    void queryHashes(std::vector<uint256>& vtxid) const;
    bool isSpent(const COutPoint& outpoint) const;
    unsigned int GetTransactionsUpdated() const;
    void AddTransactionsUpdated(unsigned int n);
    /**
     * Check that none of this transactions inputs are in the mempool, and thus
     * the tx is not dependent on other mempool transactions to be included in a block.
     */
    bool HasNoInputsOf(const CTransaction& tx) const EXCLUSIVE_LOCKS_REQUIRED(cs);

    /** Affect CreateNewBlock prioritisation of transactions */
    void PrioritiseTransaction(const uint256& hash, const CAmount& nFeeDelta);
    void ApplyDelta(const uint256& hash, CAmount &nFeeDelta) const EXCLUSIVE_LOCKS_REQUIRED(cs);
    void ClearPrioritisation(const uint256& hash) EXCLUSIVE_LOCKS_REQUIRED(cs);
    
    /** Get the transaction in the pool that spends the same prevout */
    const CTransaction* GetConflictTx(const COutPoint& prevout) const EXCLUSIVE_LOCKS_REQUIRED(cs);

    /** Returns an iterator to the given hash, if found */
    std::optional<txiter> GetIter(const uint256& txid) const EXCLUSIVE_LOCKS_REQUIRED(cs);

    /** Translate a set of hashes into a set of pool iterators to avoid repeated lookups */
    setEntries GetIterSet(const std::set<uint256>& hashes) const EXCLUSIVE_LOCKS_REQUIRED(cs);

    /** Remove a set of transactions from the mempool.
     *  If a transaction is in this set, then all in-mempool descendants must
     *  also be in the set, unless this transaction is being removed for being
     *  in a block.
     *  Set updateDescendants to true when removing a tx that was in a block, so
     *  that any in-mempool descendants have their ancestor state updated.
     */
    void RemoveStaged(setEntries& stage, bool updateDescendants, MemPoolRemovalReason reason) EXCLUSIVE_LOCKS_REQUIRED(cs);

    /** UpdateTransactionsFromBlock is called when adding transactions from a
     * disconnected block back to the mempool, new mempool entries may have
     * children in the mempool (which is generally not the case when otherwise
     * adding transactions).
     *  @post updated descendant state for descendants of each transaction in
     *        vHashesToUpdate (excluding any child transactions present in
     *        vHashesToUpdate, which are already accounted for). Updated state
     *        includes add fee/size information for such descendants to the
     *        parent and updated ancestor state to include the parent.
     *
     * @param[in] vHashesToUpdate          The set of txids from the
     *     disconnected block that have been accepted back into the mempool.
     */
    void UpdateTransactionsFromBlock(const std::vector<uint256>& vHashesToUpdate) EXCLUSIVE_LOCKS_REQUIRED(cs, cs_main) LOCKS_EXCLUDED(m_epoch);

    /**
     * Try to calculate all in-mempool ancestors of entry.
     * (these are all calculated including the tx itself)
     *
     * @param[in]   entry               CTxMemPoolEntry of which all in-mempool ancestors are calculated
     * @param[in]   limits              Maximum number and size of ancestors and descendants
     * @param[in]   fSearchForParents   Whether to search a tx's vin for in-mempool parents, or look
     *                                  up parents from mapLinks. Must be true for entries not in
     *                                  the mempool
     *
     * @return all in-mempool ancestors, or an error if any ancestor or descendant limits were hit
     */
    util::Result<setEntries> CalculateMemPoolAncestors(const CTxMemPoolEntry& entry,
                                   const Limits& limits,
                                   bool fSearchForParents = true) const EXCLUSIVE_LOCKS_REQUIRED(cs);

    /**
     * Same as CalculateMemPoolAncestors, but always returns a (non-optional) setEntries.
     * Should only be used when it is assumed CalculateMemPoolAncestors would not fail. If
     * CalculateMemPoolAncestors does unexpectedly fail, an empty setEntries is returned and the
     * error is logged to BCLog::MEMPOOL with level BCLog::Level::Error. In debug builds, failure
     * of CalculateMemPoolAncestors will lead to shutdown due to assertion failure.
     *
     * @param[in]   calling_fn_name     Name of calling function so we can properly log the call site
     *
     * @return a setEntries corresponding to the result of CalculateMemPoolAncestors or an empty
     *         setEntries if it failed
     *
     * @see CTXMemPool::CalculateMemPoolAncestors()
     */
    setEntries AssumeCalculateMemPoolAncestors(
        std::string_view calling_fn_name,
        const CTxMemPoolEntry &entry,
        const Limits& limits,
        bool fSearchForParents = true) const EXCLUSIVE_LOCKS_REQUIRED(cs);

    /** Calculate all in-mempool ancestors of a set of transactions not already in the mempool and
     * check ancestor and descendant limits. Heuristics are used to estimate the ancestor and
     * descendant count of all entries if the package were to be added to the mempool.  The limits
     * are applied to the union of all package transactions. For example, if the package has 3
     * transactions and limits.ancestor_count = 25, the union of all 3 sets of ancestors (including the
     * transactions themselves) must be <= 22.
     * @param[in]       package                 Transaction package being evaluated for acceptance
     *                                          to mempool. The transactions need not be direct
     *                                          ancestors/descendants of each other.
     * @param[in]       limits                  Maximum number and size of ancestors and descendants
     * @param[out]      errString               Populated with error reason if a limit is hit.
     */
    bool CheckPackageLimits(const Package& package,
                            const Limits& limits,
                            std::string &errString) const EXCLUSIVE_LOCKS_REQUIRED(cs);

    /** Populate setDescendants with all in-mempool descendants of hash.
     *  Assumes that setDescendants includes all in-mempool descendants of anything
     *  already in it.  */
    void CalculateDescendants(txiter it, setEntries& setDescendants) const EXCLUSIVE_LOCKS_REQUIRED(cs);

    /** The minimum fee to get into the mempool, which may itself not be enough
     *  for larger-sized transactions.
     *  The m_incremental_relay_feerate policy variable is used to bound the time it
     *  takes the fee rate to go back down all the way to 0. When the feerate
     *  would otherwise be half of this, it is set to 0 instead.
     */
    CFeeRate GetMinFee() const {
        return GetMinFee(m_max_size_bytes);
    }

    /** Remove transactions from the mempool until its dynamic size is <= sizelimit.
      *  pvNoSpendsRemaining, if set, will be populated with the list of outpoints
      *  which are not in mempool which no longer have any spends in this mempool.
      */
    void TrimToSize(size_t sizelimit, std::vector<COutPoint>* pvNoSpendsRemaining = nullptr) EXCLUSIVE_LOCKS_REQUIRED(cs);

    /** Expire all transaction (and their dependencies) in the mempool older than time. Return the number of removed transactions. */
    int Expire(std::chrono::seconds time) EXCLUSIVE_LOCKS_REQUIRED(cs);

    /**
     * Calculate the ancestor and descendant count for the given transaction.
     * The counts include the transaction itself.
     * When ancestors is non-zero (ie, the transaction itself is in the mempool),
     * ancestorsize and ancestorfees will also be set to the appropriate values.
     */
    void GetTransactionAncestry(const uint256& txid, size_t& ancestors, size_t& descendants, size_t* ancestorsize = nullptr, CAmount* ancestorfees = nullptr) const;

    /**
     * @returns true if we've made an attempt to load the mempool regardless of
     *          whether the attempt was successful or not
     */
    bool GetLoadTried() const;

    /**
     * Set whether or not we've made an attempt to load the mempool (regardless
     * of whether the attempt was successful or not)
     */
    void SetLoadTried(bool load_tried);

    unsigned long size() const
    {
        LOCK(cs);
        return mapTx.size();
    }

    uint64_t GetTotalTxSize() const EXCLUSIVE_LOCKS_REQUIRED(cs)
    {
        AssertLockHeld(cs);
        return totalTxSize;
    }

    CAmount GetTotalFee() const EXCLUSIVE_LOCKS_REQUIRED(cs)
    {
        AssertLockHeld(cs);
        return m_total_fee;
    }

    bool exists(const GenTxid& gtxid) const
    {
        LOCK(cs);
        if (gtxid.IsWtxid()) {
            return (mapTx.get<index_by_wtxid>().count(gtxid.GetHash()) != 0);
        }
        return (mapTx.count(gtxid.GetHash()) != 0);
    }

    CTransactionRef get(const uint256& hash) const;
    txiter get_iter_from_wtxid(const uint256& wtxid) const EXCLUSIVE_LOCKS_REQUIRED(cs)
    {
        AssertLockHeld(cs);
        return mapTx.project<0>(mapTx.get<index_by_wtxid>().find(wtxid));
    }
    TxMempoolInfo info(const GenTxid& gtxid) const;
    std::vector<TxMempoolInfo> infoAll() const;
    // SYSCOIN
    bool existsProviderTxConflict(const CTransaction &tx) const EXCLUSIVE_LOCKS_REQUIRED(cs, cs_main);
    size_t DynamicMemoryUsage() const;

    /** Adds a transaction to the unbroadcast set */
    void AddUnbroadcastTx(const uint256& txid)
    {
        LOCK(cs);
        // Sanity check the transaction is in the mempool & insert into
        // unbroadcast set.
        if (exists(GenTxid::Txid(txid))) m_unbroadcast_txids.insert(txid);
    };

    /** Removes a transaction from the unbroadcast set */
    void RemoveUnbroadcastTx(const uint256& txid, const bool unchecked = false);

    /** Returns transactions in unbroadcast set */
    std::set<uint256> GetUnbroadcastTxs() const
    {
        LOCK(cs);
        return m_unbroadcast_txids;
    }

    /** Returns whether a txid is in the unbroadcast set */
    bool IsUnbroadcastTx(const uint256& txid) const EXCLUSIVE_LOCKS_REQUIRED(cs)
    {
        AssertLockHeld(cs);
        return m_unbroadcast_txids.count(txid) != 0;
    }

    /** Guards this internal counter for external reporting */
    uint64_t GetAndIncrementSequence() const EXCLUSIVE_LOCKS_REQUIRED(cs) {
        return m_sequence_number++;
    }

    uint64_t GetSequence() const EXCLUSIVE_LOCKS_REQUIRED(cs) {
        return m_sequence_number;
    }

private:
    /** UpdateForDescendants is used by UpdateTransactionsFromBlock to update
     *  the descendants for a single transaction that has been added to the
     *  mempool but may have child transactions in the mempool, eg during a
     *  chain reorg.
     *
     * @pre CTxMemPoolEntry::m_children is correct for the given tx and all
     *      descendants.
     * @pre cachedDescendants is an accurate cache where each entry has all
     *      descendants of the corresponding key, including those that should
     *      be removed for violation of ancestor limits.
     * @post if updateIt has any non-excluded descendants, cachedDescendants has
     *       a new cache line for updateIt.
     * @post descendants_to_remove has a new entry for any descendant which exceeded
     *       ancestor limits relative to updateIt.
     *
     * @param[in] updateIt the entry to update for its descendants
     * @param[in,out] cachedDescendants a cache where each line corresponds to all
     *     descendants. It will be updated with the descendants of the transaction
     *     being updated, so that future invocations don't need to walk the same
     *     transaction again, if encountered in another transaction chain.
     * @param[in] setExclude the set of descendant transactions in the mempool
     *     that must not be accounted for (because any descendants in setExclude
     *     were added to the mempool after the transaction being updated and hence
     *     their state is already reflected in the parent state).
     * @param[out] descendants_to_remove Populated with the txids of entries that
     *     exceed ancestor limits. It's the responsibility of the caller to
     *     removeRecursive them.
     */
    void UpdateForDescendants(txiter updateIt, cacheMap& cachedDescendants,
                              const std::set<uint256>& setExclude, std::set<uint256>& descendants_to_remove) EXCLUSIVE_LOCKS_REQUIRED(cs);
    /** Update ancestors of hash to add/remove it as a descendant transaction. */
    void UpdateAncestorsOf(bool add, txiter hash, setEntries &setAncestors) EXCLUSIVE_LOCKS_REQUIRED(cs);
    /** Set ancestor state for an entry */
    void UpdateEntryForAncestors(txiter it, const setEntries &setAncestors) EXCLUSIVE_LOCKS_REQUIRED(cs);
    /** For each transaction being removed, update ancestors and any direct children.
      * If updateDescendants is true, then also update in-mempool descendants'
      * ancestor state. */
    void UpdateForRemoveFromMempool(const setEntries &entriesToRemove, bool updateDescendants) EXCLUSIVE_LOCKS_REQUIRED(cs);
    /** Sever link between specified transaction and direct children. */
    void UpdateChildrenForRemoval(txiter entry) EXCLUSIVE_LOCKS_REQUIRED(cs);

    /** Before calling removeUnchecked for a given transaction,
     *  UpdateForRemoveFromMempool must be called on the entire (dependent) set
     *  of transactions being removed at the same time.  We use each
     *  CTxMemPoolEntry's setMemPoolParents in order to walk ancestors of a
     *  given transaction that is removed, so we can't remove intermediate
     *  transactions in a chain before we've updated all the state for the
     *  removal.
     */
    void removeUnchecked(txiter entry, MemPoolRemovalReason reason) EXCLUSIVE_LOCKS_REQUIRED(cs);
public:
    /** visited marks a CTxMemPoolEntry as having been traversed
     * during the lifetime of the most recently created Epoch::Guard
     * and returns false if we are the first visitor, true otherwise.
     *
     * An Epoch::Guard must be held when visited is called or an assert will be
     * triggered.
     *
     */
    bool visited(const txiter it) const EXCLUSIVE_LOCKS_REQUIRED(cs, m_epoch)
    {
        return m_epoch.visited(it->m_epoch_marker);
    }

    bool visited(std::optional<txiter> it) const EXCLUSIVE_LOCKS_REQUIRED(cs, m_epoch)
    {
        assert(m_epoch.guarded()); // verify guard even when it==nullopt
        return !it || visited(*it);
    }
};

/**
 * CCoinsView that brings transactions from a mempool into view.
 * It does not check for spendings by memory pool transactions.
 * Instead, it provides access to all Coins which are either unspent in the
 * base CCoinsView, are outputs from any mempool transaction, or are
 * tracked temporarily to allow transaction dependencies in package validation.
 * This allows transaction replacement to work as expected, as you want to
 * have all inputs "available" to check signatures, and any cycles in the
 * dependency graph are checked directly in AcceptToMemoryPool.
 * It also allows you to sign a double-spend directly in
 * signrawtransactionwithkey and signrawtransactionwithwallet,
 * as long as the conflicting transaction is not yet confirmed.
 */
class CCoinsViewMemPool : public CCoinsViewBacked
{
    /**
    * Coins made available by transactions being validated. Tracking these allows for package
    * validation, since we can access transaction outputs without submitting them to mempool.
    */
    std::unordered_map<COutPoint, Coin, SaltedOutpointHasher> m_temp_added;
protected:
    const CTxMemPool& mempool;

public:
    CCoinsViewMemPool(CCoinsView* baseIn, const CTxMemPool& mempoolIn);
    bool GetCoin(const COutPoint &outpoint, Coin &coin) const override;
    /** Add the coins created by this transaction. These coins are only temporarily stored in
     * m_temp_added and cannot be flushed to the back end. Only used for package validation. */
    void PackageAddTransaction(const CTransactionRef& tx);
};

/**
 * DisconnectedBlockTransactions

 * During the reorg, it's desirable to re-add previously confirmed transactions
 * to the mempool, so that anything not re-confirmed in the new chain is
 * available to be mined. However, it's more efficient to wait until the reorg
 * is complete and process all still-unconfirmed transactions at that time,
 * since we expect most confirmed transactions to (typically) still be
 * confirmed in the new chain, and re-accepting to the memory pool is expensive
 * (and therefore better to not do in the middle of reorg-processing).
 * Instead, store the disconnected transactions (in order!) as we go, remove any
 * that are included in blocks in the new chain, and then process the remaining
 * still-unconfirmed transactions at the end.
 */

// multi_index tag names
struct txid_index {};
struct insertion_order {};

struct DisconnectedBlockTransactions {
    typedef boost::multi_index_container<
        CTransactionRef,
        boost::multi_index::indexed_by<
            // sorted by txid
            boost::multi_index::hashed_unique<
                boost::multi_index::tag<txid_index>,
                mempoolentry_txid,
                SaltedTxidHasher
            >,
            // sorted by order in the blockchain
            boost::multi_index::sequenced<
                boost::multi_index::tag<insertion_order>
            >
        >
    > indexed_disconnected_transactions;

    // It's almost certainly a logic bug if we don't clear out queuedTx before
    // destruction, as we add to it while disconnecting blocks, and then we
    // need to re-process remaining transactions to ensure mempool consistency.
    // For now, assert() that we've emptied out this object on destruction.
    // This assert() can always be removed if the reorg-processing code were
    // to be refactored such that this assumption is no longer true (for
    // instance if there was some other way we cleaned up the mempool after a
    // reorg, besides draining this object).
    ~DisconnectedBlockTransactions() { assert(queuedTx.empty()); }

    indexed_disconnected_transactions queuedTx;
    uint64_t cachedInnerUsage = 0;

    // Estimate the overhead of queuedTx to be 6 pointers + an allocation, as
    // no exact formula for boost::multi_index_contained is implemented.
    size_t DynamicMemoryUsage() const {
        return memusage::MallocUsage(sizeof(CTransactionRef) + 6 * sizeof(void*)) * queuedTx.size() + cachedInnerUsage;
    }

    void addTransaction(const CTransactionRef& tx)
    {
        queuedTx.insert(tx);
        cachedInnerUsage += RecursiveDynamicUsage(tx);
    }

    // Remove entries based on txid_index, and update memory usage.
    void removeForBlock(const std::vector<CTransactionRef>& vtx)
    {
        // Short-circuit in the common case of a block being added to the tip
        if (queuedTx.empty()) {
            return;
        }
        for (auto const &tx : vtx) {
            auto it = queuedTx.find(tx->GetHash());
            if (it != queuedTx.end()) {
                cachedInnerUsage -= RecursiveDynamicUsage(*it);
                queuedTx.erase(it);
            }
        }
    }

    // Remove an entry by insertion_order index, and update memory usage.
    void removeEntry(indexed_disconnected_transactions::index<insertion_order>::type::iterator entry)
    {
        cachedInnerUsage -= RecursiveDynamicUsage(*entry);
        queuedTx.get<insertion_order>().erase(entry);
    }

    void clear()
    {
        cachedInnerUsage = 0;
        queuedTx.clear();
    }
};

#endif // SYSCOIN_TXMEMPOOL_H
