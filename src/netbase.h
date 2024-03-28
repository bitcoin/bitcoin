// Copyright (c) 2009-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NETBASE_H
#define BITCOIN_NETBASE_H

#include <compat/compat.h>
#include <netaddress.h>
#include <serialize.h>
#include <util/sock.h>
#include <util/threadinterrupt.h>

#include <functional>
#include <memory>
#include <stdint.h>
#include <string>
#include <type_traits>
#include <unordered_set>
#include <vector>

extern int nConnectTimeout;
extern bool fNameLookup;

//! -timeout default
static const int DEFAULT_CONNECT_TIMEOUT = 5000;
//! -dns default
static const int DEFAULT_NAME_LOOKUP = true;

/** Prefix for unix domain socket addresses (which are local filesystem paths) */
const std::string ADDR_PREFIX_UNIX = "unix:";

enum class ConnectionDirection {
    None = 0,
    In = (1U << 0),
    Out = (1U << 1),
    Both = (In | Out),
};
static inline ConnectionDirection& operator|=(ConnectionDirection& a, ConnectionDirection b) {
    using underlying = typename std::underlying_type<ConnectionDirection>::type;
    a = ConnectionDirection(underlying(a) | underlying(b));
    return a;
}
static inline bool operator&(ConnectionDirection a, ConnectionDirection b) {
    using underlying = typename std::underlying_type<ConnectionDirection>::type;
    return (underlying(a) & underlying(b));
}

/**
 * Check if a string is a valid UNIX domain socket path
 *
 * @param      name     The string provided by the user representing a local path
 *
 * @returns Whether the string has proper format, length, and points to an existing file path
 */
bool IsUnixSocketPath(const std::string& name);

class Proxy
{
public:
    Proxy() : m_is_unix_socket(false), m_randomize_credentials(false) {}
    explicit Proxy(const CService& _proxy, bool _randomize_credentials = false) : proxy(_proxy), m_is_unix_socket(false), m_randomize_credentials(_randomize_credentials) {}
    explicit Proxy(const std::string path, bool _randomize_credentials = false) : m_unix_socket_path(path), m_is_unix_socket(true), m_randomize_credentials(_randomize_credentials) {}

    CService proxy;
    std::string m_unix_socket_path;
    bool m_is_unix_socket;
    bool m_randomize_credentials;

    bool IsValid() const
    {
        if (m_is_unix_socket) return IsUnixSocketPath(m_unix_socket_path);
        return proxy.IsValid();
    }

    sa_family_t GetFamily() const
    {
        if (m_is_unix_socket) return AF_UNIX;
        return proxy.GetSAFamily();
    }

    std::string ToString() const
    {
        if (m_is_unix_socket) return m_unix_socket_path;
        return proxy.ToStringAddrPort();
    }

    std::unique_ptr<Sock> Connect() const;
};

/** Credentials for proxy authentication */
struct ProxyCredentials
{
    std::string username;
    std::string password;
};

/**
 * List of reachable networks. Everything is reachable by default.
 */
class ReachableNets {
public:
    void Add(Network net) EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
    {
        AssertLockNotHeld(m_mutex);
        LOCK(m_mutex);
        m_reachable.insert(net);
    }

    void Remove(Network net) EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
    {
        AssertLockNotHeld(m_mutex);
        LOCK(m_mutex);
        m_reachable.erase(net);
    }

    void RemoveAll() EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
    {
        AssertLockNotHeld(m_mutex);
        LOCK(m_mutex);
        m_reachable.clear();
    }

    [[nodiscard]] bool Contains(Network net) const EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
    {
        AssertLockNotHeld(m_mutex);
        LOCK(m_mutex);
        return m_reachable.count(net) > 0;
    }

    [[nodiscard]] bool Contains(const CNetAddr& addr) const EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
    {
        AssertLockNotHeld(m_mutex);
        return Contains(addr.GetNetwork());
    }

