// Copyright (c) 2018-2021 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <llmq/quorums_dkgsessionhandler.h>

#include <llmq/quorums_commitment.h>
#include <llmq/quorums_dkgsession.h>
#include <llmq/quorums_blockprocessor.h>
#include <llmq/quorums_debug.h>
#include <llmq/quorums_utils.h>

#include <evo/deterministicmns.h>

#include <masternode/activemasternode.h>
#include <chainparams.h>
#include <net_processing.h>
#include <spork.h>

namespace llmq
{

CDKGPendingMessages::CDKGPendingMessages(size_t _maxMessagesPerNode, int _invType) :
    maxMessagesPerNode(_maxMessagesPerNode),
    invType(_invType)
{
}

void CDKGPendingMessages::PushPendingMessage(NodeId from, CDataStream& vRecv)
{
    // this will also consume the data, even if we bail out early
    auto pm = std::make_shared<CDataStream>(std::move(vRecv));

    CHashWriter hw(SER_GETHASH, 0);
    hw.write(pm->data(), pm->size());
    uint256 hash = hw.GetHash();

    if (from != -1) {
        LOCK(cs_main);
        EraseObjectRequest(from, CInv(invType, hash));
    }

    LOCK(cs);

    if (messagesPerNode[from] >= maxMessagesPerNode) {
        // TODO ban?
        LogPrint(BCLog::LLMQ_DKG, "CDKGPendingMessages::%s -- too many messages, peer=%d\n", __func__, from);
        return;
    }
    messagesPerNode[from]++;

    if (!seenMessages.emplace(hash).second) {
        LogPrint(BCLog::LLMQ_DKG, "CDKGPendingMessages::%s -- already seen %s, peer=%d\n", __func__, hash.ToString(), from);
        return;
    }

    pendingMessages.emplace_back(std::make_pair(from, std::move(pm)));
}

std::list<CDKGPendingMessages::BinaryMessage> CDKGPendingMessages::PopPendingMessages(size_t maxCount)
{
    LOCK(cs);

    std::list<BinaryMessage> ret;
    while (!pendingMessages.empty() && ret.size() < maxCount) {
        ret.emplace_back(std::move(pendingMessages.front()));
        pendingMessages.pop_front();
    }

    return ret;
}

bool CDKGPendingMessages::HasSeen(const uint256& hash) const
{
    LOCK(cs);
    return seenMessages.count(hash) != 0;
}

void CDKGPendingMessages::Clear()
{
    LOCK(cs);
    pendingMessages.clear();
    messagesPerNode.clear();
    seenMessages.clear();
}

//////

CDKGSessionHandler::CDKGSessionHandler(const Consensus::LLMQParams& _params, CBLSWorker& _blsWorker, CDKGSessionManager& _dkgManager) :
    params(_params),
    blsWorker(_blsWorker),
    dkgManager(_dkgManager),
    curSession(std::make_shared<CDKGSession>(_params, _blsWorker, _dkgManager)),
    pendingContributions((size_t)_params.size * 2, MSG_QUORUM_CONTRIB), // we allow size*2 messages as we need to make sure we see bad behavior (double messages)
    pendingComplaints((size_t)_params.size * 2, MSG_QUORUM_COMPLAINT),
    pendingJustifications((size_t)_params.size * 2, MSG_QUORUM_JUSTIFICATION),
    pendingPrematureCommitments((size_t)_params.size * 2, MSG_QUORUM_PREMATURE_COMMITMENT)
{
    if (params.type == Consensus::LLMQ_NONE) {
        throw std::runtime_error("Can't initialize CDKGSessionHandler with LLMQ_NONE type.");
    }
}

CDKGSessionHandler::~CDKGSessionHandler() = default;

void CDKGSessionHandler::UpdatedBlockTip(const CBlockIndex* pindexNew)
{
    LOCK(cs);

    int quorumStageInt = pindexNew->nHeight % params.dkgInterval;
    const CBlockIndex* pindexQuorum = pindexNew->GetAncestor(pindexNew->nHeight - quorumStageInt);

    currentHeight = pindexNew->nHeight;
    quorumHash = pindexQuorum->GetBlockHash();

    bool fNewPhase = (quorumStageInt % params.dkgPhaseBlocks) == 0;
    int phaseInt = quorumStageInt / params.dkgPhaseBlocks + 1;
    QuorumPhase oldPhase = phase;
    if (fNewPhase && phaseInt >= QuorumPhase_Initialized && phaseInt <= QuorumPhase_Idle) {
        phase = static_cast<QuorumPhase>(phaseInt);
    }

    LogPrint(BCLog::LLMQ_DKG, "CDKGSessionHandler::%s -- %s - currentHeight=%d, pindexQuorum->nHeight=%d, oldPhase=%d, newPhase=%d\n", __func__,
            params.name, currentHeight, pindexQuorum->nHeight, oldPhase, phase);
}

void CDKGSessionHandler::ProcessMessage(CNode* pfrom, const std::string& strCommand, CDataStream& vRecv)
{
    // We don't handle messages in the calling thread as deserialization/processing of these would block everything
    if (strCommand == NetMsgType::QCONTRIB) {
        pendingContributions.PushPendingMessage(pfrom->GetId(), vRecv);
    } else if (strCommand == NetMsgType::QCOMPLAINT) {
        pendingComplaints.PushPendingMessage(pfrom->GetId(), vRecv);
    } else if (strCommand == NetMsgType::QJUSTIFICATION) {
        pendingJustifications.PushPendingMessage(pfrom->GetId(), vRecv);
    } else if (strCommand == NetMsgType::QPCOMMITMENT) {
        pendingPrematureCommitments.PushPendingMessage(pfrom->GetId(), vRecv);
    }
}

void CDKGSessionHandler::StartThread()
{
    if (phaseHandlerThread.joinable()) {
        throw std::runtime_error("Tried to start an already started CDKGSessionHandler thread.");
    }

    std::string threadName = strprintf("llmq-%d", (uint8_t)params.type);
    phaseHandlerThread = std::thread(&TraceThread<std::function<void()> >, threadName, std::function<void()>(std::bind(&CDKGSessionHandler::PhaseHandlerThread, this)));
}

void CDKGSessionHandler::StopThread()
{
    stopRequested = true;
    if (phaseHandlerThread.joinable()) {
        phaseHandlerThread.join();
    }
}

bool CDKGSessionHandler::InitNewQuorum(const CBlockIndex* pindexQuorum)
{
    curSession = std::make_shared<CDKGSession>(params, blsWorker, dkgManager);

    if (!deterministicMNManager->IsDIP3Enforced(pindexQuorum->nHeight)) {
        return false;
    }

    auto mns = CLLMQUtils::GetAllQuorumMembers(params.type, pindexQuorum);

    if (!curSession->Init(pindexQuorum, mns, activeMasternodeInfo.proTxHash)) {
        LogPrintf("CDKGSessionManager::%s -- quorum initialization failed for %s\n", __func__, curSession->params.name);
        return false;
    }

    return true;
}

std::pair<QuorumPhase, uint256> CDKGSessionHandler::GetPhaseAndQuorumHash() const
{
    LOCK(cs);
    return std::make_pair(phase, quorumHash);
}

class AbortPhaseException : public std::exception {
};

void CDKGSessionHandler::WaitForNextPhase(QuorumPhase curPhase,
                                          QuorumPhase nextPhase,
                                          const uint256& expectedQuorumHash,
                                          const WhileWaitFunc& runWhileWaiting) const
{
    LogPrint(BCLog::LLMQ_DKG, "CDKGSessionManager::%s -- %s - starting, curPhase=%d, nextPhase=%d\n", __func__, params.name, curPhase, nextPhase);

    while (true) {
        if (stopRequested) {
            LogPrint(BCLog::LLMQ_DKG, "CDKGSessionManager::%s -- %s - aborting due to stop/shutdown requested\n", __func__, params.name);
            throw AbortPhaseException();
        }
        auto p = GetPhaseAndQuorumHash();
        if (!expectedQuorumHash.IsNull() && p.second != expectedQuorumHash) {
            LogPrint(BCLog::LLMQ_DKG, "CDKGSessionManager::%s -- %s - aborting due unexpected expectedQuorumHash change\n", __func__, params.name);
            throw AbortPhaseException();
        }
        if (p.first == nextPhase) {
            break;
        }
        if (curPhase != QuorumPhase_None && p.first != curPhase) {
            LogPrint(BCLog::LLMQ_DKG, "CDKGSessionManager::%s -- %s - aborting due unexpected phase change\n", __func__, params.name);
            throw AbortPhaseException();
        }
        if (!runWhileWaiting()) {
            MilliSleep(100);
        }
    }

    LogPrint(BCLog::LLMQ_DKG, "CDKGSessionManager::%s -- %s - done, curPhase=%d, nextPhase=%d\n", __func__, params.name, curPhase, nextPhase);

    if (nextPhase == QuorumPhase_Initialized) {
        quorumDKGDebugManager->ResetLocalSessionStatus(params.type);
    } else {
        quorumDKGDebugManager->UpdateLocalSessionStatus(params.type, [&](CDKGDebugSessionStatus& status) {
            bool changed = status.phase != (uint8_t) nextPhase;
            status.phase = (uint8_t) nextPhase;
            return changed;
        });
    }
}

void CDKGSessionHandler::WaitForNewQuorum(const uint256& oldQuorumHash) const
{
    LogPrint(BCLog::LLMQ_DKG, "CDKGSessionManager::%s -- %s - starting\n", __func__, params.name);

    while (true) {
        if (stopRequested) {
            LogPrint(BCLog::LLMQ_DKG, "CDKGSessionManager::%s -- %s - aborting due to stop/shutdown requested\n", __func__, params.name);
            throw AbortPhaseException();
        }
        auto p = GetPhaseAndQuorumHash();
        if (p.second != oldQuorumHash) {
            break;
        }
        MilliSleep(100);
    }

    LogPrint(BCLog::LLMQ_DKG, "CDKGSessionManager::%s -- %s - done\n", __func__, params.name);
}

// Sleep some time to not fully overload the whole network
void CDKGSessionHandler::SleepBeforePhase(QuorumPhase curPhase,
                                          const uint256& expectedQuorumHash,
                                          double randomSleepFactor,
                                          const WhileWaitFunc& runWhileWaiting)
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

