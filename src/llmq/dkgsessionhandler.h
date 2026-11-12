// Copyright (c) 2018-2025 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_LLMQ_DKGSESSIONHANDLER_H
#define BITCOIN_LLMQ_DKGSESSIONHANDLER_H

#include <net.h> // for NodeId
#include <sync.h>

#include <list>
#include <memory>
#include <optional>
#include <string_view>
#include <vector>

class CDataStream;
class CBlockIndex;
class uint256;

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
    const size_t maxMessagesPerProTx;
    mutable Mutex cs_messages;
    std::list<BinaryMessage> pendingMessages GUARDED_BY(cs_messages);
    // Keyed by proTxHash rather than NodeId so the quota survives reconnects,
    // and cumulative for the round (not refunded on pop) so draining the queue
    // does not regain retention slots. MNAuth pins keys to registered MNs.
    Uint256HashMap<size_t> messagesPerProTx GUARDED_BY(cs_messages);
    Uint256HashSet seenMessages GUARDED_BY(cs_messages);

public:
    explicit CDKGPendingMessages(size_t _maxMessagesPerProTx) :
        maxMessagesPerProTx(_maxMessagesPerProTx) {};

    /**
     * Enqueue a serialized DKG message under @p from with content hash @p hash.
     * @p sender_protx keys the per-proTx quota: the sender's MNAuth-verified
     * proTxHash for remote messages, or this node's own for messages it
     * produced itself (@p from == -1), which go to the front of the queue.
     * Drops the message silently on quota
     * overflow or duplicate hash; quota-dropped messages are not marked seen,
     * so another peer with budget can re-deliver them. Caller is responsible
     * for hashing the payload and (for real peers) routing the erase-request
     * to PeerManager.
     */
    void PushPendingMessage(NodeId from, const uint256& sender_protx, std::shared_ptr<CDataStream> pm,
                            const uint256& hash) EXCLUSIVE_LOCKS_REQUIRED(!cs_messages);

    std::list<BinaryMessage> PopPendingMessages(size_t maxCount) EXCLUSIVE_LOCKS_REQUIRED(!cs_messages);
    bool HasSeen(const uint256& hash) const EXCLUSIVE_LOCKS_REQUIRED(!cs_messages);
    void Clear() EXCLUSIVE_LOCKS_REQUIRED(!cs_messages);
};

/**
 * Handles multiple sequential sessions of one specific LLMQ type. There is one instance of this class per LLMQ type.
 */
class CDKGSessionHandler
{
public:
    const Consensus::LLMQParams& params;

    // Do not guard these, they protect their internals themselves
    CDKGPendingMessages pendingContributions;
    CDKGPendingMessages pendingComplaints;
    CDKGPendingMessages pendingJustifications;
    CDKGPendingMessages pendingPrematureCommitments;

public:
    explicit CDKGSessionHandler(const Consensus::LLMQParams& _params);
    virtual ~CDKGSessionHandler();

    void ClearPendingMessages();

public:
    virtual bool GetContribution(const uint256& hash, CDKGContribution& ret) const { return false; }
    virtual bool GetComplaint(const uint256& hash, CDKGComplaint& ret) const { return false; }
    virtual bool GetJustification(const uint256& hash, CDKGJustification& ret) const { return false; }
    virtual bool GetPrematureCommitment(const uint256& hash, CDKGPrematureCommitment& ret) const { return false; }
    virtual QuorumPhase GetPhase() const { return QuorumPhase::Idle; }
    virtual void UpdatedBlockTip(const CBlockIndex* pindexNew) {}
};
} // namespace llmq

#endif // BITCOIN_LLMQ_DKGSESSIONHANDLER_H
