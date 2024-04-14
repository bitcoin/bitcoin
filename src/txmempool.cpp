// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <txmempool.h>

#include <chain.h>
#include <coins.h>
#include <common/system.h>
#include <consensus/consensus.h>
#include <consensus/tx_verify.h>
#include <consensus/validation.h>
#include <logging.h>
#include <policy/policy.h>
#include <policy/settings.h>
#include <random.h>
#include <reverse_iterator.h>
#include <tinyformat.h>
#include <util/check.h>
#include <util/feefrac.h>
#include <util/moneystr.h>
#include <util/overflow.h>
#include <util/result.h>
#include <util/time.h>
#include <util/trace.h>
#include <util/translation.h>
#include <validationinterface.h>

#include <algorithm>
#include <cmath>
#include <numeric>
#include <optional>
#include <string_view>
#include <utility>

bool TestLockPointValidity(CChain& active_chain, const LockPoints& lp)
{
    AssertLockHeld(cs_main);
    // If there are relative lock times then the maxInputBlock will be set
    // If there are no relative lock times, the LockPoints don't depend on the chain
    if (lp.maxInputBlock) {
        // Check whether active_chain is an extension of the block at which the LockPoints
        // calculation was valid.  If not LockPoints are no longer valid
        if (!active_chain.Contains(lp.maxInputBlock)) {
            return false;
        }
    }

    // LockPoints still valid
    return true;
}

std::vector<TxEntry::TxEntryRef> CTxMemPool::GetChildrenOf(const TxEntry& tx)
{
    LOCK(cs);
    WITH_FRESH_EPOCH(m_epoch);
    std::vector<TxEntry::TxEntryRef> children;
    const CTxMemPoolEntry& entry = dynamic_cast<const CTxMemPoolEntry&>(tx);
    const uint256& h = entry.GetTx().GetHash();
    auto iter = mapNextTx.lower_bound(COutPoint(Txid::FromUint256(h), 0));
    for (; iter != mapNextTx.end() && iter->first->hash == h; ++iter) {
        const uint256& childHash = iter->second->GetHash();
        txiter childIter = mapTx.find(childHash);
        assert(childIter != mapTx.end());
        if (!visited(childIter)) {
            children.emplace_back(*childIter);
        }
    }
    return children;
}

void CTxMemPool::UpdateTransactionsFromBlock(const std::vector<uint256>& vHashesToUpdate)
{
    AssertLockHeld(cs);

    // Use a set for lookups into vHashesToUpdate (these entries are already
    // accounted for in the state of their ancestors)
    std::set<uint256> setAlreadyIncluded(vHashesToUpdate.begin(), vHashesToUpdate.end());

    // Iterate in reverse, so that whenever we are looking at a transaction
    // we are sure that all in-mempool descendants have already been processed.
    for (const uint256 &hash : reverse_iterate(vHashesToUpdate)) {
        // calculate children from mapNextTx
        txiter it = mapTx.find(hash);
        if (it == mapTx.end()) {
            continue;
        }
        auto iter = mapNextTx.lower_bound(COutPoint(Txid::FromUint256(hash), 0));
        // First calculate the children, and update CTxMemPoolEntry::m_children to
        // include them, and update their CTxMemPoolEntry::m_parents to include this tx.
        // we cache the in-mempool children to avoid duplicate updates
        {
            WITH_FRESH_EPOCH(m_epoch);
            for (; iter != mapNextTx.end() && iter->first->hash == hash; ++iter) {
                const uint256 &childHash = iter->second->GetHash();
                txiter childIter = mapTx.find(childHash);
                assert(childIter != mapTx.end());
                // We can skip updating entries we've encountered before or that
                // are in the block (which are already accounted for).
                if (!visited(childIter) && !setAlreadyIncluded.count(childHash)) {
                    UpdateChild(it, childIter, true);
                    UpdateParent(childIter, it, true);
                }
            }
        }
    }

    std::vector<TxEntry::TxEntryRef> parent_transactions;

    // Iterate in reverse, so that whenever we are looking at a transaction
    // we are sure that all in-mempool descendants have already been processed.
    for (const uint256 &hash : reverse_iterate(vHashesToUpdate)) {
        // calculate children from mapNextTx
        txiter it = mapTx.find(hash);
        if (it == mapTx.end()) {
            continue;
        }
        parent_transactions.emplace_back(*it);
    }

    std::vector<TxEntry::TxEntryRef> txs_to_remove;

    txgraph.AddParentTxs(parent_transactions, m_opts.limits, [&](const TxEntry &tx) {return this->GetChildrenOf(tx);}, txs_to_remove);

    for (auto &tx : txs_to_remove) {
        auto it = mapTx.iterator_to(dynamic_cast<const CTxMemPoolEntry&>(tx.get()));
        UpdateForRemoveFromMempool({it}, false);
        removeUnchecked(it, MemPoolRemovalReason::SIZELIMIT);
    }
}

CTxMemPool::setEntries CTxMemPool::CalculateMemPoolAncestors(
    const CTxMemPoolEntry &entry,
    bool fSearchForParents /* = true */) const
{
    auto ancestors = CalculateMemPoolAncestorsFast(entry, fSearchForParents);

    setEntries ret;
    for (auto ancestor : ancestors) {
        ret.insert(mapTx.iterator_to(dynamic_cast<const CTxMemPoolEntry&>(ancestor.get())));
    }
    return ret;
}

std::vector<CTxMemPoolEntry::CTxMemPoolEntryRef> CTxMemPool::CalculateMemPoolAncestorsFast(const CTxMemPoolEntry &entry, bool fSearchForParents) const
{
    std::vector<TxEntry::TxEntryRef> ancestors = CalculateAncestors(entry, fSearchForParents);

    std::vector<CTxMemPoolEntry::CTxMemPoolEntryRef> ret;
    ret.reserve(ancestors.size());
    for (auto ancestor : ancestors) {
        ret.emplace_back(dynamic_cast<const CTxMemPoolEntry&>(ancestor.get()));
    }
    return ret;
}