    int64_t sleepTime = (int64_t)(adjustedPhaseSleepTimePerMember * curSession->GetMyMemberIndex());
    int64_t endTime = GetTimeMillis() + sleepTime;
    int heightTmp{-1};
    int heightStart{-1};
    {
        LOCK(cs);
        heightTmp = heightStart = currentHeight;
    }

    LogPrint(BCLog::LLMQ_DKG, "CDKGSessionManager::%s -- %s - starting sleep for %d ms, curPhase=%d\n", __func__, params.name, sleepTime, curPhase);

    while (GetTimeMillis() < endTime) {
        if (stopRequested) {
            LogPrint(BCLog::LLMQ_DKG, "CDKGSessionManager::%s -- %s - aborting due to stop/shutdown requested\n", __func__, params.name);
            throw AbortPhaseException();
        }
        {
            LOCK(cs);
            if (currentHeight > heightTmp) {
                // New block(s) just came in
                int64_t expectedBlockTime = (currentHeight - heightStart) * Params().GetConsensus().nPowTargetSpacing * 1000;
                if (expectedBlockTime > sleepTime) {
                    // Blocks came faster than we expected, jump into the phase func asap
                    break;
                }
                heightTmp = currentHeight;
            }
            if (phase != curPhase || quorumHash != expectedQuorumHash) {
                // Something went wrong and/or we missed quite a few blocks and it's just too late now
                LogPrint(BCLog::LLMQ_DKG, "CDKGSessionManager::%s -- %s - aborting due unexpected phase/expectedQuorumHash change\n", __func__, params.name);
                throw AbortPhaseException();
            }
        }
        if (!runWhileWaiting()) {
            MilliSleep(100);
        }
    }

