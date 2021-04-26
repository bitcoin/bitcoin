// Copyright (c) 2019-2021 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <llmq/quorums_chainlocks.h>
#include <llmq/quorums.h>
#include <llmq/quorums_instantsend.h>
#include <llmq/quorums_utils.h>

#include <chain.h>
#include <consensus/validation.h>
#include <masternode/masternode-sync.h>
#include <net_processing.h>
#include <scheduler.h>
#include <spork.h>
#include <txmempool.h>

namespace llmq
{

const std::string CLSIG_REQUESTID_PREFIX = "clsig";

CChainLocksHandler* chainLocksHandler;

bool CChainLockSig::IsNull() const
{
    return nHeight == -1 && blockHash == uint256();
}

std::string CChainLockSig::ToString() const
{
    return strprintf("CChainLockSig(nHeight=%d, blockHash=%s)", nHeight, blockHash.ToString());
}

CChainLocksHandler::CChainLocksHandler()
{
    scheduler = new CScheduler();
    CScheduler::Function serviceLoop = boost::bind(&CScheduler::serviceQueue, scheduler);
    scheduler_thread = new boost::thread(boost::bind(&TraceThread<CScheduler::Function>, "cl-schdlr", serviceLoop));
}

CChainLocksHandler::~CChainLocksHandler()
{
    scheduler_thread->interrupt();
    scheduler_thread->join();
    delete scheduler_thread;
    delete scheduler;
}

void CChainLocksHandler::Start()
{
    quorumSigningManager->RegisterRecoveredSigsListener(this);
    scheduler->scheduleEvery([&]() {
        CheckActiveState();
        EnforceBestChainLock();
        // regularly retry signing the current chaintip as it might have failed before due to missing islocks
        TrySignChainTip();
    }, 5000);
}

void CChainLocksHandler::Stop()
{
    scheduler->stop();
    quorumSigningManager->UnregisterRecoveredSigsListener(this);
}

bool CChainLocksHandler::AlreadyHave(const CInv& inv)
{
    LOCK(cs);
    return seenChainLocks.count(inv.hash) != 0;
}

bool CChainLocksHandler::GetChainLockByHash(const uint256& hash, llmq::CChainLockSig& ret)
{
    LOCK(cs);

    if (hash != bestChainLockHash) {
        // we only propagate the best one and ditch all the old ones
        return false;
    }

    ret = bestChainLock;
    return true;
}

CChainLockSig CChainLocksHandler::GetBestChainLock()
{
    LOCK(cs);
    return bestChainLock;
}

void CChainLocksHandler::ProcessMessage(CNode* pfrom, const std::string& strCommand, CDataStream& vRecv)
{
    if (!AreChainLocksEnabled()) {
        return;
    }

    if (strCommand == NetMsgType::CLSIG) {
        CChainLockSig clsig;
        vRecv >> clsig;

        auto hash = ::SerializeHash(clsig);

        ProcessNewChainLock(pfrom->GetId(), clsig, hash);
    }
}

void CChainLocksHandler::ProcessNewChainLock(const NodeId from, const llmq::CChainLockSig& clsig, const uint256& hash)
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
            return;
        }

        if (!bestChainLock.IsNull() && clsig.nHeight <= bestChainLock.nHeight) {
            // no need to process/relay older CLSIGs
            return;
        }
    }

    const uint256 requestId = ::SerializeHash(std::make_pair(CLSIG_REQUESTID_PREFIX, clsig.nHeight));
    if (!quorumSigningManager->VerifyRecoveredSig(Params().GetConsensus().llmqTypeChainLocks, clsig.nHeight, requestId, clsig.blockHash, clsig.sig)) {
        LogPrint(BCLog::CHAINLOCKS, "CChainLocksHandler::%s -- invalid CLSIG (%s), peer=%d\n", __func__, clsig.ToString(), from);
        if (from != -1) {
            LOCK(cs_main);
            Misbehaving(from, 10);
        }
        return;
    }

    CBlockIndex* pindex;
    {
        LOCK(cs_main);
        pindex = LookupBlockIndex(clsig.blockHash);
    }

    {
        LOCK(cs);
        bestChainLockHash = hash;
        bestChainLock = clsig;

        if (pindex != nullptr) {

            if (pindex->nHeight != clsig.nHeight) {
                // Should not happen, same as the conflict check from above.
                LogPrintf("CChainLocksHandler::%s -- height of CLSIG (%s) does not match the specified block's height (%d)\n",
                        __func__, clsig.ToString(), pindex->nHeight);
                // Note: not relaying clsig here
                return;
            }

            bestChainLockWithKnownBlock = bestChainLock;
            bestChainLockBlockIndex = pindex;
        }
        // else if (pindex == nullptr)
        // Note: make sure to still relay clsig further.
    }

    // Note: do not hold cs while calling RelayInv
    AssertLockNotHeld(cs);
    g_connman->RelayInv(clsigInv, LLMQS_PROTO_VERSION);

    if (pindex == nullptr) {
        // we don't know the block/header for this CLSIG yet, so bail out for now
        // when the block or the header later comes in, we will enforce the correct chain
        return;
    }

    scheduler->scheduleFromNow([&]() {
        CheckActiveState();
        EnforceBestChainLock();
    }, 0);

    LogPrint(BCLog::CHAINLOCKS, "CChainLocksHandler::%s -- processed new CLSIG (%s), peer=%d\n",
              __func__, clsig.ToString(), from);
}