    [[nodiscard]] std::unordered_set<Network> All() const EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
    {
        AssertLockNotHeld(m_mutex);
        LOCK(m_mutex);
        return m_reachable;
    }

private:
    mutable Mutex m_mutex;

    std::unordered_set<Network> m_reachable GUARDED_BY(m_mutex){
        NET_UNROUTABLE,
        NET_IPV4,
        NET_IPV6,
        NET_ONION,
        NET_I2P,
        NET_CJDNS,
        NET_INTERNAL
    };
};

extern ReachableNets g_reachable_nets;

/**
 * Wrapper for getaddrinfo(3). Do not use directly: call Lookup/LookupHost/LookupNumeric/LookupSubNet.
 */
std::vector<CNetAddr> WrappedGetAddrInfo(const std::string& name, bool allow_lookup);

enum Network ParseNetwork(const std::string& net);
std::string GetNetworkName(enum Network net);
/** Return a vector of publicly routable Network names; optionally append NET_UNROUTABLE. */
std::vector<std::string> GetNetworkNames(bool append_unroutable = false);
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

using DNSLookupFn = std::function<std::vector<CNetAddr>(const std::string&, bool)>;
extern DNSLookupFn g_dns_lookup;

/**
 * Resolve a host string to its corresponding network addresses.
 *
 * @param name    The string representing a host. Could be a name or a numerical
 *                IP address (IPv6 addresses in their bracketed form are
 *                allowed).
 *
 * @returns The resulting network addresses to which the specified host
 *          string resolved.
 *
 * @see Lookup(const std::string&, uint16_t, bool, unsigned int, DNSLookupFn)
 *      for additional parameter descriptions.
 */
std::vector<CNetAddr> LookupHost(const std::string& name, unsigned int nMaxSolutions, bool fAllowLookup, DNSLookupFn dns_lookup_function = g_dns_lookup);

/**
 * Resolve a host string to its first corresponding network address.
 *
 * @returns The resulting network address to which the specified host
 *          string resolved or std::nullopt if host does not resolve to an address.
 *
 * @see LookupHost(const std::string&, unsigned int, bool, DNSLookupFn)
 *      for additional parameter descriptions.
 */
std::optional<CNetAddr> LookupHost(const std::string& name, bool fAllowLookup, DNSLookupFn dns_lookup_function = g_dns_lookup);

/**
 * Resolve a service string to its corresponding service.
 *
 * @param name    The string representing a service. Could be a name or a
 *                numerical IP address (IPv6 addresses should be in their
 *                disambiguated bracketed form), optionally followed by a uint16_t port
 *                number. (e.g. example.com:8333 or
 *                [2001:db8:85a3:8d3:1319:8a2e:370:7348]:420)
 * @param portDefault The default port for resulting services if not specified
 *                    by the service string.
 * @param fAllowLookup Whether or not hostname lookups are permitted. If yes,
 *                     external queries may be performed.
 * @param nMaxSolutions The maximum number of results we want, specifying 0
 *                      means "as many solutions as we get."
 *
 * @returns The resulting services to which the specified service string
 *          resolved.
 */
std::vector<CService> Lookup(const std::string& name, uint16_t portDefault, bool fAllowLookup, unsigned int nMaxSolutions, DNSLookupFn dns_lookup_function = g_dns_lookup);

/**
 * Resolve a service string to its first corresponding service.
 *
 * @see Lookup(const std::string&, uint16_t, bool, unsigned int, DNSLookupFn)
 *      for additional parameter descriptions.
 */
std::optional<CService> Lookup(const std::string& name, uint16_t portDefault, bool fAllowLookup, DNSLookupFn dns_lookup_function = g_dns_lookup);

/**
 * Resolve a service string with a numeric IP to its first corresponding
 * service.
 *
 * @returns The resulting CService if the resolution was successful, [::]:0 otherwise.
 *
 * @see Lookup(const std::string&, uint16_t, bool, unsigned int, DNSLookupFn)
 *      for additional parameter descriptions.
 */
