// Copyright (c) 2020-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/util/net.h>

#include <net.h>
#include <net_processing.h>
#include <netaddress.h>
#include <netmessagemaker.h>
#include <node/connection_types.h>
#include <node/eviction.h>
#include <protocol.h>
#include <random.h>
#include <serialize.h>
#include <span.h>

#include <vector>

void ConnmanTestMsg::Handshake(CNode& node,
                               bool successfully_connected,
                               ServiceFlags remote_services,
                               ServiceFlags local_services,
                               int32_t version,
                               bool relay_txs)
{
    auto& peerman{static_cast<PeerManager&>(*m_msgproc)};
    auto& connman{*this};

    peerman.InitializeNode(node, local_services);
    FlushSendBuffer(node); // Drop the version message added by InitializeNode.

    CSerializedNetMsg msg_version{
        NetMsg::Make(NetMsgType::VERSION,
                version,                                        //
                Using<CustomUintFormatter<8>>(remote_services), //
                int64_t{},                                      // dummy time
                int64_t{},                                      // ignored service bits
                CNetAddr::V1(CService{}),                       // dummy
                int64_t{},                                      // ignored service bits
                CNetAddr::V1(CService{}),                       // ignored
                uint64_t{1},                                    // dummy nonce
                std::string{},                                  // dummy subver
                int32_t{},                                      // dummy starting_height
                relay_txs),
    };

    (void)connman.ReceiveMsgFrom(node, std::move(msg_version));
    node.fPauseSend = false;
    connman.ProcessMessagesOnce(node);
    peerman.SendMessages(&node);
    FlushSendBuffer(node); // Drop the verack message added by SendMessages.
    if (node.fDisconnect) return;
    assert(node.nVersion == version);
    assert(node.GetCommonVersion() == std::min(version, PROTOCOL_VERSION));
    CNodeStateStats statestats;
    assert(peerman.GetNodeStateStats(node.GetId(), statestats));
    assert(statestats.m_relay_txs == (relay_txs && !node.IsBlockOnlyConn()));
    assert(statestats.their_services == remote_services);
    if (successfully_connected) {
        CSerializedNetMsg msg_verack{NetMsg::Make(NetMsgType::VERACK)};
        (void)connman.ReceiveMsgFrom(node, std::move(msg_verack));
        node.fPauseSend = false;
        connman.ProcessMessagesOnce(node);
        peerman.SendMessages(&node);
        assert(node.fSuccessfullyConnected == true);
    }
}

void ConnmanTestMsg::NodeReceiveMsgBytes(CNode& node, Span<const uint8_t> msg_bytes, bool& complete)
{
    assert(node.ReceiveMsgBytes(msg_bytes, complete, m_net_stats));
    if (complete) {
        node.MarkReceivedMsgsForProcessing();
    }
}

void ConnmanTestMsg::FlushSendBuffer(CNode& node) const
{
    LOCK(node.cs_vSend);
    node.vSendMsg.clear();
    node.m_send_memusage = 0;
    while (true) {
        const auto& [to_send, _more, _msg_type] = node.m_transport->GetBytesToSend(false);
        if (to_send.empty()) break;
        node.m_transport->MarkBytesSent(to_send.size());
    }
}

bool ConnmanTestMsg::ReceiveMsgFrom(CNode& node, CSerializedNetMsg&& ser_msg)
{
    bool queued = node.m_transport->SetMessageToSend(ser_msg);
    assert(queued);
    bool complete{false};
    while (true) {
        const auto& [to_send, _more, _msg_type] = node.m_transport->GetBytesToSend(false);
        if (to_send.empty()) break;
        NodeReceiveMsgBytes(node, to_send, complete);
        node.m_transport->MarkBytesSent(to_send.size());
    }
    return complete;
}

CNode* ConnmanTestMsg::ConnectNodePublic(PeerManager& peerman, const char* pszDest, ConnectionType conn_type)
{
    CNode* node = ConnectNode(CAddress{}, pszDest, /*fCountFailure=*/false, conn_type, /*use_v2transport=*/true);
    if (!node) return nullptr;
    node->SetCommonVersion(PROTOCOL_VERSION);
    peerman.InitializeNode(*node, ServiceFlags(NODE_NETWORK | NODE_WITNESS));
    node->fSuccessfullyConnected = true;
    AddTestNode(*node);
    return node;
}

std::vector<NodeEvictionCandidate> GetRandomNodeEvictionCandidates(int n_candidates, FastRandomContext& random_context)
{
    std::vector<NodeEvictionCandidate> candidates;
    candidates.reserve(n_candidates);
    for (int id = 0; id < n_candidates; ++id) {
        candidates.push_back({
            /*id=*/id,
            /*m_connected=*/std::chrono::seconds{random_context.randrange(100)},
            /*m_min_ping_time=*/std::chrono::microseconds{random_context.randrange(100)},
            /*m_last_block_time=*/std::chrono::seconds{random_context.randrange(100)},
            /*m_last_tx_time=*/std::chrono::seconds{random_context.randrange(100)},
            /*fRelevantServices=*/random_context.randbool(),
            /*m_relay_txs=*/random_context.randbool(),
            /*fBloomFilter=*/random_context.randbool(),
            /*nKeyedNetGroup=*/random_context.randrange(100),
            /*prefer_evict=*/random_context.randbool(),
            /*m_is_local=*/random_context.randbool(),
            /*m_network=*/ALL_NETWORKS[random_context.randrange(ALL_NETWORKS.size())],
            /*m_noban=*/false,
            /*m_conn_type=*/ConnectionType::INBOUND,
        });
    }
    return candidates;
}