// This function is just for internal use, not exposed publicly
std::vector<TxEntry::TxEntryRef> CTxMemPool::CalculateAncestors(const CTxMemPoolEntry& entry, bool fSearchForParents) const
{
    LOCK(cs);
    std::vector<TxEntry::TxEntryRef> parents;
    if (fSearchForParents) {
        parents = CalculateParents(entry);
    } else {
        for (auto p : entry.GetTxEntryParents()) {
            parents.emplace_back(p);
        }
    }

    return txgraph.GetAncestors(parents);
}

util::Result<void> CTxMemPool::CheckPackageLimits(const Package& package,
                                                  const int64_t total_vsize) const
{
    CTxMemPoolEntry::Parents staged_ancestors;
    for (const auto& tx : package) {
        for (const auto& input : tx->vin) {
            std::optional<txiter> piter = GetIter(input.prevout.hash);
            if (piter) {
                staged_ancestors.insert(**piter);
            }
        }
    }

    auto cluster_result{CheckClusterSizeLimit(total_vsize, package.size(), m_opts.limits, staged_ancestors)};
    if (!cluster_result) {
        return util::Error{Untranslated(util::ErrorString(cluster_result).original)};
    }
    return {};
}

util::Result<bool> CTxMemPool::CheckClusterSizeAgainstLimits(const std::vector<TxEntry::TxEntryRef>& parents, int64_t count, int64_t vbytes, GraphLimits limits) const
{
    int64_t total_cluster_count{0};
    int64_t total_cluster_vbytes{0};

    txgraph.GetClusterSize(parents, total_cluster_vbytes, total_cluster_count);

    total_cluster_count += count;
    total_cluster_vbytes += vbytes;

    if (total_cluster_count > limits.cluster_count) {
        return util::Error{Untranslated(strprintf("too many unconfirmed transactions in the cluster [limit: %ld]", limits.cluster_count))};
    }
    if (total_cluster_vbytes > limits.cluster_size_vbytes) {
        return util::Error{Untranslated(strprintf("exceeds cluster size limit [limit: %d]", limits.cluster_size_vbytes))};
    }
    return true;
}

util::Result<bool> CTxMemPool::CheckClusterSizeLimit(int64_t entry_size, size_t entry_count,
        const Limits& limits, CTxMemPoolEntry::Parents all_parents) const
{
    std::vector<TxEntry::TxEntryRef> parents;
    for (auto p : all_parents) {
        parents.emplace_back(p.get());
    }

    return CheckClusterSizeAgainstLimits(parents, entry_count, entry_size, limits);
}

bool CTxMemPool::HasDescendants(const Txid& txid) const
{
    LOCK(cs);
    auto entry = GetEntry(txid);
    if (!entry) return false;
    return txgraph.HasDescendants(*entry);
}

std::vector<TxEntry::TxEntryRef> CTxMemPool::CalculateParents(const CTransaction& tx) const
{
    std::vector<TxEntry::TxEntryRef> ret;
    {
        WITH_FRESH_EPOCH(m_epoch);
        for (const CTxIn &txin : tx.vin) {
            std::optional<txiter> piter = GetIter(txin.prevout.hash);
            if (piter && !visited(*piter)) {
                ret.emplace_back(**piter);
            }
        }
    }
    return ret;
}

std::vector<TxEntry::TxEntryRef> CTxMemPool::CalculateParents(const CTxMemPoolEntry &entry) const
{
    return CalculateParents(entry.GetTx());
}

void CTxMemPool::UpdateAncestorsOf(bool add, txiter it)
{
    const CTxMemPoolEntry::Parents& parents = it->GetMemPoolParentsConst();
    // add or remove this tx as a child of each parent
    for (const CTxMemPoolEntry& parent : parents) {
        UpdateChild(mapTx.iterator_to(parent), it, add);
    }
}

void CTxMemPool::UpdateChildrenForRemoval(txiter it)
{
    const CTxMemPoolEntry::Children& children = it->GetMemPoolChildrenConst();
    for (const CTxMemPoolEntry& updateIt : children) {
        UpdateParent(mapTx.iterator_to(updateIt), it, false);
    }
}

void CTxMemPool::UpdateForRemoveFromMempool(const setEntries &entriesToRemove, bool updateDescendants)
{
    for (txiter removeIt : entriesToRemove) {
        // Note that UpdateAncestorsOf severs the child links that point to
        // removeIt in the entries for the parents of removeIt.
        UpdateAncestorsOf(false, removeIt);
    }
    // After updating all the ancestor sizes, we can now sever the link between each
    // transaction being removed and any mempool children (ie, update CTxMemPoolEntry::m_parents
    // for each direct child of a transaction being removed).
    for (txiter removeIt : entriesToRemove) {
        UpdateChildrenForRemoval(removeIt);
    }
}

static CTxMemPool::Options&& Flatten(CTxMemPool::Options&& opts, bilingual_str& error)
{
    opts.check_ratio = std::clamp<int>(opts.check_ratio, 0, 1'000'000);
    int64_t descendant_limit_bytes = opts.limits.descendant_size_vbytes * 40;
    if (opts.max_size_bytes < 0 || opts.max_size_bytes < descendant_limit_bytes) {
        error = strprintf(_("-maxmempool must be at least %d MB"), std::ceil(descendant_limit_bytes / 1'000'000.0));
    }
    return std::move(opts);
}

CTxMemPool::CTxMemPool(Options opts, bilingual_str& error)
    : m_opts{Flatten(std::move(opts), error)}
{
}

bool CTxMemPool::isSpent(const COutPoint& outpoint) const
{
    LOCK(cs);
    return mapNextTx.count(outpoint);
}

unsigned int CTxMemPool::GetTransactionsUpdated() const
{
    return nTransactionsUpdated;
}

void CTxMemPool::AddTransactionsUpdated(unsigned int n)
{
    nTransactionsUpdated += n;
}

