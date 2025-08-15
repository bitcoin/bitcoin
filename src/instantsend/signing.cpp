// Copyright (c) 2019-2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <instantsend/signing.h>

#include <chain.h>
#include <chainparams.h>
#include <index/txindex.h>
#include <logging.h>
#include <util/irange.h>
#include <validation.h>

#include <chainlock/chainlock.h>
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

namespace instantsend {
InstantSendSigner::InstantSendSigner(CChainState& chainstate, llmq::CChainLocksHandler& clhandler,
                                     InstantSendSignerParent& isman, llmq::CSigningManager& sigman,
                                     llmq::CSigSharesManager& shareman, llmq::CQuorumManager& qman,
                                     CSporkManager& sporkman, CTxMemPool& mempool, const CMasternodeSync& mn_sync) :
    m_chainstate{chainstate},
    m_clhandler{clhandler},
    m_isman{isman},
    m_sigman{sigman},
    m_shareman{shareman},
    m_qman{qman},
    m_sporkman{sporkman},
    m_mempool{mempool},
    m_mn_sync{mn_sync}
{
}

InstantSendSigner::~InstantSendSigner() = default;

void InstantSendSigner::Start()
{
    m_sigman.RegisterRecoveredSigsListener(this);
}

void InstantSendSigner::Stop()
{
    m_sigman.UnregisterRecoveredSigsListener(this);
}

void InstantSendSigner::ClearInputsFromQueue(const std::unordered_set<uint256, StaticSaltedHasher>& ids)
{
    LOCK(cs_input_requests);
    for (const auto& id : ids) {
        inputRequestIds.erase(id);
    }
}

void InstantSendSigner::ClearLockFromQueue(const InstantSendLockPtr& islock)
{
    LOCK(cs_creating);
    creatingInstantSendLocks.erase(islock->GetRequestId());
    txToCreatingInstantSendLocks.erase(islock->txid);
}

MessageProcessingResult InstantSendSigner::HandleNewRecoveredSig(const llmq::CRecoveredSig& recoveredSig)
{
    if (!m_isman.IsInstantSendEnabled()) {
        return {};
    }

    if (Params().GetConsensus().llmqTypeDIP0024InstantSend == Consensus::LLMQType::LLMQ_NONE) {
        return {};
    }

    uint256 txid;
    if (LOCK(cs_input_requests); inputRequestIds.count(recoveredSig.getId())) {
        txid = recoveredSig.getMsgHash();
    }
    if (!txid.IsNull()) {
        HandleNewInputLockRecoveredSig(recoveredSig, txid);
    } else if (/*isInstantSendLock=*/WITH_LOCK(cs_creating, return creatingInstantSendLocks.count(recoveredSig.getId()))) {
        HandleNewInstantSendLockRecoveredSig(recoveredSig);
    }
    return {};
}

bool InstantSendSigner::IsInstantSendMempoolSigningEnabled() const
{
    return !fReindex && !fImporting && m_sporkman.GetSporkValue(SPORK_2_INSTANTSEND_ENABLED) == 0;
}

void InstantSendSigner::HandleNewInputLockRecoveredSig(const llmq::CRecoveredSig& recoveredSig, const uint256& txid)
{
    if (g_txindex) {
        g_txindex->BlockUntilSyncedToCurrentChain();
    }

    uint256 _hashBlock{};
    const auto tx = GetTransaction(nullptr, &m_mempool, txid, Params().GetConsensus(), _hashBlock);
    if (!tx) {
        return;
    }

    if (LogAcceptDebug(BCLog::INSTANTSEND)) {
        for (const auto& in : tx->vin) {
            if (GenInputLockRequestId(in.prevout) == recoveredSig.getId()) {
                LogPrint(BCLog::INSTANTSEND, "%s -- txid=%s: got recovered sig for input %s\n", __func__,
                         txid.ToString(), in.prevout.ToStringShort());
                break;
            }
        }
    }

    TrySignInstantSendLock(*tx);
}

void InstantSendSigner::ProcessPendingRetryLockTxs(const std::vector<CTransactionRef>& retryTxs)
{
    if (!m_isman.IsInstantSendEnabled()) {
        return;
    }

    int retryCount = 0;
    for (const auto& tx : retryTxs) {
        {
            if (LOCK(cs_creating); txToCreatingInstantSendLocks.count(tx->GetHash())) {
                // we're already in the middle of locking this one
                continue;
            }
            if (m_isman.IsLocked(tx->GetHash())) {
                continue;
            }
            if (m_isman.GetConflictingLock(*tx) != nullptr) {
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
            LogPrint(BCLog::INSTANTSEND, "%s -- txid=%s: retrying to lock\n", __func__, tx->GetHash().ToString());
        }

        ProcessTx(*tx, false, Params().GetConsensus());
        retryCount++;
    }

    if (retryCount != 0) {
        LogPrint(BCLog::INSTANTSEND, "%s -- retried %d TXs.\n", __func__, retryCount);
    }
}

bool InstantSendSigner::CheckCanLock(const CTransaction& tx, bool printDebug, const Consensus::Params& params) const
{
    if (tx.vin.empty()) {
        // can't lock TXs without inputs (e.g. quorum commitments)
        return false;
    }

    return ranges::all_of(tx.vin,
                          [&](const auto& in) { return CheckCanLock(in.prevout, printDebug, tx.GetHash(), params); });
}

bool InstantSendSigner::CheckCanLock(const COutPoint& outpoint, bool printDebug, const uint256& txHash,
                                     const Consensus::Params& params) const
{
    int nInstantSendConfirmationsRequired = params.nInstantSendConfirmationsRequired;

    if (m_isman.IsLocked(outpoint.hash)) {
        // if prevout was ix locked, allow locking of descendants (no matter if prevout is in mempool or already mined)
        return true;
    }

    auto mempoolTx = m_mempool.get(outpoint.hash);
    if (mempoolTx) {
        if (printDebug) {
            LogPrint(BCLog::INSTANTSEND, "%s -- txid=%s: parent mempool TX %s is not locked\n", __func__,
                     txHash.ToString(), outpoint.hash.ToString());
        }
        return false;
    }

    uint256 hashBlock{};
    const auto tx = GetTransaction(nullptr, &m_mempool, outpoint.hash, params, hashBlock);
    // this relies on enabled txindex and won't work if we ever try to remove the requirement for txindex for masternodes
    if (!tx) {
        if (printDebug) {
            LogPrint(BCLog::INSTANTSEND, "%s -- txid=%s: failed to find parent TX %s\n", __func__, txHash.ToString(),
                     outpoint.hash.ToString());
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

    if (nTxAge < nInstantSendConfirmationsRequired &&
        !m_clhandler.HasChainLock(pindexMined->nHeight, pindexMined->GetBlockHash())) {
        if (printDebug) {
            LogPrint(BCLog::INSTANTSEND, "%s -- txid=%s: outpoint %s too new and not ChainLocked. nTxAge=%d, nInstantSendConfirmationsRequired=%d\n", __func__,
                     txHash.ToString(), outpoint.ToStringShort(), nTxAge, nInstantSendConfirmationsRequired);
        }
        return false;
    }

    return true;
}

void InstantSendSigner::HandleNewInstantSendLockRecoveredSig(const llmq::CRecoveredSig& recoveredSig)
{
    InstantSendLockPtr islock;

    {
        LOCK(cs_creating);
        auto it = creatingInstantSendLocks.find(recoveredSig.getId());
        if (it == creatingInstantSendLocks.end()) {
            return;
        }

        islock = std::make_shared<InstantSendLock>(std::move(it->second));
        creatingInstantSendLocks.erase(it);
        txToCreatingInstantSendLocks.erase(islock->txid);
    }

    if (islock->txid != recoveredSig.getMsgHash()) {
        LogPrintf("%s -- txid=%s: islock conflicts with %s, dropping own version\n", __func__, islock->txid.ToString(),
                  recoveredSig.getMsgHash().ToString());
        return;
    }

    islock->sig = recoveredSig.sig;
    m_isman.TryEmplacePendingLock(/*hash=*/::SerializeHash(*islock), /*id=*/-1, islock);
}

void InstantSendSigner::ProcessTx(const CTransaction& tx, bool fRetroactive, const Consensus::Params& params)
{
    if (!m_isman.IsInstantSendEnabled() || !m_mn_sync.IsBlockchainSynced()) {
        return;
    }

    if (params.llmqTypeDIP0024InstantSend == Consensus::LLMQType::LLMQ_NONE) {
        return;
    }

    if (!CheckCanLock(tx, true, params)) {
        LogPrint(BCLog::INSTANTSEND, "%s -- txid=%s: CheckCanLock returned false\n", __func__, tx.GetHash().ToString());
        return;
    }

    auto conflictingLock = m_isman.GetConflictingLock(tx);
    if (conflictingLock != nullptr) {
        auto conflictingLockHash = ::SerializeHash(*conflictingLock);
        LogPrintf("%s -- txid=%s: conflicts with islock %s, txid=%s\n", __func__, tx.GetHash().ToString(),
                  conflictingLockHash.ToString(), conflictingLock->txid.ToString());
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

bool InstantSendSigner::TrySignInputLocks(const CTransaction& tx, bool fRetroactive, Consensus::LLMQType llmqType,
                                          const Consensus::Params& params)
{
    std::vector<uint256> ids;
    ids.reserve(tx.vin.size());

    size_t alreadyVotedCount = 0;
    for (const auto& in : tx.vin) {
        auto id = GenInputLockRequestId(in);
        ids.emplace_back(id);

        uint256 otherTxHash;
        if (m_sigman.GetVoteForId(params.llmqTypeDIP0024InstantSend, id, otherTxHash)) {
            if (otherTxHash != tx.GetHash()) {
                LogPrintf("%s -- txid=%s: input %s is conflicting with previous vote for tx %s\n", __func__,
                          tx.GetHash().ToString(), in.prevout.ToStringShort(), otherTxHash.ToString());
                return false;
            }
            alreadyVotedCount++;
        }

        // don't even try the actual signing if any input is conflicting
        if (m_sigman.IsConflicting(params.llmqTypeDIP0024InstantSend, id, tx.GetHash())) {
            LogPrintf("%s -- txid=%s: m_sigman.IsConflicting returned true. id=%s\n", __func__, tx.GetHash().ToString(),
                      id.ToString());
            return false;
        }
    }
    if (!fRetroactive && alreadyVotedCount == ids.size()) {
        LogPrint(BCLog::INSTANTSEND, "%s -- txid=%s: already voted on all inputs, bailing out\n", __func__,
                 tx.GetHash().ToString());
        return true;
    }

    LogPrint(BCLog::INSTANTSEND, "%s -- txid=%s: trying to vote on %d inputs\n", __func__, tx.GetHash().ToString(),
             tx.vin.size());

    for (const auto i : irange::range(tx.vin.size())) {
        const auto& in = tx.vin[i];
        auto& id = ids[i];
        WITH_LOCK(cs_input_requests, inputRequestIds.emplace(id));
        LogPrint(BCLog::INSTANTSEND, "%s -- txid=%s: trying to vote on input %s with id %s. fRetroactive=%d\n",
                 __func__, tx.GetHash().ToString(), in.prevout.ToStringShort(), id.ToString(), fRetroactive);
        if (m_sigman.AsyncSignIfMember(llmqType, m_shareman, id, tx.GetHash(), {}, fRetroactive)) {
            LogPrint(BCLog::INSTANTSEND, "%s -- txid=%s: voted on input %s with id %s\n", __func__,
                     tx.GetHash().ToString(), in.prevout.ToStringShort(), id.ToString());
        }
    }

    return true;
}

void InstantSendSigner::TrySignInstantSendLock(const CTransaction& tx)
{
    const auto llmqType = Params().GetConsensus().llmqTypeDIP0024InstantSend;

    for (const auto& in : tx.vin) {
        auto id = GenInputLockRequestId(in);
        if (!m_sigman.HasRecoveredSig(llmqType, id, tx.GetHash())) {
            return;
        }
    }

    LogPrint(BCLog::INSTANTSEND, "%s -- txid=%s: got all recovered sigs, creating InstantSendLock\n", __func__,
             tx.GetHash().ToString());

    InstantSendLock islock;
    islock.txid = tx.GetHash();
    for (const auto& in : tx.vin) {
        islock.inputs.emplace_back(in.prevout);
    }

    auto id = islock.GetRequestId();

    if (m_sigman.HasRecoveredSigForId(llmqType, id)) {
        return;
    }

    const auto& llmq_params_opt = Params().GetLLMQ(llmqType);
    assert(llmq_params_opt);
    const auto quorum = llmq::SelectQuorumForSigning(llmq_params_opt.value(), m_chainstate.m_chain, m_qman, id);

    if (!quorum) {
        LogPrint(BCLog::INSTANTSEND, "%s -- failed to select quorum. islock id=%s, txid=%s\n", __func__, id.ToString(),
                 tx.GetHash().ToString());
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

    m_sigman.AsyncSignIfMember(llmqType, m_shareman, id, tx.GetHash(), quorum->m_quorum_base_block_index->GetBlockHash());
}
} // namespace instantsend
