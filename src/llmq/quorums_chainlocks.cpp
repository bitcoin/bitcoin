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
#include <llmq/quorums_blockprocessor.h>
#include <netmessagemaker.h>

#include <iterator>
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

bool CChainLocksHandler::GetCLSIGFromPeers() {
    LogPrint(BCLog::CHAINLOCKS, "%s -- Get CLSIG\n", __func__);
    const CNetMsgMaker msgMaker(PROTOCOL_VERSION);
    const CConnman::NodesSnapshot snap{connman, /* filter = */ FullyConnectedOnly};
    for (auto pNodeTmp : snap.Nodes()) {
        connman.PushMessage(pNodeTmp, msgMaker.Make(NetMsgType::GETCLSIG));        
    }
    return true;
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

const CBlockIndex* CChainLocksHandler::GetBestChainLockIndex()
{
    LOCK(cs);
    return bestChainLockBlockIndex;
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
        if (it1->second->blockHash != pindex->GetBlockHash()) {
            LogPrint(BCLog::CHAINLOCKS, "CChainLocksHandler::%s -- candidate hash mismatch at height=%d candidate=%s pindex=%s, skip\n",
                    __func__, pindex->nHeight, it1->second->blockHash.ToString(), pindex->GetBlockHash().ToString());
            return false;
        }
        bestChainLockWithKnownBlock = *it1->second;
        bestChainLockBlockIndex = pindex;
        AddRecentChainLock(bestChainLockWithKnownBlock);
        // only prune blob data upon chainlock so we cannot rollback on pruned blob transactions. If we rolled back on pruned blob data then upon new inclusion there could be situation
        // where new block would fall within 2-hour time window of enforcement and include the pruned blob tx
        if(!pnevmdatadb->PruneStandalone(bestChainLockBlockIndex->GetMedianTimePast())) {
            LogPrintf("CChainLocksHandler::%s -- CNEVMDataDB::Prune failed\n", __func__);
        }
        LogPrint(BCLog::CHAINLOCKS, "CChainLocksHandler::%s -- CLSIG from candidates (%s)\n", __func__, bestChainLockWithKnownBlock.ToString());
        return true;
    }

    auto it2 = bestChainLockShares.find(pindex->nHeight);
    if (it2 == bestChainLockShares.end()) {
        return false;
    }
    const auto& llmqParams = Params().GetConsensus().llmqTypeChainLocks;
    const size_t threshold = llmqParams.signingActiveQuorumCount / 2 + 1;

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
                // all sigs should be validated already
                clsigAgg.sig = CBLSSignature::AggregateInsecure(sigs);
                bestChainLockWithKnownBlock = clsigAgg;
                bestChainLockBlockIndex = pindex;
                bestChainLockCandidates[clsigAgg.nHeight] = std::make_shared<const CChainLockSig>(clsigAgg);
                AddRecentChainLock(bestChainLockWithKnownBlock);
                // only prune blob data upon chainlock so we cannot rollback on pruned blob transactions. If we rolled back on pruned blob data then upon new inclusion there could be situation
                // where new block would fall within 2-hour time window of enforcement and include the pruned blob tx
                if(!pnevmdatadb->PruneStandalone(bestChainLockBlockIndex->GetMedianTimePast())) {
                    LogPrintf("CChainLocksHandler::%s -- CNEVMDataDB::Prune failed\n", __func__);
                }
                LogPrint(BCLog::CHAINLOCKS, "CChainLocksHandler::%s -- CLSIG aggregated (%s)\n", __func__, bestChainLockWithKnownBlock.ToString());
                return true;
            }
        }
    }
    return false;
}


bool CChainLocksHandler::VerifyChainLockShare(const CChainLockSig& clsig, const CBlockIndex* pindexScan, const uint256& idIn, std::pair<int, CQuorumCPtr>& ret, const uint256& hash)
{
    {
        LOCK(cs);
        if(sigChecked.find(hash) != sigChecked.end()) {
            return true;
        }
    }
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
        uint256 signHash = llmq::BuildSignHash(quorum->qc->quorumHash, requestId, clsig.blockHash);
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
            {
                LOCK(cs);
                sigChecked.emplace(hash, TicksSinceEpoch<std::chrono::milliseconds>(SystemClock::now()));
            }
            return true;
        }
        if (!idIn.IsNull() || fHaveSigner) {
            return false;
        }
    }
    return false;
}

