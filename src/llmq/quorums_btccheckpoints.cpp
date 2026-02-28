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
#include <net_processing.h>
#include <node/blockstorage.h>
#include <primitives/block.h>
#include <util/strencodings.h>
#include <util/time.h>
#include <validation.h>

#include <algorithm>

namespace llmq
{

static const std::string BTCCHECK_REQUESTID_PREFIX = "btcc";
static constexpr size_t MAX_SIGNED_REQUESTIDS = 4096;

CBTCCheckpointsHandler* btcCheckpointsHandler{nullptr};

static int32_t GetExpectedBTCCheckpointHeight(const ChainstateManager& chainman) EXCLUSIVE_LOCKS_REQUIRED(cs_main)
{
    // Strict height-window gating (absolute schedule):
    // - Emit exactly one checkpoint per BTCCHECK_PERIOD blocks
    // - Sign at heights where height % BTCCHECK_PERIOD == BTCCHECK_SIGN_OFFSET
    const int tip = chainman.ActiveHeight();
    static constexpr int PERIOD{BTCCHECK_PERIOD};
    static constexpr int SIGN_OFFSET{BTCCHECK_SIGN_OFFSET}; // within [0, PERIOD)

    // Compute the greatest height h <= tip such that h % PERIOD == SIGN_OFFSET.
    // Still enforce activation: don't return pre-activation heights.
    if (tip < SIGN_OFFSET) return -1;
    return tip - ((tip - SIGN_OFFSET) % PERIOD);
}

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

bool CBTCCheckpointsHandler::AlreadyHave(const uint256& hash) const
{
    LOCK(cs);
    return seenBTCCheckpointSigs.count(hash) != 0;
}

void CBTCCheckpointsHandler::Cleanup()
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

