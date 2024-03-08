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
    auto ancestors{pool.CalculateMemPoolAncestors(entry, /*fSearchForParents=*/false)};

    for (auto& entry : ancestors) {
        if (SignalsOptInRBF(entry.get().GetTx())) {
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

    if (iters_conflicting.size() > MAX_REPLACEMENT_CANDIDATES) {
        return strprintf("rejecting replacement %s; too many direct conflicts (%ud > %d)\n",
                txid.ToString(),
                iters_conflicting.size(),
                MAX_REPLACEMENT_CANDIDATES);
    }
    // Calculate the set of all transactions that would have to be evicted.
    for (CTxMemPool::txiter it : iters_conflicting) {
        // The cluster count limit ensures that we won't do too much work on a
        // single invocation of this function.
        pool.CalculateDescendantsSlow(it, all_conflicts);
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

std::optional<std::string> PaysForRBF(CAmount original_fees,
                                      CAmount replacement_fees,
                                      size_t replacement_vsize,
                                      CFeeRate relay_fee,
                                      const uint256& txid)
{
    // Rule #2: The replacement fees must be greater than or equal to fees of the
    // transactions it replaces, otherwise the bandwidth used by those conflicting transactions
    // would not be paid for.
    if (replacement_fees < original_fees) {
        return strprintf("rejecting replacement %s, less fees than conflicting txs; %s < %s",
                         txid.ToString(), FormatMoney(replacement_fees), FormatMoney(original_fees));
    }

    // Rule #3: The new transaction must pay for its own bandwidth. Otherwise, we have a DoS
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

// Compare two feerate points, where one of the points is interpolated from
// existing points in a feerate diagram.
// Return 1 if the interpolated point is greater than fee_compare; 0 if they
// are equal; -1 otherwise.
int InterpolateValueAndCompare(int64_t eval_size, const FeeFrac& p1, const FeeFrac& p2, CAmount fee_compare)
{
    // Interpolate between two points using the formula:
    // y = y1 + (x - x1) * (y2 - y1) / (x2 - x1)
    // where x is eval_size, y is the interpolated fee, x1 and y1 are p1, and x2 and y2 are p2.
    // Then evaluating y > fee_compare is equivalent to checking if y*(x2-x1) > fee_compare*(x2-x1),
    // or y1*(x2-x1) + (x - x1) * (y2 - y1) > fee_compare*(x2-x1).
    int64_t fee_compare_scaled = fee_compare * (p2.size - p1.size); // 1100* 300
    int64_t y_scaled = p1.fee * (p2.size - p1.size) + (eval_size - p1.size) * (p2.fee - p1.fee);
    if (y_scaled > fee_compare_scaled) {
        return 1;
    } else if (y_scaled == fee_compare_scaled) {
        return 0;
    } else {
        return -1;
    }
}

// returns true if the new_diagram is strictly better than the old one; false
// otherwise.
bool CompareFeeSizeDiagram(std::vector<FeeFrac> old_diagram, std::vector<FeeFrac> new_diagram)
{
    size_t old_index=0;
    size_t new_index=0;

    // whether the new diagram has at least one point better than old_diagram
    bool new_better = false;

    // whether the old diagram has at least one point better than new_diagram
    bool old_better = false;

    // Start by padding the smaller diagram with a transaction that pays the
    // tail feerate up to the size of the larger diagram.
    // For now, use an implicit tail feerate of 0, but we can change this if
    // there's an argument to be made that the true tail feerate is higher.
    // Also, if we end up needing to transform the feerates (eg to avoid
    // negative numbers or overflow in the calculations?), then the tail
    // feerate would need to be transformed as well.
    if (old_diagram.back().size < new_diagram.back().size) {
        old_diagram.push_back({old_diagram.back().fee, new_diagram.back().size});
    } else if (old_diagram.back().size > new_diagram.back().size) {
        new_diagram.push_back({new_diagram.back().fee, old_diagram.back().size});
    }

    while (old_index < old_diagram.size() && new_index < new_diagram.size()) {
        int cmp = 0;
        if (old_diagram[old_index].size < new_diagram[new_index].size) {
            cmp = InterpolateValueAndCompare(old_diagram[old_index].size, new_diagram[new_index-1], new_diagram[new_index], old_diagram[old_index].fee);
            old_better |= (cmp == -1);
            new_better |= (cmp == 1);
            old_index++;
        } else if (old_diagram[old_index].size > new_diagram[new_index].size) {
            cmp = InterpolateValueAndCompare(new_diagram[new_index].size, old_diagram[old_index-1], old_diagram[old_index], new_diagram[new_index].fee);
            old_better |= (cmp == 1);
            new_better |= (cmp == -1);
            new_index++;
        } else {
            if (old_diagram[old_index].fee > new_diagram[new_index].fee) {
                old_better = true;
            } else if (old_diagram[old_index].fee < new_diagram[new_index].fee) {
                new_better = true;
            }
            old_index++;
            new_index++;
        }
    }

    if (new_better && !old_better) return true;

    return false;
}

std::optional<std::string> ImprovesFeerateDiagram(CTxMemPool& pool,
                                                const CTxMemPool::setEntries& direct_conflicts,
                                                const CTxMemPool::setEntries& all_conflicts,
                                                CTxMemPoolEntry& entry,
                                                CAmount modified_fee)
{
    // Require that the replacement strictly improve the mempool's fee vs. size diagram.
    std::vector<FeeFrac> old_diagram, new_diagram;

    if (!pool.CalculateFeerateDiagramsForRBF(entry, modified_fee, direct_conflicts, all_conflicts, old_diagram, new_diagram)) {
        return strprintf("rejecting replacement %s, cluster size limit exceeded", entry.GetTx().GetHash().ToString());
    }

    if (!CompareFeeSizeDiagram(old_diagram, new_diagram)) {
        return strprintf("rejecting replacement %s, mempool not strictly improved",
                         entry.GetTx().GetHash().ToString());
    }
    return std::nullopt;
}
