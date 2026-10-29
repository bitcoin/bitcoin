// Copyright (c) 2018-2025 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_LLMQ_DKGSESSIONHANDLER_H
#define BITCOIN_LLMQ_DKGSESSIONHANDLER_H

#include <msg_result.h>

#include <net.h> // for NodeId
#include <net_processing.h>
#include <protocol.h>
#include <serialize.h>
#include <streams.h>
#include <sync.h>
#include <uint256.h>

#include <list>
#include <map>
#include <memory>
#include <set>
#include <string_view>
#include <vector>

class CBlockIndex;
class CConnman;
class PeerManager;

namespace Consensus {
struct LLMQParams;
} // namespace Consensus

namespace llmq
{
class CDKGContribution;
class CDKGComplaint;
class CDKGJustification;
class CDKGPrematureCommitment;
class CDKGSessionManager;

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
    const size_t maxMessagesPerProTx;
    mutable Mutex cs_messages;
    std::list<BinaryMessage> pendingMessages GUARDED_BY(cs_messages);
    // Keyed by proTxHash rather than NodeId so the quota survives reconnects,
    // and cumulative for the round (not refunded on pop) so draining the queue
    // does not regain retention slots. MNAuth pins keys to registered MNs.
    std::map<uint256, size_t> messagesPerProTx GUARDED_BY(cs_messages);
    std::set<uint256> seenMessages GUARDED_BY(cs_messages);

public:
    explicit CDKGPendingMessages(size_t _maxMessagesPerProTx, uint32_t _invType) :
            invType(_invType), maxMessagesPerProTx(_maxMessagesPerProTx) {};

    /**
     * Enqueue a serialized DKG message received from @p from. @p sender_protx keys the per-proTx
     * quota: the sender's MNAuth-verified proTxHash for remote messages, or this node's own for
     * messages it produced itself (@p from == -1). Drops the message silently on quota overflow or
     * duplicate hash; quota-dropped messages are not marked seen, so another peer with budget can
     * re-deliver them.
     */
    [[nodiscard]] MessageProcessingResult PushPendingMessage(NodeId from, const uint256& sender_protx, CDataStream& vRecv)
        EXCLUSIVE_LOCKS_REQUIRED(!cs_messages);
    std::list<BinaryMessage> PopPendingMessages(size_t maxCount) EXCLUSIVE_LOCKS_REQUIRED(!cs_messages);
    bool HasSeen(const uint256& hash) const EXCLUSIVE_LOCKS_REQUIRED(!cs_messages);
    void Misbehaving(NodeId from, int score, PeerManager& peerman);
    void Clear() EXCLUSIVE_LOCKS_REQUIRED(!cs_messages);

    template <typename Message>
    void PushPendingMessage(NodeId from, const uint256& sender_protx, Message& msg, PeerManager& peerman)
        EXCLUSIVE_LOCKS_REQUIRED(!cs_messages)
    {
        CDataStream ds(SER_NETWORK, PROTOCOL_VERSION);
        ds << msg;
        peerman.PostProcessMessage(PushPendingMessage(from, sender_protx, ds), from);
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

protected:
    const Consensus::LLMQParams& params;

    // Do not guard these, they protect their internals themselves
    CDKGPendingMessages pendingContributions;
    CDKGPendingMessages pendingComplaints;
    CDKGPendingMessages pendingJustifications;
    CDKGPendingMessages pendingPrematureCommitments;

public:
    explicit CDKGSessionHandler(const Consensus::LLMQParams& _params);
    virtual ~CDKGSessionHandler();

    [[nodiscard]] MessageProcessingResult ProcessMessage(NodeId from, const uint256& sender_protx,
                                                         std::string_view msg_type, CDataStream& vRecv);

public:
    virtual bool GetContribution(const uint256& hash, CDKGContribution& ret) const { return false; }
    virtual bool GetComplaint(const uint256& hash, CDKGComplaint& ret) const { return false; }
    virtual bool GetJustification(const uint256& hash, CDKGJustification& ret) const { return false; }
    virtual bool GetPrematureCommitment(const uint256& hash, CDKGPrematureCommitment& ret) const { return false; }
    virtual QuorumPhase GetPhase() const { return QuorumPhase::Idle; }
    virtual void StartThread(CConnman& connman, PeerManager& peerman) {}
    virtual void StopThread() {}
    virtual void UpdatedBlockTip(const CBlockIndex* pindexNew) {}
};
} // namespace llmq

#endif // BITCOIN_LLMQ_DKGSESSIONHANDLER_H