void CTxMemPool::addUnchecked(const CTxMemPoolEntry &entry)
{
    // Add to memory pool without checking anything.
    // Used by AcceptToMemoryPool(), which DOES do
    // all the appropriate checks.
    indexed_transaction_set::iterator newit = mapTx.emplace(CTxMemPoolEntry::ExplicitCopy, entry).first;

    // Update transaction for any feeDelta created by PrioritiseTransaction
    CAmount delta{0};
    ApplyDelta(entry.GetTx().GetHash(), delta);
    // The following call to UpdateModifiedFee assumes no previous fee modifications
    Assume(entry.GetFee() == entry.GetModifiedFee());
    if (delta) {
        mapTx.modify(newit, [&delta](CTxMemPoolEntry& e) { e.UpdateModifiedFee(delta); });
    }

    // Update cachedInnerUsage to include contained transaction's usage.
    // (When we update the entry for in-mempool parents, memory usage will be
    // further updated.)
    cachedInnerUsage += entry.DynamicMemoryUsage();

    const CTransaction& tx = newit->GetTx();
    std::set<Txid> setParentTransactions;
    for (unsigned int i = 0; i < tx.vin.size(); i++) {
        mapNextTx.insert(std::make_pair(&tx.vin[i].prevout, &tx));
        setParentTransactions.insert(tx.vin[i].prevout.hash);
    }

    // Add parents to parent set for this transaction.
    std::vector<TxEntry::TxEntryRef> parents;
    for (const auto& pit : GetIterSet(setParentTransactions)) {
        parents.emplace_back(*pit);
    }

    // Don't bother worrying about child transactions of this one.
    // Normal case of a new transaction arriving is that there can't be any
    // children, because such children would be orphans.
    // An exception to that is if a transaction enters that used to be in a block.
    // In that case, our disconnect block logic will call UpdateTransactionsFromBlock
    // to clean up the mess we're leaving here.

    // Update ancestors with information about this tx
    for (const auto& pit : GetIterSet(setParentTransactions)) {
        UpdateParent(newit, pit, true);
    }
    UpdateAncestorsOf(true, newit);

    nTransactionsUpdated++;
    totalTxSize += entry.GetTxSize();
    m_total_fee += entry.GetFee();

    txns_randomized.emplace_back(newit->GetSharedTx());
    newit->idx_randomized = txns_randomized.size() - 1;

    // Add this transaction to the graph.
    txgraph.AddTx(&(const_cast<CTxMemPoolEntry&>(*newit)), newit->GetTxSize(), newit->GetModifiedFee(), parents);

    TRACE3(mempool, added,
        entry.GetTx().GetHash().data(),
        entry.GetTxSize(),
        entry.GetFee()
    );
}

void CTxMemPool::removeUnchecked(txiter it, MemPoolRemovalReason reason)
{
    // We increment mempool sequence value no matter removal reason
    // even if not directly reported below.
    uint64_t mempool_sequence = GetAndIncrementSequence();

    if (reason != MemPoolRemovalReason::BLOCK && m_opts.signals) {
        // Notify clients that a transaction has been removed from the mempool
        // for any reason except being included in a block. Clients interested
        // in transactions included in blocks can subscribe to the BlockConnected
        // notification.
        m_opts.signals->TransactionRemovedFromMempool(it->GetSharedTx(), reason, mempool_sequence);
    }
    TRACE5(mempool, removed,
        it->GetTx().GetHash().data(),
        RemovalReasonToString(reason).c_str(),
        it->GetTxSize(),
        it->GetFee(),
        std::chrono::duration_cast<std::chrono::duration<std::uint64_t>>(it->GetTime()).count()
    );

    for (const CTxIn& txin : it->GetTx().vin)
        mapNextTx.erase(txin.prevout);

    RemoveUnbroadcastTx(it->GetTx().GetHash(), true /* add logging because unchecked */);

    if (txns_randomized.size() > 1) {
        // Update idx_randomized of the to-be-moved entry.
        Assert(GetEntry(txns_randomized.back()->GetHash()))->idx_randomized = it->idx_randomized;
        // Remove entry from txns_randomized by replacing it with the back and deleting the back.
        txns_randomized[it->idx_randomized] = std::move(txns_randomized.back());
        txns_randomized.pop_back();
        if (txns_randomized.size() * 2 < txns_randomized.capacity())
            txns_randomized.shrink_to_fit();
    } else
        txns_randomized.clear();

    totalTxSize -= it->GetTxSize();
    m_total_fee -= it->GetFee();
    cachedInnerUsage -= it->DynamicMemoryUsage();
    cachedInnerUsage -= memusage::DynamicUsage(it->GetMemPoolParentsConst()) + memusage::DynamicUsage(it->GetMemPoolChildrenConst());
    mapTx.erase(it);
    nTransactionsUpdated++;
}

// Calculates descendants of entry that are not already in setDescendants, and adds to
// setDescendants. Assumes entryit is already a tx in the mempool and CTxMemPoolEntry::m_children
// is correct for tx and all descendants.
// Also assumes that if an entry is in setDescendants already, then all
// in-mempool descendants of it are already in setDescendants as well, so that we
// can save time by not iterating over those entries.
void CTxMemPool::CalculateDescendants(txiter entryit, setEntries& setDescendants) const
{
    setEntries stage;
    if (setDescendants.count(entryit) == 0) {
        stage.insert(entryit);
    }
    // Traverse down the children of entry, only adding children that are not
    // accounted for in setDescendants already (because those children have either
    // already been walked, or will be walked in this iteration).
    while (!stage.empty()) {
        txiter it = *stage.begin();
        setDescendants.insert(it);
        stage.erase(it);

        const CTxMemPoolEntry::Children& children = it->GetMemPoolChildrenConst();
        for (const CTxMemPoolEntry& child : children) {
            txiter childiter = mapTx.iterator_to(child);
            if (!setDescendants.count(childiter)) {
                stage.insert(childiter);
            }
        }
    }
}

