// Copyright (c) 2018-2025 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <llmq/dkgsessionhandler.h>

#include <hash.h>
#include <logging.h>
#include <span.h>
#include <util/check.h>

#include <stdexcept>

namespace llmq {
CDKGSessionHandler::CDKGSessionHandler(const Consensus::LLMQParams& _params) :
    params{_params},
    // we allow size*2 messages as we need to make sure we see bad behavior (double messages)
    pendingContributions{(size_t)_params.size * 2, MSG_QUORUM_CONTRIB},
    pendingComplaints{(size_t)_params.size * 2, MSG_QUORUM_COMPLAINT},
    pendingJustifications{(size_t)_params.size * 2, MSG_QUORUM_JUSTIFICATION},
    pendingPrematureCommitments{(size_t)_params.size * 2, MSG_QUORUM_PREMATURE_COMMITMENT}
{
    if (params.type == Consensus::LLMQType::LLMQ_NONE) {
        throw std::runtime_error("Can't initialize CDKGSessionHandler with LLMQ_NONE type.");
    }
}

CDKGSessionHandler::~CDKGSessionHandler() = default;

MessageProcessingResult CDKGPendingMessages::PushPendingMessage(NodeId from, const uint256& sender_protx, CDataStream& vRecv)
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

    // Check duplicates before the quota so resent hashes don't burn budget
    if (seenMessages.count(hash) != 0) {
        LogPrint(BCLog::LLMQ_DKG, "CDKGPendingMessages::%s -- already seen %s, peer=%d\n", __func__, hash.ToString(), from);
        return ret;
    }

    // Callers always pass an identity (the MNAuth gate for remote, our own
    // proTxHash for local); drop rather than share a null-keyed quota bucket
    if (!Assume(!sender_protx.IsNull())) {
        return ret;
    }
    auto& count = messagesPerProTx[sender_protx];
    if (count >= maxMessagesPerProTx) {
        // TODO ban?
        LogPrint(BCLog::LLMQ_DKG, "CDKGPendingMessages::%s -- too many messages from %s, peer=%d\n", __func__,
                 sender_protx.ToString(), from);
        return ret;
    }
    count++;

    seenMessages.emplace(hash);
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
    messagesPerProTx.clear();
    seenMessages.clear();
}

//////

MessageProcessingResult CDKGSessionHandler::ProcessMessage(NodeId from, const uint256& sender_protx,
                                                           std::string_view msg_type, CDataStream& vRecv)
{
    // We don't handle messages in the calling thread as deserialization/processing of these would block everything
    if (msg_type == NetMsgType::QCONTRIB) {
        return pendingContributions.PushPendingMessage(from, sender_protx, vRecv);
    } else if (msg_type == NetMsgType::QCOMPLAINT) {
        return pendingComplaints.PushPendingMessage(from, sender_protx, vRecv);
    } else if (msg_type == NetMsgType::QJUSTIFICATION) {
        return pendingJustifications.PushPendingMessage(from, sender_protx, vRecv);
    } else if (msg_type == NetMsgType::QPCOMMITMENT) {
        return pendingPrematureCommitments.PushPendingMessage(from, sender_protx, vRecv);
    }
    return {};
}
} // namespace llmq
