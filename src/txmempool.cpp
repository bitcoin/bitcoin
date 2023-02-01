// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <txmempool.h>

#include <chain.h>
#include <coins.h>
#include <consensus/consensus.h>
#include <consensus/tx_verify.h>
#include <consensus/validation.h>
#include <policy/fees.h>
#include <policy/policy.h>
#include <policy/settings.h>
#include <reverse_iterator.h>
#include <util/check.h>
#include <util/moneystr.h>
#include <util/overflow.h>
#include <util/result.h>
#include <util/system.h>
#include <util/time.h>
#include <util/translation.h>
#include <validationinterface.h>
// SYSCOIN
#include <util/rbf.h>
#include <evo/specialtx.h>
#include <evo/providertx.h>
#include <evo/deterministicmns.h>   
extern bool EraseNEVMData(const NEVMDataVec&);
extern NEVMMintTxMap mapMintKeysMempool;
extern std::unordered_map<COutPoint, std::pair<CTransactionRef, CTransactionRef>, SaltedOutpointHasher> mapAssetAllocationConflicts;

#include <cmath>
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

void CTxMemPool::UpdateForDescendants(txiter updateIt, cacheMap& cachedDescendants,
                                      const std::set<uint256>& setExclude, std::set<uint256>& descendants_to_remove)
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
              e.UpdateAncestorState(updateIt->GetTxSize(), updateIt->GetModifiedFee(), 1, updateIt->GetSigOpCost());
            });
            // Don't directly remove the transaction here -- doing so would
            // invalidate iterators in cachedDescendants. Mark it for removal
            // by inserting into descendants_to_remove.
            if (descendant.GetCountWithAncestors() > uint64_t(m_limits.ancestor_count) || descendant.GetSizeWithAncestors() > uint64_t(m_limits.ancestor_size_vbytes)) {
                descendants_to_remove.insert(descendant.GetTx().GetHash());
            }
        }
    }
    mapTx.modify(updateIt, [=](CTxMemPoolEntry& e) { e.UpdateDescendantState(modifySize, modifyFee, modifyCount); });
}

void CTxMemPool::UpdateTransactionsFromBlock(const std::vector<uint256>& vHashesToUpdate)
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
    for (const uint256 &hash : reverse_iterate(vHashesToUpdate)) {
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
        UpdateForDescendants(it, mapMemPoolDescendantsToUpdate, setAlreadyIncluded, descendants_to_remove);
    }

    for (const auto& txid : descendants_to_remove) {
        // This txid may have been removed already in a prior call to removeRecursive.
        // Therefore we ensure it is not yet removed already.
        if (const std::optional<txiter> txiter = GetIter(txid)) {
            removeRecursive((*txiter)->GetTx(), MemPoolRemovalReason::SIZELIMIT);
        }
    }
}

util::Result<CTxMemPool::setEntries> CTxMemPool::CalculateAncestorsAndCheckLimits(
    size_t entry_size,
    size_t entry_count,
    CTxMemPoolEntry::Parents& staged_ancestors,
    const Limits& limits) const
{
    size_t totalSizeWithAncestors = entry_size;
    setEntries ancestors;

    while (!staged_ancestors.empty()) {
        const CTxMemPoolEntry& stage = staged_ancestors.begin()->get();
        txiter stageit = mapTx.iterator_to(stage);

        ancestors.insert(stageit);
        staged_ancestors.erase(stage);
        totalSizeWithAncestors += stageit->GetTxSize();

        if (stageit->GetSizeWithDescendants() + entry_size > static_cast<uint64_t>(limits.descendant_size_vbytes)) {
            return util::Error{Untranslated(strprintf("exceeds descendant size limit for tx %s [limit: %u]", stageit->GetTx().GetHash().ToString(), limits.descendant_size_vbytes))};
        } else if (stageit->GetCountWithDescendants() + entry_count > static_cast<uint64_t>(limits.descendant_count)) {
            return util::Error{Untranslated(strprintf("too many descendants for tx %s [limit: %u]", stageit->GetTx().GetHash().ToString(), limits.descendant_count))};
        } else if (totalSizeWithAncestors > static_cast<uint64_t>(limits.ancestor_size_vbytes)) {
            return util::Error{Untranslated(strprintf("exceeds ancestor size limit [limit: %u]", limits.ancestor_size_vbytes))};
        }

        const CTxMemPoolEntry::Parents& parents = stageit->GetMemPoolParentsConst();
        for (const CTxMemPoolEntry& parent : parents) {
            txiter parent_it = mapTx.iterator_to(parent);

            // If this is a new ancestor, add it.
            if (ancestors.count(parent_it) == 0) {
                staged_ancestors.insert(parent);
            }
            if (staged_ancestors.size() + ancestors.size() + entry_count > static_cast<uint64_t>(limits.ancestor_count)) {
                return util::Error{Untranslated(strprintf("too many unconfirmed ancestors [limit: %u]", limits.ancestor_count))};
            }
        }
    }

    return ancestors;
}

