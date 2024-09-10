// Copyright (c) 2014-2017 Statoshi Developers
// Copyright (c) 2020-2024 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
Copyright (c) 2014, Rex
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

* Neither the name of the {organization} nor the names of its
  contributors may be used to endorse or promote products derived from
  this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**/

#include <statsd_client.h>

#include <compat.h>
#include <netbase.h>
#include <util/system.h>

#include <cmath>
#include <cstdio>
#include <random>

std::unique_ptr<statsd::StatsdClient> g_stats_client;

namespace statsd {
bool StatsdClient::ShouldSend(float sample_rate)
{
    sample_rate = std::clamp(sample_rate, 0.f, 1.f);

    constexpr float EPSILON{0.0001f};
    /* If sample rate is 1, we should always send */
    if (std::fabs(sample_rate - 1.f) < EPSILON) return true;
    /* If sample rate is 0, we should never send */
    if (std::fabs(sample_rate) < EPSILON) return false;

    /* Sample rate is >0 and <1, roll the dice */
    LOCK(cs);
    return sample_rate > std::uniform_real_distribution<float>(0.f, 1.f)(insecure_rand);
}

StatsdClient::StatsdClient(const std::string& host, const std::string& nodename, uint16_t port, const std::string& ns,
                           bool enabled)
    : m_port{port}, m_host{host}, m_nodename{nodename}, m_ns{ns}
{
    if (!enabled) {
        LogPrintf("Transmitting stats are disabled, will not init StatsdClient\n");
        return;
    }

    CNetAddr netaddr;
    if (!LookupHost(m_host, netaddr, /*fAllowLookup=*/true)) {
        LogPrintf("ERROR: Unable to lookup host %s, cannot init StatsdClient\n", m_host);
        return;
    }
    if (!netaddr.IsIPv4()) {
        LogPrintf("ERROR: Host %s on unsupported network, cannot init StatsdClient\n", m_host);
        return;
    }
    if (!CService(netaddr, port).GetSockAddr(reinterpret_cast<struct sockaddr*>(&m_server.first), &m_server.second)) {
        LogPrintf("ERROR: Cannot get socket address for %s, cannot init StatsdClient\n", m_host);
        return;
    }

    SOCKET hSocket = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (hSocket == INVALID_SOCKET) {
        LogPrintf("ERROR: Cannot create socket (socket() returned error %s), cannot init StatsdClient\n",
                  NetworkErrorString(WSAGetLastError()));
        return;
    }
    m_sock = std::make_unique<Sock>(hSocket);

    LogPrintf("StatsdClient initialized to transmit stats to %s:%d\n", m_host, m_port);
}

/* will change the original string */
void StatsdClient::cleanup(std::string& key)
{
    auto pos = key.find_first_of(":|@");
    while (pos != std::string::npos)
    {
        key[pos] = '_';
        pos = key.find_first_of(":|@");
    }
}

bool StatsdClient::dec(const std::string& key, float sample_rate)
{
    return count(key, -1, sample_rate);
}

bool StatsdClient::inc(const std::string& key, float sample_rate)
{
    return count(key, 1, sample_rate);
}

bool StatsdClient::count(const std::string& key, int64_t value, float sample_rate)
{
    return send(key, value, "c", sample_rate);
}

bool StatsdClient::gauge(const std::string& key, int64_t value, float sample_rate)
{
    return send(key, value, "g", sample_rate);
}

bool StatsdClient::gaugeDouble(const std::string& key, double value, float sample_rate)
{
    return sendDouble(key, value, "g", sample_rate);
}

bool StatsdClient::timing(const std::string& key, int64_t ms, float sample_rate)
{
    return send(key, ms, "ms", sample_rate);
}

bool StatsdClient::send(std::string key, int64_t value, const std::string& type, float sample_rate)
{
    if (!m_sock) {
        return false;
    }

    if (!ShouldSend(sample_rate)) {
        // Not our turn to send but report that we have
        return true;
    }

    // partition stats by node name if set
    if (!m_nodename.empty())
        key = key + "." + m_nodename;

    cleanup(key);

    std::string buf{strprintf("%s%s:%d|%s", m_ns, key, value, type)};
    if (sample_rate < 1.f) {
        buf += strprintf("|@%.2f", sample_rate);
    }

    return send(buf);
}

bool StatsdClient::sendDouble(std::string key, double value, const std::string& type, float sample_rate)
{
    if (!m_sock) {
        return false;
    }

    if (!ShouldSend(sample_rate)) {
        // Not our turn to send but report that we have
        return true;
    }

    // partition stats by node name if set
    if (!m_nodename.empty())
        key = key + "." + m_nodename;

    cleanup(key);

    std::string buf{strprintf("%s%s:%f|%s", m_ns, key, value, type)};
    if (sample_rate < 1.f) {
        buf += strprintf("|@%.2f", sample_rate);
    }

    return send(buf);
}

bool StatsdClient::send(const std::string& message)
{
    assert(m_sock);

    if (::sendto(m_sock->Get(), message.data(), message.size(), /*flags=*/0,
                 reinterpret_cast<struct sockaddr*>(&m_server.first), m_server.second) == SOCKET_ERROR) {
        LogPrintf("ERROR: Unable to send message (sendto() returned error %s), host=%s:%d\n",
                  NetworkErrorString(WSAGetLastError()), m_host, m_port);
        return false;
    }

    return true;
}
} // namespace statsd
