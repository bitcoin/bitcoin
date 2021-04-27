// Copyright (c) 2018-2021 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_LLMQ_QUORUMS_DKGSESSIONHANDLER_H
#define BITCOIN_LLMQ_QUORUMS_DKGSESSIONHANDLER_H


#include <ctpl.h>
#include <net.h>

class CBLSWorker;
class CBlockIndex;

namespace llmq
{

class CDKGSession;
class CDKGSessionManager;

enum QuorumPhase {
    QuorumPhase_None = -1,
    QuorumPhase_Initialized = 1,
    QuorumPhase_Contribute,
    QuorumPhase_Complain,
    QuorumPhase_Justify,
    QuorumPhase_Commit,
    QuorumPhase_Finalize,
    QuorumPhase_Idle,
};

/**
 * Acts as a FIFO queue for incoming DKG messages. The reason we need this is that deserialization of these messages
 * is too slow to be processed in the main message handler thread. So, instead of processing them directly from the
 * main handler thread, we push them into a CDKGPendingMessages object and later pop+deserialize them in the DKG phase
 * handler thread.
 *
 * Each message type has it's own instance of this class.
 */
class CDKGPendingMessages
{
public:
    typedef std::pair<NodeId, std::shared_ptr<CDataStream>> BinaryMessage;

private:
    mutable CCriticalSection cs;
    const int invType;
    size_t maxMessagesPerNode GUARDED_BY(cs);
    std::list<BinaryMessage> pendingMessages GUARDED_BY(cs);
    std::map<NodeId, size_t> messagesPerNode GUARDED_BY(cs);
    std::set<uint256> seenMessages GUARDED_BY(cs);

public:
    explicit CDKGPendingMessages(size_t _maxMessagesPerNode, int _invType);

    void PushPendingMessage(NodeId from, CDataStream& vRecv);
    std::list<BinaryMessage> PopPendingMessages(size_t maxCount);
    bool HasSeen(const uint256& hash) const;
    void Clear();

    template<typename Message>
    void PushPendingMessage(NodeId from, Message& msg)
    {
        CDataStream ds(SER_NETWORK, PROTOCOL_VERSION);
        ds << msg;
        PushPendingMessage(from, ds);
    }

    // Might return nullptr messages, which indicates that deserialization failed for some reason
    template<typename Message>
    std::vector<std::pair<NodeId, std::shared_ptr<Message>>> PopAndDeserializeMessages(size_t maxCount)
    {
        auto binaryMessages = PopPendingMessages(maxCount);
        if (binaryMessages.empty()) {
            return {};
        }

        std::vector<std::pair<NodeId, std::shared_ptr<Message>>> ret;
        ret.reserve(binaryMessages.size());
        for (const auto& bm : binaryMessages) {
            auto msg = std::make_shared<Message>();
            try {
                *bm.second >> *msg;
            } catch (...) {
                msg = nullptr;
            }
            ret.emplace_back(std::make_pair(bm.first, std::move(msg)));
        }

        return std::move(ret);
    }
};

/**
 * Handles multiple sequential sessions of one specific LLMQ type. There is one instance of this class per LLMQ type.
 *
 * It internally starts the phase handler thread, which constantly loops and sequentially processes one session at a
 * time and waiting for the next phase if necessary.
 */
class CDKGSessionHandler
{
private:
    friend class CDKGSessionManager;

private:
    mutable CCriticalSection cs;
    std::atomic<bool> stopRequested{false};

    const Consensus::LLMQParams& params;
    CBLSWorker& blsWorker;
    CDKGSessionManager& dkgManager;

    QuorumPhase phase GUARDED_BY(cs) {QuorumPhase_Idle};
    int currentHeight GUARDED_BY(cs) {-1};
    int quorumHeight GUARDED_BY(cs) {-1};
    uint256 quorumHash GUARDED_BY(cs);

    std::shared_ptr<CDKGSession> curSession;
    std::thread phaseHandlerThread;

    CDKGPendingMessages pendingContributions GUARDED_BY(cs);
    CDKGPendingMessages pendingComplaints GUARDED_BY(cs);
    CDKGPendingMessages pendingJustifications GUARDED_BY(cs);
    CDKGPendingMessages pendingPrematureCommitments GUARDED_BY(cs);

public:
    CDKGSessionHandler(const Consensus::LLMQParams& _params, CBLSWorker& blsWorker, CDKGSessionManager& _dkgManager);
    ~CDKGSessionHandler();

    void UpdatedBlockTip(const CBlockIndex *pindexNew);
    void ProcessMessage(CNode* pfrom, const std::string& strCommand, CDataStream& vRecv);

    void StartThread();
    void StopThread();

private:
    bool InitNewQuorum(const CBlockIndex* pindexQuorum);

    std::pair<QuorumPhase, uint256> GetPhaseAndQuorumHash() const;

    typedef std::function<void()> StartPhaseFunc;
    typedef std::function<bool()> WhileWaitFunc;
    void WaitForNextPhase(QuorumPhase curPhase, QuorumPhase nextPhase, const uint256& expectedQuorumHash, const WhileWaitFunc& runWhileWaiting);
    void WaitForNewQuorum(const uint256& oldQuorumHash);
    void SleepBeforePhase(QuorumPhase curPhase, const uint256& expectedQuorumHash, double randomSleepFactor, const WhileWaitFunc& runWhileWaiting);
    void HandlePhase(QuorumPhase curPhase, QuorumPhase nextPhase, const uint256& expectedQuorumHash, double randomSleepFactor, const StartPhaseFunc& startPhaseFunc, const WhileWaitFunc& runWhileWaiting);
    void HandleDKGRound();
    void PhaseHandlerThread();
};

} // namespace llmq

#endif // BITCOIN_LLMQ_QUORUMS_DKGSESSIONHANDLER_H
