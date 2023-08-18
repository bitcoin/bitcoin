// Copyright (c) 2023 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_MEMPOOL_SET_DEFINITIONS_H
#define BITCOIN_MEMPOOL_SET_DEFINITIONS_H

#include <kernel/mempool_entry.h>
#include <uint256.h>
#include <util/hasher.h>

#include <set>

#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/identity.hpp>
#include <boost/multi_index/indexed_by.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/tag.hpp>
#include <boost/multi_index_container.hpp>

/* This file should only be included in implementation (.cpp) files. */

namespace MemPoolMultiIndex {

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

struct MapTxImpl {
    indexed_transaction_set impl;
};

typedef indexed_transaction_set::nth_index<0>::type::const_iterator raw_txiter;

struct txiter {
    raw_txiter impl;

    txiter(const raw_txiter& inner_impl)
        : impl(inner_impl) {}
};

typedef indexed_transaction_set::const_iterator const_txiter;

} // namespace MemPoolMultiIndex

#endif // BITCOIN_MEMPOOL_SET_DEFINITIONS_H