    LogPrint(BCLog::LLMQ_DKG, "CDKGSessionManager::%s -- %s - done, curPhase=%d\n", __func__, params.name, curPhase);
}

void CDKGSessionHandler::HandlePhase(QuorumPhase curPhase,
                                     QuorumPhase nextPhase,
                                     const uint256& expectedQuorumHash,
                                     double randomSleepFactor,
                                     const StartPhaseFunc& startPhaseFunc,
                                     const WhileWaitFunc& runWhileWaiting)
{
    LogPrint(BCLog::LLMQ_DKG, "CDKGSessionManager::%s -- %s - starting, curPhase=%d, nextPhase=%d\n", __func__, params.name, curPhase, nextPhase);

    SleepBeforePhase(curPhase, expectedQuorumHash, randomSleepFactor, runWhileWaiting);
    startPhaseFunc();
    WaitForNextPhase(curPhase, nextPhase, expectedQuorumHash, runWhileWaiting);

    LogPrint(BCLog::LLMQ_DKG, "CDKGSessionManager::%s -- %s - done, curPhase=%d, nextPhase=%d\n", __func__, params.name, curPhase, nextPhase);
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
    for (const auto& p : messages ) {
        const auto& msg = *p.second;

        auto member = session.GetMember(msg.proTxHash);
        if (!member) {
            // should not happen as it was verified before
            ret.emplace(p.first);
            continue;
        }

        if (first) {
            aggSig = msg.sig;
        } else {
            aggSig.AggregateInsecure(msg.sig);
        }
        first = false;

        auto msgHash = msg.GetSignHash();
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
        bool valid = aggSig.VerifyInsecureAggregated(pubKeys, messageHashes);
        if (valid) {
            // all good
            return ret;
        }

        // are all messages from the same node?
        NodeId firstNodeId;
        first = true;
        bool nodeIdsAllSame = true;
        for (auto it = messages.begin(); it != messages.end(); ++it) {
            if (first) {
                firstNodeId = it->first;
            } else {
                first = false;
                if (it->first != firstNodeId) {
                    nodeIdsAllSame = false;
                    break;
                }
            }
        }
        // if yes, take a short path and return a set with only him
        if (nodeIdsAllSame) {
            ret.emplace(firstNodeId);
            return ret;
        }
        // different nodes, let's figure out who are the bad ones
    }

    for (const auto& p : messages) {
        if (ret.count(p.first)) {
            continue;
        }

        const auto& msg = *p.second;
        auto member = session.GetMember(msg.proTxHash);
        bool valid = msg.sig.VerifyInsecure(member->dmn->pdmnState->pubKeyOperator.Get(), msg.GetSignHash());
        if (!valid) {
            ret.emplace(p.first);
        }
    }
    return ret;
}

