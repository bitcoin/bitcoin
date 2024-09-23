// Copyright (c) 2024-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit/.

#include <bitcoin-build-config.h> // IWYU pragma: keep

#include <common/sockman.h>
#include <logging.h>
#include <netbase.h>
#include <util/sock.h>
#include <util/thread.h>

CService GetBindAddress(const Sock& sock)
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
                               ? std::string{" ("} + CLIENT_NAME + " already running?)"
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
}

std::optional<NodeId>
SockMan::ConnectAndMakeNodeId(const std::variant<CService, StringHostIntPort>& to,
                              bool is_important,
                              const Proxy& proxy,
                              bool& proxy_failed,
                              CService& me,
                              std::unique_ptr<Sock>& sock,
                              std::unique_ptr<i2p::sam::Session>& i2p_transient_session)
{
    AssertLockNotHeld(m_unused_i2p_sessions_mutex);

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

    return node_id;
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

NodeId SockMan::GetNewNodeId()
{
    return m_next_node_id.fetch_add(1, std::memory_order_relaxed);
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

void SockMan::ThreadI2PAccept()
{
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

        EventNewConnectionAccepted(std::move(conn.sock), conn.me, conn.peer);

        err_wait = err_wait_begin;
    }
}
