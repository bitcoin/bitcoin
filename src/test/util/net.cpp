// Copyright (c) 2020-present The Bitcoin Core developers
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
#include <sync.h>

#include <chrono>
#include <optional>
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
    peerman.SendMessages(&node);
    FlushSendBuffer(node); // Drop the version message added by SendMessages.

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

void ConnmanTestMsg::ResetAddrCache() { m_addr_response_caches = {}; }

void ConnmanTestMsg::ResetMaxOutboundCycle()
{
    LOCK(m_total_bytes_sent_mutex);
    nMaxOutboundCycleStartTime = 0s;
    nMaxOutboundTotalBytesSentInCycle = 0;
}

void ConnmanTestMsg::NodeReceiveMsgBytes(CNode& node, std::span<const uint8_t> msg_bytes, bool& complete) const
{
    assert(node.ReceiveMsgBytes(msg_bytes, complete));
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

bool ConnmanTestMsg::ReceiveMsgFrom(CNode& node, CSerializedNetMsg&& ser_msg) const
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
            .id=id,
            .m_connected=std::chrono::seconds{random_context.randrange(100)},
            .m_min_ping_time=std::chrono::microseconds{random_context.randrange(100)},
            .m_last_block_time=std::chrono::seconds{random_context.randrange(100)},
            .m_last_tx_time=std::chrono::seconds{random_context.randrange(100)},
            .fRelevantServices=random_context.randbool(),
            .m_relay_txs=random_context.randbool(),
            .fBloomFilter=random_context.randbool(),
            .nKeyedNetGroup=random_context.randrange(100u),
            .prefer_evict=random_context.randbool(),
            .m_is_local=random_context.randbool(),
            .m_network=ALL_NETWORKS[random_context.randrange(ALL_NETWORKS.size())],
            .m_noban=false,
            .m_conn_type=ConnectionType::INBOUND,
        });
    }
    return candidates;
}

// Have different ZeroSock (or others that inherit from it) objects have different
// m_socket because EqualSharedPtrSock compares m_socket and we want to avoid two
// different objects comparing as equal.
static std::atomic<SOCKET> g_mocked_sock_fd{0};

ZeroSock::ZeroSock() : Sock{g_mocked_sock_fd++} {}

// Sock::~Sock() would try to close(2) m_socket if it is not INVALID_SOCKET, avoid that.
ZeroSock::~ZeroSock() { m_socket = INVALID_SOCKET; }

ssize_t ZeroSock::Send(const void*, size_t len, int) const { return len; }

ssize_t ZeroSock::Recv(void* buf, size_t len, int flags) const
{
    memset(buf, 0x0, len);
    return len;
}

int ZeroSock::Connect(const sockaddr*, socklen_t) const { return 0; }

int ZeroSock::Bind(const sockaddr*, socklen_t) const { return 0; }

int ZeroSock::Listen(int) const { return 0; }

std::unique_ptr<Sock> ZeroSock::Accept(sockaddr* addr, socklen_t* addr_len) const
{
    if (addr != nullptr) {
        // Pretend all connections come from 5.5.5.5:6789
        memset(addr, 0x00, *addr_len);
        const socklen_t write_len = static_cast<socklen_t>(sizeof(sockaddr_in));
        if (*addr_len >= write_len) {
            *addr_len = write_len;
            sockaddr_in* addr_in = reinterpret_cast<sockaddr_in*>(addr);
            addr_in->sin_family = AF_INET;
            memset(&addr_in->sin_addr, 0x05, sizeof(addr_in->sin_addr));
            addr_in->sin_port = htons(6789);
        }
    }
    return std::make_unique<ZeroSock>();
}

int ZeroSock::GetSockOpt(int level, int opt_name, void* opt_val, socklen_t* opt_len) const
{
    std::memset(opt_val, 0x0, *opt_len);
    return 0;
}

int ZeroSock::SetSockOpt(int, int, const void*, socklen_t) const { return 0; }

int ZeroSock::GetSockName(sockaddr* name, socklen_t* name_len) const
{
    std::memset(name, 0x0, *name_len);
    return 0;
}

bool ZeroSock::SetNonBlocking() const { return true; }

bool ZeroSock::IsSelectable() const { return true; }

bool ZeroSock::Wait(std::chrono::milliseconds timeout, Event requested, Event* occurred) const
{
    if (occurred != nullptr) {
        *occurred = requested;
    }
    return true;
}

bool ZeroSock::WaitMany(std::chrono::milliseconds timeout, EventsPerSock& events_per_sock) const
{
    for (auto& [sock, events] : events_per_sock) {
        (void)sock;
        events.occurred = events.requested;
    }
    return true;
}

ZeroSock& ZeroSock::operator=(Sock&& other)
{
    assert(false && "Move of Sock into ZeroSock not allowed.");
    return *this;
}

StaticContentsSock::StaticContentsSock(const std::string& contents)
    : m_contents{contents}
{
}

ssize_t StaticContentsSock::Recv(void* buf, size_t len, int flags) const
{
    const size_t consume_bytes{std::min(len, m_contents.size() - m_consumed)};
    std::memcpy(buf, m_contents.data() + m_consumed, consume_bytes);
    if ((flags & MSG_PEEK) == 0) {
        m_consumed += consume_bytes;
    }
    return consume_bytes;
}

