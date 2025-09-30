// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <txmempool.h>

#include <chain.h>
#include <coins.h>
#include <consensus/consensus.h>
#include <consensus/tx_verify.h>
#include <consensus/validation.h>
#include <hash.h>
#include <policy/fees.h>
#include <policy/policy.h>
#include <policy/settings.h>
#include <util/check.h>
#include <util/moneystr.h>
#include <util/overflow.h>
#include <util/system.h>
#include <util/time.h>
#include <validationinterface.h>

#include <evo/specialtx.h>
#include <evo/assetlocktx.h>
#include <evo/providertx.h>
#include <evo/deterministicmns.h>
#include <instantsend/instantsend.h>

#include <cmath>
#include <optional>
#include <ranges>

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

CTxMemPoolEntry::CTxMemPoolEntry(const CTransactionRef& tx, CAmount fee,
                                 int64_t time, unsigned int entry_height,
                                 bool spends_coinbase, int64_t sigops_count, LockPoints lp)
    : tx{tx},
      nFee{fee},
      nTxSize(tx->GetTotalSize()),
      nUsageSize{RecursiveDynamicUsage(tx)},
      nTime{time},
      entryHeight{entry_height},
      spendsCoinbase{spends_coinbase},
      sigOpCount{sigops_count},
      m_modified_fee{nFee},
      lockPoints{lp},
      nSizeWithDescendants{GetTxSize()},
      nModFeesWithDescendants{nFee},
      nSizeWithAncestors{GetTxSize()},
      nModFeesWithAncestors{nFee},
      nSigOpCountWithAncestors{sigOpCount} {}

void CTxMemPoolEntry::UpdateModifiedFee(CAmount newFeeDelta)
{
    nModFeesWithDescendants = SaturatingAdd(nModFeesWithDescendants, newFeeDelta);
    nModFeesWithAncestors = SaturatingAdd(nModFeesWithAncestors, newFeeDelta);
    m_modified_fee = SaturatingAdd(m_modified_fee, newFeeDelta);
}

void CTxMemPoolEntry::UpdateLockPoints(const LockPoints& lp)
{
    lockPoints = lp;
}

size_t CTxMemPoolEntry::GetTxSize() const
{
    return GetVirtualTransactionSize(nTxSize, sigOpCount);
}

void CTxMemPool::UpdateForDescendants(txiter updateIt, cacheMap& cachedDescendants,
                                      const std::set<uint256>& setExclude, std::set<uint256>& descendants_to_remove,
                                      uint64_t ancestor_size_limit, uint64_t ancestor_count_limit)
{
    CTxMemPoolEntry::Children stageEntries, descendants;
    stageEntries = updateIt->GetMemPoolChildrenConst();

    while (!stageEntries.empty()) {
        const CTxMemPoolEntry& descendant = *stageEntries.begin();
        descendants.insert(descendant);
        stageEntries.erase(descendant);
        const CTxMemPoolEntry::Children& children = descendant.GetMemPoolChildrenConst();
        for (const CTxMemPoolEntry& childEntry : children) {
            cacheMap::iterator cacheIt = cachedDescendants.find(mapTx.iterator_to(childEntry));
            if (cacheIt != cachedDescendants.end()) {
                // We've already calculated this one, just add the entries for this set
                // but don't traverse again.
                for (txiter cacheEntry : cacheIt->second) {
                    descendants.insert(*cacheEntry);
                }
            } else if (!descendants.count(childEntry)) {
                // Schedule for later processing
                stageEntries.insert(childEntry);
            }
        }
    }
    // descendants now contains all in-mempool descendants of updateIt.
    // Update and add to cached descendant map
    int64_t modifySize = 0;
    CAmount modifyFee = 0;
    int64_t modifyCount = 0;
    for (const CTxMemPoolEntry& descendant : descendants) {
        if (!setExclude.count(descendant.GetTx().GetHash())) {
            modifySize += descendant.GetTxSize();
            modifyFee += descendant.GetModifiedFee();
            modifyCount++;
            cachedDescendants[updateIt].insert(mapTx.iterator_to(descendant));
            // Update ancestor state for each descendant
            mapTx.modify(mapTx.iterator_to(descendant), [=](CTxMemPoolEntry& e) {
                e.UpdateAncestorState(updateIt->GetTxSize(), updateIt->GetModifiedFee(), 1, updateIt->GetSigOpCount());
            });
            // Don't directly remove the transaction here -- doing so would
            // invalidate iterators in cachedDescendants. Mark it for removal
            // by inserting into descendants_to_remove.
            if (descendant.GetCountWithAncestors() > ancestor_count_limit || descendant.GetSizeWithAncestors() > ancestor_size_limit) {
                descendants_to_remove.insert(descendant.GetTx().GetHash());
            }
        }
    }
    mapTx.modify(updateIt, [=](CTxMemPoolEntry& e) { e.UpdateDescendantState(modifySize, modifyFee, modifyCount); });
}

void CTxMemPool::UpdateTransactionsFromBlock(const std::vector<uint256> &vHashesToUpdate, uint64_t ancestor_size_limit, uint64_t ancestor_count_limit)
{
    AssertLockHeld(cs);
    // For each entry in vHashesToUpdate, store the set of in-mempool, but not
    // in-vHashesToUpdate transactions, so that we don't have to recalculate
    // descendants when we come across a previously seen entry.
    cacheMap mapMemPoolDescendantsToUpdate;

    // Use a set for lookups into vHashesToUpdate (these entries are already
    // accounted for in the state of their ancestors)
    std::set<uint256> setAlreadyIncluded(vHashesToUpdate.begin(), vHashesToUpdate.end());

    std::set<uint256> descendants_to_remove;

    // Iterate in reverse, so that whenever we are looking at a transaction
    // we are sure that all in-mempool descendants have already been processed.
    // This maximizes the benefit of the descendant cache and guarantees that
    // CTxMemPoolEntry::m_children will be updated, an assumption made in
    // UpdateForDescendants.
    for (const uint256& hash : vHashesToUpdate | std::views::reverse) {
        // calculate children from mapNextTx
        txiter it = mapTx.find(hash);
        if (it == mapTx.end()) {
            continue;
        }
        auto iter = mapNextTx.lower_bound(COutPoint(hash, 0));
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
        } // release epoch guard for UpdateForDescendants
        UpdateForDescendants(it, mapMemPoolDescendantsToUpdate, setAlreadyIncluded, descendants_to_remove, ancestor_size_limit, ancestor_count_limit);
    }

    for (const auto& txid : descendants_to_remove) {
        // This txid may have been removed already in a prior call to removeRecursive.
        // Therefore we ensure it is not yet removed already.
        if (const std::optional<txiter> txiter = GetIter(txid)) {
            removeRecursive((*txiter)->GetTx(), MemPoolRemovalReason::SIZELIMIT);
        }
    }
}

