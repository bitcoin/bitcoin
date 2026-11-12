// Copyright (c) 2018-2025 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <llmq/dkgsessionhandler.h>

#include <logging.h>
#include <uint256.h>
#include <util/check.h>

#include <stdexcept>

namespace llmq {
CDKGSessionHandler::CDKGSessionHandler(const Consensus::LLMQParams& _params) :
    params{_params},
    // we allow size*2 messages per proTx as we need to make sure we see bad behavior (double messages)
    pendingContributions{static_cast<size_t>(_params.size) * 2},
    pendingComplaints{static_cast<size_t>(_params.size) * 2},
    pendingJustifications{static_cast<size_t>(_params.size) * 2},
    pendingPrematureCommitments{static_cast<size_t>(_params.size) * 2}
{
    if (params.type == Consensus::LLMQType::LLMQ_NONE) {
        throw std::runtime_error("Can't initialize CDKGSessionHandler with LLMQ_NONE type.");
    }
}

CDKGSessionHandler::~CDKGSessionHandler() = default;

void CDKGPendingMessages::PushPendingMessage(NodeId from, const uint256& sender_protx,
                                             std::shared_ptr<CDataStream> pm, const uint256& hash)
{
    LOCK(cs_messages);

    // Check duplicates before the quota so resent hashes don't burn budget
    if (seenMessages.count(hash) != 0) {
        LogPrint(BCLog::LLMQ_DKG, "CDKGPendingMessages::%s -- already seen %s, peer=%d\n", __func__, hash.ToString(), from);
        return;
    }

    // Callers always pass an identity (the MNAuth gate for remote, our own
    // proTxHash for local); drop rather than share a null-keyed quota bucket
    if (!Assume(!sender_protx.IsNull())) {
        return;
    }
    auto& count = messagesPerProTx[sender_protx];
    if (count >= maxMessagesPerProTx) {
        // TODO ban?
        LogPrint(BCLog::LLMQ_DKG, "CDKGPendingMessages::%s -- too many messages from %s, peer=%d\n", __func__,
                 sender_protx.ToString(), from);
        return;
    }
    count++;

    seenMessages.emplace(hash);
    if (from == -1) {
        // Our own message must not wait behind a backlog of peer messages before it is relayed
        pendingMessages.emplace_front(from, std::move(pm));
    } else {
        pendingMessages.emplace_back(from, std::move(pm));
    }
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

void CDKGPendingMessages::Clear()
{
    LOCK(cs_messages);
    pendingMessages.clear();
    messagesPerProTx.clear();
    seenMessages.clear();
}

void CDKGSessionHandler::ClearPendingMessages()
{
    pendingContributions.Clear();
    pendingComplaints.Clear();
    pendingJustifications.Clear();
    pendingPrematureCommitments.Clear();
}
} // namespace llmq
