// Copyright (c) 2024-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit/.

#include <bitcoin-build-config.h> // IWYU pragma: keep

#include <common/sockman.h>
#include <logging.h>
#include <netbase.h>
#include <util/sock.h>

bool SockMan::BindListenPort(const CService& addrBind, bilingual_str& strError)
{
    int nOne = 1;

    // Create socket for listening for incoming connections
    struct sockaddr_storage sockaddr;
    socklen_t len = sizeof(sockaddr);
    if (!addrBind.GetSockAddr((struct sockaddr*)&sockaddr, &len))
    {
        strError = Untranslated(strprintf("Bind address family for %s not supported", addrBind.ToStringAddrPort()));
        LogPrintLevel(BCLog::NET, BCLog::Level::Error, "%s\n", strError.original);
        return false;
    }

    std::unique_ptr<Sock> sock = CreateSock(addrBind.GetSAFamily(), SOCK_STREAM, IPPROTO_TCP);
    if (!sock) {
        strError = Untranslated(strprintf("Couldn't open socket for incoming connections (socket returned error %s)", NetworkErrorString(WSAGetLastError())));
        LogPrintLevel(BCLog::NET, BCLog::Level::Error, "%s\n", strError.original);
        return false;
    }

    // Allow binding if the port is still in TIME_WAIT state after
    // the program was closed and restarted.
    if (sock->SetSockOpt(SOL_SOCKET, SO_REUSEADDR, (sockopt_arg_type)&nOne, sizeof(int)) == SOCKET_ERROR) {
        strError = Untranslated(strprintf("Error setting SO_REUSEADDR on socket: %s, continuing anyway", NetworkErrorString(WSAGetLastError())));
        LogPrintf("%s\n", strError.original);
    }

    // some systems don't have IPV6_V6ONLY but are always v6only; others do have the option
    // and enable it by default or not. Try to enable it, if possible.
    if (addrBind.IsIPv6()) {
#ifdef IPV6_V6ONLY
        if (sock->SetSockOpt(IPPROTO_IPV6, IPV6_V6ONLY, (sockopt_arg_type)&nOne, sizeof(int)) == SOCKET_ERROR) {
            strError = Untranslated(strprintf("Error setting IPV6_V6ONLY on socket: %s, continuing anyway", NetworkErrorString(WSAGetLastError())));
            LogPrintf("%s\n", strError.original);
        }
#endif
#ifdef WIN32
        int nProtLevel = PROTECTION_LEVEL_UNRESTRICTED;
        if (sock->SetSockOpt(IPPROTO_IPV6, IPV6_PROTECTION_LEVEL, (const char*)&nProtLevel, sizeof(int)) == SOCKET_ERROR) {
            strError = Untranslated(strprintf("Error setting IPV6_PROTECTION_LEVEL on socket: %s, continuing anyway", NetworkErrorString(WSAGetLastError())));
            LogPrintf("%s\n", strError.original);
        }
#endif
    }

    if (sock->Bind(reinterpret_cast<struct sockaddr*>(&sockaddr), len) == SOCKET_ERROR) {
        int nErr = WSAGetLastError();
        if (nErr == WSAEADDRINUSE)
            strError = strprintf(_("Unable to bind to %s on this computer. %s is probably already running."), addrBind.ToStringAddrPort(), CLIENT_NAME);
        else
            strError = strprintf(_("Unable to bind to %s on this computer (bind returned error %s)"), addrBind.ToStringAddrPort(), NetworkErrorString(nErr));
        LogPrintLevel(BCLog::NET, BCLog::Level::Error, "%s\n", strError.original);
        return false;
    }
    LogPrintf("Bound to %s\n", addrBind.ToStringAddrPort());

    // Listen for incoming connections
    if (sock->Listen(SOMAXCONN) == SOCKET_ERROR)
    {
        strError = strprintf(_("Listening for incoming connections failed (listen returned error %s)"), NetworkErrorString(WSAGetLastError()));
        LogPrintLevel(BCLog::NET, BCLog::Level::Error, "%s\n", strError.original);
        return false;
    }

    m_listen.emplace_back(std::move(sock));

    return true;
}

void SockMan::StopListening()
{
    m_listen.clear();
}