void CTxMemPool::removeRecursive(const CTransaction &origTx, MemPoolRemovalReason reason)
{
    // Remove transaction from memory pool
    AssertLockHeld(cs);
        setEntries txToRemove;
        txiter origit = mapTx.find(origTx.GetHash());
        if (origit != mapTx.end()) {
            txToRemove.insert(origit);
        } else {
            // When recursively removing but origTx isn't in the mempool
            // be sure to remove any children that are in the pool. This can
            // happen during chain re-orgs if origTx isn't re-accepted into
            // the mempool for any reason.
            for (unsigned int i = 0; i < origTx.vout.size(); i++) {
                auto it = mapNextTx.find(COutPoint(origTx.GetHash(), i));
                if (it == mapNextTx.end())
                    continue;
                txiter nextit = mapTx.find(it->second->GetHash());
                assert(nextit != mapTx.end());
                txToRemove.insert(nextit);
            }
        }
        setEntries setAllRemoves;
        for (txiter it : txToRemove) {
            CalculateDescendants(it, setAllRemoves);
        }

        RemoveStaged(setAllRemoves, false, reason);
}

void CTxMemPool::removeForReorg(CChain& chain, std::function<bool(txiter)> check_final_and_mature)
{
    // Remove transactions spending a coinbase which are now immature and no-longer-final transactions
    AssertLockHeld(cs);
    AssertLockHeld(::cs_main);

    setEntries txToRemove;
    for (indexed_transaction_set::const_iterator it = mapTx.begin(); it != mapTx.end(); it++) {
        if (check_final_and_mature(it)) txToRemove.insert(it);
    }
    setEntries setAllRemoves;
    for (txiter it : txToRemove) {
        CalculateDescendants(it, setAllRemoves);
    }
    RemoveStaged(setAllRemoves, false, MemPoolRemovalReason::REORG);
    for (indexed_transaction_set::const_iterator it = mapTx.begin(); it != mapTx.end(); it++) {
        assert(TestLockPointValidity(chain, it->GetLockPoints()));
    }
}

void CTxMemPool::removeConflicts(const CTransaction &tx)
{
    // Remove transactions which depend on inputs of tx, recursively
    AssertLockHeld(cs);
    for (const CTxIn &txin : tx.vin) {
        auto it = mapNextTx.find(txin.prevout);
        if (it != mapNextTx.end()) {
            const CTransaction &txConflict = *it->second;
            if (txConflict != tx)
            {
                ClearPrioritisation(txConflict.GetHash());
                removeRecursive(txConflict, MemPoolRemovalReason::CONFLICT);
            }
        }
    }
}

/**
 * Called when a block is connected. Removes from mempool.
 */

void CTxMemPool::removeForBlock(const std::vector<CTransactionRef>& vtx, unsigned int nBlockHeight)
{
    AssertLockHeld(cs);
    std::vector<RemovedMempoolTransactionInfo> txs_removed_for_block;
    txs_removed_for_block.reserve(vtx.size());
    std::vector<TxEntry::TxEntryRef> txs_to_remove;

    // Look up all iterators, and grab the transaction data that we'll need for
    // the callback later.
    for (const auto& tx : vtx)
    {
        txiter it = mapTx.find(tx->GetHash());
        if (it != mapTx.end()) {
            txs_removed_for_block.emplace_back(*it);
            txs_to_remove.emplace_back(*it);
        }
    }

    txgraph.RemoveBatch(txs_to_remove);

    for (auto tx : txs_to_remove) {
        txiter it = mapTx.iterator_to(dynamic_cast<const CTxMemPoolEntry&>(tx.get()));
        UpdateForRemoveFromMempool({it}, false);
        removeUnchecked(it, MemPoolRemovalReason::BLOCK);
    }

    for (const auto& tx : vtx) {
        removeConflicts(*tx);
        ClearPrioritisation(tx->GetHash());
    }
    if (m_opts.signals) {
        m_opts.signals->MempoolTransactionsRemovedForBlock(txs_removed_for_block, nBlockHeight);
    }
    lastRollingFeeUpdate = GetTime();
    blockSinceLastRollingFeeBump = true;
}

