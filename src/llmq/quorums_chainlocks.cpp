// Copyright (c) 2019 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <llmq/quorums_chainlocks.h>
#include <llmq/quorums.h>
#include <llmq/quorums_utils.h>
#include <llmq/quorums_commitment.h>
#include <chain.h>
#include <consensus/validation.h>
#include <masternode/activemasternode.h>
#include <masternode/masternodesync.h>
#include <net_processing.h>
#include <spork.h>
#include <txmempool.h>
#include <validation.h>
#include <scheduler.h>
#include <util/thread.h>
#include <services/nevmconsensus.h>
#include <evo/deterministicmns.h>
#include <logging.h>
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
    return strprintf("CChainLockSig(nHeight=%d, blockHash=%s, signers: hex=%s size=%d count=%d)",
                nHeight, blockHash.ToString(), CLLMQUtils::ToHexStr(signers), signers.size(),
                std::count(signers.begin(), signers.end(), true));
}

CChainLocksHandler::CChainLocksHandler(CConnman& _connman, PeerManager& _peerman, ChainstateManager& _chainman):     
    connman(_connman),
    peerman(_peerman),
    chainman(_chainman)
{
    scheduler = new CScheduler();
    CScheduler::Function serviceLoop = std::bind(&CScheduler::serviceQueue, scheduler);
    scheduler_thread = new std::thread(&util::TraceThread, "cl-schdlr", serviceLoop);
}

CChainLocksHandler::~CChainLocksHandler()
{
    if(scheduler)
        scheduler->stop();
    if (scheduler_thread->joinable())
        scheduler_thread->join();
    delete scheduler_thread;
    delete scheduler;
}

void CChainLocksHandler::Start()
{
    quorumSigningManager->RegisterRecoveredSigsListener(this);
    scheduler->scheduleEvery([&]() {
        if(tryLockChainTipScheduled) {
            return;
        }
        tryLockChainTipScheduled = true;
        CheckActiveState();
        bool enforced = false;
        const CBlockIndex* pindex;
        {       
            LOCK(cs);
            pindex = bestChainLockBlockIndex;
            enforced = isEnforced;
        }
        bool bEnforce = false;
        if(enforced) {
            bEnforce = chainman.ActiveChainstate().EnforceBestChainLock(pindex);
        }
        if(bEnforce)
            TrySignChainTip();
        tryLockChainTipScheduled = false;
    }, std::chrono::seconds{5});
}

