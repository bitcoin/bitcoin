// Copyright (c) 2009-2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NETBASE_H
#define BITCOIN_NETBASE_H

#if defined(HAVE_CONFIG_H)
#include <config/bitcoin-config.h>
#endif

#include <compat.h>
#include <netaddress.h>
#include <serialize.h>
#include <util/sock.h>

#include <functional>
#include <memory>
#include <stdint.h>
#include <string>
#include <type_traits>
#include <vector>

extern int nConnectTimeout;
extern bool fNameLookup;

//! -timeout default
static const int DEFAULT_CONNECT_TIMEOUT = 5000;
//! -dns default
static const int DEFAULT_NAME_LOOKUP = true;

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

class proxyType
{
public:
    proxyType(): randomize_credentials(false) {}
    explicit proxyType(const CService &_proxy, bool _randomize_credentials=false): proxy(_proxy), randomize_credentials(_randomize_credentials) {}

    bool IsValid() const { return proxy.IsValid(); }

    CService proxy;
    bool randomize_credentials;
};

/** Credentials for proxy authentication */
struct ProxyCredentials
{
    std::string username;
    std::string password;
};

/**
 * Wrapper for getaddrinfo(3). Do not use directly: call Lookup/LookupHost/LookupNumeric/LookupSubNet.
 */
std::vector<CNetAddr> WrappedGetAddrInfo(const std::string& name, bool allow_lookup);

enum Network ParseNetwork(const std::string& net);
std::string GetNetworkName(enum Network net);
/** Return a vector of publicly routable Network names; optionally append NET_UNROUTABLE. */
std::vector<std::string> GetNetworkNames(bool append_unroutable = false);
bool SetProxy(enum Network net, const proxyType &addrProxy);
bool GetProxy(enum Network net, proxyType &proxyInfoOut);
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
bool SetNameProxy(const proxyType &addrProxy);
bool HaveNameProxy();
bool GetNameProxy(proxyType &nameProxyOut);

using DNSLookupFn = std::function<std::vector<CNetAddr>(const std::string&, bool)>;
extern DNSLookupFn g_dns_lookup;

/**
 * Resolve a host string to its corresponding network addresses.
 *
 * @param name    The string representing a host. Could be a name or a numerical
 *                IP address (IPv6 addresses in their bracketed form are
 *                allowed).
 * @param[out] vIP The resulting network addresses to which the specified host
 *                 string resolved.
 *
 * @returns Whether or not the specified host string successfully resolved to
 *          any resulting network addresses.
 *
 * @see Lookup(const std::string&, std::vector<CService>&, uint16_t, bool, unsigned int, DNSLookupFn)
 *      for additional parameter descriptions.
 */
bool LookupHost(const std::string& name, std::vector<CNetAddr>& vIP, unsigned int nMaxSolutions, bool fAllowLookup, DNSLookupFn dns_lookup_function = g_dns_lookup);

/**
 * Resolve a host string to its first corresponding network address.
 *
 * @see LookupHost(const std::string&, std::vector<CNetAddr>&, uint16_t, bool, DNSLookupFn)
 *      for additional parameter descriptions.
 */
bool LookupHost(const std::string& name, CNetAddr& addr, bool fAllowLookup, DNSLookupFn dns_lookup_function = g_dns_lookup);

/**
 * Resolve a service string to its corresponding service.
 *
 * @param name    The string representing a service. Could be a name or a
 *                numerical IP address (IPv6 addresses should be in their
 *                disambiguated bracketed form), optionally followed by a uint16_t port
 *                number. (e.g. example.com:8333 or
 *                [2001:db8:85a3:8d3:1319:8a2e:370:7348]:420)
 * @param[out] vAddr The resulting services to which the specified service string
 *                   resolved.
 * @param portDefault The default port for resulting services if not specified
 *                    by the service string.
 * @param fAllowLookup Whether or not hostname lookups are permitted. If yes,
 *                     external queries may be performed.
 * @param nMaxSolutions The maximum number of results we want, specifying 0
 *                      means "as many solutions as we get."
 *
 * @returns Whether or not the service string successfully resolved to any
 *          resulting services.
 */