void CTxMemPool::check(const CCoinsViewCache& active_coins_tip, int64_t spendheight) const
{
    if (m_opts.check_ratio == 0) return;

    if (GetRand(m_opts.check_ratio) >= 1) return;

    AssertLockHeld(::cs_main);
    LOCK(cs);
    LogPrint(BCLog::MEMPOOL, "Checking mempool with %u transactions and %u inputs\n", (unsigned int)mapTx.size(), (unsigned int)mapNextTx.size());

    txgraph.Check(GraphLimits{std::numeric_limits<int64_t>::max(), std::numeric_limits<int64_t>::max()});

    uint64_t checkTotal = 0;
    CAmount check_total_fee{0};
    uint64_t innerUsage = 0;

    CCoinsViewCache mempoolDuplicate(const_cast<CCoinsViewCache*>(&active_coins_tip));

    for (const auto& it : GetSortedScoreWithTopology()) {
        checkTotal += it->GetTxSize();
        check_total_fee += it->GetFee();
        innerUsage += it->DynamicMemoryUsage();
        const CTransaction& tx = it->GetTx();
        innerUsage += memusage::DynamicUsage(it->GetMemPoolParentsConst()) + memusage::DynamicUsage(it->GetMemPoolChildrenConst());
        CTxMemPoolEntry::Parents setParentCheck;
        for (const CTxIn &txin : tx.vin) {
            // Check that every mempool transaction's inputs refer to available coins, or other mempool tx's.
            indexed_transaction_set::const_iterator it2 = mapTx.find(txin.prevout.hash);
            if (it2 != mapTx.end()) {
                const CTransaction& tx2 = it2->GetTx();
                assert(tx2.vout.size() > txin.prevout.n && !tx2.vout[txin.prevout.n].IsNull());
                setParentCheck.insert(*it2);
            }
            // We are iterating through the mempool entries sorted topologically and by score.
            // All parents must have been checked before their children and their coins added to
            // the mempoolDuplicate coins cache.
            assert(mempoolDuplicate.HaveCoin(txin.prevout));
            // Check whether its inputs are marked in mapNextTx.
            auto it3 = mapNextTx.find(txin.prevout);
            assert(it3 != mapNextTx.end());
            assert(it3->first == &txin.prevout);
            assert(it3->second == &tx);
        }
        auto comp = [](const CTxMemPoolEntry& a, const CTxMemPoolEntry& b) -> bool {
            return a.GetTx().GetHash() == b.GetTx().GetHash();
        };
        assert(setParentCheck.size() == it->GetMemPoolParentsConst().size());
        assert(std::equal(setParentCheck.begin(), setParentCheck.end(), it->GetMemPoolParentsConst().begin(), comp));

        // Check children against mapNextTx
        CTxMemPoolEntry::Children setChildrenCheck;
        auto iter = mapNextTx.lower_bound(COutPoint(it->GetTx().GetHash(), 0));
        for (; iter != mapNextTx.end() && iter->first->hash == it->GetTx().GetHash(); ++iter) {
            txiter childit = mapTx.find(iter->second->GetHash());
            assert(childit != mapTx.end()); // mapNextTx points to in-mempool transactions
            setChildrenCheck.insert(*childit);
        }
        assert(setChildrenCheck.size() == it->GetMemPoolChildrenConst().size());
        assert(std::equal(setChildrenCheck.begin(), setChildrenCheck.end(), it->GetMemPoolChildrenConst().begin(), comp));

        TxValidationState dummy_state; // Not used. CheckTxInputs() should always pass
        CAmount txfee = 0;
        assert(!tx.IsCoinBase());
        assert(Consensus::CheckTxInputs(tx, dummy_state, mempoolDuplicate, spendheight, txfee));
        for (const auto& input: tx.vin) mempoolDuplicate.SpendCoin(input.prevout);
        AddCoins(mempoolDuplicate, tx, std::numeric_limits<int>::max());
    }
    for (auto it = mapNextTx.cbegin(); it != mapNextTx.cend(); it++) {
        uint256 hash = it->second->GetHash();
        indexed_transaction_set::const_iterator it2 = mapTx.find(hash);
        const CTransaction& tx = it2->GetTx();
        assert(it2 != mapTx.end());
        assert(&tx == it->second);
    }

    assert(totalTxSize == checkTotal);
    assert(m_total_fee == check_total_fee);
    assert(innerUsage == cachedInnerUsage);
}

bool CTxMemPool::CompareMiningScoreWithTopology(const uint256& hasha, const uint256& hashb, bool wtxid)
{
    /* Return `true` if hasha should be considered sooner than hashb. Namely when:
     *   a is not in the mempool, but b is
     *   both are in the mempool and a has a higher mining score than b
     *   both are in the mempool and a appears before b in the same cluster
     */
    LOCK(cs);
    indexed_transaction_set::const_iterator j = wtxid ? get_iter_from_wtxid(hashb) : mapTx.find(hashb);
    if (j == mapTx.end()) return false;
    indexed_transaction_set::const_iterator i = wtxid ? get_iter_from_wtxid(hasha) : mapTx.find(hasha);
    if (i == mapTx.end()) return true;

    return txgraph.CompareMiningScore(*i, *j);
}

std::vector<CTxMemPool::indexed_transaction_set::const_iterator> CTxMemPool::GetSortedScoreWithTopology() const
{
    std::vector<indexed_transaction_set::const_iterator> iters;
    AssertLockHeld(cs);

    iters.reserve(mapTx.size());

    for (indexed_transaction_set::iterator mi = mapTx.begin(); mi != mapTx.end(); ++mi) {
        iters.push_back(mi);
    }
    std::sort(iters.begin(), iters.end(), [this](const CTxMemPool::indexed_transaction_set::const_iterator& a, const CTxMemPool::indexed_transaction_set::const_iterator& b) {
        LOCK(this->cs); // TODO: this is unnecessary, to quiet a compiler warning
        return this->txgraph.CompareMiningScore(*a, *b);
    });
    return iters;
}

static TxMempoolInfo GetInfo(CTxMemPool::indexed_transaction_set::const_iterator it) {
    return TxMempoolInfo{it->GetSharedTx(), it->GetTime(), it->GetFee(), it->GetTxSize(), it->GetModifiedFee() - it->GetFee()};
}

std::vector<CTxMemPoolEntryRef> CTxMemPool::entryAll() const
{
    AssertLockHeld(cs);

    std::vector<CTxMemPoolEntryRef> ret;
    ret.reserve(mapTx.size());
    for (const auto& it : GetSortedScoreWithTopology()) {
        ret.emplace_back(*it);
    }
    return ret;
}

std::vector<TxMempoolInfo> CTxMemPool::infoAll() const
{
    LOCK(cs);
    auto iters = GetSortedScoreWithTopology();

    std::vector<TxMempoolInfo> ret;
    ret.reserve(mapTx.size());
    for (auto it : iters) {
        ret.push_back(GetInfo(it));
    }

    return ret;
}

const CTxMemPoolEntry* CTxMemPool::GetEntry(const Txid& txid) const
{
    AssertLockHeld(cs);
    const auto i = mapTx.find(txid);
    return i == mapTx.end() ? nullptr : &(*i);
}

CTransactionRef CTxMemPool::get(const uint256& hash) const
{
    LOCK(cs);
    indexed_transaction_set::const_iterator i = mapTx.find(hash);
    if (i == mapTx.end())
        return nullptr;
    return i->GetSharedTx();
}

TxMempoolInfo CTxMemPool::info(const GenTxid& gtxid) const
{
    LOCK(cs);
    indexed_transaction_set::const_iterator i = (gtxid.IsWtxid() ? get_iter_from_wtxid(gtxid.GetHash()) : mapTx.find(gtxid.GetHash()));
    if (i == mapTx.end())
        return TxMempoolInfo();
    return GetInfo(i);
}