StaticContentsSock& StaticContentsSock::operator=(Sock&& other)
{
    assert(false && "Move of Sock into StaticContentsSock not allowed.");
    return *this;
}

ssize_t DynSock::Pipe::GetBytes(void* buf, size_t len, int flags)
{
    WAIT_LOCK(m_mutex, lock);

    if (m_data.empty()) {
        if (m_eof) {
            return 0;
        }
        errno = EAGAIN; // Same as recv(2) on a non-blocking socket.
        return -1;
    }

    const size_t read_bytes{std::min(len, m_data.size())};

    std::memcpy(buf, m_data.data(), read_bytes);
    if ((flags & MSG_PEEK) == 0) {
        m_data.erase(m_data.begin(), m_data.begin() + read_bytes);
    }

    return read_bytes;
}

std::optional<CNetMessage> DynSock::Pipe::GetNetMsg()
{
    V1Transport transport{NodeId{0}};

    {
        WAIT_LOCK(m_mutex, lock);

        WaitForDataOrEof(lock);
        if (m_eof && m_data.empty()) {
            return std::nullopt;
        }

        for (;;) {
            std::span<const uint8_t> s{m_data};
            if (!transport.ReceivedBytes(s)) {  // Consumed bytes are removed from the front of s.
                return std::nullopt;
            }
            m_data.erase(m_data.begin(), m_data.begin() + m_data.size() - s.size());
            if (transport.ReceivedMessageComplete()) {
                break;
            }
            if (m_data.empty()) {
                WaitForDataOrEof(lock);
                if (m_eof && m_data.empty()) {
                    return std::nullopt;
                }
            }
        }
    }

    bool reject{false};
    CNetMessage msg{transport.GetReceivedMessage(/*time=*/{}, reject)};
    if (reject) {
        return std::nullopt;
    }
    return std::make_optional<CNetMessage>(std::move(msg));
}

void DynSock::Pipe::PushBytes(const void* buf, size_t len)
{
    LOCK(m_mutex);
    const uint8_t* b = static_cast<const uint8_t*>(buf);
    m_data.insert(m_data.end(), b, b + len);
    m_cond.notify_all();
}

void DynSock::Pipe::Eof()
{
    LOCK(m_mutex);
    m_eof = true;
    m_cond.notify_all();
}

void DynSock::Pipe::WaitForDataOrEof(UniqueLock<Mutex>& lock)
{
    Assert(lock.mutex() == &m_mutex);

    m_cond.wait(lock, [&]() EXCLUSIVE_LOCKS_REQUIRED(m_mutex) {
        AssertLockHeld(m_mutex);
        return !m_data.empty() || m_eof;
    });
}

DynSock::DynSock(std::shared_ptr<Pipes> pipes, std::shared_ptr<Queue> accept_sockets)
    : m_pipes{pipes}, m_accept_sockets{accept_sockets}
{
}

DynSock::~DynSock()
{
    m_pipes->send.Eof();
}

ssize_t DynSock::Recv(void* buf, size_t len, int flags) const
{
    return m_pipes->recv.GetBytes(buf, len, flags);
}

ssize_t DynSock::Send(const void* buf, size_t len, int) const
{
    m_pipes->send.PushBytes(buf, len);
    return len;
}

std::unique_ptr<Sock> DynSock::Accept(sockaddr* addr, socklen_t* addr_len) const
{
    ZeroSock::Accept(addr, addr_len);
    return m_accept_sockets->Pop().value_or(nullptr);
}

bool DynSock::Wait(std::chrono::milliseconds timeout,
                   Event requested,
                   Event* occurred) const
{
    EventsPerSock ev;
    ev.emplace(this, Events{requested});
    const bool ret{WaitMany(timeout, ev)};
    if (occurred != nullptr) {
        *occurred = ev.begin()->second.occurred;
    }
    return ret;
}

bool DynSock::WaitMany(std::chrono::milliseconds timeout, EventsPerSock& events_per_sock) const
{
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    bool at_least_one_event_occurred{false};

    for (;;) {
        // Check all sockets for readiness without waiting.
        for (auto& [sock, events] : events_per_sock) {
            if ((events.requested & Sock::SEND) != 0) {
                // Always ready for Send().
                events.occurred |= Sock::SEND;
                at_least_one_event_occurred = true;
            }

            if ((events.requested & Sock::RECV) != 0) {
                auto dyn_sock = reinterpret_cast<const DynSock*>(sock.get());
                uint8_t b;
                if (dyn_sock->m_pipes->recv.GetBytes(&b, 1, MSG_PEEK) == 1 || !dyn_sock->m_accept_sockets->Empty()) {
                    events.occurred |= Sock::RECV;
                    at_least_one_event_occurred = true;
                }
            }
        }

        if (at_least_one_event_occurred || std::chrono::steady_clock::now() > deadline) {
            break;
        }

        std::this_thread::sleep_for(10ms);
    }

    return true;
}

DynSock& DynSock::operator=(Sock&&)
{
    assert(false && "Move of Sock into DynSock not allowed.");
    return *this;
}