CService LookupNumeric(const std::string& name, uint16_t portDefault = 0, DNSLookupFn dns_lookup_function = g_dns_lookup);

/**
 * Parse and resolve a specified subnet string into the appropriate internal
 * representation.
 *
 * @param[in]  subnet_str  A string representation of a subnet of the form
 *                         `network address [ "/", ( CIDR-style suffix | netmask ) ]`
 *                         e.g. "2001:db8::/32", "192.0.2.0/255.255.255.0" or "8.8.8.8".
 * @returns a CSubNet object (that may or may not be valid).
 */
CSubNet LookupSubNet(const std::string& subnet_str);

/**
 * Create a TCP or UNIX socket in the given address family.
 * @param[in] address_family to use for the socket.
 * @return pointer to the created Sock object or unique_ptr that owns nothing in case of failure
 */
std::unique_ptr<Sock> CreateSockOS(sa_family_t address_family);

/**
 * Socket factory. Defaults to `CreateSockOS()`, but can be overridden by unit tests.
 */
extern std::function<std::unique_ptr<Sock>(const sa_family_t&)> CreateSock;

/**
 * Create a socket and try to connect to the specified service.
 *
 * @param[in] dest The service to which to connect.
 * @param[in] manual_connection Whether or not the connection was manually requested (e.g. through the addnode RPC)
 *
 * @returns the connected socket if the operation succeeded, empty unique_ptr otherwise
 */
std::unique_ptr<Sock> ConnectDirectly(const CService& dest, bool manual_connection);

/**
 * Connect to a specified destination service through a SOCKS5 proxy by first
 * connecting to the SOCKS5 proxy.
 *
 * @param[in] proxy The SOCKS5 proxy.
 * @param[in] dest The destination service to which to connect.
 * @param[in] port The destination port.
 * @param[out] proxy_connection_failed Whether or not the connection to the SOCKS5 proxy failed.
 *
 * @returns the connected socket if the operation succeeded. Otherwise an empty unique_ptr.
 */
std::unique_ptr<Sock> ConnectThroughProxy(const Proxy& proxy,
                                          const std::string& dest,
                                          uint16_t port,
                                          bool& proxy_connection_failed);

/**
 * Interrupt SOCKS5 reads or writes.
 */
extern CThreadInterrupt g_socks5_interrupt;

/**
 * Connect to a specified destination service through an already connected
 * SOCKS5 proxy.
 *
 * @param strDest The destination fully-qualified domain name.
 * @param port The destination port.
 * @param auth The credentials with which to authenticate with the specified
 *             SOCKS5 proxy.
 * @param socket The SOCKS5 proxy socket.
 *
 * @returns Whether or not the operation succeeded.
 *
 * @note The specified SOCKS5 proxy socket must already be connected to the
 *       SOCKS5 proxy.
 *
 * @see <a href="https://www.ietf.org/rfc/rfc1928.txt">RFC1928: SOCKS Protocol
 *      Version 5</a>
 */
bool Socks5(const std::string& strDest, uint16_t port, const ProxyCredentials* auth, const Sock& socket);

/**
 * Determine if a port is "bad" from the perspective of attempting to connect
 * to a node on that port.
 * @see doc/p2p-bad-ports.md
 * @param[in] port Port to check.
 * @returns whether the port is bad
 */
bool IsBadPort(uint16_t port);

/**
 * If an IPv6 address belongs to the address range used by the CJDNS network and
 * the CJDNS network is reachable (-cjdnsreachable config is set), then change
 * the type from NET_IPV6 to NET_CJDNS.
 * @param[in] service Address to potentially convert.
 * @return a copy of `service` either unmodified or changed to CJDNS.
 */
CService MaybeFlipIPv6toCJDNS(const CService& service);

#endif // BITCOIN_NETBASE_H
