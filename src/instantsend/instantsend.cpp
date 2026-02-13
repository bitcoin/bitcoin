// Copyright (c) 2019-2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <instantsend/instantsend.h>

#include <chainlock/chainlock.h>
#include <chainparams.h>
#include <consensus/validation.h>
#include <instantsend/signing.h>
#include <masternode/sync.h>
#include <node/blockstorage.h>
#include <spork.h>
#include <stats/client.h>
#include <txmempool.h>
#include <validation.h>
#include <validationinterface.h>

// Forward declaration to break dependency over node/transaction.h
namespace node {
CTransactionRef GetTransaction(const CBlockIndex* const block_index, const CTxMemPool* const mempool,
                               const uint256& hash, const Consensus::Params& consensusParams, uint256& hashBlock);
} // namespace node

using node::fImporting;
using node::fReindex;
using node::GetTransaction;

namespace llmq {
namespace {
template <typename T>
    requires std::same_as<T, CTxIn> || std::same_as<T, COutPoint>
Uint256HashSet GetIdsFromLockable(const std::vector<T>& vec)
{
    Uint256HashSet ret{};
    if (vec.empty()) return ret;
    ret.reserve(vec.size());
    for (const auto& in : vec) {
        if constexpr (std::is_same_v<T, COutPoint>) {
            ret.emplace(instantsend::GenInputLockRequestId(in));
        } else if constexpr (std::is_same_v<T, CTxIn>) {
            ret.emplace(instantsend::GenInputLockRequestId(in.prevout));
        } else {
            assert(false);
        }
    }
    return ret;
}
} // anonymous namespace

CInstantSendManager::CInstantSendManager(const chainlock::Chainlocks& chainlocks, CChainState& chainstate,
                                         CSigningManager& _sigman, CSporkManager& sporkman, CTxMemPool& _mempool,
                                         const CMasternodeSync& mn_sync, const util::DbWrapperParams& db_params) :
    db{db_params},
    m_chainlocks{chainlocks},
    m_chainstate{chainstate},
    sigman{_sigman},
    spork_manager{sporkman},
    mempool{_mempool},
    m_mn_sync{mn_sync}
{
}

CInstantSendManager::~CInstantSendManager() = default;

bool ShouldReportISLockTiming() { return g_stats_client->active() || LogAcceptDebug(BCLog::INSTANTSEND); }

void CInstantSendManager::EnqueueInstantSendLock(NodeId from, const uint256& hash,
                                                 std::shared_ptr<instantsend::InstantSendLock> islock)
{
    if (ShouldReportISLockTiming()) {
        auto time_diff = [&]() -> int64_t {
            LOCK(cs_timingsTxSeen);
            if (auto it = timingsTxSeen.find(islock->txid); it != timingsTxSeen.end()) {
                // This is the normal case where we received the TX before the islock
                auto diff = TicksSinceEpoch<std::chrono::milliseconds>(SystemClock::now()) - it->second;
                timingsTxSeen.erase(it);
                return diff;
            }
            // But if we received the islock and don't know when we got the tx, then say 0, to indicate we received the islock first.
            return 0;
        }();
        ::g_stats_client->timing("islock_ms", time_diff);
        LogPrint(BCLog::INSTANTSEND, "CInstantSendManager::%s -- txid=%s, islock took %dms\n", __func__,
                 islock->txid.ToString(), time_diff);
    }

    LOCK(cs_pendingLocks);
    pendingInstantSendLocks.emplace(hash, instantsend::PendingISLockFromPeer{from, std::move(islock)});
}

instantsend::PendingState CInstantSendManager::FetchPendingLocks()
{
    instantsend::PendingState ret;

    LOCK(cs_pendingLocks);
    // only process a max 32 locks at a time to avoid duplicate verification of recovered signatures which have been
    // verified by CSigningManager in parallel
    const size_t maxCount = 32;
    // The keys of the removed values are temporaily stored here to avoid invalidating an iterator
    std::vector<uint256> removed;
    removed.reserve(std::min(maxCount, pendingInstantSendLocks.size()));

    for (auto& [islockHash, pending] : pendingInstantSendLocks) {
        // Check if we've reached max count
        if (ret.m_pending_is.size() >= maxCount) {
            ret.m_pending_work = true;
            break;
        }
        ret.m_pending_is.push_back(instantsend::PendingISLockEntry{std::move(pending), islockHash});
        removed.emplace_back(islockHash);
    }

    for (const auto& islockHash : removed) {
        pendingInstantSendLocks.erase(islockHash);
    }

    return ret;
}

std::variant<uint256, CTransactionRef, std::monostate> CInstantSendManager::ProcessInstantSendLock(
    NodeId from, const uint256& hash, const instantsend::InstantSendLockPtr& islock)
{
    LogPrint(BCLog::INSTANTSEND, "CInstantSendManager::%s -- txid=%s, islock=%s: processing islock, peer=%d\n",
             __func__, islock->txid.ToString(), hash.ToString(), from);

    if (auto signer = m_signer.load(std::memory_order_acquire); signer) {
        signer->ClearLockFromQueue(islock);
    }
    if (db.KnownInstantSendLock(hash)) {
        return std::monostate{};
    }

    if (const auto sameTxIsLock = db.GetInstantSendLockByTxid(islock->txid)) {
        // can happen, nothing to do
        return std::monostate{};
    }
    for (const auto& in : islock->inputs) {
        const auto sameOutpointIsLock = db.GetInstantSendLockByInput(in);
        if (sameOutpointIsLock != nullptr) {
            LogPrintf("CInstantSendManager::%s -- txid=%s, islock=%s: conflicting outpoint in islock. input=%s, other islock=%s, peer=%d\n", __func__,
                      islock->txid.ToString(), hash.ToString(), in.ToStringShort(), ::SerializeHash(*sameOutpointIsLock).ToString(), from);
        }
    }

    uint256 hashBlock{};
    auto tx = GetTransaction(nullptr, &mempool, islock->txid, Params().GetConsensus(), hashBlock);
    const bool found_transaction{tx != nullptr};
    // we ignore failure here as we must be able to propagate the lock even if we don't have the TX locally
    std::optional<int> minedHeight = GetBlockHeight(hashBlock);
    if (found_transaction) {
        if (!minedHeight.has_value()) {
            const CBlockIndex* pindexMined = WITH_LOCK(::cs_main, return m_chainstate.m_blockman.LookupBlockIndex(hashBlock));
            if (pindexMined != nullptr) {
                CacheBlockHeight(pindexMined);
                minedHeight = pindexMined->nHeight;
            }
        }
        // Let's see if the TX that was locked by this islock is already mined in a ChainLocked block. If yes,
        // we can simply ignore the islock, as the ChainLock implies locking of all TXs in that chain
        if (minedHeight.has_value() && m_chainlocks.HasChainLock(*minedHeight, hashBlock)) {
            LogPrint(BCLog::INSTANTSEND, /* Continued */
                     "CInstantSendManager::%s -- txlock=%s, islock=%s: dropping islock as it already got a "
                     "ChainLock in block %s, peer=%d\n",
                     __func__, islock->txid.ToString(), hash.ToString(), hashBlock.ToString(), from);
            return std::monostate{};
        }
    }

    if (found_transaction) {
        db.WriteNewInstantSendLock(hash, islock);
        if (minedHeight.has_value()) {
            db.WriteInstantSendLockMined(hash, *minedHeight);
        }
    } else {
        // put it in a separate pending map and try again later
        LOCK(cs_pendingLocks);
        pendingNoTxInstantSendLocks.try_emplace(hash, instantsend::PendingISLockFromPeer{from, islock});
    }

    // This will also add children TXs to pendingRetryTxs
    RemoveNonLockedTx(islock->txid, true);
    // We don't need the recovered sigs for the inputs anymore. This prevents unnecessary propagation of these sigs.
    // We only need the ISLOCK from now on to detect conflicts
    TruncateRecoveredSigsForInputs(*islock);
    ResolveBlockConflicts(hash, *islock);

    if (found_transaction) {
        RemoveMempoolConflictsForLock(hash, *islock);
        LogPrint(BCLog::INSTANTSEND, "CInstantSendManager::%s -- notify about lock %s for tx %s\n", __func__,
                 hash.ToString(), tx->GetHash().ToString());
        GetMainSignals().NotifyTransactionLock(tx, islock);
        // bump mempool counter to make sure newly locked txes are picked up by getblocktemplate
        mempool.AddTransactionsUpdated(1);
    }

    if (found_transaction) {
        return tx;
    }
    return islock->txid;
}

void CInstantSendManager::TransactionAddedToMempool(const CTransactionRef& tx)
{
    if (!IsInstantSendEnabled() || !m_mn_sync.IsBlockchainSynced() || tx->vin.empty()) {
        return;
    }

    instantsend::InstantSendLockPtr islock{nullptr};
    {
        LOCK(cs_pendingLocks);
        auto it = pendingNoTxInstantSendLocks.begin();
        while (it != pendingNoTxInstantSendLocks.end()) {
            if (it->second.islock->txid == tx->GetHash()) {
                // we received an islock earlier
                LogPrint(BCLog::INSTANTSEND, "CInstantSendManager::%s -- txid=%s, islock=%s\n", __func__,
                         tx->GetHash().ToString(), it->first.ToString());
                islock = it->second.islock;
                pendingInstantSendLocks.try_emplace(it->first, it->second);
                pendingNoTxInstantSendLocks.erase(it);
                break;
            }
            ++it;
        }
    }

    if (islock == nullptr) {
        if (auto signer = m_signer.load(std::memory_order_acquire); signer) {
            signer->ProcessTx(*tx, false, Params().GetConsensus());
        }
        // TX is not locked, so make sure it is tracked
        AddNonLockedTx(tx, nullptr);
    } else {
        RemoveMempoolConflictsForLock(::SerializeHash(*islock), *islock);
    }
}

void CInstantSendManager::TransactionRemovedFromMempool(const CTransactionRef& tx)
{
    if (tx->vin.empty()) {
        return;
    }

    instantsend::InstantSendLockPtr islock = db.GetInstantSendLockByTxid(tx->GetHash());

    if (islock == nullptr) {
        return;
    }

    LogPrint(BCLog::INSTANTSEND, "CInstantSendManager::%s -- transaction %s was removed from mempool\n", __func__,
             tx->GetHash().ToString());
    RemoveConflictingLock(::SerializeHash(*islock), *islock);
}

void CInstantSendManager::BlockConnected(const std::shared_ptr<const CBlock>& pblock, const CBlockIndex* pindex)
{
    if (!IsInstantSendEnabled()) {
        return;
    }

    CacheTipHeight(pindex);

    if (m_mn_sync.IsBlockchainSynced()) {
        const bool has_chainlock = m_chainlocks.HasChainLock(pindex->nHeight, pindex->GetBlockHash());
        for (const auto& tx : pblock->vtx) {
            if (tx->IsCoinBase() || tx->vin.empty()) {
                // coinbase and TXs with no inputs can't be locked
                continue;
            }

            if (!IsLocked(tx->GetHash()) && !has_chainlock) {
                if (auto signer = m_signer.load(std::memory_order_acquire); signer) {
                    signer->ProcessTx(*tx, true, Params().GetConsensus());
                }
                // TX is not locked, so make sure it is tracked
                AddNonLockedTx(tx, pindex);
            } else {
                // TX is locked, so make sure we don't track it anymore
                RemoveNonLockedTx(tx->GetHash(), true);
            }
        }
    }

    db.WriteBlockInstantSendLocks(pblock, pindex);
}

void CInstantSendManager::BlockDisconnected(const std::shared_ptr<const CBlock>& pblock,
                                            const CBlockIndex* pindexDisconnected)
{
    {
        LOCK(cs_height_cache);
        m_cached_block_heights.erase(pindexDisconnected->GetBlockHash());
    }

    CacheTipHeight(pindexDisconnected->pprev);

    db.RemoveBlockInstantSendLocks(pblock, pindexDisconnected);
}

void CInstantSendManager::AddNonLockedTx(const CTransactionRef& tx, const CBlockIndex* pindexMined)
{
    {
        LOCK(cs_nonLocked);
        auto [it, did_insert] = nonLockedTxs.emplace(tx->GetHash(), NonLockedTxInfo());
        auto& nonLockedTxInfo = it->second;
        nonLockedTxInfo.pindexMined = pindexMined;

        if (did_insert) {
            nonLockedTxInfo.tx = tx;
            for (const auto& in : tx->vin) {
                nonLockedTxs[in.prevout.hash].children.emplace(tx->GetHash());
                nonLockedTxsByOutpoints.emplace(in.prevout, tx->GetHash());
            }
        }
    }
    {
        LOCK(cs_pendingLocks);
        auto it = pendingNoTxInstantSendLocks.begin();
        while (it != pendingNoTxInstantSendLocks.end()) {
            if (it->second.islock->txid == tx->GetHash()) {
                // we received an islock earlier, let's put it back into pending and verify/lock
                LogPrint(BCLog::INSTANTSEND, "CInstantSendManager::%s -- txid=%s, islock=%s\n", __func__,
                         tx->GetHash().ToString(), it->first.ToString());
                pendingInstantSendLocks.try_emplace(it->first, it->second);
                pendingNoTxInstantSendLocks.erase(it);
                break;
            }
            ++it;
        }
    }

    if (ShouldReportISLockTiming()) {
        LOCK(cs_timingsTxSeen);
        // Only insert the time the first time we see the tx, as we sometimes try to resign
        timingsTxSeen.try_emplace(tx->GetHash(), TicksSinceEpoch<std::chrono::milliseconds>(SystemClock::now()));
    }

    LogPrint(BCLog::INSTANTSEND, "CInstantSendManager::%s -- txid=%s, pindexMined=%s\n", __func__,
             tx->GetHash().ToString(), pindexMined ? pindexMined->GetBlockHash().ToString() : "");
}

void CInstantSendManager::RemoveNonLockedTx(const uint256& txid, bool retryChildren)
{
    LOCK(cs_nonLocked);

    auto it = nonLockedTxs.find(txid);
    if (it == nonLockedTxs.end()) {
        return;
    }
    const auto& info = it->second;

    size_t retryChildrenCount = 0;
    if (retryChildren) {
        // TX got locked, so we can retry locking children
        LOCK(cs_pendingRetry);
        for (const auto& childTxid : info.children) {
            pendingRetryTxs.emplace(childTxid);
            retryChildrenCount++;
        }
    }
    // don't try to lock it anymore
    WITH_LOCK(cs_pendingRetry, pendingRetryTxs.erase(txid));

    if (info.tx) {
        for (const auto& in : info.tx->vin) {
            if (auto jt = nonLockedTxs.find(in.prevout.hash); jt != nonLockedTxs.end()) {
                jt->second.children.erase(txid);
                if (!jt->second.tx && jt->second.children.empty()) {
                    nonLockedTxs.erase(jt);
                }
            }
            nonLockedTxsByOutpoints.erase(in.prevout);
        }
    }

    nonLockedTxs.erase(it);

    LogPrint(BCLog::INSTANTSEND, "CInstantSendManager::%s -- txid=%s, retryChildren=%d, retryChildrenCount=%d\n",
             __func__, txid.ToString(), retryChildren, retryChildrenCount);
}

std::vector<CTransactionRef> CInstantSendManager::PrepareTxToRetry()
{
    std::vector<CTransactionRef> txns{};

    LOCK2(cs_nonLocked, cs_pendingRetry);
    if (pendingRetryTxs.empty()) return txns;
    txns.reserve(pendingRetryTxs.size());
    for (const auto& txid : pendingRetryTxs) {
        if (auto it = nonLockedTxs.find(txid); it != nonLockedTxs.end()) {
            const auto& [_, tx_info] = *it;
            if (tx_info.tx) {
                txns.push_back(tx_info.tx);
            }
        }
    }
    return txns;
}

void CInstantSendManager::RemoveConflictedTx(const CTransaction& tx)
{
    RemoveNonLockedTx(tx.GetHash(), false);
    if (auto signer = m_signer.load(std::memory_order_acquire); signer) {
        signer->ClearInputsFromQueue(GetIdsFromLockable(tx.vin));
    }
}

void CInstantSendManager::TruncateRecoveredSigsForInputs(const instantsend::InstantSendLock& islock)
{
    auto ids = GetIdsFromLockable(islock.inputs);
    if (auto signer = m_signer.load(std::memory_order_acquire); signer) {
        signer->ClearInputsFromQueue(ids);
    }
    for (const auto& id : ids) {
        sigman.TruncateRecoveredSig(Params().GetConsensus().llmqTypeDIP0024InstantSend, id);
    }
}

void CInstantSendManager::TryEmplacePendingLock(const uint256& hash, const NodeId id,
                                                const instantsend::InstantSendLockPtr& islock)
{
    if (db.KnownInstantSendLock(hash)) return;
    LOCK(cs_pendingLocks);
    if (!pendingInstantSendLocks.count(hash)) {
        pendingInstantSendLocks.emplace(hash, instantsend::PendingISLockFromPeer{id, islock});
    }
}

void CInstantSendManager::NotifyChainLock(const CBlockIndex* pindexChainLock)
{
    HandleFullyConfirmedBlock(pindexChainLock);
}

void CInstantSendManager::UpdatedBlockTip(const CBlockIndex* pindexNew)
{
    CacheTipHeight(pindexNew);

    bool fDIP0008Active = pindexNew->pprev && pindexNew->pprev->nHeight >= Params().GetConsensus().DIP0008Height;

    if (m_chainlocks.IsEnabled() && fDIP0008Active) {
        // Nothing to do here. We should keep all islocks and let chainlocks handle them.
        return;
    }

    int nConfirmedHeight = pindexNew->nHeight - Params().GetConsensus().nInstantSendKeepLock;
    const CBlockIndex* pindex = pindexNew->GetAncestor(nConfirmedHeight);

    if (pindex) {
        HandleFullyConfirmedBlock(pindex);
    }
}

void CInstantSendManager::HandleFullyConfirmedBlock(const CBlockIndex* pindex)
{
    if (!IsInstantSendEnabled()) {
        return;
    }

    auto removeISLocks = db.RemoveConfirmedInstantSendLocks(pindex->nHeight);

    for (const auto& [islockHash, islock] : removeISLocks) {
        LogPrint(BCLog::INSTANTSEND, "CInstantSendManager::%s -- txid=%s, islock=%s: removed islock as it got fully confirmed\n", __func__,
                 islock->txid.ToString(), islockHash.ToString());

        // No need to keep recovered sigs for fully confirmed IS locks, as there is no chance for conflicts
        // from now on. All inputs are spent now and can't be spend in any other TX.
        TruncateRecoveredSigsForInputs(*islock);

        // And we don't need the recovered sig for the ISLOCK anymore, as the block in which it got mined is considered
        // fully confirmed now
        sigman.TruncateRecoveredSig(Params().GetConsensus().llmqTypeDIP0024InstantSend, islock->GetRequestId());
    }

    db.RemoveArchivedInstantSendLocks(pindex->nHeight - 100);

    // Find all previously unlocked TXs that got locked by this fully confirmed (ChainLock) block and remove them
    // from the nonLockedTxs map. Also collect all children of these TXs and mark them for retrying of IS locking.
    std::vector<uint256> toRemove;
    {
        LOCK(cs_nonLocked);
        for (const auto& p : nonLockedTxs) {
            const auto* pindexMined = p.second.pindexMined;

            if (pindexMined && pindex->GetAncestor(pindexMined->nHeight) == pindexMined) {
                toRemove.emplace_back(p.first);
            }
        }
    }
    for (const auto& txid : toRemove) {
        // This will also add children to pendingRetryTxs
        RemoveNonLockedTx(txid, true);
    }
}

void CInstantSendManager::RemoveMempoolConflictsForLock(const uint256& hash, const instantsend::InstantSendLock& islock)
{
    Uint256HashMap<CTransactionRef> toDelete;

    {
        LOCK(mempool.cs);

        for (const auto& in : islock.inputs) {
            auto it = mempool.mapNextTx.find(in);
            if (it == mempool.mapNextTx.end()) {
                continue;
            }
            if (it->second->GetHash() != islock.txid) {
                toDelete.emplace(it->second->GetHash(), mempool.get(it->second->GetHash()));

                LogPrintf("CInstantSendManager::%s -- txid=%s, islock=%s: mempool TX %s with input %s conflicts with islock\n", __func__,
                          islock.txid.ToString(), hash.ToString(), it->second->GetHash().ToString(), in.ToStringShort());
            }
        }

        for (const auto& p : toDelete) {
            mempool.removeRecursive(*p.second, MemPoolRemovalReason::CONFLICT);
        }
    }

    for (const auto& p : toDelete) {
        RemoveConflictedTx(*p.second);
    }
}

void CInstantSendManager::ResolveBlockConflicts(const uint256& islockHash, const instantsend::InstantSendLock& islock)
{
    // Lets first collect all non-locked TXs which conflict with the given ISLOCK
    std::unordered_map<const CBlockIndex*, Uint256HashMap<CTransactionRef>> conflicts;
    {
        LOCK(cs_nonLocked);
        for (const auto& in : islock.inputs) {
            auto it = nonLockedTxsByOutpoints.find(in);
            if (it != nonLockedTxsByOutpoints.end()) {
                auto& conflictTxid = it->second;
                if (conflictTxid == islock.txid) {
                    continue;
                }
                auto jt = nonLockedTxs.find(conflictTxid);
                if (jt == nonLockedTxs.end()) {
                    continue;
                }
                auto& info = jt->second;
                if (!info.pindexMined || !info.tx) {
                    continue;
                }
                LogPrintf("CInstantSendManager::%s -- txid=%s, islock=%s: mined TX %s with input %s and mined in block %s conflicts with islock\n", __func__,
                          islock.txid.ToString(), islockHash.ToString(), conflictTxid.ToString(), in.ToStringShort(), info.pindexMined->GetBlockHash().ToString());
                conflicts[info.pindexMined].emplace(conflictTxid, info.tx);
            }
        }
    }

    // Lets see if any of the conflicts was already mined into a ChainLocked block
    bool hasChainLockedConflict = false;
    for (const auto& p : conflicts) {
        const auto* pindex = p.first;
        if (m_chainlocks.HasChainLock(pindex->nHeight, pindex->GetBlockHash())) {
            hasChainLockedConflict = true;
            break;
        }
    }

    // If a conflict was mined into a ChainLocked block, then we have no other choice and must prune the ISLOCK and all
    // chained ISLOCKs that build on top of this one. The probability of this is practically zero and can only happen
    // when large parts of the masternode network are controlled by an attacker. In this case we must still find
    // consensus and its better to sacrifice individual ISLOCKs then to sacrifice whole ChainLocks.
    if (hasChainLockedConflict) {
        LogPrintf("CInstantSendManager::%s -- txid=%s, islock=%s: at least one conflicted TX already got a ChainLock\n",
                  __func__, islock.txid.ToString(), islockHash.ToString());
        RemoveConflictingLock(islockHash, islock);
        return;
    }

    bool isLockedTxKnown = WITH_LOCK(cs_pendingLocks, return pendingNoTxInstantSendLocks.find(islockHash) ==
                                                             pendingNoTxInstantSendLocks.end());

    bool activateBestChain = false;
    for (const auto& p : conflicts) {
        const auto* pindex = p.first;
        for (const auto& p2 : p.second) {
            const auto& tx = *p2.second;
            RemoveConflictedTx(tx);
        }

        LogPrintf("CInstantSendManager::%s -- invalidating block %s\n", __func__, pindex->GetBlockHash().ToString());

        BlockValidationState state;
        // need non-const pointer
        auto pindex2 = WITH_LOCK(::cs_main, return m_chainstate.m_blockman.LookupBlockIndex(pindex->GetBlockHash()));
        if (!m_chainstate.InvalidateBlock(state, pindex2)) {
            LogPrintf("CInstantSendManager::%s -- InvalidateBlock failed: %s\n", __func__, state.ToString());
            // This should not have happened and we are in a state were it's not safe to continue anymore
            assert(false);
        }
        if (isLockedTxKnown) {
            activateBestChain = true;
        } else {
            LogPrintf("CInstantSendManager::%s -- resetting block %s\n", __func__, pindex2->GetBlockHash().ToString());
            LOCK(::cs_main);
            m_chainstate.ResetBlockFailureFlags(pindex2);
        }
    }

    if (activateBestChain) {
        BlockValidationState state;
        if (!m_chainstate.ActivateBestChain(state)) {
            LogPrintf("CInstantSendManager::%s -- ActivateBestChain failed: %s\n", __func__, state.ToString());
            // This should not have happened and we are in a state were it's not safe to continue anymore
            assert(false);
        }
    }
}

void CInstantSendManager::RemoveConflictingLock(const uint256& islockHash, const instantsend::InstantSendLock& islock)
{
    LogPrintf("CInstantSendManager::%s -- txid=%s, islock=%s: Removing ISLOCK and its chained children\n", __func__,
              islock.txid.ToString(), islockHash.ToString());
    const int tipHeight = GetTipHeight();

    auto removedIslocks = db.RemoveChainedInstantSendLocks(islockHash, islock.txid, tipHeight);
    for (const auto& h : removedIslocks) {
        LogPrintf("CInstantSendManager::%s -- txid=%s, islock=%s: removed (child) ISLOCK %s\n", __func__,
                  islock.txid.ToString(), islockHash.ToString(), h.ToString());
    }
}

bool CInstantSendManager::AlreadyHave(const CInv& inv) const
{
    if (!IsInstantSendEnabled()) {
        return true;
    }

    return WITH_LOCK(cs_pendingLocks, return pendingInstantSendLocks.count(inv.hash) != 0 ||
                                             pendingNoTxInstantSendLocks.count(inv.hash) != 0) ||
           db.KnownInstantSendLock(inv.hash);
}

bool CInstantSendManager::GetInstantSendLockByHash(const uint256& hash, instantsend::InstantSendLock& ret) const
{
    if (!IsInstantSendEnabled()) {
        return false;
    }

    auto islock = db.GetInstantSendLockByHash(hash);
    if (!islock) {
        LOCK(cs_pendingLocks);
        auto it = pendingInstantSendLocks.find(hash);
        if (it != pendingInstantSendLocks.end()) {
            islock = it->second.islock;
        } else {
            auto itNoTx = pendingNoTxInstantSendLocks.find(hash);
            if (itNoTx != pendingNoTxInstantSendLocks.end()) {
                islock = itNoTx->second.islock;
            } else {
                return false;
            }
        }
    }
    ret = *islock;
    return true;
}

instantsend::InstantSendLockPtr CInstantSendManager::GetInstantSendLockByTxid(const uint256& txid) const
{
    if (!IsInstantSendEnabled()) {
        return nullptr;
    }

    return db.GetInstantSendLockByTxid(txid);
}

bool CInstantSendManager::IsLocked(const uint256& txHash) const
{
    if (!IsInstantSendEnabled()) {
        return false;
    }

    return db.KnownInstantSendLock(db.GetInstantSendLockHashByTxid(txHash));
}

bool CInstantSendManager::IsWaitingForTx(const uint256& txHash) const
{
    if (!IsInstantSendEnabled()) {
        return false;
    }

    LOCK(cs_pendingLocks);
    auto it = pendingNoTxInstantSendLocks.begin();
    while (it != pendingNoTxInstantSendLocks.end()) {
        if (it->second.islock->txid == txHash) {
            LogPrint(BCLog::INSTANTSEND, "CInstantSendManager::%s -- txid=%s, islock=%s\n", __func__, txHash.ToString(),
                     it->first.ToString());
            return true;
        }
        ++it;
    }
    return false;
}

instantsend::InstantSendLockPtr CInstantSendManager::GetConflictingLock(const CTransaction& tx) const
{
    if (!IsInstantSendEnabled()) {
        return nullptr;
    }

    for (const auto& in : tx.vin) {
        auto otherIsLock = db.GetInstantSendLockByInput(in.prevout);
        if (!otherIsLock) {
            continue;
        }

        if (otherIsLock->txid != tx.GetHash()) {
            return otherIsLock;
        }
    }
    return nullptr;
}

size_t CInstantSendManager::GetInstantSendLockCount() const
{
    return db.GetInstantSendLockCount();
}

void CInstantSendManager::CacheBlockHeightInternal(const CBlockIndex* const block_index) const
{
    AssertLockHeld(cs_height_cache);
    m_cached_block_heights.insert(block_index->GetBlockHash(), block_index->nHeight);
}

void CInstantSendManager::CacheBlockHeight(const CBlockIndex* const block_index) const
{
    LOCK(cs_height_cache);
    CacheBlockHeightInternal(block_index);
}

std::optional<int> CInstantSendManager::GetBlockHeight(const uint256& hash) const
{
    if (hash.IsNull()) {
        return std::nullopt;
    }
    {
        LOCK(cs_height_cache);
        int cached_height{0};
        if (m_cached_block_heights.get(hash, cached_height)) return cached_height;
    }

    const CBlockIndex* pindex = WITH_LOCK(::cs_main, return m_chainstate.m_blockman.LookupBlockIndex(hash));
    if (pindex == nullptr) {
        return std::nullopt;
    }

    CacheBlockHeight(pindex);
    return pindex->nHeight;
}

void CInstantSendManager::CacheTipHeight(const CBlockIndex* const tip) const
{
    LOCK(cs_height_cache);
    if (tip) {
        CacheBlockHeightInternal(tip);
        m_cached_tip_height = tip->nHeight;
    } else {
        m_cached_tip_height = -1;
    }
}

int CInstantSendManager::GetTipHeight() const
{
    {
        LOCK(cs_height_cache);
        if (m_cached_tip_height >= 0) {
            return m_cached_tip_height;
        }
    }

    const CBlockIndex* tip = WITH_LOCK(::cs_main, return m_chainstate.m_chain.Tip());

    CacheTipHeight(tip);
    return tip ? tip->nHeight : -1;
}

bool CInstantSendManager::IsInstantSendEnabled() const
{
    return !fReindex && !fImporting && spork_manager.IsSporkActive(SPORK_2_INSTANTSEND_ENABLED);
}

bool CInstantSendManager::RejectConflictingBlocks() const
{
    if (!m_mn_sync.IsBlockchainSynced()) {
        return false;
    }
    if (!spork_manager.IsSporkActive(SPORK_3_INSTANTSEND_BLOCK_FILTERING)) {
        LogPrint(BCLog::INSTANTSEND, "%s: spork3 is off, skipping transaction locking checks\n", __func__);
        return false;
    }
    return true;
}
} // namespace llmq