void CChainLocksHandler::Stop()
{
    if(scheduler)
        scheduler->stop();
    if(scheduler_thread)
        scheduler_thread->join();
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

    if (::SerializeHash(mostRecentChainLockShare) == hash) {
        ret = mostRecentChainLockShare;
        return true;
    }

    if (::SerializeHash(bestChainLockWithKnownBlock) == hash) {
        ret = bestChainLockWithKnownBlock;
        return true;
    }

    for (const auto& pair : bestChainLockCandidates) {
        if (::SerializeHash(*pair.second) == hash) {
            ret = *pair.second;
            return true;
        }
    }

    for (const auto& pair : bestChainLockShares) {
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
    return mostRecentChainLockShare;
}
CChainLockSig CChainLocksHandler::GetBestChainLock()
{
    LOCK(cs);
    return bestChainLockWithKnownBlock;
}
const CBlockIndex* CChainLocksHandler::GetPreviousChainLock()
{
    LOCK(cs);
    return bestChainLockBlockIndexPrev;
}
std::map<CQuorumCPtr, CChainLockSigCPtr> CChainLocksHandler::GetBestChainLockShares()
{

    LOCK(cs);
    auto it = bestChainLockShares.find(bestChainLockWithKnownBlock.nHeight);
    if (it == bestChainLockShares.end()) {
        return {};
    }

    return it->second;
}

bool CChainLocksHandler::TryUpdateBestChainLock(const CBlockIndex* pindex)
{
    if (pindex == nullptr || pindex->nHeight <= bestChainLockWithKnownBlock.nHeight) {
        return false;
    }

    auto it1 = bestChainLockCandidates.find(pindex->nHeight);
    if (it1 != bestChainLockCandidates.end()) {
        // only prune blob data upon chainlock so we cannot rollback on pruned blob transactions. If we rolled back on pruned blob data then upon new inclusion there could be situation
        // where new block would fall within 2-hour time window of enforcement and include the pruned blob tx
        // use previous CL instead of this one due to the 2 stage CL finality
        if(bestChainLockBlockIndex && !pnevmdatadb->Prune(bestChainLockBlockIndex->GetMedianTimePast())) {
            LogPrintf("CChainLocksHandler::%s -- CNEVMDataDB::Prune failed\n", __func__);
        }
        bestChainLockWithKnownBlockPrev = bestChainLockWithKnownBlock;
        bestChainLockBlockIndexPrev = bestChainLockBlockIndex;
        bestChainLockWithKnownBlock = *it1->second;
        bestChainLockBlockIndex = pindex;
        LogPrint(BCLog::CHAINLOCKS, "CChainLocksHandler::%s -- CLSIG from candidates (%s)\n", __func__, bestChainLockWithKnownBlock.ToString());
        return true;
    }

    auto it2 = bestChainLockShares.find(pindex->nHeight);
    if (it2 == bestChainLockShares.end()) {
        return false;
    }
    const auto& llmqParams = Params().GetConsensus().llmqTypeChainLocks;
    const size_t& threshold = llmqParams.signingActiveQuorumCount / 2 + 1;

    std::vector<CBLSSignature> sigs;
    CChainLockSig clsigAgg;

    for (const auto& pair : it2->second) {
        if (pair.second->blockHash == pindex->GetBlockHash()) {
            assert(std::count(pair.second->signers.begin(), pair.second->signers.end(), true) <= 1);
            sigs.emplace_back(pair.second->sig);
            if (clsigAgg.IsNull()) {
                clsigAgg = *pair.second;
            } else {
                assert(clsigAgg.signers.size() == pair.second->signers.size());
                std::transform(clsigAgg.signers.begin(), clsigAgg.signers.end(), pair.second->signers.begin(), clsigAgg.signers.begin(), std::logical_or<bool>());
            }
            if (sigs.size() >= threshold) {
                // only prune blob data upon chainlock so we cannot rollback on pruned blob transactions. If we rolled back on pruned blob data then upon new inclusion there could be situation
                // where new block would fall within 2-hour time window of enforcement and include the pruned blob tx
                // use previous CL instead of this one due to the 2 stage CL finality
                if(bestChainLockBlockIndex && !pnevmdatadb->Prune(bestChainLockBlockIndex->GetMedianTimePast())) {
                    LogPrintf("CChainLocksHandler::%s -- CNEVMDataDB::Prune failed\n", __func__);
                }
                // all sigs should be validated already
                clsigAgg.sig = CBLSSignature::AggregateInsecure(sigs);
                bestChainLockWithKnownBlockPrev = bestChainLockWithKnownBlock;
                bestChainLockBlockIndexPrev = bestChainLockBlockIndex;
                bestChainLockWithKnownBlock = clsigAgg;
                bestChainLockBlockIndex = pindex;
                bestChainLockCandidates[clsigAgg.nHeight] = std::make_shared<const CChainLockSig>(clsigAgg);
                LogPrint(BCLog::CHAINLOCKS, "CChainLocksHandler::%s -- CLSIG aggregated (%s)\n", __func__, bestChainLockWithKnownBlock.ToString());
                return true;
            }
        }
    }
    return false;
}

bool CChainLocksHandler::VerifyChainLockShare(const CChainLockSig& clsig, const CBlockIndex* pindexScan, const uint256& idIn, std::pair<int, CQuorumCPtr>& ret)
{
    const auto& consensus = Params().GetConsensus();
    const auto& llmqParams = consensus.llmqTypeChainLocks;
    const auto& signingActiveQuorumCount = llmqParams.signingActiveQuorumCount;


    if (clsig.signers.size() != (size_t)signingActiveQuorumCount) {
        return false;
    }

    if (std::count(clsig.signers.begin(), clsig.signers.end(), true) > 1) {
        // too may signers
        return false;
    }
    bool fHaveSigner{std::count(clsig.signers.begin(), clsig.signers.end(), true) > 0};
    const auto quorums_scanned = llmq::quorumManager->ScanQuorums(pindexScan, signingActiveQuorumCount);

    for (size_t i = 0; i < quorums_scanned.size(); ++i) {
        const CQuorumCPtr& quorum = quorums_scanned[i];
        if (quorum == nullptr) {
            return false;
        }
        uint256 requestId = ::SerializeHash(std::make_tuple(CLSIG_REQUESTID_PREFIX, clsig.nHeight, quorum->qc->quorumHash));
        if ((!idIn.IsNull() && idIn != requestId)) {
            continue;
        }
        if (fHaveSigner && !clsig.signers[i]) {
            continue;
        }
        uint256 signHash = CLLMQUtils::BuildSignHash(quorum->qc->quorumHash, requestId, clsig.blockHash);
        LogPrint(BCLog::CHAINLOCKS, "CChainLocksHandler::%s -- CLSIG (%s) requestId=%s, signHash=%s\n",
                __func__, clsig.ToString(), requestId.ToString(), signHash.ToString());

        if (clsig.sig.VerifyInsecure(quorum->qc->quorumPublicKey, signHash)) {
            if (idIn.IsNull() && !quorumSigningManager->HasRecoveredSigForId(requestId)) {
                // We can reconstruct the CRecoveredSig from the clsig and pass it to the signing manager, which
                // avoids unnecessary double-verification of the signature. We can do this here because we just
                // verified the sig.
                auto rs = std::make_shared<CRecoveredSig>(quorum->qc->quorumHash, requestId, clsig.blockHash, clsig.sig);
                quorumSigningManager->PushReconstructedRecoveredSig(rs);
            }
            ret = std::make_pair(i, quorum);
            return true;
        }
        if (!idIn.IsNull() || fHaveSigner) {
            return false;
        }
    }
    return false;
}

bool CChainLocksHandler::VerifyAggregatedChainLock(const CChainLockSig& clsig, const CBlockIndex* pindexScan)
{
    const auto& consensus = Params().GetConsensus();
    const auto& llmqParams = consensus.llmqTypeChainLocks;
    const auto& signingActiveQuorumCount = llmqParams.signingActiveQuorumCount;

    std::vector<uint256> hashes;
    std::vector<CBLSPublicKey> quorumPublicKeys;

    if (clsig.signers.size() != (size_t)signingActiveQuorumCount) {
        return false;
    }

    if (std::count(clsig.signers.begin(), clsig.signers.end(), true) < (signingActiveQuorumCount / 2 + 1)) {
        // not enough signers
        return false;
    }
    const auto quorums_scanned = llmq::quorumManager->ScanQuorums(pindexScan, signingActiveQuorumCount);
    if(quorums_scanned.empty()) {
        return false;
    }
    for (size_t i = 0; i < quorums_scanned.size(); ++i) {
        const CQuorumCPtr& quorum = quorums_scanned[i];
        if (quorum == nullptr) {
            return false;
        }
        if (!clsig.signers[i]) {
            continue;
        }
        quorumPublicKeys.emplace_back(quorum->qc->quorumPublicKey);
        uint256 requestId = ::SerializeHash(std::make_tuple(CLSIG_REQUESTID_PREFIX, clsig.nHeight, quorum->qc->quorumHash));
        uint256 signHash = CLLMQUtils::BuildSignHash(quorum->qc->quorumHash, requestId, clsig.blockHash);
        hashes.emplace_back(signHash);
        LogPrint(BCLog::CHAINLOCKS, "CChainLocksHandler::%s -- CLSIG (%s) requestId=%s, signHash=%s\n",
                __func__, clsig.ToString(), requestId.ToString(), signHash.ToString());
    }
    return clsig.sig.VerifyInsecureAggregated(quorumPublicKeys, hashes);
}

void CChainLocksHandler::ProcessMessage(CNode* pfrom, const std::string& strCommand, CDataStream& vRecv)
{
    if (!AreChainLocksEnabled()) {
        return;
    }

    if (strCommand == NetMsgType::CLSIG) {
        CChainLockSig clsig;
        vRecv >> clsig;

        ProcessNewChainLock(pfrom->GetId(), clsig, ::SerializeHash(clsig));
    }
}

void CChainLocksHandler::ProcessNewChainLock(const NodeId from, llmq::CChainLockSig& clsig, const uint256&hash, const uint256& idIn )
{
    assert((from == -1) ^ idIn.IsNull());
    if (from != -1) {
        LOCK(cs_main);
        peerman.ReceivedResponse(from, hash);
    }
    CheckActiveState();
    PeerRef peer = peerman.GetPeerRef(from);
    bool bReturn = false;
    {
        LOCK(cs);
        if (!seenChainLocks.emplace(hash, TicksSinceEpoch<std::chrono::milliseconds>(SystemClock::now())).second) {
            bReturn = true;
        }

        if (!bestChainLockWithKnownBlock.IsNull() && clsig.nHeight <= bestChainLockWithKnownBlock.nHeight) {
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
    CBlockIndex* pindexScan{nullptr};
    const CBlockIndex* bestIndex;
    {
        LOCK(cs);
        bestIndex = bestChainLockBlockIndex;
    }
    {
        LOCK(cs_main);
        if(chainman.m_best_header) {
            const auto& nHeightDiff = chainman.m_best_header->nHeight - clsig.nHeight;
            // height from best known header does not look back enough (SIGN_HEIGHT_LOOKBACK blocks). MNs should be locking active chain - SIGN_HEIGHT_LOOKBACK block height
            if(nHeightDiff < CSigningManager::SIGN_HEIGHT_LOOKBACK) {
                // too far into the future
                LogPrint(BCLog::CHAINLOCKS, "CChainLocksHandler::%s -- future CLSIG (%s), peer=%d\n", __func__, clsig.ToString(), from);
                return;
            }
            // if best known header has moved on 2 more blocks from when it should have locked then also reject
            // this means there is a window of time of 2 blocks when a chainlock should be formed, completed and propagated
            // it will stop scenario of masternodes/miners colluding to form their own chain as they see fit after the fact (after any number of blocks), rolling back the chain to previous chainlock
            if(nHeightDiff > (CSigningManager::SIGN_HEIGHT_LOOKBACK+2)) {
                // too far into the past
                LogPrint(BCLog::CHAINLOCKS, "CChainLocksHandler::%s -- old CLSIG (%s), peer=%d\n", __func__, clsig.ToString(), from);
                return;
            }
        }
        pindexScan = chainman.m_blockman.LookupBlockIndex(clsig.blockHash);
        // make sure block or header exists before accepting CLSIG
        if (pindexScan == nullptr) {
            LogPrintf("CChainLocksHandler::%s -- block of CLSIG (%s) does not exist\n",
                    __func__, clsig.ToString());
            return;
        }
        if (pindexScan != nullptr && pindexScan->nHeight != clsig.nHeight) {
            // Should not happen
            LogPrintf("CChainLocksHandler::%s -- height of CLSIG (%s) does not match the expected block's height (%d)\n",
                    __func__, clsig.ToString(), pindexScan->nHeight);
            if (from != -1) {
                if(peer)
                    peerman.Misbehaving(*peer, 10, "invalid CLSIG");
            }
            return;
        }
        if ((clsig.nHeight%CSigningManager::SIGN_HEIGHT_LOOKBACK) != 0) {
            // Should not happen
            LogPrintf("CChainLocksHandler::%s -- height of CLSIG (%s) is not a factor of 5\n",
                    __func__, clsig.ToString());
            if (from != -1) {
                if(peer)
                    peerman.Misbehaving(*peer, 10, "invalid CLSIG");
            }
            return;
        }
        // current chainlock should be confirmed before trying to make new one (don't let headers-only be locked by more than 1 CL)
        if (bestIndex && !chainman.ActiveChain().Contains(bestIndex)) {
            // Should not happen
            LogPrintf("CChainLocksHandler::%s -- current chainlock not confirmed. CLSIG (%s) rejected\n",
                    __func__, clsig.ToString());
            if (from != -1) {
                if(peer)
                    peerman.Misbehaving(*peer, 10, "invalid CLSIG");
            }
            return;
        }
    }
    bool bConflict{false};
    {
        LOCK(cs);
        bConflict = InternalHasConflictingChainLock(clsig.nHeight, clsig.blockHash);
    }
    if (bConflict) {
        // don't allow CLSIG if another conflicting CLSIG is already present. EnforceBestChainLock will later enforce
        // the correct chain.
        LogPrintf("CChainLocksHandler::%s -- conflicting CLSIG (%s)\n",
                    __func__, clsig.ToString());
        if (from != -1) {
            if(peer)
                peerman.Misbehaving(*peer, 10, "invalid CLSIG");
        }
        return;
    }
    const auto& consensus = Params().GetConsensus();
    const auto& llmqParams = consensus.llmqTypeChainLocks;
    const auto& signingActiveQuorumCount = llmqParams.signingActiveQuorumCount;
    size_t signers_count = std::count(clsig.signers.begin(), clsig.signers.end(), true);
    if (from != -1 && (clsig.signers.empty() || signers_count == 0)) {
        LogPrint(BCLog::CHAINLOCKS, "CChainLocksHandler::%s -- invalid signers count (%d) for CLSIG (%s), peer=%d\n", __func__, signers_count, clsig.ToString(), from);
        if(peer)
            peerman.Misbehaving(*peer, 10, "invalid CLSIG");
        return;
    }
    if (from == -1 || signers_count == 1) {
        // A part of a multi-quorum CLSIG signed by a single quorum
        std::pair<int, CQuorumCPtr> ret;
        clsig.signers.resize(signingActiveQuorumCount, false);
        if (!VerifyChainLockShare(clsig, pindexScan, idIn, ret)) {
            LogPrint(BCLog::CHAINLOCKS, "CChainLocksHandler::%s -- invalid CLSIG (%s), peer=%d\n", __func__, clsig.ToString(), from);
            if (from != -1) {
                if(peer)
                    peerman.Misbehaving(*peer, 10, "invalid CLSIG");
            }
            return;
        }
        CInv clsigAggInv;
        {
            LOCK(cs);
            clsig.signers[ret.first] = true;
            if (std::count(clsig.signers.begin(), clsig.signers.end(), true) > 1) {
                // this should never happen
                LogPrint(BCLog::CHAINLOCKS, "CChainLocksHandler::%s -- ERROR in VerifyChainLockShare, CLSIG (%s), peer=%d\n", __func__, clsig.ToString(), from);
                return;
            }
            auto it = bestChainLockShares.find(clsig.nHeight);
            if (it == bestChainLockShares.end()) {
                bestChainLockShares[clsig.nHeight].emplace(ret.second, std::make_shared<const CChainLockSig>(clsig));
            } else {
                it->second.emplace(ret.second, std::make_shared<const CChainLockSig>(clsig));
            }
            mostRecentChainLockShare = clsig;
            if (TryUpdateBestChainLock(pindexScan)) {
                clsigAggInv = CInv(MSG_CLSIG, ::SerializeHash(bestChainLockWithKnownBlock));
            }
        }
        if (clsigAggInv.type == MSG_CLSIG) {
            // We just created an aggregated CLSIG, relay it
            peerman.RelayTransactionOther(clsigAggInv);
        } else {
            {
                LOCK(cs_main);
                CInv clsigInv(MSG_CLSIG, ::SerializeHash(clsig));
                // Relay partial CLSIGs to full nodes only, SPV wallets should wait for the aggregated CLSIG.
                connman.ForEachNode([&](CNode* pnode) EXCLUSIVE_LOCKS_REQUIRED(::cs_main) {
                    AssertLockHeld(::cs_main);
                    bool fSPV{pnode->m_bloom_filter_loaded.load()};
                    if (!fSPV && pnode->CanRelay()) {
                        PeerRef peer = peerman.GetPeerRef(pnode->GetId());
                        if(peer) {
                            peerman.PushTxInventoryOther(*peer, clsigInv);
                        }
                    }
                });
            }
            // Try signing the tip ourselves
            TrySignChainTip();
        }
    } else {
        CInv clsigInv(MSG_CLSIG, hash);
        // An aggregated CLSIG
        if (!VerifyAggregatedChainLock(clsig, pindexScan)) {
            LogPrint(BCLog::CHAINLOCKS, "CChainLocksHandler::%s -- invalid CLSIG (%s), peer=%d\n", __func__, clsig.ToString(), from);
            if (from != -1) {
                if(peer)
                    peerman.Misbehaving(*peer, 10, "invalid CLSIG");
            }
            return;
        }
        {
            LOCK(cs);
            bestChainLockCandidates[clsig.nHeight] = std::make_shared<const CChainLockSig>(clsig);
            mostRecentChainLockShare = clsig;
            TryUpdateBestChainLock(pindexScan);
        }
        peerman.RelayTransactionOther(clsigInv);
    }
    
    bool bChainLockMatchSigIndex = WITH_LOCK(cs, return bestChainLockBlockIndex == pindexScan);
    if (bChainLockMatchSigIndex) {
        CheckActiveState();
        bool enforced = false;
        const CBlockIndex* pindex;
        {       
            LOCK(cs);
            pindex = bestChainLockBlockIndex;
            enforced = isEnforced;
        }
        if(enforced) {
            chainman.ActiveChainstate().EnforceBestChainLock(pindex);
        }
        LogPrint(BCLog::CHAINLOCKS, "CChainLocksHandler::%s -- processed new CLSIG (%s), peer=%d\n",
              __func__, clsig.ToString(), from);
    }
}

void CChainLocksHandler::NotifyHeaderTip(const CBlockIndex* pindexNew)
{
    LOCK(cs);

    auto it = bestChainLockCandidates.find(pindexNew->nHeight);
    if (it == bestChainLockCandidates.end()) {
        return;
    }

    LogPrint(BCLog::CHAINLOCKS, "CChainLocksHandler::%s -- block header %s came in late, updating and enforcing\n", __func__, pindexNew->GetBlockHash().ToString());

    // when EnforceBestChainLock is called later, it might end up invalidating other chains but not activating the
    // CLSIG locked chain. This happens when only the header is known but the block is still missing yet. The usual
    // block processing logic will handle this when the block arrives
    TryUpdateBestChainLock(pindexNew);
}

void CChainLocksHandler::UpdatedBlockTip(const CBlockIndex* pindexNew, bool fInitialDownload)
{
    if(fInitialDownload)
        return;
    // don't call TrySignChainTip directly but instead let the scheduler call it. This way we ensure that cs_main is
    // never locked and TrySignChainTip is not called twice in parallel. Also avoids recursive calls due to
    // EnforceBestChainLock switching chains.
    // atomic[If tryLockChainTipScheduled is false, do (set it to true] and schedule signing).
    if (!tryLockChainTipScheduled) {
        tryLockChainTipScheduled = true;
        scheduler->scheduleFromNow([&]() {
            CheckActiveState();
            bool enforced = false;
            const CBlockIndex* pindex;
            {       
                LOCK(cs);
                pindex = bestChainLockBlockIndex;
                enforced = isEnforced;
            }
            bool bEnforce = false;
            if(enforced) {
                bEnforce = chainman.ActiveChainstate().EnforceBestChainLock(pindex);
            }
            if(bEnforce)
                TrySignChainTip();
            tryLockChainTipScheduled = false;
        }, std::chrono::seconds{0});
    }
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
            mostRecentChainLockShare = bestChainLockWithKnownBlock = bestChainLockWithKnownBlockPrev = CChainLockSig();
            bestChainLockBlockIndex = bestChainLockBlockIndexPrev = nullptr;
            bestChainLockCandidates.clear();
            bestChainLockShares.clear();
        }
    }
}

static struct SigningState
{
private:
    const static int ATTEMPT_START{-2}; // let a couple of extra attempts to wait for busy/slow quorums

    int attempt GUARDED_BY(cs) {ATTEMPT_START};
    int lastSignedHeight GUARDED_BY(cs) {-1};

public:
    mutable RecursiveMutex cs;
    void SetLastSignedHeight(int _lastSignedHeight)
    {
        LOCK(cs);
        lastSignedHeight = _lastSignedHeight;
        attempt = ATTEMPT_START;
    }
    int GetLastSignedHeight() const
    {
        LOCK(cs);
        return lastSignedHeight;
    }
    void BumpAttempt()
    {
        LOCK(cs);
        ++attempt;
    }
    int GetAttempt() const
    {
        LOCK(cs);
        return attempt;
    }
} signingState;

void CChainLocksHandler::TrySignChainTip()
{
    Cleanup();
    if (!fMasternodeMode) {
        return;
    }

    if (!masternodeSync.IsBlockchainSynced()) {
        return;
    }

    const CBlockIndex* pindex = WITH_LOCK(cs_main, return chainman.ActiveChain()[chainman.ActiveHeight()-CSigningManager::SIGN_HEIGHT_LOOKBACK]);
    if (!pindex || !pindex->pprev) {
        return;
    }
    const uint256 msgHash = pindex->GetBlockHash();
    const int32_t nHeight = pindex->nHeight;


    // DIP8 defines a process called "Signing attempts" which should run before the CLSIG is finalized
    // To simplify the initial implementation, we skip this process and directly try to create a CLSIG
    // This will fail when multiple blocks compete, but we accept this for the initial implementation.
    // Later, we'll add the multiple attempts process.
    const CBlockIndex* bestIndex;
    {
        LOCK(cs);
        bestIndex = bestChainLockBlockIndex;
    }
    {
        LOCK(cs_main);
        // current chainlock should be confirmed before trying to make new one (don't let headers-only be locked by more than 1 CL)
        if (bestIndex && !chainman.ActiveChain().Contains(bestIndex)) {
            return;
        }
    }
    {
        LOCK(cs);

        if (!isEnabled) {
            return;
        }
        // only sign every fifth block
        if ((nHeight%CSigningManager::SIGN_HEIGHT_LOOKBACK) != 0) {
            return;
        }
        if (nHeight == signingState.GetLastSignedHeight()) {
            // already signed this one
            return;
        }

        if (bestChainLockWithKnownBlock.nHeight >= nHeight) {
            // already got the same CLSIG or a better one, reset and bail out
            signingState.SetLastSignedHeight(bestChainLockWithKnownBlock.nHeight);
            return;
        }

        if (InternalHasConflictingChainLock(nHeight, msgHash)) {
            // don't sign if another conflicting CLSIG is already present. EnforceBestChainLock will later enforce
            // the correct chain.
            return;
        }
        mapSignedRequestIds.clear();

    }

    LogPrint(BCLog::CHAINLOCKS, "CChainLocksHandler::%s -- trying to sign %s, height=%d\n", __func__, msgHash.ToString(), nHeight);
    const auto& consensus = Params().GetConsensus();
    const auto& llmqParams = consensus.llmqTypeChainLocks;
    const auto& signingActiveQuorumCount = llmqParams.signingActiveQuorumCount;
    
    const auto quorums_scanned = llmq::quorumManager->ScanQuorums(pindex, signingActiveQuorumCount);
    std::map<CQuorumCPtr, CChainLockSigCPtr> mapSharesAtTip;
    {
        LOCK(cs);
        const auto it = bestChainLockShares.find(nHeight);
        if (it != bestChainLockShares.end()) {
            mapSharesAtTip = it->second;
        }
    }
    bool fMemberOfSomeQuorum{false};
    const auto heightHashKP = std::make_pair(nHeight, msgHash);
    signingState.BumpAttempt();
    auto proTxHash = WITH_LOCK(activeMasternodeInfoCs, return activeMasternodeInfo.proTxHash);
    for (size_t i = 0; i < quorums_scanned.size(); ++i) {
        int nQuorumIndex = (nHeight + i) % quorums_scanned.size();
        const CQuorumCPtr& quorum = quorums_scanned[nQuorumIndex];
        if (quorum == nullptr) {
            return;
        }
        if (!quorum->IsValidMember(proTxHash)) {
            continue;
        }
        fMemberOfSomeQuorum = true;
        if (i > 0) {
            int nQuorumIndexPrev = (nQuorumIndex + 1) % quorums_scanned.size();
            auto it2 = mapSharesAtTip.find(quorums_scanned[nQuorumIndexPrev]);
            if (it2 == mapSharesAtTip.end() && signingState.GetAttempt() > (int)i) {
                // look deeper after 'i' attempts
                while (nQuorumIndexPrev != nQuorumIndex) {
                    nQuorumIndexPrev = (nQuorumIndexPrev + 1) % quorums_scanned.size();
                    it2 = mapSharesAtTip.find(quorums_scanned[nQuorumIndexPrev]);
                    if (it2 != mapSharesAtTip.end()) {
                        break;
                    }
                    LogPrint(BCLog::CHAINLOCKS, "CChainLocksHandler::%s -- previous quorum (%d, %s) didn't sign a chainlock at height %d yet\n",
                            __func__, nQuorumIndexPrev, quorums_scanned[nQuorumIndexPrev]->qc->quorumHash.ToString(), nHeight);
                }
            }
            if (it2 == mapSharesAtTip.end()) {
                if (signingState.GetAttempt() <= (int)i) {
                    // previous quorum did not sign a chainlock, bail out for now
                    LogPrint(BCLog::CHAINLOCKS, "CChainLocksHandler::%s -- previous quorum did not sign a chainlock at height %d yet, signingState.GetAttempt() %d i %d\n", __func__, nHeight, signingState.GetAttempt(), i);
                    return;
                }
                // else
                // waiting for previous quorum(s) to sign anything for too long already,
                // just sign whatever we think is a good tip
            } else if (it2->second->blockHash != msgHash) {
                LOCK(cs_main);
                auto shareBlockIndex = chainman.m_blockman.LookupBlockIndex(it2->second->blockHash);
                if (shareBlockIndex != nullptr && shareBlockIndex->nHeight == nHeight) {
                    // previous quorum signed an alternative chain tip, sign it too instead
                    LogPrint(BCLog::CHAINLOCKS, "CChainLocksHandler::%s -- previous quorum (%d, %s) signed an alternative chaintip (%s != %s) at height %d, join it\n",
                            __func__, nQuorumIndexPrev, quorums_scanned[nQuorumIndexPrev]->qc->quorumHash.ToString(), it2->second->blockHash.ToString(), msgHash.ToString(), nHeight);
                    pindex = shareBlockIndex;
                } else if (signingState.GetAttempt() <= (int)i) {
                    // previous quorum signed some different hash we have no idea about, bail out for now
                    LogPrint(BCLog::CHAINLOCKS, "CChainLocksHandler::%s -- previous quorum (%d, %s) signed an unknown or an invalid blockHash (%s != %s) at height %d\n",
                            __func__, nQuorumIndexPrev, quorums_scanned[nQuorumIndexPrev]->qc->quorumHash.ToString(), it2->second->blockHash.ToString(), msgHash.ToString(), nHeight);
                    return;
                }
                // else
                // waiting the unknown/invalid hash for too long already,
                // just sign whatever we think is a good tip
            }
        }
        LogPrint(BCLog::CHAINLOCKS, "CChainLocksHandler::%s -- use quorum (%d, %s) and try to sign %s at height %d\n",
                __func__, nQuorumIndex, quorums_scanned[nQuorumIndex]->qc->quorumHash.ToString(), msgHash.ToString(), nHeight);
        uint256 requestId = ::SerializeHash(std::make_tuple(CLSIG_REQUESTID_PREFIX, nHeight, quorum->qc->quorumHash));
        {
            LOCK(cs);
            if (bestChainLockWithKnownBlock.nHeight >= nHeight) {
                // might have happened while we didn't hold cs
                return;
            }
            mapSignedRequestIds.emplace(requestId, heightHashKP);
        }
        quorumSigningManager->AsyncSignIfMember(requestId, msgHash, quorum->qc->quorumHash);
    }
    if (!fMemberOfSomeQuorum || signingState.GetAttempt() >= (int)quorums_scanned.size()) {
        // not a member or tried too many times, nothing to do
        signingState.SetLastSignedHeight(nHeight);
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

        auto it = mapSignedRequestIds.find(recoveredSig.id);
        if (it == mapSignedRequestIds.end() || recoveredSig.msgHash != it->second.second) {
            // this is not what we signed, so lets not create a CLSIG for it
            return;
        }
        if (bestChainLockWithKnownBlock.nHeight >= it->second.first) {
            // already got the same or a better CLSIG through the CLSIG message
            return;
        }

        clsig.nHeight = it->second.first;
        clsig.blockHash = it->second.second;
        clsig.sig = recoveredSig.sig.Get();
        mapSignedRequestIds.erase(recoveredSig.id);
    }
    ProcessNewChainLock(-1, clsig, ::SerializeHash(clsig), recoveredSig.id);
}

bool CChainLocksHandler::HasChainLock(int nHeight, const uint256& blockHash)
{
    LOCK(cs);
    return InternalHasChainLock(nHeight, blockHash);
}

bool CChainLocksHandler::InternalHasChainLock(int nHeight, const uint256& blockHash) const
{
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

void CChainLocksHandler::SetToPreviousChainLock()
{
    const CBlockIndex* prevIndex = GetPreviousChainLock();
    if(!prevIndex) {
        return;
    }
    LOCK(cs);
    bestChainLockShares.erase(bestChainLockBlockIndex->nHeight);
    bestChainLockWithKnownBlock = bestChainLockWithKnownBlockPrev;
    // move index back to previous lock position so chain cannot reorg farther than 2 chainlocks no matter what
    bestChainLockBlockIndex = prevIndex;
    bestChainLockCandidates.clear();
    bestChainLockCandidates[bestChainLockWithKnownBlock.nHeight] = std::make_shared<const CChainLockSig>(bestChainLockWithKnownBlock);
    bestChainLockShares.clear();
    signingState.SetLastSignedHeight(bestChainLockWithKnownBlock.nHeight);
    mostRecentChainLockShare = CChainLockSig();
    // clear recovered sig db and cl vote db
    quorumSigningManager->Clear();
}

bool CChainLocksHandler::InternalHasConflictingChainLock(int nHeight, const uint256& blockHash) const
{
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
        if (TicksSinceEpoch<std::chrono::milliseconds>(SystemClock::now()) - lastCleanupTime < CLEANUP_INTERVAL) {
            return;
        }
    }

    LOCK(cs);

    for (auto it = seenChainLocks.begin(); it != seenChainLocks.end(); ) {
        if (TicksSinceEpoch<std::chrono::milliseconds>(SystemClock::now()) - it->second >= CLEANUP_SEEN_TIMEOUT) {
            it = seenChainLocks.erase(it);
        } else {
            ++it;
        }
    }

    if (bestChainLockBlockIndex != nullptr) {
        for (auto it = bestChainLockCandidates.begin(); it != bestChainLockCandidates.end(); ) {
            if (it->first == bestChainLockBlockIndex->nHeight) {
                it = bestChainLockCandidates.erase(++it, bestChainLockCandidates.end());
            } else {
                ++it;
            }
        }
        for (auto it = bestChainLockShares.begin(); it != bestChainLockShares.end(); ) {
            if (it->first == bestChainLockBlockIndex->nHeight) {
                it = bestChainLockShares.erase(++it, bestChainLockShares.end());
            } else {
                ++it;
            }
        }
    }
    lastCleanupTime = TicksSinceEpoch<std::chrono::milliseconds>(SystemClock::now());
}

bool AreChainLocksEnabled()
{
    return sporkManager->IsSporkActive(SPORK_19_CHAINLOCKS_ENABLED);
}

} // namespace llmq