void CChainLocksHandler::AcceptedBlockHeader(const CBlockIndex* pindexNew)
{
    LOCK(cs);

    if (pindexNew->GetBlockHash() == bestChainLock.blockHash) {
        LogPrint(BCLog::CHAINLOCKS, "CChainLocksHandler::%s -- block header %s came in late, updating and enforcing\n", __func__, pindexNew->GetBlockHash().ToString());

        if (bestChainLock.nHeight != pindexNew->nHeight) {
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

void CChainLocksHandler::UpdatedBlockTip(const CBlockIndex* pindexNew)
{
    // don't call TrySignChainTip directly but instead let the scheduler call it. This way we ensure that cs_main is
    // never locked and TrySignChainTip is not called twice in parallel. Also avoids recursive calls due to
    // EnforceBestChainLock switching chains.
    LOCK(cs);
    if (tryLockChainTipScheduled) {
        return;
    }
    tryLockChainTipScheduled = true;
    scheduler->scheduleFromNow([&]() {
        CheckActiveState();
        EnforceBestChainLock();
        TrySignChainTip();
        LOCK(cs);
        tryLockChainTipScheduled = false;
    }, 0);
}

void CChainLocksHandler::CheckActiveState()
{
    bool fDIP0008Active;
    {
        LOCK(cs_main);
        fDIP0008Active = chainActive.Tip() && chainActive.Tip()->pprev && chainActive.Tip()->pprev->nHeight >= Params().GetConsensus().DIP0008Height;
    }

    LOCK(cs);
    bool oldIsEnforced = isEnforced;
    isEnabled = AreChainLocksEnabled();
    isEnforced = (fDIP0008Active && isEnabled);

    if (!oldIsEnforced && isEnforced) {
        // ChainLocks got activated just recently, but it's possible that it was already running before, leaving
        // us with some stale values which we should not try to enforce anymore (there probably was a good reason
        // to disable spork19)
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

    if (!masternodeSync.IsBlockchainSynced()) {
        return;
    }

    const CBlockIndex* pindex;
    {
        LOCK(cs_main);
        pindex = chainActive.Tip();
    }

    if (!pindex->pprev) {
        return;
    }

    // DIP8 defines a process called "Signing attempts" which should run before the CLSIG is finalized
    // To simplify the initial implementation, we skip this process and directly try to create a CLSIG
    // This will fail when multiple blocks compete, but we accept this for the initial implementation.
    // Later, we'll add the multiple attempts process.

    {
        LOCK(cs);

        if (!isEnabled) {
            return;
        }

        if (pindex->nHeight == lastSignedHeight) {
            // already signed this one
            return;
        }

        if (bestChainLock.nHeight >= pindex->nHeight) {
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
    if (IsInstantSendEnabled() && RejectConflictingBlocks()) {
        auto pindexWalk = pindex;
        while (pindexWalk) {
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

            for (auto& txid : *txids) {
                int64_t txAge = 0;
                {
                    LOCK(cs);
                    auto it = txFirstSeenTime.find(txid);
                    if (it != txFirstSeenTime.end()) {
                        txAge = GetAdjustedTime() - it->second;
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
        if (bestChainLock.nHeight >= pindex->nHeight) {
            // might have happened while we didn't hold cs
            return;
        }
        lastSignedHeight = pindex->nHeight;
        lastSignedRequestId = requestId;
        lastSignedMsgHash = msgHash;
    }

    quorumSigningManager->AsyncSignIfMember(Params().GetConsensus().llmqTypeChainLocks, requestId, msgHash);
}

void CChainLocksHandler::TransactionAddedToMempool(const CTransactionRef& tx, int64_t nAcceptTime)
{
    if (tx->IsCoinBase() || tx->vin.empty()) {
        return;
    }

    LOCK(cs);
    txFirstSeenTime.emplace(tx->GetHash(), nAcceptTime);
}

void CChainLocksHandler::BlockConnected(const std::shared_ptr<const CBlock>& pblock, const CBlockIndex* pindex, const std::vector<CTransactionRef>& vtxConflicted)
{
    if (!masternodeSync.IsBlockchainSynced()) {
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

    int64_t curTime = GetAdjustedTime();

    for (const auto& tx : pblock->vtx) {
        if (tx->IsCoinBase() || tx->vin.empty()) {
            continue;
        }

        txids.emplace(tx->GetHash());
        txFirstSeenTime.emplace(tx->GetHash(), curTime);
    }

}

void CChainLocksHandler::BlockDisconnected(const std::shared_ptr<const CBlock>& pblock, const CBlockIndex* pindexDisconnected)
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
            auto pindex = LookupBlockIndex(blockHash);
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
        for (auto& txid : *ret) {
            txFirstSeenTime.emplace(txid, blockTime);
        }
    }
    return ret;
}

bool CChainLocksHandler::IsTxSafeForMining(const uint256& txid)
{
    if (!RejectConflictingBlocks()) {
        return true;
    }
    if (!IsInstantSendEnabled()) {
        return true;
    }

    int64_t txAge = 0;
    {
        LOCK(cs);
        if (!isEnabled || !isEnforced) {
            return true;
        }
        auto it = txFirstSeenTime.find(txid);
        if (it != txFirstSeenTime.end()) {
            txAge = GetAdjustedTime() - it->second;
        }
    }

    if (txAge < WAIT_FOR_ISLOCK_TIMEOUT && !quorumInstantSendManager->IsLocked(txid)) {
        return false;
    }
    return true;
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

        if (!isEnforced) {
            return;
        }

        clsig = std::make_shared<CChainLockSig>(bestChainLockWithKnownBlock);
        pindex = currentBestChainLockBlockIndex = this->bestChainLockBlockIndex;

        if (!currentBestChainLockBlockIndex) {
            // we don't have the header/block, so we can't do anything right now
            return;
        }
    }

    bool activateNeeded;
    CValidationState state;
    const auto &params = Params();
    {
        LOCK(cs_main);

        // Go backwards through the chain referenced by clsig until we find a block that is part of the main chain.
        // For each of these blocks, check if there are children that are NOT part of the chain referenced by clsig
        // and mark all of them as conflicting.
        while (pindex && !chainActive.Contains(pindex)) {
            // Mark all blocks that have the same prevBlockHash but are not equal to blockHash as conflicting
            auto itp = mapPrevBlockIndex.equal_range(pindex->pprev->GetBlockHash());
            for (auto jt = itp.first; jt != itp.second; ++jt) {
                if (jt->second == pindex) {
                    continue;
                }
                if (!MarkConflictingBlock(state, params, jt->second)) {
                    LogPrintf("CChainLocksHandler::%s -- MarkConflictingBlock failed: %s\n", __func__, FormatStateMessage(state));
                    // This should not have happened and we are in a state were it's not safe to continue anymore
                    assert(false);
                }
                LogPrintf("CChainLocksHandler::%s -- CLSIG (%s) marked block %s as conflicting\n",
                          __func__, clsig->ToString(), jt->second->GetBlockHash().ToString());
            }

            pindex = pindex->pprev;
        }
        // In case blocks from the correct chain are invalid at the moment, reconsider them. The only case where this
        // can happen right now is when missing superblock triggers caused the main chain to be dismissed first. When
        // the trigger later appears, this should bring us to the correct chain eventually. Please note that this does
        // NOT enforce invalid blocks in any way, it just causes re-validation.
        if (!currentBestChainLockBlockIndex->IsValid()) {
            ResetBlockFailureFlags(LookupBlockIndex(currentBestChainLockBlockIndex->GetBlockHash()));
        }

        activateNeeded = chainActive.Tip()->GetAncestor(currentBestChainLockBlockIndex->nHeight) != currentBestChainLockBlockIndex;
    }

    if (activateNeeded && !ActivateBestChain(state, params)) {
        LogPrintf("CChainLocksHandler::%s -- ActivateBestChain failed: %s\n", __func__, FormatStateMessage(state));
    }

    const CBlockIndex* pindexNotify = nullptr;
    {
        LOCK(cs_main);
        if (lastNotifyChainLockBlockIndex != currentBestChainLockBlockIndex &&
            chainActive.Tip()->GetAncestor(currentBestChainLockBlockIndex->nHeight) == currentBestChainLockBlockIndex) {
            lastNotifyChainLockBlockIndex = currentBestChainLockBlockIndex;
            pindexNotify = currentBestChainLockBlockIndex;
        }
    }

    if (pindexNotify) {
        GetMainSignals().NotifyChainLock(pindexNotify, clsig);
    }
}

void CChainLocksHandler::HandleNewRecoveredSig(const llmq::CRecoveredSig& recoveredSig)
{
    CChainLockSig clsig;
    {
        LOCK(cs);

        if (!isEnabled) {
            return;
        }

        if (recoveredSig.id != lastSignedRequestId || recoveredSig.msgHash != lastSignedMsgHash) {
            // this is not what we signed, so lets not create a CLSIG for it
            return;
        }
        if (bestChainLock.nHeight >= lastSignedHeight) {
            // already got the same or a better CLSIG through the CLSIG message
            return;
        }

        clsig.nHeight = lastSignedHeight;
        clsig.blockHash = lastSignedMsgHash;
        clsig.sig = recoveredSig.sig.Get();
    }
    ProcessNewChainLock(-1, clsig, ::SerializeHash(clsig));
}

bool CChainLocksHandler::HasChainLock(int nHeight, const uint256& blockHash)
{
    LOCK(cs);
    return InternalHasChainLock(nHeight, blockHash);
}

bool CChainLocksHandler::InternalHasChainLock(int nHeight, const uint256& blockHash)
{
    AssertLockHeld(cs);

    if (!isEnforced) {
        return false;
    }

    if (!bestChainLockBlockIndex) {
        return false;
    }

    if (nHeight > bestChainLockBlockIndex->nHeight) {
        return false;
    }

    if (nHeight == bestChainLockBlockIndex->nHeight) {
        return blockHash == bestChainLockBlockIndex->GetBlockHash();
    }

    auto pAncestor = bestChainLockBlockIndex->GetAncestor(nHeight);
    return pAncestor && pAncestor->GetBlockHash() == blockHash;
}

bool CChainLocksHandler::HasConflictingChainLock(int nHeight, const uint256& blockHash)
{
    LOCK(cs);
    return InternalHasConflictingChainLock(nHeight, blockHash);
}

bool CChainLocksHandler::InternalHasConflictingChainLock(int nHeight, const uint256& blockHash)
{
    AssertLockHeld(cs);

    if (!isEnforced) {
        return false;
    }

    if (!bestChainLockBlockIndex) {
        return false;
    }

    if (nHeight > bestChainLockBlockIndex->nHeight) {
        return false;
    }

    if (nHeight == bestChainLockBlockIndex->nHeight) {
        return blockHash != bestChainLockBlockIndex->GetBlockHash();
    }

    auto pAncestor = bestChainLockBlockIndex->GetAncestor(nHeight);
    assert(pAncestor);
    return pAncestor->GetBlockHash() != blockHash;
}

void CChainLocksHandler::Cleanup()
{
    if (!masternodeSync.IsBlockchainSynced()) {
        return;
    }

    {
        LOCK(cs);
        if (GetTimeMillis() - lastCleanupTime < CLEANUP_INTERVAL) {
            return;
        }
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
        auto pindex = LookupBlockIndex(it->first);
        if (InternalHasChainLock(pindex->nHeight, pindex->GetBlockHash())) {
            for (auto& txid : *it->second) {
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
        CTransactionRef tx;
        uint256 hashBlock;
        if (!GetTransaction(it->first, tx, Params().GetConsensus(), hashBlock)) {
            // tx has vanished, probably due to conflicts
            it = txFirstSeenTime.erase(it);
        } else if (!hashBlock.IsNull()) {
            auto pindex = LookupBlockIndex(hashBlock);
            if (chainActive.Tip()->GetAncestor(pindex->nHeight) == pindex && chainActive.Height() - pindex->nHeight >= 6) {
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

bool AreChainLocksEnabled()
{
    return sporkManager.IsSporkActive(SPORK_19_CHAINLOCKS_ENABLED);
}

} // namespace llmq