TxMempoolInfo CTxMemPool::info_for_relay(const GenTxid& gtxid, uint64_t last_sequence) const
{
    LOCK(cs);
    indexed_transaction_set::const_iterator i = (gtxid.IsWtxid() ? get_iter_from_wtxid(gtxid.GetHash()) : mapTx.find(gtxid.GetHash()));
    if (i != mapTx.end() && i->GetSequence() < last_sequence) {
        return GetInfo(i);
    } else {
        return TxMempoolInfo();
    }
}

void CTxMemPool::PrioritiseTransaction(const uint256& hash, const CAmount& nFeeDelta)
{
    {
        LOCK(cs);
        CAmount &delta = mapDeltas[hash];
        delta = SaturatingAdd(delta, nFeeDelta);
        txiter it = mapTx.find(hash);
        if (it != mapTx.end()) {
            mapTx.modify(it, [&nFeeDelta](CTxMemPoolEntry& e) { e.UpdateModifiedFee(nFeeDelta); });

            ++nTransactionsUpdated;

            txgraph.UpdateForPrioritisedTransaction(*it);
        }
        if (delta == 0) {
            mapDeltas.erase(hash);
            LogPrintf("PrioritiseTransaction: %s (%sin mempool) delta cleared\n", hash.ToString(), it == mapTx.end() ? "not " : "");
        } else {
            LogPrintf("PrioritiseTransaction: %s (%sin mempool) fee += %s, new delta=%s\n",
                      hash.ToString(),
                      it == mapTx.end() ? "not " : "",
                      FormatMoney(nFeeDelta),
                      FormatMoney(delta));
        }
    }
}

void CTxMemPool::ApplyDelta(const uint256& hash, CAmount &nFeeDelta) const
{
    AssertLockHeld(cs);
    std::map<uint256, CAmount>::const_iterator pos = mapDeltas.find(hash);
    if (pos == mapDeltas.end())
        return;
    const CAmount &delta = pos->second;
    nFeeDelta += delta;
}

void CTxMemPool::ClearPrioritisation(const uint256& hash)
{
    AssertLockHeld(cs);
    mapDeltas.erase(hash);
}

std::vector<CTxMemPool::delta_info> CTxMemPool::GetPrioritisedTransactions() const
{
    AssertLockNotHeld(cs);
    LOCK(cs);
    std::vector<delta_info> result;
    result.reserve(mapDeltas.size());
    for (const auto& [txid, delta] : mapDeltas) {
        const auto iter{mapTx.find(txid)};
        const bool in_mempool{iter != mapTx.end()};
        std::optional<CAmount> modified_fee;
        if (in_mempool) modified_fee = iter->GetModifiedFee();
        result.emplace_back(delta_info{in_mempool, delta, modified_fee, txid});
    }
    return result;
}

const CTransaction* CTxMemPool::GetConflictTx(const COutPoint& prevout) const
{
    const auto it = mapNextTx.find(prevout);
    return it == mapNextTx.end() ? nullptr : it->second;
}

std::optional<CTxMemPool::txiter> CTxMemPool::GetIter(const uint256& txid) const
{
    auto it = mapTx.find(txid);
    if (it != mapTx.end()) return it;
    return std::nullopt;
}

CTxMemPool::setEntries CTxMemPool::GetIterSet(const std::set<Txid>& hashes) const
{
    CTxMemPool::setEntries ret;
    for (const auto& h : hashes) {
        const auto mi = GetIter(h);
        if (mi) ret.insert(*mi);
    }
    return ret;
}

std::vector<CTxMemPool::txiter> CTxMemPool::GetIterVec(const std::vector<uint256>& txids) const
{
    AssertLockHeld(cs);
    std::vector<txiter> ret;
    ret.reserve(txids.size());
    for (const auto& txid : txids) {
        const auto it{GetIter(txid)};
        if (!it) return {};
        ret.push_back(*it);
    }
    return ret;
}

bool CTxMemPool::HasNoInputsOf(const CTransaction &tx) const
{
    for (unsigned int i = 0; i < tx.vin.size(); i++)
        if (exists(GenTxid::Txid(tx.vin[i].prevout.hash)))
            return false;
    return true;
}

CCoinsViewMemPool::CCoinsViewMemPool(CCoinsView* baseIn, const CTxMemPool& mempoolIn) : CCoinsViewBacked(baseIn), mempool(mempoolIn) { }

bool CCoinsViewMemPool::GetCoin(const COutPoint &outpoint, Coin &coin) const {
    // Check to see if the inputs are made available by another tx in the package.
    // These Coins would not be available in the underlying CoinsView.
    if (auto it = m_temp_added.find(outpoint); it != m_temp_added.end()) {
        coin = it->second;
        return true;
    }

    // If an entry in the mempool exists, always return that one, as it's guaranteed to never
    // conflict with the underlying cache, and it cannot have pruned entries (as it contains full)
    // transactions. First checking the underlying cache risks returning a pruned entry instead.
    CTransactionRef ptx = mempool.get(outpoint.hash);
    if (ptx) {
        if (outpoint.n < ptx->vout.size()) {
            coin = Coin(ptx->vout[outpoint.n], MEMPOOL_HEIGHT, false);
            m_non_base_coins.emplace(outpoint);
            return true;
        } else {
            return false;
        }
    }
    return base->GetCoin(outpoint, coin);
}

void CCoinsViewMemPool::PackageAddTransaction(const CTransactionRef& tx)
{
    for (unsigned int n = 0; n < tx->vout.size(); ++n) {
        m_temp_added.emplace(COutPoint(tx->GetHash(), n), Coin(tx->vout[n], MEMPOOL_HEIGHT, false));
        m_non_base_coins.emplace(tx->GetHash(), n);
    }
}
void CCoinsViewMemPool::Reset()
{
    m_temp_added.clear();
    m_non_base_coins.clear();
}