bool CTxMemPool::CalculateAncestorsAndCheckLimits(size_t entry_size,
                                                  size_t entry_count,
                                                  setEntries& setAncestors,
                                                  CTxMemPoolEntry::Parents& staged_ancestors,
                                                  uint64_t limitAncestorCount,
                                                  uint64_t limitAncestorSize,
                                                  uint64_t limitDescendantCount,
                                                  uint64_t limitDescendantSize,
                                                  std::string &errString) const
{
    size_t totalSizeWithAncestors = entry_size;

    while (!staged_ancestors.empty()) {
        const CTxMemPoolEntry& stage = staged_ancestors.begin()->get();
        txiter stageit = mapTx.iterator_to(stage);

        setAncestors.insert(stageit);
        staged_ancestors.erase(stage);
        totalSizeWithAncestors += stageit->GetTxSize();

        if (stageit->GetSizeWithDescendants() + entry_size > limitDescendantSize) {
            errString = strprintf("exceeds descendant size limit for tx %s [limit: %u]", stageit->GetTx().GetHash().ToString(), limitDescendantSize);
            return false;
        } else if (stageit->GetCountWithDescendants() + entry_count > limitDescendantCount) {
            errString = strprintf("too many descendants for tx %s [limit: %u]", stageit->GetTx().GetHash().ToString(), limitDescendantCount);
            return false;
        } else if (totalSizeWithAncestors > limitAncestorSize) {
            errString = strprintf("exceeds ancestor size limit [limit: %u]", limitAncestorSize);
            return false;
        }

        const CTxMemPoolEntry::Parents& parents = stageit->GetMemPoolParentsConst();
        for (const CTxMemPoolEntry& parent : parents) {
            txiter parent_it = mapTx.iterator_to(parent);

            // If this is a new ancestor, add it.
            if (setAncestors.count(parent_it) == 0) {
                staged_ancestors.insert(parent);
            }
            if (staged_ancestors.size() + setAncestors.size() + entry_count > limitAncestorCount) {
                errString = strprintf("too many unconfirmed ancestors [limit: %u]", limitAncestorCount);
                return false;
            }
        }
    }

    return true;
}

bool CTxMemPool::CheckPackageLimits(const Package& package,
                                    uint64_t limitAncestorCount,
                                    uint64_t limitAncestorSize,
                                    uint64_t limitDescendantCount,
                                    uint64_t limitDescendantSize,
                                    std::string &errString) const
{
    CTxMemPoolEntry::Parents staged_ancestors;
    size_t total_size = 0;
    for (const auto& tx : package) {
        total_size += GetVirtualTransactionSize(*tx);
        for (const auto& input : tx->vin) {
            std::optional<txiter> piter = GetIter(input.prevout.hash);
            if (piter) {
                staged_ancestors.insert(**piter);
                if (staged_ancestors.size() + package.size() > limitAncestorCount) {
                    errString = strprintf("too many unconfirmed parents [limit: %u]", limitAncestorCount);
                    return false;
                }
            }
        }
    }
    // When multiple transactions are passed in, the ancestors and descendants of all transactions
    // considered together must be within limits even if they are not interdependent. This may be
    // stricter than the limits for each individual transaction.
    setEntries setAncestors;
    const auto ret = CalculateAncestorsAndCheckLimits(total_size, package.size(),
                                                      setAncestors, staged_ancestors,
                                                      limitAncestorCount, limitAncestorSize,
                                                      limitDescendantCount, limitDescendantSize, errString);
    // It's possible to overestimate the ancestor/descendant totals.
    if (!ret) errString.insert(0, "possibly ");
    return ret;
}

bool CTxMemPool::CalculateMemPoolAncestors(const CTxMemPoolEntry &entry,
                                           setEntries &setAncestors,
                                           uint64_t limitAncestorCount,
                                           uint64_t limitAncestorSize,
                                           uint64_t limitDescendantCount,
                                           uint64_t limitDescendantSize,
                                           std::string &errString,
                                           bool fSearchForParents /* = true */) const
{
    CTxMemPoolEntry::Parents staged_ancestors;
    const CTransaction &tx = entry.GetTx();

    if (fSearchForParents) {
        // Get parents of this transaction that are in the mempool
        // GetMemPoolParents() is only valid for entries in the mempool, so we
        // iterate mapTx to find parents.
        for (unsigned int i = 0; i < tx.vin.size(); i++) {
            std::optional<txiter> piter = GetIter(tx.vin[i].prevout.hash);
            if (piter) {
                staged_ancestors.insert(**piter);
                if (staged_ancestors.size() + 1 > limitAncestorCount) {
                    errString = strprintf("too many unconfirmed parents [limit: %u]", limitAncestorCount);
                    return false;
                }
            }
        }
    } else {
        // If we're not searching for parents, we require this to already be an
        // entry in the mempool and use the entry's cached parents.
        txiter it = mapTx.iterator_to(entry);
        staged_ancestors = it->GetMemPoolParentsConst();
    }

    return CalculateAncestorsAndCheckLimits(entry.GetTxSize(), /*entry_count=*/1,
                                            setAncestors, staged_ancestors,
                                            limitAncestorCount, limitAncestorSize,
                                            limitDescendantCount, limitDescendantSize, errString);
}

void CTxMemPool::UpdateAncestorsOf(bool add, txiter it, setEntries &setAncestors)
{
    const CTxMemPoolEntry::Parents& parents = it->GetMemPoolParentsConst();
    // add or remove this tx as a child of each parent
    for (const CTxMemPoolEntry& parent : parents) {
        UpdateChild(mapTx.iterator_to(parent), it, add);
    }
    const int64_t updateCount = (add ? 1 : -1);
    const int64_t updateSize = updateCount * it->GetTxSize();
    const CAmount updateFee = updateCount * it->GetModifiedFee();
    for (txiter ancestorIt : setAncestors) {
        mapTx.modify(ancestorIt, [=](CTxMemPoolEntry& e) { e.UpdateDescendantState(updateSize, updateFee, updateCount); });
    }
}

void CTxMemPool::UpdateEntryForAncestors(txiter it, const setEntries &setAncestors)
{
    int64_t updateCount = setAncestors.size();
    int64_t updateSize = 0;
    CAmount updateFee = 0;
    int updateSigOps = 0;
    for (txiter ancestorIt : setAncestors) {
        updateSize += ancestorIt->GetTxSize();
        updateFee += ancestorIt->GetModifiedFee();
        updateSigOps += ancestorIt->GetSigOpCount();
    }
    mapTx.modify(it, [=](CTxMemPoolEntry& e){ e.UpdateAncestorState(updateSize, updateFee, updateCount, updateSigOps); });
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
    // For each entry, walk back all ancestors and decrement size associated with this
    // transaction
    const uint64_t nNoLimit = std::numeric_limits<uint64_t>::max();
    if (updateDescendants) {
        // updateDescendants should be true whenever we're not recursively
        // removing a tx and all its descendants, eg when a transaction is
        // confirmed in a block.
        // Here we only update statistics and not data in CTxMemPool::Parents
        // and CTxMemPoolEntry::Children (which we need to preserve until we're
        // finished with all operations that need to traverse the mempool).
        for (txiter removeIt : entriesToRemove) {
            setEntries setDescendants;
            CalculateDescendants(removeIt, setDescendants);
            setDescendants.erase(removeIt); // don't update state for self
            int64_t modifySize = -((int64_t)removeIt->GetTxSize());
            CAmount modifyFee = -removeIt->GetModifiedFee();
            int modifySigOps = -removeIt->GetSigOpCount();
            for (txiter dit : setDescendants) {
                mapTx.modify(dit, [=](CTxMemPoolEntry& e){ e.UpdateAncestorState(modifySize, modifyFee, -1, modifySigOps); });
            }
        }
    }
    for (txiter removeIt : entriesToRemove) {
        setEntries setAncestors;
        const CTxMemPoolEntry &entry = *removeIt;
        std::string dummy;
        // Since this is a tx that is already in the mempool, we can call CMPA
        // with fSearchForParents = false.  If the mempool is in a consistent
        // state, then using true or false should both be correct, though false
        // should be a bit faster.
        // However, if we happen to be in the middle of processing a reorg, then
        // the mempool can be in an inconsistent state.  In this case, the set
        // of ancestors reachable via GetMemPoolParents()/GetMemPoolChildren()
        // will be the same as the set of ancestors whose packages include this
        // transaction, because when we add a new transaction to the mempool in
        // addUnchecked(), we assume it has no children, and in the case of a
        // reorg where that assumption is false, the in-mempool children aren't
        // linked to the in-block tx's until UpdateTransactionsFromBlock() is
        // called.
        // So if we're being called during a reorg, ie before
        // UpdateTransactionsFromBlock() has been called, then
        // GetMemPoolParents()/GetMemPoolChildren() will differ from the set of
        // mempool parents we'd calculate by searching, and it's important that
        // we use the cached notion of ancestor transactions as the set of
        // things to update for removal.
        CalculateMemPoolAncestors(entry, setAncestors, nNoLimit, nNoLimit, nNoLimit, nNoLimit, dummy, false);
        // Note that UpdateAncestorsOf severs the child links that point to
        // removeIt in the entries for the parents of removeIt.
        UpdateAncestorsOf(false, removeIt, setAncestors);
    }
    // After updating all the ancestor sizes, we can now sever the link between each
    // transaction being removed and any mempool children (ie, update CTxMemPoolEntry::m_parents
    // for each direct child of a transaction being removed).
    for (txiter removeIt : entriesToRemove) {
        UpdateChildrenForRemoval(removeIt);
    }
}

