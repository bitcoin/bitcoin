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
#include <ranges>
#include <string_view>
#include <utility>

TRACEPOINT_SEMAPHORE(mempool, added);
TRACEPOINT_SEMAPHORE(mempool, removed);

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

std::vector<CTxMemPoolEntry::CTxMemPoolEntryRef> CTxMemPool::GetChildren(const CTxMemPoolEntry& entry) const
{
    LOCK(cs);
    std::vector<CTxMemPoolEntry::CTxMemPoolEntryRef> ret;
    setEntries children;
    auto iter = mapNextTx.lower_bound(COutPoint(entry.GetTx().GetHash(), 0));
    for (; iter != mapNextTx.end() && iter->first->hash == entry.GetTx().GetHash(); ++iter) {
        children.insert(iter->second);
    }
    for (const auto& child : children) {
        ret.emplace_back(*child);
    }
    return ret;
}

std::vector<CTxMemPoolEntry::CTxMemPoolEntryRef> CTxMemPool::GetParents(const CTxMemPoolEntry& entry) const
{
    LOCK(cs);
    std::vector<CTxMemPoolEntry::CTxMemPoolEntryRef> ret;
    std::set<Txid> inputs;
    for (const auto& txin : entry.GetTx().vin) {
        inputs.insert(txin.prevout.hash);
    }
    for (const auto& hash : inputs) {
        std::optional<txiter> piter = GetIter(hash);
        if (piter) {
            ret.emplace_back(**piter);
        }
    }
    return ret;
}

void CTxMemPool::UpdateTransactionsFromBlock(const std::vector<Txid>& vHashesToUpdate)
{
    AssertLockHeld(cs);

    // Iterate in reverse, so that whenever we are looking at a transaction
    // we are sure that all in-mempool descendants have already been processed.
    for (const Txid& hash : vHashesToUpdate | std::views::reverse) {
        // calculate children from mapNextTx
        txiter it = mapTx.find(hash);
        if (it == mapTx.end()) {
            continue;
        }
        auto iter = mapNextTx.lower_bound(COutPoint(hash, 0));
        {
            for (; iter != mapNextTx.end() && iter->first->hash == hash; ++iter) {
                txiter childIter = iter->second;
                assert(childIter != mapTx.end());
                // Add dependencies that are discovered between transactions in the
                // block and transactions that were in the mempool to txgraph.
                m_txgraph->AddDependency(*it, *childIter);
            }
        }
    }

    auto txs_to_remove = m_txgraph->Trim(); // Enforce cluster size limits.
    for (auto txptr : txs_to_remove) {
        const CTxMemPoolEntry& entry = *(static_cast<const CTxMemPoolEntry*>(txptr));
        removeUnchecked(mapTx.iterator_to(entry), MemPoolRemovalReason::SIZELIMIT);
    }
}

bool CTxMemPool::HasDescendants(const Txid& txid) const
{
    LOCK(cs);
    auto entry = GetEntry(txid);
    if (!entry) return false;
    return m_txgraph->GetDescendants(*entry, TxGraph::Level::MAIN).size() > 1;
}

