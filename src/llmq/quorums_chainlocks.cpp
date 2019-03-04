// Copyright (c) 2019 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "quorums.h"
#include "quorums_chainlocks.h"
#include "quorums_instantsend.h"
#include "quorums_signing.h"
#include "quorums_utils.h"

#include "chain.h"
#include "net_processing.h"
#include "scheduler.h"
#include "spork.h"
#include "txmempool.h"
#include "validation.h"

namespace llmq
{

static const std::string CLSIG_REQUESTID_PREFIX = "clsig";

CChainLocksHandler* chainLocksHandler;

std::string CChainLockSig::ToString() const
{
    return strprintf("CChainLockSig(nHeight=%d, blockHash=%s)", nHeight, blockHash.ToString());
}

CChainLocksHandler::CChainLocksHandler(CScheduler* _scheduler) :
    scheduler(_scheduler)
{
}

CChainLocksHandler::~CChainLocksHandler()
{
}

void CChainLocksHandler::Start()
{
    quorumSigningManager->RegisterRecoveredSigsListener(this);
    scheduler->scheduleEvery([&]() {
        // regularely retry signing the current chaintip as it might have failed before due to missing ixlocks
        TrySignChainTip();
    }, 5000);
}

void CChainLocksHandler::Stop()
{
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

void CChainLocksHandler::ProcessMessage(CNode* pfrom, const std::string& strCommand, CDataStream& vRecv, CConnman& connman)
{
    if (!sporkManager.IsSporkActive(SPORK_19_CHAINLOCKS_ENABLED)) {
        return;
    }

    if (strCommand == NetMsgType::CLSIG) {
        CChainLockSig clsig;
        vRecv >> clsig;

        auto hash = ::SerializeHash(clsig);

        ProcessNewChainLock(pfrom->id, clsig, hash);
    }
}

void CChainLocksHandler::ProcessNewChainLock(NodeId from, const llmq::CChainLockSig& clsig, const uint256& hash)
{
    {
        LOCK(cs_main);
        g_connman->RemoveAskFor(hash);
    }

    {
        LOCK(cs);
        if (!seenChainLocks.emplace(hash, GetTimeMillis()).second) {
            return;
        }

        if (bestChainLock.nHeight != -1 && clsig.nHeight <= bestChainLock.nHeight) {
            // no need to process/relay older CLSIGs
            return;
        }
    }

    uint256 requestId = ::SerializeHash(std::make_pair(CLSIG_REQUESTID_PREFIX, clsig.nHeight));
    uint256 msgHash = clsig.blockHash;
    if (!quorumSigningManager->VerifyRecoveredSig(Params().GetConsensus().llmqChainLocks, clsig.nHeight, requestId, msgHash, clsig.sig)) {
        LogPrintf("CChainLocksHandler::%s -- invalid CLSIG (%s), peer=%d\n", __func__, clsig.ToString(), from);
        if (from != -1) {
            LOCK(cs_main);
            Misbehaving(from, 10);
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
            return;
        }

        bestChainLockHash = hash;
        bestChainLock = clsig;

        CInv inv(MSG_CLSIG, hash);
        g_connman->RelayInv(inv);

        auto blockIt = mapBlockIndex.find(clsig.blockHash);
        if (blockIt == mapBlockIndex.end()) {
            // we don't know the block/header for this CLSIG yet, so bail out for now
            // when the block or the header later comes in, we will enforce the correct chain
            return;
        }

        if (blockIt->second->nHeight != clsig.nHeight) {
            // Should not happen, same as the conflict check from above.
            LogPrintf("CChainLocksHandler::%s -- height of CLSIG (%s) does not match the specified block's height (%d)\n",
                    __func__, clsig.ToString(), blockIt->second->nHeight);
            return;
        }

        const CBlockIndex* pindex = blockIt->second;
        bestChainLockWithKnownBlock = bestChainLock;
        bestChainLockBlockIndex = pindex;
    }

    EnforceBestChainLock();

    LogPrintf("CChainLocksHandler::%s -- processed new CLSIG (%s), peer=%d\n",
              __func__, clsig.ToString(), from);

    if (lastNotifyChainLockBlockIndex != bestChainLockBlockIndex) {
        lastNotifyChainLockBlockIndex = bestChainLockBlockIndex;
        GetMainSignals().NotifyChainLock(bestChainLockBlockIndex);
    }
}

void CChainLocksHandler::AcceptedBlockHeader(const CBlockIndex* pindexNew)
{
    bool doEnforce = false;
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

            bestChainLockBlockIndex = pindexNew;
            doEnforce = true;
        }
    }
    if (doEnforce) {
        EnforceBestChainLock();
    }
}

