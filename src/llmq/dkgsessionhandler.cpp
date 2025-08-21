// Copyright (c) 2018-2025 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <llmq/dkgsessionhandler.h>

#include <llmq/commitment.h>
#include <llmq/dkgsession.h>
#include <llmq/blockprocessor.h>
#include <llmq/debug.h>
#include <llmq/options.h>
#include <llmq/utils.h>

#include <evo/deterministicmns.h>

#include <deploymentstatus.h>
#include <masternode/node.h>
#include <chainparams.h>
#include <net_processing.h>
#include <validation.h>
#include <util/thread.h>
#include <util/underlying.h>

namespace llmq
{

CDKGSessionHandler::CDKGSessionHandler(CBLSWorker& _blsWorker, CChainState& chainstate, CDeterministicMNManager& dmnman,
                                       CDKGDebugManager& _dkgDebugManager, CDKGSessionManager& _dkgManager,
                                       CMasternodeMetaMan& mn_metaman, CQuorumBlockProcessor& _quorumBlockProcessor,
                                       CQuorumSnapshotManager& qsnapman,
                                       const CActiveMasternodeManager* const mn_activeman, const CSporkManager& sporkman,
                                       const Consensus::LLMQParams& _params, int _quorumIndex) :
    blsWorker(_blsWorker),
    m_chainstate(chainstate),
    m_dmnman(dmnman),
    dkgDebugManager(_dkgDebugManager),
    dkgManager(_dkgManager),
    m_mn_metaman(mn_metaman),
    quorumBlockProcessor(_quorumBlockProcessor),
    m_qsnapman(qsnapman),
    m_mn_activeman(mn_activeman),
    m_sporkman(sporkman),
    params(_params),
    quorumIndex(_quorumIndex),
    curSession(std::make_unique<CDKGSession>(nullptr, _params, _blsWorker, dmnman, _dkgManager, _dkgDebugManager,
                                             m_mn_metaman, m_qsnapman, m_mn_activeman, sporkman)),
    pendingContributions(
        (size_t)_params.size * 2,
        MSG_QUORUM_CONTRIB), // we allow size*2 messages as we need to make sure we see bad behavior (double messages)
    pendingComplaints((size_t)_params.size * 2, MSG_QUORUM_COMPLAINT),
    pendingJustifications((size_t)_params.size * 2, MSG_QUORUM_JUSTIFICATION),
    pendingPrematureCommitments((size_t)_params.size * 2, MSG_QUORUM_PREMATURE_COMMITMENT)
{
    if (params.type == Consensus::LLMQType::LLMQ_NONE) {
        throw std::runtime_error("Can't initialize CDKGSessionHandler with LLMQ_NONE type.");
    }
}

CDKGSessionHandler::~CDKGSessionHandler() = default;

MessageProcessingResult CDKGPendingMessages::PushPendingMessage(NodeId from, CDataStream& vRecv)
{
    // this will also consume the data, even if we bail out early
    auto pm = std::make_shared<CDataStream>(std::move(vRecv));

    CHashWriter hw(SER_GETHASH, 0);
    hw.write(AsWritableBytes(Span{*pm}));
    uint256 hash = hw.GetHash();

    MessageProcessingResult ret{};
    if (from != -1) {
        ret.m_to_erase = CInv{invType, hash};
    }

    LOCK(cs_messages);

    if (messagesPerNode[from] >= maxMessagesPerNode) {
        // TODO ban?
        LogPrint(BCLog::LLMQ_DKG, "CDKGPendingMessages::%s -- too many messages, peer=%d\n", __func__, from);
        return ret;
    }
    messagesPerNode[from]++;

    if (!seenMessages.emplace(hash).second) {
        LogPrint(BCLog::LLMQ_DKG, "CDKGPendingMessages::%s -- already seen %s, peer=%d\n", __func__, hash.ToString(), from);
        return ret;
    }

    pendingMessages.emplace_back(std::make_pair(from, std::move(pm)));
    return ret;
}

std::list<CDKGPendingMessages::BinaryMessage> CDKGPendingMessages::PopPendingMessages(size_t maxCount)
{
    LOCK(cs_messages);

    std::list<BinaryMessage> ret;
    while (!pendingMessages.empty() && ret.size() < maxCount) {
        ret.emplace_back(std::move(pendingMessages.front()));
        pendingMessages.pop_front();
    }

    return ret;
}

bool CDKGPendingMessages::HasSeen(const uint256& hash) const
{
    LOCK(cs_messages);
    return seenMessages.count(hash) != 0;
}

void CDKGPendingMessages::Misbehaving(const NodeId from, const int score, PeerManager& peerman)
{
    if (from == -1) return;
    peerman.Misbehaving(from, score);
}

void CDKGPendingMessages::Clear()
{
    LOCK(cs_messages);
    pendingMessages.clear();
    messagesPerNode.clear();
    seenMessages.clear();
}

//////

void CDKGSessionHandler::UpdatedBlockTip(const CBlockIndex* pindexNew)
{
    //AssertLockNotHeld(cs_main);
    //Indexed quorums (greater than 0) are enabled with Quorum Rotation
    if (quorumIndex > 0 && !IsQuorumRotationEnabled(params, pindexNew)) {
        return;
    }
    LOCK(cs_phase_qhash);

    int quorumStageInt = (pindexNew->nHeight - quorumIndex) % params.dkgInterval;

    const CBlockIndex* pQuorumBaseBlockIndex = pindexNew->GetAncestor(pindexNew->nHeight - quorumStageInt);

    currentHeight = pindexNew->nHeight;
    quorumHash = pQuorumBaseBlockIndex->GetBlockHash();

    bool fNewPhase = (quorumStageInt % params.dkgPhaseBlocks) == 0;
    int phaseInt = quorumStageInt / params.dkgPhaseBlocks + 1;
    QuorumPhase oldPhase = phase;
    if (fNewPhase && phaseInt >= ToUnderlying(QuorumPhase::Initialized) && phaseInt <= ToUnderlying(QuorumPhase::Idle)) {
        phase = static_cast<QuorumPhase>(phaseInt);
    }

    LogPrint(BCLog::LLMQ_DKG, "CDKGSessionHandler::%s -- %s qi[%d] currentHeight=%d, pQuorumBaseBlockIndex->nHeight=%d, oldPhase=%d, newPhase=%d\n", __func__,
             params.name, quorumIndex, currentHeight, pQuorumBaseBlockIndex->nHeight, ToUnderlying(oldPhase), ToUnderlying(phase));
}

MessageProcessingResult CDKGSessionHandler::ProcessMessage(NodeId from, std::string_view msg_type, CDataStream& vRecv)
{
    // We don't handle messages in the calling thread as deserialization/processing of these would block everything
    if (msg_type == NetMsgType::QCONTRIB) {
        return pendingContributions.PushPendingMessage(from, vRecv);
    } else if (msg_type == NetMsgType::QCOMPLAINT) {
        return pendingComplaints.PushPendingMessage(from, vRecv);
    } else if (msg_type == NetMsgType::QJUSTIFICATION) {
        return pendingJustifications.PushPendingMessage(from, vRecv);
    } else if (msg_type == NetMsgType::QPCOMMITMENT) {
        return pendingPrematureCommitments.PushPendingMessage(from, vRecv);
    }
    return {};
}

void CDKGSessionHandler::StartThread(CConnman& connman, PeerManager& peerman)
{
    if (phaseHandlerThread.joinable()) {
        throw std::runtime_error("Tried to start an already started CDKGSessionHandler thread.");
    }

    m_thread_name = strprintf("llmq-%d-%d", ToUnderlying(params.type), quorumIndex);
    phaseHandlerThread = std::thread(&util::TraceThread, m_thread_name.c_str(),
                                     [this, &connman, &peerman] { PhaseHandlerThread(connman, peerman); });
}

void CDKGSessionHandler::StopThread()
{
    stopRequested = true;
    if (phaseHandlerThread.joinable()) {
        phaseHandlerThread.join();
    }
}

bool CDKGSessionHandler::InitNewQuorum(const CBlockIndex* pQuorumBaseBlockIndex)
{
    if (!DeploymentDIP0003Enforced(pQuorumBaseBlockIndex->nHeight, Params().GetConsensus())) {
        return false;
    }

    curSession = std::make_unique<CDKGSession>(pQuorumBaseBlockIndex, params, blsWorker, m_dmnman, dkgManager,
                                               dkgDebugManager, m_mn_metaman, m_qsnapman, m_mn_activeman, m_sporkman);

    if (!curSession->Init(m_mn_activeman->GetProTxHash(), quorumIndex)) {
        LogPrintf("CDKGSessionManager::%s -- height[%d] quorum initialization failed for %s qi[%d]\n", __func__,
                  pQuorumBaseBlockIndex->nHeight, curSession->params.name, quorumIndex);
        return false;
    }

    LogPrintf("CDKGSessionManager::%s -- height[%d] quorum initialization OK for %s qi[%d]\n", __func__, pQuorumBaseBlockIndex->nHeight, curSession->params.name, quorumIndex);
    return true;
}

std::pair<QuorumPhase, uint256> CDKGSessionHandler::GetPhaseAndQuorumHash() const
{
    LOCK(cs_phase_qhash);
    return std::make_pair(phase, quorumHash);
}

class AbortPhaseException : public std::exception {
};

void CDKGSessionHandler::WaitForNextPhase(std::optional<QuorumPhase> curPhase,
                                          QuorumPhase nextPhase,
                                          const uint256& expectedQuorumHash,
                                          const WhileWaitFunc& shouldNotWait) const
{
    LogPrint(BCLog::LLMQ_DKG, "CDKGSessionManager::%s -- %s qi[%d] - starting, curPhase=%d, nextPhase=%d\n", __func__, params.name, quorumIndex, curPhase.has_value() ? ToUnderlying(*curPhase) : -1, ToUnderlying(nextPhase));

    while (true) {
        if (stopRequested) {
            LogPrint(BCLog::LLMQ_DKG, "CDKGSessionManager::%s -- %s qi[%d] - aborting due to stop/shutdown requested\n", __func__, params.name, quorumIndex);
            throw AbortPhaseException();
        }
        auto [_phase, _quorumHash] = GetPhaseAndQuorumHash();
        if (!expectedQuorumHash.IsNull() && _quorumHash != expectedQuorumHash) {
            LogPrint(BCLog::LLMQ_DKG, "CDKGSessionManager::%s -- %s qi[%d] - aborting due unexpected expectedQuorumHash change\n", __func__, params.name, quorumIndex);
            throw AbortPhaseException();
        }
        if (_phase == nextPhase) {
            break;
        }
        if (curPhase.has_value() && _phase != curPhase) {
            LogPrint(BCLog::LLMQ_DKG, "CDKGSessionManager::%s -- %s qi[%d] - aborting due unexpected phase change, _phase=%d, curPhase=%d\n", __func__, params.name, quorumIndex, ToUnderlying(_phase), curPhase.has_value() ? ToUnderlying(*curPhase) : -1);
            throw AbortPhaseException();
        }
        if (!shouldNotWait()) {
            UninterruptibleSleep(std::chrono::milliseconds{100});
        }
    }

    LogPrint(BCLog::LLMQ_DKG, "CDKGSessionManager::%s -- %s qi[%d] - done, curPhase=%d, nextPhase=%d\n", __func__, params.name, quorumIndex, curPhase.has_value() ? ToUnderlying(*curPhase) : -1, ToUnderlying(nextPhase));

    if (nextPhase == QuorumPhase::Initialized) {
        dkgDebugManager.ResetLocalSessionStatus(params.type, quorumIndex);
    } else {
        dkgDebugManager.UpdateLocalSessionStatus(params.type, quorumIndex, [&](CDKGDebugSessionStatus& status) {
            bool changed = status.phase != nextPhase;
            status.phase = nextPhase;
            return changed;
        });
    }
}

void CDKGSessionHandler::WaitForNewQuorum(const uint256& oldQuorumHash) const
{
    LogPrint(BCLog::LLMQ_DKG, "CDKGSessionManager::%s -- %s qi[%d]- starting\n", __func__, params.name, quorumIndex);

    while (true) {
        if (stopRequested) {
            LogPrint(BCLog::LLMQ_DKG, "CDKGSessionManager::%s -- %s qi[%d] - aborting due to stop/shutdown requested\n", __func__, params.name, quorumIndex);
            throw AbortPhaseException();
        }
        auto [_, _quorumHash] = GetPhaseAndQuorumHash();
        if (_quorumHash != oldQuorumHash) {
            break;
        }
        UninterruptibleSleep(std::chrono::milliseconds{100});
    }

    LogPrint(BCLog::LLMQ_DKG, "CDKGSessionManager::%s -- %s qi[%d] - done\n", __func__, params.name, quorumIndex);
}

// Sleep some time to not fully overload the whole network
void CDKGSessionHandler::SleepBeforePhase(QuorumPhase curPhase,
                                          const uint256& expectedQuorumHash,
                                          double randomSleepFactor,
                                          const WhileWaitFunc& runWhileWaiting) const
{
    if (!curSession->AreWeMember()) {
        // Non-members do not participate and do not create any network load, no need to sleep.
        return;
    }

    if (Params().MineBlocksOnDemand()) {
        // On regtest, blocks can be mined on demand without any significant time passing between these.
        // We shouldn't wait before phases in this case.
        return;
    }

    // Two blocks can come very close to each other, this happens pretty regularly. We don't want to be
    // left behind and marked as a bad member. This means that we should not count the last block of the
    // phase as a safe one to keep sleeping, that's why we calculate the phase sleep time as a time of
    // the full phase minus one block here.
    double phaseSleepTime = (params.dkgPhaseBlocks - 1) * Params().GetConsensus().nPowTargetSpacing * 1000;
    // Expected phase sleep time per member
    double phaseSleepTimePerMember = phaseSleepTime / params.size;
    // Don't expect perfect block times and thus reduce the phase time to be on the secure side (caller chooses factor)
    double adjustedPhaseSleepTimePerMember = phaseSleepTimePerMember * randomSleepFactor;

    int64_t sleepTime = (int64_t)(adjustedPhaseSleepTimePerMember * curSession->GetMyMemberIndex().value_or(0));
    int64_t endTime = TicksSinceEpoch<std::chrono::milliseconds>(SystemClock::now()) + sleepTime;
    int heightTmp{currentHeight.load()};
    int heightStart{heightTmp};

    LogPrint(BCLog::LLMQ_DKG, "CDKGSessionManager::%s -- %s qi[%d] - starting sleep for %d ms, curPhase=%d\n", __func__, params.name, quorumIndex, sleepTime, ToUnderlying(curPhase));

    while (TicksSinceEpoch<std::chrono::milliseconds>(SystemClock::now()) < endTime) {
        if (stopRequested) {
            LogPrint(BCLog::LLMQ_DKG, "CDKGSessionManager::%s -- %s qi[%d] - aborting due to stop/shutdown requested\n", __func__, params.name, quorumIndex);
            throw AbortPhaseException();
        }
        auto cur_height = currentHeight.load();
        if (cur_height > heightTmp) {
            // New block(s) just came in
            int64_t expectedBlockTime = (cur_height - heightStart) * Params().GetConsensus().nPowTargetSpacing * 1000;
            if (expectedBlockTime > sleepTime) {
                // Blocks came faster than we expected, jump into the phase func asap
                break;
            }
            heightTmp = cur_height;
        }
        if (WITH_LOCK(cs_phase_qhash, return phase != curPhase || quorumHash != expectedQuorumHash)) {
            // Something went wrong and/or we missed quite a few blocks and it's just too late now
            LogPrint(BCLog::LLMQ_DKG, "CDKGSessionManager::%s -- %s qi[%d] - aborting due unexpected phase/expectedQuorumHash change\n", __func__, params.name, quorumIndex);
            throw AbortPhaseException();
        }
        if (!runWhileWaiting()) {
            UninterruptibleSleep(std::chrono::milliseconds{100});
        }
    }

    LogPrint(BCLog::LLMQ_DKG, "CDKGSessionManager::%s -- %s qi[%d] - done, curPhase=%d\n", __func__, params.name, quorumIndex, ToUnderlying(curPhase));
}

void CDKGSessionHandler::HandlePhase(QuorumPhase curPhase,
                                     QuorumPhase nextPhase,
                                     const uint256& expectedQuorumHash,
                                     double randomSleepFactor,
                                     const StartPhaseFunc& startPhaseFunc,
                                     const WhileWaitFunc& runWhileWaiting)
{
    LogPrint(BCLog::LLMQ_DKG, "CDKGSessionManager::%s -- %s qi[%d] - starting, curPhase=%d, nextPhase=%d\n", __func__, params.name, quorumIndex, ToUnderlying(curPhase), ToUnderlying(nextPhase));

    SleepBeforePhase(curPhase, expectedQuorumHash, randomSleepFactor, runWhileWaiting);
    startPhaseFunc();
    WaitForNextPhase(curPhase, nextPhase, expectedQuorumHash, runWhileWaiting);

    LogPrint(BCLog::LLMQ_DKG, "CDKGSessionManager::%s -- %s qi[%d] - done, curPhase=%d, nextPhase=%d\n", __func__, params.name, quorumIndex, ToUnderlying(curPhase), ToUnderlying(nextPhase));
}

// returns a set of NodeIds which sent invalid messages
template<typename Message>
std::set<NodeId> BatchVerifyMessageSigs(CDKGSession& session, const std::vector<std::pair<NodeId, std::shared_ptr<Message>>>& messages)
{
    if (messages.empty()) {
        return {};
    }

    std::set<NodeId> ret;
    bool revertToSingleVerification = false;

    CBLSSignature aggSig;
    std::vector<CBLSPublicKey> pubKeys;
    std::vector<uint256> messageHashes;
    std::set<uint256> messageHashesSet;
    pubKeys.reserve(messages.size());
    messageHashes.reserve(messages.size());
    bool first = true;
    for (const auto& [nodeId, msg] : messages) {
        auto member = session.GetMember(msg->proTxHash);
        if (!member) {
            // should not happen as it was verified before
            ret.emplace(nodeId);
            continue;
        }

        if (first) {
            aggSig = msg->sig;
        } else {
            aggSig.AggregateInsecure(msg->sig);
        }
        first = false;

        auto msgHash = msg->GetSignHash();
        if (!messageHashesSet.emplace(msgHash).second) {
            // can only happen in 2 cases:
            // 1. Someone sent us the same message twice but with differing signature, meaning that at least one of them
            //    must be invalid. In this case, we'd have to revert to single message verification nevertheless
            // 2. Someone managed to find a way to create two different binary representations of a message that deserializes
            //    to the same object representation. This would be some form of malleability. However, this shouldn't be
            //    possible as only deterministic/unique BLS signatures and very simple data types are involved
            revertToSingleVerification = true;
            break;
        }

        pubKeys.emplace_back(member->dmn->pdmnState->pubKeyOperator.Get());
        messageHashes.emplace_back(msgHash);
    }
    if (!revertToSingleVerification) {
        if (aggSig.VerifyInsecureAggregated(pubKeys, messageHashes)) {
            // all good
            return ret;
        }

        // are all messages from the same node?
        bool nodeIdsAllSame = std::adjacent_find( messages.begin(), messages.end(), [](const auto& first, const auto& second){
            return first.first != second.first;
        }) == messages.end();

        // if yes, take a short path and return a set with only him
        if (nodeIdsAllSame) {
            ret.emplace(messages[0].first);
            return ret;
        }
        // different nodes, let's figure out who are the bad ones
    }

    for (const auto& [nodeId, msg] : messages) {
        if (ret.count(nodeId)) {
            continue;
        }

        auto member = session.GetMember(msg->proTxHash);
        bool valid = msg->sig.VerifyInsecure(member->dmn->pdmnState->pubKeyOperator.Get(), msg->GetSignHash());
        if (!valid) {
            ret.emplace(nodeId);
        }
    }
    return ret;
}

static void RelayInvToParticipants(const CDKGSession& session, const CConnman& connman, PeerManager& peerman,
                                   const CInv& inv)
{
    CDKGLogger logger(session, __func__, __LINE__);
    std::stringstream ss;
    const auto& relayMembers = session.RelayMembers();
    for (const auto& r : relayMembers) {
        ss << r.ToString().substr(0, 4) << " | ";
    }
    logger.Batch("RelayInvToParticipants inv[%s] relayMembers[%d] GetNodeCount[%d] GetNetworkActive[%d] "
                 "HasMasternodeQuorumNodes[%d] for quorumHash[%s] forMember[%s] relayMembers[%s]",
                 inv.ToString(), relayMembers.size(), connman.GetNodeCount(ConnectionDirection::Both),
                 connman.GetNetworkActive(),
                 connman.HasMasternodeQuorumNodes(session.GetParams().type, session.BlockIndex()->GetBlockHash()),
                 session.BlockIndex()->GetBlockHash().ToString(), session.ProTx().ToString().substr(0, 4), ss.str());

    std::stringstream ss2;
    connman.ForEachNode([&](const CNode* pnode) {
        if (pnode->qwatch ||
            (!pnode->GetVerifiedProRegTxHash().IsNull() && (relayMembers.count(pnode->GetVerifiedProRegTxHash()) != 0))) {
            peerman.PushInventory(pnode->GetId(), inv);
        }

        if (pnode->GetVerifiedProRegTxHash().IsNull()) {
            logger.Batch("node[%d:%s] not mn", pnode->GetId(), pnode->m_addr_name);
        } else if (relayMembers.count(pnode->GetVerifiedProRegTxHash()) == 0) {
            ss2 << pnode->GetVerifiedProRegTxHash().ToString().substr(0, 4) << " | ";
        }
    });
    logger.Batch("forMember[%s] NOTrelayMembers[%s]", session.ProTx().ToString().substr(0, 4), ss2.str());
    logger.Flush();
}

template <typename Message>
bool ProcessPendingMessageBatch(const CConnman& connman, CDKGSession& session, CDKGPendingMessages& pendingMessages,
                                PeerManager& peerman, size_t maxCount)
{
    auto msgs = pendingMessages.PopAndDeserializeMessages<Message>(maxCount);
    if (msgs.empty()) {
        return false;
    }

    std::vector<std::pair<NodeId, std::shared_ptr<Message>>> preverifiedMessages;
    preverifiedMessages.reserve(msgs.size());

    for (const auto& p : msgs) {
        const NodeId &nodeId = p.first;
        if (!p.second) {
            LogPrint(BCLog::LLMQ_DKG, "%s -- failed to deserialize message, peer=%d\n", __func__, nodeId);
            {
                pendingMessages.Misbehaving(nodeId, 100, peerman);
            }
            continue;
        }
        bool ban = false;
        if (!session.PreVerifyMessage(*p.second, ban)) {
            if (ban) {
                LogPrint(BCLog::LLMQ_DKG, "%s -- banning node due to failed preverification, peer=%d\n", __func__, nodeId);
                {
                    pendingMessages.Misbehaving(nodeId, 100, peerman);
                }
            }
            LogPrint(BCLog::LLMQ_DKG, "%s -- skipping message due to failed preverification, peer=%d\n", __func__, nodeId);
            continue;
        }
        preverifiedMessages.emplace_back(p);
    }
    if (preverifiedMessages.empty()) {
        return true;
    }

    auto badNodes = BatchVerifyMessageSigs(session, preverifiedMessages);
    if (!badNodes.empty()) {
        for (auto nodeId : badNodes) {
            LogPrint(BCLog::LLMQ_DKG, "%s -- failed to verify signature, peer=%d\n", __func__, nodeId);
            pendingMessages.Misbehaving(nodeId, 100, peerman);
        }
    }

    for (const auto& p : preverifiedMessages) {
        const NodeId &nodeId = p.first;
        if (badNodes.count(nodeId)) {
            continue;
        }
        const std::optional<CInv> inv = session.ReceiveMessage(*p.second);
        if (inv) {
            RelayInvToParticipants(session, connman, peerman, *inv);
        }
    }

    return true;
}

void CDKGSessionHandler::HandleDKGRound(CConnman& connman, PeerManager& peerman)
{
    WaitForNextPhase(std::nullopt, QuorumPhase::Initialized);

    pendingContributions.Clear();
    pendingComplaints.Clear();
    pendingJustifications.Clear();
    pendingPrematureCommitments.Clear();
    uint256 curQuorumHash = WITH_LOCK(cs_phase_qhash, return quorumHash);

    const CBlockIndex* pQuorumBaseBlockIndex = WITH_LOCK(::cs_main, return m_chainstate.m_blockman.LookupBlockIndex(curQuorumHash));

    if (!pQuorumBaseBlockIndex || !InitNewQuorum(pQuorumBaseBlockIndex)) {
        // should actually never happen
        WaitForNewQuorum(curQuorumHash);
        throw AbortPhaseException();
    }

    dkgDebugManager.UpdateLocalSessionStatus(params.type, quorumIndex, [&](CDKGDebugSessionStatus& status) {
        bool changed = status.phase != QuorumPhase::Initialized;
        status.phase = QuorumPhase::Initialized;
        return changed;
    });

    if (params.size == 1) {
        auto finalCommitment = curSession->FinalizeSingleCommitment();
        if (!finalCommitment.IsNull()) { // it can be null only if we are not member
            if (auto inv_opt = quorumBlockProcessor.AddMineableCommitment(finalCommitment); inv_opt.has_value()) {
                peerman.RelayInv(inv_opt.value());
            }
        }
        WaitForNextPhase(QuorumPhase::Initialized, QuorumPhase::Contribute, curQuorumHash);
        return;
    }

    const auto tip_mn_list = m_dmnman.GetListAtChainTip();
    utils::EnsureQuorumConnections(params, connman, m_dmnman, m_sporkman, m_qsnapman, tip_mn_list, pQuorumBaseBlockIndex,
                                   curSession->myProTxHash, /* is_masternode = */ m_mn_activeman != nullptr);
    if (curSession->AreWeMember()) {
        utils::AddQuorumProbeConnections(params, connman, m_dmnman, m_mn_metaman, m_qsnapman, m_sporkman, tip_mn_list,
                                         pQuorumBaseBlockIndex, curSession->myProTxHash);
    }

    WaitForNextPhase(QuorumPhase::Initialized, QuorumPhase::Contribute, curQuorumHash);

    // Contribute
    auto fContributeStart = [this, &peerman]() { curSession->Contribute(pendingContributions, peerman); };
    auto fContributeWait = [this, &connman, &peerman] {
        return ProcessPendingMessageBatch<CDKGContribution>(connman, *curSession, pendingContributions, peerman, 8);
    };
    HandlePhase(QuorumPhase::Contribute, QuorumPhase::Complain, curQuorumHash, 0.05, fContributeStart, fContributeWait);

    // Complain
    auto fComplainStart = [this, &connman, &peerman]() {
        curSession->VerifyAndComplain(connman, pendingComplaints, peerman);
    };
    auto fComplainWait = [this, &connman, &peerman] {
        return ProcessPendingMessageBatch<CDKGComplaint>(connman, *curSession, pendingComplaints, peerman, 8);
    };
    HandlePhase(QuorumPhase::Complain, QuorumPhase::Justify, curQuorumHash, 0.05, fComplainStart, fComplainWait);

    // Justify
    auto fJustifyStart = [this, &peerman]() { curSession->VerifyAndJustify(pendingJustifications, peerman); };
    auto fJustifyWait = [this, &connman, &peerman] {
        return ProcessPendingMessageBatch<CDKGJustification>(connman, *curSession, pendingJustifications, peerman, 8);
    };
    HandlePhase(QuorumPhase::Justify, QuorumPhase::Commit, curQuorumHash, 0.05, fJustifyStart, fJustifyWait);

    // Commit
    auto fCommitStart = [this, &peerman]() { curSession->VerifyAndCommit(pendingPrematureCommitments, peerman); };
    auto fCommitWait = [this, &connman, &peerman] {
        return ProcessPendingMessageBatch<CDKGPrematureCommitment>(connman, *curSession, pendingPrematureCommitments,
                                                                   peerman, 8);
    };
    HandlePhase(QuorumPhase::Commit, QuorumPhase::Finalize, curQuorumHash, 0.1, fCommitStart, fCommitWait);

    auto finalCommitments = curSession->FinalizeCommitments();
    for (const auto& fqc : finalCommitments) {
        if (auto inv_opt = quorumBlockProcessor.AddMineableCommitment(fqc); inv_opt.has_value()) {
            peerman.RelayInv(inv_opt.value());
        }
    }
}

void CDKGSessionHandler::PhaseHandlerThread(CConnman& connman, PeerManager& peerman)
{
    while (!stopRequested) {
        try {
            LogPrint(BCLog::LLMQ_DKG, "CDKGSessionHandler::%s -- %s qi[%d] - starting HandleDKGRound\n", __func__, params.name, quorumIndex);
            HandleDKGRound(connman, peerman);
        } catch (AbortPhaseException& e) {
            dkgDebugManager.UpdateLocalSessionStatus(params.type, quorumIndex, [&](CDKGDebugSessionStatus& status) {
                status.statusBits.aborted = true;
                return true;
            });
            LogPrint(BCLog::LLMQ_DKG, "CDKGSessionHandler::%s -- %s qi[%d] - aborted current DKG session\n", __func__, params.name, quorumIndex);
        }
    }
}

bool CDKGSessionHandler::GetContribution(const uint256& hash, CDKGContribution& ret) const
{
    LOCK(curSession->invCs);
    auto it = curSession->contributions.find(hash);
    if (it != curSession->contributions.end()) {
        ret = it->second;
        return true;
    }
    return false;
}

bool CDKGSessionHandler::GetComplaint(const uint256& hash, CDKGComplaint& ret) const
{
    LOCK(curSession->invCs);
    auto it = curSession->complaints.find(hash);
    if (it != curSession->complaints.end()) {
        ret = it->second;
        return true;
    }
    return false;
}

bool CDKGSessionHandler::GetJustification(const uint256& hash, CDKGJustification& ret) const
{
    LOCK(curSession->invCs);
    auto it = curSession->justifications.find(hash);
    if (it != curSession->justifications.end()) {
        ret = it->second;
        return true;
    }
    return false;
}

bool CDKGSessionHandler::GetPrematureCommitment(const uint256& hash, CDKGPrematureCommitment& ret) const
{
    LOCK(curSession->invCs);
    auto it = curSession->prematureCommitments.find(hash);
    if (it != curSession->prematureCommitments.end() && curSession->validCommitments.count(hash)) {
        ret = it->second;
        return true;
    }
    return false;
}

} // namespace llmq
