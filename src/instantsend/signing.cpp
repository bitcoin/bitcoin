// Copyright (c) 2019-2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chain.h>
#include <chainparams.h>
#include <index/txindex.h>
#include <logging.h>
#include <util/irange.h>
#include <validation.h>

#include <instantsend/db.h>
#include <instantsend/instantsend.h>
#include <llmq/chainlocks.h>
#include <llmq/quorums.h>
#include <masternode/sync.h>
#include <spork.h>

// Forward declaration to break dependency over node/transaction.h
namespace node {
CTransactionRef GetTransaction(const CBlockIndex* const block_index, const CTxMemPool* const mempool,
                               const uint256& hash, const Consensus::Params& consensusParams, uint256& hashBlock);
} // namespace node

using node::fImporting;
using node::fReindex;
using node::GetTransaction;

namespace llmq {
static const std::string_view INPUTLOCK_REQUESTID_PREFIX = "inlock";

MessageProcessingResult CInstantSendManager::HandleNewRecoveredSig(const CRecoveredSig& recoveredSig)
{
    if (!IsInstantSendEnabled()) {
        return {};
    }

    if (Params().GetConsensus().llmqTypeDIP0024InstantSend == Consensus::LLMQType::LLMQ_NONE) {
        return {};
    }

    uint256 txid;
    if (LOCK(cs_inputReqests); inputRequestIds.count(recoveredSig.getId())) {
        txid = recoveredSig.getMsgHash();
    }
    if (!txid.IsNull()) {
        HandleNewInputLockRecoveredSig(recoveredSig, txid);
    } else if (/*isInstantSendLock=*/ WITH_LOCK(cs_creating, return creatingInstantSendLocks.count(recoveredSig.getId()))) {
        HandleNewInstantSendLockRecoveredSig(recoveredSig);
    }
    return {};
}

bool CInstantSendManager::IsInstantSendMempoolSigningEnabled() const
{
    return !fReindex && !fImporting && spork_manager.GetSporkValue(SPORK_2_INSTANTSEND_ENABLED) == 0;
}

void CInstantSendManager::HandleNewInputLockRecoveredSig(const CRecoveredSig& recoveredSig, const uint256& txid)
{
    if (g_txindex) {
        g_txindex->BlockUntilSyncedToCurrentChain();
    }

    uint256 hashBlock{};
    const auto tx = GetTransaction(nullptr, &mempool, txid, Params().GetConsensus(), hashBlock);
    if (!tx) {
        return;
    }

    if (LogAcceptDebug(BCLog::INSTANTSEND)) {
        for (const auto& in : tx->vin) {
            auto id = ::SerializeHash(std::make_pair(INPUTLOCK_REQUESTID_PREFIX, in.prevout));
            if (id == recoveredSig.getId()) {
                LogPrint(BCLog::INSTANTSEND, "CInstantSendManager::%s -- txid=%s: got recovered sig for input %s\n", __func__,
                         txid.ToString(), in.prevout.ToStringShort());
                break;
            }
        }
    }

    TrySignInstantSendLock(*tx);
}

void CInstantSendManager::ProcessPendingRetryLockTxs()
{
    const auto retryTxs = WITH_LOCK(cs_pendingRetry, return pendingRetryTxs);

    if (retryTxs.empty()) {
        return;
    }

    if (!IsInstantSendEnabled()) {
        return;
    }

    int retryCount = 0;
    for (const auto& txid : retryTxs) {
        CTransactionRef tx;
        {
            {
                LOCK(cs_nonLocked);
                auto it = nonLockedTxs.find(txid);
                if (it == nonLockedTxs.end()) {
                    continue;
                }
                tx = it->second.tx;
            }
            if (!tx) {
                continue;
            }

            if (LOCK(cs_creating); txToCreatingInstantSendLocks.count(tx->GetHash())) {
                // we're already in the middle of locking this one
                continue;
            }
            if (IsLocked(tx->GetHash())) {
                continue;
            }
            if (GetConflictingLock(*tx) != nullptr) {
                // should not really happen as we have already filtered these out
                continue;
            }
        }

        // CheckCanLock is already called by ProcessTx, so we should avoid calling it twice. But we also shouldn't spam
        // the logs when retrying TXs that are not ready yet.
        if (LogAcceptDebug(BCLog::INSTANTSEND)) {
            if (!CheckCanLock(*tx, false, Params().GetConsensus())) {
                continue;
            }
            LogPrint(BCLog::INSTANTSEND, "CInstantSendManager::%s -- txid=%s: retrying to lock\n", __func__,
                     tx->GetHash().ToString());
        }

        ProcessTx(*tx, false, Params().GetConsensus());
        retryCount++;
    }

    if (retryCount != 0) {
        LogPrint(BCLog::INSTANTSEND, "CInstantSendManager::%s -- retried %d TXs. nonLockedTxs.size=%d\n", __func__,
                 retryCount, WITH_LOCK(cs_nonLocked, return nonLockedTxs.size()));
    }
}

bool CInstantSendManager::CheckCanLock(const CTransaction& tx, bool printDebug, const Consensus::Params& params) const
{
    if (tx.vin.empty()) {
        // can't lock TXs without inputs (e.g. quorum commitments)
        return false;
    }

    return ranges::all_of(tx.vin,
                          [&](const auto& in) { return CheckCanLock(in.prevout, printDebug, tx.GetHash(), params); });
}

bool CInstantSendManager::CheckCanLock(const COutPoint& outpoint, bool printDebug, const uint256& txHash, const Consensus::Params& params) const
{
    int nInstantSendConfirmationsRequired = params.nInstantSendConfirmationsRequired;

    if (IsLocked(outpoint.hash)) {
        // if prevout was ix locked, allow locking of descendants (no matter if prevout is in mempool or already mined)
        return true;
    }

    auto mempoolTx = mempool.get(outpoint.hash);
    if (mempoolTx) {
        if (printDebug) {
            LogPrint(BCLog::INSTANTSEND, "CInstantSendManager::%s -- txid=%s: parent mempool TX %s is not locked\n", __func__,
                     txHash.ToString(), outpoint.hash.ToString());
        }
        return false;
    }

    uint256 hashBlock{};
    const auto tx = GetTransaction(nullptr, &mempool, outpoint.hash, params, hashBlock);
    // this relies on enabled txindex and won't work if we ever try to remove the requirement for txindex for masternodes
    if (!tx) {
        if (printDebug) {
            LogPrint(BCLog::INSTANTSEND, "CInstantSendManager::%s -- txid=%s: failed to find parent TX %s\n", __func__,
                     txHash.ToString(), outpoint.hash.ToString());
        }
        return false;
    }

    const CBlockIndex* pindexMined;
    int nTxAge;
    {
        LOCK(::cs_main);
        pindexMined = m_chainstate.m_blockman.LookupBlockIndex(hashBlock);
        nTxAge = m_chainstate.m_chain.Height() - pindexMined->nHeight + 1;
    }

    if (nTxAge < nInstantSendConfirmationsRequired && !clhandler.HasChainLock(pindexMined->nHeight, pindexMined->GetBlockHash())) {
        if (printDebug) {
            LogPrint(BCLog::INSTANTSEND, "CInstantSendManager::%s -- txid=%s: outpoint %s too new and not ChainLocked. nTxAge=%d, nInstantSendConfirmationsRequired=%d\n", __func__,
                     txHash.ToString(), outpoint.ToStringShort(), nTxAge, nInstantSendConfirmationsRequired);
        }
        return false;
    }

    return true;
}

void CInstantSendManager::HandleNewInstantSendLockRecoveredSig(const llmq::CRecoveredSig& recoveredSig)
{
    CInstantSendLockPtr islock;

    {
        LOCK(cs_creating);
        auto it = creatingInstantSendLocks.find(recoveredSig.getId());
        if (it == creatingInstantSendLocks.end()) {
            return;
        }

        islock = std::make_shared<CInstantSendLock>(std::move(it->second));
        creatingInstantSendLocks.erase(it);
        txToCreatingInstantSendLocks.erase(islock->txid);
    }

    if (islock->txid != recoveredSig.getMsgHash()) {
        LogPrintf("CInstantSendManager::%s -- txid=%s: islock conflicts with %s, dropping own version\n", __func__,
                islock->txid.ToString(), recoveredSig.getMsgHash().ToString());
        return;
    }

    islock->sig = recoveredSig.sig;
    auto hash = ::SerializeHash(*islock);

    if (WITH_LOCK(cs_pendingLocks, return pendingInstantSendLocks.count(hash)) || db.KnownInstantSendLock(hash)) {
        return;
    }
    LOCK(cs_pendingLocks);
    pendingInstantSendLocks.emplace(hash, std::make_pair(-1, islock));
}

void CInstantSendManager::ProcessTx(const CTransaction& tx, bool fRetroactive, const Consensus::Params& params)
{
    if (!m_is_masternode || !IsInstantSendEnabled() || !m_mn_sync.IsBlockchainSynced()) {
        return;
    }

    if (params.llmqTypeDIP0024InstantSend == Consensus::LLMQType::LLMQ_NONE) {
        return;
    }

    if (!CheckCanLock(tx, true, params)) {
        LogPrint(BCLog::INSTANTSEND, "CInstantSendManager::%s -- txid=%s: CheckCanLock returned false\n", __func__,
                  tx.GetHash().ToString());
        return;
    }

    auto conflictingLock = GetConflictingLock(tx);
    if (conflictingLock != nullptr) {
        auto conflictingLockHash = ::SerializeHash(*conflictingLock);
        LogPrintf("CInstantSendManager::%s -- txid=%s: conflicts with islock %s, txid=%s\n", __func__,
                  tx.GetHash().ToString(), conflictingLockHash.ToString(), conflictingLock->txid.ToString());
        return;
    }

    // Only sign for inlocks or islocks if mempool IS signing is enabled.
    // However, if we are processing a tx because it was included in a block we should
    // sign even if mempool IS signing is disabled. This allows a ChainLock to happen on this
    // block after we retroactively locked all transactions.
    if (!IsInstantSendMempoolSigningEnabled() && !fRetroactive) return;

    if (!TrySignInputLocks(tx, fRetroactive, params.llmqTypeDIP0024InstantSend, params)) {
        return;
    }

    // We might have received all input locks before we got the corresponding TX. In this case, we have to sign the
    // islock now instead of waiting for the input locks.
    TrySignInstantSendLock(tx);
}

bool CInstantSendManager::TrySignInputLocks(const CTransaction& tx, bool fRetroactive, Consensus::LLMQType llmqType, const Consensus::Params& params)
{
    std::vector<uint256> ids;
    ids.reserve(tx.vin.size());

    size_t alreadyVotedCount = 0;
    for (const auto& in : tx.vin) {
        auto id = ::SerializeHash(std::make_pair(INPUTLOCK_REQUESTID_PREFIX, in.prevout));
        ids.emplace_back(id);

        uint256 otherTxHash;
        if (sigman.GetVoteForId(params.llmqTypeDIP0024InstantSend, id, otherTxHash)) {
            if (otherTxHash != tx.GetHash()) {
                LogPrintf("CInstantSendManager::%s -- txid=%s: input %s is conflicting with previous vote for tx %s\n", __func__,
                          tx.GetHash().ToString(), in.prevout.ToStringShort(), otherTxHash.ToString());
                return false;
            }
            alreadyVotedCount++;
        }

        // don't even try the actual signing if any input is conflicting
        if (sigman.IsConflicting(params.llmqTypeDIP0024InstantSend, id, tx.GetHash())) {
            LogPrintf("CInstantSendManager::%s -- txid=%s: sigman.IsConflicting returned true. id=%s\n", __func__,
                      tx.GetHash().ToString(), id.ToString());
            return false;
        }
    }
    if (!fRetroactive && alreadyVotedCount == ids.size()) {
        LogPrint(BCLog::INSTANTSEND, "CInstantSendManager::%s -- txid=%s: already voted on all inputs, bailing out\n", __func__,
                 tx.GetHash().ToString());
        return true;
    }

    LogPrint(BCLog::INSTANTSEND, "CInstantSendManager::%s -- txid=%s: trying to vote on %d inputs\n", __func__,
             tx.GetHash().ToString(), tx.vin.size());

    for (const auto i : irange::range(tx.vin.size())) {
        const auto& in = tx.vin[i];
        auto& id = ids[i];
        WITH_LOCK(cs_inputReqests, inputRequestIds.emplace(id));
        LogPrint(BCLog::INSTANTSEND, "CInstantSendManager::%s -- txid=%s: trying to vote on input %s with id %s. fRetroactive=%d\n", __func__,
                 tx.GetHash().ToString(), in.prevout.ToStringShort(), id.ToString(), fRetroactive);
        if (sigman.AsyncSignIfMember(llmqType, shareman, id, tx.GetHash(), {}, fRetroactive)) {
            LogPrint(BCLog::INSTANTSEND, "CInstantSendManager::%s -- txid=%s: voted on input %s with id %s\n", __func__,
                     tx.GetHash().ToString(), in.prevout.ToStringShort(), id.ToString());
        }
    }

    return true;
}

void CInstantSendManager::TrySignInstantSendLock(const CTransaction& tx)
{
    const auto llmqType = Params().GetConsensus().llmqTypeDIP0024InstantSend;

    for (const auto& in : tx.vin) {
        auto id = ::SerializeHash(std::make_pair(INPUTLOCK_REQUESTID_PREFIX, in.prevout));
        if (!sigman.HasRecoveredSig(llmqType, id, tx.GetHash())) {
            return;
        }
    }

    LogPrint(BCLog::INSTANTSEND, "CInstantSendManager::%s -- txid=%s: got all recovered sigs, creating CInstantSendLock\n", __func__,
            tx.GetHash().ToString());

    CInstantSendLock islock;
    islock.txid = tx.GetHash();
    for (const auto& in : tx.vin) {
        islock.inputs.emplace_back(in.prevout);
    }

    auto id = islock.GetRequestId();

    if (sigman.HasRecoveredSigForId(llmqType, id)) {
        return;
    }

    const auto& llmq_params_opt = Params().GetLLMQ(llmqType);
    assert(llmq_params_opt);
    const auto quorum = SelectQuorumForSigning(llmq_params_opt.value(), m_chainstate.m_chain, qman, id);

    if (!quorum) {
        LogPrint(BCLog::INSTANTSEND, "CInstantSendManager::%s -- failed to select quorum. islock id=%s, txid=%s\n",
                 __func__, id.ToString(), tx.GetHash().ToString());
        return;
    }

    const int cycle_height = quorum->m_quorum_base_block_index->nHeight -
                             quorum->m_quorum_base_block_index->nHeight % llmq_params_opt->dkgInterval;
    islock.cycleHash = quorum->m_quorum_base_block_index->GetAncestor(cycle_height)->GetBlockHash();

    {
        LOCK(cs_creating);
        auto e = creatingInstantSendLocks.emplace(id, std::move(islock));
        if (!e.second) {
            return;
        }
        txToCreatingInstantSendLocks.emplace(tx.GetHash(), &e.first->second);
    }

    sigman.AsyncSignIfMember(llmqType, shareman, id, tx.GetHash(), quorum->m_quorum_base_block_index->GetBlockHash());
}
} // namespace llmq
