// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit/.

#include <bitcoin-build-config.h> // IWYU pragma: keep

#include <common/sockman.h>
#include <logging.h>
#include <netbase.h>
#include <util/sock.h>

util::Result<void> SockMan::BindAndStartListening(const CService& to)
{
    // Create socket for listening for incoming connections
    sockaddr_storage storage;
    socklen_t len{sizeof(storage)};
    if (!to.GetSockAddr(reinterpret_cast<sockaddr*>(&storage), &len)) {
        return util::Error{Untranslated(strprintf("Bind address family for %s not supported", to.ToStringAddrPort()))};
    }

    std::unique_ptr<Sock> sock{CreateSock(to.GetSAFamily(), SOCK_STREAM, IPPROTO_TCP)};
    if (!sock) {
        return util::Error{Untranslated(strprintf("Cannot create %s listen socket: %s",
                                                    to.ToStringAddrPort(),
                                                    NetworkErrorString(WSAGetLastError())))};
    }

    int socket_option_true{1};

    // Allow binding if the port is still in TIME_WAIT state after
    // the program was closed and restarted.
    if (sock->SetSockOpt(SOL_SOCKET, SO_REUSEADDR, &socket_option_true, sizeof(socket_option_true)) == SOCKET_ERROR) {
        LogPrintLevel(BCLog::NET,
                      BCLog::Level::Info,
                      "Cannot set SO_REUSEADDR on %s listen socket: %s, continuing anyway",
                      to.ToStringAddrPort(),
                      NetworkErrorString(WSAGetLastError()));
    }

    // some systems don't have IPV6_V6ONLY but are always v6only; others do have the option
    // and enable it by default or not. Try to enable it, if possible.
    if (to.IsIPv6()) {
#ifdef IPV6_V6ONLY
        if (sock->SetSockOpt(IPPROTO_IPV6, IPV6_V6ONLY, &socket_option_true, sizeof(socket_option_true)) == SOCKET_ERROR) {
            LogPrintLevel(BCLog::NET,
                          BCLog::Level::Info,
                          "Cannot set IPV6_V6ONLY on %s listen socket: %s, continuing anyway",
                          to.ToStringAddrPort(),
                          NetworkErrorString(WSAGetLastError()));
        }
#endif
#ifdef WIN32
        int prot_level{PROTECTION_LEVEL_UNRESTRICTED};
        if (sock->SetSockOpt(IPPROTO_IPV6,
                             IPV6_PROTECTION_LEVEL,
                             &prot_level,
                             sizeof(prot_level)) == SOCKET_ERROR) {
            LogPrintLevel(BCLog::NET,
                          BCLog::Level::Info,
                          "Cannot set IPV6_PROTECTION_LEVEL on %s listen socket: %s, continuing anyway",
                          to.ToStringAddrPort(),
                          NetworkErrorString(WSAGetLastError()));
        }
#endif
    }

    if (sock->Bind(reinterpret_cast<sockaddr*>(&storage), len) == SOCKET_ERROR) {
        const int err{WSAGetLastError()};
        if (err == WSAEADDRINUSE) {
            return util::Error{strprintf(_("Unable to bind to %s on this computer. %s is probably already running."),
                                            to.ToStringAddrPort(),
                                            CLIENT_NAME)};
        } else {
            return util::Error{strprintf(_("Unable to bind to %s on this computer (bind returned error %s)"),
                                            to.ToStringAddrPort(),
                                            NetworkErrorString(err))};
        }
    }

    // Listen for incoming connections
    if (sock->Listen(SOMAXCONN) == SOCKET_ERROR) {
        return util::Error{strprintf(_("Cannot listen on %s: %s"),
                                        to.ToStringAddrPort(),
                                        NetworkErrorString(WSAGetLastError()))};
    }

    m_listen.emplace_back(std::move(sock));

    return {};
}

void SockMan::StopListening()
{
    m_listen.clear();
}