void CChainLocksHandler::UpdatedBlockTip(const CBlockIndex* pindexNew, const CBlockIndex* pindexFork)
{
    // don't call TrySignChainTip directly but instead let the scheduler call it. This way we ensure that cs_main is
    // never locked and TrySignChainTip is not called twice in parallel
    LOCK(cs);
    if (tryLockChainTipScheduled) {
        return;
    }
    tryLockChainTipScheduled = true;
    scheduler->scheduleFromNow([&]() {
        TrySignChainTip();
        LOCK(cs);
        tryLockChainTipScheduled = false;
    }, 0);
}

void CChainLocksHandler::TrySignChainTip()
{
    Cleanup();

    const CBlockIndex* pindex;
    {
        LOCK(cs_main);
        pindex = chainActive.Tip();
    }

    if (!fMasternodeMode) {
        return;
    }
    if (!pindex->pprev) {
        return;
    }
    if (!sporkManager.IsSporkActive(SPORK_19_CHAINLOCKS_ENABLED)) {
        return;
    }

    // DIP8 defines a process called "Signing attempts" which should run before the CLSIG is finalized
    // To simplify the initial implementation, we skip this process and directly try to create a CLSIG
    // This will fail when multiple blocks compete, but we accept this for the initial implementation.
    // Later, we'll add the multiple attempts process.

    {
        LOCK(cs);

        if (bestChainLockBlockIndex == pindex) {
            // we first got the CLSIG, then the header, and then the block was connected.
            // In this case there is no need to continue here.
            // However, NotifyChainLock might not have been called yet, so call it now if needed
            if (lastNotifyChainLockBlockIndex != bestChainLockBlockIndex) {
                lastNotifyChainLockBlockIndex = bestChainLockBlockIndex;
                GetMainSignals().NotifyChainLock(bestChainLockBlockIndex);
            }
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
            if (!inEnforceBestChainLock) {
                // we accepted this block when there was no lock yet, but now a conflicting lock appeared. Invalidate it.
                LogPrintf("CChainLocksHandler::%s -- conflicting lock after block was accepted, invalidating now\n",
                          __func__);
                ScheduleInvalidateBlock(pindex);
            }
            return;
        }
    }

    LogPrintf("CChainLocksHandler::%s -- trying to sign %s, height=%d\n", __func__, pindex->GetBlockHash().ToString(), pindex->nHeight);

    // When the new IX system is activated, we only try to ChainLock blocks which include safe transactions. A TX is
    // considered safe when it is ixlocked or at least known since 10 minutes (from mempool or block). These checks are
    // performed for the tip (which we try to sign) and the previous 5 blocks. If a ChainLocked block is found on the
    // way down, we consider all TXs to be safe.
    if (IsNewInstantSendEnabled() && sporkManager.IsSporkActive(SPORK_3_INSTANTSEND_BLOCK_FILTERING)) {
        auto pindexWalk = pindex;
        while (pindexWalk) {
            if (pindex->nHeight - pindexWalk->nHeight > 5) {
                // no need to check further down, 6 confs is safe to assume that TXs below this height won't be
                // ixlocked anymore if they aren't already
                LogPrintf("CChainLocksHandler::%s -- tip and previous 5 blocks all safe\n", __func__);
                break;
            }
            if (HasChainLock(pindexWalk->nHeight, pindexWalk->GetBlockHash())) {
                // we don't care about ixlocks for TXs that are ChainLocked already
                LogPrintf("CChainLocksHandler::%s -- chainlock at height %d \n", __func__, pindexWalk->nHeight);
                break;
            }

            decltype(blockTxs.begin()->second) txids;
            {
                LOCK(cs);
                auto it = blockTxs.find(pindexWalk->GetBlockHash());
                if (it == blockTxs.end()) {
                    // this should actually not happen as NewPoWValidBlock should have been called before
                    LogPrintf("CChainLocksHandler::%s -- blockTxs for %s not found\n", __func__,
                              pindexWalk->GetBlockHash().ToString());
                    return;
                }
                txids = it->second;
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

                if (txAge < WAIT_FOR_IXLOCK_TIMEOUT && !quorumInstantSendManager->IsLocked(txid)) {
                    LogPrintf("CChainLocksHandler::%s -- not signing block %s due to TX %s not being ixlocked and not old enough. age=%d\n", __func__,
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

    quorumSigningManager->AsyncSignIfMember(Params().GetConsensus().llmqChainLocks, requestId, msgHash);
}

void CChainLocksHandler::NewPoWValidBlock(const CBlockIndex* pindex, const std::shared_ptr<const CBlock>& block)
{
    LOCK(cs);
    if (blockTxs.count(pindex->GetBlockHash())) {
        // should actually not happen (blocks are only written once to disk and this is when NewPoWValidBlock is called)
        // but be extra safe here in case this behaviour changes.
        return;
    }

    int64_t curTime = GetAdjustedTime();

    // We listen for NewPoWValidBlock so that we can collect all TX ids of all included TXs of newly received blocks
    // We need this information later when we try to sign a new tip, so that we can determine if all included TXs are
    // safe.

    auto txs = std::make_shared<std::unordered_set<uint256, StaticSaltedHasher>>();
    for (const auto& tx : block->vtx) {
        if (tx->nVersion == 3) {
            if (tx->nType == TRANSACTION_COINBASE ||
                tx->nType == TRANSACTION_QUORUM_COMMITMENT) {
                continue;
            }
        }
        txs->emplace(tx->GetHash());
        txFirstSeenTime.emplace(tx->GetHash(), curTime);
    }
    blockTxs[pindex->GetBlockHash()] = txs;
}

void CChainLocksHandler::SyncTransaction(const CTransaction& tx, const CBlockIndex* pindex, int posInBlock)
{
    if (tx.nVersion == 3) {
        if (tx.nType == TRANSACTION_COINBASE ||
            tx.nType == TRANSACTION_QUORUM_COMMITMENT) {
            return;
        }
    }

    LOCK(cs);
    int64_t curTime = GetAdjustedTime();
    txFirstSeenTime.emplace(tx.GetHash(), curTime);
}

bool CChainLocksHandler::IsTxSafeForMining(const uint256& txid)
{
    if (!sporkManager.IsSporkActive(SPORK_19_CHAINLOCKS_ENABLED) || !sporkManager.IsSporkActive(SPORK_3_INSTANTSEND_BLOCK_FILTERING)) {
        return true;
    }
    if (!IsNewInstantSendEnabled()) {
        return true;
    }

    int64_t txAge = 0;
    {
        LOCK(cs);
        auto it = txFirstSeenTime.find(txid);
        if (it != txFirstSeenTime.end()) {
            txAge = GetAdjustedTime() - it->second;
        }
    }

    if (txAge < WAIT_FOR_IXLOCK_TIMEOUT && !quorumInstantSendManager->IsLocked(txid)) {
        return false;
    }
    return true;
}

// WARNING: cs_main and cs should not be held!
void CChainLocksHandler::EnforceBestChainLock()
{
    CChainLockSig clsig;
    const CBlockIndex* pindex;
    {
        LOCK(cs);
        clsig = bestChainLockWithKnownBlock;
        pindex = bestChainLockBlockIndex;
    }

    {
        LOCK(cs_main);

        // Go backwards through the chain referenced by clsig until we find a block that is part of the main chain.
        // For each of these blocks, check if there are children that are NOT part of the chain referenced by clsig
        // and invalidate each of them.
        inEnforceBestChainLock = true; // avoid unnecessary ScheduleInvalidateBlock calls inside UpdatedBlockTip
        while (pindex && !chainActive.Contains(pindex)) {
            // Invalidate all blocks that have the same prevBlockHash but are not equal to blockHash
            auto itp = mapPrevBlockIndex.equal_range(pindex->pprev->GetBlockHash());
            for (auto jt = itp.first; jt != itp.second; ++jt) {
                if (jt->second == pindex) {
                    continue;
                }
                LogPrintf("CChainLocksHandler::%s -- CLSIG (%s) invalidates block %s\n",
                          __func__, bestChainLockWithKnownBlock.ToString(), jt->second->GetBlockHash().ToString());
                DoInvalidateBlock(jt->second, false);
            }

            pindex = pindex->pprev;
        }
        inEnforceBestChainLock = false;
    }

    CValidationState state;
    if (!ActivateBestChain(state, Params())) {
        LogPrintf("CChainLocksHandler::UpdatedBlockTip -- ActivateBestChain failed: %s\n", FormatStateMessage(state));
        // This should not have happened and we are in a state were it's not safe to continue anymore
        assert(false);
    }
}

void CChainLocksHandler::HandleNewRecoveredSig(const llmq::CRecoveredSig& recoveredSig)
{
    if (!sporkManager.IsSporkActive(SPORK_19_CHAINLOCKS_ENABLED)) {
        return;
    }

    CChainLockSig clsig;
    {
        LOCK(cs);

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
        clsig.sig = recoveredSig.sig;
    }
    ProcessNewChainLock(-1, clsig, ::SerializeHash(clsig));
}

void CChainLocksHandler::ScheduleInvalidateBlock(const CBlockIndex* pindex)
{
    // Calls to InvalidateBlock and ActivateBestChain might result in re-invocation of the UpdatedBlockTip and other
    // signals, so we can't directly call it from signal handlers. We solve this by doing the call from the scheduler

    scheduler->scheduleFromNow([this, pindex]() {
        DoInvalidateBlock(pindex, true);
    }, 0);
}

// WARNING, do not hold cs while calling this method as we'll otherwise run into a deadlock
void CChainLocksHandler::DoInvalidateBlock(const CBlockIndex* pindex, bool activateBestChain)
{
    auto& params = Params();

    {
        LOCK(cs_main);

        // get the non-const pointer
        CBlockIndex* pindex2 = mapBlockIndex[pindex->GetBlockHash()];

        CValidationState state;
        if (!InvalidateBlock(state, params, pindex2)) {
            LogPrintf("CChainLocksHandler::UpdatedBlockTip -- InvalidateBlock failed: %s\n", FormatStateMessage(state));
            // This should not have happened and we are in a state were it's not safe to continue anymore
            assert(false);
        }
    }

    CValidationState state;
    if (activateBestChain && !ActivateBestChain(state, params)) {
        LogPrintf("CChainLocksHandler::UpdatedBlockTip -- ActivateBestChain failed: %s\n", FormatStateMessage(state));
        // This should not have happened and we are in a state were it's not safe to continue anymore
        assert(false);
    }
}

bool CChainLocksHandler::HasChainLock(int nHeight, const uint256& blockHash)
{
    if (!sporkManager.IsSporkActive(SPORK_19_CHAINLOCKS_ENABLED)) {
        return false;
    }

    LOCK(cs);
    return InternalHasChainLock(nHeight, blockHash);
}

bool CChainLocksHandler::InternalHasChainLock(int nHeight, const uint256& blockHash)
{
    AssertLockHeld(cs);

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
    if (!sporkManager.IsSporkActive(SPORK_19_CHAINLOCKS_ENABLED)) {
        return false;
    }

    LOCK(cs);
    return InternalHasConflictingChainLock(nHeight, blockHash);
}

bool CChainLocksHandler::InternalHasConflictingChainLock(int nHeight, const uint256& blockHash)
{
    AssertLockHeld(cs);

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
        auto pindex = mapBlockIndex.at(it->first);
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
            auto pindex = mapBlockIndex.at(hashBlock);
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

}
