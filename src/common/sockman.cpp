// Copyright (c) 2024-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit/.

#include <bitcoin-build-config.h> // IWYU pragma: keep

#include <common/sockman.h>
#include <logging.h>
#include <netbase.h>
#include <util/sock.h>
#include <util/thread.h>

// The set of sockets cannot be modified while waiting
// The sleep time needs to be small to avoid new sockets stalling
static constexpr auto SELECT_TIMEOUT{50ms};

/** Get the bind address for a socket as CService. */
static CService GetBindAddress(const Sock& sock)
{
    CService addr_bind;
    struct sockaddr_storage sockaddr_bind;
    socklen_t sockaddr_bind_len = sizeof(sockaddr_bind);
    if (!sock.GetSockName((struct sockaddr*)&sockaddr_bind, &sockaddr_bind_len)) {
        addr_bind.SetSockAddr((const struct sockaddr*)&sockaddr_bind, sockaddr_bind_len);
    } else {
        LogPrintLevel(BCLog::NET, BCLog::Level::Warning, "getsockname failed\n");
    }
    return addr_bind;
}

bool SockMan::BindAndStartListening(const CService& to, bilingual_str& err_msg)
{
    // Create socket for listening for incoming connections
    sockaddr_storage storage;
    socklen_t len{sizeof(storage)};
    if (!to.GetSockAddr(reinterpret_cast<sockaddr*>(&storage), &len)) {
        err_msg = Untranslated(strprintf("Bind address family for %s not supported", to.ToStringAddrPort()));
        return false;
    }

    std::unique_ptr<Sock> sock{CreateSock(to.GetSAFamily(), SOCK_STREAM, IPPROTO_TCP)};
    if (!sock) {
        err_msg = Untranslated(strprintf("Cannot create %s listen socket: %s",
                                         to.ToStringAddrPort(),
                                         NetworkErrorString(WSAGetLastError())));
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
        if (err == WSAEADDRINUSE) {
            err_msg = strprintf(_("Unable to bind to %s on this computer. %s is probably already running."),
                                to.ToStringAddrPort(),
                                CLIENT_NAME);
        } else {
            err_msg = strprintf(_("Unable to bind to %s on this computer (bind returned error %s)"),
                                to.ToStringAddrPort(),
                                NetworkErrorString(err));
        }
        return false;
    }

    // Listen for incoming connections
    if (sock->Listen(SOMAXCONN) == SOCKET_ERROR) {
        err_msg = strprintf(_("Cannot listen on %s: %s"), to.ToStringAddrPort(), NetworkErrorString(WSAGetLastError()));
        return false;
    }

    m_listen.emplace_back(std::move(sock));

    return true;
}

void SockMan::StartSocketsThreads(const Options& options)
{
    m_thread_socket_handler = std::thread(
        &util::TraceThread, options.socket_handler_thread_name, [this] { ThreadSocketHandler(); });
}