void CTxMemPoolEntry::UpdateDescendantState(int64_t modifySize, CAmount modifyFee, int64_t modifyCount)
{
    nSizeWithDescendants += modifySize;
    assert(int64_t(nSizeWithDescendants) > 0);
    nModFeesWithDescendants = SaturatingAdd(nModFeesWithDescendants, modifyFee);
    nCountWithDescendants += modifyCount;
    assert(int64_t(nCountWithDescendants) > 0);
}

void CTxMemPoolEntry::UpdateAncestorState(int64_t modifySize, CAmount modifyFee, int64_t modifyCount, int64_t modifySigOps)
{
    nSizeWithAncestors += modifySize;
    assert(int64_t(nSizeWithAncestors) > 0);
    nModFeesWithAncestors = SaturatingAdd(nModFeesWithAncestors, modifyFee);
    nCountWithAncestors += modifyCount;
    assert(int64_t(nCountWithAncestors) > 0);
    nSigOpCountWithAncestors += modifySigOps;
    assert(int(nSigOpCountWithAncestors) >= 0);
}

CTxMemPool::CTxMemPool(CBlockPolicyEstimator* estimator, int check_ratio)
    : m_check_ratio(check_ratio), minerPolicyEstimator(estimator)
{
    _clear(); //lock free clear
}

void CTxMemPool::ConnectManagers(gsl::not_null<CDeterministicMNManager*> dmnman, gsl::not_null<llmq::CInstantSendManager*> isman)
{
    // Do not allow double-initialization
    assert(m_dmnman.load(std::memory_order_acquire) == nullptr);
    m_dmnman.store(dmnman, std::memory_order_release);
    assert(m_isman.load(std::memory_order_acquire) == nullptr);
    m_isman.store(isman, std::memory_order_release);
}

