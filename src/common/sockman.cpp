// Copyright (c) 2024-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit/.

#include <config/bitcoin-config.h> // IWYU pragma: keep

#include <common/sockman.h>
#include <logging.h>
#include <netbase.h>
#include <util/sock.h>
#include <util/thread.h>

// The set of sockets cannot be modified while waiting
// The sleep time needs to be small to avoid new sockets stalling
static constexpr auto SELECT_TIMEOUT{50ms};

static CService GetBindAddress(const Sock& sock)
{
    CService addr_bind;
    struct sockaddr_storage sockaddr_bind;
    socklen_t sockaddr_bind_len = sizeof(sockaddr_bind);
    if (!sock.GetSockName((struct sockaddr*)&sockaddr_bind, &sockaddr_bind_len)) {
        addr_bind.SetSockAddr((const struct sockaddr*)&sockaddr_bind);
    } else {
        LogPrintLevel(BCLog::NET, BCLog::Level::Warning, "getsockname failed\n");
    }
    return addr_bind;
}

bool SockMan::BindAndStartListening(const CService& to, bilingual_str& errmsg)
{
    // Create socket for listening for incoming connections
    struct sockaddr_storage storage;
    socklen_t len{sizeof(storage)};
    if (!to.GetSockAddr(reinterpret_cast<sockaddr*>(&storage), &len)) {
        errmsg = strprintf(Untranslated("Bind address family for %s not supported"), to.ToStringAddrPort());
        return false;
    }

    std::unique_ptr<Sock> sock{CreateSock(to.GetSAFamily(), SOCK_STREAM, IPPROTO_TCP)};
    if (!sock) {
        errmsg = strprintf(Untranslated("Cannot create %s listen socket: %s"),
                           to.ToStringAddrPort(),
                           NetworkErrorString(WSAGetLastError()));
        return false;
    }

    int one{1};

    // Allow binding if the port is still in TIME_WAIT state after
    // the program was closed and restarted.
    if (sock->SetSockOpt(SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<sockopt_arg_type>(&one), sizeof(one)) == SOCKET_ERROR) {
        LogPrintLevel(BCLog::NET,
                      BCLog::Level::Info,
                      "Cannot set SO_REUSEADDR on %s listen socket: %s, continuing anyway\n",
                      to.ToStringAddrPort(),
                      NetworkErrorString(WSAGetLastError()));
    }

    // some systems don't have IPV6_V6ONLY but are always v6only; others do have the option
    // and enable it by default or not. Try to enable it, if possible.
    if (to.IsIPv6()) {
#ifdef IPV6_V6ONLY
        if (sock->SetSockOpt(IPPROTO_IPV6, IPV6_V6ONLY, reinterpret_cast<sockopt_arg_type>(&one), sizeof(one)) == SOCKET_ERROR) {
            LogPrintLevel(BCLog::NET,
                          BCLog::Level::Info,
                          "Cannot set IPV6_V6ONLY on %s listen socket: %s, continuing anyway\n",
                          to.ToStringAddrPort(),
                          NetworkErrorString(WSAGetLastError()));
        }
#endif
#ifdef WIN32
        int prot_level{PROTECTION_LEVEL_UNRESTRICTED};
        if (sock->SetSockOpt(IPPROTO_IPV6,
                             IPV6_PROTECTION_LEVEL,
                             reinterpret_cast<const char*>(&prot_level),
                             sizeof(prot_level)) == SOCKET_ERROR) {
            LogPrintLevel(BCLog::NET,
                          BCLog::Level::Info,
                          "Cannot set IPV6_PROTECTION_LEVEL on %s listen socket: %s, continuing anyway\n",
                          to.ToStringAddrPort(),
                          NetworkErrorString(WSAGetLastError()));
        }
#endif
    }

    if (sock->Bind(reinterpret_cast<sockaddr*>(&storage), len) == SOCKET_ERROR) {
        const int err{WSAGetLastError()};
        errmsg = strprintf(_("Cannot bind to %s: %s%s"),
                           to.ToStringAddrPort(),
                           NetworkErrorString(err),
                           err == WSAEADDRINUSE
                               ? std::string{" ("} + PACKAGE_NAME + " already running?)"
                               : "");
        return false;
    }

    // Listen for incoming connections
    if (sock->Listen(SOMAXCONN) == SOCKET_ERROR) {
        errmsg = strprintf(_("Cannot listen to %s: %s"), to.ToStringAddrPort(), NetworkErrorString(WSAGetLastError()));
        return false;
    }

    m_listen.emplace_back(std::move(sock));

    return true;
}