size_t CTxMemPool::DynamicMemoryUsage() const {
    LOCK(cs);
    // Estimate the overhead of mapTx to be 9 pointers + an allocation, as no exact formula for boost::multi_index_contained is implemented.
    return memusage::MallocUsage(sizeof(CTxMemPoolEntry) + 9 * sizeof(void*)) * mapTx.size() + memusage::DynamicUsage(mapNextTx) + memusage::DynamicUsage(mapDeltas) + memusage::DynamicUsage(txns_randomized) + txgraph.GetInnerUsage() + cachedInnerUsage;
}

void CTxMemPool::RemoveUnbroadcastTx(const uint256& txid, const bool unchecked) {
    LOCK(cs);

    if (m_unbroadcast_txids.erase(txid))
    {
        LogPrint(BCLog::MEMPOOL, "Removed %i from set of unbroadcast txns%s\n", txid.GetHex(), (unchecked ? " before confirmation that txn was sent out" : ""));
    }
}

void CTxMemPool::RemoveStaged(setEntries &stage, bool updateDescendants, MemPoolRemovalReason reason) {
    AssertLockHeld(cs);

    std::vector<TxEntry::TxEntryRef> txs_to_remove;
    for (auto& it : stage) {
        txs_to_remove.emplace_back(*it);
    }
    txgraph.RemoveBatch(txs_to_remove);

    UpdateForRemoveFromMempool(stage, updateDescendants);
    for (txiter it : stage) {
        removeUnchecked(it, reason);
    }
}

int CTxMemPool::Expire(std::chrono::seconds time)
{
    AssertLockHeld(cs);
    indexed_transaction_set::index<entry_time>::type::iterator it = mapTx.get<entry_time>().begin();
    setEntries toremove;
    while (it != mapTx.get<entry_time>().end() && it->GetTime() < time) {
        toremove.insert(mapTx.project<0>(it));
        it++;
    }
    setEntries stage;
    for (txiter removeit : toremove) {
        CalculateDescendants(removeit, stage);
    }
    RemoveStaged(stage, false, MemPoolRemovalReason::EXPIRY);
    return stage.size();
}

void CTxMemPool::UpdateChild(txiter entry, txiter child, bool add)
{
    AssertLockHeld(cs);
    CTxMemPoolEntry::Children s;
    if (add && entry->GetMemPoolChildren().insert(*child).second) {
        cachedInnerUsage += memusage::IncrementalDynamicUsage(s);
    } else if (!add && entry->GetMemPoolChildren().erase(*child)) {
        cachedInnerUsage -= memusage::IncrementalDynamicUsage(s);
    }
}

void CTxMemPool::UpdateParent(txiter entry, txiter parent, bool add)
{
    AssertLockHeld(cs);
    CTxMemPoolEntry::Parents s;
    if (add && entry->GetMemPoolParents().insert(*parent).second) {
        cachedInnerUsage += memusage::IncrementalDynamicUsage(s);
    } else if (!add && entry->GetMemPoolParents().erase(*parent)) {
        cachedInnerUsage -= memusage::IncrementalDynamicUsage(s);
    }
}

CFeeRate CTxMemPool::GetMinFee(size_t sizelimit) const {
    LOCK(cs);
    if (!blockSinceLastRollingFeeBump || rollingMinimumFeeRate == 0)
        return CFeeRate(llround(rollingMinimumFeeRate));

    int64_t time = GetTime();
    if (time > lastRollingFeeUpdate + 10) {
        double halflife = ROLLING_FEE_HALFLIFE;
        if (DynamicMemoryUsage() < sizelimit / 4)
            halflife /= 4;
        else if (DynamicMemoryUsage() < sizelimit / 2)
            halflife /= 2;

        rollingMinimumFeeRate = rollingMinimumFeeRate / pow(2.0, (time - lastRollingFeeUpdate) / halflife);
        lastRollingFeeUpdate = time;

        if (rollingMinimumFeeRate < (double)m_opts.incremental_relay_feerate.GetFeePerK() / 2) {
            rollingMinimumFeeRate = 0;
            return CFeeRate(0);
        }
    }
    return std::max(CFeeRate(llround(rollingMinimumFeeRate)), m_opts.incremental_relay_feerate);
}

void CTxMemPool::trackPackageRemoved(const CFeeRate& rate) {
    AssertLockHeld(cs);
    if (rate.GetFeePerK() > rollingMinimumFeeRate) {
        rollingMinimumFeeRate = rate.GetFeePerK();
        blockSinceLastRollingFeeBump = false;
    }
}

void CTxMemPool::TrimToSize(size_t sizelimit, std::vector<COutPoint>* pvNoSpendsRemaining) {
    AssertLockHeld(cs);

    unsigned nTxnRemoved = 0;
    CFeeRate maxFeeRateRemoved(0);

    Trimmer trimmer(&txgraph);

    while (!mapTx.empty() && DynamicMemoryUsage() > sizelimit) {
        std::vector<TxEntry::TxEntryRef> txs_to_remove;
        CFeeRate removed = trimmer.RemoveWorstChunk(txs_to_remove);

        // We set the new mempool min fee to the feerate of the removed set, plus the
        // "minimum reasonable fee rate" (ie some value under which we consider txn
        // to have 0 fee). This way, we don't allow txn to enter mempool with feerate
        // equal to txn which were removed with no block in between.
        removed += m_opts.incremental_relay_feerate;
        trackPackageRemoved(removed);
        maxFeeRateRemoved = std::max(maxFeeRateRemoved, removed);

        nTxnRemoved += txs_to_remove.size();

        std::vector<CTransaction> txn;
        if (pvNoSpendsRemaining) {
            txn.reserve(txs_to_remove.size());
            for (auto tx_entry_ref : txs_to_remove)
                txn.emplace_back(dynamic_cast<const CTxMemPoolEntry&>(tx_entry_ref.get()).GetTx());
        }

        setEntries stage;
        for (auto tx : txs_to_remove) {
            stage.insert(mapTx.iterator_to(dynamic_cast<const CTxMemPoolEntry&>(tx.get())));
        }
        UpdateForRemoveFromMempool(stage, false);
        for (auto e : stage) {
            removeUnchecked(e, MemPoolRemovalReason::SIZELIMIT);
        }
        if (pvNoSpendsRemaining) {
            for (const CTransaction& tx : txn) {
                for (const CTxIn& txin : tx.vin) {
                    if (exists(GenTxid::Txid(txin.prevout.hash))) continue;
                    pvNoSpendsRemaining->push_back(txin.prevout);
                }
            }
        }
    }

    if (maxFeeRateRemoved > CFeeRate(0)) {
        LogPrint(BCLog::MEMPOOL, "Removed %u txn, rolling minimum fee bumped to %s\n", nTxnRemoved, maxFeeRateRemoved.ToString());
    }
}