void CTxMemPool::DisconnectManagers()
{
    m_dmnman.store(nullptr, std::memory_order_release);
    m_isman.store(nullptr, std::memory_order_release);
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

void CTxMemPool::addUnchecked(const CTxMemPoolEntry &entry, setEntries &setAncestors, bool validFeeEstimate)
{
    // Add to memory pool without checking anything.
    // Used by AcceptToMemoryPool(), which DOES do
    // all the appropriate checks.
    indexed_transaction_set::iterator newit = mapTx.insert(entry).first;

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
    std::set<uint256> setParentTransactions;
    for (unsigned int i = 0; i < tx.vin.size(); i++) {
        mapNextTx.insert(std::make_pair(&tx.vin[i].prevout, &tx));
        setParentTransactions.insert(tx.vin[i].prevout.hash);
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
    UpdateAncestorsOf(true, newit, setAncestors);
    UpdateEntryForAncestors(newit, setAncestors);

    nTransactionsUpdated++;
    totalTxSize += entry.GetTxSize();
    m_total_fee += entry.GetFee();
    if (minerPolicyEstimator) {
        minerPolicyEstimator->processTransaction(entry, validFeeEstimate);
    }

    vTxHashes.emplace_back(entry.GetTx().GetHash(), newit);
    newit->vTxHashesIdx = vTxHashes.size() - 1;

    // Invalid ProTxes should never get this far because transactions should be
    // fully checked by AcceptToMemoryPool() at this point, so we just assume that
    // everything is fine here.
    if (auto dmnman = m_dmnman.load(std::memory_order_acquire); dmnman) {
        addUncheckedProTx(*dmnman, newit, tx);
    }
}

void CTxMemPool::addAddressIndex(const CTxMemPoolEntry& entry, const CCoinsViewCache& view)
{
    LOCK(cs);
    const CTransaction& tx = entry.GetTx();
    std::vector<CMempoolAddressDeltaKey> inserted;

    uint256 txhash = tx.GetHash();
    for (unsigned int j = 0; j < tx.vin.size(); j++) {
        const CTxIn input = tx.vin[j];
        const Coin& coin = view.AccessCoin(input.prevout);
        const CTxOut &prevout = coin.out;

        AddressType address_type{AddressType::UNKNOWN};
        uint160 address_bytes;

        if (!AddressBytesFromScript(prevout.scriptPubKey, address_type, address_bytes)) {
            continue;
        }

        CMempoolAddressDeltaKey key(address_type, address_bytes, txhash, j, /* tx_spent */ true);
        CMempoolAddressDelta delta(entry.GetTime(), prevout.nValue * -1, input.prevout.hash, input.prevout.n);
        mapAddress.insert(std::make_pair(key, delta));
        inserted.push_back(key);
    }

    for (unsigned int k = 0; k < tx.vout.size(); k++) {
        const CTxOut &out = tx.vout[k];

        AddressType address_type{AddressType::UNKNOWN};
        uint160 address_bytes;

        if (!AddressBytesFromScript(out.scriptPubKey, address_type, address_bytes)) {
            continue;
        }

        CMempoolAddressDeltaKey key(address_type, address_bytes, txhash, k, /* tx_spent */ false);
        mapAddress.insert(std::make_pair(key, CMempoolAddressDelta(entry.GetTime(), out.nValue)));
        inserted.push_back(key);
    }

    mapAddressInserted.insert(std::make_pair(txhash, inserted));
}

bool CTxMemPool::getAddressIndex(const std::vector<CMempoolAddressDeltaKey>& addresses,
                                 std::vector<CMempoolAddressDeltaEntry>& results) const
{
    LOCK(cs);
    for (const auto& address : addresses) {
        auto ait = mapAddress.lower_bound(address);
        while (ait != mapAddress.end() && (*ait).first.m_address_bytes == address.m_address_bytes
               && (*ait).first.m_address_type == address.m_address_type) {
            results.push_back(*ait);
            ait++;
        }
    }
    return true;
}

void CTxMemPool::removeAddressIndex(const uint256 txhash)
{
    LOCK(cs);
    if (auto it = mapAddressInserted.find(txhash); it != mapAddressInserted.end()) {
        for (const auto& key : (*it).second) {
            mapAddress.erase(key);
        }
        mapAddressInserted.erase(it);
    }
}

void CTxMemPool::addSpentIndex(const CTxMemPoolEntry& entry, const CCoinsViewCache& view)
{
    LOCK(cs);

    const CTransaction& tx = entry.GetTx();
    std::vector<CSpentIndexKey> inserted;

    uint256 txhash = tx.GetHash();
    for (unsigned int j = 0; j < tx.vin.size(); j++) {
        const CTxIn input = tx.vin[j];
        const Coin& coin = view.AccessCoin(input.prevout);
        const CTxOut &prevout = coin.out;

        AddressType address_type{AddressType::UNKNOWN};
        uint160 address_bytes;

        if (!AddressBytesFromScript(prevout.scriptPubKey, address_type, address_bytes)) {
            continue;
        }

        CSpentIndexKey key = CSpentIndexKey(input.prevout.hash, input.prevout.n);
        CSpentIndexValue value = CSpentIndexValue(txhash, j, -1, prevout.nValue, address_type, address_bytes);

        mapSpent.insert(std::make_pair(key, value));
        inserted.push_back(key);
    }

    mapSpentInserted.insert(make_pair(txhash, inserted));
}

bool CTxMemPool::getSpentIndex(const CSpentIndexKey& key, CSpentIndexValue& value) const
{
    LOCK(cs);
    if (auto it = mapSpent.find(key); it != mapSpent.end()) {
        value = it->second;
        return true;
    }
    return false;
}

void CTxMemPool::removeSpentIndex(const uint256 txhash)
{
    LOCK(cs);
    if (auto it = mapSpentInserted.find(txhash); it != mapSpentInserted.end()) {
        for (const auto& key : (*it).second) {
            mapSpent.erase(key);
        }
        mapSpentInserted.erase(it);
    }
}

void CTxMemPool::addUncheckedProTx(CDeterministicMNManager& dmnman, indexed_transaction_set::iterator& newit,
                                   const CTransaction& tx)
{
    const uint256 tx_hash{tx.GetHash()};
    if (tx.nType == TRANSACTION_PROVIDER_REGISTER) {
        auto proTx = *Assert(GetTxPayload<CProRegTx>(tx));
        if (!proTx.collateralOutpoint.hash.IsNull()) {
            mapProTxRefs.emplace(tx_hash, proTx.collateralOutpoint.hash);
        }
        for (const auto& entry : proTx.netInfo->GetEntries()) {
            mapProTxAddresses.emplace(entry, tx_hash);
        }
        mapProTxPubKeyIDs.emplace(proTx.keyIDOwner, tx_hash);
        mapProTxBlsPubKeyHashes.emplace(proTx.pubKeyOperator.GetHash(), tx_hash);
        if (!proTx.collateralOutpoint.hash.IsNull()) {
            mapProTxCollaterals.emplace(proTx.collateralOutpoint, tx_hash);
        } else {
            mapProTxCollaterals.emplace(COutPoint(tx_hash, proTx.collateralOutpoint.n), tx_hash);
        }
    } else if (tx.nType == TRANSACTION_PROVIDER_UPDATE_SERVICE) {
        auto proTx = *Assert(GetTxPayload<CProUpServTx>(tx));
        mapProTxRefs.emplace(proTx.proTxHash, tx_hash);
        for (const auto& entry : proTx.netInfo->GetEntries()) {
            mapProTxAddresses.emplace(entry, tx_hash);
        }
    } else if (tx.nType == TRANSACTION_PROVIDER_UPDATE_REGISTRAR) {
        auto proTx = *Assert(GetTxPayload<CProUpRegTx>(tx));
        mapProTxRefs.emplace(proTx.proTxHash, tx_hash);
        mapProTxBlsPubKeyHashes.emplace(proTx.pubKeyOperator.GetHash(), tx_hash);
        auto dmn = Assert(dmnman.GetListAtChainTip().GetMN(proTx.proTxHash));
        newit->validForProTxKey = ::SerializeHash(dmn->pdmnState->pubKeyOperator);
        if (dmn->pdmnState->pubKeyOperator != proTx.pubKeyOperator) {
            newit->isKeyChangeProTx = true;
        }
    } else if (tx.nType == TRANSACTION_PROVIDER_UPDATE_REVOKE) {
        auto proTx = *Assert(GetTxPayload<CProUpRevTx>(tx));
        mapProTxRefs.emplace(proTx.proTxHash, tx_hash);
        auto dmn = Assert(dmnman.GetListAtChainTip().GetMN(proTx.proTxHash));
        newit->validForProTxKey = ::SerializeHash(dmn->pdmnState->pubKeyOperator);
        if (dmn->pdmnState->pubKeyOperator.Get() != CBLSPublicKey()) {
            newit->isKeyChangeProTx = true;
        }
    } else if (tx.nType == TRANSACTION_ASSET_UNLOCK) {
        auto assetUnlockTx = *Assert(GetTxPayload<CAssetUnlockPayload>(tx));
        mapAssetUnlockExpiry.insert({tx_hash, assetUnlockTx.getHeightToExpiry()});
    } else if (tx.nType == TRANSACTION_MNHF_SIGNAL) {
        PrioritiseTransaction(tx_hash, 0.1 * COIN);
    }
}

void CTxMemPool::removeUnchecked(txiter it, MemPoolRemovalReason reason)
{
    // We increment mempool sequence value no matter removal reason
    // even if not directly reported below.
    uint64_t mempool_sequence = GetAndIncrementSequence();

    if (reason != MemPoolRemovalReason::BLOCK) {
        // Notify clients that a transaction has been removed from the mempool
        // for any reason except being included in a block. Clients interested
        // in transactions included in blocks can subscribe to the BlockConnected
        // notification.
        GetMainSignals().TransactionRemovedFromMempool(it->GetSharedTx(), reason, mempool_sequence);
    }

    const uint256 hash = it->GetTx().GetHash();
    for (const CTxIn& txin : it->GetTx().vin)
        mapNextTx.erase(txin.prevout);

    RemoveUnbroadcastTx(hash, true /* add logging because unchecked */ );

    if (vTxHashes.size() > 1) {
        vTxHashes[it->vTxHashesIdx] = std::move(vTxHashes.back());
        vTxHashes[it->vTxHashesIdx].second->vTxHashesIdx = it->vTxHashesIdx;
        vTxHashes.pop_back();
        if (vTxHashes.size() * 2 < vTxHashes.capacity())
            vTxHashes.shrink_to_fit();
    } else
        vTxHashes.clear();

    if (m_dmnman.load(std::memory_order_acquire)) {
        removeUncheckedProTx(it->GetTx());
    }

    totalTxSize -= it->GetTxSize();
    m_total_fee -= it->GetFee();
    cachedInnerUsage -= it->DynamicMemoryUsage();
    cachedInnerUsage -= memusage::DynamicUsage(it->GetMemPoolParentsConst()) + memusage::DynamicUsage(it->GetMemPoolChildrenConst());
    mapTx.erase(it);
    nTransactionsUpdated++;
    if (minerPolicyEstimator) {minerPolicyEstimator->removeTx(hash, false);}
    removeAddressIndex(hash);
    removeSpentIndex(hash);
}

void CTxMemPool::removeUncheckedProTx(const CTransaction& tx)
{
    auto eraseProTxRef = [&](const uint256& proTxHash, const uint256& txHash) {
        auto its = mapProTxRefs.equal_range(proTxHash);
        for (auto it = its.first; it != its.second;) {
            if (it->second == txHash) {
                it = mapProTxRefs.erase(it);
            } else {
                ++it;
            }
        }
    };

    const uint256 tx_hash{tx.GetHash()};
    if (tx.nType == TRANSACTION_PROVIDER_REGISTER) {
        auto proTx = *Assert(GetTxPayload<CProRegTx>(tx));
        if (!proTx.collateralOutpoint.IsNull()) {
            eraseProTxRef(tx_hash, proTx.collateralOutpoint.hash);
        }
        for (const auto& entry : proTx.netInfo->GetEntries()) {
            mapProTxAddresses.erase(entry);
        }
        mapProTxPubKeyIDs.erase(proTx.keyIDOwner);
        mapProTxBlsPubKeyHashes.erase(proTx.pubKeyOperator.GetHash());
        mapProTxCollaterals.erase(proTx.collateralOutpoint);
        mapProTxCollaterals.erase(COutPoint(tx_hash, proTx.collateralOutpoint.n));
    } else if (tx.nType == TRANSACTION_PROVIDER_UPDATE_SERVICE) {
        auto proTx = *Assert(GetTxPayload<CProUpServTx>(tx));
        eraseProTxRef(proTx.proTxHash, tx_hash);
        for (const auto& entry : proTx.netInfo->GetEntries()) {
            mapProTxAddresses.erase(entry);
        }
    } else if (tx.nType == TRANSACTION_PROVIDER_UPDATE_REGISTRAR) {
        auto proTx = *Assert(GetTxPayload<CProUpRegTx>(tx));
        eraseProTxRef(proTx.proTxHash, tx_hash);
        mapProTxBlsPubKeyHashes.erase(proTx.pubKeyOperator.GetHash());
    } else if (tx.nType == TRANSACTION_PROVIDER_UPDATE_REVOKE) {
        auto proTx = *Assert(GetTxPayload<CProUpRevTx>(tx));
        eraseProTxRef(proTx.proTxHash, tx_hash);
    } else if (tx.nType == TRANSACTION_ASSET_UNLOCK) {
        mapAssetUnlockExpiry.erase(tx_hash);
    }
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

void CTxMemPool::removeForReorg(CChain& chain, std::function<bool(txiter)> check_final_and_mature) EXCLUSIVE_LOCKS_REQUIRED(cs_main)
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
                if (txConflict.nType == TRANSACTION_PROVIDER_REGISTER) {
                    // Remove all other protxes which refer to this protx
                    // NOTE: Can't use equal_range here as every call to removeRecursive might invalidate iterators
                    while (true) {
                        auto itPro = mapProTxRefs.find(txConflict.GetHash());
                        if (itPro == mapProTxRefs.end()) {
                            break;
                        }
                        auto txit = mapTx.find(itPro->second);
                        if (txit != mapTx.end()) {
                            ClearPrioritisation(txit->GetTx().GetHash());
                            removeRecursive(txit->GetTx(), MemPoolRemovalReason::CONFLICT);
                        } else {
                            mapProTxRefs.erase(itPro);
                        }
                    }
                }
                ClearPrioritisation(txConflict.GetHash());
                removeRecursive(txConflict, MemPoolRemovalReason::CONFLICT);
            }
        }
    }
}