bool CChainLocksHandler::VerifyAggregatedChainLock(const CChainLockSig& clsig, const CBlockIndex* pindexScan, const uint256 &hash)
{
    {
        LOCK(cs);
        if(sigChecked.find(hash) != sigChecked.end()) {
            return true;
        }
    }
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
        uint256 signHash = llmq::BuildSignHash(quorum->qc->quorumHash, requestId, clsig.blockHash);
        hashes.emplace_back(signHash);
        LogPrint(BCLog::CHAINLOCKS, "CChainLocksHandler::%s -- index %d CLSIG (%s) pindexScan=%s requestId=%s (clsig.nHeight %d, quorum->qc->quorumHash %s), signHash=%s (quorum->qc->quorumHash, requestId, clsig.blockHash)\n",
                __func__, i, clsig.ToString(), pindexScan->GetBlockHash().ToString(), requestId.ToString(), clsig.nHeight, quorum->qc->quorumHash.ToString(), signHash.ToString());
    }
    bool result = clsig.sig.VerifyInsecureAggregated(quorumPublicKeys, hashes);
    if(result) {
        LOCK(cs);
        sigChecked.emplace(hash, TicksSinceEpoch<std::chrono::milliseconds>(SystemClock::now()));
    } else {
        LogPrintf("CChainLocksHandler::%s -- aggregated verify failed nHeight=%d blockHash=%s pindexScan=%s signers_count=%d signers_size=%d quorums_scanned=%d\n",
                __func__,
                clsig.nHeight,
                clsig.blockHash.ToString(),
                pindexScan ? pindexScan->GetBlockHash().ToString() : "null",
                (int)std::count(clsig.signers.begin(), clsig.signers.end(), true),
                (int)clsig.signers.size(),
                (int)quorums_scanned.size());
    }
    return result;
}

bool CChainLocksHandler::VerifyAggregatedChainLockNoCache(const CChainLockSig& clsig, const CBlockIndex* pindexScan)
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
        return false;
    }

    const auto quorums_scanned = llmq::quorumManager->ScanQuorums(pindexScan, signingActiveQuorumCount);
    if (quorums_scanned.empty()) {
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
        uint256 signHash = llmq::BuildSignHash(quorum->qc->quorumHash, requestId, clsig.blockHash);
        hashes.emplace_back(signHash);
    }
    return clsig.sig.VerifyInsecureAggregated(quorumPublicKeys, hashes);
}

bool CChainLocksHandler::GetRecentChainLockByHeight(int32_t nHeight, CChainLockSig& ret)
{
    LOCK(cs);
    auto it = recentChainLocks.find(nHeight);
    if (it == recentChainLocks.end()) {
        return false;
    }
    ret = it->second;
    return true;
}

void CChainLocksHandler::AddRecentChainLock(const CChainLockSig& clsig)
{
    if (clsig.IsNull()) {
        return;
    }
    recentChainLocks[clsig.nHeight] = clsig;
    while ((int32_t)recentChainLocks.size() > RECENT_CHAINLOCKS_MAX) {
        recentChainLocks.erase(std::prev(recentChainLocks.end()));
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
        BlockValidationState state;
        ProcessNewChainLock(pfrom->GetId(), clsig, state, ::SerializeHash(clsig));
    }
}

