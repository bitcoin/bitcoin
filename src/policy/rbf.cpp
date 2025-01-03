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
    if (!pool.exists(GenTxid::Txid(tx.GetHash()))) {
        return RBFTransactionState::UNKNOWN;
    }

    // If all the inputs have nSequence >= maxint-1, it still might be
    // signaled for RBF if any unconfirmed parents have signaled.
    const auto& entry{*Assert(pool.GetEntry(tx.GetHash()))};
    auto ancestors{pool.AssumeCalculateMemPoolAncestors(__func__, entry, CTxMemPool::Limits::NoLimits(),
                                                        /*fSearchForParents=*/false)};

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
    const uint256 txid = tx.GetHash();
    uint64_t nConflictingCount = 0;
    for (const auto& mi : iters_conflicting) {
        nConflictingCount += mi->GetCountWithDescendants();
        // Rule #5: don't consider replacing more than MAX_REPLACEMENT_CANDIDATES
        // entries from the mempool. This potentially overestimates the number of actual
        // descendants (i.e. if multiple conflicts share a descendant, it will be counted multiple
        // times), but we just want to be conservative to avoid doing too much work.
        if (nConflictingCount > MAX_REPLACEMENT_CANDIDATES) {
            return strprintf("rejecting replacement %s; too many potential replacements (%d > %d)",
                             txid.ToString(),
                             nConflictingCount,
                             MAX_REPLACEMENT_CANDIDATES);
        }
    }
    // Calculate the set of all transactions that would have to be evicted.
    for (CTxMemPool::txiter it : iters_conflicting) {
        pool.CalculateDescendants(it, all_conflicts);
    }
    return std::nullopt;
}

std::optional<std::string> HasNoNewUnconfirmed(const CTransaction& tx,
                                               const CTxMemPool& pool,
                                               const CTxMemPool::setEntries& iters_conflicting)
{
    AssertLockHeld(pool.cs);
    std::set<uint256> parents_of_conflicts;
    for (const auto& mi : iters_conflicting) {
        for (const CTxIn& txin : mi->GetTx().vin) {
            parents_of_conflicts.insert(txin.prevout.hash);
        }
    }

    for (unsigned int j = 0; j < tx.vin.size(); j++) {
        // Rule #2: We don't want to accept replacements that require low feerate junk to be
        // mined first.  Ideally we'd keep track of the ancestor feerates and make the decision
        // based on that, but for now requiring all new inputs to be confirmed works.
        //
        // Note that if you relax this to make RBF a little more useful, this may break the
        // CalculateMempoolAncestors RBF relaxation which subtracts the conflict count/size from the
        // descendant limit.
        if (!parents_of_conflicts.count(tx.vin[j].prevout.hash)) {
            // Rather than check the UTXO set - potentially expensive - it's cheaper to just check
            // if the new input refers to a tx that's in the mempool.
            if (pool.exists(GenTxid::Txid(tx.vin[j].prevout.hash))) {
                return strprintf("replacement %s adds unconfirmed input, idx %d",
                                 tx.GetHash().ToString(), j);
            }
        }
    }
    return std::nullopt;
}

std::optional<std::string> EntriesAndTxidsDisjoint(const CTxMemPool::setEntries& ancestors,
                                                   const std::set<Txid>& direct_conflicts,
                                                   const uint256& txid)
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

std::optional<std::string> PaysMoreThanConflicts(const CTxMemPool::setEntries& iters_conflicting,
                                                 CFeeRate replacement_feerate,
                                                 const uint256& txid)
{
    for (const auto& mi : iters_conflicting) {
        // Don't allow the replacement to reduce the feerate of the mempool.
        //
        // We usually don't want to accept replacements with lower feerates than what they replaced
        // as that would lower the feerate of the next block. Requiring that the feerate always be
        // increased is also an easy-to-reason about way to prevent DoS attacks via replacements.
        //
        // We only consider the feerates of transactions being directly replaced, not their indirect
        // descendants. While that does mean high feerate children are ignored when deciding whether
        // or not to replace, we do require the replacement to pay more overall fees too, mitigating
        // most cases.
        CFeeRate original_feerate(mi->GetModifiedFee(), mi->GetTxSize());
        if (replacement_feerate <= original_feerate) {
            return strprintf("rejecting replacement %s; new feerate %s <= old feerate %s",
                             txid.ToString(),
                             replacement_feerate.ToString(),
                             original_feerate.ToString());
        }
    }
    return std::nullopt;
}

std::optional<std::string> PaysForRBF(CAmount original_fees,
                                      CAmount replacement_fees,
                                      size_t replacement_vsize,
                                      CFeeRate relay_fee,
                                      const uint256& txid)
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