void SockMan::StartSocketsThreads(const Options& options)
{
    m_thread_socket_handler = std::thread(&util::TraceThread, "net", [this] { ThreadSocketHandler(); });

    if (options.i2p.has_value()) {
        m_i2p_sam_session = std::make_unique<i2p::sam::Session>(
            options.i2p->private_key_file, options.i2p->sam_proxy, &interruptNet);

        m_thread_i2p_accept =
            std::thread(&util::TraceThread, "i2paccept", [this] { ThreadI2PAccept(); });
    }
}

void SockMan::JoinSocketsThreads()
{
    if (m_thread_i2p_accept.joinable()) {
        m_thread_i2p_accept.join();
    }

    if (m_thread_socket_handler.joinable()) {
        m_thread_socket_handler.join();
    }
}

std::optional<NodeId>
SockMan::ConnectAndMakeNodeId(const std::variant<CService, StringHostIntPort>& to,
                              bool is_important,
                              const Proxy& proxy,
                              bool& proxy_failed,
                              CService& me)
{
    AssertLockNotHeld(m_connected_mutex);
    AssertLockNotHeld(m_unused_i2p_sessions_mutex);

    std::unique_ptr<Sock> sock;
    std::unique_ptr<i2p::sam::Session> i2p_transient_session;

    Assume(!me.IsValid());

    if (std::holds_alternative<CService>(to)) {
        const CService& addr_to{std::get<CService>(to)};
        if (addr_to.IsI2P()) {
            if (!Assume(proxy.IsValid())) {
                return std::nullopt;
            }

            i2p::Connection conn;
            bool connected{false};

            if (m_i2p_sam_session) {
                connected = m_i2p_sam_session->Connect(addr_to, conn, proxy_failed);
            } else {
                {
                    LOCK(m_unused_i2p_sessions_mutex);
                    if (m_unused_i2p_sessions.empty()) {
                        i2p_transient_session = std::make_unique<i2p::sam::Session>(proxy, &interruptNet);
                    } else {
                        i2p_transient_session.swap(m_unused_i2p_sessions.front());
                        m_unused_i2p_sessions.pop();
                    }
                }
                connected = i2p_transient_session->Connect(addr_to, conn, proxy_failed);
                if (!connected) {
                    LOCK(m_unused_i2p_sessions_mutex);
                    if (m_unused_i2p_sessions.size() < MAX_UNUSED_I2P_SESSIONS_SIZE) {
                        m_unused_i2p_sessions.emplace(i2p_transient_session.release());
                    }
                }
            }

            if (connected) {
                sock = std::move(conn.sock);
                me = conn.me;
            }
        } else if (proxy.IsValid()) {
            sock = ConnectThroughProxy(proxy, addr_to.ToStringAddr(), addr_to.GetPort(), proxy_failed);
        } else {
            sock = ConnectDirectly(addr_to, is_important);
        }
    } else {
        if (!Assume(proxy.IsValid())) {
            return std::nullopt;
        }

        const auto& hostport{std::get<StringHostIntPort>(to)};

        bool dummy_proxy_failed;
        sock = ConnectThroughProxy(proxy, hostport.host, hostport.port, dummy_proxy_failed);
    }

    if (!sock) {
        return std::nullopt;
    }

    if (!me.IsValid()) {
        me = GetBindAddress(*sock);
    }

    const NodeId node_id{GetNewNodeId()};

    {
        LOCK(m_connected_mutex);
        m_connected.emplace(node_id, std::make_shared<NodeSockets>(std::move(sock),
                                                                   std::move(i2p_transient_session)));
    }

    return node_id;
}

bool SockMan::CloseConnection(NodeId node_id)
{
    LOCK(m_connected_mutex);
    return m_connected.erase(node_id) > 0;
}

ssize_t SockMan::SendBytes(NodeId node_id,
                           Span<const unsigned char> data,
                           bool will_send_more,
                           std::string& errmsg) const
{
    AssertLockNotHeld(m_connected_mutex);

    if (data.empty()) {
        return 0;
    }

    auto node_sockets{GetNodeSockets(node_id)};
    if (!node_sockets) {
        // Bail out immediately and just leave things in the caller's send queue.
        return 0;
    }

    int flags{MSG_NOSIGNAL | MSG_DONTWAIT};
#ifdef MSG_MORE
    if (will_send_more) {
        flags |= MSG_MORE;
    }
#endif

    const ssize_t sent{WITH_LOCK(
        node_sockets->mutex,
        return node_sockets->sock->Send(reinterpret_cast<const char*>(data.data()), data.size(), flags);)};

    if (sent >= 0) {
        return sent;
    }

    const int err{WSAGetLastError()};
    if (err == WSAEWOULDBLOCK || err == WSAEMSGSIZE || err == WSAEINTR || err == WSAEINPROGRESS) {
        return 0;
    }
    errmsg = NetworkErrorString(err);
    return -1;
}

