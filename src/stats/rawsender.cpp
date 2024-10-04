// Copyright (c) 2017-2023 Vincent Thiery
// Copyright (c) 2024 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <stats/rawsender.h>

#include <netaddress.h>
#include <netbase.h>
#include <util/sock.h>

RawSender::RawSender(const std::string& host, uint16_t port, std::optional<std::string>& error) :
    m_host{host},
    m_port{port}
{
    if (host.empty()) {
        error = "No host specified";
        return;
    }

    if (auto netaddr = LookupHost(m_host, /*fAllowLookup=*/true); netaddr.has_value()) {
        if (!netaddr->IsIPv4()) {
            error = strprintf("Host %s on unsupported network", m_host);
            return;
        }
        if (!CService(*netaddr, port).GetSockAddr(reinterpret_cast<struct sockaddr*>(&m_server.first), &m_server.second)) {
            error = strprintf("Cannot get socket address for %s", m_host);
            return;
        }
    } else {
        error = strprintf("Unable to lookup host %s", m_host);
        return;
    }

    SOCKET hSocket = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (hSocket == INVALID_SOCKET) {
        error = strprintf("Cannot create socket (socket() returned error %s)", NetworkErrorString(WSAGetLastError()));
        return;
    }
    m_sock = std::make_unique<Sock>(hSocket);

    LogPrintf("Started RawSender sending messages to %s:%d\n", m_host, m_port);
}

RawSender::~RawSender()
{
    LogPrintf("Stopping RawSender instance sending messages to %s:%d. %d successes, %d failures.\n",
              m_host, m_port, m_successes, m_failures);
}

std::optional<std::string> RawSender::Send(const RawMessage& msg)
{
    if (!m_sock) {
        m_failures++;
        return "Socket not initialized, cannot send message";
    }

    if (::sendto(m_sock->Get(), reinterpret_cast<const char*>(msg.data()),
#ifdef WIN32
                 static_cast<int>(msg.size()),
#else
                 msg.size(),
#endif // WIN32
                 /*flags=*/0, reinterpret_cast<struct sockaddr*>(&m_server.first), m_server.second) == SOCKET_ERROR) {
        m_failures++;
        return strprintf("Unable to send message to %s (sendto() returned error %s)", this->ToStringHostPort(),
                         NetworkErrorString(WSAGetLastError()));
    }

    m_successes++;
    return std::nullopt;
}

std::string RawSender::ToStringHostPort() const { return strprintf("%s:%d", m_host, m_port); }