void CTxMemPool::CalculateAncestorData(const CTxMemPoolEntry& entry, size_t& ancestor_count, size_t& ancestor_size, CAmount& ancestor_fees) const
{
    std::vector<TxEntry::TxEntryRef> ancestors = txgraph.GetAncestors({entry});

    ancestor_count = ancestors.size();
    ancestor_size = 0;
    ancestor_fees = 0;
    for (auto tx: ancestors) {
        ancestor_size += tx.get().GetTxSize();
        ancestor_fees += tx.get().GetModifiedFee();
    }
}

void CTxMemPool::CalculateDescendantData(const CTxMemPoolEntry& entry, size_t& descendant_count, size_t& descendant_size, CAmount& descendant_fees) const
{
    std::vector<TxEntry::TxEntryRef> descendants = txgraph.GetDescendants({entry});
    descendant_count = descendants.size();
    descendant_size = 0;
    descendant_fees = 0;

    for (auto tx: descendants) {
        descendant_size += tx.get().GetTxSize();
        descendant_fees += tx.get().GetModifiedFee();
    }
}

void CTxMemPool::GetTransactionAncestry(const uint256& txid, size_t& ancestors, size_t& clustersize, size_t* const ancestorsize, CAmount* const ancestorfees) const {
    LOCK(cs);
    auto it = mapTx.find(txid);
    ancestors = clustersize = 0;
    if (it != mapTx.end()) {
        size_t dummysize{0};
        CAmount dummyfees{0};
        CalculateAncestorData(*it, ancestors, ancestorsize ? *ancestorsize :
                dummysize, ancestorfees ? *ancestorfees : dummyfees);
        clustersize = txgraph.GetClusterCount(*it);
    }
}

bool CTxMemPool::GetLoadTried() const
{
    LOCK(cs);
    return m_load_tried;
}

void CTxMemPool::SetLoadTried(bool load_tried)
{
    LOCK(cs);
    m_load_tried = load_tried;
}

std::vector<CTxMemPool::txiter> CTxMemPool::GatherClusters(const std::vector<uint256>& txids) const
{
    AssertLockHeld(cs);
    std::vector<txiter> clustered_txs{GetIterVec(txids)};
    // Use epoch: visiting an entry means we have added it to the clustered_txs vector. It does not
    // necessarily mean the entry has been processed.
    WITH_FRESH_EPOCH(m_epoch);
    for (const auto& it : clustered_txs) {
        visited(it);
    }
    // i = index of where the list of entries to process starts
    for (size_t i{0}; i < clustered_txs.size(); ++i) {
        // DoS protection: if there are 500 or more entries to process, just quit.
        if (clustered_txs.size() > 500) return {};
        const txiter& tx_iter = clustered_txs.at(i);
        for (const auto& entries : {tx_iter->GetMemPoolParentsConst(), tx_iter->GetMemPoolChildrenConst()}) {
            for (const CTxMemPoolEntry& entry : entries) {
                const auto entry_it = mapTx.iterator_to(entry);
                if (!visited(entry_it)) {
                    clustered_txs.push_back(entry_it);
                }
            }
        }
    }
    return clustered_txs;
}

util::Result<std::pair<std::vector<FeeFrac>, std::vector<FeeFrac>>> CTxMemPool::CalculateChunksForRBF(std::vector<std::pair<CTxMemPoolEntry*, CAmount>> new_entries, const setEntries& direct_conflicts, const setEntries& all_conflicts)
{
    std::vector<FeeFrac> old_diagram;
    std::vector<FeeFrac> new_diagram;

    std::vector<CAmount> old_fees;


    for (auto& e : new_entries) {
        CTxMemPoolEntry &entry = *e.first;
        old_fees.push_back(entry.GetModifiedFee());
        entry.m_modified_fee = e.second;
    }

    auto cleanup = [&]() {
        for (size_t i = 0; i < new_entries.size(); i++) {
            CTxMemPoolEntry &entry = *(new_entries[i].first);
            entry.m_modified_fee = old_fees[i];
        }
    };

    std::vector<TxEntry::TxEntryRef> to_remove;
    for (auto it : all_conflicts) {
        to_remove.emplace_back(*it);
    }

    TxGraphChangeSet changeset(&txgraph, m_opts.limits, to_remove);
    std::map<uint256, CTxMemPoolEntry*> new_entries_map;
    for (auto& e : new_entries) {
        auto parents = CalculateParents((*e.first));
        // Add in any parents from new_entries
        std::set<CTxMemPoolEntry*> additional_parents;
        for (auto& input: e.first->GetTx().vin) {
            auto it = new_entries_map.find(input.prevout.hash);
            if (it != new_entries_map.end()) {
                additional_parents.insert(it->second);
            }
        }
        for (auto p : additional_parents) {
            parents.emplace_back(*p);
        }
        if (!changeset.AddTx(*(e.first), parents)) {
            cleanup();
            return util::Error{Untranslated("cluster size limit exceeded")};
        }
        new_entries_map.insert({e.first->GetTx().GetHash(), e.first});
    }
    changeset.GetFeerateDiagramOld(old_diagram);
    changeset.GetFeerateDiagramNew(new_diagram);

    cleanup();
    return std::make_pair(old_diagram, new_diagram);
}
