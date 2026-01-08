// Copyright (c) 2018-2025 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <llmq/signing_shares.h>

#include <active/masternode.h>
#include <bls/bls_batchverifier.h>
#include <evo/deterministicmns.h>
#include <llmq/commitment.h>
#include <llmq/options.h>
#include <llmq/quorumsman.h>
#include <llmq/signhash.h>
#include <llmq/signing.h>
#include <spork.h>
#include <util/irange.h>
#include <util/underlying.h>

#include <chainparams.h>
#include <net_processing.h>
#include <netmessagemaker.h>
#include <util/thread.h>
#include <util/time.h>
#include <validation.h>

#include <cxxtimer.hpp>

namespace llmq
{
void CSigShare::UpdateKey()
{
    key.first = this->buildSignHash().Get();
    key.second = quorumMember;
}

std::string CSigSesAnn::ToString() const
{
    return strprintf("sessionId=%d, llmqType=%d, quorumHash=%s, id=%s, msgHash=%s",
                     sessionId, ToUnderlying(getLlmqType()), getQuorumHash().ToString(), getId().ToString(), getMsgHash().ToString());
}

void CSigSharesInv::Merge(const CSigSharesInv& inv2)
{
    for (const auto i : irange::range(inv.size())) {
        if (inv2.inv[i]) {
            inv[i] = inv2.inv[i];
        }
    }
}

size_t CSigSharesInv::CountSet() const
{
    return (size_t)std::count(inv.begin(), inv.end(), true);
}

std::string CSigSharesInv::ToString() const
{
    std::string str = "(";
    bool first = true;
    for (const auto i : irange::range(inv.size())) {
        if (!inv[i]) {
            continue;
        }

        if (!first) {
            str += ",";
        }
        first = false;
        str += strprintf("%d", i);
    }
    str += ")";
    return str;
}

void CSigSharesInv::Init(size_t size)
{
    inv.resize(size, false);
}

void CSigSharesInv::Set(uint16_t quorumMember, bool v)
{
    assert(quorumMember < inv.size());
    inv[quorumMember] = v;
}

void CSigSharesInv::SetAll(bool v)
{
    std::fill(inv.begin(), inv.end(), v);
}

std::string CBatchedSigShares::ToInvString() const
{
    CSigSharesInv inv;
    // we use 400 here no matter what the real size is. We don't really care about that size as we just want to call ToString()
    inv.Init(400);
    for (const auto& sigShare : sigShares) {
        inv.inv[sigShare.first] = true;
    }
    return inv.ToString();
}

static void InitSession(CSigSharesNodeState::Session& s, const llmq::SignHash& signHash, CSigBase from)
{
    const auto& llmq_params_opt = Params().GetLLMQ(from.getLlmqType());
    assert(llmq_params_opt.has_value());
    const auto& llmq_params = llmq_params_opt.value();

    s.llmqType = from.getLlmqType();
    s.quorumHash = from.getQuorumHash();
    s.id = from.getId();
    s.msgHash = from.getMsgHash();
    s.signHash = signHash;
    s.announced.Init((size_t)llmq_params.size);
    s.requested.Init((size_t)llmq_params.size);
    s.knows.Init((size_t)llmq_params.size);
}

CSigSharesNodeState::Session& CSigSharesNodeState::GetOrCreateSessionFromShare(const llmq::CSigShare& sigShare)
{
    auto& s = sessions[sigShare.GetSignHash()];
    if (s.announced.inv.empty()) {
        InitSession(s, sigShare.buildSignHash(), sigShare);
    }
    return s;
}

CSigSharesNodeState::Session& CSigSharesNodeState::GetOrCreateSessionFromAnn(const llmq::CSigSesAnn& ann)
{
    auto signHash = ann.buildSignHash();
    auto& s = sessions[signHash.Get()];
    if (s.announced.inv.empty()) {
        InitSession(s, signHash, ann);
    }
    return s;
}

CSigSharesNodeState::Session* CSigSharesNodeState::GetSessionBySignHash(const uint256& signHash)
{
    auto it = sessions.find(signHash);
    if (it == sessions.end()) {
        return nullptr;
    }
    return &it->second;
}

CSigSharesNodeState::Session* CSigSharesNodeState::GetSessionByRecvId(uint32_t sessionId)
{
    auto it = sessionByRecvId.find(sessionId);
    if (it == sessionByRecvId.end()) {
        return nullptr;
    }
    return it->second;
}

bool CSigSharesNodeState::GetSessionInfoByRecvId(uint32_t sessionId, SessionInfo& retInfo)
{
    const auto* s = GetSessionByRecvId(sessionId);
    if (s == nullptr) {
        return false;
    }
    retInfo.llmqType = s->llmqType;
    retInfo.quorumHash = s->quorumHash;
    retInfo.id = s->id;
    retInfo.msgHash = s->msgHash;
    retInfo.signHash = s->signHash;
    retInfo.quorum = s->quorum;

    return true;
}

void CSigSharesNodeState::RemoveSession(const uint256& signHash)
{
    if (const auto it = sessions.find(signHash); it != sessions.end()) {
        sessionByRecvId.erase(it->second.recvSessionId);
        sessions.erase(it);
    }
    requestedSigShares.EraseAllForSignHash(signHash);
    pendingIncomingSigShares.EraseAllForSignHash(signHash);
}

//////////////////////

CSigSharesManager::CSigSharesManager(CConnman& connman, CChainState& chainstate, CSigningManager& _sigman,
                                     PeerManager& peerman, const CActiveMasternodeManager& mn_activeman,
                                     const CQuorumManager& _qman, const CSporkManager& sporkman) :
    m_connman{connman},
    m_chainstate{chainstate},
    sigman{_sigman},
    m_peerman{peerman},
    m_mn_activeman{mn_activeman},
    qman{_qman},
    m_sporkman{sporkman}
{
    workInterrupt.reset();
}

CSigSharesManager::~CSigSharesManager() = default;

void CSigSharesManager::Start()
{
    // can't start if threads are already running
    if (housekeepingThread.joinable() || dispatcherThread.joinable()) {
        assert(false);
    }

    // Initialize worker pool
    int workerCount = std::clamp(static_cast<int>(std::thread::hardware_concurrency() / 2), 1, 4);
    workerPool.resize(workerCount);
    RenameThreadPool(workerPool, "sigsh-work");

    // Start housekeeping thread
    housekeepingThread = std::thread(&util::TraceThread, "sigsh-maint",
        [this] { HousekeepingThreadMain(); });

    // Start dispatcher thread
    dispatcherThread = std::thread(&util::TraceThread, "sigsh-dispat",
        [this] { WorkDispatcherThreadMain(); });
}

void CSigSharesManager::Stop()
{
    // make sure to call InterruptWorkerThread() first
    if (!workInterrupt) {
        assert(false);
    }

    // Join threads FIRST to stop any pending push() calls
    if (housekeepingThread.joinable()) {
        housekeepingThread.join();
    }
    if (dispatcherThread.joinable()) {
        dispatcherThread.join();
    }

    // Then stop worker pool (now safe, no more push() calls)
    workerPool.clear_queue();
    workerPool.stop(true);
}

void CSigSharesManager::RegisterRecoveryInterface()
{
    sigman.RegisterRecoveredSigsListener(this);
}

void CSigSharesManager::UnregisterRecoveryInterface()
{
    sigman.UnregisterRecoveredSigsListener(this);
}

void CSigSharesManager::InterruptWorkerThread()
{
    workInterrupt();
}

void CSigSharesManager::ProcessMessage(const CNode& pfrom, const std::string& msg_type, CDataStream& vRecv)
{
    // non-masternodes are not interested in sigshares
    if (m_mn_activeman.GetProTxHash().IsNull()) return;

    if (m_sporkman.IsSporkActive(SPORK_21_QUORUM_ALL_CONNECTED) && msg_type == NetMsgType::QSIGSHARE) {
        std::vector<CSigShare> receivedSigShares;
        vRecv >> receivedSigShares;

        if (receivedSigShares.size() > MAX_MSGS_SIG_SHARES) {
            LogPrint(BCLog::LLMQ_SIGS, "CSigSharesManager::%s -- too many sigs in QSIGSHARE message. cnt=%d, max=%d, node=%d\n", __func__, receivedSigShares.size(), MAX_MSGS_SIG_SHARES, pfrom.GetId());
            BanNode(pfrom.GetId());
            return;
        }

        for (const auto& sigShare : receivedSigShares) {
            ProcessMessageSigShare(pfrom.GetId(), sigShare);
        }
    }

    if (msg_type == NetMsgType::QSIGSESANN) {
        std::vector<CSigSesAnn> msgs;
        vRecv >> msgs;
        if (msgs.size() > MAX_MSGS_CNT_QSIGSESANN) {
            LogPrint(BCLog::LLMQ_SIGS, "CSigSharesManager::%s -- too many announcements in QSIGSESANN message. cnt=%d, max=%d, node=%d\n", __func__, msgs.size(), MAX_MSGS_CNT_QSIGSESANN, pfrom.GetId());
            BanNode(pfrom.GetId());
            return;
        }
        if (!ranges::all_of(msgs,
                            [this, &pfrom](const auto& ann){ return ProcessMessageSigSesAnn(pfrom, ann); })) {
            BanNode(pfrom.GetId());
            return;
        }
    } else if (msg_type == NetMsgType::QSIGSHARESINV) {
        std::vector<CSigSharesInv> msgs;
        vRecv >> msgs;
        if (msgs.size() > MAX_MSGS_CNT_QSIGSHARESINV) {
            LogPrint(BCLog::LLMQ_SIGS, "CSigSharesManager::%s -- too many invs in QSIGSHARESINV message. cnt=%d, max=%d, node=%d\n", __func__, msgs.size(), MAX_MSGS_CNT_QSIGSHARESINV, pfrom.GetId());
            BanNode(pfrom.GetId());
            return;
        }
        if (!ranges::all_of(msgs,
                            [this, &pfrom](const auto& inv){ return ProcessMessageSigSharesInv(pfrom, inv); })) {
            BanNode(pfrom.GetId());
            return;
        }
    } else if (msg_type == NetMsgType::QGETSIGSHARES) {
        std::vector<CSigSharesInv> msgs;
        vRecv >> msgs;
        if (msgs.size() > MAX_MSGS_CNT_QGETSIGSHARES) {
            LogPrint(BCLog::LLMQ_SIGS, "CSigSharesManager::%s -- too many invs in QGETSIGSHARES message. cnt=%d, max=%d, node=%d\n", __func__, msgs.size(), MAX_MSGS_CNT_QGETSIGSHARES, pfrom.GetId());
            BanNode(pfrom.GetId());
            return;
        }
        if (!ranges::all_of(msgs,
                            [this, &pfrom](const auto& inv){ return ProcessMessageGetSigShares(pfrom, inv); })) {
            BanNode(pfrom.GetId());
            return;
        }
    } else if (msg_type == NetMsgType::QBSIGSHARES) {
        std::vector<CBatchedSigShares> msgs;
        vRecv >> msgs;
        size_t totalSigsCount = 0;
        for (const auto& bs : msgs) {
            totalSigsCount += bs.sigShares.size();
        }
        if (totalSigsCount > MAX_MSGS_TOTAL_BATCHED_SIGS) {
            LogPrint(BCLog::LLMQ_SIGS, "CSigSharesManager::%s -- too many sigs in QBSIGSHARES message. cnt=%d, max=%d, node=%d\n", __func__, msgs.size(), MAX_MSGS_TOTAL_BATCHED_SIGS, pfrom.GetId());
            BanNode(pfrom.GetId());
            return;
        }
        if (!ranges::all_of(msgs,
                            [this, &pfrom](const auto& bs){ return ProcessMessageBatchedSigShares(pfrom, bs); })) {
            BanNode(pfrom.GetId());
            return;
        }
    }
}

bool CSigSharesManager::ProcessMessageSigSesAnn(const CNode& pfrom, const CSigSesAnn& ann)
{
    auto llmqType = ann.getLlmqType();
    if (!Params().GetLLMQ(llmqType).has_value()) {
        return false;
    }
    if (ann.getSessionId() == UNINITIALIZED_SESSION_ID || ann.getQuorumHash().IsNull() || ann.getId().IsNull() || ann.getMsgHash().IsNull()) {
        return false;
    }

    LogPrint(BCLog::LLMQ_SIGS, "CSigSharesManager::%s -- ann={%s}, node=%d\n", __func__, ann.ToString(), pfrom.GetId());

    auto quorum = qman.GetQuorum(llmqType, ann.getQuorumHash());
    if (!quorum) {
        // TODO should we ban here?
        LogPrint(BCLog::LLMQ_SIGS, "CSigSharesManager::%s -- quorum %s not found, node=%d\n", __func__,
                  ann.getQuorumHash().ToString(), pfrom.GetId());
        return true; // let's still try other announcements from the same message
    }

    LOCK(cs);
    auto& nodeState = nodeStates[pfrom.GetId()];
    auto& session = nodeState.GetOrCreateSessionFromAnn(ann);
    nodeState.sessionByRecvId.erase(session.recvSessionId);
    nodeState.sessionByRecvId.erase(ann.getSessionId());
    session.recvSessionId = ann.getSessionId();
    session.quorum = quorum;
    nodeState.sessionByRecvId.try_emplace(ann.getSessionId(), &session);

    return true;
}

bool CSigSharesManager::VerifySigSharesInv(Consensus::LLMQType llmqType, const CSigSharesInv& inv)
{
    const auto& llmq_params_opt = Params().GetLLMQ(llmqType);
    return llmq_params_opt.has_value() && (inv.inv.size() == size_t(llmq_params_opt->size));
}

bool CSigSharesManager::ProcessMessageSigSharesInv(const CNode& pfrom, const CSigSharesInv& inv)
{
    CSigSharesNodeState::SessionInfo sessionInfo;
    if (!GetSessionInfoByRecvId(pfrom.GetId(), inv.sessionId, sessionInfo)) {
        return true;
    }

    if (!VerifySigSharesInv(sessionInfo.llmqType, inv)) {
        return false;
    }

    // TODO for PoSe, we should consider propagating shares even if we already have a recovered sig
    if (sigman.HasRecoveredSigForSession(sessionInfo.signHash.Get())) {
        return true;
    }

    LogPrint(BCLog::LLMQ_SIGS, "CSigSharesManager::%s -- signHash=%s, inv={%s}, node=%d\n", __func__,
            sessionInfo.signHash.ToString(), inv.ToString(), pfrom.GetId());

    if (!sessionInfo.quorum->HasVerificationVector()) {
        // TODO we should allow to ask other nodes for the quorum vvec if we missed it in the DKG
        LogPrint(BCLog::LLMQ_SIGS, "CSigSharesManager::%s -- we don't have the quorum vvec for %s, not requesting sig shares. node=%d\n", __func__,
                  sessionInfo.quorumHash.ToString(), pfrom.GetId());
        return true;
    }

    LOCK(cs);
    auto& nodeState = nodeStates[pfrom.GetId()];
    auto* session = nodeState.GetSessionByRecvId(inv.sessionId);
    if (session == nullptr) {
        return true;
    }
    session->announced.Merge(inv);
    session->knows.Merge(inv);
    return true;
}

bool CSigSharesManager::ProcessMessageGetSigShares(const CNode& pfrom, const CSigSharesInv& inv)
{
    CSigSharesNodeState::SessionInfo sessionInfo;
    if (!GetSessionInfoByRecvId(pfrom.GetId(), inv.sessionId, sessionInfo)) {
        return true;
    }

    if (!VerifySigSharesInv(sessionInfo.llmqType, inv)) {
        return false;
    }

    // TODO for PoSe, we should consider propagating shares even if we already have a recovered sig
    if (sigman.HasRecoveredSigForSession(sessionInfo.signHash.Get())) {
        return true;
    }

    LogPrint(BCLog::LLMQ_SIGS, "CSigSharesManager::%s -- signHash=%s, inv={%s}, node=%d\n", __func__,
            sessionInfo.signHash.ToString(), inv.ToString(), pfrom.GetId());

    LOCK(cs);
    auto& nodeState = nodeStates[pfrom.GetId()];
    auto* session = nodeState.GetSessionByRecvId(inv.sessionId);
    if (session == nullptr) {
        return true;
    }
    session->requested.Merge(inv);
    session->knows.Merge(inv);
    return true;
}

bool CSigSharesManager::ProcessMessageBatchedSigShares(const CNode& pfrom, const CBatchedSigShares& batchedSigShares)
{
    CSigSharesNodeState::SessionInfo sessionInfo;
    if (!GetSessionInfoByRecvId(pfrom.GetId(), batchedSigShares.sessionId, sessionInfo)) {
        return true;
    }

    if (bool ban{false}; !PreVerifyBatchedSigShares(m_mn_activeman, qman, sessionInfo, batchedSigShares, ban)) {
        return !ban;
    }

    std::vector<CSigShare> sigSharesToProcess;
    sigSharesToProcess.reserve(batchedSigShares.sigShares.size());

    {
        LOCK(cs);
        auto& nodeState = nodeStates[pfrom.GetId()];

        for (const auto& sigSharetmp : batchedSigShares.sigShares) {
            CSigShare sigShare = RebuildSigShare(sessionInfo, sigSharetmp);
            nodeState.requestedSigShares.Erase(sigShare.GetKey());

            // TODO track invalid sig shares received for PoSe?
            // It's important to only skip seen *valid* sig shares here. If a node sends us a
            // batch of mostly valid sig shares with a single invalid one and thus batched
            // verification fails, we'd skip the valid ones in the future if received from other nodes
            if (sigShares.Has(sigShare.GetKey())) {
                continue;
            }

            // TODO for PoSe, we should consider propagating shares even if we already have a recovered sig
            if (sigman.HasRecoveredSigForId(sigShare.getLlmqType(), sigShare.getId())) {
                continue;
            }

            sigSharesToProcess.emplace_back(sigShare);
        }
    }

    LogPrint(BCLog::LLMQ_SIGS, "CSigSharesManager::%s -- signHash=%s, shares=%d, new=%d, inv={%s}, node=%d\n", __func__,
             sessionInfo.signHash.ToString(), batchedSigShares.sigShares.size(), sigSharesToProcess.size(), batchedSigShares.ToInvString(), pfrom.GetId());

    if (sigSharesToProcess.empty()) {
        return true;
    }

    LOCK(cs);
    auto& nodeState = nodeStates[pfrom.GetId()];
    for (const auto& s : sigSharesToProcess) {
        nodeState.pendingIncomingSigShares.Add(s.GetKey(), s);
    }
    return true;
}

void CSigSharesManager::ProcessMessageSigShare(NodeId fromId, const CSigShare& sigShare)
{
    auto quorum = qman.GetQuorum(sigShare.getLlmqType(), sigShare.getQuorumHash());
    if (!quorum) {
        return;
    }
    if (!IsQuorumActive(sigShare.getLlmqType(), qman, quorum->qc->quorumHash)) {
        // quorum is too old
        return;
    }
    if (!quorum->IsMember(m_mn_activeman.GetProTxHash())) {
        // we're not a member so we can't verify it (we actually shouldn't have received it)
        return;
    }
    if (!quorum->HasVerificationVector()) {
        // TODO we should allow to ask other nodes for the quorum vvec if we missed it in the DKG
        LogPrint(BCLog::LLMQ_SIGS, "CSigSharesManager::%s -- we don't have the quorum vvec for %s, no verification possible. node=%d\n", __func__,
                 quorum->qc->quorumHash.ToString(), fromId);
        return;
    }

    if (sigShare.getQuorumMember() >= quorum->members.size()) {
        LogPrint(BCLog::LLMQ_SIGS, "CSigSharesManager::%s -- quorumMember out of bounds\n", __func__);
        BanNode(fromId);
        return;
    }
    if (!quorum->qc->validMembers[sigShare.getQuorumMember()]) {
        LogPrint(BCLog::LLMQ_SIGS, "CSigSharesManager::%s -- quorumMember not valid\n", __func__);
        BanNode(fromId);
        return;
    }

    const auto signHash = sigShare.GetSignHash();
    const bool alreadyRecovered = sigman.HasRecoveredSigForId(sigShare.getLlmqType(), sigShare.getId()) ||
                                  sigman.HasRecoveredSigForSession(signHash);

    {
        LOCK(cs);

        if (alreadyRecovered) {
            LogPrint(BCLog::LLMQ_SIGS, /* Continued */
                     "CSigSharesManager::%s -- dropping sigShare for recovered session. signHash=%s, id=%s, "
                     "msgHash=%s, member=%d, node=%d\n",
                     __func__, signHash.ToString(), sigShare.getId().ToString(), sigShare.getMsgHash().ToString(),
                     sigShare.getQuorumMember(), fromId);
            return;
        }

        if (sigShares.Has(sigShare.GetKey())) {
            return;
        }

        auto& nodeState = nodeStates[fromId];
        nodeState.pendingIncomingSigShares.Add(sigShare.GetKey(), sigShare);
    }

    LogPrint(BCLog::LLMQ_SIGS, "CSigSharesManager::%s -- signHash=%s, id=%s, msgHash=%s, member=%d, node=%d\n", __func__,
             signHash.ToString(), sigShare.getId().ToString(), sigShare.getMsgHash().ToString(), sigShare.getQuorumMember(), fromId);
}

bool CSigSharesManager::PreVerifyBatchedSigShares(const CActiveMasternodeManager& mn_activeman, const CQuorumManager& quorum_manager,
                                                  const CSigSharesNodeState::SessionInfo& session, const CBatchedSigShares& batchedSigShares, bool& retBan)
{
    retBan = false;

    if (!IsQuorumActive(session.llmqType, quorum_manager, session.quorum->qc->quorumHash)) {
        // quorum is too old
        return false;
    }
    if (!session.quorum->IsMember(mn_activeman.GetProTxHash())) {
        // we're not a member so we can't verify it (we actually shouldn't have received it)
        return false;
    }
    if (!session.quorum->HasVerificationVector()) {
        // TODO we should allow to ask other nodes for the quorum vvec if we missed it in the DKG
        LogPrint(BCLog::LLMQ_SIGS, "CSigSharesManager::%s -- we don't have the quorum vvec for %s, no verification possible.\n", __func__,
                  session.quorumHash.ToString());
        return false;
    }

    std::unordered_set<uint16_t> dupMembers;

    for (const auto& [quorumMember, _] : batchedSigShares.sigShares) {
        if (!dupMembers.emplace(quorumMember).second) {
            retBan = true;
            return false;
        }

        if (quorumMember >= session.quorum->members.size()) {
            LogPrint(BCLog::LLMQ_SIGS, "CSigSharesManager::%s -- quorumMember out of bounds\n", __func__);
            retBan = true;
            return false;
        }
        if (!session.quorum->qc->validMembers[quorumMember]) {
            LogPrint(BCLog::LLMQ_SIGS, "CSigSharesManager::%s -- quorumMember not valid\n", __func__);
            retBan = true;
            return false;
        }
    }
    return true;
}

bool CSigSharesManager::CollectPendingSigSharesToVerify(
    size_t maxUniqueSessions, std::unordered_map<NodeId, std::vector<CSigShare>>& retSigShares,
    std::unordered_map<std::pair<Consensus::LLMQType, uint256>, CQuorumCPtr, StaticSaltedHasher>& retQuorums)
{
    bool more_work{false};

    {
        LOCK(cs);
        if (nodeStates.empty()) {
            return false;
        }

        // This will iterate node states in random order and pick one sig share at a time. This avoids processing
        // of large batches at once from the same node while other nodes also provided shares. If we wouldn't do this,
        // other nodes would be able to poison us with a large batch with N-1 valid shares and the last one being
        // invalid, making batch verification fail and revert to per-share verification, which in turn would slow down
        // the whole verification process
        std::unordered_set<std::pair<NodeId, uint256>, StaticSaltedHasher> uniqueSignHashes;
        IterateNodesRandom(
            nodeStates,
            [&]() {
                return uniqueSignHashes.size() < maxUniqueSessions;
                // TODO: remove NO_THREAD_SAFETY_ANALYSIS
                // using here template IterateNodesRandom makes impossible to use lock annotation
            },
            [&](NodeId nodeId, CSigSharesNodeState& ns) NO_THREAD_SAFETY_ANALYSIS {
                if (ns.pendingIncomingSigShares.Empty()) {
                    return false;
                }
                const auto& sigShare = *ns.pendingIncomingSigShares.GetFirst();

                AssertLockHeld(cs);
                if (const bool alreadyHave = this->sigShares.Has(sigShare.GetKey()); !alreadyHave) {
                    uniqueSignHashes.emplace(nodeId, sigShare.GetSignHash());
                    retSigShares[nodeId].emplace_back(sigShare);
                }
                ns.pendingIncomingSigShares.Erase(sigShare.GetKey());
                return !ns.pendingIncomingSigShares.Empty();
            },
            rnd);

        if (retSigShares.empty()) {
            return false;
        }

        // Determine if there is still work left in any node state after pulling this batch
        more_work = std::any_of(nodeStates.begin(), nodeStates.end(),
                                [](const auto& entry) {
                                    const auto& ns = entry.second;
                                    return !ns.pendingIncomingSigShares.Empty();
                                });
    }

    // For the convenience of the caller, also build a map of quorumHash -> quorum

    for (const auto& [_, vecSigShares] : retSigShares) {
        for (const auto& sigShare : vecSigShares) {
            auto llmqType = sigShare.getLlmqType();

            auto k = std::make_pair(llmqType, sigShare.getQuorumHash());
            if (retQuorums.count(k) != 0) {
                continue;
            }

            auto quorum = qman.GetQuorum(llmqType, sigShare.getQuorumHash());
            // Despite constructing a convenience map, we assume that the quorum *must* be present.
            // The absence of it might indicate an inconsistent internal state, so we should report
            // nothing instead of reporting flawed data.
            if (!quorum) {
                LogPrintf("%s: ERROR! Unexpected missing quorum with llmqType=%d, quorumHash=%s\n", __func__,
                          ToUnderlying(llmqType), sigShare.getQuorumHash().ToString());
                return false;
            }
            retQuorums.try_emplace(k, quorum);
        }
    }

    return more_work;
}

bool CSigSharesManager::ProcessPendingSigShares()
{
    std::unordered_map<NodeId, std::vector<CSigShare>> sigSharesByNodes;
    std::unordered_map<std::pair<Consensus::LLMQType, uint256>, CQuorumCPtr, StaticSaltedHasher> quorums;

    const size_t nMaxBatchSize{32};
    bool more_work = CollectPendingSigSharesToVerify(nMaxBatchSize, sigSharesByNodes, quorums);
    if (sigSharesByNodes.empty()) {
        return false;
    }

    // It's ok to perform insecure batched verification here as we verify against the quorum public key shares,
    // which are not craftable by individual entities, making the rogue public key attack impossible
    CBLSBatchVerifier<NodeId, SigShareKey> batchVerifier(false, true);

    cxxtimer::Timer prepareTimer(true);
    size_t verifyCount = 0;
    for (const auto& [nodeId, v] : sigSharesByNodes) {
        for (const auto& sigShare : v) {
            if (sigman.HasRecoveredSigForId(sigShare.getLlmqType(), sigShare.getId())) {
                continue;
            }

            // Materialize the signature once. Get() internally validates, so if it returns an invalid signature,
            // we know it's malformed. This avoids calling Get() twice (once for IsValid(), once for PushMessage).
            CBLSSignature sig = sigShare.sigShare.Get();
            // we didn't check this earlier because we use a lazy BLS signature and tried to avoid doing the expensive
            // deserialization in the message thread
            if (!sig.IsValid()) {
                BanNode(nodeId);
                // don't process any additional shares from this node
                break;
            }

            auto quorum = quorums.at(std::make_pair(sigShare.getLlmqType(), sigShare.getQuorumHash()));
            auto pubKeyShare = quorum->GetPubKeyShare(sigShare.getQuorumMember());

            if (!pubKeyShare.IsValid()) {
                // this should really not happen (we already ensured we have the quorum vvec,
                // so we should also be able to create all pubkey shares)
                LogPrintf("CSigSharesManager::%s -- pubKeyShare is invalid, which should not be possible here\n", __func__);
                assert(false);
            }

            batchVerifier.PushMessage(nodeId, sigShare.GetKey(), sigShare.GetSignHash(), sig, pubKeyShare);
            verifyCount++;
        }
    }
    prepareTimer.stop();

    cxxtimer::Timer verifyTimer(true);
    batchVerifier.Verify();
    verifyTimer.stop();

    LogPrint(BCLog::LLMQ_SIGS, "CSigSharesManager::%s -- verified sig shares. count=%d, pt=%d, vt=%d, nodes=%d\n", __func__, verifyCount, prepareTimer.count(), verifyTimer.count(), sigSharesByNodes.size());

    for (const auto& [nodeId, v] : sigSharesByNodes) {
        if (batchVerifier.badSources.count(nodeId) != 0) {
            LogPrint(BCLog::LLMQ_SIGS, "CSigSharesManager::%s -- invalid sig shares from other node, banning peer=%d\n",
                     __func__, nodeId);
            // this will also cause re-requesting of the shares that were sent by this node
            BanNode(nodeId);
            continue;
        }

        ProcessPendingSigShares(v, quorums);
    }

    return more_work;
}

// It's ensured that no duplicates are passed to this method
void CSigSharesManager::ProcessPendingSigShares(
    const std::vector<CSigShare>& sigSharesToProcess,
    const std::unordered_map<std::pair<Consensus::LLMQType, uint256>, CQuorumCPtr, StaticSaltedHasher>& quorums)
{
    cxxtimer::Timer t(true);
    for (const auto& sigShare : sigSharesToProcess) {
        auto quorumKey = std::make_pair(sigShare.getLlmqType(), sigShare.getQuorumHash());
        ProcessSigShare(sigShare, quorums.at(quorumKey));
    }
    t.stop();

    LogPrint(BCLog::LLMQ_SIGS, "CSigSharesManager::%s -- processed sigShare batch. shares=%d, time=%ds\n", __func__,
             sigSharesToProcess.size(), t.count());
}

// sig shares are already verified when entering this method
void CSigSharesManager::ProcessSigShare(const CSigShare& sigShare, const CQuorumCPtr& quorum)
{
    auto llmqType = quorum->params.type;
    bool canTryRecovery = false;

    const bool isAllMembersConnectedEnabled = IsAllMembersConnectedEnabled(llmqType, m_sporkman);

    // prepare node set for direct-push in case this is our sig share
    std::vector<NodeId> quorumNodes;
    if (!isAllMembersConnectedEnabled &&
        sigShare.getQuorumMember() == quorum->GetMemberIndex(m_mn_activeman.GetProTxHash())) {
        quorumNodes = m_connman.GetMasternodeQuorumNodes(sigShare.getLlmqType(), sigShare.getQuorumHash());
    }

    if (sigman.HasRecoveredSigForId(llmqType, sigShare.getId())) {
        return;
    }

    {
        LOCK(cs);

        if (!sigShares.Add(sigShare.GetKey(), sigShare)) {
            return;
        }
        if (!isAllMembersConnectedEnabled) {
            sigSharesQueuedToAnnounce.Add(sigShare.GetKey(), true);
        }

        // Update the time we've seen the last sigShare
        timeSeenForSessions[sigShare.GetSignHash()] = GetTime<std::chrono::seconds>().count();

        // don't announce and wait for other nodes to request this share and directly send it to them
        // there is no way the other nodes know about this share as this is the one created on this node
        for (auto otherNodeId : quorumNodes) {
            auto& nodeState = nodeStates[otherNodeId];
            auto& session = nodeState.GetOrCreateSessionFromShare(sigShare);
            session.quorum = quorum;
            session.requested.Set(sigShare.getQuorumMember(), true);
            session.knows.Set(sigShare.getQuorumMember(), true);
        }

        size_t sigShareCount = sigShares.CountForSignHash(sigShare.GetSignHash());
        if (sigShareCount >= size_t(quorum->params.threshold)) {
            canTryRecovery = true;
        }
    }

    if (canTryRecovery) {
        auto rs = TryRecoverSig(*quorum, sigShare.getId(), sigShare.getMsgHash());
        if (rs != nullptr) {
            if (sigman.ProcessRecoveredSig(rs)) {
                // TODO: remove duplicated code with NetSigning
                auto listeners = sigman.GetListeners();
                for (auto& l : listeners) {
                    m_peerman.PostProcessMessage(l->HandleNewRecoveredSig(*rs));
                }

                bool proactive_relay = rs->getLlmqType() != Consensus::LLMQType::LLMQ_100_67 &&
                                       rs->getLlmqType() != Consensus::LLMQType::LLMQ_400_60 &&
                                       rs->getLlmqType() != Consensus::LLMQType::LLMQ_400_85;
                GetMainSignals().NotifyRecoveredSig(rs, rs->GetHash().ToString(), proactive_relay);
            }
        }
    }
}

std::shared_ptr<CRecoveredSig> CSigSharesManager::TryRecoverSig(const CQuorum& quorum, const uint256& id,
                                                                const uint256& msgHash)
{
    if (sigman.HasRecoveredSigForId(quorum.params.type, id)) {
        return nullptr;
    }

    std::vector<CBLSSignature> sigSharesForRecovery;
    std::vector<CBLSId> idsForRecovery;
    {
        LOCK(cs);

        auto signHash = SignHash(quorum.params.type, quorum.qc->quorumHash, id, msgHash).Get();
        const auto* sigSharesForSignHash = sigShares.GetAllForSignHash(signHash);
        if (sigSharesForSignHash == nullptr) {
            return nullptr;
        }

        std::shared_ptr<CRecoveredSig> singleMemberRecoveredSig;
        if (quorum.params.is_single_member()) {
            if (sigSharesForSignHash->empty()) {
                LogPrint(BCLog::LLMQ_SIGS, /* Continued */
                         "CSigSharesManager::%s -- impossible to recover single-node signature - no shares yet. id=%s, "
                         "msgHash=%s\n",
                         __func__, id.ToString(), msgHash.ToString());
                return nullptr;
            }
            const auto& sigShare = sigSharesForSignHash->begin()->second;
            CBLSSignature recoveredSig = sigShare.sigShare.Get();
            LogPrint(BCLog::LLMQ_SIGS, "CSigSharesManager::%s -- recover single-node signature. id=%s, msgHash=%s\n",
                     __func__, id.ToString(), msgHash.ToString());

            singleMemberRecoveredSig = std::make_shared<CRecoveredSig>(quorum.params.type, quorum.qc->quorumHash, id, msgHash,
                                                      recoveredSig);
        }

        sigSharesForRecovery.reserve((size_t) quorum.params.threshold);
        idsForRecovery.reserve((size_t) quorum.params.threshold);
        for (auto it = sigSharesForSignHash->begin(); it != sigSharesForSignHash->end() && sigSharesForRecovery.size() < size_t(quorum.params.threshold); ++it) {
            const auto& sigShare = it->second;
            sigSharesForRecovery.emplace_back(sigShare.sigShare.Get());
            idsForRecovery.emplace_back(quorum.members[sigShare.getQuorumMember()]->proTxHash);
        }

        // check if we can recover the final signature
        if (sigSharesForRecovery.size() < size_t(quorum.params.threshold)) {
            return nullptr;
        }
        if (quorum.params.is_single_member()) {
            return singleMemberRecoveredSig; // end of single-quorum processing
        }
    }

    // now recover it
    cxxtimer::Timer t(true);
    CBLSSignature recoveredSig;
    if (!recoveredSig.Recover(sigSharesForRecovery, idsForRecovery)) {
        LogPrint(BCLog::LLMQ_SIGS, "CSigSharesManager::%s -- failed to recover signature. id=%s, msgHash=%s, time=%d\n", __func__,
                  id.ToString(), msgHash.ToString(), t.count());
        return nullptr;
    }

    LogPrint(BCLog::LLMQ_SIGS, "CSigSharesManager::%s -- recovered signature. id=%s, msgHash=%s, time=%d\n", __func__,
              id.ToString(), msgHash.ToString(), t.count());

    auto rs = std::make_shared<CRecoveredSig>(quorum.params.type, quorum.qc->quorumHash, id, msgHash, recoveredSig);

    // There should actually be no need to verify the self-recovered signatures as it should always succeed. Let's
    // however still verify it from time to time, so that we have a chance to catch bugs. We do only this sporadic
    // verification because this is unbatched and thus slow verification that happens here.
    if (((recoveredSigsCounter++) % 100) == 0) {
        auto signHash = rs->buildSignHash();
        bool valid = recoveredSig.VerifyInsecure(quorum.qc->quorumPublicKey, signHash.Get());
        if (!valid) {
            // this should really not happen as we have verified all signature shares before
            LogPrintf("CSigSharesManager::%s -- own recovered signature is invalid. id=%s, msgHash=%s\n", __func__,
                      id.ToString(), msgHash.ToString());
            return nullptr;
        }
    }
    return rs;
}

CDeterministicMNCPtr CSigSharesManager::SelectMemberForRecovery(const CQuorum& quorum, const uint256 &id, int attempt)
{
    assert(attempt < quorum.params.recoveryMembers);

    std::vector<std::pair<uint256, CDeterministicMNCPtr>> v;
    v.reserve(quorum.members.size());
    for (const auto& dmn : quorum.members) {
        auto h = ::SerializeHash(std::make_pair(dmn->proTxHash, id));
        v.emplace_back(h, dmn);
    }
    std::sort(v.begin(), v.end());

    return v[attempt % v.size()].second;
}

bool CSigSharesManager::AsyncSignIfMember(Consensus::LLMQType llmqType, CSigningManager& sigman, const uint256& id,
                                          const uint256& msgHash, const uint256& quorumHash, bool allowReSign,
                                          bool allowDiffMsgHashSigning)
{
    AssertLockNotHeld(cs_pendingSigns);

    if (m_mn_activeman.GetProTxHash().IsNull()) return false;

    auto quorum = [&]() {
        if (quorumHash.IsNull()) {
            // This might end up giving different results on different members
            // This might happen when we are on the brink of confirming a new quorum
            // This gives a slight risk of not getting enough shares to recover a signature
            // But at least it shouldn't be possible to get conflicting recovered signatures
            // TODO fix this by re-signing when the next block arrives, but only when that block results in a change of
            // the quorum list and no recovered signature has been created in the mean time
            const auto& llmq_params_opt = Params().GetLLMQ(llmqType);
            assert(llmq_params_opt.has_value());
            return SelectQuorumForSigning(llmq_params_opt.value(), m_chainstate.m_chain, qman, id);
        } else {
            return qman.GetQuorum(llmqType, quorumHash);
        }
    }();

    if (!quorum) {
        LogPrint(BCLog::LLMQ, "CSigningManager::%s -- failed to select quorum. id=%s, msgHash=%s\n", __func__,
                 id.ToString(), msgHash.ToString());
        return false;
    }

    if (!quorum->IsValidMember(m_mn_activeman.GetProTxHash())) {
        return false;
    }

    {
        auto& db = sigman.GetDb();
        bool hasVoted = db.HasVotedOnId(llmqType, id);
        if (hasVoted) {
            uint256 prevMsgHash;
            db.GetVoteForId(llmqType, id, prevMsgHash);
            if (msgHash != prevMsgHash) {
                if (allowDiffMsgHashSigning) {
                    LogPrintf("%s -- already voted for id=%s and msgHash=%s. Signing for different " /* Continued */
                              "msgHash=%s\n",
                              __func__, id.ToString(), prevMsgHash.ToString(), msgHash.ToString());
                    hasVoted = false;
                } else {
                    LogPrintf("%s -- already voted for id=%s and msgHash=%s. Not voting on " /* Continued */
                              "conflicting msgHash=%s\n",
                              __func__, id.ToString(), prevMsgHash.ToString(), msgHash.ToString());
                    return false;
                }
            } else if (allowReSign) {
                LogPrint(BCLog::LLMQ, "%s -- already voted for id=%s and msgHash=%s. Resigning!\n", __func__,
                         id.ToString(), prevMsgHash.ToString());
            } else {
                LogPrint(BCLog::LLMQ, "%s -- already voted for id=%s and msgHash=%s. Not voting again.\n", __func__,
                         id.ToString(), prevMsgHash.ToString());
                return false;
            }
        }

        if (db.HasRecoveredSigForId(llmqType, id)) {
            // no need to sign it if we already have a recovered sig
            return true;
        }
        if (!hasVoted) {
            db.WriteVoteForId(llmqType, id, msgHash);
        }
    }

    if (allowReSign) {
        // make us re-announce all known shares (other nodes might have run into a timeout)
        ForceReAnnouncement(*quorum, llmqType, id, msgHash);
    }
    AsyncSign(std::move(quorum), id, msgHash);

    return true;
}

void CSigSharesManager::NotifyRecoveredSig(const std::shared_ptr<const CRecoveredSig>& sig, bool proactive_relay) const
{
    m_peerman.RelayRecoveredSig(*Assert(sig), proactive_relay);
}

void CSigSharesManager::CollectSigSharesToRequest(std::unordered_map<NodeId, Uint256HashMap<CSigSharesInv>>& sigSharesToRequest)
{
    AssertLockHeld(cs);

    int64_t now = GetTime<std::chrono::seconds>().count();
    const size_t maxRequestsForNode = 32;

    // avoid requesting from same nodes all the time
    std::vector<NodeId> shuffledNodeIds;
    shuffledNodeIds.reserve(nodeStates.size());
    for (const auto& [nodeId, nodeState] : nodeStates) {
        if (nodeState.sessions.empty()) {
            continue;
        }
        shuffledNodeIds.emplace_back(nodeId);
    }
    Shuffle(shuffledNodeIds.begin(), shuffledNodeIds.end(), rnd);

    for (const auto& nodeId : shuffledNodeIds) {
        auto& nodeState = nodeStates[nodeId];

        if (nodeState.banned) {
            continue;
        }

        nodeState.requestedSigShares.EraseIf([&now, &nodeId](const SigShareKey& k, int64_t t) {
            if (now - t >= SIG_SHARE_REQUEST_TIMEOUT) {
                // timeout while waiting for this one, so retry it with another node
                LogPrint(BCLog::LLMQ_SIGS, "CSigSharesManager::CollectSigSharesToRequest -- timeout while waiting for %s-%d, node=%d\n",
                         k.first.ToString(), k.second, nodeId);
                return true;
            }
            return false;
        });

        decltype(sigSharesToRequest.begin()->second)* invMap = nullptr;

        for (auto& [signHash, session] : nodeState.sessions) {
            if (IsAllMembersConnectedEnabled(session.llmqType, m_sporkman)) {
                continue;
            }

            if (sigman.HasRecoveredSigForSession(signHash)) {
                continue;
            }

            for (const auto i : irange::range(session.announced.inv.size())) {
                if (!session.announced.inv[i]) {
                    continue;
                }
                auto k = std::make_pair(signHash, (uint16_t) i);
                if (sigShares.Has(k)) {
                    // we already have it
                    session.announced.inv[i] = false;
                    continue;
                }
                if (nodeState.requestedSigShares.Size() >= maxRequestsForNode) {
                    // too many pending requests for this node
                    break;
                }
                if (auto *const p = sigSharesRequested.Get(k)) {
                    if (now - p->second >= SIG_SHARE_REQUEST_TIMEOUT && nodeId != p->first) {
                        // other node timed out, re-request from this node
                        LogPrint(BCLog::LLMQ_SIGS, "CSigSharesManager::%s -- other node timeout while waiting for %s-%d, re-request from=%d, node=%d\n", __func__,
                                 k.first.ToString(), k.second, nodeId, p->first);
                    } else {
                        continue;
                    }
                }
                // if we got this far we should do a request

                // track when we initiated the request so that we can detect timeouts
                nodeState.requestedSigShares.Add(k, now);

                // don't request it from other nodes until a timeout happens
                auto& r = sigSharesRequested.GetOrAdd(k);
                r.first = nodeId;
                r.second = now;

                if (invMap == nullptr) {
                    invMap = &sigSharesToRequest[nodeId];
                }
                auto& inv = (*invMap)[signHash];
                if (inv.inv.empty()) {
                    const auto& llmq_params_opt = Params().GetLLMQ(session.llmqType);
                    assert(llmq_params_opt.has_value());
                    inv.Init(llmq_params_opt->size);
                }
                inv.inv[k.second] = true;

                // don't request it again from this node
                session.announced.inv[i] = false;
            }
        }
    }
}

void CSigSharesManager::CollectSigSharesToSend(std::unordered_map<NodeId, Uint256HashMap<CBatchedSigShares>>& sigSharesToSend)
{
    AssertLockHeld(cs);

    for (auto& [nodeId, nodeState] : nodeStates) {
        if (nodeState.banned) {
            continue;
        }

        decltype(sigSharesToSend.begin()->second)* sigSharesToSend2 = nullptr;

        for (auto& [signHash, session] : nodeState.sessions) {
            if (IsAllMembersConnectedEnabled(session.llmqType, m_sporkman)) {
                continue;
            }

            if (sigman.HasRecoveredSigForSession(signHash)) {
                continue;
            }

            CBatchedSigShares batchedSigShares;

            for (const auto i : irange::range(session.requested.inv.size())) {
                if (!session.requested.inv[i]) {
                    continue;
                }
                session.requested.inv[i] = false;

                auto k = std::make_pair(signHash, (uint16_t)i);
                const CSigShare* sigShare = sigShares.Get(k);
                if (sigShare == nullptr) {
                    // he requested something we don't have
                    session.requested.inv[i] = false;
                    continue;
                }

                batchedSigShares.sigShares.emplace_back((uint16_t)i, sigShare->sigShare);
            }

            if (!batchedSigShares.sigShares.empty()) {
                if (sigSharesToSend2 == nullptr) {
                    // only create the map if we actually add a batched sig
                    sigSharesToSend2 = &sigSharesToSend[nodeId];
                }
                sigSharesToSend2->try_emplace(signHash, std::move(batchedSigShares));
            }
        }
    }
}

void CSigSharesManager::CollectSigSharesToSendConcentrated(std::unordered_map<NodeId, std::vector<CSigShare>>& sigSharesToSend, const std::vector<CNode*>& vNodes)
{
    AssertLockHeld(cs);

    Uint256HashMap<CNode*> proTxToNode;
    for (const auto& pnode : vNodes) {
        auto verifiedProRegTxHash = pnode->GetVerifiedProRegTxHash();
        if (verifiedProRegTxHash.IsNull()) {
            continue;
        }
        proTxToNode.try_emplace(verifiedProRegTxHash, pnode);
    }

    auto curTime = GetTime<std::chrono::milliseconds>().count();

    for (auto& [_, signedSession] : signedSessions) {
        if (!IsAllMembersConnectedEnabled(signedSession.quorum->params.type, m_sporkman)) {
            continue;
        }

        if (signedSession.attempt >= signedSession.quorum->params.recoveryMembers) {
            continue;
        }

        if (curTime >= signedSession.nextAttemptTime) {
            int64_t waitTime = exp2(signedSession.attempt) * EXP_SEND_FOR_RECOVERY_TIMEOUT;
            waitTime = std::min(MAX_SEND_FOR_RECOVERY_TIMEOUT, waitTime);
            signedSession.nextAttemptTime = curTime + waitTime;
            auto dmn = SelectMemberForRecovery(*signedSession.quorum, signedSession.sigShare.getId(), signedSession.attempt);
            signedSession.attempt++;

            LogPrint(BCLog::LLMQ_SIGS, "CSigSharesManager::%s -- signHash=%s, sending to %s, attempt=%d\n", __func__,
                     signedSession.sigShare.GetSignHash().ToString(), dmn->proTxHash.ToString(), signedSession.attempt);

            auto it = proTxToNode.find(dmn->proTxHash);
            if (it == proTxToNode.end()) {
                continue;
            }

            auto& m = sigSharesToSend[it->second->GetId()];
            m.emplace_back(signedSession.sigShare);
        }
    }
}

void CSigSharesManager::CollectSigSharesToAnnounce(std::unordered_map<NodeId, Uint256HashMap<CSigSharesInv>>& sigSharesToAnnounce)
{
    AssertLockHeld(cs);

    std::unordered_map<std::pair<Consensus::LLMQType, uint256>, std::unordered_set<NodeId>, StaticSaltedHasher> quorumNodesMap;

    // TODO: remove NO_THREAD_SAFETY_ANALYSIS
    // using here template ForEach makes impossible to use lock annotation
    sigSharesQueuedToAnnounce.ForEach([this, &quorumNodesMap, &sigSharesToAnnounce](const SigShareKey& sigShareKey,
                                                                                    bool) NO_THREAD_SAFETY_ANALYSIS {
        AssertLockHeld(cs);
        const auto& signHash = sigShareKey.first;
        auto quorumMember = sigShareKey.second;
        const CSigShare* sigShare = sigShares.Get(sigShareKey);
        if (sigShare == nullptr) {
            return;
        }

        // announce to the nodes which we know through the intra-quorum-communication system
        auto quorumKey = std::make_pair(sigShare->getLlmqType(), sigShare->getQuorumHash());
        auto it = quorumNodesMap.find(quorumKey);
        if (it == quorumNodesMap.end()) {
            auto nodeIds = m_connman.GetMasternodeQuorumNodes(quorumKey.first, quorumKey.second);
            it = quorumNodesMap.emplace(std::piecewise_construct, std::forward_as_tuple(quorumKey), std::forward_as_tuple(nodeIds.begin(), nodeIds.end())).first;
        }

        const auto& quorumNodes = it->second;

        for (const auto& nodeId : quorumNodes) {
            auto& nodeState = nodeStates[nodeId];

            if (nodeState.banned) {
                continue;
            }

            auto& session = nodeState.GetOrCreateSessionFromShare(*sigShare);

            if (session.knows.inv[quorumMember]) {
                // he already knows that one
                continue;
            }

            auto& inv = sigSharesToAnnounce[nodeId][signHash];
            if (inv.inv.empty()) {
                const auto& llmq_params_opt = Params().GetLLMQ(sigShare->getLlmqType());
                assert(llmq_params_opt.has_value());
                inv.Init(llmq_params_opt->size);
            }
            inv.inv[quorumMember] = true;
            session.knows.inv[quorumMember] = true;
        }
    });

    // don't announce these anymore
    sigSharesQueuedToAnnounce.Clear();
}

bool CSigSharesManager::SendMessages()
{
    std::unordered_map<NodeId, Uint256HashMap<CSigSharesInv>> sigSharesToRequest;
    std::unordered_map<NodeId, Uint256HashMap<CBatchedSigShares>> sigShareBatchesToSend;
    std::unordered_map<NodeId, std::vector<CSigShare>> sigSharesToSend;
    std::unordered_map<NodeId, Uint256HashMap<CSigSharesInv>> sigSharesToAnnounce;
    std::unordered_map<NodeId, std::vector<CSigSesAnn>> sigSessionAnnouncements;

    auto addSigSesAnnIfNeeded = [&](NodeId nodeId, const uint256& signHash) EXCLUSIVE_LOCKS_REQUIRED(cs) {
        AssertLockHeld(cs);
        auto& nodeState = nodeStates[nodeId];
        auto* session = nodeState.GetSessionBySignHash(signHash);
        assert(session);
        while (session->sendSessionId == UNINITIALIZED_SESSION_ID) {
            const uint32_t session_id{GetRand<uint32_t>()};
            if (ranges::all_of(nodeState.sessions,
                               [&session_id](const auto& s) { return s.second.sendSessionId != session_id; })) {
                // No session is using this id yet
                session->sendSessionId = session_id;
                sigSessionAnnouncements[nodeId].emplace_back(
                    CSigSesAnn(/*sessionId=*/session->sendSessionId, /*llmqType=*/session->llmqType,
                               /*quorumHash=*/session->quorumHash, /*id=*/session->id, /*msgHash=*/session->msgHash));
            }
            // It's very unlikely that there is a session with the same id,
            // but if there is one we just start over and pick another id
        }
        return session->sendSessionId;
    };

    const CConnman::NodesSnapshot snap{m_connman, /* cond = */ CConnman::FullyConnectedOnly};
    {
        LOCK(cs);
        CollectSigSharesToRequest(sigSharesToRequest);
        CollectSigSharesToSend(sigShareBatchesToSend);
        CollectSigSharesToAnnounce(sigSharesToAnnounce);
        CollectSigSharesToSendConcentrated(sigSharesToSend, snap.Nodes());

        for (auto& [nodeId, sigShareMap] : sigSharesToRequest) {
            for (auto& [hash, sigShareInv] : sigShareMap) {
                sigShareInv.sessionId = addSigSesAnnIfNeeded(nodeId, hash);
            }
        }
        for (auto& [nodeId, sigShareBatchesMap] : sigShareBatchesToSend) {
            for (auto& [hash, sigShareBatch] : sigShareBatchesMap) {
                sigShareBatch.sessionId = addSigSesAnnIfNeeded(nodeId, hash);
            }
        }
        for (auto& [nodeId, sigShareMap] : sigSharesToAnnounce) {
            for (auto& [hash, sigShareInv] : sigShareMap) {
                sigShareInv.sessionId = addSigSesAnnIfNeeded(nodeId, hash);
            }
        }
    }

    bool didSend = false;

    for (auto& pnode : snap.Nodes()) {
        CNetMsgMaker msgMaker(pnode->GetCommonVersion());

        if (const auto it1 = sigSessionAnnouncements.find(pnode->GetId()); it1 != sigSessionAnnouncements.end()) {
            std::vector<CSigSesAnn> msgs;
            msgs.reserve(it1->second.size());
            for (auto& sigSesAnn : it1->second) {
                LogPrint(BCLog::LLMQ_SIGS, "CSigSharesManager::SendMessages -- QSIGSESANN signHash=%s, sessionId=%d, node=%d\n",
                         sigSesAnn.buildSignHash().ToString(), sigSesAnn.getSessionId(), pnode->GetId());
                msgs.emplace_back(sigSesAnn);
                if (msgs.size() == MAX_MSGS_CNT_QSIGSESANN) {
                    m_connman.PushMessage(pnode, msgMaker.Make(NetMsgType::QSIGSESANN, msgs));
                    msgs.clear();
                    didSend = true;
                }
            }
            if (!msgs.empty()) {
                m_connman.PushMessage(pnode, msgMaker.Make(NetMsgType::QSIGSESANN, msgs));
                didSend = true;
            }
        }

        if (const auto it = sigSharesToRequest.find(pnode->GetId()); it != sigSharesToRequest.end()) {
            std::vector<CSigSharesInv> msgs;
            for (const auto& [signHash, inv] : it->second) {
                assert(inv.CountSet() != 0);
                LogPrint(BCLog::LLMQ_SIGS, "CSigSharesManager::SendMessages -- QGETSIGSHARES signHash=%s, inv={%s}, node=%d\n",
                         signHash.ToString(), inv.ToString(), pnode->GetId());
                msgs.emplace_back(inv);
                if (msgs.size() == MAX_MSGS_CNT_QGETSIGSHARES) {
                    m_connman.PushMessage(pnode, msgMaker.Make(NetMsgType::QGETSIGSHARES, msgs));
                    msgs.clear();
                    didSend = true;
                }
            }
            if (!msgs.empty()) {
                m_connman.PushMessage(pnode, msgMaker.Make(NetMsgType::QGETSIGSHARES, msgs));
                didSend = true;
            }
        }

        if (const auto jt = sigShareBatchesToSend.find(pnode->GetId()); jt != sigShareBatchesToSend.end()) {
            size_t totalSigsCount = 0;
            std::vector<CBatchedSigShares> msgs;
            for (const auto& [signHash, inv] : jt->second) {
                assert(!inv.sigShares.empty());
                LogPrint(BCLog::LLMQ_SIGS, "CSigSharesManager::SendMessages -- QBSIGSHARES signHash=%s, inv={%s}, node=%d\n",
                         signHash.ToString(), inv.ToInvString(), pnode->GetId());
                if (totalSigsCount + inv.sigShares.size() > MAX_MSGS_TOTAL_BATCHED_SIGS) {
                    m_connman.PushMessage(pnode, msgMaker.Make(NetMsgType::QBSIGSHARES, msgs));
                    msgs.clear();
                    totalSigsCount = 0;
                    didSend = true;
                }
                totalSigsCount += inv.sigShares.size();
                msgs.emplace_back(inv);
            }
            if (!msgs.empty()) {
                m_connman.PushMessage(pnode, msgMaker.Make(NetMsgType::QBSIGSHARES, std::move(msgs)));
                didSend = true;
            }
        }

        if (const auto kt = sigSharesToAnnounce.find(pnode->GetId()); kt != sigSharesToAnnounce.end()) {
            std::vector<CSigSharesInv> msgs;
            for (const auto& [signHash, inv] : kt->second) {
                assert(inv.CountSet() != 0);
                LogPrint(BCLog::LLMQ_SIGS, "CSigSharesManager::SendMessages -- QSIGSHARESINV signHash=%s, inv={%s}, node=%d\n",
                         signHash.ToString(), inv.ToString(), pnode->GetId());
                msgs.emplace_back(inv);
                if (msgs.size() == MAX_MSGS_CNT_QSIGSHARESINV) {
                    m_connman.PushMessage(pnode, msgMaker.Make(NetMsgType::QSIGSHARESINV, msgs));
                    msgs.clear();
                    didSend = true;
                }
            }
            if (!msgs.empty()) {
                m_connman.PushMessage(pnode, msgMaker.Make(NetMsgType::QSIGSHARESINV, msgs));
                didSend = true;
            }
        }

        auto lt = sigSharesToSend.find(pnode->GetId());
        if (lt != sigSharesToSend.end()) {
            std::vector<CSigShare> msgs;
            for (auto& sigShare : lt->second) {
                LogPrint(BCLog::LLMQ_SIGS, "CSigSharesManager::SendMessages -- QSIGSHARE signHash=%s, node=%d\n",
                         sigShare.GetSignHash().ToString(), pnode->GetId());
                msgs.emplace_back(std::move(sigShare));
                if (msgs.size() == MAX_MSGS_SIG_SHARES) {
                    m_connman.PushMessage(pnode, msgMaker.Make(NetMsgType::QSIGSHARE, msgs));
                    msgs.clear();
                    didSend = true;
                }
            }
            if (!msgs.empty()) {
                m_connman.PushMessage(pnode, msgMaker.Make(NetMsgType::QSIGSHARE, msgs));
                didSend = true;
            }
        }
    }

    return didSend;
}

bool CSigSharesManager::GetSessionInfoByRecvId(NodeId nodeId, uint32_t sessionId, CSigSharesNodeState::SessionInfo& retInfo)
{
    LOCK(cs);
    return nodeStates[nodeId].GetSessionInfoByRecvId(sessionId, retInfo);
}

CSigShare CSigSharesManager::RebuildSigShare(const CSigSharesNodeState::SessionInfo& session, const std::pair<uint16_t, CBLSLazySignature>& in)
{
    const auto& [member, sig] = in;
    CSigShare sigShare(session.llmqType, session.quorumHash, session.id, session.msgHash, member, sig);
    sigShare.UpdateKey();
    return sigShare;
}

void CSigSharesManager::Cleanup()
{
    constexpr auto CLEANUP_INTERVAL{5s};
    if (!cleanupThrottler.TryCleanup(CLEANUP_INTERVAL)) {
        return;
    }

    // This map is first filled with all quorums found in all sig shares. Then we remove all inactive quorums and
    // loop through all sig shares again to find the ones belonging to the inactive quorums. We then delete the
    // sessions belonging to the sig shares. At the same time, we use this map as a cache when we later need to resolve
    // quorumHash -> quorumPtr (as GetQuorum() requires cs_main, leading to deadlocks with cs held)
    std::unordered_map<std::pair<Consensus::LLMQType, uint256>, CQuorumCPtr, StaticSaltedHasher> quorums;

    {
        LOCK(cs);
        sigShares.ForEach([&quorums](const SigShareKey&, const CSigShare& sigShare) {
            quorums.try_emplace(std::make_pair(sigShare.getLlmqType(), sigShare.getQuorumHash()), nullptr);
        });
    }

    // Find quorums which became inactive
    for (auto it = quorums.begin(); it != quorums.end(); ) {
        if (IsQuorumActive(it->first.first, qman, it->first.second)) {
            auto quorum = qman.GetQuorum(it->first.first, it->first.second);
            if (quorum) {
                it->second = quorum;
                ++it;
                continue;
            }
        }
        it = quorums.erase(it);
    }

    {
        // Now delete sessions which are for inactive quorums
        LOCK(cs);
        Uint256HashSet inactiveQuorumSessions;
        sigShares.ForEach([&quorums, &inactiveQuorumSessions](const SigShareKey&, const CSigShare& sigShare) {
            if (quorums.count(std::make_pair(sigShare.getLlmqType(), sigShare.getQuorumHash())) == 0) {
                inactiveQuorumSessions.emplace(sigShare.GetSignHash());
            }
        });
        for (const auto& signHash : inactiveQuorumSessions) {
            RemoveSigSharesForSession(signHash);
        }
    }

    {
        LOCK(cs);

        // Remove sessions which were successfully recovered
        Uint256HashSet doneSessions;
        sigShares.ForEach([&doneSessions, this](const SigShareKey&, const CSigShare& sigShare) {
            if (doneSessions.count(sigShare.GetSignHash()) != 0) {
                return;
            }
            if (sigman.HasRecoveredSigForSession(sigShare.GetSignHash())) {
                doneSessions.emplace(sigShare.GetSignHash());
            }
        });
        for (const auto& signHash : doneSessions) {
            RemoveSigSharesForSession(signHash);
        }

        // Remove sessions which timed out
        Uint256HashSet timeoutSessions;
        int64_t now = GetTime<std::chrono::seconds>().count();
        for (const auto& [signHash, lastSeenTime] : timeSeenForSessions) {
            if (now - lastSeenTime >= SESSION_NEW_SHARES_TIMEOUT) {
                timeoutSessions.emplace(signHash);
            }
        }
        for (const auto& signHash : timeoutSessions) {

            if (const size_t count = sigShares.CountForSignHash(signHash); count > 0) {
                const auto* m = sigShares.GetAllForSignHash(signHash);
                assert(m);

                const auto& oneSigShare = m->begin()->second;

                std::string strMissingMembers;
                if (LogAcceptDebug(BCLog::LLMQ_SIGS)) {
                    if (const auto quorumIt = quorums.find(std::make_pair(oneSigShare.getLlmqType(), oneSigShare.getQuorumHash())); quorumIt != quorums.end()) {
                        const auto& quorum = quorumIt->second;
                        for (const auto i : irange::range(quorum->members.size())) {
                            if (m->count((uint16_t)i) == 0) {
                                const auto& dmn = quorum->members[i];
                                strMissingMembers += strprintf("\n  %s", dmn->proTxHash.ToString());
                            }
                        }
                    }
                }

                LogPrintLevel(BCLog::LLMQ_SIGS, BCLog::Level::Info, /* Continued */
                              "CSigSharesManager::%s -- signing session timed out. signHash=%s, id=%s, msgHash=%s, "
                              "sigShareCount=%d, missingMembers=%s\n",
                              __func__, signHash.ToString(), oneSigShare.getId().ToString(),
                              oneSigShare.getMsgHash().ToString(), count, strMissingMembers);
            } else {
                LogPrintLevel(BCLog::LLMQ_SIGS, BCLog::Level::Info, /* Continued */
                              "CSigSharesManager::%s -- signing session timed out. signHash=%s, sigShareCount=%d\n",
                              __func__, signHash.ToString(), count);
            }
            RemoveSigSharesForSession(signHash);
        }
    }

    // Find node states for peers that disappeared from CConnman
    std::unordered_set<NodeId> nodeStatesToDelete;
    {
        LOCK(cs);
        for (const auto& [nodeId, _] : nodeStates) {
            nodeStatesToDelete.emplace(nodeId);
        }
    }
    m_connman.ForEachNode([&nodeStatesToDelete](const CNode* pnode) { nodeStatesToDelete.erase(pnode->GetId()); });

    // Now delete these node states
    LOCK(cs);
    for (const auto& nodeId : nodeStatesToDelete) {
        auto it = nodeStates.find(nodeId);
        if (it == nodeStates.end()) {
            continue;
        }
        // remove global requested state to force a re-request from another node
        // TODO: remove NO_THREAD_SAFETY_ANALYSIS
        // using here template ForEach makes impossible to use lock annotation
        it->second.requestedSigShares.ForEach([this](const SigShareKey& k, bool) NO_THREAD_SAFETY_ANALYSIS {
            AssertLockHeld(cs);
            sigSharesRequested.Erase(k);
        });
        nodeStates.erase(nodeId);
    }
}

void CSigSharesManager::RemoveSigSharesForSession(const uint256& signHash)
{
    AssertLockHeld(cs);

    for (auto& [_, nodeState] : nodeStates) {
        nodeState.RemoveSession(signHash);
    }

    sigSharesRequested.EraseAllForSignHash(signHash);
    sigSharesQueuedToAnnounce.EraseAllForSignHash(signHash);
    sigShares.EraseAllForSignHash(signHash);
    signedSessions.erase(signHash);
    timeSeenForSessions.erase(signHash);
}

void CSigSharesManager::RemoveBannedNodeStates()
{
    // Called regularly to cleanup local node states for banned nodes

    LOCK(cs);
    for (auto it = nodeStates.begin(); it != nodeStates.end();) {
        if (m_peerman.IsBanned(it->first)) {
            // re-request sigshares from other nodes
            // TODO: remove NO_THREAD_SAFETY_ANALYSIS
            // using here template ForEach makes impossible to use lock annotation
            it->second.requestedSigShares.ForEach([this](const SigShareKey& k, int64_t) NO_THREAD_SAFETY_ANALYSIS {
                AssertLockHeld(cs);
                sigSharesRequested.Erase(k);
            });
            it = nodeStates.erase(it);
        } else {
            ++it;
        }
    }
}

void CSigSharesManager::BanNode(NodeId nodeId)
{
    if (nodeId == -1) {
        return;
    }

    m_peerman.Misbehaving(nodeId, 100);

    LOCK(cs);
    auto it = nodeStates.find(nodeId);
    if (it == nodeStates.end()) {
        return;
    }

    auto& nodeState = it->second;
    // Whatever we requested from him, let's request it from someone else now
    // TODO: remove NO_THREAD_SAFETY_ANALYSIS
    // using here template ForEach makes impossible to use lock annotation
    nodeState.requestedSigShares.ForEach([this](const SigShareKey& k, int64_t) NO_THREAD_SAFETY_ANALYSIS {
        AssertLockHeld(cs);
        sigSharesRequested.Erase(k);
    });
    nodeState.requestedSigShares.Clear();
    nodeState.banned = true;
}

void CSigSharesManager::HousekeepingThreadMain()
{
    while (!workInterrupt) {
        RemoveBannedNodeStates();
        SendMessages();
        Cleanup();

        workInterrupt.sleep_for(std::chrono::milliseconds(100));
    }
}

void CSigSharesManager::WorkDispatcherThreadMain()
{
    while (!workInterrupt) {
        // Dispatch all pending signs (individual tasks)
        DispatchPendingSigns();

        // If there's processing work, spawn a helper worker
        DispatchPendingProcessing();

        // Always sleep briefly between checks
        workInterrupt.sleep_for(std::chrono::milliseconds(10));
    }
}

void CSigSharesManager::DispatchPendingSigns()
{
    // Swap out entire vector to avoid lock thrashing
    std::vector<PendingSignatureData> signs;
    {
        LOCK(cs_pendingSigns);
        signs.swap(pendingSigns);
    }

    // Dispatch all signs to worker pool
    for (auto& work : signs) {
        if (workInterrupt) break;

        workerPool.push([this, work = std::move(work)](int) mutable {
            SignAndProcessSingleShare(std::move(work));
        });
    }
}

void CSigSharesManager::DispatchPendingProcessing()
{
    // Check if there's work, spawn a helper if so
    bool hasWork = false;
    {
        LOCK(cs);
        hasWork = std::any_of(nodeStates.begin(), nodeStates.end(),
            [](const auto& entry) {
                return !entry.second.pendingIncomingSigShares.Empty();
            });
    }

    if (hasWork) {
        // Work exists - spawn a worker to help!
        workerPool.push([this](int) {
            ProcessPendingSigSharesLoop();
        });
    }
}

void CSigSharesManager::ProcessPendingSigSharesLoop()
{
    while (!workInterrupt) {
        bool moreWork = ProcessPendingSigShares();

        if (!moreWork) {
            return;  // No work found, exit immediately
        }
    }
}

void CSigSharesManager::SignAndProcessSingleShare(PendingSignatureData work)
{
    auto opt_sigShare = CreateSigShare(*work.quorum, work.id, work.msgHash);

    if (opt_sigShare.has_value() && opt_sigShare->sigShare.Get().IsValid()) {
        auto& sigShare = *opt_sigShare;
        ProcessSigShare(sigShare, work.quorum);

        if (IsAllMembersConnectedEnabled(work.quorum->params.type, m_sporkman)) {
            LOCK(cs);
            auto& session = signedSessions[sigShare.GetSignHash()];
            session.sigShare = std::move(sigShare);
            session.quorum = work.quorum;
            session.nextAttemptTime = 0;
            session.attempt = 0;
        }
    }
}

void CSigSharesManager::AsyncSign(CQuorumCPtr quorum, const uint256& id, const uint256& msgHash)
{
    LOCK(cs_pendingSigns);
    pendingSigns.emplace_back(std::move(quorum), id, msgHash);
}

std::optional<CSigShare> CSigSharesManager::CreateSigShareForSingleMember(const CQuorum& quorum, const uint256& id, const uint256& msgHash) const
{
    cxxtimer::Timer t(true);
    auto activeMasterNodeProTxHash = m_mn_activeman.GetProTxHash();

    int memberIdx = quorum.GetMemberIndex(activeMasterNodeProTxHash);
    if (memberIdx == -1) {
        // this should really not happen (IsValidMember gave true)
        return std::nullopt;
    }

    CSigShare sigShare(quorum.params.type, quorum.qc->quorumHash, id, msgHash, uint16_t(memberIdx), {});
    uint256 signHash = sigShare.buildSignHash().Get();

    // TODO: This one should be SIGN by QUORUM key, not by OPERATOR key
    // see TODO in CDKGSession::FinalizeSingleCommitment for details
    auto bls_scheme = bls::bls_legacy_scheme.load();
    sigShare.sigShare.Set(m_mn_activeman.Sign(signHash, bls_scheme), bls_scheme);

    if (!sigShare.sigShare.Get().IsValid()) {
        LogPrintf("CSigSharesManager::%s -- failed to sign sigShare. signHash=%s, id=%s, msgHash=%s, time=%s\n",
                  __func__, signHash.ToString(), sigShare.getId().ToString(), sigShare.getMsgHash().ToString(),
                  t.count());
        return std::nullopt;
    }

    sigShare.UpdateKey();

    LogPrint(BCLog::LLMQ_SIGS, /* Continued */
             "CSigSharesManager::%s -- created sigShare. signHash=%s, id=%s, msgHash=%s, llmqType=%d, quorum=%s, "
             "time=%s\n",
             __func__, signHash.ToString(), sigShare.getId().ToString(), sigShare.getMsgHash().ToString(),
             ToUnderlying(quorum.params.type), quorum.qc->quorumHash.ToString(), t.count());

    return sigShare;
}

std::optional<CSigShare> CSigSharesManager::CreateSigShare(const CQuorum& quorum, const uint256& id, const uint256& msgHash) const
{
    auto activeMasterNodeProTxHash = m_mn_activeman.GetProTxHash();

    if (!quorum.IsValidMember(activeMasterNodeProTxHash)) {
        return std::nullopt;
    }

    if (quorum.params.is_single_member()) {
        return CreateSigShareForSingleMember(quorum, id, msgHash);
    }
    cxxtimer::Timer t(true);
    const CBLSSecretKey& skShare = quorum.GetSkShare();
    if (!skShare.IsValid()) {
        LogPrint(BCLog::LLMQ_SIGS, "CSigSharesManager::%s -- we don't have our skShare for quorum %s\n", __func__, quorum.qc->quorumHash.ToString());
        return std::nullopt;
    }

    int memberIdx = quorum.GetMemberIndex(activeMasterNodeProTxHash);
    if (memberIdx == -1) {
        // this should really not happen (IsValidMember gave true)
        return std::nullopt;
    }

    CSigShare sigShare(quorum.params.type, quorum.qc->quorumHash, id, msgHash, uint16_t(memberIdx), {});
    uint256 signHash = sigShare.buildSignHash().Get();

    auto bls_scheme = bls::bls_legacy_scheme.load();
    sigShare.sigShare.Set(skShare.Sign(signHash, bls_scheme), bls_scheme);
    if (!sigShare.sigShare.Get().IsValid()) {
        LogPrintf("CSigSharesManager::%s -- failed to sign sigShare. signHash=%s, id=%s, msgHash=%s, time=%s\n", __func__,
                  signHash.ToString(), sigShare.getId().ToString(), sigShare.getMsgHash().ToString(), t.count());
        return std::nullopt;
    }

    sigShare.UpdateKey();

    LogPrint(BCLog::LLMQ_SIGS, "CSigSharesManager::%s -- created sigShare. signHash=%s, id=%s, msgHash=%s, llmqType=%d, quorum=%s, time=%s\n", __func__,
              signHash.ToString(), sigShare.getId().ToString(), sigShare.getMsgHash().ToString(), ToUnderlying(quorum.params.type), quorum.qc->quorumHash.ToString(), t.count());

    return sigShare;
}

// causes all known sigShares to be re-announced
void CSigSharesManager::ForceReAnnouncement(const CQuorum& quorum, Consensus::LLMQType llmqType, const uint256& id, const uint256& msgHash)
{
    if (IsAllMembersConnectedEnabled(llmqType, m_sporkman)) {
        return;
    }

    LOCK(cs);
    auto signHash = SignHash(llmqType, quorum.qc->quorumHash, id, msgHash).Get();
    if (const auto *const sigs = sigShares.GetAllForSignHash(signHash)) {
        for (const auto& [quorumMemberIndex, _] : *sigs) {
            // re-announce every sigshare to every node
            sigSharesQueuedToAnnounce.Add(std::make_pair(signHash, quorumMemberIndex), true);
        }
    }
    for (auto& [_, nodeState] : nodeStates) {
        auto* session = nodeState.GetSessionBySignHash(signHash);
        if (session == nullptr) {
            continue;
        }
        // pretend that the other node doesn't know about any shares so that we re-announce everything
        session->knows.SetAll(false);
        // we need to use a new session id as we don't know if the other node has run into a timeout already
        session->sendSessionId = UNINITIALIZED_SESSION_ID;
    }
}

MessageProcessingResult CSigSharesManager::HandleNewRecoveredSig(const llmq::CRecoveredSig& recoveredSig)
{
    auto signHash = recoveredSig.buildSignHash().Get();
    LOCK(cs);
    RemoveSigSharesForSession(signHash);
    return {};
}
} // namespace llmq