CTxMemPool::setEntries CTxMemPool::CalculateMemPoolAncestors(const CTxMemPoolEntry &entry) const
{
    auto ancestors = m_txgraph->GetAncestors(entry, TxGraph::Level::MAIN);
    setEntries ret;
    if (ancestors.size() > 0) {
        for (auto ancestor : ancestors) {
            if (ancestor != &entry) {
                ret.insert(mapTx.iterator_to(static_cast<const CTxMemPoolEntry&>(*ancestor)));
            }
        }
        return ret;
    }

    // If we didn't get anything back, the transaction is not in the graph.
    // Find each parent and call GetAncestors on each.
    setEntries staged_parents;
    const CTransaction &tx = entry.GetTx();

    // Get parents of this transaction that are in the mempool
    for (unsigned int i = 0; i < tx.vin.size(); i++) {
        std::optional<txiter> piter = GetIter(tx.vin[i].prevout.hash);
        if (piter) {
            staged_parents.insert(*piter);
        }
    }

    for (const auto& parent : staged_parents) {
        auto parent_ancestors = m_txgraph->GetAncestors(*parent, TxGraph::Level::MAIN);
        for (auto ancestor : parent_ancestors) {
            ret.insert(mapTx.iterator_to(static_cast<const CTxMemPoolEntry&>(*ancestor)));
        }
    }

    return ret;
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
    m_txgraph = MakeTxGraph(m_opts.limits.cluster_count, m_opts.limits.cluster_size_vbytes * WITNESS_SCALE_FACTOR, ACCEPTABLE_ITERS);
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

void CTxMemPool::Apply(ChangeSet* changeset)
{
    AssertLockHeld(cs);
    m_txgraph->CommitStaging();

    RemoveStaged(changeset->m_to_remove, false, MemPoolRemovalReason::REPLACED);

    for (size_t i=0; i<changeset->m_entry_vec.size(); ++i) {
        auto tx_entry = changeset->m_entry_vec[i];
        // First splice this entry into mapTx.
        auto node_handle = changeset->m_to_add.extract(tx_entry);
        auto result = mapTx.insert(std::move(node_handle));

        Assume(result.inserted);
        txiter it = result.position;

        addNewTransaction(it);
    }
    m_txgraph->DoWork(POST_CHANGE_WORK);
}

void CTxMemPool::addNewTransaction(CTxMemPool::txiter newit)
{
    const CTxMemPoolEntry& entry = *newit;

    // Update cachedInnerUsage to include contained transaction's usage.
    // (When we update the entry for in-mempool parents, memory usage will be
    // further updated.)
    cachedInnerUsage += entry.DynamicMemoryUsage();

    const CTransaction& tx = newit->GetTx();
    for (unsigned int i = 0; i < tx.vin.size(); i++) {
        mapNextTx.insert(std::make_pair(&tx.vin[i].prevout, newit));
    }
    // Don't bother worrying about child transactions of this one.
    // Normal case of a new transaction arriving is that there can't be any
    // children, because such children would be orphans.
    // An exception to that is if a transaction enters that used to be in a block.
    // In that case, our disconnect block logic will call UpdateTransactionsFromBlock
    // to clean up the mess we're leaving here.

    nTransactionsUpdated++;
    totalTxSize += entry.GetTxSize();
    m_total_fee += entry.GetFee();

    txns_randomized.emplace_back(tx.GetWitnessHash(), newit);
    newit->idx_randomized = txns_randomized.size() - 1;

    TRACEPOINT(mempool, added,
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
    TRACEPOINT(mempool, removed,
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
        // Remove entry from txns_randomized by replacing it with the back and deleting the back.
        txns_randomized[it->idx_randomized] = std::move(txns_randomized.back());
        txns_randomized[it->idx_randomized].second->idx_randomized = it->idx_randomized;
        txns_randomized.pop_back();
        if (txns_randomized.size() * 2 < txns_randomized.capacity()) {
            txns_randomized.shrink_to_fit();
        }
    } else {
        txns_randomized.clear();
    }

    totalTxSize -= it->GetTxSize();
    m_total_fee -= it->GetFee();
    cachedInnerUsage -= it->DynamicMemoryUsage();
    mapTx.erase(it);
    nTransactionsUpdated++;
}

// Calculates descendants of given entry and adds to setDescendants.
void CTxMemPool::CalculateDescendants(txiter entryit, setEntries& setDescendants) const
{
    (void)CalculateDescendants(*entryit, setDescendants);
    return;
}

CTxMemPool::txiter CTxMemPool::CalculateDescendants(const CTxMemPoolEntry& entry, setEntries& setDescendants) const
{
    for (auto tx : m_txgraph->GetDescendants(entry, TxGraph::Level::MAIN)) {
        setDescendants.insert(mapTx.iterator_to(static_cast<const CTxMemPoolEntry&>(*tx)));
    }
    return mapTx.iterator_to(entry);
}

void CTxMemPool::removeRecursive(const CTransaction &origTx, MemPoolRemovalReason reason)
{
    // Remove transaction from memory pool
    AssertLockHeld(cs);
    Assume(!m_have_changeset);
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
                txiter nextit = it->second;
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
    Assume(!m_have_changeset);

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
    m_txgraph->DoWork(POST_CHANGE_WORK);
}

void CTxMemPool::removeConflicts(const CTransaction &tx)
{
    // Remove transactions which depend on inputs of tx, recursively
    AssertLockHeld(cs);
    for (const CTxIn &txin : tx.vin) {
        auto it = mapNextTx.find(txin.prevout);
        if (it != mapNextTx.end()) {
            const CTransaction &txConflict = it->second->GetTx();
            if (Assume(txConflict.GetHash() != tx.GetHash()))
            {
                ClearPrioritisation(txConflict.GetHash());
                removeRecursive(txConflict, MemPoolRemovalReason::CONFLICT);
            }
        }
    }
}

void CTxMemPool::removeForBlock(const std::vector<CTransactionRef>& vtx, unsigned int nBlockHeight)
{
    // Remove confirmed txs and conflicts when a new block is connected, updating the fee logic
    AssertLockHeld(cs);
    Assume(!m_have_changeset);
    std::vector<RemovedMempoolTransactionInfo> txs_removed_for_block;
    if (mapTx.size() || mapNextTx.size() || mapDeltas.size()) {
        txs_removed_for_block.reserve(vtx.size());
        for (const auto& tx : vtx) {
            txiter it = mapTx.find(tx->GetHash());
            if (it != mapTx.end()) {
                setEntries stage;
                stage.insert(it);
                txs_removed_for_block.emplace_back(*it);
                RemoveStaged(stage, true, MemPoolRemovalReason::BLOCK);
            }
            removeConflicts(*tx);
            ClearPrioritisation(tx->GetHash());
        }
    }
    if (m_opts.signals) {
        m_opts.signals->MempoolTransactionsRemovedForBlock(txs_removed_for_block, nBlockHeight);
    }
    lastRollingFeeUpdate = GetTime();
    blockSinceLastRollingFeeBump = true;
    m_txgraph->DoWork(POST_CHANGE_WORK);
}

void CTxMemPool::check(const CCoinsViewCache& active_coins_tip, int64_t spendheight) const
{
    if (m_opts.check_ratio == 0) return;

    if (FastRandomContext().randrange(m_opts.check_ratio) >= 1) return;

    AssertLockHeld(::cs_main);
    LOCK(cs);
    LogDebug(BCLog::MEMPOOL, "Checking mempool with %u transactions and %u inputs\n", (unsigned int)mapTx.size(), (unsigned int)mapNextTx.size());

    uint64_t checkTotal = 0;
    CAmount check_total_fee{0};
    uint64_t innerUsage = 0;

    assert(!m_txgraph->IsOversized(TxGraph::Level::MAIN));
    m_txgraph->SanityCheck();

    CCoinsViewCache mempoolDuplicate(const_cast<CCoinsViewCache*>(&active_coins_tip));

    std::optional<Wtxid> last_wtxid = std::nullopt;

    for (const auto& it : GetSortedScoreWithTopology()) {
        checkTotal += it->GetTxSize();
        check_total_fee += it->GetFee();
        innerUsage += it->DynamicMemoryUsage();
        const CTransaction& tx = it->GetTx();

        // CompareMiningScoreWithTopology should agree with GetSortedScoreWithTopology()
        if (last_wtxid) {
            assert(CompareMiningScoreWithTopology(*last_wtxid, tx.GetWitnessHash()));
        }
        last_wtxid = tx.GetWitnessHash();

        std::set<CTxMemPoolEntry::CTxMemPoolEntryRef, CompareIteratorByHash> setParentCheck;
        std::set<CTxMemPoolEntry::CTxMemPoolEntryRef, CompareIteratorByHash> setParentsStored;
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
            assert(&it3->second->GetTx() == &tx);
        }
        auto comp = [](const CTxMemPoolEntry& a, const CTxMemPoolEntry& b) -> bool {
            return a.GetTx().GetHash() == b.GetTx().GetHash();
        };
        for (auto &txentry : GetParents(*it)) {
            setParentsStored.insert(dynamic_cast<const CTxMemPoolEntry&>(txentry.get()));
        }
        assert(setParentCheck.size() == setParentsStored.size());
        assert(std::equal(setParentCheck.begin(), setParentCheck.end(), setParentsStored.begin(), comp));

        // Check children against mapNextTx
        std::set<CTxMemPoolEntry::CTxMemPoolEntryRef, CompareIteratorByHash> setChildrenCheck;
        std::set<CTxMemPoolEntry::CTxMemPoolEntryRef, CompareIteratorByHash> setChildrenStored;
        auto iter = mapNextTx.lower_bound(COutPoint(it->GetTx().GetHash(), 0));
        for (; iter != mapNextTx.end() && iter->first->hash == it->GetTx().GetHash(); ++iter) {
            txiter childit = iter->second;
            assert(childit != mapTx.end()); // mapNextTx points to in-mempool transactions
            setChildrenCheck.insert(*childit);
        }
        for (auto &txentry : GetChildren(*it)) {
            setChildrenStored.insert(dynamic_cast<const CTxMemPoolEntry&>(txentry.get()));
        }
        assert(setChildrenCheck.size() == setChildrenStored.size());
        assert(std::equal(setChildrenCheck.begin(), setChildrenCheck.end(), setChildrenStored.begin(), comp));

        TxValidationState dummy_state; // Not used. CheckTxInputs() should always pass
        CAmount txfee = 0;
        assert(!tx.IsCoinBase());
        assert(Consensus::CheckTxInputs(tx, dummy_state, mempoolDuplicate, spendheight, txfee));
        for (const auto& input: tx.vin) mempoolDuplicate.SpendCoin(input.prevout);
        AddCoins(mempoolDuplicate, tx, std::numeric_limits<int>::max());
    }
    for (auto it = mapNextTx.cbegin(); it != mapNextTx.cend(); it++) {
        indexed_transaction_set::const_iterator it2 = it->second;
        assert(it2 != mapTx.end());
    }

    assert(totalTxSize == checkTotal);
    assert(m_total_fee == check_total_fee);
    assert(innerUsage == cachedInnerUsage);
}

bool CTxMemPool::CompareMiningScoreWithTopology(const Wtxid& hasha, const Wtxid& hashb) const
{
    /* Return `true` if hasha should be considered sooner than hashb. Namely when:
     *   a is not in the mempool, but b is
     *   both are in the mempool and a has a higher mining score than b
     *   both are in the mempool and a appears before b in the same cluster
     */
    LOCK(cs);
    auto j{GetIter(hashb)};
    if (!j.has_value()) return false;
    auto i{GetIter(hasha)};
    if (!i.has_value()) return true;

    return m_txgraph->CompareMainOrder(*i.value(), *j.value()) < 0;
}

std::vector<CTxMemPool::indexed_transaction_set::const_iterator> CTxMemPool::GetSortedScoreWithTopology() const
{
    std::vector<indexed_transaction_set::const_iterator> iters;
    AssertLockHeld(cs);

    iters.reserve(mapTx.size());

    for (indexed_transaction_set::iterator mi = mapTx.begin(); mi != mapTx.end(); ++mi) {
        iters.push_back(mi);
    }
    std::sort(iters.begin(), iters.end(), [this](const auto& a, const auto& b) EXCLUSIVE_LOCKS_REQUIRED(cs) noexcept {
        return m_txgraph->CompareMainOrder(*a, *b) < 0;
    });
    return iters;
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

CTransactionRef CTxMemPool::get(const Txid& hash) const
{
    LOCK(cs);
    indexed_transaction_set::const_iterator i = mapTx.find(hash);
    if (i == mapTx.end())
        return nullptr;
    return i->GetSharedTx();
}

void CTxMemPool::PrioritiseTransaction(const Txid& hash, const CAmount& nFeeDelta)
{
    {
        LOCK(cs);
        CAmount &delta = mapDeltas[hash];
        delta = SaturatingAdd(delta, nFeeDelta);
        txiter it = mapTx.find(hash);
        if (it != mapTx.end()) {
            m_txgraph->SetTransactionFee(*it, it->GetModifiedFee() + nFeeDelta);
            mapTx.modify(it, [&nFeeDelta](CTxMemPoolEntry& e) { e.UpdateModifiedFee(nFeeDelta); });

            ++nTransactionsUpdated;
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

void CTxMemPool::ApplyDelta(const Txid& hash, CAmount &nFeeDelta) const
{
    AssertLockHeld(cs);
    std::map<Txid, CAmount>::const_iterator pos = mapDeltas.find(hash);
    if (pos == mapDeltas.end())
        return;
    const CAmount &delta = pos->second;
    nFeeDelta += delta;
}

void CTxMemPool::ClearPrioritisation(const Txid& hash)
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
    return it == mapNextTx.end() ? nullptr : &(it->second->GetTx());
}

std::optional<CTxMemPool::txiter> CTxMemPool::GetIter(const Txid& txid) const
{
    AssertLockHeld(cs);
    auto it = mapTx.find(txid);
    return it != mapTx.end() ? std::make_optional(it) : std::nullopt;
}

std::optional<CTxMemPool::txiter> CTxMemPool::GetIter(const Wtxid& wtxid) const
{
    AssertLockHeld(cs);
    auto it{mapTx.project<0>(mapTx.get<index_by_wtxid>().find(wtxid))};
    return it != mapTx.end() ? std::make_optional(it) : std::nullopt;
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

std::vector<CTxMemPool::txiter> CTxMemPool::GetIterVec(const std::vector<Txid>& txids) const
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
        if (exists(tx.vin[i].prevout.hash))
            return false;
    return true;
}

CCoinsViewMemPool::CCoinsViewMemPool(CCoinsView* baseIn, const CTxMemPool& mempoolIn) : CCoinsViewBacked(baseIn), mempool(mempoolIn) { }

std::optional<Coin> CCoinsViewMemPool::GetCoin(const COutPoint& outpoint) const
{
    // Check to see if the inputs are made available by another tx in the package.
    // These Coins would not be available in the underlying CoinsView.
    if (auto it = m_temp_added.find(outpoint); it != m_temp_added.end()) {
        return it->second;
    }

    // If an entry in the mempool exists, always return that one, as it's guaranteed to never
    // conflict with the underlying cache, and it cannot have pruned entries (as it contains full)
    // transactions. First checking the underlying cache risks returning a pruned entry instead.
    CTransactionRef ptx = mempool.get(outpoint.hash);
    if (ptx) {
        if (outpoint.n < ptx->vout.size()) {
            Coin coin(ptx->vout[outpoint.n], MEMPOOL_HEIGHT, false);
            m_non_base_coins.emplace(outpoint);
            return coin;
        }
        return std::nullopt;
    }
    return base->GetCoin(outpoint);
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
    // Estimate the overhead of mapTx to be 9 pointers (3 pointers per index) + an allocation, as no exact formula for boost::multi_index_contained is implemented.
    return memusage::MallocUsage(sizeof(CTxMemPoolEntry) + 9 * sizeof(void*)) * mapTx.size() + memusage::DynamicUsage(mapNextTx) + memusage::DynamicUsage(mapDeltas) + memusage::DynamicUsage(txns_randomized) + m_txgraph->GetMainMemoryUsage() + cachedInnerUsage;
}

void CTxMemPool::RemoveUnbroadcastTx(const Txid& txid, const bool unchecked) {
    LOCK(cs);

    if (m_unbroadcast_txids.erase(txid))
    {
        LogDebug(BCLog::MEMPOOL, "Removed %i from set of unbroadcast txns%s\n", txid.GetHex(), (unchecked ? " before confirmation that txn was sent out" : ""));
    }
}

void CTxMemPool::RemoveStaged(setEntries &stage, bool updateDescendants, MemPoolRemovalReason reason) {
    AssertLockHeld(cs);
    for (txiter it : stage) {
        removeUnchecked(it, reason);
    }
}

int CTxMemPool::Expire(std::chrono::seconds time)
{
    AssertLockHeld(cs);
    Assume(!m_have_changeset);
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
    Assume(!m_have_changeset);

    unsigned nTxnRemoved = 0;
    CFeeRate maxFeeRateRemoved(0);

    while (!mapTx.empty() && DynamicMemoryUsage() > sizelimit) {
        const auto &[worst_chunk, feeperweight] = m_txgraph->GetWorstMainChunk();
        FeePerVSize feerate = ToFeePerVSize(feeperweight);
        CFeeRate removed{feerate.fee, feerate.size};

        // We set the new mempool min fee to the feerate of the removed set, plus the
        // "minimum reasonable fee rate" (ie some value under which we consider txn
        // to have 0 fee). This way, we don't allow txn to enter mempool with feerate
        // equal to txn which were removed with no block in between.
        removed += m_opts.incremental_relay_feerate;
        trackPackageRemoved(removed);
        maxFeeRateRemoved = std::max(maxFeeRateRemoved, removed);

        nTxnRemoved += worst_chunk.size();

        std::vector<CTransaction> txn;
        if (pvNoSpendsRemaining) {
            txn.reserve(worst_chunk.size());
            for (auto ref : worst_chunk) {
                txn.emplace_back(static_cast<const CTxMemPoolEntry&>(*ref).GetTx());
            }
        }

        setEntries stage;
        for (auto ref : worst_chunk) {
            stage.insert(mapTx.iterator_to(static_cast<const CTxMemPoolEntry&>(*ref)));
        }
        for (auto e : stage) {
            removeUnchecked(e, MemPoolRemovalReason::SIZELIMIT);
        }
        if (pvNoSpendsRemaining) {
            for (const CTransaction& tx : txn) {
                for (const CTxIn& txin : tx.vin) {
                    if (exists(txin.prevout.hash)) continue;
                    pvNoSpendsRemaining->push_back(txin.prevout);
                }
            }
        }
    }

    if (maxFeeRateRemoved > CFeeRate(0)) {
        LogDebug(BCLog::MEMPOOL, "Removed %u txn, rolling minimum fee bumped to %s\n", nTxnRemoved, maxFeeRateRemoved.ToString());
    }
}

void CTxMemPool::CalculateAncestorData(const CTxMemPoolEntry& entry, size_t& ancestor_count, size_t& ancestor_size, CAmount& ancestor_fees) const
{
    auto ancestors = m_txgraph->GetAncestors(entry, TxGraph::Level::MAIN);

    ancestor_count = ancestors.size();
    ancestor_size = 0;
    ancestor_fees = 0;
    for (auto tx: ancestors) {
        const CTxMemPoolEntry& anc = static_cast<const CTxMemPoolEntry&>(*tx);
        ancestor_size += anc.GetTxSize();
        ancestor_fees += anc.GetModifiedFee();
    }
}

void CTxMemPool::CalculateDescendantData(const CTxMemPoolEntry& entry, size_t& descendant_count, size_t& descendant_size, CAmount& descendant_fees) const
{
    auto descendants = m_txgraph->GetDescendants(entry, TxGraph::Level::MAIN);
    descendant_count = descendants.size();
    descendant_size = 0;
    descendant_fees = 0;

    for (auto tx: descendants) {
        const CTxMemPoolEntry &desc = static_cast<const CTxMemPoolEntry&>(*tx);
        descendant_size += desc.GetTxSize();
        descendant_fees += desc.GetModifiedFee();
    }
}

void CTxMemPool::GetTransactionAncestry(const Txid& txid, size_t& ancestors, size_t& clustersize, size_t* const ancestorsize, CAmount* const ancestorfees) const {
    LOCK(cs);
    auto it = mapTx.find(txid);
    ancestors = clustersize = 0;
    if (it != mapTx.end()) {
        size_t dummysize{0};
        CAmount dummyfees{0};
        CalculateAncestorData(*it, ancestors, ancestorsize ? *ancestorsize :
                dummysize, ancestorfees ? *ancestorfees : dummyfees);
        clustersize = m_txgraph->GetCluster(*it, TxGraph::Level::MAIN).size();
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

std::vector<CTxMemPool::txiter> CTxMemPool::GatherClusters(const std::vector<Txid>& txids) const
{
    AssertLockHeld(cs);

    std::vector<CTxMemPool::txiter> ret;
    std::set<const CTxMemPoolEntry*> unique_cluster_representatives;
    for (auto txid : txids) {
        auto it = mapTx.find(txid);
        if (it != mapTx.end()) {
            auto cluster = m_txgraph->GetCluster(*it, TxGraph::Level::MAIN);
            if (unique_cluster_representatives.insert(static_cast<const CTxMemPoolEntry*>(&(**cluster.begin()))).second) {
                for (auto tx : cluster) {
                    ret.emplace_back(mapTx.iterator_to(static_cast<const CTxMemPoolEntry&>(*tx)));
                }
            }
        }
    }
    if (ret.size() > 500) {
        return {};
    }
    return ret;
}

util::Result<std::pair<std::vector<FeeFrac>, std::vector<FeeFrac>>> CTxMemPool::ChangeSet::CalculateChunksForRBF()
{
    LOCK(m_pool->cs);

    if (!CheckMemPoolPolicyLimits()) {
        return util::Error{Untranslated("cluster size limit exceeded")};
    }

    return m_pool->m_txgraph->GetMainStagingDiagrams();
}

CTxMemPool::ChangeSet::TxHandle CTxMemPool::ChangeSet::StageAddition(const CTransactionRef& tx, const CAmount fee, int64_t time, unsigned int entry_height, uint64_t entry_sequence, bool spends_coinbase, int64_t sigops_cost, LockPoints lp)
{
    LOCK(m_pool->cs);
    Assume(m_to_add.find(tx->GetHash()) == m_to_add.end());
    Assume(!m_dependencies_processed);

    // We need to reprocess dependencies after adding a new transaction.
    m_dependencies_processed = false;

    CAmount delta{0};
    m_pool->ApplyDelta(tx->GetHash(), delta);

    TxGraph::Ref ref(m_pool->m_txgraph->AddTransaction(FeePerWeight(fee+delta, GetSigOpsAdjustedWeight(GetTransactionWeight(*tx), sigops_cost, ::nBytesPerSigOp))));
    auto newit = m_to_add.emplace(std::move(ref), tx, fee, time, entry_height, entry_sequence, spends_coinbase, sigops_cost, lp).first;
    if (delta) {
        m_to_add.modify(newit, [&delta](CTxMemPoolEntry& e) { e.UpdateModifiedFee(delta); });
    }

    m_entry_vec.push_back(newit);

    return newit;
}

void CTxMemPool::ChangeSet::StageRemoval(CTxMemPool::txiter it)
{
    LOCK(m_pool->cs);
    m_pool->m_txgraph->RemoveTransaction(*it);
    m_to_remove.insert(it);
}

void CTxMemPool::ChangeSet::Apply()
{
    LOCK(m_pool->cs);
    if (!m_dependencies_processed) {
        ProcessDependencies();
    }
    m_pool->Apply(this);
    m_to_add.clear();
    m_to_remove.clear();
    m_entry_vec.clear();
    m_ancestors.clear();
}

void CTxMemPool::ChangeSet::ProcessDependencies()
{
    LOCK(m_pool->cs);
    Assume(!m_dependencies_processed); // should only call this once.
    for (const auto& entryptr : m_entry_vec) {
        for (const auto &txin : entryptr->GetSharedTx()->vin) {
            std::optional<txiter> piter = m_pool->GetIter(txin.prevout.hash);
            if (!piter) {
                auto it = m_to_add.find(txin.prevout.hash);
                if (it != m_to_add.end()) {
                    piter = std::make_optional(it);
                }
            }
            if (piter) {
                m_pool->m_txgraph->AddDependency(**piter, *entryptr);
            }
        }
    }
    m_dependencies_processed = true;
    return;
 }

bool CTxMemPool::ChangeSet::CheckMemPoolPolicyLimits()
{
    LOCK(m_pool->cs);
    if (!m_dependencies_processed) {
        ProcessDependencies();
    }

    return !m_pool->m_txgraph->IsOversized(TxGraph::Level::TOP);
}

std::vector<FeePerVSize> CTxMemPool::GetFeerateDiagram() const
{
    FeePerVSize zero{};
    std::vector<FeePerVSize> ret;

    ret.emplace_back(zero);

    StartBlockBuilding();

    std::vector<CTxMemPoolEntry::CTxMemPoolEntryRef> dummy;

    FeePerWeight last_selection = GetBlockBuilderChunk(dummy);
    while (last_selection != FeePerWeight{}) {
        FeePerVSize accumulated_fee_per_vsize = ToFeePerVSize(last_selection);
        accumulated_fee_per_vsize += ret.back();
        ret.emplace_back(accumulated_fee_per_vsize);
        IncludeBuilderChunk();
        last_selection = GetBlockBuilderChunk(dummy);
    }
    StopBlockBuilding();
    return ret;
}
