#ifndef BITCOIN_NODE_PROXY_H
#define BITCOIN_NODE_PROXY_H

#include <netaddress.h>
#include <sync.h>

#include <array>
#include <optional>
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

class ProxyManager
{
public:
    bool SetProxy(enum Network net, const Proxy& proxy)
        EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);
    std::optional<Proxy> GetProxy(enum Network net) const
        EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);
    bool HasProxy(const CNetAddr& addr) const
        EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);

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
    bool SetNameProxy(const Proxy& addrProxy)
        EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);
    bool HaveNameProxy() const
        EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);
    std::optional<Proxy> GetNameProxy() const
        EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);

private:
    mutable Mutex m_mutex;

    std::array<std::optional<Proxy>, NET_MAX> m_proxies GUARDED_BY(m_mutex);
    std::optional<Proxy> m_name_proxy GUARDED_BY(m_mutex);
};

#endif // BITCOIN_NODE_PROXY_H
