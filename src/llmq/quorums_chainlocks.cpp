// Copyright (c) 2019 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <llmq/quorums_chainlocks.h>
#include <llmq/quorums_utils.h>

#include <chain.h>
#include <masternode/masternodesync.h>
#include <net_processing.h>
#include <spork.h>
#include <txmempool.h>
#include <validation.h>

namespace llmq
{

static const std::string CLSIG_REQUESTID_PREFIX = "clsig";

// Used for iterating while signing/verifying CLSIGs by non-default quorums
struct RequestIdStep
{
    int nHeight{-1};
    int nStep{0};

    RequestIdStep(int _nHeight) : nHeight(_nHeight) {}

    SERIALIZE_METHODS(RequestIdStep, obj)
    {
        READWRITE(CLSIG_REQUESTID_PREFIX, obj.nHeight, obj.nStep);
    }
};

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
}

CChainLocksHandler::~CChainLocksHandler()
{
}

void CChainLocksHandler::Start()
{
    quorumSigningManager->RegisterRecoveredSigsListener(this);
}

void CChainLocksHandler::Stop()
{
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

    for (const auto& pair : bestChainLockCandidates) {
        for (const auto& pair2 : pair.second) {
        if (::SerializeHash(*pair2.second) == hash) {
                ret = *pair2.second;
                return true;
            }
        }
    }

    return false;
}

CChainLockSig CChainLocksHandler::GetMostRecentChainLock()
{
    LOCK(cs);
    return mostRecentChainLockCandidate;
}

const std::map<CQuorumCPtr, CChainLockSigCPtr> CChainLocksHandler::GetBestChainLocks()
{
    const auto& consensus = Params().GetConsensus();
    const auto& llmqParams = consensus.llmqs.at(consensus.llmqTypeChainLocks);
    const auto& signingActiveQuorumCount = llmqParams.signingActiveQuorumCount;

    LOCK(cs);
    std::map<uint256, std::map<CQuorumCPtr, CChainLockSigCPtr> > mapBlockHashCount;
    for (const auto& pair : bestChainLockCandidates) {
        for (const auto& pair2 : pair.second) {
            auto it = mapBlockHashCount.find(pair2.second->blockHash);
            if (it == mapBlockHashCount.end()) {
                mapBlockHashCount[pair2.second->blockHash].insert(pair2);
            } else {
                it->second.insert(pair2);
            }
        }
        for (const auto& pair2 : mapBlockHashCount) {
            if (pair2.second.size() > ((size_t)signingActiveQuorumCount / 2)) {
                return pair2.second;
            }
        }
        mapBlockHashCount.clear();
    }

    return {};
}

