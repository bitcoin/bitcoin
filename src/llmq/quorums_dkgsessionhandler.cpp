// Copyright (c) 2018 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "quorums_dkgsessionhandler.h"
#include "quorums_blockprocessor.h"
#include "quorums_debug.h"
#include "quorums_init.h"
#include "quorums_utils.h"

#include "activemasternode.h"
#include "chainparams.h"
#include "init.h"
#include "net_processing.h"
#include "validation.h"

namespace llmq
{

CDKGPendingMessages::CDKGPendingMessages(size_t _maxMessagesPerNode) :
    maxMessagesPerNode(_maxMessagesPerNode)
{
}

void CDKGPendingMessages::PushPendingMessage(NodeId from, CDataStream& vRecv)
{
    // this will also consume the data, even if we bail out early
    auto pm = std::make_shared<CDataStream>(std::move(vRecv));

    {
        LOCK(cs);

        if (messagesPerNode[from] >= maxMessagesPerNode) {
            // TODO ban?
            LogPrint("net", "CDKGPendingMessages::%s -- too many messages, peer=%d\n", __func__, from);
            return;
        }
        messagesPerNode[from]++;
    }

    CHashWriter hw(SER_GETHASH, 0);
    hw.write(pm->data(), pm->size());
    uint256 hash = hw.GetHash();

    LOCK2(cs_main, cs);

    if (!seenMessages.emplace(hash).second) {
        LogPrint("net", "CDKGPendingMessages::%s -- already seen %s, peer=%d", __func__, from);
        return;
    }

    g_connman->RemoveAskFor(hash);

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

    return std::move(ret);
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

CDKGSessionHandler::CDKGSessionHandler(const Consensus::LLMQParams& _params, CEvoDB& _evoDb, ctpl::thread_pool& _messageHandlerPool, CBLSWorker& _blsWorker, CDKGSessionManager& _dkgManager) :
    params(_params),
    evoDb(_evoDb),
    messageHandlerPool(_messageHandlerPool),
    blsWorker(_blsWorker),
    dkgManager(_dkgManager),
    curSession(std::make_shared<CDKGSession>(_params, _evoDb, _blsWorker, _dkgManager)),
    pendingContributions((size_t)_params.size * 2), // we allow size*2 messages as we need to make sure we see bad behavior (double messages)
    pendingComplaints((size_t)_params.size * 2),
    pendingJustifications((size_t)_params.size * 2),
    pendingPrematureCommitments((size_t)_params.size * 2)
{
    phaseHandlerThread = std::thread([this] {
        RenameThread(strprintf("quorum-phase-%d", (uint8_t)params.type).c_str());
        PhaseHandlerThread();
    });
}

CDKGSessionHandler::~CDKGSessionHandler()
{
    stopRequested = true;
    if (phaseHandlerThread.joinable()) {
        phaseHandlerThread.join();
    }
}

void CDKGSessionHandler::UpdatedBlockTip(const CBlockIndex* pindexNew, const CBlockIndex* pindexFork, bool fInitialDownload)
{
    AssertLockHeld(cs_main);
    LOCK(cs);

    int quorumStageInt = pindexNew->nHeight % params.dkgInterval;
    CBlockIndex* pindexQuorum = chainActive[pindexNew->nHeight - quorumStageInt];

    quorumHeight = pindexQuorum->nHeight;
    quorumHash = pindexQuorum->GetBlockHash();

    bool fNewPhase = (quorumStageInt % params.dkgPhaseBlocks) == 0;
    int phaseInt = quorumStageInt / params.dkgPhaseBlocks;
    if (fNewPhase && phaseInt >= QuorumPhase_Initialized && phaseInt <= QuorumPhase_Idle) {
        phase = static_cast<QuorumPhase>(phaseInt);
    }

    quorumDKGDebugManager->UpdateLocalStatus([&](CDKGDebugStatus& status) {
        bool changed = status.nHeight != pindexNew->nHeight;
        status.nHeight = (uint32_t)pindexNew->nHeight;
        return changed;
    });
    quorumDKGDebugManager->UpdateLocalSessionStatus(params.type, [&](CDKGDebugSessionStatus& status) {
        bool changed = status.phase != (uint8_t)phase;
        status.phase = (uint8_t)phase;
        return changed;
    });
}

void CDKGSessionHandler::ProcessMessage(CNode* pfrom, const std::string& strCommand, CDataStream& vRecv, CConnman& connman)
{
    // We don't handle messages in the calling thread as deserialization/processing of these would block everything
    if (strCommand == NetMsgType::QCONTRIB) {
        pendingContributions.PushPendingMessage(pfrom->id, vRecv);
    } else if (strCommand == NetMsgType::QCOMPLAINT) {
        pendingComplaints.PushPendingMessage(pfrom->id, vRecv);
    } else if (strCommand == NetMsgType::QJUSTIFICATION) {
        pendingJustifications.PushPendingMessage(pfrom->id, vRecv);
    } else if (strCommand == NetMsgType::QPCOMMITMENT) {
        pendingPrematureCommitments.PushPendingMessage(pfrom->id, vRecv);
    }
}

bool CDKGSessionHandler::InitNewQuorum(int newQuorumHeight, const uint256& newQuorumHash)
{
    //AssertLockHeld(cs_main);

    const auto& consensus = Params().GetConsensus();

    curSession = std::make_shared<CDKGSession>(params, evoDb, blsWorker, dkgManager);

    if (!deterministicMNManager->IsDIP3Active(newQuorumHeight)) {
        return false;
    }

    auto mns = CLLMQUtils::GetAllQuorumMembers(params.type, newQuorumHash);

    if (!curSession->Init(newQuorumHeight, newQuorumHash, mns, activeMasternodeInfo.proTxHash)) {
        LogPrintf("CDKGSessionManager::%s -- quorum initialiation failed\n", __func__);
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
                                          const WhileWaitFunc& runWhileWaiting)
{
    while (true) {
        if (stopRequested || ShutdownRequested()) {
            throw AbortPhaseException();
        }
        auto p = GetPhaseAndQuorumHash();
        if (!expectedQuorumHash.IsNull() && p.second != expectedQuorumHash) {
            throw AbortPhaseException();
        }
        if (p.first == nextPhase) {
            return;
        }
        if (curPhase != QuorumPhase_None && p.first != curPhase) {
            throw AbortPhaseException();
        }
        if (!runWhileWaiting()) {
            MilliSleep(100);
        }
    }
}

void CDKGSessionHandler::WaitForNewQuorum(const uint256& oldQuorumHash)
{
    while (true) {
        if (stopRequested || ShutdownRequested()) {
            throw AbortPhaseException();
        }
        auto p = GetPhaseAndQuorumHash();
        if (p.second != oldQuorumHash) {
            return;
        }
        MilliSleep(100);
    }
}

// Sleep some time to not fully overload the whole network
void CDKGSessionHandler::SleepBeforePhase(QuorumPhase curPhase,
                                          const uint256& expectedQuorumHash,
                                          double randomSleepFactor,
                                          const WhileWaitFunc& runWhileWaiting)
{
    // expected time for a full phase
    double phaseTime = params.dkgPhaseBlocks * Params().GetConsensus().nPowTargetSpacing * 1000;
    // expected time per member
    phaseTime = phaseTime / params.size;
    // Don't expect perfect block times and thus reduce the phase time to be on the secure side (caller chooses factor)
    phaseTime *= randomSleepFactor;

    if (Params().MineBlocksOnDemand()) {
        // on regtest, blocks can be mined on demand without any significant time passing between these. We shouldn't
        // wait before phases in this case
        phaseTime = 0;
    }

    int64_t sleepTime = (int64_t)(phaseTime * curSession->GetMyMemberIndex());
    int64_t endTime = GetTimeMillis() + sleepTime;
    while (GetTimeMillis() < endTime) {
        if (stopRequested || ShutdownRequested()) {
            throw AbortPhaseException();
        }
        auto p = GetPhaseAndQuorumHash();
        if (p.first != curPhase || p.second != expectedQuorumHash) {
            throw AbortPhaseException();
        }
        if (!runWhileWaiting()) {
            MilliSleep(100);
        }
    }
}

void CDKGSessionHandler::HandlePhase(QuorumPhase curPhase,
                                     QuorumPhase nextPhase,
                                     const uint256& expectedQuorumHash,
                                     double randomSleepFactor,
                                     const StartPhaseFunc& startPhaseFunc,
                                     const WhileWaitFunc& runWhileWaiting)
{
    SleepBeforePhase(curPhase, expectedQuorumHash, randomSleepFactor, runWhileWaiting);
    startPhaseFunc();
    WaitForNextPhase(curPhase, nextPhase, expectedQuorumHash, runWhileWaiting);
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

        pubKeys.emplace_back(member->dmn->pdmnState->pubKeyOperator);
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
        bool valid = msg.sig.VerifyInsecure(member->dmn->pdmnState->pubKeyOperator, msg.GetSignHash());
        if (!valid) {
            ret.emplace(p.first);
        }
    }
    return ret;
}

template<typename Message>
bool ProcessPendingMessageBatch(CDKGSession& session, CDKGPendingMessages& pendingMessages, size_t maxCount)
{
    auto msgs = pendingMessages.PopAndDeserializeMessages<Message>(maxCount);
    if (msgs.empty()) {
        return false;
    }

    std::vector<uint256> hashes;
    std::vector<std::pair<NodeId, std::shared_ptr<Message>>> preverifiedMessages;
    hashes.reserve(msgs.size());
    preverifiedMessages.reserve(msgs.size());

    for (const auto& p : msgs) {
        if (!p.second) {
            LogPrint("net", "%s -- failed to deserialize message, peer=%d", __func__, p.first);
            {
                LOCK(cs_main);
                Misbehaving(p.first, 100);
            }
            continue;
        }
        const auto& msg = *p.second;

        auto hash = ::SerializeHash(msg);
        {
            LOCK(cs_main);
            g_connman->RemoveAskFor(hash);
        }

        bool ban = false;
        if (!session.PreVerifyMessage(hash, msg, ban)) {
            if (ban) {
                LogPrint("net", "%s -- banning node due to failed preverification, peer=%d", __func__, p.first);
                {
                    LOCK(cs_main);
                    Misbehaving(p.first, 100);
                }
            }
            LogPrint("net", "%s -- skipping message due to failed preverification, peer=%d", __func__, p.first);
            continue;
        }
        hashes.emplace_back(hash);
        preverifiedMessages.emplace_back(p);
    }
    if (preverifiedMessages.empty()) {
        return true;
    }

    auto badNodes = BatchVerifyMessageSigs(session, preverifiedMessages);
    if (!badNodes.empty()) {
        LOCK(cs_main);
        for (auto nodeId : badNodes) {
            LogPrint("net", "%s -- failed to verify signature, peer=%d", __func__, nodeId);
            Misbehaving(nodeId, 100);
        }
    }

    for (size_t i = 0; i < preverifiedMessages.size(); i++) {
        NodeId nodeId = preverifiedMessages[i].first;
        if (badNodes.count(nodeId)) {
            continue;
        }
        const auto& msg = *preverifiedMessages[i].second;
        bool ban = false;
        session.ReceiveMessage(hashes[i], msg, ban);
        if (ban) {
            LogPrint("net", "%s -- banning node after ReceiveMessage failed, peer=%d", __func__, nodeId);
            LOCK(cs_main);
            Misbehaving(nodeId, 100);
            badNodes.emplace(nodeId);
        }
    }

    for (const auto& p : preverifiedMessages) {
        NodeId nodeId = p.first;
        if (badNodes.count(nodeId)) {
            continue;
        }
        session.AddParticipatingNode(nodeId);
    }

    return true;
}

void CDKGSessionHandler::HandleDKGRound()
{
    uint256 curQuorumHash;
    int curQuorumHeight;

    WaitForNextPhase(QuorumPhase_None, QuorumPhase_Initialized, uint256(), []{return false;});

    {
        LOCK(cs);
        pendingContributions.Clear();
        pendingComplaints.Clear();
        pendingJustifications.Clear();
        pendingPrematureCommitments.Clear();
        curQuorumHash = quorumHash;
        curQuorumHeight = quorumHeight;
    }

    if (!InitNewQuorum(curQuorumHeight, curQuorumHash)) {
        // should actually never happen
        WaitForNewQuorum(curQuorumHash);
        throw AbortPhaseException();
    }

    if (curSession->AreWeMember() || GetBoolArg("-watchquorums", DEFAULT_WATCH_QUORUMS)) {
        std::set<CService> connections;
        if (curSession->AreWeMember()) {
            connections = CLLMQUtils::GetQuorumConnections(params.type, curQuorumHash, curSession->myProTxHash);
        } else {
            auto cindexes = CLLMQUtils::CalcDeterministicWatchConnections(params.type, curQuorumHash, curSession->members.size(), 1);
            for (auto idx : cindexes) {
                connections.emplace(curSession->members[idx]->dmn->pdmnState->addr);
            }
        }
        if (!connections.empty()) {
            std::string debugMsg = strprintf("CDKGSessionManager::%s -- adding masternodes quorum connections for quorum %s:\n", __func__, curSession->quorumHash.ToString());
            for (const auto& c : connections) {
                debugMsg += strprintf("  %s\n", c.ToString(false));
            }
            LogPrintf(debugMsg);
            g_connman->AddMasternodeQuorumNodes(params.type, curQuorumHash, connections);

            auto participatingNodesTmp = g_connman->GetMasternodeQuorumAddresses(params.type, curQuorumHash);
            LOCK(curSession->invCs);
            curSession->participatingNodes = std::move(participatingNodesTmp);
        }
    }

    WaitForNextPhase(QuorumPhase_Initialized, QuorumPhase_Contribute, curQuorumHash, []{return false;});

    // Contribute
    auto fContributeStart = [this]() {
        curSession->Contribute(pendingContributions);
    };
    auto fContributeWait = [this] {
        return ProcessPendingMessageBatch<CDKGContribution>(*curSession, pendingContributions, 8);
    };
    HandlePhase(QuorumPhase_Contribute, QuorumPhase_Complain, curQuorumHash, 0.5, fContributeStart, fContributeWait);

    // Complain
    auto fComplainStart = [this]() {
        curSession->VerifyAndComplain(pendingComplaints);
    };
    auto fComplainWait = [this] {
        return ProcessPendingMessageBatch<CDKGComplaint>(*curSession, pendingComplaints, 8);
    };
    HandlePhase(QuorumPhase_Complain, QuorumPhase_Justify, curQuorumHash, 0.1, fComplainStart, fComplainWait);

    // Justify
    auto fJustifyStart = [this]() {
        curSession->VerifyAndJustify(pendingJustifications);
    };
    auto fJustifyWait = [this] {
        return ProcessPendingMessageBatch<CDKGJustification>(*curSession, pendingJustifications, 8);
    };
    HandlePhase(QuorumPhase_Justify, QuorumPhase_Commit, curQuorumHash, 0.1, fJustifyStart, fJustifyWait);

    // Commit
    auto fCommitStart = [this]() {
        curSession->VerifyAndCommit(pendingPrematureCommitments);
    };
    auto fCommitWait = [this] {
        return ProcessPendingMessageBatch<CDKGPrematureCommitment>(*curSession, pendingPrematureCommitments, 8);
    };
    HandlePhase(QuorumPhase_Commit, QuorumPhase_Finalize, curQuorumHash, 0.5, fCommitStart, fCommitWait);

    auto finalCommitments = curSession->FinalizeCommitments();
    for (const auto& fqc : finalCommitments) {
        quorumBlockProcessor->AddMinableCommitment(fqc);
    }
}

void CDKGSessionHandler::PhaseHandlerThread()
{
    while (!stopRequested && !ShutdownRequested()) {
        try {
            HandleDKGRound();
        } catch (AbortPhaseException& e) {
            quorumDKGDebugManager->UpdateLocalSessionStatus(params.type, [&](CDKGDebugSessionStatus& status) {
                status.aborted = true;
                return true;
            });
            LogPrintf("CDKGSessionHandler::%s -- aborted current DKG session\n", __func__);
        }
    }
}

}