void SockMan::CloseSockets()
{
    m_listen.clear();
}

bool SockMan::ShouldTryToSend(NodeId node_id) const { return true; }

bool SockMan::ShouldTryToRecv(NodeId node_id) const { return true; }

void SockMan::EventIOLoopCompletedForNode(NodeId node_id) {}

void SockMan::EventIOLoopCompletedForAllPeers() {}

void SockMan::EventI2PListen(const CService&, bool) {}

void SockMan::TestOnlyAddExistentNode(NodeId node_id, std::unique_ptr<Sock>&& sock)
{
    LOCK(m_connected_mutex);
    m_connected.emplace(node_id, std::make_shared<NodeSockets>(std::move(sock)));
}

void SockMan::ThreadI2PAccept()
{
    AssertLockNotHeld(m_connected_mutex);

    static constexpr auto err_wait_begin = 1s;
    static constexpr auto err_wait_cap = 5min;
    auto err_wait = err_wait_begin;

    i2p::Connection conn;

    auto SleepOnFailure = [&]() {
        interruptNet.sleep_for(err_wait);
        if (err_wait < err_wait_cap) {
            err_wait += 1s;
        }
    };

    while (!interruptNet) {

        if (!m_i2p_sam_session->Listen(conn)) {
            EventI2PListen(conn.me, /*success=*/false);
            SleepOnFailure();
            continue;
        }

        EventI2PListen(conn.me, /*success=*/true);

        if (!m_i2p_sam_session->Accept(conn)) {
            SleepOnFailure();
            continue;
        }

        NewSockAccepted(std::move(conn.sock), conn.me, conn.peer);

        err_wait = err_wait_begin;
    }
}

void SockMan::ThreadSocketHandler()
{
    AssertLockNotHeld(m_connected_mutex);

    while (!interruptNet) {
        EventIOLoopCompletedForAllPeers();

        // Check for the readiness of the already connected sockets and the
        // listening sockets in one call ("readiness" as in poll(2) or
        // select(2)). If none are ready, wait for a short while and return
        // empty sets.
        auto io_readiness{GenerateWaitSockets()};
        if (io_readiness.events_per_sock.empty() ||
            !io_readiness.events_per_sock.begin()->first->WaitMany(SELECT_TIMEOUT,
                                                                   io_readiness.events_per_sock)) {
            interruptNet.sleep_for(SELECT_TIMEOUT);
        }

        // Service (send/receive) each of the already connected sockets.
        SocketHandlerConnected(io_readiness);

        // Accept new connections from listening sockets.
        SocketHandlerListening(io_readiness.events_per_sock);
    }
}

std::unique_ptr<Sock> SockMan::AcceptConnection(const Sock& listen_sock, CService& addr)
{
    sockaddr_storage storage;
    socklen_t len{sizeof(storage)};

    auto sock{listen_sock.Accept(reinterpret_cast<sockaddr*>(&storage), &len)};

    if (!sock) {
        const int err{WSAGetLastError()};
        if (err != WSAEWOULDBLOCK) {
            LogPrintLevel(BCLog::NET,
                          BCLog::Level::Error,
                          "Cannot accept new connection: %s\n",
                          NetworkErrorString(err));
        }
        return {};
    }

    if (!addr.SetSockAddr(reinterpret_cast<sockaddr*>(&storage))) {
        LogPrintLevel(BCLog::NET, BCLog::Level::Warning, "Unknown socket family\n");
    }

    return sock;
}

