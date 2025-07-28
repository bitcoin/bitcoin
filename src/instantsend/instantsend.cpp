// Copyright (c) 2019-2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <instantsend/instantsend.h>

#include <chainparams.h>
#include <consensus/validation.h>
#include <net_processing.h>
#include <node/blockstorage.h>
#include <stats/client.h>
#include <txmempool.h>
#include <util/thread.h>
#include <validation.h>

#include <bls/bls_batchverifier.h>
#include <instantsend/signing.h>
#include <llmq/chainlocks.h>
#include <llmq/commitment.h>
#include <llmq/quorums.h>
#include <masternode/sync.h>
#include <spork.h>

#include <cxxtimer.hpp>

using node::fImporting;
using node::fReindex;

// Forward declaration to break dependency over node/transaction.h
namespace node
{
CTransactionRef GetTransaction(const CBlockIndex* const block_index, const CTxMemPool* const mempool,
                               const uint256& hash, const Consensus::Params& consensusParams, uint256& hashBlock);
} // namespace node
using node::GetTransaction;

namespace llmq {
static const std::string_view INPUTLOCK_REQUESTID_PREFIX = "inlock";

namespace {
template <typename T>
    requires std::same_as<T, CTxIn> || std::same_as<T, COutPoint>
std::unordered_set<uint256, StaticSaltedHasher> GetIdsFromLockable(const std::vector<T>& vec)
{
    std::unordered_set<uint256, StaticSaltedHasher> ret{};
    if (vec.empty()) return ret;
    ret.reserve(vec.size());
    for (const auto& in : vec) {
        ret.emplace(::SerializeHash(std::make_pair(INPUTLOCK_REQUESTID_PREFIX, in)));
    }
    return ret;
}
} // anonymous namespace

CInstantSendManager::CInstantSendManager(CChainLocksHandler& _clhandler, CChainState& chainstate, CQuorumManager& _qman,
                                         CSigningManager& _sigman, CSigSharesManager& _shareman,
                                         CSporkManager& sporkman, CTxMemPool& _mempool, const CMasternodeSync& mn_sync,
                                         bool is_masternode, bool unitTests, bool fWipe) :
    db{unitTests, fWipe},
    clhandler{_clhandler},
    m_chainstate{chainstate},
    qman{_qman},
    sigman{_sigman},
    spork_manager{sporkman},
    mempool{_mempool},
    m_mn_sync{mn_sync},
    m_signer{is_masternode ? std::make_unique<instantsend::InstantSendSigner>(chainstate, _clhandler, *this, _sigman,
                                                                              _shareman, _qman, sporkman, _mempool, mn_sync)
                           : nullptr}
{
    workInterrupt.reset();
}

CInstantSendManager::~CInstantSendManager() = default;

void CInstantSendManager::Start(PeerManager& peerman)
{
    // can't start new thread if we have one running already
    if (workThread.joinable()) {
        assert(false);
    }

    workThread = std::thread(&util::TraceThread, "isman", [this, &peerman] { WorkThreadMain(peerman); });

    if (m_signer) {
        m_signer->Start();
    }
}

void CInstantSendManager::Stop()
{
    if (m_signer) {
        m_signer->Stop();
    }

    // make sure to call InterruptWorkerThread() first
    if (!workInterrupt) {
        assert(false);
    }

    if (workThread.joinable()) {
        workThread.join();
    }
}

PeerMsgRet CInstantSendManager::ProcessMessage(const CNode& pfrom, PeerManager& peerman, std::string_view msg_type,
                                               CDataStream& vRecv)
{
    if (IsInstantSendEnabled() && msg_type == NetMsgType::ISDLOCK) {
        const auto islock = std::make_shared<instantsend::InstantSendLock>();
        vRecv >> *islock;
        return ProcessMessageInstantSendLock(pfrom, peerman, islock);
    }
    return {};
}

bool ShouldReportISLockTiming() {
    return g_stats_client->active() || LogAcceptDebug(BCLog::INSTANTSEND);
}

PeerMsgRet CInstantSendManager::ProcessMessageInstantSendLock(const CNode& pfrom, PeerManager& peerman,
                                                              const instantsend::InstantSendLockPtr& islock)
{
    auto hash = ::SerializeHash(*islock);

    WITH_LOCK(::cs_main, peerman.EraseObjectRequest(pfrom.GetId(), CInv(MSG_ISDLOCK, hash)));

    if (!islock->TriviallyValid()) {
        return tl::unexpected{100};
    }

    const auto blockIndex = WITH_LOCK(::cs_main, return m_chainstate.m_blockman.LookupBlockIndex(islock->cycleHash));
    if (blockIndex == nullptr) {
        // Maybe we don't have the block yet or maybe some peer spams invalid values for cycleHash
        return tl::unexpected{1};
    }

    // Deterministic islocks MUST use rotation based llmq
    auto llmqType = Params().GetConsensus().llmqTypeDIP0024InstantSend;
    const auto& llmq_params_opt = Params().GetLLMQ(llmqType);
    assert(llmq_params_opt);
    if (blockIndex->nHeight % llmq_params_opt->dkgInterval != 0) {
        return tl::unexpected{100};
    }

    if (WITH_LOCK(cs_pendingLocks, return pendingInstantSendLocks.count(hash) || pendingNoTxInstantSendLocks.count(hash))
            || db.KnownInstantSendLock(hash)) {
        return {};
    }

    LogPrint(BCLog::INSTANTSEND, "CInstantSendManager::%s -- txid=%s, islock=%s: received islock, peer=%d\n", __func__,
            islock->txid.ToString(), hash.ToString(), pfrom.GetId());

    if (ShouldReportISLockTiming()) {
        auto time_diff =  [&] () -> int64_t {
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
    pendingInstantSendLocks.emplace(hash, std::make_pair(pfrom.GetId(), islock));
    return {};
}

bool CInstantSendManager::ProcessPendingInstantSendLocks(PeerManager& peerman)
{
    decltype(pendingInstantSendLocks) pend;
    bool fMoreWork{false};

    if (!IsInstantSendEnabled()) {
        return false;
    }

    {
        LOCK(cs_pendingLocks);
        // only process a max 32 locks at a time to avoid duplicate verification of recovered signatures which have been
        // verified by CSigningManager in parallel
        const size_t maxCount = 32;
        // The keys of the removed values are temporaily stored here to avoid invalidating an iterator
        std::vector<uint256> removed;
        removed.reserve(maxCount);

        for (const auto& [islockHash, nodeid_islptr_pair] : pendingInstantSendLocks) {
            // Check if we've reached max count
            if (pend.size() >= maxCount) {
                fMoreWork = true;
                break;
            }
            pend.emplace(islockHash, std::move(nodeid_islptr_pair));
            removed.emplace_back(islockHash);
        }

        for (const auto& islockHash : removed) {
            pendingInstantSendLocks.erase(islockHash);
        }
    }

    if (pend.empty()) {
        return false;
    }

    //TODO Investigate if leaving this is ok
    auto llmqType = Params().GetConsensus().llmqTypeDIP0024InstantSend;
    const auto& llmq_params_opt = Params().GetLLMQ(llmqType);
    assert(llmq_params_opt);
    const auto& llmq_params = llmq_params_opt.value();
    auto dkgInterval = llmq_params.dkgInterval;

    // First check against the current active set and don't ban
    auto badISLocks = ProcessPendingInstantSendLocks(llmq_params, peerman, /*signOffset=*/0, pend, false);
    if (!badISLocks.empty()) {
        LogPrint(BCLog::INSTANTSEND, "CInstantSendManager::%s -- doing verification on old active set\n", __func__);

        // filter out valid IS locks from "pend"
        for (auto it = pend.begin(); it != pend.end(); ) {
            if (!badISLocks.count(it->first)) {
                it = pend.erase(it);
            } else {
                ++it;
            }
        }
        // Now check against the previous active set and perform banning if this fails
        ProcessPendingInstantSendLocks(llmq_params, peerman, dkgInterval, pend, true);
    }

    return fMoreWork;
}

std::unordered_set<uint256, StaticSaltedHasher> CInstantSendManager::ProcessPendingInstantSendLocks(
    const Consensus::LLMQParams& llmq_params, PeerManager& peerman, int signOffset,
    const std::unordered_map<uint256, std::pair<NodeId, instantsend::InstantSendLockPtr>, StaticSaltedHasher>& pend, bool ban)
{
    CBLSBatchVerifier<NodeId, uint256> batchVerifier(false, true, 8);
    std::unordered_map<uint256, CRecoveredSig, StaticSaltedHasher> recSigs;

    size_t verifyCount = 0;
    size_t alreadyVerified = 0;
    for (const auto& p : pend) {
        const auto& hash = p.first;
        auto nodeId = p.second.first;
        const auto& islock = p.second.second;

        if (batchVerifier.badSources.count(nodeId)) {
            continue;
        }

        if (!islock->sig.Get().IsValid()) {
            batchVerifier.badSources.emplace(nodeId);
            continue;
        }

        auto id = islock->GetRequestId();

        // no need to verify an ISLOCK if we already have verified the recovered sig that belongs to it
        if (sigman.HasRecoveredSig(llmq_params.type, id, islock->txid)) {
            alreadyVerified++;
            continue;
        }

        const auto blockIndex = WITH_LOCK(::cs_main, return m_chainstate.m_blockman.LookupBlockIndex(islock->cycleHash));
        if (blockIndex == nullptr) {
            batchVerifier.badSources.emplace(nodeId);
            continue;
        }

        int nSignHeight{-1};
        const auto dkgInterval = llmq_params.dkgInterval;
        if (blockIndex->nHeight + dkgInterval < m_chainstate.m_chain.Height()) {
            nSignHeight = blockIndex->nHeight + dkgInterval - 1;
        }
        // For RegTest non-rotating quorum cycleHash has directly quorum hash
        auto quorum = llmq_params.useRotation ? llmq::SelectQuorumForSigning(llmq_params, m_chainstate.m_chain, qman,
                                                                             id, nSignHeight, signOffset)
                                              : qman.GetQuorum(llmq_params.type, islock->cycleHash);

        if (!quorum) {
            // should not happen, but if one fails to select, all others will also fail to select
            return {};
        }
        uint256 signHash = BuildSignHash(llmq_params.type, quorum->qc->quorumHash, id, islock->txid);
        batchVerifier.PushMessage(nodeId, hash, signHash, islock->sig.Get(), quorum->qc->quorumPublicKey);
        verifyCount++;

        // We can reconstruct the CRecoveredSig objects from the islock and pass it to the signing manager, which
        // avoids unnecessary double-verification of the signature. We however only do this when verification here
        // turns out to be good (which is checked further down)
        if (!sigman.HasRecoveredSigForId(llmq_params.type, id)) {
            recSigs.try_emplace(hash, CRecoveredSig(llmq_params.type, quorum->qc->quorumHash, id, islock->txid, islock->sig));
        }
    }

    cxxtimer::Timer verifyTimer(true);
    batchVerifier.Verify();
    verifyTimer.stop();

    LogPrint(BCLog::INSTANTSEND, "CInstantSendManager::%s -- verified locks. count=%d, alreadyVerified=%d, vt=%d, nodes=%d\n", __func__,
            verifyCount, alreadyVerified, verifyTimer.count(), batchVerifier.GetUniqueSourceCount());

    std::unordered_set<uint256, StaticSaltedHasher> badISLocks;

    if (ban && !batchVerifier.badSources.empty()) {
        LOCK(::cs_main);
        for (const auto& nodeId : batchVerifier.badSources) {
            // Let's not be too harsh, as the peer might simply be unlucky and might have sent us an old lock which
            // does not validate anymore due to changed quorums
            peerman.Misbehaving(nodeId, 20);
        }
    }
    for (const auto& p : pend) {
        const auto& hash = p.first;
        auto nodeId = p.second.first;
        const auto& islock = p.second.second;

        if (batchVerifier.badMessages.count(hash)) {
            LogPrint(BCLog::INSTANTSEND, "CInstantSendManager::%s -- txid=%s, islock=%s: invalid sig in islock, peer=%d\n", __func__,
                     islock->txid.ToString(), hash.ToString(), nodeId);
            badISLocks.emplace(hash);
            continue;
        }

        ProcessInstantSendLock(nodeId, peerman, hash, islock);

        // See comment further on top. We pass a reconstructed recovered sig to the signing manager to avoid
        // double-verification of the sig.
        auto it = recSigs.find(hash);
        if (it != recSigs.end()) {
            auto recSig = std::make_shared<CRecoveredSig>(std::move(it->second));
            if (!sigman.HasRecoveredSigForId(llmq_params.type, recSig->getId())) {
                LogPrint(BCLog::INSTANTSEND, "CInstantSendManager::%s -- txid=%s, islock=%s: passing reconstructed recSig to signing mgr, peer=%d\n", __func__,
                         islock->txid.ToString(), hash.ToString(), nodeId);
                sigman.PushReconstructedRecoveredSig(recSig);
            }
        }
    }

    return badISLocks;
}

void CInstantSendManager::ProcessInstantSendLock(NodeId from, PeerManager& peerman, const uint256& hash,
                                                 const instantsend::InstantSendLockPtr& islock)
{
    LogPrint(BCLog::INSTANTSEND, "CInstantSendManager::%s -- txid=%s, islock=%s: processing islock, peer=%d\n", __func__,
             islock->txid.ToString(), hash.ToString(), from);
    if (m_signer) {
        m_signer->ClearLockFromQueue(islock);
    }
    if (db.KnownInstantSendLock(hash)) {
        return;
    }

    uint256 hashBlock{};
    const auto tx = GetTransaction(nullptr, &mempool, islock->txid, Params().GetConsensus(), hashBlock);
    const CBlockIndex* pindexMined{nullptr};
    // we ignore failure here as we must be able to propagate the lock even if we don't have the TX locally
    if (tx && !hashBlock.IsNull()) {
        pindexMined = WITH_LOCK(::cs_main, return m_chainstate.m_blockman.LookupBlockIndex(hashBlock));

        // Let's see if the TX that was locked by this islock is already mined in a ChainLocked block. If yes,
        // we can simply ignore the islock, as the ChainLock implies locking of all TXs in that chain
        if (pindexMined != nullptr && clhandler.HasChainLock(pindexMined->nHeight, pindexMined->GetBlockHash())) {
            LogPrint(BCLog::INSTANTSEND, "CInstantSendManager::%s -- txlock=%s, islock=%s: dropping islock as it already got a ChainLock in block %s, peer=%d\n", __func__,
                     islock->txid.ToString(), hash.ToString(), hashBlock.ToString(), from);
            return;
        }
    }

    const auto sameTxIsLock = db.GetInstantSendLockByTxid(islock->txid);
    if (sameTxIsLock != nullptr) {
        // can happen, nothing to do
        return;
    }
    for (const auto& in : islock->inputs) {
        const auto sameOutpointIsLock = db.GetInstantSendLockByInput(in);
        if (sameOutpointIsLock != nullptr) {
            LogPrintf("CInstantSendManager::%s -- txid=%s, islock=%s: conflicting outpoint in islock. input=%s, other islock=%s, peer=%d\n", __func__,
                      islock->txid.ToString(), hash.ToString(), in.ToStringShort(), ::SerializeHash(*sameOutpointIsLock).ToString(), from);
        }
    }

    if (tx == nullptr) {
        // put it in a separate pending map and try again later
        LOCK(cs_pendingLocks);
        pendingNoTxInstantSendLocks.try_emplace(hash, std::make_pair(from, islock));
    } else {
        db.WriteNewInstantSendLock(hash, *islock);
        if (pindexMined) {
            db.WriteInstantSendLockMined(hash, pindexMined->nHeight);
        }
    }

    // This will also add children TXs to pendingRetryTxs
    RemoveNonLockedTx(islock->txid, true);
    // We don't need the recovered sigs for the inputs anymore. This prevents unnecessary propagation of these sigs.
    // We only need the ISLOCK from now on to detect conflicts
    TruncateRecoveredSigsForInputs(*islock);

    CInv inv(MSG_ISDLOCK, hash);
    if (tx != nullptr) {
        peerman.RelayInvFiltered(inv, *tx, ISDLOCK_PROTO_VERSION);
    } else {
        // we don't have the TX yet, so we only filter based on txid. Later when that TX arrives, we will re-announce
        // with the TX taken into account.
        peerman.RelayInvFiltered(inv, islock->txid, ISDLOCK_PROTO_VERSION);
    }

    ResolveBlockConflicts(hash, *islock);

    if (tx != nullptr) {
        RemoveMempoolConflictsForLock(peerman, hash, *islock);
        LogPrint(BCLog::INSTANTSEND, "CInstantSendManager::%s -- notify about lock %s for tx %s\n", __func__,
                hash.ToString(), tx->GetHash().ToString());
        GetMainSignals().NotifyTransactionLock(tx, islock);
        // bump mempool counter to make sure newly locked txes are picked up by getblocktemplate
        mempool.AddTransactionsUpdated(1);
    } else {
        peerman.AskPeersForTransaction(islock->txid, /*is_masternode=*/m_signer != nullptr);
    }
}

void CInstantSendManager::TransactionAddedToMempool(PeerManager& peerman, const CTransactionRef& tx)
{
    if (!IsInstantSendEnabled() || !m_mn_sync.IsBlockchainSynced() || tx->vin.empty()) {
        return;
    }

    instantsend::InstantSendLockPtr islock{nullptr};
    {
        LOCK(cs_pendingLocks);
        auto it = pendingNoTxInstantSendLocks.begin();
        while (it != pendingNoTxInstantSendLocks.end()) {
            if (it->second.second->txid == tx->GetHash()) {
                // we received an islock earlier
                LogPrint(BCLog::INSTANTSEND, "CInstantSendManager::%s -- txid=%s, islock=%s\n", __func__,
                         tx->GetHash().ToString(), it->first.ToString());
                islock = it->second.second;
                pendingInstantSendLocks.try_emplace(it->first, it->second);
                pendingNoTxInstantSendLocks.erase(it);
                break;
            }
            ++it;
        }
    }

    if (islock == nullptr) {
        if (m_signer) {
            m_signer->ProcessTx(*tx, false, Params().GetConsensus());
        }
        // TX is not locked, so make sure it is tracked
        AddNonLockedTx(tx, nullptr);
    } else {
        RemoveMempoolConflictsForLock(peerman, ::SerializeHash(*islock), *islock);
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

    LogPrint(BCLog::INSTANTSEND, "CInstantSendManager::%s -- transaction %s was removed from mempool\n", __func__, tx->GetHash().ToString());
    RemoveConflictingLock(::SerializeHash(*islock), *islock);
}

void CInstantSendManager::BlockConnected(const std::shared_ptr<const CBlock>& pblock, const CBlockIndex* pindex)
{
    if (!IsInstantSendEnabled()) {
        return;
    }

    if (m_mn_sync.IsBlockchainSynced()) {
        const bool has_chainlock = clhandler.HasChainLock(pindex->nHeight, pindex->GetBlockHash());
        for (const auto& tx : pblock->vtx) {
            if (tx->IsCoinBase() || tx->vin.empty()) {
                // coinbase and TXs with no inputs can't be locked
                continue;
            }

            if (!IsLocked(tx->GetHash()) && !has_chainlock) {
                if (m_signer) {
                    m_signer->ProcessTx(*tx, true, Params().GetConsensus());
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

void CInstantSendManager::BlockDisconnected(const std::shared_ptr<const CBlock>& pblock, const CBlockIndex* pindexDisconnected)
{
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
            for (const auto &in: tx->vin) {
                nonLockedTxs[in.prevout.hash].children.emplace(tx->GetHash());
                nonLockedTxsByOutpoints.emplace(in.prevout, tx->GetHash());
            }
        }
    }
    {
        LOCK(cs_pendingLocks);
        auto it = pendingNoTxInstantSendLocks.begin();
        while (it != pendingNoTxInstantSendLocks.end()) {
            if (it->second.second->txid == tx->GetHash()) {
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

    LogPrint(BCLog::INSTANTSEND, "CInstantSendManager::%s -- txid=%s, retryChildren=%d, retryChildrenCount=%d\n", __func__,
             txid.ToString(), retryChildren, retryChildrenCount);
}

void CInstantSendManager::RemoveConflictedTx(const CTransaction& tx)
{
    RemoveNonLockedTx(tx.GetHash(), false);
    if (m_signer) {
        m_signer->ClearInputsFromQueue(GetIdsFromLockable(tx.vin));
    }
}

void CInstantSendManager::TruncateRecoveredSigsForInputs(const instantsend::InstantSendLock& islock)
{
    auto ids = GetIdsFromLockable(islock.inputs);
    if (m_signer) {
        m_signer->ClearInputsFromQueue(ids);
    }
    for (const auto& id : ids) {
        sigman.TruncateRecoveredSig(Params().GetConsensus().llmqTypeDIP0024InstantSend, id);
    }
}

void CInstantSendManager::TryEmplacePendingLock(const uint256& hash, const NodeId id, const instantsend::InstantSendLockPtr& islock)
{
    if (db.KnownInstantSendLock(hash)) return;
    LOCK(cs_pendingLocks);
    if (!pendingInstantSendLocks.count(hash)) {
        pendingInstantSendLocks.emplace(hash, std::make_pair(id, islock));
    }
}

void CInstantSendManager::NotifyChainLock(const CBlockIndex* pindexChainLock)
{
    HandleFullyConfirmedBlock(pindexChainLock);
}

void CInstantSendManager::UpdatedBlockTip(const CBlockIndex* pindexNew)
{
    bool fDIP0008Active = pindexNew->pprev && pindexNew->pprev->nHeight >= Params().GetConsensus().DIP0008Height;

    if (AreChainLocksEnabled(spork_manager) && fDIP0008Active) {
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
        for (const auto& p: nonLockedTxs) {
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

void CInstantSendManager::RemoveMempoolConflictsForLock(PeerManager& peerman, const uint256& hash,
                                                        const instantsend::InstantSendLock& islock)
{
    std::unordered_map<uint256, CTransactionRef, StaticSaltedHasher> toDelete;

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

    if (!toDelete.empty()) {
        for (const auto& p : toDelete) {
            RemoveConflictedTx(*p.second);
        }
        peerman.AskPeersForTransaction(islock.txid, /*is_masternode=*/m_signer != nullptr);
    }
}

void CInstantSendManager::ResolveBlockConflicts(const uint256& islockHash, const instantsend::InstantSendLock& islock)
{
    // Lets first collect all non-locked TXs which conflict with the given ISLOCK
    std::unordered_map<const CBlockIndex*, std::unordered_map<uint256, CTransactionRef, StaticSaltedHasher>> conflicts;
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
        if (clhandler.HasChainLock(pindex->nHeight, pindex->GetBlockHash())) {
            hasChainLockedConflict = true;
            break;
        }
    }

    // If a conflict was mined into a ChainLocked block, then we have no other choice and must prune the ISLOCK and all
    // chained ISLOCKs that build on top of this one. The probability of this is practically zero and can only happen
    // when large parts of the masternode network are controlled by an attacker. In this case we must still find consensus
    // and its better to sacrifice individual ISLOCKs then to sacrifice whole ChainLocks.
    if (hasChainLockedConflict) {
        LogPrintf("CInstantSendManager::%s -- txid=%s, islock=%s: at least one conflicted TX already got a ChainLock\n", __func__,
                  islock.txid.ToString(), islockHash.ToString());
        RemoveConflictingLock(islockHash, islock);
        return;
    }

    bool isLockedTxKnown = WITH_LOCK(cs_pendingLocks, return pendingNoTxInstantSendLocks.find(islockHash) == pendingNoTxInstantSendLocks.end());

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
            LogPrintf("CChainLocksHandler::%s -- ActivateBestChain failed: %s\n", __func__, state.ToString());
            // This should not have happened and we are in a state were it's not safe to continue anymore
            assert(false);
        }
    }
}

void CInstantSendManager::RemoveConflictingLock(const uint256& islockHash, const instantsend::InstantSendLock& islock)
{
    LogPrintf("CInstantSendManager::%s -- txid=%s, islock=%s: Removing ISLOCK and its chained children\n", __func__,
              islock.txid.ToString(), islockHash.ToString());
    int tipHeight = WITH_LOCK(::cs_main, return m_chainstate.m_chain.Height());

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

    return WITH_LOCK(cs_pendingLocks, return pendingInstantSendLocks.count(inv.hash) != 0 || pendingNoTxInstantSendLocks.count(inv.hash) != 0)
            || db.KnownInstantSendLock(inv.hash);
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
            islock = it->second.second;
        } else {
            auto itNoTx = pendingNoTxInstantSendLocks.find(hash);
            if (itNoTx != pendingNoTxInstantSendLocks.end()) {
                islock = itNoTx->second.second;
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
        if (it->second.second->txid == txHash) {
            LogPrint(BCLog::INSTANTSEND, "CInstantSendManager::%s -- txid=%s, islock=%s\n", __func__,
                     txHash.ToString(), it->first.ToString());
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

void CInstantSendManager::WorkThreadMain(PeerManager& peerman)
{
    while (!workInterrupt) {
        bool fMoreWork = [&]() -> bool {
            if (!IsInstantSendEnabled()) return false;
            const bool more_work{ProcessPendingInstantSendLocks(peerman)};
            if (!m_signer) return more_work;
            // Construct set of non-locked transactions that are pending to retry
            std::vector<CTransactionRef> txns{};
            {
                LOCK2(cs_nonLocked, cs_pendingRetry);
                if (pendingRetryTxs.empty()) return more_work;
                txns.reserve(pendingRetryTxs.size());
                for (const auto& txid : pendingRetryTxs) {
                    if (auto it = nonLockedTxs.find(txid); it != nonLockedTxs.end()) {
                        const auto& [_, tx_info] = *it;
                        if (tx_info.tx) {
                            txns.push_back(tx_info.tx);
                        }
                    }
                }
            }
            // Retry processing them
            m_signer->ProcessPendingRetryLockTxs(txns);
            return more_work;
        }();

        if (!fMoreWork && !workInterrupt.sleep_for(std::chrono::milliseconds(100))) {
            return;
        }
    }
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
