// Copyright (c) 2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/util/net.h>

#include <chainparams.h>
#include <net.h>
#include <span.h>

#include <vector>

void ConnmanTestMsg::NodeReceiveMsgBytes(CNode& node, Span<const uint8_t> msg_bytes, bool& complete) const
{
    assert(node.ReceiveMsgBytes(msg_bytes, complete));
    if (complete) {
        size_t nSizeAdded = 0;
        auto it(node.vRecvMsg.begin());
        auto it2 = it;
        for (; it != node.vRecvMsg.end(); it2 = it++) {
            // vRecvMsg contains only completed CNetMessage
            // the single possible partially deserialized message are held by TransportDeserializer
            nSizeAdded += it->m_raw_message_size;
        }
        {
            LOCK(node.cs_vProcessMsg);
            node.vProcessMsg.splice_after(node.m_process_msg_most_recent, node.vRecvMsg, node.vRecvMsg.before_begin(), it);
            node.m_process_msg_most_recent = it2;
            node.m_recv_msg_most_recent = node.vRecvMsg.before_begin();
            node.nProcessQueueSize += nSizeAdded;
            node.fPauseRecv = node.nProcessQueueSize > nReceiveFloodSize;
        }
    }
}

bool ConnmanTestMsg::ReceiveMsgFrom(CNode& node, CSerializedNetMsg& ser_msg) const
{
    std::vector<uint8_t> ser_msg_header;
    node.m_serializer->prepareForTransport(ser_msg, ser_msg_header);

    bool complete;
    NodeReceiveMsgBytes(node, ser_msg_header, complete);
    NodeReceiveMsgBytes(node, ser_msg.data, complete);
    return complete;
}

std::vector<NodeEvictionCandidate> GetRandomNodeEvictionCandidates(int n_candidates, FastRandomContext& random_context)
{
    std::vector<NodeEvictionCandidate> candidates;
    for (int id = 0; id < n_candidates; ++id) {
        candidates.push_back({
            /* id */ id,
            /* nTimeConnected */ static_cast<int64_t>(random_context.randrange(100)),
            /* m_min_ping_time */ std::chrono::microseconds{random_context.randrange(100)},
            /* nLastBlockTime */ static_cast<int64_t>(random_context.randrange(100)),
            /* nLastTXTime */ static_cast<int64_t>(random_context.randrange(100)),
            /* fRelevantServices */ random_context.randbool(),
            /* fRelayTxes */ random_context.randbool(),
            /* fBloomFilter */ random_context.randbool(),
            /* nKeyedNetGroup */ random_context.randrange(100),
            /* prefer_evict */ random_context.randbool(),
            /* m_is_local */ random_context.randbool(),
            /* m_network */ ALL_NETWORKS[random_context.randrange(ALL_NETWORKS.size())],
        });
    }
    return candidates;
}