bool CTxMemPool::CheckPackageLimits(const Package& package,
                                    const Limits& limits,
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
                if (staged_ancestors.size() + package.size() > static_cast<uint64_t>(limits.ancestor_count)) {
                    errString = strprintf("too many unconfirmed parents [limit: %u]", limits.ancestor_count);
                    return false;
                }
            }
        }
    }
    // When multiple transactions are passed in, the ancestors and descendants of all transactions
    // considered together must be within limits even if they are not interdependent. This may be
    // stricter than the limits for each individual transaction.
    const auto ancestors{CalculateAncestorsAndCheckLimits(total_size, package.size(),
                                                          staged_ancestors, limits)};
    // It's possible to overestimate the ancestor/descendant totals.
    if (!ancestors.has_value()) errString = "possibly " + util::ErrorString(ancestors).original;
    return ancestors.has_value();
}

util::Result<CTxMemPool::setEntries> CTxMemPool::CalculateMemPoolAncestors(
    const CTxMemPoolEntry &entry,
    const Limits& limits,
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
                if (staged_ancestors.size() + 1 > static_cast<uint64_t>(limits.ancestor_count)) {
                    return util::Error{Untranslated(strprintf("too many unconfirmed parents [limit: %u]", limits.ancestor_count))};
                }
            }
        }
    } else {
        // If we're not searching for parents, we require this to already be an
        // entry in the mempool and use the entry's cached parents.
        txiter it = mapTx.iterator_to(entry);
        staged_ancestors = it->GetMemPoolParentsConst();
    }

    return CalculateAncestorsAndCheckLimits(entry.GetTxSize(), /*entry_count=*/1, staged_ancestors,
                                            limits);
}

CTxMemPool::setEntries CTxMemPool::AssumeCalculateMemPoolAncestors(
    std::string_view calling_fn_name,
    const CTxMemPoolEntry &entry,
    const Limits& limits,
    bool fSearchForParents /* = true */) const
{
    auto result{CalculateMemPoolAncestors(entry, limits, fSearchForParents)};
    if (!Assume(result)) {
        LogPrintLevel(BCLog::MEMPOOL, BCLog::Level::Error, "%s: CalculateMemPoolAncestors failed unexpectedly, continuing with empty ancestor set (%s)\n",
                      calling_fn_name, util::ErrorString(result).original);
    }
    return std::move(result).value_or(CTxMemPool::setEntries{});
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
    int64_t updateSigOpsCost = 0;
    for (txiter ancestorIt : setAncestors) {
        updateSize += ancestorIt->GetTxSize();
        updateFee += ancestorIt->GetModifiedFee();
        updateSigOpsCost += ancestorIt->GetSigOpCost();
    }
    mapTx.modify(it, [=](CTxMemPoolEntry& e){ e.UpdateAncestorState(updateSize, updateFee, updateCount, updateSigOpsCost); });
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
            int modifySigOps = -removeIt->GetSigOpCost();
            for (txiter dit : setDescendants) {
                mapTx.modify(dit, [=](CTxMemPoolEntry& e){ e.UpdateAncestorState(modifySize, modifyFee, -1, modifySigOps); });
            }
        }
    }
    for (txiter removeIt : entriesToRemove) {
        const CTxMemPoolEntry &entry = *removeIt;
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
        auto ancestors{AssumeCalculateMemPoolAncestors(__func__, entry, Limits::NoLimits(), /*fSearchForParents=*/false)};
        // Note that UpdateAncestorsOf severs the child links that point to
        // removeIt in the entries for the parents of removeIt.
        UpdateAncestorsOf(false, removeIt, ancestors);
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
    nSigOpCostWithAncestors += modifySigOps;
    assert(int(nSigOpCostWithAncestors) >= 0);
}

