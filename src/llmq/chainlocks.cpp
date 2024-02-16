// Copyright (c) 2019-2024 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <llmq/chainlocks.h>
#include <llmq/quorums.h>
#include <llmq/instantsend.h>
#include <llmq/signing_shares.h>

#include <chain.h>
#include <chainparams.h>
#include <consensus/validation.h>
#include <masternode/sync.h>
#include <node/blockstorage.h>
#include <node/ui_interface.h>
#include <scheduler.h>
#include <spork.h>
#include <txmempool.h>
#include <util/thread.h>
#include <util/time.h>
#include <validation.h>
#include <validationinterface.h>

namespace llmq
{
std::unique_ptr<CChainLocksHandler> chainLocksHandler;

CChainLocksHandler::CChainLocksHandler(CChainState& chainstate, CConnman& _connman, CMasternodeSync& mn_sync, CQuorumManager& _qman,
                                       CSigningManager& _sigman, CSigSharesManager& _shareman, CSporkManager& sporkManager,
                                       CTxMemPool& _mempool) :
    m_chainstate(chainstate),
    connman(_connman),
    m_mn_sync(mn_sync),
    qman(_qman),
    sigman(_sigman),
    shareman(_shareman),
    spork_manager(sporkManager),
    mempool(_mempool),
    scheduler(std::make_unique<CScheduler>()),
    scheduler_thread(std::make_unique<std::thread>(std::thread(util::TraceThread, "cl-schdlr", [&] { scheduler->serviceQueue(); })))
{
}

CChainLocksHandler::~CChainLocksHandler()
{
    scheduler->stop();
    scheduler_thread->join();
}

void CChainLocksHandler::Start()
{
    sigman.RegisterRecoveredSigsListener(this);
    scheduler->scheduleEvery([&]() {
        CheckActiveState();
        EnforceBestChainLock();
        // regularly retry signing the current chaintip as it might have failed before due to missing islocks
        TrySignChainTip();
    }, std::chrono::seconds{5});
}

void CChainLocksHandler::Stop()
{
    scheduler->stop();
    sigman.UnregisterRecoveredSigsListener(this);
}

bool CChainLocksHandler::AlreadyHave(const CInv& inv) const
{
    LOCK(cs);
    return seenChainLocks.count(inv.hash) != 0;
}

bool CChainLocksHandler::GetChainLockByHash(const uint256& hash, llmq::CChainLockSig& ret) const
{
    LOCK(cs);

    if (hash != bestChainLockHash) {
        // we only propagate the best one and ditch all the old ones
        return false;
    }

    ret = bestChainLock;
    return true;
}

CChainLockSig CChainLocksHandler::GetBestChainLock() const
{
    LOCK(cs);
    return bestChainLock;
}

PeerMsgRet CChainLocksHandler::ProcessMessage(const CNode& pfrom, const std::string& msg_type, CDataStream& vRecv)
{
    if (!AreChainLocksEnabled(spork_manager)) {
        return {};
    }

    if (msg_type == NetMsgType::CLSIG) {
        CChainLockSig clsig;
        vRecv >> clsig;

        return ProcessNewChainLock(pfrom.GetId(), clsig, ::SerializeHash(clsig));
    }
    return {};
}

PeerMsgRet CChainLocksHandler::ProcessNewChainLock(const NodeId from, const llmq::CChainLockSig& clsig, const uint256& hash)
{
    CheckActiveState();

    CInv clsigInv(MSG_CLSIG, hash);

    if (from != -1) {
        LOCK(cs_main);
        EraseObjectRequest(from, clsigInv);
    }

    {
        LOCK(cs);
        if (!seenChainLocks.emplace(hash, GetTimeMillis()).second) {
            return {};
        }

        if (!bestChainLock.IsNull() && clsig.getHeight() <= bestChainLock.getHeight()) {
            // no need to process/relay older CLSIGs
            return {};
        }
    }

    if (!VerifyChainLock(clsig)) {
        LogPrint(BCLog::CHAINLOCKS, "CChainLocksHandler::%s -- invalid CLSIG (%s), peer=%d\n", __func__, clsig.ToString(), from);
        if (from != -1) {
            return tl::unexpected{10};
        }
        return {};
    }

    CBlockIndex* pindex = WITH_LOCK(cs_main, return m_chainstate.m_blockman.LookupBlockIndex(clsig.getBlockHash()));

    {
        LOCK(cs);
        bestChainLockHash = hash;
        bestChainLock = clsig;

        if (pindex != nullptr) {

            if (pindex->nHeight != clsig.getHeight()) {
                // Should not happen, same as the conflict check from above.
                LogPrintf("CChainLocksHandler::%s -- height of CLSIG (%s) does not match the specified block's height (%d)\n",
                        __func__, clsig.ToString(), pindex->nHeight);
                // Note: not relaying clsig here
                return {};
            }

            bestChainLockWithKnownBlock = bestChainLock;
            bestChainLockBlockIndex = pindex;
        }
        // else if (pindex == nullptr)
        // Note: make sure to still relay clsig further.
    }

    // Note: do not hold cs while calling RelayInv
    AssertLockNotHeld(cs);
    connman.RelayInv(clsigInv);

    if (pindex == nullptr) {
        // we don't know the block/header for this CLSIG yet, so bail out for now
        // when the block or the header later comes in, we will enforce the correct chain
        return {};
    }

    scheduler->scheduleFromNow([&]() {
        CheckActiveState();
        EnforceBestChainLock();
    }, std::chrono::seconds{0});

    LogPrint(BCLog::CHAINLOCKS, "CChainLocksHandler::%s -- processed new CLSIG (%s), peer=%d\n",
              __func__, clsig.ToString(), from);
    return {};
}

void CChainLocksHandler::AcceptedBlockHeader(gsl::not_null<const CBlockIndex*> pindexNew)
{
    LOCK(cs);

    if (pindexNew->GetBlockHash() == bestChainLock.getBlockHash()) {
        LogPrint(BCLog::CHAINLOCKS, "CChainLocksHandler::%s -- block header %s came in late, updating and enforcing\n", __func__, pindexNew->GetBlockHash().ToString());

        if (bestChainLock.getHeight() != pindexNew->nHeight) {
            // Should not happen, same as the conflict check from ProcessNewChainLock.
            LogPrintf("CChainLocksHandler::%s -- height of CLSIG (%s) does not match the specified block's height (%d)\n",
                      __func__, bestChainLock.ToString(), pindexNew->nHeight);
            return;
        }

        // when EnforceBestChainLock is called later, it might end up invalidating other chains but not activating the
        // CLSIG locked chain. This happens when only the header is known but the block is still missing yet. The usual
        // block processing logic will handle this when the block arrives
        bestChainLockWithKnownBlock = bestChainLock;
        bestChainLockBlockIndex = pindexNew;
    }
}

void CChainLocksHandler::UpdatedBlockTip()
{
    // don't call TrySignChainTip directly but instead let the scheduler call it. This way we ensure that cs_main is
    // never locked and TrySignChainTip is not called twice in parallel. Also avoids recursive calls due to
    // EnforceBestChainLock switching chains.
    // atomic[If tryLockChainTipScheduled is false, do (set it to true] and schedule signing).
    if (bool expected = false; tryLockChainTipScheduled.compare_exchange_strong(expected, true)) {
        scheduler->scheduleFromNow([&]() {
            CheckActiveState();
            EnforceBestChainLock();
            TrySignChainTip();
            tryLockChainTipScheduled = false;
        }, std::chrono::seconds{0});
    }
}

void CChainLocksHandler::CheckActiveState()
{
    bool oldIsEnabled = isEnabled;
    isEnabled = AreChainLocksEnabled(spork_manager);

    if (!oldIsEnabled && isEnabled) {
        // ChainLocks got activated just recently, but it's possible that it was already running before, leaving
        // us with some stale values which we should not try to enforce anymore (there probably was a good reason
        // to disable spork19)
        LOCK(cs);
        bestChainLockHash = uint256();
        bestChainLock = bestChainLockWithKnownBlock = CChainLockSig();
        bestChainLockBlockIndex = lastNotifyChainLockBlockIndex = nullptr;
    }
}

void CChainLocksHandler::TrySignChainTip()
{
    Cleanup();

    if (!fMasternodeMode) {
        return;
    }

    if (!m_mn_sync.IsBlockchainSynced()) {
        return;
    }

    if (!isEnabled) {
        return;
    }

    if (!ChainLocksSigningEnabled(spork_manager)) {
        return;
    }

    const CBlockIndex* pindex = WITH_LOCK(cs_main, return m_chainstate.m_chain.Tip());

    if (pindex->pprev == nullptr) {
        return;
    }

    // DIP8 defines a process called "Signing attempts" which should run before the CLSIG is finalized
    // To simplify the initial implementation, we skip this process and directly try to create a CLSIG
    // This will fail when multiple blocks compete, but we accept this for the initial implementation.
    // Later, we'll add the multiple attempts process.

    {
        LOCK(cs);

        if (pindex->nHeight == lastSignedHeight) {
            // already signed this one
            return;
        }

        if (bestChainLock.getHeight() >= pindex->nHeight) {
            // already got the same CLSIG or a better one
            return;
        }

        if (InternalHasConflictingChainLock(pindex->nHeight, pindex->GetBlockHash())) {
            // don't sign if another conflicting CLSIG is already present. EnforceBestChainLock will later enforce
            // the correct chain.
            return;
        }
    }

    LogPrint(BCLog::CHAINLOCKS, "CChainLocksHandler::%s -- trying to sign %s, height=%d\n", __func__, pindex->GetBlockHash().ToString(), pindex->nHeight);

    // When the new IX system is activated, we only try to ChainLock blocks which include safe transactions. A TX is
    // considered safe when it is islocked or at least known since 10 minutes (from mempool or block). These checks are
    // performed for the tip (which we try to sign) and the previous 5 blocks. If a ChainLocked block is found on the
    // way down, we consider all TXs to be safe.
    if (quorumInstantSendManager->IsInstantSendEnabled() && quorumInstantSendManager->RejectConflictingBlocks()) {
        const auto* pindexWalk = pindex;
        while (pindexWalk != nullptr) {
            if (pindex->nHeight - pindexWalk->nHeight > 5) {
                // no need to check further down, 6 confs is safe to assume that TXs below this height won't be
                // islocked anymore if they aren't already
                LogPrint(BCLog::CHAINLOCKS, "CChainLocksHandler::%s -- tip and previous 5 blocks all safe\n", __func__);
                break;
            }
            if (HasChainLock(pindexWalk->nHeight, pindexWalk->GetBlockHash())) {
                // we don't care about islocks for TXs that are ChainLocked already
                LogPrint(BCLog::CHAINLOCKS, "CChainLocksHandler::%s -- chainlock at height %d\n", __func__, pindexWalk->nHeight);
                break;
            }

            auto txids = GetBlockTxs(pindexWalk->GetBlockHash());
            if (!txids) {
                pindexWalk = pindexWalk->pprev;
                continue;
            }

            for (const auto& txid : *txids) {
                int64_t txAge = 0;
                {
                    LOCK(cs);
                    auto it = txFirstSeenTime.find(txid);
                    if (it != txFirstSeenTime.end()) {
                        txAge = GetTime<std::chrono::seconds>().count() - it->second;
                    }
                }

                if (txAge < WAIT_FOR_ISLOCK_TIMEOUT && !quorumInstantSendManager->IsLocked(txid)) {
                    LogPrint(BCLog::CHAINLOCKS, "CChainLocksHandler::%s -- not signing block %s due to TX %s not being islocked and not old enough. age=%d\n", __func__,
                              pindexWalk->GetBlockHash().ToString(), txid.ToString(), txAge);
                    return;
                }
            }

            pindexWalk = pindexWalk->pprev;
        }
    }

    uint256 requestId = ::SerializeHash(std::make_pair(CLSIG_REQUESTID_PREFIX, pindex->nHeight));
    uint256 msgHash = pindex->GetBlockHash();

    {
        LOCK(cs);
        if (bestChainLock.getHeight() >= pindex->nHeight) {
            // might have happened while we didn't hold cs
            return;
        }
        lastSignedHeight = pindex->nHeight;
        lastSignedRequestId = requestId;
        lastSignedMsgHash = msgHash;
    }

    sigman.AsyncSignIfMember(Params().GetConsensus().llmqTypeChainLocks, shareman, requestId, msgHash);
}

void CChainLocksHandler::TransactionAddedToMempool(const CTransactionRef& tx, int64_t nAcceptTime)
{
    if (tx->IsCoinBase() || tx->vin.empty()) {
        return;
    }

    LOCK(cs);
    txFirstSeenTime.emplace(tx->GetHash(), nAcceptTime);
}

void CChainLocksHandler::BlockConnected(const std::shared_ptr<const CBlock>& pblock, gsl::not_null<const CBlockIndex*> pindex)
{
    if (!m_mn_sync.IsBlockchainSynced()) {
        return;
    }

    // We listen for BlockConnected so that we can collect all TX ids of all included TXs of newly received blocks
    // We need this information later when we try to sign a new tip, so that we can determine if all included TXs are
    // safe.

    LOCK(cs);

    auto it = blockTxs.find(pindex->GetBlockHash());
    if (it == blockTxs.end()) {
        // we must create this entry even if there are no lockable transactions in the block, so that TrySignChainTip
        // later knows about this block
        it = blockTxs.emplace(pindex->GetBlockHash(), std::make_shared<std::unordered_set<uint256, StaticSaltedHasher>>()).first;
    }
    auto& txids = *it->second;

    int64_t curTime = GetTime<std::chrono::seconds>().count();

    for (const auto& tx : pblock->vtx) {
        if (tx->IsCoinBase() || tx->vin.empty()) {
            continue;
        }

        txids.emplace(tx->GetHash());
        txFirstSeenTime.emplace(tx->GetHash(), curTime);
    }

}

void CChainLocksHandler::BlockDisconnected(const std::shared_ptr<const CBlock>& pblock, gsl::not_null<const CBlockIndex*> pindexDisconnected)
{
    LOCK(cs);
    blockTxs.erase(pindexDisconnected->GetBlockHash());
}

CChainLocksHandler::BlockTxs::mapped_type CChainLocksHandler::GetBlockTxs(const uint256& blockHash)
{
    AssertLockNotHeld(cs);
    AssertLockNotHeld(cs_main);

    CChainLocksHandler::BlockTxs::mapped_type ret;

    {
        LOCK(cs);
        auto it = blockTxs.find(blockHash);
        if (it != blockTxs.end()) {
            ret = it->second;
        }
    }
    if (!ret) {
        // This should only happen when freshly started.
        // If running for some time, SyncTransaction should have been called before which fills blockTxs.
        LogPrint(BCLog::CHAINLOCKS, "CChainLocksHandler::%s -- blockTxs for %s not found. Trying ReadBlockFromDisk\n", __func__,
                 blockHash.ToString());

        uint32_t blockTime;
        {
            LOCK(cs_main);
            const auto* pindex = m_chainstate.m_blockman.LookupBlockIndex(blockHash);
            CBlock block;
            if (!ReadBlockFromDisk(block, pindex, Params().GetConsensus())) {
                return nullptr;
            }

            ret = std::make_shared<std::unordered_set<uint256, StaticSaltedHasher>>();
            for (auto& tx : block.vtx) {
                if (tx->IsCoinBase() || tx->vin.empty()) {
                    continue;
                }
                ret->emplace(tx->GetHash());
            }

            blockTime = block.nTime;
        }

        LOCK(cs);
        blockTxs.emplace(blockHash, ret);
        for (const auto& txid : *ret) {
            txFirstSeenTime.emplace(txid, blockTime);
        }
    }
    return ret;
}

bool CChainLocksHandler::IsTxSafeForMining(const uint256& txid) const
{
    int64_t txAge = 0;
    {
        LOCK(cs);
        auto it = txFirstSeenTime.find(txid);
        if (it != txFirstSeenTime.end()) {
            txAge = GetTime<std::chrono::seconds>().count() - it->second;
        }
    }

    return txAge >= WAIT_FOR_ISLOCK_TIMEOUT;
}

// WARNING: cs_main and cs should not be held!
// This should also not be called from validation signals, as this might result in recursive calls
void CChainLocksHandler::EnforceBestChainLock()
{
    AssertLockNotHeld(cs);
    AssertLockNotHeld(cs_main);

    std::shared_ptr<CChainLockSig> clsig;
    const CBlockIndex* pindex;
    const CBlockIndex* currentBestChainLockBlockIndex;
    {
        LOCK(cs);

        if (!isEnabled) {
            return;
        }

        clsig = std::make_shared<CChainLockSig>(bestChainLockWithKnownBlock);
        pindex = currentBestChainLockBlockIndex = this->bestChainLockBlockIndex;

        if (currentBestChainLockBlockIndex == nullptr) {
            // we don't have the header/block, so we can't do anything right now
            return;
        }
    }

    BlockValidationState dummy_state;

    // Go backwards through the chain referenced by clsig until we find a block that is part of the main chain.
    // For each of these blocks, check if there are children that are NOT part of the chain referenced by clsig
    // and mark all of them as conflicting.
    LogPrint(BCLog::CHAINLOCKS, "CChainLocksHandler::%s -- enforcing block %s via CLSIG (%s)\n", __func__, pindex->GetBlockHash().ToString(), clsig->ToString());
    m_chainstate.EnforceBlock(dummy_state, pindex);

    bool activateNeeded = WITH_LOCK(::cs_main, return m_chainstate.m_chain.Tip()->GetAncestor(currentBestChainLockBlockIndex->nHeight)) != currentBestChainLockBlockIndex;

    if (activateNeeded) {
        if (!m_chainstate.ActivateBestChain(dummy_state)) {
            LogPrintf("CChainLocksHandler::%s -- ActivateBestChain failed: %s\n", __func__, dummy_state.ToString());
            return;
        }
        LOCK(cs_main);
        if (m_chainstate.m_chain.Tip()->GetAncestor(currentBestChainLockBlockIndex->nHeight) != currentBestChainLockBlockIndex) {
            return;
        }
    }

    {
        LOCK(cs);
        if (lastNotifyChainLockBlockIndex == currentBestChainLockBlockIndex) return;
        lastNotifyChainLockBlockIndex = currentBestChainLockBlockIndex;
    }

    GetMainSignals().NotifyChainLock(currentBestChainLockBlockIndex, clsig);
    uiInterface.NotifyChainLock(clsig->getBlockHash().ToString(), clsig->getHeight());
}

void CChainLocksHandler::HandleNewRecoveredSig(const llmq::CRecoveredSig& recoveredSig)
{
    if (!isEnabled) {
        return;
    }

    CChainLockSig clsig;
    {
        LOCK(cs);

        if (recoveredSig.getId() != lastSignedRequestId || recoveredSig.getMsgHash() != lastSignedMsgHash) {
            // this is not what we signed, so lets not create a CLSIG for it
            return;
        }
        if (bestChainLock.getHeight() >= lastSignedHeight) {
            // already got the same or a better CLSIG through the CLSIG message
            return;
        }


        clsig = CChainLockSig(lastSignedHeight, lastSignedMsgHash, recoveredSig.sig.Get());
    }
    ProcessNewChainLock(-1, clsig, ::SerializeHash(clsig));
}

bool CChainLocksHandler::HasChainLock(int nHeight, const uint256& blockHash) const
{
    LOCK(cs);
    return InternalHasChainLock(nHeight, blockHash);
}

bool CChainLocksHandler::VerifyChainLock(const CChainLockSig& clsig) const
{
    const auto llmqType = Params().GetConsensus().llmqTypeChainLocks;
    const uint256 nRequestId = ::SerializeHash(std::make_pair(llmq::CLSIG_REQUESTID_PREFIX, clsig.getHeight()));
    return llmq::VerifyRecoveredSig(llmqType, qman, clsig.getHeight(), nRequestId, clsig.getBlockHash(), clsig.getSig());
}

bool CChainLocksHandler::InternalHasChainLock(int nHeight, const uint256& blockHash) const
{
    AssertLockHeld(cs);

    if (!isEnabled) {
        return false;
    }

    if (bestChainLockBlockIndex == nullptr) {
        return false;
    }

    if (nHeight > bestChainLockBlockIndex->nHeight) {
        return false;
    }

    if (nHeight == bestChainLockBlockIndex->nHeight) {
        return blockHash == bestChainLockBlockIndex->GetBlockHash();
    }

    const auto* pAncestor = bestChainLockBlockIndex->GetAncestor(nHeight);
    return (pAncestor != nullptr) && pAncestor->GetBlockHash() == blockHash;
}

bool CChainLocksHandler::HasConflictingChainLock(int nHeight, const uint256& blockHash) const
{
    LOCK(cs);
    return InternalHasConflictingChainLock(nHeight, blockHash);
}

bool CChainLocksHandler::InternalHasConflictingChainLock(int nHeight, const uint256& blockHash) const
{
    AssertLockHeld(cs);

    if (!isEnabled) {
        return false;
    }

    if (bestChainLockBlockIndex == nullptr) {
        return false;
    }

    if (nHeight > bestChainLockBlockIndex->nHeight) {
        return false;
    }

    if (nHeight == bestChainLockBlockIndex->nHeight) {
        return blockHash != bestChainLockBlockIndex->GetBlockHash();
    }

    const auto* pAncestor = bestChainLockBlockIndex->GetAncestor(nHeight);
    assert(pAncestor);
    return pAncestor->GetBlockHash() != blockHash;
}

void CChainLocksHandler::Cleanup()
{
    if (!m_mn_sync.IsBlockchainSynced()) {
        return;
    }

    if (GetTimeMillis() - lastCleanupTime < CLEANUP_INTERVAL) {
        return;
    }

    // need mempool.cs due to GetTransaction calls
    LOCK2(cs_main, mempool.cs);
    LOCK(cs);

    for (auto it = seenChainLocks.begin(); it != seenChainLocks.end(); ) {
        if (GetTimeMillis() - it->second >= CLEANUP_SEEN_TIMEOUT) {
            it = seenChainLocks.erase(it);
        } else {
            ++it;
        }
    }

    for (auto it = blockTxs.begin(); it != blockTxs.end(); ) {
        const auto* pindex = m_chainstate.m_blockman.LookupBlockIndex(it->first);
        if (InternalHasChainLock(pindex->nHeight, pindex->GetBlockHash())) {
            for (const auto& txid : *it->second) {
                txFirstSeenTime.erase(txid);
            }
            it = blockTxs.erase(it);
        } else if (InternalHasConflictingChainLock(pindex->nHeight, pindex->GetBlockHash())) {
            it = blockTxs.erase(it);
        } else {
            ++it;
        }
    }
    for (auto it = txFirstSeenTime.begin(); it != txFirstSeenTime.end(); ) {
        uint256 hashBlock;
        CTransactionRef tx = GetTransaction(/* block_index */ nullptr, &mempool, it->first, Params().GetConsensus(), hashBlock);
        if (!tx) {
            // tx has vanished, probably due to conflicts
            it = txFirstSeenTime.erase(it);
        } else if (!hashBlock.IsNull()) {
            const auto* pindex = m_chainstate.m_blockman.LookupBlockIndex(hashBlock);
            if (m_chainstate.m_chain.Tip()->GetAncestor(pindex->nHeight) == pindex && m_chainstate.m_chain.Height() - pindex->nHeight >= 6) {
                // tx got confirmed >= 6 times, so we can stop keeping track of it
                it = txFirstSeenTime.erase(it);
            } else {
                ++it;
            }
        } else {
            ++it;
        }
    }

    lastCleanupTime = GetTimeMillis();
}

bool AreChainLocksEnabled(const CSporkManager& sporkManager)
{
    return sporkManager.IsSporkActive(SPORK_19_CHAINLOCKS_ENABLED);
}

bool ChainLocksSigningEnabled(const CSporkManager& sporkManager)
{
    return sporkManager.GetSporkValue(SPORK_19_CHAINLOCKS_ENABLED) == 0;
}

} // namespace llmq