void CTxMemPool::removeProTxPubKeyConflicts(const CTransaction &tx, const CKeyID &keyId)
{
    if (mapProTxPubKeyIDs.count(keyId)) {
        uint256 conflictHash = mapProTxPubKeyIDs[keyId];
        if (conflictHash != tx.GetHash() && mapTx.count(conflictHash)) {
            removeRecursive(mapTx.find(conflictHash)->GetTx(), MemPoolRemovalReason::CONFLICT);
        }
    }
}

void CTxMemPool::removeProTxPubKeyConflicts(const CTransaction &tx, const CBLSLazyPublicKey &pubKey)
{
    if (mapProTxBlsPubKeyHashes.count(pubKey.GetHash())) {
        uint256 conflictHash = mapProTxBlsPubKeyHashes[pubKey.GetHash()];
        if (conflictHash != tx.GetHash() && mapTx.count(conflictHash)) {
            removeRecursive(mapTx.find(conflictHash)->GetTx(), MemPoolRemovalReason::CONFLICT);
        }
    }
}

void CTxMemPool::removeProTxCollateralConflicts(const CTransaction &tx, const COutPoint &collateralOutpoint)
{
    if (mapProTxCollaterals.count(collateralOutpoint)) {
        uint256 conflictHash = mapProTxCollaterals[collateralOutpoint];
        if (conflictHash != tx.GetHash() && mapTx.count(conflictHash)) {
            removeRecursive(mapTx.find(conflictHash)->GetTx(), MemPoolRemovalReason::CONFLICT);
        }
    }
}

void CTxMemPool::removeProTxSpentCollateralConflicts(const CTransaction &tx)
{
    auto dmnman = Assert(m_dmnman.load(std::memory_order_acquire));

    // Remove TXs that refer to a MN for which the collateral was spent
    auto removeSpentCollateralConflict = [&](const uint256& proTxHash) EXCLUSIVE_LOCKS_REQUIRED(cs) {
        // Can't use equal_range here as every call to removeRecursive might invalidate iterators
        AssertLockHeld(cs);
        while (true) {
            auto it = mapProTxRefs.find(proTxHash);
            if (it == mapProTxRefs.end()) {
                break;
            }
            auto conflictIt = mapTx.find(it->second);
            if (conflictIt != mapTx.end()) {
                removeRecursive(conflictIt->GetTx(), MemPoolRemovalReason::CONFLICT);
            } else {
                // Should not happen as we track referencing TXs in addUnchecked/removeUnchecked.
                // But lets be on the safe side and not run into an endless loop...
                LogPrint(BCLog::MEMPOOL, "%s: ERROR: found invalid TX ref in mapProTxRefs, proTxHash=%s, txHash=%s\n", __func__, proTxHash.ToString(), it->second.ToString());
                mapProTxRefs.erase(it);
            }
        }
    };
    auto mnList = dmnman->GetListAtChainTip();
    for (const auto& in : tx.vin) {
        auto collateralIt = mapProTxCollaterals.find(in.prevout);
        if (collateralIt != mapProTxCollaterals.end()) {
            // These are not yet mined ProRegTxs
            removeSpentCollateralConflict(collateralIt->second);
        }
        auto dmn = mnList.GetMNByCollateral(in.prevout);
        if (dmn) {
            // These are updates referring to a mined ProRegTx
            removeSpentCollateralConflict(dmn->proTxHash);
        }
    }
}

void CTxMemPool::removeProTxKeyChangedConflicts(const CTransaction &tx, const uint256& proTxHash, const uint256& newKeyHash)
{
    std::set<uint256> conflictingTxs;
    for (auto its = mapProTxRefs.equal_range(proTxHash); its.first != its.second; ++its.first) {
        auto txit = mapTx.find(its.first->second);
        if (txit == mapTx.end()) {
            continue;
        }
        if (txit->validForProTxKey != newKeyHash) {
            conflictingTxs.emplace(txit->GetTx().GetHash());
        }
    }
    for (const auto& txHash : conflictingTxs) {
        auto& tx = mapTx.find(txHash)->GetTx();
        removeRecursive(tx, MemPoolRemovalReason::CONFLICT);
    }
}