CTxMemPool::CTxMemPool(const Options& opts)
    : m_check_ratio{opts.check_ratio},
      minerPolicyEstimator{opts.estimator},
      m_max_size_bytes{opts.max_size_bytes},
      m_expiry{opts.expiry},
      m_incremental_relay_feerate{opts.incremental_relay_feerate},
      m_min_relay_feerate{opts.min_relay_feerate},
      m_dust_relay_feerate{opts.dust_relay_feerate},
      m_permit_bare_multisig{opts.permit_bare_multisig},
      m_max_datacarrier_bytes{opts.max_datacarrier_bytes},
      m_require_standard{opts.require_standard},
      m_full_rbf{opts.full_rbf},
      m_limits{opts.limits}
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

    vTxHashes.emplace_back(tx.GetWitnessHash(), newit);
    newit->vTxHashesIdx = vTxHashes.size() - 1;
    // SYSCOIN
    // Invalid ProTxes should never get this far because transactions should be
    // fully checked by AcceptToMemoryPool() at this point, so we just assume that
    // everything is fine here.
    if (tx.nVersion == SYSCOIN_TX_VERSION_MN_REGISTER) {
        CProRegTx proTx;
        if(GetTxPayload(tx, proTx)) {
            if (!proTx.collateralOutpoint.hash.IsNull()) {
                mapProTxRefs.emplace(tx.GetHash(), proTx.collateralOutpoint.hash);
            }
            mapProTxAddresses.emplace(proTx.addr, tx.GetHash());
            mapProTxPubKeyIDs.emplace(proTx.keyIDOwner, tx.GetHash());
            mapProTxBlsPubKeyHashes.emplace(proTx.pubKeyOperator.GetHash(), tx.GetHash());
            if (!proTx.collateralOutpoint.hash.IsNull()) {
                mapProTxCollaterals.emplace(proTx.collateralOutpoint, tx.GetHash());
            } else {
                mapProTxCollaterals.emplace(COutPoint(tx.GetHash(), proTx.collateralOutpoint.n), tx.GetHash());
            }
        }
    } else if (tx.nVersion == SYSCOIN_TX_VERSION_MN_UPDATE_SERVICE) {
        CProUpServTx proTx;
        if(GetTxPayload(tx, proTx)) {
            mapProTxRefs.emplace(proTx.proTxHash, tx.GetHash());
            mapProTxAddresses.emplace(proTx.addr, tx.GetHash());
        }
    } else if (tx.nVersion == SYSCOIN_TX_VERSION_MN_UPDATE_REGISTRAR) {
        CProUpRegTx proTx;
        if(GetTxPayload(tx, proTx)) {
            mapProTxRefs.emplace(proTx.proTxHash, tx.GetHash());
            mapProTxBlsPubKeyHashes.emplace(proTx.pubKeyOperator.GetHash(), tx.GetHash());
            auto dmn = deterministicMNManager->GetListAtChainTip().GetMN(proTx.proTxHash);
            if(dmn) {
                newit->validForProTxKey = ::SerializeHash(dmn->pdmnState->pubKeyOperator);
                if (dmn->pdmnState->pubKeyOperator.Get() != proTx.pubKeyOperator) {
                    newit->isKeyChangeProTx = true;
                }
            }
        }
    } else if (tx.nVersion == SYSCOIN_TX_VERSION_MN_UPDATE_REVOKE) {
        CProUpRevTx proTx;
        if(GetTxPayload(tx, proTx)) {
            mapProTxRefs.emplace(proTx.proTxHash, tx.GetHash());
            auto dmn = deterministicMNManager->GetListAtChainTip().GetMN(proTx.proTxHash);
            if(dmn) {
                newit->validForProTxKey = ::SerializeHash(dmn->pdmnState->pubKeyOperator);
                if (dmn->pdmnState->pubKeyOperator.Get() != CBLSPublicKey()) {
                    newit->isKeyChangeProTx = true;
                }
            }
        }
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

    totalTxSize -= it->GetTxSize();
    m_total_fee -= it->GetFee();
    cachedInnerUsage -= it->DynamicMemoryUsage();
    // SYSCOIN deal with pro tx stuff first
    auto eraseProTxRef = [&](const uint256& proTxHash, const uint256& txHash) {
        LOCK2(cs_main, cs);
        auto its = mapProTxRefs.equal_range(proTxHash);
        for (auto it = its.first; it != its.second;) {
            if (it->second == txHash) {
                it = mapProTxRefs.erase(it);
            } else {
                ++it;
            }
        }
    };

    if (it->GetTx().nVersion == SYSCOIN_TX_VERSION_MN_REGISTER) {
        CProRegTx proTx;
        if (GetTxPayload(it->GetTx(), proTx)) {
            if (!proTx.collateralOutpoint.IsNull()) {
                eraseProTxRef(it->GetTx().GetHash(), proTx.collateralOutpoint.hash);
            }
            mapProTxAddresses.erase(proTx.addr);
            mapProTxPubKeyIDs.erase(proTx.keyIDOwner);
            mapProTxBlsPubKeyHashes.erase(proTx.pubKeyOperator.GetHash());
            mapProTxCollaterals.erase(proTx.collateralOutpoint);
            mapProTxCollaterals.erase(COutPoint(it->GetTx().GetHash(), proTx.collateralOutpoint.n));
        }
    } else if (it->GetTx().nVersion == SYSCOIN_TX_VERSION_MN_UPDATE_SERVICE) {
        CProUpServTx proTx;
        if (GetTxPayload(it->GetTx(), proTx)) {
            eraseProTxRef(proTx.proTxHash, it->GetTx().GetHash());
            mapProTxAddresses.erase(proTx.addr);
        }
    } else if (it->GetTx().nVersion == SYSCOIN_TX_VERSION_MN_UPDATE_REGISTRAR) {
        CProUpRegTx proTx;
        if (GetTxPayload(it->GetTx(), proTx)) { 
            eraseProTxRef(proTx.proTxHash, it->GetTx().GetHash());
            mapProTxBlsPubKeyHashes.erase(proTx.pubKeyOperator.GetHash());
        }
    } else if (it->GetTx().nVersion == SYSCOIN_TX_VERSION_MN_UPDATE_REVOKE) {
        CProUpRevTx proTx;
        if (GetTxPayload(it->GetTx(), proTx)) {
            eraseProTxRef(proTx.proTxHash, it->GetTx().GetHash());
        }
    }
    // remove nevm tx from mempool structure
    if(IsSyscoinMintTx(it->GetTx().nVersion)) {
        CMintSyscoin mintSyscoin(it->GetTx());
        if(!mintSyscoin.IsNull())
            mapMintKeysMempool.erase(mintSyscoin.nTxHash);
    }
    // completely remove data if not removing for block (if block we should keep data alive for some time)
    else if(it->GetTx().IsNEVMData() && reason != MemPoolRemovalReason::BLOCK) {
        CNEVMData nevmData(it->GetTx());
        if(!nevmData.IsNull()){
            EraseNEVMData({nevmData.vchVersionHash});
        }
    }
    cachedInnerUsage -= memusage::DynamicUsage(it->GetMemPoolParentsConst()) + memusage::DynamicUsage(it->GetMemPoolChildrenConst());
    mapTx.erase(it);
    nTransactionsUpdated++;
    if (minerPolicyEstimator) {minerPolicyEstimator->removeTx(hash, false);}
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
// SYSCOIN
bool CTxMemPool::existsConflicts(const CTransaction &tx) const
{
    AssertLockHeld(cs_main);
    AssertLockHeld(cs);
    for (const CTxIn &txin : tx.vin) {
        if(mapAssetAllocationConflicts.find(txin.prevout) != mapAssetAllocationConflicts.end())
            return true;
    }
    return false;
}

void CTxMemPool::removeConflicts(const CTransaction &tx)
{
    // Remove transactions which depend on inputs of tx, recursively
    AssertLockHeld(cs_main);
    AssertLockHeld(cs);
    for (const CTxIn &txin : tx.vin) {
        auto it = mapNextTx.find(txin.prevout);
        if (it != mapNextTx.end()) {
            const CTransaction &txConflict = *it->second;
            if (txConflict != tx)
            {
                if (txConflict.nVersion == SYSCOIN_TX_VERSION_MN_REGISTER) {
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

// SYSCOIN
void CTxMemPool::removeZDAGConflicts(const CTransaction &tx)
{
    // Remove conflicting zdag transactions which depend on inputs of tx, recursively
    AssertLockHeld(cs_main);
    AssertLockHeld(cs);
    for (const CTxIn &txin : tx.vin) {
        auto it = mapAssetAllocationConflicts.find(txin.prevout);
        // remove the two transactions linked to this prevout in event of a conflict
        if (it != mapAssetAllocationConflicts.end()) {
            if(it->second.first) {
                ClearPrioritisation(it->second.first->GetHash());
                removeRecursive(*it->second.first, MemPoolRemovalReason::CONFLICT);
            }
            if(it->second.second) {
                ClearPrioritisation(it->second.second->GetHash());
                removeRecursive(*it->second.second, MemPoolRemovalReason::CONFLICT);
            }
        } 
    }
}

// true if other tx (conflicting) was first in mempool and it was involved in asset double spend
bool CTxMemPool::isSyscoinConflictIsFirstSeen(const CTransaction &tx) const {
    AssertLockHeld(cs);
    if(mapAssetAllocationConflicts.empty())
        return true;
    for (const CTxIn &txin : tx.vin) {
        auto it = mapAssetAllocationConflicts.find(txin.prevout);
        // ensure that we check for mapAssetAllocationConflicts intersection of this input
        // the only time conflicts are allowed and would cause problems for zdag is when its double spent without RBF
        // we allow one double spend input to be propagated and here we ensure we are only dealing with skipping transactions based on time
        // if it is one of those transactions that propagated double spent input related to syscoin asset tx
        if (it != mapAssetAllocationConflicts.end()) {
            txiter thisit, conflictit;
            txiter firstit = mapTx.find(it->second.first->GetHash());
            thisit = mapTx.end();
            if(firstit != mapTx.end()){
                if(firstit->GetTx() == tx)
                    thisit = firstit;
            }
            txiter secondit = mapTx.find(it->second.second->GetHash());
            if(secondit != mapTx.end()){
                if(secondit->GetTx() == tx) {
                    thisit = secondit;
                    conflictit = firstit;
                } else {
                    conflictit = secondit;
                }
            }
            // if for some reason thisit is not found (it should be) we just return false
            if(thisit == mapTx.end()) {
                return false;
            }
            // if first tx found not second, true if first one is tx, otherwise false
            if (firstit != mapTx.end() && secondit == mapTx.end()) {
                return firstit == thisit;
            // if second tx found not first, true if second one is tx, otherwise false
            } else if (secondit != mapTx.end() && firstit == mapTx.end()) {
                return secondit == thisit;
            // if first tx and second tx are not in mempool
            } else if(firstit == mapTx.end() && secondit == mapTx.end())
                return false;


            // if transaction in question was signalling RBF but conflicting transaction was not
            // prefer the conflict version over this one as prescedence over time based ordering
            // if both signal RBF, just choose the first one in mempool based on time below (they wouldn't have been used for point-of-sale anyway due to RBF)
            const bool &thisRBF = SignalsOptInRBF(thisit->GetTx());
            const bool &otherRBF = SignalsOptInRBF(conflictit->GetTx());
            if(thisRBF && !otherRBF) {
                return false;
            // if this transaction is non-RBF but conflict signals it, prefer this one regardless of time order
            } else if(!thisRBF && otherRBF) {
                return true;
            }
            // if conflicting transaction was received before the transaction in question
            // idea is to mine the oldest transaction in event of conflict
            // upon block, the conflict is removed
            const auto& time1 = conflictit->GetTime();
            const auto& time2 = thisit->GetTime();
            if(time1 < time2){
                return false;
            } else if(time1 == time2) {
                return thisit->GetTx().GetHash() < conflictit->GetTx().GetHash();
            }
        }
    }
    return true;
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

void CTxMemPool::removeProTxPubKeyConflicts(const CTransaction &tx, const CBLSPublicKey &pubKey)
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
    AssertLockHeld(cs_main);
    AssertLockHeld(cs);
    // Remove TXs that refer to a MN for which the collateral was spent
    auto removeSpentCollateralConflict = [&](const uint256& proTxHash) {
        LOCK2(cs_main, cs);
        // Can't use equal_range here as every call to removeRecursive might invalidate iterators
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
    auto mnList = deterministicMNManager->GetListAtChainTip();
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
    AssertLockHeld(cs_main);
    AssertLockHeld(cs);
    removeProTxSpentCollateralConflicts(tx);

    if (tx.nVersion == SYSCOIN_TX_VERSION_MN_REGISTER) {
        CProRegTx proTx;
        if (!GetTxPayload(tx, proTx)) {
            LogPrint(BCLog::MEMPOOL, "%s: ERROR: Invalid transaction payload, tx: %s\n", __func__, tx.ToString());
            return;
        }

        if (mapProTxAddresses.count(proTx.addr)) {
            uint256 conflictHash = mapProTxAddresses[proTx.addr];
            if (conflictHash != tx.GetHash() && mapTx.count(conflictHash)) {
                removeRecursive(mapTx.find(conflictHash)->GetTx(), MemPoolRemovalReason::CONFLICT);
            }
        }
        removeProTxPubKeyConflicts(tx, proTx.keyIDOwner);
        removeProTxPubKeyConflicts(tx, proTx.pubKeyOperator);
        if (!proTx.collateralOutpoint.hash.IsNull()) {
            removeProTxCollateralConflicts(tx, proTx.collateralOutpoint);
        } else {
            removeProTxCollateralConflicts(tx, COutPoint(tx.GetHash(), proTx.collateralOutpoint.n));
        }
    } else if (tx.nVersion == SYSCOIN_TX_VERSION_MN_UPDATE_SERVICE) {
        CProUpServTx proTx;
        if (!GetTxPayload(tx, proTx)) {
            LogPrint(BCLog::MEMPOOL, "%s: ERROR: Invalid transaction payload, tx: %s\n", __func__, tx.ToString());
            return;
        }

        if (mapProTxAddresses.count(proTx.addr)) {
            uint256 conflictHash = mapProTxAddresses[proTx.addr];
            if (conflictHash != tx.GetHash() && mapTx.count(conflictHash)) {
                removeRecursive(mapTx.find(conflictHash)->GetTx(), MemPoolRemovalReason::CONFLICT);
            }
        }
    } else if (tx.nVersion == SYSCOIN_TX_VERSION_MN_UPDATE_REGISTRAR) {
        CProUpRegTx proTx;
        if (!GetTxPayload(tx, proTx)) {
            LogPrint(BCLog::MEMPOOL, "%s: ERROR: Invalid transaction payload, tx: %s\n", __func__, tx.ToString());
            return;
        }

        removeProTxPubKeyConflicts(tx, proTx.pubKeyOperator);
        removeProTxKeyChangedConflicts(tx, proTx.proTxHash, ::SerializeHash(proTx.pubKeyOperator));
    } else if (tx.nVersion == SYSCOIN_TX_VERSION_MN_UPDATE_REVOKE) {
        CProUpRevTx proTx;
        if (!GetTxPayload(tx, proTx)) {
            LogPrint(BCLog::MEMPOOL, "%s: ERROR: Invalid transaction payload, tx: %s\n", __func__, tx.ToString());
            return;
        }

        removeProTxKeyChangedConflicts(tx, proTx.proTxHash, ::SerializeHash(CBLSPublicKey()));
    }
}
/**
 * Called when a block is connected. Removes from mempool and updates the miner fee estimator.
 */
void CTxMemPool::removeForBlock(const std::vector<CTransactionRef>& vtx, unsigned int nBlockHeight)
{
    AssertLockHeld(cs_main);
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
        // SYSCOIN
        removeZDAGConflicts(*tx);
        removeProTxConflicts(*tx);
        ClearPrioritisation(tx->GetHash());
    }
    lastRollingFeeUpdate = GetTime();
    blockSinceLastRollingFeeBump = true;
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
        // SYSCOIN
        bool bFoundConflict = false;
        bool bAssetAllocationTX = IsAssetAllocationTx(tx.nVersion);
        for (const CTxIn &txin : tx.vin) {
            if(mapAssetAllocationConflicts.find(txin.prevout) != mapAssetAllocationConflicts.end()) {
                bFoundConflict = true;
                break;
            }
        }
        for (const CTxIn &txin : tx.vin) {
            // Check that every mempool transaction's inputs refer to available coins, or other mempool tx's.
            indexed_transaction_set::const_iterator it2 = mapTx.find(txin.prevout.hash);
            if (it2 != mapTx.end()) {
                const CTransaction& tx2 = it2->GetTx();
                assert(tx2.vout.size() > txin.prevout.n && !tx2.vout[txin.prevout.n].IsNull());
                setParentCheck.insert(*it2);
            }
            // SYSCOIN We are iterating through the mempool entries sorted in order by ancestor count.
            // All parents must have been checked before their children and their coins added to
            // the mempoolDuplicate coins cache.
            if(!bFoundConflict)
                assert(mempoolDuplicate.HaveCoin(txin.prevout));
            // Check whether its inputs are marked in mapNextTx.
            auto it3 = mapNextTx.find(txin.prevout);
            assert(it3 != mapNextTx.end());
            // SYSCOIN
            if(bFoundConflict) {
                assert(*it3->first == txin.prevout);
                auto itzdagconflict = mapAssetAllocationConflicts.find(txin.prevout);
                // does dbl-spend conflict exist, we don't have enough info to check tx otherwise if no conflict
                if(itzdagconflict != mapAssetAllocationConflicts.end()) {
                    // the tx must be one of the dbl-spend conflicts
                    assert((itzdagconflict->second.first && *itzdagconflict->second.first == tx) || (itzdagconflict->second.second && *itzdagconflict->second.second == tx));
                }
            } else {
                assert(it3->first == &txin.prevout);
                assert(*it3->second == tx);          
            }
        }
        auto comp = [](const CTxMemPoolEntry& a, const CTxMemPoolEntry& b) -> bool {
            return a.GetTx().GetHash() == b.GetTx().GetHash();
        };
        assert(setParentCheck.size() == it->GetMemPoolParentsConst().size());
        assert(std::equal(setParentCheck.begin(), setParentCheck.end(), it->GetMemPoolParentsConst().begin(), comp));
        // Verify ancestor state is correct.
        auto ancestors{AssumeCalculateMemPoolAncestors(__func__, *it, Limits::NoLimits())};
        uint64_t nCountCheck = ancestors.size() + 1;
        uint64_t nSizeCheck = it->GetTxSize();
        CAmount nFeesCheck = it->GetModifiedFee();
        int64_t nSigOpCheck = it->GetSigOpCost();

        for (txiter ancestorIt : ancestors) {
            nSizeCheck += ancestorIt->GetTxSize();
            nFeesCheck += ancestorIt->GetModifiedFee();
            nSigOpCheck += ancestorIt->GetSigOpCost();
        }

        assert(it->GetCountWithAncestors() == nCountCheck);
        assert(it->GetSizeWithAncestors() == nSizeCheck);
        assert(it->GetSigOpCostWithAncestors() == nSigOpCheck);
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
        if(!bAssetAllocationTX) {
            assert(setChildrenCheck.size() == it->GetMemPoolChildrenConst().size());
            assert(std::equal(setChildrenCheck.begin(), setChildrenCheck.end(), it->GetMemPoolChildrenConst().begin(), comp));
        }
            
        // Also check to make sure size is greater than sum with immediate children.
        // just a sanity check, not definitive that this calc is correct...
        assert(it->GetSizeWithDescendants() >= child_sizes + it->GetTxSize());

        TxValidationState dummy_state; // Not used. CheckTxInputs() should always pass
        CAmount txfee = 0;
        assert(!tx.IsCoinBase());
        // SYSCOIN
        CAssetsMap mapAssetIn, mapAssetOut;
        if(!bFoundConflict)
            assert(Consensus::CheckTxInputs(tx, dummy_state, mempoolDuplicate, spendheight, txfee, mapAssetIn, mapAssetOut));
        // SYSCOIN
        for (const auto& input: tx.vin) if(mempoolDuplicate.HaveCoin(input.prevout)) mempoolDuplicate.SpendCoin(input.prevout);
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

bool CTxMemPool::CompareDepthAndScore(const uint256& hasha, const uint256& hashb, bool wtxid)
{
    LOCK(cs);
    indexed_transaction_set::const_iterator i = wtxid ? get_iter_from_wtxid(hasha) : mapTx.find(hasha);
    if (i == mapTx.end()) return false;
    indexed_transaction_set::const_iterator j = wtxid ? get_iter_from_wtxid(hashb) : mapTx.find(hashb);
    if (j == mapTx.end()) return true;
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

TxMempoolInfo CTxMemPool::info(const GenTxid& gtxid) const
{
    LOCK(cs);
    indexed_transaction_set::const_iterator i = (gtxid.IsWtxid() ? get_iter_from_wtxid(gtxid.GetHash()) : mapTx.find(gtxid.GetHash()));
    if (i == mapTx.end())
        return TxMempoolInfo();
    return GetInfo(i);
}
// SYSCOIN

bool CTxMemPool::existsProviderTxConflict(const CTransaction &tx) const {
    AssertLockHeld(cs_main);
    AssertLockHeld(cs);

    auto hasKeyChangeInMempool = [&](const uint256& proTxHash) {
        LOCK2(cs_main, cs);
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

    if (tx.nVersion == SYSCOIN_TX_VERSION_MN_REGISTER) {
        CProRegTx proTx;
        if (!GetTxPayload(tx, proTx)) {
            LogPrint(BCLog::MEMPOOL, "%s: ERROR: Invalid transaction payload, tx: %s\n", __func__, tx.ToString());
            return true; // i.e. can't decode payload == conflict
        }
        if (mapProTxAddresses.count(proTx.addr) || mapProTxPubKeyIDs.count(proTx.keyIDOwner) || mapProTxBlsPubKeyHashes.count(proTx.pubKeyOperator.GetHash()))
            return true;
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
    } else if (tx.nVersion == SYSCOIN_TX_VERSION_MN_UPDATE_SERVICE) {
        CProUpServTx proTx;
        if (!GetTxPayload(tx, proTx)) {
            LogPrint(BCLog::MEMPOOL, "%s: ERROR: Invalid transaction payload, tx: %s\n", __func__, tx.ToString());
            return true; // i.e. can't decode payload == conflict
        }
        auto it = mapProTxAddresses.find(proTx.addr);
        return it != mapProTxAddresses.end() && it->second != proTx.proTxHash;
    } else if (tx.nVersion == SYSCOIN_TX_VERSION_MN_UPDATE_REGISTRAR) {
        CProUpRegTx proTx;
        if (!GetTxPayload(tx, proTx)) {
            LogPrint(BCLog::MEMPOOL, "%s: ERROR: Invalid transaction payload, tx: %s\n", __func__, tx.ToString());
            return true; // i.e. can't decode payload == conflict
        }

        // this method should only be called with validated ProTxs
        auto dmn = deterministicMNManager->GetListAtChainTip().GetMN(proTx.proTxHash);
        if (!dmn) {
            LogPrint(BCLog::MEMPOOL, "%s: ERROR: Masternode is not in the list, proTxHash: %s\n", __func__, proTx.proTxHash.ToString());
            return true; // i.e. failed to find validated ProTx == conflict
        }
        // only allow one operator key change in the mempool
        if (dmn->pdmnState->pubKeyOperator.Get() != proTx.pubKeyOperator) {
            if (hasKeyChangeInMempool(proTx.proTxHash)) {
                return true;
            }
        }

        auto it = mapProTxBlsPubKeyHashes.find(proTx.pubKeyOperator.GetHash());
        return it != mapProTxBlsPubKeyHashes.end() && it->second != proTx.proTxHash;
    } else if (tx.nVersion == SYSCOIN_TX_VERSION_MN_UPDATE_REVOKE) {
        CProUpRevTx proTx;
        if (!GetTxPayload(tx, proTx)) {
            LogPrint(BCLog::MEMPOOL, "%s: ERROR: Invalid transaction payload, tx: %s\n", __func__, tx.ToString());
            return true; // i.e. can't decode payload == conflict
        }

        // this method should only be called with validated ProTxs
        auto dmn = deterministicMNManager->GetListAtChainTip().GetMN(proTx.proTxHash);
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
            auto ancestors{AssumeCalculateMemPoolAncestors(__func__, *it, Limits::NoLimits(), /*fSearchForParents=*/false)};
            for (txiter ancestorIt : ancestors) {
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
    LogPrintf("PrioritiseTransaction: %s fee += %s\n", hash.ToString(), FormatMoney(nFeeDelta));
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
    // Estimate the overhead of mapTx to be 15 pointers + an allocation, as no exact formula for boost::multi_index_contained is implemented.
    return memusage::MallocUsage(sizeof(CTxMemPoolEntry) + 15 * sizeof(void*)) * mapTx.size() + memusage::DynamicUsage(mapNextTx) + memusage::DynamicUsage(mapDeltas) + memusage::DynamicUsage(vTxHashes) + cachedInnerUsage;
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

void CTxMemPool::addUnchecked(const CTxMemPoolEntry &entry, bool validFeeEstimate)
{
    auto ancestors{AssumeCalculateMemPoolAncestors(__func__, entry, Limits::NoLimits())};
    return addUnchecked(entry, ancestors, validFeeEstimate);
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

        if (rollingMinimumFeeRate < (double)m_incremental_relay_feerate.GetFeePerK() / 2) {
            rollingMinimumFeeRate = 0;
            return CFeeRate(0);
        }
    }
    return std::max(CFeeRate(llround(rollingMinimumFeeRate)), m_incremental_relay_feerate);
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
        removed += m_incremental_relay_feerate;
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


std::string RemovalReasonToString(const MemPoolRemovalReason& r) noexcept
{
    switch (r) {
        case MemPoolRemovalReason::EXPIRY: return "expiry";
        case MemPoolRemovalReason::SIZELIMIT: return "sizelimit";
        case MemPoolRemovalReason::REORG: return "reorg";
        case MemPoolRemovalReason::BLOCK: return "block";
        case MemPoolRemovalReason::CONFLICT: return "conflict";
        case MemPoolRemovalReason::REPLACED: return "replaced";
    }
    assert(false);
}