void CChainLocksHandler::TryUpdateBestChainLockIndex(const CBlockIndex* pindex, size_t threshold)
{
    AssertLockHeld(cs);

    size_t count{0};
    for (const auto& pair : bestChainLockCandidates.at(pindex->nHeight)) {
        if (pair.second->blockHash == pindex->GetBlockHash()) {
            if (++count >= threshold) {
                bestChainLockBlockIndex = pindex;
                break;
            }
        }
    }

    if (bestChainLockBlockIndex == pindex) {
        // New best chainlock, drop all the old ones
        for (auto it = bestChainLockCandidates.begin(); it != bestChainLockCandidates.end(); ) {
            if (it->first == pindex->nHeight) {
                it = bestChainLockCandidates.erase(++it, bestChainLockCandidates.end());
            } else {
                ++it;
            }
        }
        LogPrint(BCLog::CHAINLOCKS, "CChainLocksHandler::%s -- Dropped old CLSIGs below the locked height (%d), bestChainLockCandidates size: %d\n",
                __func__, pindex->nHeight, bestChainLockCandidates.size());
    }
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

void CChainLocksHandler::ProcessNewChainLock(const NodeId from, const llmq::CChainLockSig& clsig, const uint256&hash, const uint256& id )
{
    bool enforced = false;
    if (from != -1) {
        LOCK(cs_main);
        peerman.ReceivedResponse(from, hash);
    }
    bool bReturn = false;
    {
        LOCK(cs);
        if (!seenChainLocks.try_emplace(hash, GetTimeMillis()).second) {
            bReturn = true;
        }

        if (bestChainLockBlockIndex != nullptr && clsig.nHeight <= bestChainLockBlockIndex->nHeight) {
            // no need to process/relay older CLSIGs
            bReturn = true;
        }
    }
    // don't hold lock while relaying which locks nodes/mempool
    if (from != -1) {
        LOCK(cs_main);
        peerman.ForgetTxHash(from, hash);
    }
    if(bReturn) {
        return;
    }

    CBlockIndex* pindexSig{nullptr};
    CBlockIndex* pindexScan{nullptr};
    {
        LOCK(cs_main);
        if (clsig.nHeight > ::ChainActive().Height() + CSigningManager::SIGN_HEIGHT_OFFSET) {
            // too far into the future
            LogPrint(BCLog::CHAINLOCKS, "CChainLocksHandler::%s -- future CLSIG (%s), peer=%d\n", __func__, clsig.ToString(), from);
            return;
        }
        pindexSig = pindexScan = g_chainman.m_blockman.LookupBlockIndex(clsig.blockHash);
        if (pindexScan == nullptr) {
            // we don't know the block/header for this CLSIG yet, try scanning quorums at chain tip
            pindexScan = ::ChainActive().Tip();
        }
        if (pindexSig != nullptr && pindexSig->nHeight != clsig.nHeight) {
            // Should not happen
            LogPrintf("CChainLocksHandler::%s -- height of CLSIG (%s) does not match the expected block's height (%d)\n",
                    __func__, clsig.ToString(), pindexSig->nHeight);
            if (from != -1) {
                peerman.Misbehaving(from, 10, "invalid CLSIG");
            }
            return;
        }
    }
    const auto& consensus = Params().GetConsensus();
    const auto& llmqType = consensus.llmqTypeChainLocks;
    const auto& llmqParams = consensus.llmqs.at(llmqType);
    const auto& signingActiveQuorumCount = llmqParams.signingActiveQuorumCount;
    std::vector<CQuorumCPtr> quorums_scanned;
    llmq::quorumManager->ScanQuorums(llmqType, pindexScan, signingActiveQuorumCount, quorums_scanned);

    uint256 requestId;
    RequestIdStep requestIdStep{clsig.nHeight};
    bool fValid{false};
    for(const auto& quorum: quorums_scanned) {
        requestId = ::SerializeHash(requestIdStep);
        LogPrint(BCLog::CHAINLOCKS, "CChainLocksHandler::%s -- CLSIG (%s) requestId=%s, peer=%d\n",
            __func__, clsig.ToString(), requestId.ToString(), from);
        if (quorum == nullptr) {
            return;
        }
        // if id is passed in just ensure it matches without having to check sig on every quorum
        if((!id.IsNull() && requestId == id) || id.IsNull()) {
            const uint256 &signHash = CLLMQUtils::BuildSignHash(llmqType, quorum->qc.quorumHash, requestId, clsig.blockHash);
            if (clsig.sig.VerifyInsecure(quorum->qc.quorumPublicKey, signHash)) {
                LOCK(cs);
                // found valid
                auto it = bestChainLockCandidates.find(clsig.nHeight);
                if (it == bestChainLockCandidates.end()) {
                    bestChainLockCandidates[clsig.nHeight].emplace(quorum, std::make_shared<const CChainLockSig>(clsig));
                } else {
                    it->second.emplace(quorum, std::make_shared<const CChainLockSig>(clsig));
                }
                fValid = true;
                break;
            }
        }
        // Try other quorums
        ++requestIdStep.nStep;
    }
    if(!fValid) {
        // tried every possible quorum
        LogPrint(BCLog::CHAINLOCKS, "CChainLocksHandler::%s -- invalid CLSIG (%s), peer=%d\n", __func__, clsig.ToString(), from);
        if (from != -1) {
            LOCK(cs_main);
            peerman.Misbehaving(from, 10, "invalid CLSIG");
        }
        return;
    }
    {
        LOCK(cs);
        mostRecentChainLockCandidate = clsig;

        if (pindexSig != nullptr) {
            TryUpdateBestChainLockIndex(pindexSig, (signingActiveQuorumCount / 2 + 1));
        }
        // else if (pindexSig == nullptr)
        // Note: make sure to still relay clsig further.
    }
    // Note: do not hold cs while calling RelayInv
    AssertLockNotHeld(cs);
    connman.RelayOtherInv(CInv(MSG_CLSIG, hash));
    if (pindexSig == nullptr) {
        // we don't know the block/header for this CLSIG yet, so bail out for now
        // when the block or the header later comes in, we will enforce the correct chain
        return;
    }
    if (bestChainLockBlockIndex == pindexSig) {
        CheckActiveState();
        const CBlockIndex* pindex;
        {       
            LOCK(cs);
            pindex = bestChainLockBlockIndex;
            enforced = isEnforced;
        }
        if(enforced) {
            AssertLockNotHeld(cs);
            EnforceBestChainLock(pindex);
        }
        LogPrint(BCLog::CHAINLOCKS, "CChainLocksHandler::%s -- processed new CLSIG (%s), peer=%d\n",
              __func__, clsig.ToString(), from);
    }
}

void CChainLocksHandler::AcceptedBlockHeader(const CBlockIndex* pindexNew)
{
    const auto& consensus = Params().GetConsensus();
    const auto& llmqParams = consensus.llmqs.at(consensus.llmqTypeChainLocks);
    const auto& signingActiveQuorumCount = llmqParams.signingActiveQuorumCount;

    {
        LOCK(cs);

        // Double check this in case it has changed while we were not holding cs
        auto it = bestChainLockCandidates.find(pindexNew->nHeight);
        if (it == bestChainLockCandidates.end()) {
            return;
        }

        LogPrintf("CChainLocksHandler::%s -- block header %s came in late, updating and enforcing\n", __func__, pindexNew->GetBlockHash().ToString());

        // when EnforceBestChainLock is called later, it might end up invalidating other chains but not activating the
        // CLSIG locked chain. This happens when only the header is known but the block is still missing yet. The usual
        // block processing logic will handle this when the block arrives
        TryUpdateBestChainLockIndex(pindexNew, (signingActiveQuorumCount / 2 + 1));
    }

    if (bestChainLockBlockIndex == pindexNew) {
        CheckActiveState();
        bool enforced;
        const CBlockIndex* pindex;
        {       
            LOCK(cs);
            pindex = bestChainLockBlockIndex;
            enforced = isEnforced;
        }
        if(enforced) {
            AssertLockNotHeld(cs);
            EnforceBestChainLock(pindex);
        }
    }
}

void CChainLocksHandler::UpdatedBlockTip(const CBlockIndex* pindexNew, bool fInitialDownload)
{
    if(fInitialDownload)
        return;
    CheckActiveState();
    bool enforced;
    const CBlockIndex* pindex;
    {       
        LOCK(cs);
        pindex = bestChainLockBlockIndex;
        enforced = isEnforced;
    }
    
    if(enforced) {
        AssertLockNotHeld(cs);
        EnforceBestChainLock(pindex);
    }
    TrySignChainTip(pindexNew);
}

void CChainLocksHandler::CheckActiveState()
{
    bool sporkActive = AreChainLocksEnabled();
    {
        LOCK(cs);
        bool oldIsEnforced = isEnforced;
        isEnabled = sporkActive;
        isEnforced = isEnabled;
        if (!oldIsEnforced && isEnforced) {
            
            // ChainLocks got activated just recently, but it's possible that it was already running before, leaving
            // us with some stale values which we should not try to enforce anymore (there probably was a good reason
            // to disable spork19)
            mostRecentChainLockCandidate = CChainLockSig();
            bestChainLockBlockIndex = nullptr;
        }
    }
}

void CChainLocksHandler::TrySignChainTip(const CBlockIndex* pindex)
{
    Cleanup();

    if (!fMasternodeMode) {
        return;
    }

    if (!masternodeSync.IsBlockchainSynced()) {
        return;
    }
    if (!pindex->pprev) {
        return;
    }
    const uint256 msgHash = pindex->GetBlockHash();
    const int32_t nHeight = pindex->nHeight;
    


    // DIP8 defines a process called "Signing attempts" which should run before the CLSIG is finalized
    // To simplify the initial implementation, we skip this process and directly try to create a CLSIG
    // This will fail when multiple blocks compete, but we accept this for the initial implementation.
    // Later, we'll add the multiple attempts process.

    {
        LOCK(cs);

        if (!isEnabled) {
            return;
        }

        if (nHeight == lastTrySignHeight) {
            // already signed this one
            return;
        }

        if (bestChainLockBlockIndex != nullptr && bestChainLockBlockIndex->nHeight >= pindex->nHeight) {
            // already got the same CLSIG or a better one
            return;
        }

        if (InternalHasConflictingChainLock(nHeight, msgHash)) {
            // don't sign if another conflicting CLSIG is already present. EnforceBestChainLock will later enforce
            // the correct chain.
            return;
        }
    }

    LogPrint(BCLog::CHAINLOCKS, "CChainLocksHandler::%s -- trying to sign %s, height=%d\n", __func__, msgHash.ToString(), nHeight);
    const auto& consensus = Params().GetConsensus();
    const auto& llmqType = consensus.llmqTypeChainLocks;
    const auto& llmqParams = consensus.llmqs.at(consensus.llmqTypeChainLocks);
    const auto& signingActiveQuorumCount = llmqParams.signingActiveQuorumCount;
    std::vector<CQuorumCPtr> quorums_scanned;
    llmq::quorumManager->ScanQuorums(llmqType, pindex, signingActiveQuorumCount, quorums_scanned);

    uint256 requestId;
    RequestIdStep requestIdStep{pindex->nHeight};
    for(const auto& quorum: quorums_scanned) {
        requestId = ::SerializeHash(requestIdStep);
        if (quorum == nullptr) {
            return;
        }

        if (quorumSigningManager->AsyncSignIfMember(llmqType, requestId, pindex->GetBlockHash(), quorum->qc.quorumHash)) {
            LOCK(cs);
            lastSignedRequestIds.insert(requestId);
            lastSignedMsgHash = pindex->GetBlockHash();
        }
        ++requestIdStep.nStep;
    }
    {
        LOCK(cs);
        if (bestChainLockBlockIndex != nullptr && bestChainLockBlockIndex->nHeight >= nHeight) {
            // might have happened while we didn't hold cs
            return;
        }
        lastTrySignHeight = nHeight;
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

        if (lastSignedRequestIds.count(recoveredSig.id) == 0 || recoveredSig.msgHash != lastSignedMsgHash) {
            // this is not what we signed, so lets not create a CLSIG for it
            return;
        }
        if (bestChainLockBlockIndex != nullptr && bestChainLockBlockIndex->nHeight >= lastTrySignHeight) {
            // already got the same or a better CLSIG through the CLSIG message
            return;
        }

        clsig.nHeight = lastTrySignHeight;
        clsig.blockHash = lastSignedMsgHash;
        clsig.sig = recoveredSig.sig.Get();
        lastSignedRequestIds.erase(recoveredSig.id);
    }
    ProcessNewChainLock(-1, clsig, ::SerializeHash(clsig), recoveredSig.id);
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

bool AreChainLocksEnabled()
{
    return sporkManager.IsSporkActive(SPORK_19_CHAINLOCKS_ENABLED);
}

} // namespace llmq
