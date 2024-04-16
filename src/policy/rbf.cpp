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
    auto ancestors{pool.CalculateMemPoolAncestorsFast(entry, /*fSearchForParents=*/false)};

    for (auto tx : ancestors) {
        if (SignalsOptInRBF(tx.get().GetTx())) {
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

    // Rule #5: don't consider replacements that conflict directly with more
    // than MAX_REPLACEMENT_CANDIDATES. This gives us a bound on how many
    // mempool clusters might need to be re-sorted in order to process the
    // replacement, preventing CPU DoS.
    if (iters_conflicting.size() > MAX_REPLACEMENT_CANDIDATES) {
        return strprintf("rejecting replacement %s; too many direct conflicts (%u > %d)\n",
                txid.ToString(),
                iters_conflicting.size(),
                MAX_REPLACEMENT_CANDIDATES);
    }

    // Calculate the set of all transactions that would have to be evicted.
    CTxMemPool::Entries direct_conflicts{iters_conflicting.begin(), iters_conflicting.end()};
    auto descendants = pool.CalculateDescendants(direct_conflicts);
    for (auto it : descendants) {
        all_conflicts.insert(it);
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

std::optional<std::pair<DiagramCheckError, std::string>> ImprovesFeerateDiagram(CTxMemPool& pool,
                                                const CTxMemPool::setEntries& direct_conflicts,
                                                const CTxMemPool::setEntries& all_conflicts,
                                                std::vector<std::pair<CTxMemPoolEntry*, CAmount>> new_entries)
{
    // Require that the replacement strictly improves the mempool's feerate diagram.
    const auto chunk_results{pool.CalculateChunksForRBF(new_entries, direct_conflicts, all_conflicts)};

    if (!chunk_results.has_value()) {
        return std::make_pair(DiagramCheckError::UNCALCULABLE, util::ErrorString(chunk_results).original);
    }

    if (!std::is_gt(CompareChunks(chunk_results.value().second, chunk_results.value().first))) {
        return std::make_pair(DiagramCheckError::FAILURE, "insufficient feerate: does not improve feerate diagram");
    }
    return std::nullopt;
}