bool Lookup(const std::string& name, std::vector<CService>& vAddr, uint16_t portDefault, bool fAllowLookup, unsigned int nMaxSolutions, DNSLookupFn dns_lookup_function = g_dns_lookup);

/**
 * Resolve a service string to its first corresponding service.
 *
 * @see Lookup(const std::string&, std::vector<CService>&, uint16_t, bool, unsigned int, DNSLookupFn)
 *      for additional parameter descriptions.
 */
bool Lookup(const std::string& name, CService& addr, uint16_t portDefault, bool fAllowLookup, DNSLookupFn dns_lookup_function = g_dns_lookup);

/**
 * Resolve a service string with a numeric IP to its first corresponding
 * service.
 *
 * @returns The resulting CService if the resolution was successful, [::]:0 otherwise.
 *
 * @see Lookup(const std::string&, std::vector<CService>&, uint16_t, bool, unsigned int, DNSLookupFn)
 *      for additional parameter descriptions.
 */
CService LookupNumeric(const std::string& name, uint16_t portDefault = 0, DNSLookupFn dns_lookup_function = g_dns_lookup);

/**
 * Parse and resolve a specified subnet string into the appropriate internal
 * representation.
 *
 * @param strSubnet A string representation of a subnet of the form `network
 *                address [ "/", ( CIDR-style suffix | netmask ) ]`(e.g.
 *                `2001:db8::/32`, `192.0.2.0/255.255.255.0`, or `8.8.8.8`).
 *
 * @returns Whether the operation succeeded or not.
 */
bool LookupSubNet(const std::string& strSubnet, CSubNet& subnet, DNSLookupFn dns_lookup_function = g_dns_lookup);

/**
 * Create a TCP socket in the given address family.
 * @param[in] address_family The socket is created in the same address family as this address.
 * @return pointer to the created Sock object or unique_ptr that owns nothing in case of failure
 */
std::unique_ptr<Sock> CreateSockTCP(const CService& address_family);

/**
 * Socket factory. Defaults to `CreateSockTCP()`, but can be overridden by unit tests.
 */
extern std::function<std::unique_ptr<Sock>(const CService&)> CreateSock;

/**
 * Try to connect to the specified service on the specified socket.
 *
 * @param addrConnect The service to which to connect.
 * @param sock The socket on which to connect.
 * @param nTimeout Wait this many milliseconds for the connection to be
 *                 established.
 * @param manual_connection Whether or not the connection was manually requested
 *                          (e.g. through the addnode RPC)
 *
 * @returns Whether or not a connection was successfully made.
 */
bool ConnectSocketDirectly(const CService &addrConnect, const Sock& sock, int nTimeout, bool manual_connection);

/**
 * Connect to a specified destination service through a SOCKS5 proxy by first
 * connecting to the SOCKS5 proxy.
 *
 * @param proxy The SOCKS5 proxy.
 * @param strDest The destination service to which to connect.
 * @param port The destination port.
 * @param sock The socket on which to connect to the SOCKS5 proxy.
 * @param nTimeout Wait this many milliseconds for the connection to the SOCKS5
 *                 proxy to be established.
 * @param[out] outProxyConnectionFailed Whether or not the connection to the
 *                                      SOCKS5 proxy failed.
 *
 * @returns Whether or not the operation succeeded.
 */
bool ConnectThroughProxy(const proxyType& proxy, const std::string& strDest, uint16_t port, const Sock& sock, int nTimeout, bool& outProxyConnectionFailed);

/** Disable or enable blocking-mode for a socket */
bool SetSocketNonBlocking(const SOCKET& hSocket, bool fNonBlocking);
void InterruptSocks5(bool interrupt);

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

#endif // BITCOIN_NETBASE_H