bool CChainLocksHandler::ProcessNewChainLock(const NodeId from, llmq::CChainLockSig& clsig, BlockValidationState& state, const uint256& hash, const uint256& idIn )
{
    assert((from == -1) ^ idIn.IsNull());
    CheckActiveState();
    PeerRef peer = peerman.GetPeerRef(from);
    if (from != -1) {
        if (peer)
            peerman.AddKnownTx(*peer, hash);
        LOCK(cs_main);
        peerman.ReceivedResponse(from, hash);
    }
    {
        LOCK2(cs_main, cs);
        if(seenChainLocks.find(hash) != seenChainLocks.end()) {
            if (from != -1) {
                peerman.ForgetTxHash(from, hash);
            }
            return state.Invalid(BlockValidationResult::BLOCK_CHAINLOCK, "already-seen-clsig");
        }
        if (!bestChainLockWithKnownBlock.IsNull() && clsig.nHeight <= bestChainLockWithKnownBlock.nHeight) {
            if (from != -1) {
                peerman.ForgetTxHash(from, hash);
            }
            return state.Invalid(BlockValidationResult::BLOCK_CHAINLOCK, "existing-clsig-height");
        }
    
        int nActiveHeight = chainman.ActiveHeight()-SIGN_HEIGHT_OFFSET;
        nActiveHeight -= nActiveHeight%SIGN_HEIGHT_OFFSET;
        if(nActiveHeight != clsig.nHeight) {
            if (from != -1) {
                peerman.ForgetTxHash(from, hash);
            }
            return state.Invalid(BlockValidationResult::BLOCK_CHAINLOCK, "mismatch-clsig-height");  
        }
    }
    CBlockIndex* pindexScan{nullptr};
    const CBlockIndex* bestIndex;
    {
        LOCK(cs);
        bestIndex = bestChainLockBlockIndex;
    }
    {
        LOCK(cs_main);
        pindexScan = chainman.m_blockman.LookupBlockIndex(clsig.blockHash);
        // make sure block or header exists before accepting CLSIG
        if (pindexScan == nullptr) {
            LogPrintf("CChainLocksHandler::%s -- block of CLSIG (%s) does not exist\n",
                    __func__, clsig.ToString());
            if (from != -1) {
                peerman.ForgetTxHash(from, hash);
            }
            return state.Invalid(BlockValidationResult::BLOCK_CHAINLOCK, "bad-clsig-block");
        }
        if (pindexScan != nullptr && pindexScan->nHeight != clsig.nHeight) {
            // Should not happen
            LogPrintf("CChainLocksHandler::%s -- height of CLSIG (%s) does not match the expected block's height (%d)\n",
                    __func__, clsig.ToString(), pindexScan->nHeight);
            if (from != -1) {
                peerman.ForgetTxHash(from, hash);                  
            }
            return state.Invalid(BlockValidationResult::BLOCK_CHAINLOCK, "bad-clsig-height");
        }
        if ((clsig.nHeight%SIGN_HEIGHT_OFFSET) != 0) {
            // Should not happen
            LogPrintf("CChainLocksHandler::%s -- height of CLSIG (%s) is not a factor of 5\n",
                    __func__, clsig.ToString());
            if (from != -1) {
                peerman.ForgetTxHash(from, hash);            
            }
            return state.Invalid(BlockValidationResult::BLOCK_CHAINLOCK, "clsig-height-align");
        }
        // current chainlock should be confirmed before trying to make new one (don't let headers-only be locked by more than 1 CL)
        if (bestIndex && (!chainman.ActiveChain().Contains(bestIndex) || !bestIndex->IsValid(BLOCK_VALID_SCRIPTS))) {
            // Should not happen
            LogPrintf("CChainLocksHandler::%s -- current chainlock not confirmed. CLSIG (%s) rejected\n",
                    __func__, clsig.ToString());
            if (from != -1) {
                peerman.ForgetTxHash(from, hash);
            }
            return state.Invalid(BlockValidationResult::BLOCK_CHAINLOCK, "clsig-chainstate-missing");
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
            {
                LOCK(cs_main);
                peerman.ForgetTxHash(from, hash);
            }
        }
        return state.Invalid(BlockValidationResult::BLOCK_CHAINLOCK, "clsig-conflict");
    }
    const auto& consensus = Params().GetConsensus();
    const auto& llmqParams = consensus.llmqTypeChainLocks;
    const auto& signingActiveQuorumCount = llmqParams.signingActiveQuorumCount;
    size_t signers_count = std::count(clsig.signers.begin(), clsig.signers.end(), true);
    if (from != -1 && (clsig.signers.empty() || signers_count == 0)) {
        LogPrint(BCLog::CHAINLOCKS, "CChainLocksHandler::%s -- invalid signers count (%d) for CLSIG (%s), peer=%d\n", __func__, signers_count, clsig.ToString(), from);
        {
            LOCK(cs_main);
            peerman.ForgetTxHash(from, hash);
        }
        return state.Invalid(BlockValidationResult::BLOCK_CHAINLOCK, "clsig-invalid-signers-count");
    }
    if (from == -1 || signers_count == 1) {
        // A part of a multi-quorum CLSIG signed by a single quorum
        std::pair<int, CQuorumCPtr> ret;
        clsig.signers.resize(signingActiveQuorumCount, false);
        if (!VerifyChainLockShare(clsig, pindexScan, idIn, ret, hash)) {
            LogPrint(BCLog::CHAINLOCKS, "CChainLocksHandler::%s -- invalid CLSIG (%s), peer=%d\n", __func__, clsig.ToString(), from);
            if (from != -1) {
                {
                    LOCK(cs_main);
                    peerman.ForgetTxHash(from, hash);
                }
            }
            return state.Invalid(BlockValidationResult::BLOCK_CHAINLOCK, "clsig-invalid-share-sig");
        }
        CInv clsigAggInv;
        {
            LOCK2(cs_main, cs);
            clsig.signers[ret.first] = true;
            if (std::count(clsig.signers.begin(), clsig.signers.end(), true) > 1) {
                // this should never happen
                LogPrint(BCLog::CHAINLOCKS, "CChainLocksHandler::%s -- ERROR in VerifyChainLockShare, CLSIG (%s), peer=%d\n", __func__, clsig.ToString(), from);
                if (from != -1) {
                    peerman.ForgetTxHash(from, hash);
                }
                return state.Invalid(BlockValidationResult::BLOCK_CHAINLOCK, "clsig-invalid-signer-count");
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
            peerman.RelayInv(clsigAggInv);
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
        if (!VerifyAggregatedChainLock(clsig, pindexScan, hash)) {
            LogPrint(BCLog::CHAINLOCKS, "CChainLocksHandler::%s -- invalid CLSIG (%s), peer=%d\n", __func__, clsig.ToString(), from);
            if (from != -1) {
                {
                    LOCK(cs_main);
                    peerman.ForgetTxHash(from, hash);
                }
            }
            return state.Invalid(BlockValidationResult::BLOCK_CHAINLOCK, "clsig-invalid-sig");
        }
            {
                LOCK(cs);
                bestChainLockCandidates[clsig.nHeight] = std::make_shared<const CChainLockSig>(clsig);
                mostRecentChainLockShare = clsig;
                TryUpdateBestChainLock(pindexScan);
            }
            peerman.RelayInv(clsigInv);
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
    {
        LOCK(cs);
        seenChainLocks.emplace(hash, TicksSinceEpoch<std::chrono::milliseconds>(SystemClock::now()));
    }
    if (from != -1) {
        LOCK(cs_main);
        peerman.ForgetTxHash(from, hash);
    }
    return true;
}

void CChainLocksHandler::NotifyHeaderTip(const CBlockIndex* pindexNew)
{
    LOCK(cs);

    auto it = bestChainLockCandidates.find(pindexNew->nHeight);
    if (it == bestChainLockCandidates.end()) {
        return;
    }
    if (it->second->blockHash != pindexNew->GetBlockHash()) {
        LogPrint(BCLog::CHAINLOCKS, "CChainLocksHandler::%s -- ignoring non-matching header at height=%d candidate=%s header=%s\n",
                __func__, pindexNew->nHeight, it->second->blockHash.ToString(), pindexNew->GetBlockHash().ToString());
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
            mostRecentChainLockShare = bestChainLockWithKnownBlock = CChainLockSig();
            bestChainLockBlockIndex = nullptr;
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
    uint256 lastSignedHash GUARDED_BY(cs);

public:
    mutable Mutex cs;
    void SetLastSigned(int _lastSignedHeight, const uint256& _lastSignedHash) EXCLUSIVE_LOCKS_REQUIRED(!cs)
    {
        LOCK(cs);
        lastSignedHeight = _lastSignedHeight;
        lastSignedHash = _lastSignedHash;
        attempt = ATTEMPT_START;
    }
    bool IsAlreadySigned(int _lastSignedHeight, const uint256& _lastSignedHash) const EXCLUSIVE_LOCKS_REQUIRED(!cs)
    {
        LOCK(cs);
        return lastSignedHeight == _lastSignedHeight && lastSignedHash == _lastSignedHash;
    }
    void BumpAttempt() EXCLUSIVE_LOCKS_REQUIRED(!cs)
    {
        LOCK(cs);
        ++attempt;
    }
    int GetAttempt() const EXCLUSIVE_LOCKS_REQUIRED(!cs)
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
        LogPrint(BCLog::CHAINLOCKS, "CChainLocksHandler: -- mnsync false\n");
        return;
    }
    const CBlockIndex* pindex;
    {
        LOCK(cs_main);
        int nActiveHeight = chainman.ActiveHeight()-SIGN_HEIGHT_OFFSET;
        nActiveHeight -= nActiveHeight%SIGN_HEIGHT_OFFSET;
        pindex = chainman.ActiveChain()[nActiveHeight];
        LogPrint(BCLog::CHAINLOCKS, "CChainLocksHandler::%s -- PRE trying to sign nActiveHeight=%d\n", __func__, nActiveHeight);
        if (!pindex || !pindex->pprev || !pindex->IsValid(BLOCK_VALID_SCRIPTS)) {
            LogPrint(BCLog::CHAINLOCKS, "CChainLocksHandle pindex not valid\n");
            return;
        }
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
        if (bestIndex && (!chainman.ActiveChain().Contains(bestIndex) || !bestIndex->IsValid(BLOCK_VALID_SCRIPTS))) {
            LogPrint(BCLog::CHAINLOCKS, "CChainLocksHandler:: -- best index not in active chain\n");
            return;
        }
    }
    {
        LOCK(cs);

        if (!isEnabled) {
            LogPrint(BCLog::CHAINLOCKS, "CChainLocksHandler:: -- not enabled\n");
            return;
        }
        if (signingState.IsAlreadySigned(nHeight, msgHash)) {
            // already signed this one
            LogPrint(BCLog::CHAINLOCKS, "CChainLocksHandler:: -- already signed\n");
            return;
        }

        if (bestChainLockWithKnownBlock.nHeight >= nHeight) {
            // already got the same CLSIG or a better one, reset and bail out
            signingState.SetLastSigned(bestChainLockWithKnownBlock.nHeight, bestChainLockWithKnownBlock.blockHash);
            LogPrint(BCLog::CHAINLOCKS, "CChainLocksHandler:: -- we have a clsig at this height or better already\n");
            return;
        }

        if (InternalHasConflictingChainLock(nHeight, msgHash)) {
            // don't sign if another conflicting CLSIG is already present. EnforceBestChainLock will later enforce
            // the correct chain.
            LogPrint(BCLog::CHAINLOCKS, "CChainLocksHandler:: -- conflicting internal chainlock\n");
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
            LogPrint(BCLog::CHAINLOCKS, "CChainLocksHandler:: -- quorum null %d\n", i);
            return;
        }
        if (!quorum->IsValidMember(proTxHash)) {
            LogPrint(BCLog::CHAINLOCKS, "CChainLocksHandler:: -- not member %i\n", i);
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
                __func__, nQuorumIndex, quorum->qc->quorumHash.ToString(), msgHash.ToString(), nHeight);
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
        signingState.SetLastSigned(nHeight, msgHash);
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

        auto it = mapSignedRequestIds.find(recoveredSig.getId());
        if (it == mapSignedRequestIds.end() || recoveredSig.getMsgHash() != it->second.second) {
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
        mapSignedRequestIds.erase(recoveredSig.getId());
    }
    BlockValidationState state;
    ProcessNewChainLock(-1, clsig, state, ::SerializeHash(clsig), recoveredSig.getId());
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
    for (auto it = sigChecked.begin(); it != sigChecked.end(); ) {
        if (TicksSinceEpoch<std::chrono::milliseconds>(SystemClock::now()) - it->second >= CLEANUP_SEEN_TIMEOUT) {
            it = sigChecked.erase(it);
        } else {
            ++it;
        }
    }
    for (auto it = recentChainLocks.begin(); it != recentChainLocks.end(); ) {
        if (bestChainLockBlockIndex != nullptr && it->first < bestChainLockBlockIndex->nHeight - RECENT_CHAINLOCKS_MAX) {
            it = recentChainLocks.erase(it);
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