void CTxMemPool::removeProTxConflicts(const CTransaction &tx)
{
    removeProTxSpentCollateralConflicts(tx);

    const uint256 tx_hash{tx.GetHash()};
    if (tx.nType == TRANSACTION_PROVIDER_REGISTER) {
        const auto opt_proTx = GetTxPayload<CProRegTx>(tx);
        if (!opt_proTx) {
            LogPrint(BCLog::MEMPOOL, "%s: ERROR: Invalid transaction payload, tx: %s\n", __func__, tx_hash.ToString());
            return;
        }
        auto& proTx = *opt_proTx;

        for (const auto& entry : proTx.netInfo->GetEntries()) {
            if (mapProTxAddresses.count(entry)) {
                uint256 conflictHash = mapProTxAddresses[entry];
                if (conflictHash != tx_hash && mapTx.count(conflictHash)) {
                    removeRecursive(mapTx.find(conflictHash)->GetTx(), MemPoolRemovalReason::CONFLICT);
                }
            }
        }
        removeProTxPubKeyConflicts(tx, proTx.keyIDOwner);
        removeProTxPubKeyConflicts(tx, proTx.pubKeyOperator);
        if (!proTx.collateralOutpoint.hash.IsNull()) {
            removeProTxCollateralConflicts(tx, proTx.collateralOutpoint);
        } else {
            removeProTxCollateralConflicts(tx, COutPoint(tx_hash, proTx.collateralOutpoint.n));
        }
    } else if (tx.nType == TRANSACTION_PROVIDER_UPDATE_SERVICE) {
        const auto opt_proTx = GetTxPayload<CProUpServTx>(tx);
        if (!opt_proTx) {
            LogPrint(BCLog::MEMPOOL, "%s: ERROR: Invalid transaction payload, tx: %s\n", __func__, tx_hash.ToString());
            return;
        }

        for (const auto& entry : opt_proTx->netInfo->GetEntries()) {
            if (mapProTxAddresses.count(entry)) {
                uint256 conflictHash = mapProTxAddresses[entry];
                if (conflictHash != tx_hash && mapTx.count(conflictHash)) {
                    removeRecursive(mapTx.find(conflictHash)->GetTx(), MemPoolRemovalReason::CONFLICT);
                }
            }
        }
    } else if (tx.nType == TRANSACTION_PROVIDER_UPDATE_REGISTRAR) {
        const auto opt_proTx = GetTxPayload<CProUpRegTx>(tx);
        if (!opt_proTx) {
            LogPrint(BCLog::MEMPOOL, "%s: ERROR: Invalid transaction payload, tx: %s\n", __func__, tx_hash.ToString());
            return;
        }

        removeProTxPubKeyConflicts(tx, opt_proTx->pubKeyOperator);
        removeProTxKeyChangedConflicts(tx, opt_proTx->proTxHash, ::SerializeHash(opt_proTx->pubKeyOperator));
    } else if (tx.nType == TRANSACTION_PROVIDER_UPDATE_REVOKE) {
        const auto opt_proTx = GetTxPayload<CProUpRevTx>(tx);
        if (!opt_proTx) {
            LogPrint(BCLog::MEMPOOL, "%s: ERROR: Invalid transaction payload, tx: %s\n", __func__, tx_hash.ToString());
            return;
        }

        removeProTxKeyChangedConflicts(tx, opt_proTx->proTxHash, ::SerializeHash(CBLSPublicKey()));
    }
}

/**
 * Called when a block is connected. Removes from mempool and updates the miner fee estimator.
 */
void CTxMemPool::removeForBlock(const std::vector<CTransactionRef>& vtx, unsigned int nBlockHeight)
{
    AssertLockHeld(cs);
    std::vector<const CTxMemPoolEntry*> entries;
    for (const auto& tx : vtx)
    {
        uint256 hash = tx->GetHash();

        indexed_transaction_set::iterator i = mapTx.find(hash);
        if (i != mapTx.end())
            entries.push_back(&*i);
    }
    // Before the txs in the new block have been removed from the mempool, update policy estimates
    if (minerPolicyEstimator) {minerPolicyEstimator->processBlock(nBlockHeight, entries);}
    for (const auto& tx : vtx)
    {
        txiter it = mapTx.find(tx->GetHash());
        if (it != mapTx.end()) {
            setEntries stage;
            stage.insert(it);
            RemoveStaged(stage, true, MemPoolRemovalReason::BLOCK);
        }
        removeConflicts(*tx);
        if (m_dmnman.load(std::memory_order_acquire)) {
            removeProTxConflicts(*tx);
        }
        ClearPrioritisation(tx->GetHash());
    }
    lastRollingFeeUpdate = GetTime();
    blockSinceLastRollingFeeBump = true;
}

/**
  * Called when a length of chain is increased. Removes from mempool expired asset-unlock transactions
  */
void CTxMemPool::removeExpiredAssetUnlock(int nBlockHeight)
{
    AssertLockHeld(cs);
    // items to removed should be firstly collected to independent list,
    // because removing items by `removeRecursive` changes the mapAssetUnlockExpiry
    std::vector<CTransactionRef> entries;
    for (const auto& item: mapAssetUnlockExpiry) {
        if (item.second < nBlockHeight) {
            entries.push_back(get(item.first));
        }
    }
    for (const auto& tx : entries) {
        removeRecursive(*tx, MemPoolRemovalReason::EXPIRY);
    }
}

void CTxMemPool::_clear()
{
    vTxHashes.clear();
    mapTx.clear();
    mapNextTx.clear();
    mapProTxAddresses.clear();
    mapProTxPubKeyIDs.clear();
    totalTxSize = 0;
    m_total_fee = 0;
    cachedInnerUsage = 0;
    lastRollingFeeUpdate = GetTime();
    blockSinceLastRollingFeeBump = false;
    rollingMinimumFeeRate = 0;
    ++nTransactionsUpdated;
}

void CTxMemPool::clear()
{
    LOCK(cs);
    _clear();
}

