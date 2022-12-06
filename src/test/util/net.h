// Copyright (c) 2020-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_TEST_UTIL_NET_H
#define BITCOIN_TEST_UTIL_NET_H

#include <compat/compat.h>
#include <net.h>
#include <net_permissions.h>
#include <net_processing.h>
#include <netaddress.h>
#include <node/connection_types.h>
#include <node/eviction.h>
#include <sync.h>
#include <util/sock.h>

#include <algorithm>
#include <array>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

class FastRandomContext;

template <typename C>
class Span;

struct ConnmanTestMsg : public CConnman {
    using CConnman::CConnman;

    void SetMsgProc(NetEventsInterface* msgproc)
    {
        m_msgproc = msgproc;
    }

    void SetPeerConnectTimeout(std::chrono::seconds timeout)
    {
        m_peer_connect_timeout = timeout;
    }

    std::vector<CNode*> TestNodes()
    {
        LOCK(m_nodes_mutex);
        return m_nodes;
    }

    void AddTestNode(CNode& node)
    {
        LOCK(m_nodes_mutex);
        m_nodes.push_back(&node);

        if (node.IsManualOrFullOutboundConn()) ++m_network_conn_counts[node.addr.GetNetwork()];
    }

    void ClearTestNodes()
    {
        LOCK(m_nodes_mutex);
        for (CNode* node : m_nodes) {
            delete node;
        }
        m_nodes.clear();
    }

    void Handshake(CNode& node,
                   bool successfully_connected,
                   ServiceFlags remote_services,
                   ServiceFlags local_services,
                   int32_t version,
                   bool relay_txs)
        EXCLUSIVE_LOCKS_REQUIRED(NetEventsInterface::g_msgproc_mutex);

    bool ProcessMessagesOnce(CNode& node) EXCLUSIVE_LOCKS_REQUIRED(NetEventsInterface::g_msgproc_mutex)
    {
        return m_msgproc->ProcessMessages(&node, flagInterruptMsgProc);
    }

    void NodeReceiveMsgBytes(CNode& node, Span<const uint8_t> msg_bytes, bool& complete) const;

    bool ReceiveMsgFrom(CNode& node, CSerializedNetMsg&& ser_msg) const;
    void FlushSendBuffer(CNode& node) const;

    bool AlreadyConnectedPublic(const CAddress& addr) { return AlreadyConnectedToAddress(addr); };

    CNode* ConnectNodePublic(PeerManager& peerman, const char* pszDest, ConnectionType conn_type)
        EXCLUSIVE_LOCKS_REQUIRED(!m_unused_i2p_sessions_mutex);
};

constexpr ServiceFlags ALL_SERVICE_FLAGS[]{
    NODE_NONE,
    NODE_NETWORK,
    NODE_BLOOM,
    NODE_WITNESS,
    NODE_COMPACT_FILTERS,
    NODE_NETWORK_LIMITED,
    NODE_P2P_V2,
};

constexpr NetPermissionFlags ALL_NET_PERMISSION_FLAGS[]{
    NetPermissionFlags::None,
    NetPermissionFlags::BloomFilter,
    NetPermissionFlags::Relay,
    NetPermissionFlags::ForceRelay,
    NetPermissionFlags::NoBan,
    NetPermissionFlags::Mempool,
    NetPermissionFlags::Addr,
    NetPermissionFlags::Download,
    NetPermissionFlags::Implicit,
    NetPermissionFlags::All,
};

constexpr ConnectionType ALL_CONNECTION_TYPES[]{
    ConnectionType::INBOUND,
    ConnectionType::OUTBOUND_FULL_RELAY,
    ConnectionType::MANUAL,
    ConnectionType::FEELER,
    ConnectionType::BLOCK_RELAY,
    ConnectionType::ADDR_FETCH,
};

constexpr auto ALL_NETWORKS = std::array{
    Network::NET_UNROUTABLE,
    Network::NET_IPV4,
    Network::NET_IPV6,
    Network::NET_ONION,
    Network::NET_I2P,
    Network::NET_CJDNS,
    Network::NET_INTERNAL,
};

/**
 * A mocked Sock alternative that returns a statically contained data upon read and succeeds
 * and ignores all writes. The data to be returned is given to the constructor and when it is
 * exhausted an EOF is returned by further reads.
 */
class StaticContentsSock : public Sock
{
public:
    explicit StaticContentsSock(const std::string& contents);

    ~StaticContentsSock() override;

    StaticContentsSock& operator=(Sock&& other) override;

    ssize_t Send(const void*, size_t len, int) const override;

    ssize_t Recv(void* buf, size_t len, int flags) const override;

    int Connect(const sockaddr*, socklen_t) const override;

    int Bind(const sockaddr*, socklen_t) const override;

    int Listen(int) const override;

    std::unique_ptr<Sock> Accept(sockaddr* addr, socklen_t* addr_len) const override;

    int GetSockOpt(int level, int opt_name, void* opt_val, socklen_t* opt_len) const override;

    int SetSockOpt(int, int, const void*, socklen_t) const override;

    int GetSockName(sockaddr* name, socklen_t* name_len) const override;

    bool SetNonBlocking() const override;

    bool IsSelectable() const override;

    bool Wait(std::chrono::milliseconds timeout,
              Event requested,
              Event* occurred = nullptr) const override;

    bool WaitMany(std::chrono::milliseconds timeout, EventsPerSock& events_per_sock) const override;

    bool IsConnected(std::string&) const override;

private:
    const std::string m_contents;
    mutable size_t m_consumed{0};
};

std::vector<NodeEvictionCandidate> GetRandomNodeEvictionCandidates(int n_candidates, FastRandomContext& random_context);

#endif // BITCOIN_TEST_UTIL_NET_H
