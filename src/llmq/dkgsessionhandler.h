// Copyright (c) 2018-2022 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_LLMQ_DKGSESSIONHANDLER_H
#define BITCOIN_LLMQ_DKGSESSIONHANDLER_H


#include <ctpl_stl.h>
#include <net.h>

#include <optional>

class CBLSWorker;
class CBlockIndex;
class PeerManager;

namespace llmq
{
class CDKGDebugManager;
class CDKGSession;
class CDKGSessionManager;
class CQuorumBlockProcessor;

enum class QuorumPhase {
    Initialized = 1,
    Contribute,
    Complain,
    Justify,
    Commit,
    Finalize,
    Idle,
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
    using BinaryMessage = std::pair<NodeId, std::shared_ptr<CDataStream>>;

private:
    mutable RecursiveMutex cs;
    const int invType;
    size_t maxMessagesPerNode GUARDED_BY(cs);
    std::list<BinaryMessage> pendingMessages GUARDED_BY(cs);
    std::map<NodeId, size_t> messagesPerNode GUARDED_BY(cs);
    std::set<uint256> seenMessages GUARDED_BY(cs);

public:
    explicit CDKGPendingMessages(size_t _maxMessagesPerNode, int _invType) :
            invType(_invType), maxMessagesPerNode(_maxMessagesPerNode) {};

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

        return ret;
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
    mutable RecursiveMutex cs;
    std::atomic<bool> stopRequested{false};

    const Consensus::LLMQParams params;
    CConnman& connman;
    const int quorumIndex;
    CBLSWorker& blsWorker;
    CDKGSessionManager& dkgManager;
    CDKGDebugManager& dkgDebugManager;
    CQuorumBlockProcessor& quorumBlockProcessor;

    const std::unique_ptr<PeerManager>& m_peerman;

    QuorumPhase phase GUARDED_BY(cs) {QuorumPhase::Idle};
    int currentHeight GUARDED_BY(cs) {-1};
    uint256 quorumHash GUARDED_BY(cs);

    std::unique_ptr<CDKGSession> curSession;
    std::thread phaseHandlerThread;

    // Do not guard these, they protect their internals themselves
    CDKGPendingMessages pendingContributions;
    CDKGPendingMessages pendingComplaints;
    CDKGPendingMessages pendingJustifications;
    CDKGPendingMessages pendingPrematureCommitments;

public:
    CDKGSessionHandler(const Consensus::LLMQParams& _params, CBLSWorker& _blsWorker, CDKGSessionManager& _dkgManager,
                       CDKGDebugManager& _dkgDebugManager, CQuorumBlockProcessor& _quorumBlockProcessor,
                       CConnman& _connman, const std::unique_ptr<PeerManager>& peerman, int _quorumIndex);
    ~CDKGSessionHandler() = default;

    void UpdatedBlockTip(const CBlockIndex *pindexNew);
    void ProcessMessage(const CNode& pfrom, const std::string& msg_type, CDataStream& vRecv);

    void StartThread();
    void StopThread();

private:
    bool InitNewQuorum(const CBlockIndex* pQuorumBaseBlockIndex);

    std::pair<QuorumPhase, uint256> GetPhaseAndQuorumHash() const;

    using StartPhaseFunc = std::function<void()>;
    using WhileWaitFunc = std::function<bool()>;
    /**
     * @param curPhase current QuorumPhase
     * @param nextPhase next QuorumPhase
     * @param expectedQuorumHash expected QuorumHash, defaults to null
     * @param shouldNotWait function that returns bool, defaults to function that returns false. If the function returns false, we will wait in the loop, if true, we don't wait
     */
    void WaitForNextPhase(std::optional<QuorumPhase> curPhase, QuorumPhase nextPhase, const uint256& expectedQuorumHash=uint256(), const WhileWaitFunc& shouldNotWait=[]{return false;}) const;
    void WaitForNewQuorum(const uint256& oldQuorumHash) const;
    void SleepBeforePhase(QuorumPhase curPhase, const uint256& expectedQuorumHash, double randomSleepFactor, const WhileWaitFunc& runWhileWaiting) const;
    void HandlePhase(QuorumPhase curPhase, QuorumPhase nextPhase, const uint256& expectedQuorumHash, double randomSleepFactor, const StartPhaseFunc& startPhaseFunc, const WhileWaitFunc& runWhileWaiting);
    void HandleDKGRound();
    void PhaseHandlerThread();
};

} // namespace llmq

#endif // BITCOIN_LLMQ_DKGSESSIONHANDLER_H
