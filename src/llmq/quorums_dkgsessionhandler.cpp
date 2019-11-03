// Copyright (c) 2018-2019 The Dash Core developers
// Copyright (c) 2019 The BitGreen Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <llmq/quorums_dkgsessionhandler.h>
#include <llmq/quorums_blockprocessor.h>
#include <llmq/quorums_debug.h>
#include <llmq/quorums_init.h>
#include <llmq/quorums_utils.h>

#include <masternodes/activemasternode.h>
#include <chainparams.h>
#include <init.h>
#include <net_processing.h>
#include <shutdown.h>
#include <validation.h>

namespace llmq
{

CDKGPendingMessages::CDKGPendingMessages(size_t _maxMessagesPerNode) :
    maxMessagesPerNode(_maxMessagesPerNode)
{
}

void CDKGPendingMessages::PushPendingMessage(NodeId from, const int nMsgType, CDataStream& vRecv)
{
    // this will also consume the data, even if we bail out early
    auto pm = std::make_shared<CDataStream>(std::move(vRecv));

    {
        LOCK(cs);

        if (messagesPerNode[from] >= maxMessagesPerNode) {
            // TODO ban?
            LogPrintf("CDKGPendingMessages::%s -- too many messages, peer=%d\n", __func__, from);
            return;
        }
        messagesPerNode[from]++;
    }

    CHashWriter hw(SER_GETHASH, 0);
    hw.write(pm->data(), pm->size());
    uint256 hash = hw.GetHash();

    LOCK2(cs_main, cs);

    if (!seenMessages.emplace(hash).second) {
        LogPrint(BCLog::LLMQDKG, "CDKGPendingMessages::%s -- already seen %s, peer=%d", __func__, from);
        return;
    }

    if (from > -1 && nMsgType > -1) {
        RemoveDataRequest(-1, CInv(nMsgType, hash));
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

CDKGSessionHandler::CDKGSessionHandler(const Consensus::LLMQParams& _params, ctpl::thread_pool& _messageHandlerPool, CBLSWorker& _blsWorker, CDKGSessionManager& _dkgManager) :
    params(_params),
    messageHandlerPool(_messageHandlerPool),
    blsWorker(_blsWorker),
    dkgManager(_dkgManager),
    curSession(std::make_shared<CDKGSession>(_params, _blsWorker, _dkgManager)),
    pendingContributions((size_t)_params.size * 2), // we allow size*2 messages as we need to make sure we see bad behavior (double messages)
    pendingComplaints((size_t)_params.size * 2),
    pendingJustifications((size_t)_params.size * 2),
    pendingPrematureCommitments((size_t)_params.size * 2)
{
    phaseHandlerThread = std::thread([this] {
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

void CDKGSessionHandler::UpdatedBlockTip(const CBlockIndex* pindexNew)
{
    LOCK(cs);

    int quorumStageInt = pindexNew->nHeight % params.dkgInterval;
    const CBlockIndex* pindexQuorum = pindexNew->GetAncestor(pindexNew->nHeight - quorumStageInt);

    quorumHeight = pindexQuorum->nHeight;
    quorumHash = pindexQuorum->GetBlockHash();

    bool fNewPhase = (quorumStageInt % params.dkgPhaseBlocks) == 0;
    int phaseInt = quorumStageInt / params.dkgPhaseBlocks + 1;
    if (fNewPhase && phaseInt >= QuorumPhase_Initialized && phaseInt <= QuorumPhase_Idle) {
        phase = static_cast<QuorumPhase>(phaseInt);
    }
}

void CDKGSessionHandler::ProcessMessage(CNode* pfrom, const std::string& strCommand, CDataStream& vRecv, CConnman* connman)
{
    // We don't handle messages in the calling thread as deserialization/processing of these would block everything
    if (strCommand == NetMsgType::QCONTRIB) {
        pendingContributions.PushPendingMessage(pfrom->GetId(), MSG_QUORUM_CONTRIB, vRecv);
    } else if (strCommand == NetMsgType::QCOMPLAINT) {
        pendingComplaints.PushPendingMessage(pfrom->GetId(), MSG_QUORUM_COMPLAINT, vRecv);
    } else if (strCommand == NetMsgType::QJUSTIFICATION) {
        pendingJustifications.PushPendingMessage(pfrom->GetId(), MSG_QUORUM_JUSTIFICATION, vRecv);
    } else if (strCommand == NetMsgType::QPCOMMITMENT) {
        pendingPrematureCommitments.PushPendingMessage(pfrom->GetId(), MSG_QUORUM_PREMATURE_COMMITMENT, vRecv);
    }
}

bool CDKGSessionHandler::InitNewQuorum(const CBlockIndex* pindexQuorum)
{
    //AssertLockHeld(cs_main);

    const auto& consensus = Params().GetConsensus();

    if (pindexQuorum->nHeight < consensus.nLLMQActivationHeight)
        return false;

    curSession = std::make_shared<CDKGSession>(params, blsWorker, dkgManager);

    auto mns = CLLMQUtils::GetAllQuorumMembers(params.type, pindexQuorum);

    if (!curSession->Init(pindexQuorum, mns, activeMasternodeInfo.proTxHash)) {
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
            break;
        }
        if (curPhase != QuorumPhase_None && p.first != curPhase) {
            throw AbortPhaseException();
        }
        if (!runWhileWaiting()) {
            MilliSleep(100);
        }
    }

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

template<typename Message>
bool ProcessPendingMessageBatch(CDKGSession& session, CDKGPendingMessages& pendingMessages, int nMsgType, size_t maxCount)
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
            LogPrintf("%s -- failed to deserialize message, peer=%d\n", __func__, p.first);
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
            RemoveDataRequest(-1, CInv(nMsgType, hash));
        }

        bool ban = false;
        if (!session.PreVerifyMessage(hash, msg, ban)) {
            if (ban) {
                LogPrintf("%s -- banning node due to failed preverification, peer=%d\n", __func__, p.first);
                {
                    LOCK(cs_main);
                    Misbehaving(p.first, 100);
                }
            }
            LogPrintf("%s -- skipping message due to failed preverification, peer=%d\n", __func__, p.first);
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
            LogPrintf("%s -- failed to verify signature, peer=%d\n", __func__, nodeId);
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
            LogPrintf("%s -- banning node after ReceiveMessage failed, peer=%d\n", __func__, nodeId);
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

    const CBlockIndex* pindexQuorum;
    {
        LOCK(cs_main);
        pindexQuorum = ::BlockIndex().at(curQuorumHash);
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

    if (curSession->AreWeMember() || gArgs.GetBoolArg("-watchquorums", DEFAULT_WATCH_QUORUMS)) {
        std::set<uint256> connections;
        if (curSession->AreWeMember()) {
            connections = CLLMQUtils::GetQuorumConnections(params.type, pindexQuorum, curSession->myProTxHash);
        } else {
            auto cindexes = CLLMQUtils::CalcDeterministicWatchConnections(params.type, pindexQuorum, curSession->members.size(), 1);
            for (auto idx : cindexes) {
                connections.emplace(curSession->members[idx]->dmn->proTxHash);
            }
        }
        if (!connections.empty()) {
            if (LogAcceptCategory(BCLog::LLMQDKG)) {
                std::string debugMsg = strprintf("CDKGSessionManager::%s -- adding masternodes quorum connections for quorum %s:\n", __func__, curSession->pindexQuorum->GetBlockHash().ToString());
                auto mnList = deterministicMNManager->GetListAtChainTip();
                for (const auto& c : connections) {
                    auto dmn = mnList.GetValidMN(c);
                    if (!dmn) {
                        debugMsg += strprintf("  %s (not in valid MN set anymore)\n", c.ToString());
                    } else {
                        debugMsg += strprintf("  %s (%s)\n", c.ToString(), dmn->pdmnState->addr.ToString());
                    }
                }
                LogPrint(BCLog::LLMQDKG, debugMsg.c_str());
            }
            g_connman->AddMasternodeQuorumNodes(params.type, curQuorumHash, connections);
        }
    }

    WaitForNextPhase(QuorumPhase_Initialized, QuorumPhase_Contribute, curQuorumHash, []{return false;});

    // Contribute
    auto fContributeStart = [this]() {
        curSession->Contribute(pendingContributions);
    };
    auto fContributeWait = [this] {
        return ProcessPendingMessageBatch<CDKGContribution>(*curSession, pendingContributions, MSG_QUORUM_CONTRIB, 8);
    };
    HandlePhase(QuorumPhase_Contribute, QuorumPhase_Complain, curQuorumHash, 0.05, fContributeStart, fContributeWait);

    // Complain
    auto fComplainStart = [this]() {
        curSession->VerifyAndComplain(pendingComplaints);
    };
    auto fComplainWait = [this] {
        return ProcessPendingMessageBatch<CDKGComplaint>(*curSession, pendingComplaints, MSG_QUORUM_COMPLAINT, 8);
    };
    HandlePhase(QuorumPhase_Complain, QuorumPhase_Justify, curQuorumHash, 0.05, fComplainStart, fComplainWait);

    // Justify
    auto fJustifyStart = [this]() {
        curSession->VerifyAndJustify(pendingJustifications);
    };
    auto fJustifyWait = [this] {
        return ProcessPendingMessageBatch<CDKGJustification>(*curSession, pendingJustifications, MSG_QUORUM_JUSTIFICATION, 8);
    };
    HandlePhase(QuorumPhase_Justify, QuorumPhase_Commit, curQuorumHash, 0.05, fJustifyStart, fJustifyWait);

    // Commit
    auto fCommitStart = [this]() {
        curSession->VerifyAndCommit(pendingPrematureCommitments);
    };
    auto fCommitWait = [this] {
        return ProcessPendingMessageBatch<CDKGPrematureCommitment>(*curSession, pendingPrematureCommitments, MSG_QUORUM_PREMATURE_COMMITMENT, 8);
    };
    HandlePhase(QuorumPhase_Commit, QuorumPhase_Finalize, curQuorumHash, 0.1, fCommitStart, fCommitWait);

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
            LogPrint(BCLog::LLMQDKG, "CDKGSessionHandler::%s -- aborted current DKG session for llmq=%s\n", __func__, params.name);
        }
    }
}

}
