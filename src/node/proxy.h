#ifndef BITCOIN_NODE_PROXY_H
#define BITCOIN_NODE_PROXY_H

#include <netaddress.h>

#include <string>

class Proxy
{
public:
    Proxy() : randomize_credentials(false) {}
    explicit Proxy(const CService& _proxy, bool _randomize_credentials = false) : proxy(_proxy), randomize_credentials(_randomize_credentials) {}

    bool IsValid() const { return proxy.IsValid(); }

    CService proxy;
    bool randomize_credentials;
};

/** Credentials for proxy authentication */
struct ProxyCredentials {
    std::string username;
    std::string password;
};

bool SetProxy(enum Network net, const Proxy &addrProxy);
bool GetProxy(enum Network net, Proxy &proxyInfoOut);
bool IsProxy(const CNetAddr &addr);
/**
 * Set the name proxy to use for all connections to nodes specified by a
 * hostname. After setting this proxy, connecting to a node specified by a
 * hostname won't result in a local lookup of said hostname, rather, connect to
 * the node by asking the name proxy for a proxy connection to the hostname,
 * effectively delegating the hostname lookup to the specified proxy.
 *
 * This delegation increases privacy for those who set the name proxy as they no
 * longer leak their external hostname queries to their DNS servers.
 *
 * @returns Whether or not the operation succeeded.
 *
 * @note SOCKS5's support for UDP-over-SOCKS5 has been considered, but no SOCK5
 *       server in common use (most notably Tor) actually implements UDP
 *       support, and a DNS resolver is beyond the scope of this project.
 */
bool SetNameProxy(const Proxy &addrProxy);
bool HaveNameProxy();
bool GetNameProxy(Proxy &nameProxyOut);

#endif // BITCOIN_NODE_PROXY_H