    for (auto it = seenBTCCheckpointSigs.begin(); it != seenBTCCheckpointSigs.end(); ) {
        if (TicksSinceEpoch<std::chrono::milliseconds>(SystemClock::now()) - it->second >= CLEANUP_SEEN_TIMEOUT) {
            it = seenBTCCheckpointSigs.erase(it);
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

    // prune height-indexed maps behind the best candidate window
    const int32_t bestHeight = bestCandidates.empty() ? -1 : bestCandidates.rbegin()->first;
    if (bestHeight >= 0) {
        const int32_t pruneBelow = std::max<int32_t>(0, bestHeight - RECENT_BTCCHECKPOINTS_MAX);
        for (auto it = recentBTCCheckpoints.begin(); it != recentBTCCheckpoints.end();) {
            if (it->first < pruneBelow) {
                it = recentBTCCheckpoints.erase(it);
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
        for (auto it = bestShares.begin(); it != bestShares.end();) {
            if (it->first < pruneBelow) {
                it = bestShares.erase(it);
            } else {
                ++it;
            }
        }
    }

    lastCleanupTime = TicksSinceEpoch<std::chrono::milliseconds>(SystemClock::now());
}

bool CBTCCheckpointsHandler::GetBTCCheckpointByHash(const uint256& hash, CBTCCheckpointSig& ret) const
{
    LOCK(cs);

    for (const auto& pair : bestCandidates) {
        if (pair.second && ::SerializeHash(*pair.second) == hash) {
            ret = *pair.second;
            return true;
        }
    }

    for (const auto& pair : bestShares) {
        for (const auto& pair2 : pair.second) {
            if (pair2.second && ::SerializeHash(*pair2.second) == hash) {
                ret = *pair2.second;
                return true;
            }
        }
    }

    for (const auto& pair : recentBTCCheckpoints) {
        if (::SerializeHash(pair.second) == hash) {
            ret = pair.second;
            return true;
        }
    }

    return false;
}

bool CBTCCheckpointsHandler::VerifyAggregatedBTCCheckpointNoCache(const CBTCCheckpointSig& btcsig, const CBlockIndex* pindexScan) const
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

void CBTCCheckpointsHandler::ProcessMessage(CNode* pfrom, const std::string& msg_type, CDataStream& vRecv)
{
    if (msg_type != NetMsgType::BTCCSIG) {
        return;
    }

    CBTCCheckpointSig btccsig;
    vRecv >> btccsig;
    const uint256 hash = ::SerializeHash(btccsig);

    const NodeId from = pfrom->GetId();
    PeerRef peer = peerman.GetPeerRef(from);
    if (peer) {
        peerman.AddKnownTx(*peer, hash);
    }
    {
        LOCK(cs_main);
        peerman.ReceivedResponse(from, hash);
    }
    auto forget_tx_hash = [&]() {
        LOCK(cs_main);
        peerman.ForgetTxHash(from, hash);
    };

    Cleanup();

    {
        LOCK(cs);
        if (seenBTCCheckpointSigs.count(hash) != 0) {
            // We already processed this object.
            // Must not call forget_tx_hash() under cs (it locks cs_main).
            // Defer handling outside the cs scope to avoid lock-order inversion.
            // (ChainLocks uses LOCK2(cs_main, cs) ordering elsewhere.)
            // Fall through to handle after unlocking.
            goto already_seen;
        }
    }
    goto not_seen;
already_seen:
    forget_tx_hash();
    return;
not_seen:

    const CBlockIndex* pindexScan{nullptr};
    int32_t expectedHeight{-1};
    {
        LOCK(cs_main);
        expectedHeight = GetExpectedBTCCheckpointHeight(chainman);
        pindexScan = chainman.m_blockman.LookupBlockIndex(btccsig.sysHash);
    }
    if (pindexScan == nullptr || pindexScan->nHeight != btccsig.nHeight) {
        forget_tx_hash();
        return;
    }
    {
        LOCK(cs_main);
        if (btccsig.nHeight < 0 || btccsig.nHeight > chainman.ActiveHeight()) {
            forget_tx_hash();
            return;
        }
        if (chainman.ActiveChain()[btccsig.nHeight] != pindexScan) {
            // Don't accept/relay checkpoint sigs for non-active forks.
            forget_tx_hash();
            return;
        }
        if (expectedHeight == -1 || btccsig.nHeight != expectedHeight) {
            // Strict height-window acceptance (ChainLocks parity).
            // NOTE: Do not mark as "seen" here; it might become valid later as the window advances.
            forget_tx_hash();
            return;
        }
    }

    const auto& llmqParams = Params().GetConsensus().llmqTypeChainLocks;
    const size_t signers_count = std::count(btccsig.signers.begin(), btccsig.signers.end(), true);
    if (btccsig.signers.size() != (size_t)llmqParams.signingActiveQuorumCount) {
        forget_tx_hash();
        return;
    }

    // Drop old material once we have a newer checkpoint (ChainLocks parity: "existing height" gating).
    {
        LOCK(cs);
        if (!bestCandidates.empty() && bestCandidates.rbegin()->first > btccsig.nHeight) {
            // Must not call forget_tx_hash() under cs (it locks cs_main).
            goto older_height;
        }
    }
    goto not_older;
older_height:
    forget_tx_hash();
    return;
not_older:

    if (signers_count == 1) {
        // share
        std::pair<int, CQuorumCPtr> ret;
        if (!VerifyBTCCheckpointShare(btccsig, pindexScan, uint256(), ret, hash)) {
            forget_tx_hash();
            return;
        }
        {
            LOCK(cs);
            seenBTCCheckpointSigs.emplace(hash, TicksSinceEpoch<std::chrono::milliseconds>(SystemClock::now()));
        }
        bool agg{false};
        uint256 agg_hash;
        {
            LOCK(cs);
            bestShares[btccsig.nHeight].emplace(ret.second, std::make_shared<const CBTCCheckpointSig>(btccsig));
            agg = TryUpdateBestBTCCheckpoint(pindexScan);
            if (agg) {
                auto it = bestCandidates.find(btccsig.nHeight);
                if (it != bestCandidates.end() && it->second) {
                    agg_hash = ::SerializeHash(*it->second);
                    seenBTCCheckpointSigs.emplace(agg_hash, TicksSinceEpoch<std::chrono::milliseconds>(SystemClock::now()));
                }
            }
        }
        if (agg && !agg_hash.IsNull()) {
            peerman.RelayInv(CInv(MSG_BTCCSIG, agg_hash));
        } else {
            // Relay partial shares to full nodes only, mirroring ChainLocks behavior.
            LOCK(cs_main);
            connman.ForEachNode([&](CNode* pnode) EXCLUSIVE_LOCKS_REQUIRED(::cs_main) {
                AssertLockHeld(::cs_main);
                const bool fSPV{pnode->m_bloom_filter_loaded.load()};
                if (!fSPV && pnode->CanRelay()) {
                    PeerRef peer = peerman.GetPeerRef(pnode->GetId());
                    if (peer) {
                        peerman.PushTxInventoryOther(*peer, CInv(MSG_BTCCSIG, hash));
                    }
                }
            });
        }
        // Always clear request bookkeeping on handled messages (ChainLocks parity).
        forget_tx_hash();
        return;
    }

    // aggregated
    if (!VerifyAggregatedBTCCheckpoint(btccsig, pindexScan)) {
        forget_tx_hash();
        return;
    }
    {
        LOCK(cs);
        seenBTCCheckpointSigs.emplace(hash, TicksSinceEpoch<std::chrono::milliseconds>(SystemClock::now()));
    }

    bool accepted{false};
    {
        LOCK(cs);
        auto it = bestCandidates.find(btccsig.nHeight);
        if (it == bestCandidates.end() || it->second == nullptr || it->second->sysHash != btccsig.sysHash) {
            bestCandidates[btccsig.nHeight] = std::make_shared<const CBTCCheckpointSig>(btccsig);
            AddRecentBTCCheckpoint(btccsig);
            accepted = true;
        }
    }

    if (accepted) {
        peerman.RelayInv(CInv(MSG_BTCCSIG, hash));
    }
    // Always clear request bookkeeping on handled messages (ChainLocks parity).
    forget_tx_hash();
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
        const int start = Params().GetConsensus().nCLReceiptStartBlock;
        if (chainman.ActiveHeight() < start) {
            return;
        }
        // We want a propagation buffer between signing and mining the carrier block.
        // Sign at epoch offset +BTCCHECK_SIGN_OFFSET and embed at +BTCCHECK_CARRIER_OFFSET (buffer BTCCHECK_PROP_BUFFER).
        const int nActiveHeight = GetExpectedBTCCheckpointHeight(chainman);
        if (nActiveHeight == -1) return;
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

bool CBTCCheckpointsHandler::VerifyBTCCheckpointShare(const CBTCCheckpointSig& btcsig, const CBlockIndex* pindexScan, const uint256& idIn, std::pair<int, CQuorumCPtr>& ret, const uint256& hash) const
{
    {
        LOCK(cs);
        if (sigChecked.find(hash) != sigChecked.end()) {
            return true;
        }
    }
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
        if (!idIn.IsNull() && requestId != idIn) continue;
        if (idIn.IsNull()) {
            // For p2p-delivered shares, determine quorum by signer bit (must be exactly one).
            if (btcsig.signers.size() != (size_t)signingActiveQuorumCount) return false;
            if (!btcsig.signers[i]) continue;
        }

        const uint256 signHash = llmq::BuildSignHash(quorum->qc->quorumHash, requestId, msgHash);
        if (!btcsig.sig.VerifyInsecure(quorum->qc->quorumPublicKey, signHash)) {
            return false;
        }

        ret = std::make_pair((int)i, quorum);
        {
            LOCK(cs);
            sigChecked.emplace(hash, TicksSinceEpoch<std::chrono::milliseconds>(SystemClock::now()));
        }
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
    Cleanup();

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
    int32_t expectedHeight{-1};
    {
        LOCK(cs_main);
        expectedHeight = GetExpectedBTCCheckpointHeight(chainman);
        pindexScan = chainman.m_blockman.LookupBlockIndex(recoveredSig.getMsgHash());
    }
    if (pindexScan == nullptr) return;
    {
        LOCK(cs_main);
        if (expectedHeight == -1) return;
        if (pindexScan->nHeight < 0 || pindexScan->nHeight > chainman.ActiveHeight()) return;
        if (chainman.ActiveChain()[pindexScan->nHeight] != pindexScan) {
            // Don't accept recovered sigs for non-active forks.
            return;
        }
    }

    // Accept recovered sigs originating from peers even if we didn't sign locally.
    // msgHash is sysHash, so derive height/hash directly from it.
    if (!haveSignedTarget) {
        share.nHeight = pindexScan->nHeight;
        share.sysHash = pindexScan->GetBlockHash();
    }
    if (recoveredSig.getMsgHash() != share.sysHash || pindexScan->nHeight != share.nHeight) return;
    if (share.nHeight != expectedHeight) {
        // Strict height-window acceptance (ChainLocks parity).
        return;
    }

    {
        LOCK(cs);
        // If we already have an aggregated candidate for this height, drop further processing.
        // This mirrors ChainLocks behavior of ignoring superseded/stale material.
        auto itCand = bestCandidates.find(share.nHeight);
        if (itCand != bestCandidates.end() && itCand->second != nullptr && itCand->second->sysHash == share.sysHash) {
            return;
        }
    }

    std::pair<int, CQuorumCPtr> ret;
    share.sig = recoveredSig.sig.Get();
    share.signers.clear();
    const auto& llmqParams = Params().GetConsensus().llmqTypeChainLocks;
    share.signers.resize(llmqParams.signingActiveQuorumCount, false);

    // Verify against the recoveredSig id. The share hash used for caching is computed before the signer bit is set;
    // after verification we re-hash the fully-formed share for correct INV/getdata.
    const uint256 share_hash_pre = ::SerializeHash(share);
    if (!VerifyBTCCheckpointShare(share, pindexScan, recoveredSig.getId(), ret, share_hash_pre)) {
        return;
    }
    share.signers[ret.first] = true;
    const uint256 share_hash = ::SerializeHash(share);
    {
        LOCK(cs);
        // Mark the fully-formed share as seen so AlreadyHave/GetData works with the relayed INV.
        seenBTCCheckpointSigs.emplace(share_hash, TicksSinceEpoch<std::chrono::milliseconds>(SystemClock::now()));
        sigChecked.emplace(share_hash, TicksSinceEpoch<std::chrono::milliseconds>(SystemClock::now()));
    }

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
    if (agg) {
        CBTCCheckpointSig best;
        {
            LOCK(cs);
            auto it = bestCandidates.find(share.nHeight);
            if (it != bestCandidates.end() && it->second) {
                best = *it->second;
            }
        }
        if (!best.IsNull()) {
            const uint256 hash = ::SerializeHash(best);
            {
                LOCK(cs);
                seenBTCCheckpointSigs.emplace(hash, TicksSinceEpoch<std::chrono::milliseconds>(SystemClock::now()));
            }
            peerman.RelayInv(CInv(MSG_BTCCSIG, hash));
        }
    } else {
        // Relay partial shares to full nodes only, mirroring ChainLocks behavior.
        LOCK(cs_main);
        connman.ForEachNode([&](CNode* pnode) EXCLUSIVE_LOCKS_REQUIRED(::cs_main) {
            AssertLockHeld(::cs_main);
            bool fSPV{pnode->m_bloom_filter_loaded.load()};
            if (!fSPV && pnode->CanRelay()) {
                PeerRef peer = peerman.GetPeerRef(pnode->GetId());
                if (peer) {
                    peerman.PushTxInventoryOther(*peer, CInv(MSG_BTCCSIG, share_hash));
                }
            }
        });
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
    const uint256 hash = ::SerializeHash(btcsig);
    {
        LOCK(cs);
        if (sigChecked.find(hash) != sigChecked.end()) {
            return true;
        }
    }
    const bool ok = VerifyAggregatedBTCCheckpointNoCache(btcsig, pindexScan);
    if (ok) {
        LOCK(cs);
        sigChecked.emplace(hash, TicksSinceEpoch<std::chrono::milliseconds>(SystemClock::now()));
    }
    return ok;
}

} // namespace llmq