void SockMan::JoinSocketsThreads()
{
    if (m_thread_socket_handler.joinable()) {
        m_thread_socket_handler.join();
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

    if (!addr.SetSockAddr(reinterpret_cast<sockaddr*>(&storage), len)) {
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

    const Id id{GetNewId()};

    {
        LOCK(m_connected_mutex);
        m_connected.emplace(id, std::make_shared<ConnectionSockets>(std::move(sock)));
    }

    if (!EventNewConnectionAccepted(id, me, them)) {
        CloseConnection(id);
    }
}

SockMan::Id SockMan::GetNewId()
{
    return m_next_id.fetch_add(1, std::memory_order_relaxed);
}

bool SockMan::CloseConnection(Id id)
{
    LOCK(m_connected_mutex);
    return m_connected.erase(id) > 0;
}

ssize_t SockMan::SendBytes(Id id,
                           std::span<const unsigned char> data,
                           bool will_send_more,
                           std::string& errmsg) const
{
    AssertLockNotHeld(m_connected_mutex);

    if (data.empty()) {
        return 0;
    }

    auto sockets{GetConnectionSockets(id)};
    if (!sockets) {
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
        sockets->mutex,
        return sockets->sock->Send(reinterpret_cast<const char*>(data.data()), data.size(), flags);)};

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

void SockMan::StopListening()
{
    m_listen.clear();
}

bool SockMan::ShouldTryToSend(Id id) const { return true; }

bool SockMan::ShouldTryToRecv(Id id) const { return true; }

void SockMan::EventIOLoopCompletedForOne(Id id) {}

void SockMan::EventIOLoopCompletedForAll() {}

void SockMan::ThreadSocketHandler()
{
    AssertLockNotHeld(m_connected_mutex);

    while (!interruptNet) {
        EventIOLoopCompletedForAll();

        // Check for the readiness of the already connected sockets and the
        // listening sockets in one call ("readiness" as in poll(2) or
        // select(2)). If none are ready, wait for a short while and return
        // empty sets.
        auto io_readiness{GenerateWaitSockets()};
        if (io_readiness.events_per_sock.empty() ||
            // WaitMany() may as well be a static method, the context of the first Sock in the vector is not relevant.
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

SockMan::IOReadiness SockMan::GenerateWaitSockets()
{
    AssertLockNotHeld(m_connected_mutex);

    IOReadiness io_readiness;

    for (const auto& sock : m_listen) {
        io_readiness.events_per_sock.emplace(sock, Sock::Events{Sock::RECV});
    }

    auto connected_snapshot{WITH_LOCK(m_connected_mutex, return m_connected;)};

    for (const auto& [id, sockets] : connected_snapshot) {
        const bool select_recv{ShouldTryToRecv(id)};
        const bool select_send{ShouldTryToSend(id)};
        if (!select_recv && !select_send) continue;

        Sock::Event event = (select_send ? Sock::SEND : 0) | (select_recv ? Sock::RECV : 0);
        io_readiness.events_per_sock.emplace(sockets->sock, Sock::Events{event});
        io_readiness.ids_per_sock.emplace(sockets->sock, id);
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

        auto it{io_readiness.ids_per_sock.find(sock)};
        if (it == io_readiness.ids_per_sock.end()) {
            continue;
        }
        const Id id{it->second};

        bool send_ready = events.occurred & Sock::SEND; // Sock::SEND could only be set if ShouldTryToSend() has returned true in GenerateWaitSockets().
        bool recv_ready = events.occurred & Sock::RECV; // Sock::RECV could only be set if ShouldTryToRecv() has returned true in GenerateWaitSockets().
        bool err_ready = events.occurred & Sock::ERR;

        if (send_ready) {
            bool cancel_recv;

            EventReadyToSend(id, cancel_recv);

            if (cancel_recv) {
                recv_ready = false;
            }
        }

        if (recv_ready || err_ready) {
            uint8_t buf[0x10000]; // typical socket buffer is 8K-64K

            auto sockets{GetConnectionSockets(id)};
            if (!sockets) {
                continue;
            }

            const ssize_t nrecv{WITH_LOCK(
                sockets->mutex,
                return sockets->sock->Recv(buf, sizeof(buf), MSG_DONTWAIT);)};

            if (nrecv < 0) { // In all cases (including -1 and 0) EventIOLoopCompletedForOne() should be executed after this, don't change the code to skip it.
                const int err = WSAGetLastError();
                if (err != WSAEWOULDBLOCK && err != WSAEMSGSIZE && err != WSAEINTR && err != WSAEINPROGRESS) {
                    EventGotPermanentReadError(id, NetworkErrorString(err));
                }
            } else if (nrecv == 0) {
                EventGotEOF(id);
            } else {
                EventGotData(id, {buf, static_cast<size_t>(nrecv)});
            }
        }

        EventIOLoopCompletedForOne(id);
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

std::shared_ptr<SockMan::ConnectionSockets> SockMan::GetConnectionSockets(Id id) const
{
    LOCK(m_connected_mutex);

    auto it{m_connected.find(id)};
    if (it == m_connected.end()) {
        // There is no socket in case we've already disconnected, or in test cases without
        // real connections.
        return {};
    }

    return it->second;
}
