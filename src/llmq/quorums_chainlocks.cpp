// Copyright (c) 2019 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <llmq/quorums_chainlocks.h>
#include <llmq/quorums_utils.h>

#include <chain.h>
#include <masternode/masternodesync.h>
#include <net_processing.h>
#include <scheduler.h>
#include <spork.h>
#include <txmempool.h>
#include <validation.h>

namespace llmq
{

static const std::string CLSIG_REQUESTID_PREFIX = "clsig";

CChainLocksHandler* chainLocksHandler;

bool CChainLockSig::IsNull() const
{
    return nHeight == -1 && blockHash == uint256();
}

std::string CChainLockSig::ToString() const
{
    return strprintf("CChainLockSig(nHeight=%d, blockHash=%s)", nHeight, blockHash.ToString());
}

CChainLocksHandler::CChainLocksHandler(CConnman& _connman, PeerManager& _peerman):     
    connman(_connman),
    peerman(_peerman)
{
    scheduler = new CScheduler();
    CScheduler::Function serviceLoop = boost::bind(&CScheduler::serviceQueue, scheduler);
    scheduler_thread = new boost::thread(boost::bind(&TraceThread<CScheduler::Function>, "cl-scheduler", serviceLoop));
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
    }, std::chrono::milliseconds(5000));
}

void CChainLocksHandler::Stop()
{
    scheduler->stop();
    quorumSigningManager->UnregisterRecoveredSigsListener(this);
}

bool CChainLocksHandler::AlreadyHave(const uint256& hash)
{
    LOCK(cs);
    return seenChainLocks.count(hash) != 0;
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
    if (!sporkManager.IsSporkActive(SPORK_19_CHAINLOCKS_ENABLED)) {
        return;
    }

    if (strCommand == NetMsgType::CLSIG) {
        CChainLockSig clsig;
        vRecv >> clsig;

        auto hash = ::SerializeHash(clsig);

        ProcessNewChainLock(pfrom->GetId(), clsig, hash);
    }
}

void CChainLocksHandler::ProcessNewChainLock(NodeId from, const llmq::CChainLockSig& clsig, const uint256&hash )
{
    if (from != -1) {
        LOCK(cs_main);
        peerman.ReceivedResponse(from, hash);
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

    uint256 requestId = ::SerializeHash(std::make_pair(CLSIG_REQUESTID_PREFIX, clsig.nHeight));
    uint256 msgHash = clsig.blockHash;
    if (!quorumSigningManager->VerifyRecoveredSig(Params().GetConsensus().llmqTypeChainLocks, clsig.nHeight, requestId, msgHash, clsig.sig)) {
        LogPrint(BCLog::CHAINLOCKS, "CChainLocksHandler::%s -- invalid CLSIG (%s), peer=%d\n", __func__, clsig.ToString(), from);
        if (from != -1) {
            LOCK(cs_main);
            peerman.ForgetTxHash(from, hash);
            Misbehaving(from, 10, "invalid CLSIG");
        }
        return;
    }

    {
        LOCK2(cs_main, cs);

        if (InternalHasConflictingChainLock(clsig.nHeight, clsig.blockHash)) {
            // This should not happen. If it happens, it means that a malicious entity controls a large part of the MN
            // network. In this case, we don't allow him to reorg older chainlocks.
            LogPrintf("CChainLocksHandler::%s -- new CLSIG (%s) tries to reorg previous CLSIG (%s), peer=%d\n",
                      __func__, clsig.ToString(), bestChainLock.ToString(), from);
            peerman.ForgetTxHash(from, hash);
            return;
        }

        bestChainLockHash = hash;
        bestChainLock = clsig;

        CInv inv(MSG_CLSIG, hash);
        connman.RelayOtherInv(inv);
        const CBlockIndex* pindex = LookupBlockIndex(clsig.blockHash);
        if (pindex == nullptr) {
            // we don't know the block/header for this CLSIG yet, so bail out for now
            // when the block or the header later comes in, we will enforce the correct chain
            return;
        }

        if (pindex->nHeight != clsig.nHeight) {
            // Should not happen, same as the conflict check from above.
            LogPrintf("CChainLocksHandler::%s -- height of CLSIG (%s) does not match the specified block's height (%d)\n",
                    __func__, clsig.ToString(), pindex->nHeight);
            return;
        }

        bestChainLockWithKnownBlock = bestChainLock;
        bestChainLockBlockIndex = pindex;
        peerman.ForgetTxHash(from, hash);
    }

    scheduler->scheduleFromNow([&]() {
        CheckActiveState();
        EnforceBestChainLock();
    }, std::chrono::milliseconds(0));

    LogPrint(BCLog::CHAINLOCKS, "CChainLocksHandler::%s -- processed new CLSIG (%s), peer=%d\n",
              __func__, clsig.ToString(), from);
}

void CChainLocksHandler::AcceptedBlockHeader(const CBlockIndex* pindexNew)
{
    LOCK2(cs_main, cs);

    if (pindexNew->GetBlockHash() == bestChainLock.blockHash) {
        LogPrintf("CChainLocksHandler::%s -- block header %s came in late, updating and enforcing\n", __func__, pindexNew->GetBlockHash().ToString());

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
    }, std::chrono::milliseconds(0));
}

