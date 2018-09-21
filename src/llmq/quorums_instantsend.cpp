// Copyright (c) 2019 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "quorums_chainlocks.h"
#include "quorums_instantsend.h"
#include "quorums_utils.h"

#include "bls/bls_batchverifier.h"
#include "chainparams.h"
#include "coins.h"
#include "txmempool.h"
#include "masternode-sync.h"
#include "net_processing.h"
#include "scheduler.h"
#include "spork.h"
#include "validation.h"
#include "wallet/wallet.h"

// needed for nCompleteTXLocks
#include "instantx.h"

#include <boost/algorithm/string/replace.hpp>

namespace llmq
{

static const std::string INPUTLOCK_REQUESTID_PREFIX = "inlock";
static const std::string IXLOCK_REQUESTID_PREFIX = "ixlock";

CInstantSendManager* quorumInstantSendManager;

uint256 CInstantXLock::GetRequestId() const
{
    CHashWriter hw(SER_GETHASH, 0);
    hw << IXLOCK_REQUESTID_PREFIX;
    hw << inputs;
    return hw.GetHash();
}

CInstantSendManager::CInstantSendManager(CScheduler* _scheduler) :
    scheduler(_scheduler)
{
}

CInstantSendManager::~CInstantSendManager()
{
}

void CInstantSendManager::RegisterAsRecoveredSigsListener()
{
    quorumSigningManager->RegisterRecoveredSigsListener(this);
}

void CInstantSendManager::UnregisterAsRecoveredSigsListener()
{
    quorumSigningManager->UnregisterRecoveredSigsListener(this);
}

bool CInstantSendManager::ProcessTx(CNode* pfrom, const CTransaction& tx, CConnman& connman, const Consensus::Params& params)
{
    if (!IsNewInstantSendEnabled()) {
        return true;
    }

    auto llmqType = params.llmqForInstantSend;
    if (llmqType == Consensus::LLMQ_NONE) {
        return true;
    }
    if (!fMasternodeMode) {
        return true;
    }

    // Ignore any InstantSend messages until blockchain is synced
    if (!masternodeSync.IsBlockchainSynced()) {
        return true;
    }

    if (IsConflicted(tx)) {
        return false;
    }

    if (!CheckCanLock(tx, true, params)) {
        return false;
    }

    std::vector<uint256> ids;
    ids.reserve(tx.vin.size());

    for (const auto& in : tx.vin) {
        auto id = ::SerializeHash(std::make_pair(INPUTLOCK_REQUESTID_PREFIX, in.prevout));
        ids.emplace_back(id);
    }

    {
        LOCK(cs);
        size_t alreadyVotedCount = 0;
        for (size_t i = 0; i < ids.size(); i++) {
            auto it = inputVotes.find(ids[i]);
            if (it != inputVotes.end()) {
                if (it->second != tx.GetHash()) {
                    LogPrint("instantsend", "CInstantSendManager::%s -- txid=%s: input %s is conflicting with ixlock %s\n", __func__,
                            tx.GetHash().ToString(), tx.vin[i].prevout.ToStringShort(), it->second.ToString());
                    return false;
                }
                alreadyVotedCount++;
            }
        }
        if (alreadyVotedCount == ids.size()) {
            return true;
        }

        for (auto& id : ids) {
            inputVotes.emplace(id, tx.GetHash());
        }
    }

    // don't even try the actual signing if any input is conflicting
    for (auto& id : ids) {
        if (quorumSigningManager->IsConflicting(llmqType, id, tx.GetHash())) {
            return false;
        }
    }
    for (auto& id : ids) {
        quorumSigningManager->AsyncSignIfMember(llmqType, id, tx.GetHash());
    }

    // We might have received all input locks before we got the corresponding TX. In this case, we have to sign the
    // ixlock now instead of waiting for the input locks.
    TrySignInstantXLock(tx);

    return true;
}

bool CInstantSendManager::CheckCanLock(const CTransaction& tx, bool printDebug, const Consensus::Params& params)
{
    int nInstantSendConfirmationsRequired = params.nInstantSendConfirmationsRequired;

    uint256 txHash = tx.GetHash();
    CAmount nValueIn = 0;
    for (const auto& in : tx.vin) {
        CAmount v = 0;
        if (!CheckCanLock(in.prevout, printDebug, &txHash, &v, params)) {
            return false;
        }

        nValueIn += v;
    }

    // TODO decide if we should limit max input values. This was ok to do in the old system, but in the new system
    // where we want to have all TXs locked at some point, this is counterproductive (especially when ChainLocks later
    // depend on all TXs being locked first)
//    CAmount maxValueIn = sporkManager.GetSporkValue(SPORK_5_INSTANTSEND_MAX_VALUE);
//    if (nValueIn > maxValueIn * COIN) {
//        if (printDebug) {
//            LogPrint("instantsend", "CInstantSendManager::%s -- txid=%s: TX input value too high. nValueIn=%f, maxValueIn=%d", __func__,
//                     tx.GetHash().ToString(), nValueIn / (double)COIN, maxValueIn);
//        }
//        return false;
//    }

    return true;
}

bool CInstantSendManager::CheckCanLock(const COutPoint& outpoint, bool printDebug, const uint256* _txHash, CAmount* retValue, const Consensus::Params& params)
{
    int nInstantSendConfirmationsRequired = params.nInstantSendConfirmationsRequired;

    if (IsLocked(outpoint.hash)) {
        // if prevout was ix locked, allow locking of descendants (no matter if prevout is in mempool or already mined)
        return true;
    }

    static uint256 txHashNull;
    const uint256* txHash = &txHashNull;
    if (_txHash) {
        txHash = _txHash;
    }

    auto mempoolTx = mempool.get(outpoint.hash);
    if (mempoolTx) {
        if (printDebug) {
            LogPrint("instantsend", "CInstantSendManager::%s -- txid=%s: parent mempool TX %s is not locked\n", __func__,
                     txHash->ToString(), outpoint.hash.ToString());
        }
        return false;
    }

    Coin coin;
    const CBlockIndex* pindexMined = nullptr;
    {
        LOCK(cs_main);
        if (!pcoinsTip->GetCoin(outpoint, coin) || coin.IsSpent()) {
            if (printDebug) {
                LogPrint("instantsend", "CInstantSendManager::%s -- txid=%s: failed to find UTXO %s\n", __func__,
                         txHash->ToString(), outpoint.ToStringShort());
            }
            return false;
        }
        pindexMined = chainActive[coin.nHeight];
    }

    int nTxAge = chainActive.Height() - coin.nHeight + 1;
    // 1 less than the "send IX" gui requires, in case of a block propagating the network at the time
    int nConfirmationsRequired = nInstantSendConfirmationsRequired - 1;

    if (nTxAge < nConfirmationsRequired) {
        if (!llmq::chainLocksHandler->HasChainLock(pindexMined->nHeight, pindexMined->GetBlockHash())) {
            if (printDebug) {
                LogPrint("instantsend", "CInstantSendManager::%s -- txid=%s: outpoint %s too new and not ChainLocked. nTxAge=%d, nConfirmationsRequired=%d\n", __func__,
                         txHash->ToString(), outpoint.ToStringShort(), nTxAge, nConfirmationsRequired);
            }
            return false;
        }
    }

    if (retValue) {
        *retValue = coin.out.nValue;
    }

    return true;
}

void CInstantSendManager::HandleNewRecoveredSig(const CRecoveredSig& recoveredSig)
{
    if (!IsNewInstantSendEnabled()) {
        return;
    }

    auto llmqType = Params().GetConsensus().llmqForInstantSend;
    if (llmqType == Consensus::LLMQ_NONE) {
        return;
    }
    auto& params = Params().GetConsensus().llmqs.at(llmqType);

    uint256 txid;
    bool isInstantXLock = false;
    {
        LOCK(cs);
        auto it = inputVotes.find(recoveredSig.id);
        if (it != inputVotes.end()) {
            txid = it->second;
        }
        if (creatingInstantXLocks.count(recoveredSig.id)) {
            isInstantXLock = true;
        }
    }
    if (!txid.IsNull()) {
        HandleNewInputLockRecoveredSig(recoveredSig, txid);
    } else if (isInstantXLock) {
        HandleNewInstantXLockRecoveredSig(recoveredSig);
    }
}

void CInstantSendManager::HandleNewInputLockRecoveredSig(const CRecoveredSig& recoveredSig, const uint256& txid)
{
    auto llmqType = Params().GetConsensus().llmqForInstantSend;

    CTransactionRef tx;
    uint256 hashBlock;
    if (!GetTransaction(txid, tx, Params().GetConsensus(), hashBlock, true)) {
        return;
    }

    if (LogAcceptCategory("instantsend")) {
        for (auto& in : tx->vin) {
            auto id = ::SerializeHash(std::make_pair(INPUTLOCK_REQUESTID_PREFIX, in.prevout));
            if (id == recoveredSig.id) {
                LogPrint("instantsend", "CInstantSendManager::%s -- txid=%s: got recovered sig for input %s\n", __func__,
                         txid.ToString(), in.prevout.ToStringShort());
                break;
            }
        }
    }

    TrySignInstantXLock(*tx);
}

void CInstantSendManager::TrySignInstantXLock(const CTransaction& tx)
{
    auto llmqType = Params().GetConsensus().llmqForInstantSend;

    for (auto& in : tx.vin) {
        auto id = ::SerializeHash(std::make_pair(INPUTLOCK_REQUESTID_PREFIX, in.prevout));
        if (!quorumSigningManager->HasRecoveredSig(llmqType, id, tx.GetHash())) {
            return;
        }
    }

    LogPrint("instantsend", "CInstantSendManager::%s -- txid=%s: got all recovered sigs, creating CInstantXLock\n", __func__,
            tx.GetHash().ToString());

    CInstantXLockInfo ixlockInfo;
    ixlockInfo.time = GetTimeMillis();
    ixlockInfo.ixlock.txid = tx.GetHash();
    for (auto& in : tx.vin) {
        ixlockInfo.ixlock.inputs.emplace_back(in.prevout);
    }

    auto id = ixlockInfo.ixlock.GetRequestId();

    if (quorumSigningManager->HasRecoveredSigForId(llmqType, id)) {
        return;
    }

    {
        LOCK(cs);
        auto e = creatingInstantXLocks.emplace(id, ixlockInfo);
        if (!e.second) {
            return;
        }
        txToCreatingInstantXLocks.emplace(tx.GetHash(), &e.first->second);
    }

    quorumSigningManager->AsyncSignIfMember(llmqType, id, tx.GetHash());
}

void CInstantSendManager::HandleNewInstantXLockRecoveredSig(const llmq::CRecoveredSig& recoveredSig)
{
    CInstantXLockInfo ixlockInfo;

    {
        LOCK(cs);
        auto it = creatingInstantXLocks.find(recoveredSig.id);
        if (it == creatingInstantXLocks.end()) {
            return;
        }

        ixlockInfo = std::move(it->second);
        creatingInstantXLocks.erase(it);
        txToCreatingInstantXLocks.erase(ixlockInfo.ixlock.txid);
    }

    if (ixlockInfo.ixlock.txid != recoveredSig.msgHash) {
        LogPrint("instantsend", "CInstantSendManager::%s -- txid=%s: ixlock conflicts with %s, dropping own version", __func__,
                ixlockInfo.ixlock.txid.ToString(), recoveredSig.msgHash.ToString());
        return;
    }

    ixlockInfo.ixlock.sig = recoveredSig.sig;
    ProcessInstantXLock(-1, ::SerializeHash(ixlockInfo.ixlock), ixlockInfo.ixlock);
}

void CInstantSendManager::ProcessMessage(CNode* pfrom, const std::string& strCommand, CDataStream& vRecv, CConnman& connman)
{
    if (!IsNewInstantSendEnabled()) {
        return;
    }

    if (strCommand == NetMsgType::IXLOCK) {
        CInstantXLock ixlock;
        vRecv >> ixlock;
        ProcessMessageInstantXLock(pfrom, ixlock, connman);
    }
}

void CInstantSendManager::ProcessMessageInstantXLock(CNode* pfrom, const llmq::CInstantXLock& ixlock, CConnman& connman)
{
    bool ban = false;
    if (!PreVerifyInstantXLock(pfrom->id, ixlock, ban)) {
        if (ban) {
            LOCK(cs_main);
            Misbehaving(pfrom->id, 100);
        }
        return;
    }

    auto hash = ::SerializeHash(ixlock);

    LOCK(cs);
    if (pendingInstantXLocks.count(hash) || finalInstantXLocks.count(hash)) {
        return;
    }

    LogPrint("instantsend", "CInstantSendManager::%s -- txid=%s, ixlock=%s: received ixlock, peer=%d\n", __func__,
            ixlock.txid.ToString(), hash.ToString(), pfrom->id);

    pendingInstantXLocks.emplace(hash, std::make_pair(pfrom->id, std::move(ixlock)));

    if (!hasScheduledProcessPending) {
        hasScheduledProcessPending = true;
        scheduler->schedule([&] {
            ProcessPendingInstantXLocks();
        }, boost::chrono::system_clock::now() + boost::chrono::milliseconds(100));
    }
}

bool CInstantSendManager::PreVerifyInstantXLock(NodeId nodeId, const llmq::CInstantXLock& ixlock, bool& retBan)
{
    retBan = false;

    if (ixlock.txid.IsNull() || !ixlock.sig.IsValid() || ixlock.inputs.empty()) {
        retBan = true;
        return false;
    }

    std::set<COutPoint> dups;
    for (auto& o : ixlock.inputs) {
        if (!dups.emplace(o).second) {
            retBan = true;
            return false;
        }
    }

    return true;
}

void CInstantSendManager::ProcessPendingInstantXLocks()
{
    auto llmqType = Params().GetConsensus().llmqForInstantSend;

    decltype(pendingInstantXLocks) pend;

    {
        LOCK(cs);
        hasScheduledProcessPending = false;
        pend = std::move(pendingInstantXLocks);
    }

    if (!IsNewInstantSendEnabled()) {
        return;
    }

    int tipHeight;
    {
        LOCK(cs_main);
        tipHeight = chainActive.Height();
    }

    CBLSBatchVerifier<NodeId, uint256> batchVerifier(false, true, 8);
    std::unordered_map<uint256, std::pair<CQuorumCPtr, CRecoveredSig>> recSigs;

    for (const auto& p : pend) {
        auto& hash = p.first;
        auto nodeId = p.second.first;
        auto& ixlock = p.second.second;

        auto id = ixlock.GetRequestId();

        // no need to verify an IXLOCK if we already have verified the recovered sig that belongs to it
        if (quorumSigningManager->HasRecoveredSig(llmqType, id, ixlock.txid)) {
            continue;
        }

        auto quorum = quorumSigningManager->SelectQuorumForSigning(llmqType, tipHeight, id);
        if (!quorum) {
            // should not happen, but if one fails to select, all others will also fail to select
            return;
        }
        uint256 signHash = CLLMQUtils::BuildSignHash(llmqType, quorum->quorumHash, id, ixlock.txid);
        batchVerifier.PushMessage(nodeId, hash, signHash, ixlock.sig, quorum->quorumPublicKey);

        // We can reconstruct the CRecoveredSig objects from the ixlock and pass it to the signing manager, which
        // avoids unnecessary double-verification of the signature. We however only do this when verification here
        // turns out to be good (which is checked further down)
        if (!quorumSigningManager->HasRecoveredSigForId(llmqType, id)) {
            CRecoveredSig recSig;
            recSig.llmqType = llmqType;
            recSig.quorumHash = quorum->quorumHash;
            recSig.id = id;
            recSig.msgHash = ixlock.txid;
            recSig.sig = ixlock.sig;
            recSigs.emplace(std::piecewise_construct,
                    std::forward_as_tuple(hash),
                    std::forward_as_tuple(std::move(quorum), std::move(recSig)));
        }
    }

    batchVerifier.Verify();

    if (!batchVerifier.badSources.empty()) {
        LOCK(cs_main);
        for (auto& nodeId : batchVerifier.badSources) {
            // Let's not be too harsh, as the peer might simply be unlucky and might have sent us an old lock which
            // does not validate anymore due to changed quorums
            Misbehaving(nodeId, 20);
        }
    }
    for (const auto& p : pend) {
        auto& hash = p.first;
        auto nodeId = p.second.first;
        auto& ixlock = p.second.second;

        if (batchVerifier.badMessages.count(hash)) {
            LogPrint("instantsend", "CInstantSendManager::%s -- txid=%s, ixlock=%s: invalid sig in ixlock, peer=%d\n", __func__,
                     ixlock.txid.ToString(), hash.ToString(), nodeId);
            continue;
        }

        ProcessInstantXLock(nodeId, hash, ixlock);

        // See comment further on top. We pass a reconstructed recovered sig to the signing manager to avoid
        // double-verification of the sig.
        auto it = recSigs.find(hash);
        if (it != recSigs.end()) {
            auto& quorum = it->second.first;
            auto& recSig = it->second.second;
            if (!quorumSigningManager->HasRecoveredSigForId(llmqType, recSig.id)) {
                recSig.UpdateHash();
                LogPrint("instantsend", "CInstantSendManager::%s -- txid=%s, ixlock=%s: passing reconstructed recSig to signing mgr, peer=%d\n", __func__,
                         ixlock.txid.ToString(), hash.ToString(), nodeId);
                quorumSigningManager->PushReconstructedRecoveredSig(recSig, quorum);
            }
        }
    }
}

void CInstantSendManager::ProcessInstantXLock(NodeId from, const uint256& hash, const CInstantXLock& ixlock)
{
    {
        LOCK(cs_main);
        g_connman->RemoveAskFor(hash);
    }

    CInstantXLockInfo ixlockInfo;
    ixlockInfo.time = GetTimeMillis();
    ixlockInfo.ixlock = ixlock;
    ixlockInfo.ixlockHash = hash;

    uint256 hashBlock;
    // we ignore failure here as we must be able to propagate the lock even if we don't have the TX locally
    if (GetTransaction(ixlock.txid, ixlockInfo.tx, Params().GetConsensus(), hashBlock)) {
        if (!hashBlock.IsNull()) {
            {
                LOCK(cs_main);
                ixlockInfo.pindexMined = mapBlockIndex.at(hashBlock);
            }

            // Let's see if the TX that was locked by this ixlock is already mined in a ChainLocked block. If yes,
            // we can simply ignore the ixlock, as the ChainLock implies locking of all TXs in that chain
            if (llmq::chainLocksHandler->HasChainLock(ixlockInfo.pindexMined->nHeight, ixlockInfo.pindexMined->GetBlockHash())) {
                LogPrint("instantsend", "CInstantSendManager::%s -- txlock=%s, ixlock=%s: dropping ixlock as it already got a ChainLock in block %s, peer=%d\n", __func__,
                         ixlock.txid.ToString(), hash.ToString(), hashBlock.ToString(), from);
                return;
            }
        }
    }

    {
        LOCK(cs);
        auto e = finalInstantXLocks.emplace(hash, ixlockInfo);
        if (!e.second) {
            return;
        }
        auto ixlockInfoPtr = &e.first->second;

        creatingInstantXLocks.erase(ixlockInfoPtr->ixlock.GetRequestId());
        txToCreatingInstantXLocks.erase(ixlockInfoPtr->ixlock.txid);

        LogPrint("instantsend", "CInstantSendManager::%s -- txid=%s, ixlock=%s: processsing ixlock, peer=%d\n", __func__,
                 ixlock.txid.ToString(), hash.ToString(), from);

        if (!txToInstantXLock.emplace(ixlock.txid, ixlockInfoPtr).second) {
            LogPrint("instantsend", "CInstantSendManager::%s -- txid=%s, ixlock=%s: duplicate ixlock, other ixlock=%s, peer=%d\n", __func__,
                    ixlock.txid.ToString(), hash.ToString(), txToInstantXLock[ixlock.txid]->ixlockHash.ToString(), from);
            txToInstantXLock.erase(hash);
            return;
        }
        for (size_t i = 0; i < ixlock.inputs.size(); i++) {
            auto& in = ixlock.inputs[i];
            if (!inputToInstantXLock.emplace(in, ixlockInfoPtr).second) {
                LogPrint("instantsend", "CInstantSendManager::%s -- txid=%s, ixlock=%s: conflicting input in ixlock. input=%s, other ixlock=%s, peer=%d\n", __func__,
                         ixlock.txid.ToString(), hash.ToString(), in.ToStringShort(), inputToInstantXLock[in]->ixlockHash.ToString(), from);
                txToInstantXLock.erase(hash);
                for (size_t j = 0; j < i; j++) {
                    inputToInstantXLock.erase(ixlock.inputs[j]);
                }
                return;
            }
        }
    }

    CInv inv(MSG_IXLOCK, hash);
    g_connman->RelayInv(inv);

    RemoveMempoolConflictsForLock(hash, ixlock);
    RetryLockMempoolTxs(ixlock.txid);

    UpdateWalletTransaction(ixlock.txid);
}

void CInstantSendManager::UpdateWalletTransaction(const uint256& txid)
{
#ifdef ENABLE_WALLET
    if (!pwalletMain) {
        return;
    }

    if (pwalletMain->UpdatedTransaction(txid)) {
        // bumping this to update UI
        nCompleteTXLocks++;
        // notify an external script once threshold is reached
        std::string strCmd = GetArg("-instantsendnotify", "");
        if (!strCmd.empty()) {
            boost::replace_all(strCmd, "%s", txid.GetHex());
            boost::thread t(runCommand, strCmd); // thread runs free
        }
    }
#endif

    LOCK(cs);
    auto it = txToInstantXLock.find(txid);
    if (it == txToInstantXLock.end()) {
        return;
    }
    if (it->second->tx == nullptr) {
        return;
    }

    GetMainSignals().NotifyTransactionLock(*it->second->tx);
}

void CInstantSendManager::SyncTransaction(const CTransaction& tx, const CBlockIndex* pindex, int posInBlock)
{
    if (!IsNewInstantSendEnabled()) {
        return;
    }

    {
        LOCK(cs);
        auto it = txToInstantXLock.find(tx.GetHash());
        if (it == txToInstantXLock.end()) {
            return;
        }
        auto ixlockInfo = it->second;
        if (ixlockInfo->tx == nullptr) {
            ixlockInfo->tx = MakeTransactionRef(tx);
        }

        if (posInBlock == CMainSignals::SYNC_TRANSACTION_NOT_IN_BLOCK) {
            UpdateIxLockMinedBlock(ixlockInfo, nullptr);
            return;
        }
        UpdateIxLockMinedBlock(ixlockInfo, pindex);
    }

    if (IsLocked(tx.GetHash())) {
        RetryLockMempoolTxs(tx.GetHash());
    }
}

void CInstantSendManager::NotifyChainLock(const CBlockIndex* pindex)
{
    {
        LOCK(cs);

        // Let's find all ixlocks that correspond to TXs which are part of the freshly ChainLocked chain and then delete
        // the ixlocks. We do this because the ChainLocks imply locking and thus it's not needed to further track
        // or propagate the ixlocks
        std::unordered_set<uint256> toDelete;
        while (pindex && pindex != pindexLastChainLock) {
            auto its = blockToInstantXLocks.equal_range(pindex->GetBlockHash());
            while (its.first != its.second) {
                auto ixlockInfo = its.first->second;
                LogPrint("instantsend", "CInstantSendManager::%s -- txid=%s, ixlock=%s: removing ixlock as it got ChainLocked in block %s\n", __func__,
                         ixlockInfo->ixlock.txid.ToString(), ixlockInfo->ixlockHash.ToString(), pindex->GetBlockHash().ToString());
                toDelete.emplace(its.first->second->ixlockHash);
                ++its.first;
            }

            pindex = pindex->pprev;
        }

        pindexLastChainLock = pindex;

        for (auto& ixlockHash : toDelete) {
            RemoveFinalIxLock(ixlockHash);
        }
    }

    RetryLockMempoolTxs(uint256());
}

void CInstantSendManager::UpdateIxLockMinedBlock(llmq::CInstantXLockInfo* ixlockInfo, const CBlockIndex* pindex)
{
    AssertLockHeld(cs);
    
    if (ixlockInfo->pindexMined == pindex) {
        return;
    }
    
    if (ixlockInfo->pindexMined) {
        auto its = blockToInstantXLocks.equal_range(ixlockInfo->pindexMined->GetBlockHash());
        while (its.first != its.second) {
            if (its.first->second == ixlockInfo) {
                its.first = blockToInstantXLocks.erase(its.first);
            } else {
                ++its.first;
            }
        }
    }
    
    if (pindex) {
        blockToInstantXLocks.emplace(pindex->GetBlockHash(), ixlockInfo);
    }
    
    ixlockInfo->pindexMined = pindex;
}

void CInstantSendManager::RemoveFinalIxLock(const uint256& hash)
{
    AssertLockHeld(cs);
    
    auto it = finalInstantXLocks.find(hash);
    if (it == finalInstantXLocks.end()) {
        return;
    }
    auto& ixlockInfo = it->second;

    txToInstantXLock.erase(ixlockInfo.ixlock.txid);
    for (auto& in : ixlockInfo.ixlock.inputs) {
        auto inputRequestId = ::SerializeHash(std::make_pair(INPUTLOCK_REQUESTID_PREFIX, in));
        inputVotes.erase(inputRequestId);
        inputToInstantXLock.erase(in);
    }
    UpdateIxLockMinedBlock(&ixlockInfo, nullptr);
}

void CInstantSendManager::RemoveMempoolConflictsForLock(const uint256& hash, const CInstantXLock& ixlock)
{
    LOCK(mempool.cs);

    std::unordered_map<uint256, CTransactionRef> toDelete;

    for (auto& in : ixlock.inputs) {
        auto it = mempool.mapNextTx.find(in);
        if (it == mempool.mapNextTx.end()) {
            continue;
        }
        if (it->second->GetHash() != ixlock.txid) {
            toDelete.emplace(it->second->GetHash(), mempool.get(it->second->GetHash()));

            LogPrint("instantsend", "CInstantSendManager::%s -- txid=%s, ixlock=%s: mempool TX %s with input %s conflicts with ixlock\n", __func__,
                     ixlock.txid.ToString(), hash.ToString(), it->second->GetHash().ToString(), in.ToStringShort());
        }
    }

    for (auto& p : toDelete) {
        mempool.removeRecursive(*p.second, MemPoolRemovalReason::CONFLICT);
    }
}

void CInstantSendManager::RetryLockMempoolTxs(const uint256& lockedParentTx)
{
    // Let's retry all mempool TXs which don't have an ixlock yet and where the parents got ChainLocked now

    std::unordered_map<uint256, CTransactionRef> txs;

    {
        LOCK(mempool.cs);

        if (lockedParentTx.IsNull()) {
            txs.reserve(mempool.mapTx.size());
            for (auto it = mempool.mapTx.begin(); it != mempool.mapTx.end(); ++it) {
                txs.emplace(it->GetTx().GetHash(), it->GetSharedTx());
            }
        } else {
            auto it = mempool.mapNextTx.lower_bound(COutPoint(lockedParentTx, 0));
            while (it != mempool.mapNextTx.end() && it->first->hash == lockedParentTx) {
                txs.emplace(it->second->GetHash(), mempool.get(it->second->GetHash()));
                ++it;
            }
        }
    }
    for (auto& p : txs) {
        auto& tx = p.second;
        {
            LOCK(cs);
            if (txToCreatingInstantXLocks.count(tx->GetHash())) {
                // we're already in the middle of locking this one
                continue;
            }
            if (IsLocked(tx->GetHash())) {
                continue;
            }
            if (IsConflicted(*tx)) {
                // should not really happen as we have already filtered these out
                continue;
            }
        }

        // CheckCanLock is already called by ProcessTx, so we should avoid calling it twice. But we also shouldn't spam
        // the logs when retrying TXs that are not ready yet.
        if (LogAcceptCategory("instantsend")) {
            if (!CheckCanLock(*tx, false, Params().GetConsensus())) {
                continue;
            }
            LogPrint("instantsend", "CInstantSendManager::%s -- txid=%s: retrying to lock\n", __func__,
                     tx->GetHash().ToString());
        }

        ProcessTx(nullptr, *tx, *g_connman, Params().GetConsensus());
    }
}

bool CInstantSendManager::AlreadyHave(const CInv& inv)
{
    if (!IsNewInstantSendEnabled()) {
        return true;
    }

    LOCK(cs);
    return finalInstantXLocks.count(inv.hash) != 0 || pendingInstantXLocks.count(inv.hash) != 0;
}

bool CInstantSendManager::GetInstantXLockByHash(const uint256& hash, llmq::CInstantXLock& ret)
{
    if (!IsNewInstantSendEnabled()) {
        return false;
    }

    LOCK(cs);
    auto it = finalInstantXLocks.find(hash);
    if (it == finalInstantXLocks.end()) {
        return false;
    }
    ret = it->second.ixlock;
    return true;
}

bool CInstantSendManager::IsLocked(const uint256& txHash)
{
    if (!IsNewInstantSendEnabled()) {
        return false;
    }

    LOCK(cs);
    return txToInstantXLock.count(txHash) != 0;
}

bool CInstantSendManager::IsConflicted(const CTransaction& tx)
{
    LOCK(cs);
    uint256 dummy;
    return GetConflictingTx(tx, dummy);
}

bool CInstantSendManager::GetConflictingTx(const CTransaction& tx, uint256& retConflictTxHash)
{
    if (!IsNewInstantSendEnabled()) {
        return false;
    }

    LOCK(cs);
    for (const auto& in : tx.vin) {
        auto it = inputToInstantXLock.find(in.prevout);
        if (it == inputToInstantXLock.end()) {
            continue;
        }

        if (it->second->ixlock.txid != tx.GetHash()) {
            retConflictTxHash = it->second->ixlock.txid;
            return true;
        }
    }
    return false;
}

bool IsOldInstantSendEnabled()
{
    int spork2Value = sporkManager.GetSporkValue(SPORK_2_INSTANTSEND_ENABLED);
    if (spork2Value == 0) {
        return true;
    }
    return false;
}

bool IsNewInstantSendEnabled()
{
    int spork2Value = sporkManager.GetSporkValue(SPORK_2_INSTANTSEND_ENABLED);
    if (spork2Value == 1) {
        return true;
    }
    return false;
}

bool IsInstantSendEnabled()
{
    int spork2Value = sporkManager.GetSporkValue(SPORK_2_INSTANTSEND_ENABLED);
    return spork2Value == 0 || spork2Value == 1;
}

}
