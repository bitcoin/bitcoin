
#include <math.h>
#include <netdb.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <netinet/in.h>
#include "statsd_client.h"
#include "util.h"
#include <fcntl.h>

using namespace std;
namespace statsd {

inline bool fequal(float a, float b)
{
    const float epsilon = 0.0001;
    return ( fabs(a - b) < epsilon );
}

inline bool should_send(float sample_rate)
{
    if ( fequal(sample_rate, 1.0) )
    {
        return true;
    }

    float p = ((float)random() / RAND_MAX);
    return sample_rate > p;
}

struct _StatsdClientData {
    int     sock;
    struct  sockaddr_in server;

    string  ns;
    string  host;
    string  nodename;
    short   port;
    bool    init;

    char    errmsg[1024];
};

StatsdClient::StatsdClient(const string& host, int port, const string& ns)
{
    d = new _StatsdClientData;
    d->sock = -1;
    config(host, port, ns);
    srandom(time(NULL));
}

StatsdClient::~StatsdClient()
{
    // close socket
    if (d->sock >= 0) {
        close(d->sock);
        d->sock = -1;
        delete d;
        d = NULL;
    }
}

void StatsdClient::config(const string& host, int port, const string& ns)
{
    d->ns = ns;
    d->host = host;
    d->port = port;
    d->init = false;
    if ( d->sock >= 0 ) {
        close(d->sock);
    }
    d->sock = -1;
}

int StatsdClient::init()
{
    if ( d->init ) return 0;

    d->sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if ( d->sock == -1 ) {
        snprintf(d->errmsg, sizeof(d->errmsg), "could not create socket, err=%m");
        return -1;
    }

    memset(&d->server, 0, sizeof(d->server));
    d->server.sin_family = AF_INET;
    d->server.sin_port = htons(d->port);

    int ret = inet_aton(d->host.c_str(), &d->server.sin_addr);
    if ( ret == 0 )
    {
        // host must be a domain, get it from internet
        struct addrinfo hints, *result = NULL;
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_DGRAM;

        ret = getaddrinfo(d->host.c_str(), NULL, &hints, &result);
        if ( ret ) {
            snprintf(d->errmsg, sizeof(d->errmsg),
                    "getaddrinfo fail, error=%d, msg=%s", ret, gai_strerror(ret) );
            return -2;
        }
        struct sockaddr_in* host_addr = (struct sockaddr_in*)result->ai_addr;
        memcpy(&d->server.sin_addr, &host_addr->sin_addr, sizeof(struct in_addr));
        freeaddrinfo(result);
    }

    string name = GetArg("-statshostname", "");
    if (!name.empty())
        d->nodename = name;

    d->init = true;
    return 0;
}

/* will change the original string */
void StatsdClient::cleanup(string& key)
{
    size_t pos = key.find_first_of(":|@");
    while ( pos != string::npos )
    {
        key[pos] = '_';
        pos = key.find_first_of(":|@");
    }
}

int StatsdClient::dec(const string& key, float sample_rate)
{
    return count(key, -1, sample_rate);
}

int StatsdClient::inc(const string& key, float sample_rate)
{
    return count(key, 1, sample_rate);
}

int StatsdClient::count(const string& key, size_t value, float sample_rate)
{
    return send(key, value, "c", sample_rate);
}

int StatsdClient::gauge(const string& key, size_t value, float sample_rate)
{
    return send(key, value, "g", sample_rate);
}

int StatsdClient::timing(const string& key, size_t ms, float sample_rate)
{
    return send(key, ms, "ms", sample_rate);
}

int StatsdClient::send(string key, size_t value, const string &type, float sample_rate)
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

int StatsdClient::send(const string &message)
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

}