void CChainLocksHandler::CheckActiveState()
{

    LOCK(cs);
    bool oldIsEnforced = isEnforced;
    isSporkActive = sporkManager.IsSporkActive(SPORK_19_CHAINLOCKS_ENABLED);
    isEnforced = isSporkActive;

    if (!oldIsEnforced && isEnforced) {
        // ChainLocks got activated just recently, but it's possible that it was already running before, leaving
        // us with some stale values which we should not try to enforce anymore (there probably was a good reason
        // to disable spork19)
        bestChainLockHash = uint256();
        bestChainLock = bestChainLockWithKnownBlock = CChainLockSig();
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
        pindex = ::ChainActive().Tip();
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

        if (!isSporkActive) {
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


// This should also not be called from validation signals, as this might result in recursive calls
void CChainLocksHandler::EnforceBestChainLock()
{
    const CBlockIndex* currentBestChainLockBlockIndex;
    std::vector<const CBlockIndex*> invalidatingBlockIndexes;
    {
        LOCK2(cs_main, cs);
        CChainLockSig clsig;
        const CBlockIndex* pindex;
        if (!isEnforced) {
            return;
        }

        clsig = bestChainLockWithKnownBlock;
        pindex = currentBestChainLockBlockIndex = this->bestChainLockBlockIndex;

        if (!currentBestChainLockBlockIndex) {
            // we don't have the header/block, so we can't do anything right now
            return;
        }
        // Go backwards through the chain referenced by clsig until we find a block that is part of the main chain.
        // For each of these blocks, check if there are children that are NOT part of the chain referenced by clsig
        // and invalidate each of them.
        while (pindex && !::ChainActive().Contains(pindex)) {
            // Invalidate all blocks that have the same prevBlockHash but are not equal to blockHash
            auto itp = LookupBlockIndexPrev(pindex->pprev->GetBlockHash());
            for (auto jt = itp.first; jt != itp.second; ++jt) {
                if (jt->second == pindex) {
                    continue;
                }
                LogPrintf("CChainLocksHandler::%s -- CLSIG (%s) invalidates block %s\n",
                            __func__, clsig.ToString(), jt->second->GetBlockHash().ToString());
                invalidatingBlockIndexes.emplace_back(jt->second); 
            }

            pindex = pindex->pprev;
        }
    }
    // no cs_main allowed
    for(const auto& index: invalidatingBlockIndexes) {
        DoInvalidateBlock(index);
    }
    bool activateNeeded;
    {
        LOCK(cs_main);
        // In case blocks from the correct chain are invalid at the moment, reconsider them. The only case where this
        // can happen right now is when missing superblock triggers caused the main chain to be dismissed first. When
        // the trigger later appears, this should bring us to the correct chain eventually. Please note that this does
        // NOT enforce invalid blocks in any way, it just causes re-validation.
        if (!currentBestChainLockBlockIndex->IsValid()) {
            ResetBlockFailureFlags(LookupBlockIndex(currentBestChainLockBlockIndex->GetBlockHash()));
        }
        activateNeeded = ::ChainActive().Tip()->GetAncestor(currentBestChainLockBlockIndex->nHeight) != currentBestChainLockBlockIndex;
    }
    // no cs_main allowed
    BlockValidationState state;
    if (activateNeeded && !ActivateBestChain(state, Params(), nullptr)) {
        LogPrintf("CChainLocksHandler::%s -- ActivateBestChain failed: %s\n", __func__, state.ToString());
    }
}

void CChainLocksHandler::HandleNewRecoveredSig(const llmq::CRecoveredSig& recoveredSig)
{
    CChainLockSig clsig;
    {
        LOCK(cs);

        if (!isSporkActive) {
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

// WARNING, do not hold cs while calling this method as we'll otherwise run into a deadlock
void CChainLocksHandler::DoInvalidateBlock(const CBlockIndex* pindex)
{
    auto& params = Params();
    // get the non-const pointer
    CBlockIndex* pindex2;
    {
        LOCK(cs_main);
        pindex2 = LookupBlockIndex(pindex->GetBlockHash());
    }
    BlockValidationState state;
    if (!InvalidateBlock(state, params, pindex2)) {
        LogPrintf("CChainLocksHandler::%s -- InvalidateBlock failed: %s\n", __func__, state.ToString());
        // This should not have happened and we are in a state were it's not safe to continue anymore
        assert(false);
    }
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

    LOCK(cs);

    for (auto it = seenChainLocks.begin(); it != seenChainLocks.end(); ) {
        if (GetTimeMillis() - it->second >= CLEANUP_SEEN_TIMEOUT) {
            it = seenChainLocks.erase(it);
        } else {
            ++it;
        }
    }

    lastCleanupTime = GetTimeMillis();
}

} // namespace llmq