void SockMan::NewSockAccepted(std::unique_ptr<Sock>&& sock, const CService& me, const CService& them)
{
    AssertLockNotHeld(m_connected_mutex);

    if (!sock->IsSelectable()) {
        LogPrintf("connection from %s dropped: non-selectable socket\n", them.ToStringAddrPort());
        return;
    }

    // According to the internet TCP_NODELAY is not carried into accepted sockets
    // on all platforms.  Set it again here just to be sure.
    const int on{1};
    if (sock->SetSockOpt(IPPROTO_TCP, TCP_NODELAY, &on, sizeof(on)) == SOCKET_ERROR) {
        LogDebug(BCLog::NET, "connection from %s: unable to set TCP_NODELAY, continuing anyway\n",
                 them.ToStringAddrPort());
    }

    const NodeId node_id{GetNewNodeId()};

    {
        LOCK(m_connected_mutex);
        m_connected.emplace(node_id, std::make_shared<NodeSockets>(std::move(sock)));
    }

    if (!EventNewConnectionAccepted(node_id, me, them)) {
        CloseConnection(node_id);
    }
}

NodeId SockMan::GetNewNodeId()
{
    return m_next_node_id.fetch_add(1, std::memory_order_relaxed);
}

SockMan::IOReadiness SockMan::GenerateWaitSockets()
{
    AssertLockNotHeld(m_connected_mutex);

    IOReadiness io_readiness;

    for (const auto& sock : m_listen) {
        io_readiness.events_per_sock.emplace(sock, Sock::Events{Sock::RECV});
    }

    auto connected_snapshot{WITH_LOCK(m_connected_mutex, return m_connected;)};

    for (const auto& [node_id, node_sockets] : connected_snapshot) {
        const bool select_recv{ShouldTryToRecv(node_id)};
        const bool select_send{ShouldTryToSend(node_id)};
        if (!select_recv && !select_send) continue;

        Sock::Event event = (select_send ? Sock::SEND : 0) | (select_recv ? Sock::RECV : 0);
        io_readiness.events_per_sock.emplace(node_sockets->sock, Sock::Events{event});
        io_readiness.node_ids_per_sock.emplace(node_sockets->sock, node_id);
    }

    return io_readiness;
}

void SockMan::SocketHandlerConnected(const IOReadiness& io_readiness)
{
    AssertLockNotHeld(m_connected_mutex);

    for (const auto& [sock, events] : io_readiness.events_per_sock) {
        if (interruptNet) {
            return;
        }

        auto it{io_readiness.node_ids_per_sock.find(sock)};
        if (it == io_readiness.node_ids_per_sock.end()) {
            continue;
        }
        const NodeId node_id{it->second};

        bool send_ready = events.occurred & Sock::SEND;
        bool recv_ready = events.occurred & Sock::RECV;
        bool err_ready = events.occurred & Sock::ERR;

        if (send_ready) {
            bool cancel_recv;

            EventReadyToSend(node_id, cancel_recv);

            if (cancel_recv) {
                recv_ready = false;
            }
        }

        if (recv_ready || err_ready) {
            uint8_t buf[0x10000]; // typical socket buffer is 8K-64K

            auto node_sockets{GetNodeSockets(node_id)};
            if (!node_sockets) {
                continue;
            }

            const ssize_t nrecv{WITH_LOCK(
                node_sockets->mutex,
                return node_sockets->sock->Recv(buf, sizeof(buf), MSG_DONTWAIT);)};

            switch (nrecv) {
            case -1: {
                const int err = WSAGetLastError();
                if (err != WSAEWOULDBLOCK && err != WSAEMSGSIZE && err != WSAEINTR && err != WSAEINPROGRESS) {
                    EventGotPermanentReadError(node_id, NetworkErrorString(err));
                }
                break;
            }
            case 0:
                EventGotEOF(node_id);
                break;
            default:
                EventGotData(node_id, buf, nrecv);
                break;
            }
        }

        EventIOLoopCompletedForNode(node_id);
    }
}

void SockMan::SocketHandlerListening(const Sock::EventsPerSock& events_per_sock)
{
    AssertLockNotHeld(m_connected_mutex);

    for (const auto& sock : m_listen) {
        if (interruptNet) {
            return;
        }
        const auto it = events_per_sock.find(sock);
        if (it != events_per_sock.end() && it->second.occurred & Sock::RECV) {
            CService addr_accepted;

            auto sock_accepted{AcceptConnection(*sock, addr_accepted)};

            if (sock_accepted) {
                NewSockAccepted(std::move(sock_accepted), GetBindAddress(*sock), addr_accepted);
            }
        }
    }
}

std::shared_ptr<SockMan::NodeSockets> SockMan::GetNodeSockets(NodeId node_id) const
{
    LOCK(m_connected_mutex);

    auto it{m_connected.find(node_id)};
    if (it == m_connected.end()) {
        // There is no socket in case we've already disconnected, or in test cases without
        // real connections.
        return {};
    }

    return it->second;
}