template<typename Message, int MessageType>
bool ProcessPendingMessageBatch(CDKGSession& session, CDKGPendingMessages& pendingMessages, size_t maxCount)
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
                LOCK(cs_main);
                Misbehaving(nodeId, 100);
            }
            continue;
        }
        bool ban = false;
        if (!session.PreVerifyMessage(*p.second, ban)) {
            if (ban) {
                LogPrint(BCLog::LLMQ_DKG, "%s -- banning node due to failed preverification, peer=%d\n", __func__, nodeId);
                {
                    LOCK(cs_main);
                    Misbehaving(nodeId, 100);
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
        LOCK(cs_main);
        for (auto nodeId : badNodes) {
            LogPrint(BCLog::LLMQ_DKG, "%s -- failed to verify signature, peer=%d\n", __func__, nodeId);
            Misbehaving(nodeId, 100);
        }
    }

    for (const auto& p : preverifiedMessages) {
        const NodeId &nodeId = p.first;
        if (badNodes.count(nodeId)) {
            continue;
        }
        bool ban = false;
        session.ReceiveMessage(*p.second, ban);
        if (ban) {
            LogPrint(BCLog::LLMQ_DKG, "%s -- banning node after ReceiveMessage failed, peer=%d\n", __func__, nodeId);
            LOCK(cs_main);
            Misbehaving(nodeId, 100);
            badNodes.emplace(nodeId);
        }
    }

    return true;
}

void CDKGSessionHandler::HandleDKGRound()
{
    uint256 curQuorumHash;

    WaitForNextPhase(QuorumPhase_None, QuorumPhase_Initialized, uint256(), []{return false;});

    {
        LOCK(cs);
        pendingContributions.Clear();
        pendingComplaints.Clear();
        pendingJustifications.Clear();
        pendingPrematureCommitments.Clear();
        curQuorumHash = quorumHash;
    }

    const CBlockIndex* pindexQuorum;
    {
        LOCK(cs_main);
        pindexQuorum = LookupBlockIndex(curQuorumHash);
    }

    if (!InitNewQuorum(pindexQuorum)) {
        // should actually never happen
        WaitForNewQuorum(curQuorumHash);
        throw AbortPhaseException();
    }

    quorumDKGDebugManager->UpdateLocalSessionStatus(params.type, [&](CDKGDebugSessionStatus& status) {
        bool changed = status.phase != (uint8_t) QuorumPhase_Initialized;
        status.phase = (uint8_t) QuorumPhase_Initialized;
        return changed;
    });

    CLLMQUtils::EnsureQuorumConnections(params.type, pindexQuorum, curSession->myProTxHash);
    if (curSession->AreWeMember()) {
        CLLMQUtils::AddQuorumProbeConnections(params.type, pindexQuorum, curSession->myProTxHash);
    }

    WaitForNextPhase(QuorumPhase_Initialized, QuorumPhase_Contribute, curQuorumHash, []{return false;});

    // Contribute
    auto fContributeStart = [this]() {
        curSession->Contribute(pendingContributions);
    };
    auto fContributeWait = [this] {
        return ProcessPendingMessageBatch<CDKGContribution, MSG_QUORUM_CONTRIB>(*curSession, pendingContributions, 8);
    };
    HandlePhase(QuorumPhase_Contribute, QuorumPhase_Complain, curQuorumHash, 0.05, fContributeStart, fContributeWait);

    // Complain
    auto fComplainStart = [this]() {
        curSession->VerifyAndComplain(pendingComplaints);
    };
    auto fComplainWait = [this] {
        return ProcessPendingMessageBatch<CDKGComplaint, MSG_QUORUM_COMPLAINT>(*curSession, pendingComplaints, 8);
    };
    HandlePhase(QuorumPhase_Complain, QuorumPhase_Justify, curQuorumHash, 0.05, fComplainStart, fComplainWait);

    // Justify
    auto fJustifyStart = [this]() {
        curSession->VerifyAndJustify(pendingJustifications);
    };
    auto fJustifyWait = [this] {
        return ProcessPendingMessageBatch<CDKGJustification, MSG_QUORUM_JUSTIFICATION>(*curSession, pendingJustifications, 8);
    };
    HandlePhase(QuorumPhase_Justify, QuorumPhase_Commit, curQuorumHash, 0.05, fJustifyStart, fJustifyWait);

    // Commit
    auto fCommitStart = [this]() {
        curSession->VerifyAndCommit(pendingPrematureCommitments);
    };
    auto fCommitWait = [this] {
        return ProcessPendingMessageBatch<CDKGPrematureCommitment, MSG_QUORUM_PREMATURE_COMMITMENT>(*curSession, pendingPrematureCommitments, 8);
    };
    HandlePhase(QuorumPhase_Commit, QuorumPhase_Finalize, curQuorumHash, 0.1, fCommitStart, fCommitWait);

    auto finalCommitments = curSession->FinalizeCommitments();
    for (const auto& fqc : finalCommitments) {
        quorumBlockProcessor->AddMineableCommitment(fqc);
    }
}

void CDKGSessionHandler::PhaseHandlerThread()
{
    while (!stopRequested) {
        try {
            LogPrint(BCLog::LLMQ_DKG, "CDKGSessionHandler::%s -- %s - starting HandleDKGRound\n", __func__, params.name);
            HandleDKGRound();
        } catch (AbortPhaseException& e) {
            quorumDKGDebugManager->UpdateLocalSessionStatus(params.type, [&](CDKGDebugSessionStatus& status) {
                status.aborted = true;
                return true;
            });
            LogPrint(BCLog::LLMQ_DKG, "CDKGSessionHandler::%s -- %s - aborted current DKG session\n", __func__, params.name);
        }
    }
}

} // namespace llmq