void CTxMemPool::check(const CCoinsViewCache& active_coins_tip, int64_t spendheight) const
{
    if (m_check_ratio == 0) return;

    if (GetRand(m_check_ratio) >= 1) return;

    AssertLockHeld(::cs_main);
    LOCK(cs);
    LogPrint(BCLog::MEMPOOL, "Checking mempool with %u transactions and %u inputs\n", (unsigned int)mapTx.size(), (unsigned int)mapNextTx.size());

    uint64_t checkTotal = 0;
    CAmount check_total_fee{0};
    uint64_t innerUsage = 0;
    uint64_t prev_ancestor_count{0};

    CCoinsViewCache mempoolDuplicate(const_cast<CCoinsViewCache*>(&active_coins_tip));

    for (const auto& it : GetSortedDepthAndScore()) {
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
            // We are iterating through the mempool entries sorted in order by ancestor count.
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
        // Verify ancestor state is correct.
        setEntries setAncestors;
        uint64_t nNoLimit = std::numeric_limits<uint64_t>::max();
        std::string dummy;
        CalculateMemPoolAncestors(*it, setAncestors, nNoLimit, nNoLimit, nNoLimit, nNoLimit, dummy);
        uint64_t nCountCheck = setAncestors.size() + 1;
        uint64_t nSizeCheck = it->GetTxSize();
        CAmount nFeesCheck = it->GetModifiedFee();
        unsigned int nSigOpCheck = it->GetSigOpCount();

        for (txiter ancestorIt : setAncestors) {
            nSizeCheck += ancestorIt->GetTxSize();
            nFeesCheck += ancestorIt->GetModifiedFee();
            nSigOpCheck += ancestorIt->GetSigOpCount();
        }

        assert(it->GetCountWithAncestors() == nCountCheck);
        assert(it->GetSizeWithAncestors() == nSizeCheck);
        assert(it->GetSigOpCountWithAncestors() == nSigOpCheck);
        assert(it->GetModFeesWithAncestors() == nFeesCheck);
        // Sanity check: we are walking in ascending ancestor count order.
        assert(prev_ancestor_count <= it->GetCountWithAncestors());
        prev_ancestor_count = it->GetCountWithAncestors();

        // Check children against mapNextTx
        CTxMemPoolEntry::Children setChildrenCheck;
        auto iter = mapNextTx.lower_bound(COutPoint(it->GetTx().GetHash(), 0));
        uint64_t child_sizes = 0;
        for (; iter != mapNextTx.end() && iter->first->hash == it->GetTx().GetHash(); ++iter) {
            txiter childit = mapTx.find(iter->second->GetHash());
            assert(childit != mapTx.end()); // mapNextTx points to in-mempool transactions
            if (setChildrenCheck.insert(*childit).second) {
                child_sizes += childit->GetTxSize();
            }
        }
        assert(setChildrenCheck.size() == it->GetMemPoolChildrenConst().size());
        assert(std::equal(setChildrenCheck.begin(), setChildrenCheck.end(), it->GetMemPoolChildrenConst().begin(), comp));
        // Also check to make sure size is greater than sum with immediate children.
        // just a sanity check, not definitive that this calc is correct...
        assert(it->GetSizeWithDescendants() >= child_sizes + it->GetTxSize());

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

bool CTxMemPool::CompareDepthAndScore(const uint256& hasha, const uint256& hashb)
{
    /* Return `true` if hasha should be considered sooner than hashb. Namely when:
     *   a is not in the mempool, but b is
     *   both are in the mempool and a has fewer ancestors than b
     *   both are in the mempool and a has a higher score than b
     */
    LOCK(cs);
    indexed_transaction_set::const_iterator j = mapTx.find(hashb);
    if (j == mapTx.end()) return false;
    indexed_transaction_set::const_iterator i = mapTx.find(hasha);
    if (i == mapTx.end()) return true;
    uint64_t counta = i->GetCountWithAncestors();
    uint64_t countb = j->GetCountWithAncestors();
    if (counta == countb) {
        return CompareTxMemPoolEntryByScore()(*i, *j);
    }
    return counta < countb;
}

namespace {
class DepthAndScoreComparator
{
public:
    bool operator()(const CTxMemPool::indexed_transaction_set::const_iterator& a, const CTxMemPool::indexed_transaction_set::const_iterator& b)
    {
        uint64_t counta = a->GetCountWithAncestors();
        uint64_t countb = b->GetCountWithAncestors();
        if (counta == countb) {
            return CompareTxMemPoolEntryByScore()(*a, *b);
        }
        return counta < countb;
    }
};
} // namespace

std::vector<CTxMemPool::indexed_transaction_set::const_iterator> CTxMemPool::GetSortedDepthAndScore() const
{
    std::vector<indexed_transaction_set::const_iterator> iters;
    AssertLockHeld(cs);

    iters.reserve(mapTx.size());

    for (indexed_transaction_set::iterator mi = mapTx.begin(); mi != mapTx.end(); ++mi) {
        iters.push_back(mi);
    }
    std::sort(iters.begin(), iters.end(), DepthAndScoreComparator());
    return iters;
}

void CTxMemPool::queryHashes(std::vector<uint256>& vtxid) const
{
    LOCK(cs);
    auto iters = GetSortedDepthAndScore();

    vtxid.clear();
    vtxid.reserve(mapTx.size());

    for (auto it : iters) {
        vtxid.push_back(it->GetTx().GetHash());
    }
}

static TxMempoolInfo GetInfo(CTxMemPool::indexed_transaction_set::const_iterator it) {
    return TxMempoolInfo{it->GetSharedTx(), it->GetTime(), it->GetFee(), it->GetTxSize(), it->GetModifiedFee() - it->GetFee()};
}

std::vector<TxMempoolInfo> CTxMemPool::infoAll() const
{
    LOCK(cs);
    auto iters = GetSortedDepthAndScore();

    std::vector<TxMempoolInfo> ret;
    ret.reserve(mapTx.size());
    for (auto it : iters) {
        ret.push_back(GetInfo(it));
    }

    return ret;
}

CTransactionRef CTxMemPool::get(const uint256& hash) const
{
    LOCK(cs);
    indexed_transaction_set::const_iterator i = mapTx.find(hash);
    if (i == mapTx.end())
        return nullptr;
    return i->GetSharedTx();
}

TxMempoolInfo CTxMemPool::info(const uint256& hash) const
{
    LOCK(cs);
    indexed_transaction_set::const_iterator i = mapTx.find(hash);
    if (i == mapTx.end())
        return TxMempoolInfo();
    return GetInfo(i);
}

bool CTxMemPool::existsProviderTxConflict(const CTransaction &tx) const {
    auto dmnman = Assert(m_dmnman.load(std::memory_order_acquire));

    LOCK(cs);

    auto hasKeyChangeInMempool = [&](const uint256& proTxHash) EXCLUSIVE_LOCKS_REQUIRED(cs) {
        AssertLockHeld(cs);
        for (auto its = mapProTxRefs.equal_range(proTxHash); its.first != its.second; ++its.first) {
            auto txit = mapTx.find(its.first->second);
            if (txit == mapTx.end()) {
                continue;
            }
            if (txit->isKeyChangeProTx) {
                return true;
            }
        }
        return false;
    };

    const uint256 tx_hash{tx.GetHash()};
    if (tx.nType == TRANSACTION_PROVIDER_REGISTER) {
        const auto opt_proTx = GetTxPayload<CProRegTx>(tx);
        if (!opt_proTx) {
            LogPrint(BCLog::MEMPOOL, "%s: ERROR: Invalid transaction payload, tx: %s\n", __func__, tx_hash.ToString());
            return true; // i.e. can't decode payload == conflict
        }
        auto& proTx = *opt_proTx;
        for (const auto& entry : proTx.netInfo->GetEntries()) {
            if (mapProTxAddresses.count(entry)) {
                return true;
            }
        }
        if (mapProTxPubKeyIDs.count(proTx.keyIDOwner) || mapProTxBlsPubKeyHashes.count(proTx.pubKeyOperator.GetHash())) {
            return true;
        }
        if (!proTx.collateralOutpoint.hash.IsNull()) {
            if (mapProTxCollaterals.count(proTx.collateralOutpoint)) {
                // there is another ProRegTx that refers to the same collateral
                return true;
            }
            if (mapNextTx.count(proTx.collateralOutpoint)) {
                // there is another tx that spends the collateral
                return true;
            }
        }
        return false;
    } else if (tx.nType == TRANSACTION_PROVIDER_UPDATE_SERVICE) {
        const auto opt_proTx = GetTxPayload<CProUpServTx>(tx);
        if (!opt_proTx) {
            LogPrint(BCLog::MEMPOOL, "%s: ERROR: Invalid transaction payload, tx: %s\n", __func__, tx_hash.ToString());
            return true; // i.e. can't decode payload == conflict
        }
        for (const auto& entry : opt_proTx->netInfo->GetEntries()) {
            auto it = mapProTxAddresses.find(entry);
            if (it != mapProTxAddresses.end() && it->second != opt_proTx->proTxHash) {
                return true;
            }
        }
    } else if (tx.nType == TRANSACTION_PROVIDER_UPDATE_REGISTRAR) {
        const auto opt_proTx = GetTxPayload<CProUpRegTx>(tx);
        if (!opt_proTx) {
            LogPrint(BCLog::MEMPOOL, "%s: ERROR: Invalid transaction payload, tx: %s\n", __func__, tx_hash.ToString());
            return true; // i.e. can't decode payload == conflict
        }
        auto& proTx = *opt_proTx;

        // this method should only be called with validated ProTxs
        auto dmn = dmnman->GetListAtChainTip().GetMN(proTx.proTxHash);
        if (!dmn) {
            LogPrint(BCLog::MEMPOOL, "%s: ERROR: Masternode is not in the list, proTxHash: %s\n", __func__, proTx.proTxHash.ToString());
            return true; // i.e. failed to find validated ProTx == conflict
        }
        // only allow one operator key change in the mempool
        if (dmn->pdmnState->pubKeyOperator != proTx.pubKeyOperator) {
            if (hasKeyChangeInMempool(proTx.proTxHash)) {
                return true;
            }
        }

        auto it = mapProTxBlsPubKeyHashes.find(proTx.pubKeyOperator.GetHash());
        return it != mapProTxBlsPubKeyHashes.end() && it->second != proTx.proTxHash;
    } else if (tx.nType == TRANSACTION_PROVIDER_UPDATE_REVOKE) {
        const auto opt_proTx = GetTxPayload<CProUpRevTx>(tx);
        if (!opt_proTx) {
            LogPrint(BCLog::MEMPOOL, "%s: ERROR: Invalid transaction payload, tx: %s\n", __func__, tx_hash.ToString());
            return true; // i.e. can't decode payload == conflict
        }
        auto& proTx = *opt_proTx;
        // this method should only be called with validated ProTxs
        auto dmn = dmnman->GetListAtChainTip().GetMN(proTx.proTxHash);
        if (!dmn) {
            LogPrint(BCLog::MEMPOOL, "%s: ERROR: Masternode is not in the list, proTxHash: %s\n", __func__, proTx.proTxHash.ToString());
            return true; // i.e. failed to find validated ProTx == conflict
        }
        // only allow one operator key change in the mempool
        if (dmn->pdmnState->pubKeyOperator.Get() != CBLSPublicKey()) {
            if (hasKeyChangeInMempool(proTx.proTxHash)) {
                return true;
            }
        }
    }
    return false;
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
            // Now update all ancestors' modified fees with descendants
            setEntries setAncestors;
            uint64_t nNoLimit = std::numeric_limits<uint64_t>::max();
            std::string dummy;
            CalculateMemPoolAncestors(*it, setAncestors, nNoLimit, nNoLimit, nNoLimit, nNoLimit, dummy, false);
            for (txiter ancestorIt : setAncestors) {
                mapTx.modify(ancestorIt, [=](CTxMemPoolEntry& e){ e.UpdateDescendantState(0, nFeeDelta, 0);});
            }
            // Now update all descendants' modified fees with ancestors
            setEntries setDescendants;
            CalculateDescendants(it, setDescendants);
            setDescendants.erase(it);
            for (txiter descendantIt : setDescendants) {
                mapTx.modify(descendantIt, [=](CTxMemPoolEntry& e){ e.UpdateAncestorState(0, nFeeDelta, 0, 0); });
            }
            ++nTransactionsUpdated;
        }
    }
    LogPrint(BCLog::MEMPOOL, "PrioritiseTransaction: %s fee += %s\n", hash.ToString(), FormatMoney(nFeeDelta));
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

CTxMemPool::setEntries CTxMemPool::GetIterSet(const std::set<uint256>& hashes) const
{
    CTxMemPool::setEntries ret;
    for (const auto& h : hashes) {
        const auto mi = GetIter(h);
        if (mi) ret.insert(*mi);
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
    }
}

size_t CTxMemPool::DynamicMemoryUsage() const {
    LOCK(cs);
    // Estimate the overhead of mapTx to be 12 pointers + an allocation, as no exact formula for boost::multi_index_contained is implemented.
    return memusage::MallocUsage(sizeof(CTxMemPoolEntry) + 12 * sizeof(void*)) * mapTx.size() + memusage::DynamicUsage(mapNextTx) + memusage::DynamicUsage(mapDeltas) + memusage::DynamicUsage(vTxHashes) + cachedInnerUsage;
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
    UpdateForRemoveFromMempool(stage, updateDescendants);
    for (txiter it : stage) {
        removeUnchecked(it, reason);
    }
}

int CTxMemPool::Expire(std::chrono::seconds time)
{
    AssertLockHeld(cs);
    auto isman = Assert(m_isman.load(std::memory_order_acquire));
    indexed_transaction_set::index<entry_time>::type::iterator it = mapTx.get<entry_time>().begin();
    setEntries toremove;
    while (it != mapTx.get<entry_time>().end() && it->GetTime() < time) {
        // locked txes do not expire until mined and have sufficient confirmations
        if (isman->IsLocked(it->GetTx().GetHash())) {
            it++;
            continue;
        }
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

void CTxMemPool::addUnchecked(const CTxMemPoolEntry &entry, bool validFeeEstimate)
{
    setEntries setAncestors;
    uint64_t nNoLimit = std::numeric_limits<uint64_t>::max();
    std::string dummy;
    CalculateMemPoolAncestors(entry, setAncestors, nNoLimit, nNoLimit, nNoLimit, nNoLimit, dummy);
    return addUnchecked(entry, setAncestors, validFeeEstimate);
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

        if (rollingMinimumFeeRate < (double)incrementalRelayFee.GetFeePerK() / 2) {
            rollingMinimumFeeRate = 0;
            return CFeeRate(0);
        }
    }
    return std::max(CFeeRate(llround(rollingMinimumFeeRate)), incrementalRelayFee);
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
    while (!mapTx.empty() && DynamicMemoryUsage() > sizelimit) {
        indexed_transaction_set::index<descendant_score>::type::iterator it = mapTx.get<descendant_score>().begin();

        // We set the new mempool min fee to the feerate of the removed set, plus the
        // "minimum reasonable fee rate" (ie some value under which we consider txn
        // to have 0 fee). This way, we don't allow txn to enter mempool with feerate
        // equal to txn which were removed with no block in between.
        CFeeRate removed(it->GetModFeesWithDescendants(), it->GetSizeWithDescendants());
        removed += incrementalRelayFee;
        trackPackageRemoved(removed);
        maxFeeRateRemoved = std::max(maxFeeRateRemoved, removed);

        setEntries stage;
        CalculateDescendants(mapTx.project<0>(it), stage);
        nTxnRemoved += stage.size();

        std::vector<CTransaction> txn;
        if (pvNoSpendsRemaining) {
            txn.reserve(stage.size());
            for (txiter iter : stage)
                txn.push_back(iter->GetTx());
        }
        RemoveStaged(stage, false, MemPoolRemovalReason::SIZELIMIT);
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
        LogPrint(BCLog::MEMPOOL, "Removed %u txn, rolling minimum fee bumped to %s\n", nTxnRemoved, maxFeeRateRemoved.ToString());
    }
}

uint64_t CTxMemPool::CalculateDescendantMaximum(txiter entry) const {
    // find parent with highest descendant count
    std::vector<txiter> candidates;
    setEntries counted;
    candidates.push_back(entry);
    uint64_t maximum = 0;
    while (candidates.size()) {
        txiter candidate = candidates.back();
        candidates.pop_back();
        if (!counted.insert(candidate).second) continue;
        const CTxMemPoolEntry::Parents& parents = candidate->GetMemPoolParentsConst();
        if (parents.size() == 0) {
            maximum = std::max(maximum, candidate->GetCountWithDescendants());
        } else {
            for (const CTxMemPoolEntry& i : parents) {
                candidates.push_back(mapTx.iterator_to(i));
            }
        }
    }
    return maximum;
}

void CTxMemPool::GetTransactionAncestry(const uint256& txid, size_t& ancestors, size_t& descendants, size_t* const ancestorsize, CAmount* const ancestorfees) const {
    LOCK(cs);
    auto it = mapTx.find(txid);
    ancestors = descendants = 0;
    if (it != mapTx.end()) {
        ancestors = it->GetCountWithAncestors();
        if (ancestorsize) *ancestorsize = it->GetSizeWithAncestors();
        if (ancestorfees) *ancestorfees = it->GetModFeesWithAncestors();
        descendants = CalculateDescendantMaximum(it);
    }
}

bool CTxMemPool::IsLoaded() const
{
    LOCK(cs);
    return m_is_loaded;
}

void CTxMemPool::SetIsLoaded(bool loaded)
{
    LOCK(cs);
    m_is_loaded = loaded;
}


std::string RemovalReasonToString(const MemPoolRemovalReason& r) noexcept
{
    switch (r) {
        case MemPoolRemovalReason::EXPIRY: return "expiry";
        case MemPoolRemovalReason::SIZELIMIT: return "sizelimit";
        case MemPoolRemovalReason::REORG: return "reorg";
        case MemPoolRemovalReason::BLOCK: return "block";
        case MemPoolRemovalReason::CONFLICT: return "conflict";
        case MemPoolRemovalReason::MANUAL: return "manual";
    }
    assert(false);
}
