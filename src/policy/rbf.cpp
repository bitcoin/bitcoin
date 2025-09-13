// Copyright (c) 2016-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <policy/rbf.h>

#include <consensus/amount.h>
#include <kernel/mempool_entry.h>
#include <policy/feerate.h>
#include <primitives/transaction.h>
#include <sync.h>
#include <tinyformat.h>
#include <txmempool.h>
#include <uint256.h>
#include <util/check.h>
#include <util/moneystr.h>
#include <util/rbf.h>

#include <limits>
#include <vector>

#include <compare>

RBFTransactionState IsRBFOptIn(const CTransaction& tx, const CTxMemPool& pool)
{
    AssertLockHeld(pool.cs);

    // First check the transaction itself.
    if (SignalsOptInRBF(tx)) {
        return RBFTransactionState::REPLACEABLE_BIP125;
    }

    // If this transaction is not in our mempool, then we can't be sure
    // we will know about all its inputs.
    if (!pool.exists(tx.GetHash())) {
        return RBFTransactionState::UNKNOWN;
    }

    // If all the inputs have nSequence >= maxint-1, it still might be
    // signaled for RBF if any unconfirmed parents have signaled.
    const auto& entry{*Assert(pool.GetEntry(tx.GetHash()))};
    auto ancestors{pool.CalculateMemPoolAncestors(entry)};

    for (CTxMemPool::txiter it : ancestors) {
        if (SignalsOptInRBF(it->GetTx())) {
            return RBFTransactionState::REPLACEABLE_BIP125;
        }
    }
    return RBFTransactionState::FINAL;
}

RBFTransactionState IsRBFOptInEmptyMempool(const CTransaction& tx)
{
    // If we don't have a local mempool we can only check the transaction itself.
    return SignalsOptInRBF(tx) ? RBFTransactionState::REPLACEABLE_BIP125 : RBFTransactionState::UNKNOWN;
}

std::optional<std::string> GetEntriesForConflicts(const CTransaction& tx,
                                                  CTxMemPool& pool,
                                                  const CTxMemPool::setEntries& iters_conflicting,
                                                  CTxMemPool::setEntries& all_conflicts)
{
    AssertLockHeld(pool.cs);
    // Rule #5: don't consider replacements that conflict directly with more
    // than MAX_REPLACEMENT_CANDIDATES distinct clusters. This implies a bound
    // on how many mempool clusters might need to be re-sorted in order to
    // process the replacement (though the actual number of clusters we
    // relinearize may be greater than this number, due to cluster splitting).
    auto num_clusters = pool.GetUniqueClusterCount(iters_conflicting);
    if (num_clusters > MAX_REPLACEMENT_CANDIDATES) {
        return strprintf("rejecting replacement %s; too many conflicting clusters (%u > %d)",
                tx.GetHash().ToString(),
                num_clusters,
                MAX_REPLACEMENT_CANDIDATES);
    }
    // Calculate the set of all transactions that would have to be evicted.
    for (CTxMemPool::txiter it : iters_conflicting) {
        // The cluster count limit ensures that we won't do too much work on a
        // single invocation of this function.
        pool.CalculateDescendants(it, all_conflicts);
    }
    return std::nullopt;
}

std::optional<std::string> EntriesAndTxidsDisjoint(const CTxMemPool::setEntries& ancestors,
                                                   const std::set<Txid>& direct_conflicts,
                                                   const Txid& txid)
{
    for (CTxMemPool::txiter ancestorIt : ancestors) {
        const Txid& hashAncestor = ancestorIt->GetTx().GetHash();
        if (direct_conflicts.count(hashAncestor)) {
            return strprintf("%s spends conflicting transaction %s",
                             txid.ToString(),
                             hashAncestor.ToString());
        }
    }
    return std::nullopt;
}

std::optional<std::string> PaysForRBF(CAmount original_fees,
                                      CAmount replacement_fees,
                                      size_t replacement_vsize,
                                      CFeeRate relay_fee,
                                      const Txid& txid)
{
    // Rule #3: The replacement fees must be greater than or equal to fees of the
    // transactions it replaces, otherwise the bandwidth used by those conflicting transactions
    // would not be paid for.
    if (replacement_fees < original_fees) {
        return strprintf("rejecting replacement %s, less fees than conflicting txs; %s < %s",
                         txid.ToString(), FormatMoney(replacement_fees), FormatMoney(original_fees));
    }

    // Rule #4: The new transaction must pay for its own bandwidth. Otherwise, we have a DoS
    // vector where attackers can cause a transaction to be replaced (and relayed) repeatedly by
    // increasing the fee by tiny amounts.
    CAmount additional_fees = replacement_fees - original_fees;
    if (additional_fees < relay_fee.GetFee(replacement_vsize)) {
        return strprintf("rejecting replacement %s, not enough additional fees to relay; %s < %s",
                         txid.ToString(),
                         FormatMoney(additional_fees),
                         FormatMoney(relay_fee.GetFee(replacement_vsize)));
    }
    return std::nullopt;
}

std::optional<std::pair<DiagramCheckError, std::string>> ImprovesFeerateDiagram(CTxMemPool::ChangeSet& changeset)
{
    // Require that the replacement strictly improves the mempool's feerate diagram.
    const auto chunk_results{changeset.CalculateChunksForRBF()};

    if (!chunk_results.has_value()) {
        return std::make_pair(DiagramCheckError::UNCALCULABLE, util::ErrorString(chunk_results).original);
    }

    if (!std::is_gt(CompareChunks(chunk_results.value().second, chunk_results.value().first))) {
        return std::make_pair(DiagramCheckError::FAILURE, "insufficient feerate: does not improve feerate diagram");
    }
    return std::nullopt;
}
