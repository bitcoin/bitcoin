// Copyright (c) 2018-2025 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_LLMQ_DKGSESSIONHANDLER_H
#define BITCOIN_LLMQ_DKGSESSIONHANDLER_H

#include <net.h> // for NodeId
#include <net_processing.h>

#include <atomic>
#include <list>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

class CActiveMasternodeManager;
class CBLSWorker;
class CBlockIndex;
class CChainState;
class CConnman;
class CDeterministicMNManager;
class CMasternodeMetaMan;
class CSporkManager;
class PeerManager;

namespace llmq
{
class CDKGContribution;
class CDKGComplaint;
class CDKGJustification;
class CDKGPrematureCommitment;
class CDKGDebugManager;
class CDKGSession;
class CDKGSessionManager;
class CQuorumBlockProcessor;
class CQuorumSnapshotManager;

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
    const uint32_t invType;
    const size_t maxMessagesPerNode;
    mutable Mutex cs_messages;
    std::list<BinaryMessage> pendingMessages GUARDED_BY(cs_messages);
    std::map<NodeId, size_t> messagesPerNode GUARDED_BY(cs_messages);
    std::set<uint256> seenMessages GUARDED_BY(cs_messages);

public:
    explicit CDKGPendingMessages(size_t _maxMessagesPerNode, uint32_t _invType) :
            invType(_invType), maxMessagesPerNode(_maxMessagesPerNode) {};

    [[nodiscard]] MessageProcessingResult PushPendingMessage(NodeId from, CDataStream& vRecv)
        EXCLUSIVE_LOCKS_REQUIRED(!cs_messages);
    std::list<BinaryMessage> PopPendingMessages(size_t maxCount) EXCLUSIVE_LOCKS_REQUIRED(!cs_messages);
    bool HasSeen(const uint256& hash) const EXCLUSIVE_LOCKS_REQUIRED(!cs_messages);
    void Misbehaving(NodeId from, int score, PeerManager& peerman);
    void Clear() EXCLUSIVE_LOCKS_REQUIRED(!cs_messages);

    template <typename Message>
    void PushPendingMessage(NodeId from, Message& msg, PeerManager& peerman) EXCLUSIVE_LOCKS_REQUIRED(!cs_messages)
    {
        CDataStream ds(SER_NETWORK, PROTOCOL_VERSION);
        ds << msg;
        peerman.PostProcessMessage(PushPendingMessage(from, ds), from);
    }

    // Might return nullptr messages, which indicates that deserialization failed for some reason
    template <typename Message>
    std::vector<std::pair<NodeId, std::shared_ptr<Message>>> PopAndDeserializeMessages(size_t maxCount)
        EXCLUSIVE_LOCKS_REQUIRED(!cs_messages)
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
    std::atomic<bool> stopRequested{false};

    CBLSWorker& blsWorker;
    CChainState& m_chainstate;
    CDeterministicMNManager& m_dmnman;
    CDKGDebugManager& dkgDebugManager;
    CDKGSessionManager& dkgManager;
    CMasternodeMetaMan& m_mn_metaman;
    CQuorumBlockProcessor& quorumBlockProcessor;
    CQuorumSnapshotManager& m_qsnapman;
    const CActiveMasternodeManager* const m_mn_activeman;
    const CSporkManager& m_sporkman;
    const Consensus::LLMQParams params;
    const int quorumIndex;

    std::atomic<int> currentHeight {-1};
    mutable Mutex cs_phase_qhash;
    QuorumPhase phase GUARDED_BY(cs_phase_qhash) {QuorumPhase::Idle};
    uint256 quorumHash GUARDED_BY(cs_phase_qhash);

    std::unique_ptr<CDKGSession> curSession;
    std::thread phaseHandlerThread;
    std::string m_thread_name;

    // Do not guard these, they protect their internals themselves
    CDKGPendingMessages pendingContributions;
    CDKGPendingMessages pendingComplaints;
    CDKGPendingMessages pendingJustifications;
    CDKGPendingMessages pendingPrematureCommitments;

public:
    CDKGSessionHandler(CBLSWorker& _blsWorker, CChainState& chainstate, CDeterministicMNManager& dmnman,
                       CDKGDebugManager& _dkgDebugManager, CDKGSessionManager& _dkgManager,
                       CMasternodeMetaMan& mn_metaman, CQuorumBlockProcessor& _quorumBlockProcessor,
                       CQuorumSnapshotManager& qsnapman, const CActiveMasternodeManager* const mn_activeman,
                       const CSporkManager& sporkman, const Consensus::LLMQParams& _params, int _quorumIndex);
    ~CDKGSessionHandler();

    void UpdatedBlockTip(const CBlockIndex* pindexNew) EXCLUSIVE_LOCKS_REQUIRED(!cs_phase_qhash);
    [[nodiscard]] MessageProcessingResult ProcessMessage(NodeId from, std::string_view msg_type, CDataStream& vRecv);

    void StartThread(CConnman& connman, PeerManager& peerman);
    void StopThread();

    bool GetContribution(const uint256& hash, CDKGContribution& ret) const;
    bool GetComplaint(const uint256& hash, CDKGComplaint& ret) const;
    bool GetJustification(const uint256& hash, CDKGJustification& ret) const;
    bool GetPrematureCommitment(const uint256& hash, CDKGPrematureCommitment& ret) const;

private:
    bool InitNewQuorum(const CBlockIndex* pQuorumBaseBlockIndex);

    std::pair<QuorumPhase, uint256> GetPhaseAndQuorumHash() const EXCLUSIVE_LOCKS_REQUIRED(!cs_phase_qhash);

    using StartPhaseFunc = std::function<void()>;
    using WhileWaitFunc = std::function<bool()>;
    /**
     * @param curPhase current QuorumPhase
     * @param nextPhase next QuorumPhase
     * @param expectedQuorumHash expected QuorumHash, defaults to null
     * @param shouldNotWait function that returns bool, defaults to function that returns false. If the function returns false, we will wait in the loop, if true, we don't wait
     */
    void WaitForNextPhase(
        std::optional<QuorumPhase> curPhase, QuorumPhase nextPhase, const uint256& expectedQuorumHash = uint256(),
        const WhileWaitFunc& shouldNotWait = [] { return false; }) const EXCLUSIVE_LOCKS_REQUIRED(!cs_phase_qhash);
    void WaitForNewQuorum(const uint256& oldQuorumHash) const EXCLUSIVE_LOCKS_REQUIRED(!cs_phase_qhash);
    void SleepBeforePhase(QuorumPhase curPhase, const uint256& expectedQuorumHash, double randomSleepFactor,
                          const WhileWaitFunc& runWhileWaiting) const EXCLUSIVE_LOCKS_REQUIRED(!cs_phase_qhash);
    void HandlePhase(QuorumPhase curPhase, QuorumPhase nextPhase, const uint256& expectedQuorumHash,
                     double randomSleepFactor, const StartPhaseFunc& startPhaseFunc,
                     const WhileWaitFunc& runWhileWaiting) EXCLUSIVE_LOCKS_REQUIRED(!cs_phase_qhash);
    void HandleDKGRound(CConnman& connman, PeerManager& peerman) EXCLUSIVE_LOCKS_REQUIRED(!cs_phase_qhash);
    void PhaseHandlerThread(CConnman& connman, PeerManager& peerman) EXCLUSIVE_LOCKS_REQUIRED(!cs_phase_qhash);
};

} // namespace llmq

#endif // BITCOIN_LLMQ_DKGSESSIONHANDLER_H
