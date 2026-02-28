// Copyright (c) 2026 The Syscoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <llmq/quorums_btccheckpoints.h>

#include <llmq/quorums.h>
#include <llmq/quorums_commitment.h>
#include <llmq/quorums_utils.h>
#include <chain.h>
#include <chainparams.h>
#include <evo/deterministicmns.h>
#include <logging.h>
#include <masternode/activemasternode.h>
#include <masternode/masternodesync.h>
#include <node/blockstorage.h>
#include <primitives/block.h>
#include <util/strencodings.h>
#include <validation.h>

#include <algorithm>

namespace llmq
{

static const std::string BTCCHECK_REQUESTID_PREFIX = "btcc";
static constexpr size_t MAX_SIGNED_REQUESTIDS = 4096;

CBTCCheckpointsHandler* btcCheckpointsHandler{nullptr};

bool CBTCCheckpointSig::IsNull() const
{
    return nHeight == -1 && sysHash == uint256();
}

std::string CBTCCheckpointSig::ToString() const
{
    return strprintf("CBTCCheckpointSig(nHeight=%d, sysHash=%s, signers: hex=%s size=%d count=%d)",
                     nHeight, sysHash.ToString(),
                     CLLMQUtils::ToHexStr(signers), signers.size(), std::count(signers.begin(), signers.end(), true));
}

CBTCCheckpointsHandler::CBTCCheckpointsHandler(CConnman& _connman, PeerManager& _peerman, ChainstateManager& _chainman) :
    chainman(_chainman),
    connman(_connman),
    peerman(_peerman)
{
}

void CBTCCheckpointsHandler::Start()
{
    quorumSigningManager->RegisterRecoveredSigsListener(this);
}

void CBTCCheckpointsHandler::Stop()
{
    quorumSigningManager->UnregisterRecoveredSigsListener(this);
}

void CBTCCheckpointsHandler::TrySignBTCCheckpointTip()
{
    if (!fMasternodeMode) return;
    if (!masternodeSync.IsBlockchainSynced()) return;

    // Policy: for now, only enabled on regtest-like setups.
    // This will later be replaced with a real BTC header-chain membership check.
    if (!Params().MineBlocksOnDemand()) return;

    const CBlockIndex* pindex{nullptr};
    {
        LOCK(cs_main);
        int nActiveHeight = chainman.ActiveHeight() - 10;
        if (nActiveHeight < 0) return;
        nActiveHeight -= nActiveHeight % 10;
        pindex = chainman.ActiveChain()[nActiveHeight];
        if (!pindex || !pindex->pprev || !pindex->IsValid(BLOCK_VALID_SCRIPTS)) {
            return;
        }
    }

    // Keep internal state bounded. We only need a recent window for aggregation/replay.
    // This is intentionally simple (no timers) to minimize behavioral changes.
    {
        const int32_t pruneBelow = std::max<int32_t>(0, pindex->nHeight - RECENT_BTCCHECKPOINTS_MAX - 16);
        LOCK(cs);
        for (auto it = bestShares.begin(); it != bestShares.end();) {
            if (it->first < pruneBelow) {
                it = bestShares.erase(it);
            } else {
                ++it;
            }
        }
        for (auto it = bestCandidates.begin(); it != bestCandidates.end();) {
            if (it->first < pruneBelow) {
                it = bestCandidates.erase(it);
            } else {
                ++it;
            }
        }
    }

    CBTCCheckpointSig want;
    want.nHeight = pindex->nHeight;
    want.sysHash = pindex->GetBlockHash();

    // CL-like: sign msgHash = sysHash, while requestId binds height and quorum.
    const uint256 msgHash = want.sysHash;

    const auto& consensus = Params().GetConsensus();
    const auto& llmqParams = consensus.llmqTypeChainLocks;
    const auto& signingActiveQuorumCount = llmqParams.signingActiveQuorumCount;

    const auto quorums_scanned = llmq::quorumManager->ScanQuorums(pindex, signingActiveQuorumCount);
    if (quorums_scanned.empty()) return;

    auto proTxHash = WITH_LOCK(activeMasternodeInfoCs, return activeMasternodeInfo.proTxHash);

    LOCK(cs);
    if (lastSignedHeight == want.nHeight && lastSignedSysHash == want.sysHash) {
        auto it = bestCandidates.find(want.nHeight);
        if (it != bestCandidates.end() && it->second && it->second->sysHash == want.sysHash) {
            // Suppress redundant signing only after this exact target was already aggregated.
            return;
        }
    }
    bool queued_sign = false;
    for (size_t i = 0; i < quorums_scanned.size(); ++i) {
        const CQuorumCPtr& quorum = quorums_scanned[i];
        if (quorum == nullptr) return;
        if (!quorum->IsValidMember(proTxHash)) continue;

        const uint256 requestId = ::SerializeHash(std::make_tuple(BTCCHECK_REQUESTID_PREFIX, want.nHeight, quorum->qc->quorumHash));
        mapSignedRequestIds.emplace(requestId, std::make_pair(want.nHeight, want.sysHash));
        while (mapSignedRequestIds.size() > MAX_SIGNED_REQUESTIDS) {
            mapSignedRequestIds.erase(mapSignedRequestIds.begin());
        }
        quorumSigningManager->AsyncSignIfMember(requestId, msgHash, quorum->qc->quorumHash);
        queued_sign = true;
    }
    if (queued_sign) {
        lastSignedHeight = want.nHeight;
        lastSignedSysHash = want.sysHash;
    }
}

bool CBTCCheckpointsHandler::VerifyBTCCheckpointShare(const CBTCCheckpointSig& btcsig, const CBlockIndex* pindexScan, const uint256& idIn, std::pair<int, CQuorumCPtr>& ret) const
{
    const auto& consensus = Params().GetConsensus();
    const auto& llmqParams = consensus.llmqTypeChainLocks;
    const auto& signingActiveQuorumCount = llmqParams.signingActiveQuorumCount;

    const auto quorums_scanned = llmq::quorumManager->ScanQuorums(pindexScan, signingActiveQuorumCount);
    if (quorums_scanned.empty()) return false;

    const uint256 msgHash = btcsig.sysHash;

    for (size_t i = 0; i < quorums_scanned.size(); ++i) {
        const CQuorumCPtr& quorum = quorums_scanned[i];
        if (quorum == nullptr) return false;

        const uint256 requestId = ::SerializeHash(std::make_tuple(BTCCHECK_REQUESTID_PREFIX, btcsig.nHeight, quorum->qc->quorumHash));
        if (requestId != idIn) continue;

        const uint256 signHash = llmq::BuildSignHash(quorum->qc->quorumHash, requestId, msgHash);
        if (!btcsig.sig.VerifyInsecure(quorum->qc->quorumPublicKey, signHash)) {
            return false;
        }

        ret = std::make_pair((int)i, quorum);
        return true;
    }
    return false;
}

bool CBTCCheckpointsHandler::TryUpdateBestBTCCheckpoint(const CBlockIndex* pindexScan)
{
    const int32_t height = pindexScan->nHeight;
    auto it2 = bestShares.find(height);
    if (it2 == bestShares.end()) return false;

    const auto& llmqParams = Params().GetConsensus().llmqTypeChainLocks;
    const size_t threshold = llmqParams.signingActiveQuorumCount / 2 + 1;

    std::vector<CBLSSignature> sigs;
    CBTCCheckpointSig agg;

    for (const auto& pair : it2->second) {
        const auto& share = pair.second;
        if (share->sysHash != pindexScan->GetBlockHash()) continue;

        sigs.emplace_back(share->sig);
        if (agg.IsNull()) {
            agg = *share;
        } else {
            std::transform(agg.signers.begin(), agg.signers.end(), share->signers.begin(), agg.signers.begin(), std::logical_or<bool>());
        }
        if (sigs.size() >= threshold) {
            agg.sig = CBLSSignature::AggregateInsecure(sigs);
            bestCandidates[height] = std::make_shared<const CBTCCheckpointSig>(agg);
            AddRecentBTCCheckpoint(agg);
            return true;
        }
    }

    return false;
}

void CBTCCheckpointsHandler::AddRecentBTCCheckpoint(const CBTCCheckpointSig& btcsig)
{
    // Match ChainLocks semantics: newer aggregates can replace older ones at same height.
    recentBTCCheckpoints[btcsig.nHeight] = btcsig;
    while ((int32_t)recentBTCCheckpoints.size() > RECENT_BTCCHECKPOINTS_MAX) {
        // prune lowest height entry (map is ascending by height)
        recentBTCCheckpoints.erase(recentBTCCheckpoints.begin());
    }
}

void CBTCCheckpointsHandler::HandleNewRecoveredSig(const CRecoveredSig& recoveredSig)
{
    CBTCCheckpointSig share;
    bool haveSignedTarget{false};
    {
        LOCK(cs);
        auto it = mapSignedRequestIds.find(recoveredSig.getId());
        if (it != mapSignedRequestIds.end()) {
            haveSignedTarget = true;
            share.nHeight = it->second.first;
            share.sysHash = it->second.second;
            mapSignedRequestIds.erase(it);
        }
    }

    const CBlockIndex* pindexScan;
    {
        LOCK(cs_main);
        pindexScan = chainman.m_blockman.LookupBlockIndex(recoveredSig.getMsgHash());
    }
    if (pindexScan == nullptr) return;

    // Accept recovered sigs originating from peers even if we didn't sign locally.
    // msgHash is sysHash, so derive height/hash directly from it.
    if (!haveSignedTarget) {
        share.nHeight = pindexScan->nHeight;
        share.sysHash = pindexScan->GetBlockHash();
    }
    if (recoveredSig.getMsgHash() != share.sysHash || pindexScan->nHeight != share.nHeight) return;

    {
        LOCK(cs);
        // If we already have an aggregated candidate for this height, drop further processing.
        // This mirrors ChainLocks behavior of ignoring superseded/stale material.
        auto itCand = bestCandidates.find(share.nHeight);
        if (itCand != bestCandidates.end() && itCand->second != nullptr) {
            return;
        }
    }

    std::pair<int, CQuorumCPtr> ret;
    share.sig = recoveredSig.sig.Get();
    share.signers.clear();
    const auto& llmqParams = Params().GetConsensus().llmqTypeChainLocks;
    share.signers.resize(llmqParams.signingActiveQuorumCount, false);

    if (!VerifyBTCCheckpointShare(share, pindexScan, recoveredSig.getId(), ret)) {
        return;
    }
    share.signers[ret.first] = true;

    bool agg{false};
    std::string agg_str;
    {
        LOCK(cs);
        bestShares[share.nHeight].emplace(ret.second, std::make_shared<const CBTCCheckpointSig>(share));
        agg = TryUpdateBestBTCCheckpoint(pindexScan);
        if (agg) {
            auto it = bestCandidates.find(share.nHeight);
            if (it != bestCandidates.end() && it->second) {
                agg_str = it->second->ToString();
            }
        }
    }
    if (!agg_str.empty()) {
        LogPrint(BCLog::CHAINLOCKS, "CBTCCheckpointsHandler -- aggregated btcc (%s)\n", agg_str);
    }
}

CBTCCheckpointSig CBTCCheckpointsHandler::GetMostRecentBTCCheckpoint() const
{
    LOCK(cs);
    if (recentBTCCheckpoints.empty()) {
        return CBTCCheckpointSig();
    }
    return recentBTCCheckpoints.rbegin()->second;
}

CBTCCheckpointSig CBTCCheckpointsHandler::GetBestBTCCheckpoint() const
{
    LOCK(cs);
    if (bestCandidates.empty()) {
        return CBTCCheckpointSig();
    }
    auto it = bestCandidates.rbegin();
    if (!it->second) {
        return CBTCCheckpointSig();
    }
    return *it->second;
}

std::map<CQuorumCPtr, CBTCCheckpointSigCPtr> CBTCCheckpointsHandler::GetBestBTCCheckpointShares() const
{
    LOCK(cs);
    if (bestCandidates.empty()) {
        return {};
    }
    auto it = bestCandidates.rbegin();
    if (!it->second) {
        return {};
    }
    auto jt = bestShares.find(it->first);
    if (jt == bestShares.end()) {
        return {};
    }
    return jt->second;
}

bool CBTCCheckpointsHandler::GetRecentBTCCheckpointByHeight(int32_t nHeight, CBTCCheckpointSig& ret) const
{
    LOCK(cs);
    auto it = recentBTCCheckpoints.find(nHeight);
    if (it == recentBTCCheckpoints.end()) return false;
    ret = it->second;
    return true;
}

bool CBTCCheckpointsHandler::VerifyAggregatedBTCCheckpoint(const CBTCCheckpointSig& btcsig, const CBlockIndex* pindexScan) const
{
    const auto& consensus = Params().GetConsensus();
    const auto& llmqParams = consensus.llmqTypeChainLocks;
    const auto& signingActiveQuorumCount = llmqParams.signingActiveQuorumCount;

    if (btcsig.signers.size() != (size_t)signingActiveQuorumCount) return false;
    if (std::count(btcsig.signers.begin(), btcsig.signers.end(), true) < (signingActiveQuorumCount / 2 + 1)) return false;

    const auto quorums_scanned = llmq::quorumManager->ScanQuorums(pindexScan, signingActiveQuorumCount);
    if (quorums_scanned.empty()) return false;

    const uint256 msgHash = btcsig.sysHash;

    std::vector<uint256> hashes;
    std::vector<CBLSPublicKey> quorumPublicKeys;
    for (size_t i = 0; i < quorums_scanned.size(); ++i) {
        const CQuorumCPtr& quorum = quorums_scanned[i];
        if (quorum == nullptr) return false;
        if (!btcsig.signers[i]) continue;
        quorumPublicKeys.emplace_back(quorum->qc->quorumPublicKey);
        const uint256 requestId = ::SerializeHash(std::make_tuple(BTCCHECK_REQUESTID_PREFIX, btcsig.nHeight, quorum->qc->quorumHash));
        const uint256 signHash = llmq::BuildSignHash(quorum->qc->quorumHash, requestId, msgHash);
        hashes.emplace_back(signHash);
    }
    return btcsig.sig.VerifyInsecureAggregated(quorumPublicKeys, hashes);
}

} // namespace llmq

