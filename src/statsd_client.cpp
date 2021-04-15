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
#include <random.h>
#include <util.h>

#include <cmath>
#include <cstdio>

statsd::StatsdClient statsClient;

namespace statsd {

inline bool fequal(float a, float b)
{
    const float epsilon = 0.0001;
    return ( fabs(a - b) < epsilon );
}

thread_local FastRandomContext insecure_rand;

inline bool should_send(float sample_rate)
{
    if ( fequal(sample_rate, 1.0) )
    {
        return true;
    }

    float p = ((float)insecure_rand(std::numeric_limits<int>::max()) / std::numeric_limits<int>::max());
    return sample_rate > p;
}

struct _StatsdClientData {
    SOCKET  sock;
    struct  sockaddr_in server;

    std::string  ns;
    std::string  host;
    std::string  nodename;
    short   port;
    bool    init;

    char    errmsg[1024];
};

StatsdClient::StatsdClient(const std::string& host, int port, const std::string& ns)
{
    d = new _StatsdClientData;
    d->sock = INVALID_SOCKET;
    config(host, port, ns);
}

StatsdClient::~StatsdClient()
{
    // close socket
    CloseSocket(d->sock);
    delete d;
    d = nullptr;
}

void StatsdClient::config(const std::string& host, int port, const std::string& ns)
{
    d->ns = ns;
    d->host = host;
    d->port = port;
    d->init = false;
    CloseSocket(d->sock);
}

int StatsdClient::init()
{
    static bool fEnabled = gArgs.GetBoolArg("-statsenabled", DEFAULT_STATSD_ENABLE);
    if (!fEnabled) return -3;

    if ( d->init ) return 0;

    config(gArgs.GetArg("-statshost", DEFAULT_STATSD_HOST), gArgs.GetArg("-statsport", DEFAULT_STATSD_PORT), gArgs.GetArg("-statsns", DEFAULT_STATSD_NAMESPACE));

    d->sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if ( d->sock == INVALID_SOCKET ) {
        snprintf(d->errmsg, sizeof(d->errmsg), "could not create socket, err=%m");
        return -1;
    }

    memset(&d->server, 0, sizeof(d->server));
    d->server.sin_family = AF_INET;
    d->server.sin_port = htons(d->port);

    CNetAddr netaddr(d->server.sin_addr);
    if (!LookupHost(d->host.c_str(), netaddr, true) || !netaddr.GetInAddr(&d->server.sin_addr)) {
        snprintf(d->errmsg, sizeof(d->errmsg), "LookupHost or GetInAddr failed");
        return -2;
    }

    if (gArgs.IsArgSet("-statshostname")) {
        d->nodename = gArgs.GetArg("-statshostname", DEFAULT_STATSD_HOSTNAME);
    }

    d->init = true;
    return 0;
}

/* will change the original string */
void StatsdClient::cleanup(std::string& key)
{
    size_t pos = key.find_first_of(":|@");
    while ( pos != std::string::npos )
    {
        key[pos] = '_';
        pos = key.find_first_of(":|@");
    }
}

int StatsdClient::dec(const std::string& key, float sample_rate)
{
    return count(key, -1, sample_rate);
}

int StatsdClient::inc(const std::string& key, float sample_rate)
{
    return count(key, 1, sample_rate);
}

int StatsdClient::count(const std::string& key, size_t value, float sample_rate)
{
    return send(key, value, "c", sample_rate);
}

int StatsdClient::gauge(const std::string& key, size_t value, float sample_rate)
{
    return send(key, value, "g", sample_rate);
}

int StatsdClient::gaugeDouble(const std::string& key, double value, float sample_rate)
{
    return sendDouble(key, value, "g", sample_rate);
}

int StatsdClient::timing(const std::string& key, size_t ms, float sample_rate)
{
    return send(key, ms, "ms", sample_rate);
}

int StatsdClient::send(std::string key, size_t value, const std::string& type, float sample_rate)
{
    if (!should_send(sample_rate)) {
        return 0;
    }

    // partition stats by node name if set
    if (!d->nodename.empty())
        key = key + "." + d->nodename;

    cleanup(key);

    char buf[256];
    if ( fequal( sample_rate, 1.0 ) )
    {
        snprintf(buf, sizeof(buf), "%s%s:%zd|%s",
                d->ns.c_str(), key.c_str(), value, type.c_str());
    }
    else
    {
        snprintf(buf, sizeof(buf), "%s%s:%zd|%s|@%.2f",
                d->ns.c_str(), key.c_str(), value, type.c_str(), sample_rate);
    }

    return send(buf);
}

int StatsdClient::sendDouble(std::string key, double value, const std::string& type, float sample_rate)
{
    if (!should_send(sample_rate)) {
        return 0;
    }

    // partition stats by node name if set
    if (!d->nodename.empty())
        key = key + "." + d->nodename;

    cleanup(key);

    char buf[256];
    if ( fequal( sample_rate, 1.0 ) )
    {
        snprintf(buf, sizeof(buf), "%s%s:%f|%s",
                d->ns.c_str(), key.c_str(), value, type.c_str());
    }
    else
    {
        snprintf(buf, sizeof(buf), "%s%s:%f|%s|@%.2f",
                d->ns.c_str(), key.c_str(), value, type.c_str(), sample_rate);
    }

    return send(buf);
}

int StatsdClient::send(const std::string& message)
{
    int ret = init();
    if ( ret )
    {
        return ret;
    }
    ret = sendto(d->sock, message.data(), message.size(), 0, (struct sockaddr *) &d->server, sizeof(d->server));
    if ( ret == -1) {
        snprintf(d->errmsg, sizeof(d->errmsg),
                "sendto server fail, host=%s:%d, err=%m", d->host.c_str(), d->port);
        return -1;
    }
    return 0;
}

const char* StatsdClient::errmsg()
{
    return d->errmsg;
}

} // namespace statsd
