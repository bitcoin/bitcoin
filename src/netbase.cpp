// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bitcoin-build-config.h> // IWYU pragma: keep

#include <netbase.h>

#include <compat/compat.h>
#include <logging.h>
#include <sync.h>
#include <tinyformat.h>
#include <util/sock.h>
#include <util/strencodings.h>
#include <util/string.h>
#include <util/time.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <limits>
#include <memory>

#ifdef HAVE_SOCKADDR_UN
#include <sys/un.h>
#endif

using util::ContainsNoNUL;

// Settings
static GlobalMutex g_proxyinfo_mutex;
static Proxy proxyInfo[NET_MAX] GUARDED_BY(g_proxyinfo_mutex);
static Proxy nameProxy GUARDED_BY(g_proxyinfo_mutex);
int nConnectTimeout = DEFAULT_CONNECT_TIMEOUT;
bool fNameLookup = DEFAULT_NAME_LOOKUP;

// Need ample time for negotiation for very slow proxies such as Tor
std::chrono::milliseconds g_socks5_recv_timeout = 20s;
CThreadInterrupt g_socks5_interrupt;

ReachableNets g_reachable_nets;

std::vector<CNetAddr> WrappedGetAddrInfo(const std::string& name, bool allow_lookup)
{
    addrinfo ai_hint{};
    // We want a TCP port, which is a streaming socket type
    ai_hint.ai_socktype = SOCK_STREAM;
    ai_hint.ai_protocol = IPPROTO_TCP;
    // We don't care which address family (IPv4 or IPv6) is returned
    ai_hint.ai_family = AF_UNSPEC;

    // If we allow lookups of hostnames, use the AI_ADDRCONFIG flag to only
    // return addresses whose family we have an address configured for.
    //
    // If we don't allow lookups, then use the AI_NUMERICHOST flag for
    // getaddrinfo to only decode numerical network addresses and suppress
    // hostname lookups.
    ai_hint.ai_flags = allow_lookup ? AI_ADDRCONFIG : AI_NUMERICHOST;

    addrinfo* ai_res{nullptr};
    const int n_err{getaddrinfo(name.c_str(), nullptr, &ai_hint, &ai_res)};
    if (n_err != 0) {
        if ((ai_hint.ai_flags & AI_ADDRCONFIG) == AI_ADDRCONFIG) {
            // AI_ADDRCONFIG on some systems may exclude loopback-only addresses
            // If first lookup failed we perform a second lookup without AI_ADDRCONFIG
            ai_hint.ai_flags = (ai_hint.ai_flags & ~AI_ADDRCONFIG);
            const int n_err_retry{getaddrinfo(name.c_str(), nullptr, &ai_hint, &ai_res)};
            if (n_err_retry != 0) {
                return {};
            }
        } else {
            return {};
        }
    }

    // Traverse the linked list starting with ai_trav.
    addrinfo* ai_trav{ai_res};
    std::vector<CNetAddr> resolved_addresses;
    while (ai_trav != nullptr) {
        if (ai_trav->ai_family == AF_INET) {
            assert(ai_trav->ai_addrlen >= sizeof(sockaddr_in));
            resolved_addresses.emplace_back(reinterpret_cast<sockaddr_in*>(ai_trav->ai_addr)->sin_addr);
        }
        if (ai_trav->ai_family == AF_INET6) {
            assert(ai_trav->ai_addrlen >= sizeof(sockaddr_in6));
            const sockaddr_in6* s6{reinterpret_cast<sockaddr_in6*>(ai_trav->ai_addr)};
            resolved_addresses.emplace_back(s6->sin6_addr, s6->sin6_scope_id);
        }
        ai_trav = ai_trav->ai_next;
    }
    freeaddrinfo(ai_res);

    return resolved_addresses;
}

DNSLookupFn g_dns_lookup{WrappedGetAddrInfo};

enum Network ParseNetwork(const std::string& net_in) {
    std::string net = ToLower(net_in);
    if (net == "ipv4") return NET_IPV4;
    if (net == "ipv6") return NET_IPV6;
    if (net == "onion") return NET_ONION;
    if (net == "tor") {
        LogPrintf("Warning: net name 'tor' is deprecated and will be removed in the future. You should use 'onion' instead.\n");
        return NET_ONION;
    }
    if (net == "i2p") {
        return NET_I2P;
    }
    if (net == "cjdns") {
        return NET_CJDNS;
    }
    return NET_UNROUTABLE;
}

std::string GetNetworkName(enum Network net)
{
    switch (net) {
    case NET_UNROUTABLE: return "not_publicly_routable";
    case NET_IPV4: return "ipv4";
    case NET_IPV6: return "ipv6";
    case NET_ONION: return "onion";
    case NET_I2P: return "i2p";
    case NET_CJDNS: return "cjdns";
    case NET_INTERNAL: return "internal";
    case NET_MAX: assert(false);
    } // no default case, so the compiler can warn about missing cases

    assert(false);
}

std::vector<std::string> GetNetworkNames(bool append_unroutable)
{
    std::vector<std::string> names;
    for (int n = 0; n < NET_MAX; ++n) {
        const enum Network network{static_cast<Network>(n)};
        if (network == NET_UNROUTABLE || network == NET_INTERNAL) continue;
        names.emplace_back(GetNetworkName(network));
    }
    if (append_unroutable) {
        names.emplace_back(GetNetworkName(NET_UNROUTABLE));
    }
    return names;
}

static std::vector<CNetAddr> LookupIntern(const std::string& name, unsigned int nMaxSolutions, bool fAllowLookup, DNSLookupFn dns_lookup_function)
{
    if (!ContainsNoNUL(name)) return {};
    {
        CNetAddr addr;
        // From our perspective, onion addresses are not hostnames but rather
        // direct encodings of CNetAddr much like IPv4 dotted-decimal notation
        // or IPv6 colon-separated hextet notation. Since we can't use
        // getaddrinfo to decode them and it wouldn't make sense to resolve
        // them, we return a network address representing it instead. See
        // CNetAddr::SetSpecial(const std::string&) for more details.
        if (addr.SetSpecial(name)) return {addr};
    }

    std::vector<CNetAddr> addresses;

    for (const CNetAddr& resolved : dns_lookup_function(name, fAllowLookup)) {
        if (nMaxSolutions > 0 && addresses.size() >= nMaxSolutions) {
            break;
        }
        /* Never allow resolving to an internal address. Consider any such result invalid */
        if (!resolved.IsInternal()) {
            addresses.push_back(resolved);
        }
    }

    return addresses;
}

std::vector<CNetAddr> LookupHost(const std::string& name, unsigned int nMaxSolutions, bool fAllowLookup, DNSLookupFn dns_lookup_function)
{
    if (!ContainsNoNUL(name)) return {};
    std::string strHost = name;
    if (strHost.empty()) return {};
    if (strHost.front() == '[' && strHost.back() == ']') {
        strHost = strHost.substr(1, strHost.size() - 2);
    }

    return LookupIntern(strHost, nMaxSolutions, fAllowLookup, dns_lookup_function);
}

std::optional<CNetAddr> LookupHost(const std::string& name, bool fAllowLookup, DNSLookupFn dns_lookup_function)
{
    const std::vector<CNetAddr> addresses{LookupHost(name, 1, fAllowLookup, dns_lookup_function)};
    return addresses.empty() ? std::nullopt : std::make_optional(addresses.front());
}

std::vector<CService> Lookup(const std::string& name, uint16_t portDefault, bool fAllowLookup, unsigned int nMaxSolutions, DNSLookupFn dns_lookup_function)
{
    if (name.empty() || !ContainsNoNUL(name)) {
        return {};
    }
    uint16_t port{portDefault};
    std::string hostname;
    SplitHostPort(name, port, hostname);

    const std::vector<CNetAddr> addresses{LookupIntern(hostname, nMaxSolutions, fAllowLookup, dns_lookup_function)};
    if (addresses.empty()) return {};
    std::vector<CService> services;
    services.reserve(addresses.size());
    for (const auto& addr : addresses)
        services.emplace_back(addr, port);
    return services;
}

std::optional<CService> Lookup(const std::string& name, uint16_t portDefault, bool fAllowLookup, DNSLookupFn dns_lookup_function)
{
    const std::vector<CService> services{Lookup(name, portDefault, fAllowLookup, 1, dns_lookup_function)};

    return services.empty() ? std::nullopt : std::make_optional(services.front());
}

CService LookupNumeric(const std::string& name, uint16_t portDefault, DNSLookupFn dns_lookup_function)
{
    if (!ContainsNoNUL(name)) {
        return {};
    }
    // "1.2:345" will fail to resolve the ip, but will still set the port.
    // If the ip fails to resolve, re-init the result.
    return Lookup(name, portDefault, /*fAllowLookup=*/false, dns_lookup_function).value_or(CService{});
}

bool IsUnixSocketPath(const std::string& name)
{
#ifdef HAVE_SOCKADDR_UN
    if (!name.starts_with(ADDR_PREFIX_UNIX)) return false;

    // Split off "unix:" prefix
    std::string str{name.substr(ADDR_PREFIX_UNIX.length())};

    // Path size limit is platform-dependent
    // see https://manpages.ubuntu.com/manpages/xenial/en/man7/unix.7.html
    if (str.size() + 1 > sizeof(((sockaddr_un*)nullptr)->sun_path)) return false;

    return true;
#else
    return false;
#endif
}

/** SOCKS version */
enum SOCKSVersion: uint8_t {
    SOCKS4 = 0x04,
    SOCKS5 = 0x05
};

/** Values defined for METHOD in RFC1928 */
enum SOCKS5Method: uint8_t {
    NOAUTH = 0x00,        //!< No authentication required
    GSSAPI = 0x01,        //!< GSSAPI
    USER_PASS = 0x02,     //!< Username/password
    NO_ACCEPTABLE = 0xff, //!< No acceptable methods
};

/** Values defined for CMD in RFC1928 */
enum SOCKS5Command: uint8_t {
    CONNECT = 0x01,
    BIND = 0x02,
    UDP_ASSOCIATE = 0x03
};

/** Values defined for REP in RFC1928 and https://spec.torproject.org/socks-extensions.html */
enum SOCKS5Reply: uint8_t {
    SUCCEEDED = 0x00,                  //!< RFC1928: Succeeded
    GENFAILURE = 0x01,                 //!< RFC1928: General failure
    NOTALLOWED = 0x02,                 //!< RFC1928: Connection not allowed by ruleset
    NETUNREACHABLE = 0x03,             //!< RFC1928: Network unreachable
    HOSTUNREACHABLE = 0x04,            //!< RFC1928: Network unreachable
    CONNREFUSED = 0x05,                //!< RFC1928: Connection refused
    TTLEXPIRED = 0x06,                 //!< RFC1928: TTL expired
    CMDUNSUPPORTED = 0x07,             //!< RFC1928: Command not supported
    ATYPEUNSUPPORTED = 0x08,           //!< RFC1928: Address type not supported
    TOR_HS_DESC_NOT_FOUND = 0xf0,      //!< Tor: Onion service descriptor can not be found
    TOR_HS_DESC_INVALID = 0xf1,        //!< Tor: Onion service descriptor is invalid
    TOR_HS_INTRO_FAILED = 0xf2,        //!< Tor: Onion service introduction failed
    TOR_HS_REND_FAILED = 0xf3,         //!< Tor: Onion service rendezvous failed
    TOR_HS_MISSING_CLIENT_AUTH = 0xf4, //!< Tor: Onion service missing client authorization
    TOR_HS_WRONG_CLIENT_AUTH = 0xf5,   //!< Tor: Onion service wrong client authorization
    TOR_HS_BAD_ADDRESS = 0xf6,         //!< Tor: Onion service invalid address
    TOR_HS_INTRO_TIMEOUT = 0xf7,       //!< Tor: Onion service introduction timed out
};

/** Values defined for ATYPE in RFC1928 */
enum SOCKS5Atyp: uint8_t {
    IPV4 = 0x01,
    DOMAINNAME = 0x03,
    IPV6 = 0x04,
};

/** Status codes that can be returned by InterruptibleRecv */
enum class IntrRecvError {
    OK,
    Timeout,
    Disconnected,
    NetworkError,
    Interrupted
};

/**
 * Try to read a specified number of bytes from a socket. Please read the "see
 * also" section for more detail.
 *
 * @param data The buffer where the read bytes should be stored.
 * @param len The number of bytes to read into the specified buffer.
 * @param timeout The total timeout for this read.
 * @param sock The socket (has to be in non-blocking mode) from which to read bytes.
 *
 * @returns An IntrRecvError indicating the resulting status of this read.
 *          IntrRecvError::OK only if all of the specified number of bytes were
 *          read.
 *
 * @see This function can be interrupted by calling g_socks5_interrupt().
 *      Sockets can be made non-blocking with Sock::SetNonBlocking().
 */
static IntrRecvError InterruptibleRecv(uint8_t* data, size_t len, std::chrono::milliseconds timeout, const Sock& sock)
{
    auto curTime{Now<SteadyMilliseconds>()};
    const auto endTime{curTime + timeout};
    while (len > 0 && curTime < endTime) {
        ssize_t ret = sock.Recv(data, len, 0); // Optimistically try the recv first
        if (ret > 0) {
            len -= ret;
            data += ret;
        } else if (ret == 0) { // Unexpected disconnection
            return IntrRecvError::Disconnected;
        } else { // Other error or blocking
            int nErr = WSAGetLastError();
            if (nErr == WSAEINPROGRESS || nErr == WSAEWOULDBLOCK || nErr == WSAEINVAL) {
                // Only wait at most MAX_WAIT_FOR_IO at a time, unless
                // we're approaching the end of the specified total timeout
                const auto remaining = std::chrono::milliseconds{endTime - curTime};
                const auto timeout = std::min(remaining, std::chrono::milliseconds{MAX_WAIT_FOR_IO});
                if (!sock.Wait(timeout, Sock::RECV)) {
                    return IntrRecvError::NetworkError;
                }
            } else {
                return IntrRecvError::NetworkError;
            }
        }
        if (g_socks5_interrupt) {
            return IntrRecvError::Interrupted;
        }
        curTime = Now<SteadyMilliseconds>();
    }
    return len == 0 ? IntrRecvError::OK : IntrRecvError::Timeout;
}

/** Convert SOCKS5 reply to an error message */
static std::string Socks5ErrorString(uint8_t err)
{
    switch(err) {
        case SOCKS5Reply::GENFAILURE:
            return "general failure";
        case SOCKS5Reply::NOTALLOWED:
            return "connection not allowed";
        case SOCKS5Reply::NETUNREACHABLE:
            return "network unreachable";
        case SOCKS5Reply::HOSTUNREACHABLE:
            return "host unreachable";
        case SOCKS5Reply::CONNREFUSED:
            return "connection refused";
        case SOCKS5Reply::TTLEXPIRED:
            return "TTL expired";
        case SOCKS5Reply::CMDUNSUPPORTED:
            return "protocol error";
        case SOCKS5Reply::ATYPEUNSUPPORTED:
            return "address type not supported";
        case SOCKS5Reply::TOR_HS_DESC_NOT_FOUND:
            return "onion service descriptor can not be found";
        case SOCKS5Reply::TOR_HS_DESC_INVALID:
            return "onion service descriptor is invalid";
        case SOCKS5Reply::TOR_HS_INTRO_FAILED:
            return "onion service introduction failed";
        case SOCKS5Reply::TOR_HS_REND_FAILED:
            return "onion service rendezvous failed";
        case SOCKS5Reply::TOR_HS_MISSING_CLIENT_AUTH:
            return "onion service missing client authorization";
        case SOCKS5Reply::TOR_HS_WRONG_CLIENT_AUTH:
            return "onion service wrong client authorization";
        case SOCKS5Reply::TOR_HS_BAD_ADDRESS:
            return "onion service invalid address";
        case SOCKS5Reply::TOR_HS_INTRO_TIMEOUT:
            return "onion service introduction timed out";
        default:
            return strprintf("unknown (0x%02x)", err);
    }
}

bool Socks5(const std::string& strDest, uint16_t port, const ProxyCredentials* auth, const Sock& sock)
{
    try {
        IntrRecvError recvr;
        LogDebug(BCLog::NET, "SOCKS5 connecting %s\n", strDest);
        if (strDest.size() > 255) {
            LogError("Hostname too long\n");
            return false;
        }
        // Construct the version identifier/method selection message
        std::vector<uint8_t> vSocks5Init;
        vSocks5Init.push_back(SOCKSVersion::SOCKS5); // We want the SOCK5 protocol
        if (auth) {
            vSocks5Init.push_back(0x02); // 2 method identifiers follow...
            vSocks5Init.push_back(SOCKS5Method::NOAUTH);
            vSocks5Init.push_back(SOCKS5Method::USER_PASS);
        } else {
            vSocks5Init.push_back(0x01); // 1 method identifier follows...
            vSocks5Init.push_back(SOCKS5Method::NOAUTH);
        }
        sock.SendComplete(vSocks5Init, g_socks5_recv_timeout, g_socks5_interrupt);
        uint8_t pchRet1[2];
        if (InterruptibleRecv(pchRet1, 2, g_socks5_recv_timeout, sock) != IntrRecvError::OK) {
            LogPrintf("Socks5() connect to %s:%d failed: InterruptibleRecv() timeout or other failure\n", strDest, port);
            return false;
        }
        if (pchRet1[0] != SOCKSVersion::SOCKS5) {
            LogError("Proxy failed to initialize\n");
            return false;
        }
        if (pchRet1[1] == SOCKS5Method::USER_PASS && auth) {
            // Perform username/password authentication (as described in RFC1929)
            std::vector<uint8_t> vAuth;
            vAuth.push_back(0x01); // Current (and only) version of user/pass subnegotiation
            if (auth->username.size() > 255 || auth->password.size() > 255) {
                LogError("Proxy username or password too long\n");
                return false;
            }
            vAuth.push_back(auth->username.size());
            vAuth.insert(vAuth.end(), auth->username.begin(), auth->username.end());
            vAuth.push_back(auth->password.size());
            vAuth.insert(vAuth.end(), auth->password.begin(), auth->password.end());
            sock.SendComplete(vAuth, g_socks5_recv_timeout, g_socks5_interrupt);
            LogDebug(BCLog::PROXY, "SOCKS5 sending proxy authentication %s:%s\n", auth->username, auth->password);
            uint8_t pchRetA[2];
            if (InterruptibleRecv(pchRetA, 2, g_socks5_recv_timeout, sock) != IntrRecvError::OK) {
                LogError("Error reading proxy authentication response\n");
                return false;
            }
            if (pchRetA[0] != 0x01 || pchRetA[1] != 0x00) {
                LogError("Proxy authentication unsuccessful\n");
                return false;
            }
        } else if (pchRet1[1] == SOCKS5Method::NOAUTH) {
            // Perform no authentication
        } else {
            LogError("Proxy requested wrong authentication method %02x\n", pchRet1[1]);
            return false;
        }
        std::vector<uint8_t> vSocks5;
        vSocks5.push_back(SOCKSVersion::SOCKS5);   // VER protocol version
        vSocks5.push_back(SOCKS5Command::CONNECT); // CMD CONNECT
        vSocks5.push_back(0x00);                   // RSV Reserved must be 0
        vSocks5.push_back(SOCKS5Atyp::DOMAINNAME); // ATYP DOMAINNAME
        vSocks5.push_back(strDest.size());         // Length<=255 is checked at beginning of function
        vSocks5.insert(vSocks5.end(), strDest.begin(), strDest.end());
        vSocks5.push_back((port >> 8) & 0xFF);
        vSocks5.push_back((port >> 0) & 0xFF);
        sock.SendComplete(vSocks5, g_socks5_recv_timeout, g_socks5_interrupt);
        uint8_t pchRet2[4];
        if ((recvr = InterruptibleRecv(pchRet2, 4, g_socks5_recv_timeout, sock)) != IntrRecvError::OK) {
            if (recvr == IntrRecvError::Timeout) {
                /* If a timeout happens here, this effectively means we timed out while connecting
                 * to the remote node. This is very common for Tor, so do not print an
                 * error message. */
                return false;
            } else {
                LogError("Error while reading proxy response\n");
                return false;
            }
        }
        if (pchRet2[0] != SOCKSVersion::SOCKS5) {
            LogError("Proxy failed to accept request\n");
            return false;
        }
        if (pchRet2[1] != SOCKS5Reply::SUCCEEDED) {
            // Failures to connect to a peer that are not proxy errors
            LogPrintLevel(BCLog::NET, BCLog::Level::Debug,
                          "Socks5() connect to %s:%d failed: %s\n", strDest, port, Socks5ErrorString(pchRet2[1]));
            return false;
        }
        if (pchRet2[2] != 0x00) { // Reserved field must be 0
            LogError("Error: malformed proxy response\n");
            return false;
        }
        uint8_t pchRet3[256];
        switch (pchRet2[3]) {
        case SOCKS5Atyp::IPV4: recvr = InterruptibleRecv(pchRet3, 4, g_socks5_recv_timeout, sock); break;
        case SOCKS5Atyp::IPV6: recvr = InterruptibleRecv(pchRet3, 16, g_socks5_recv_timeout, sock); break;
        case SOCKS5Atyp::DOMAINNAME: {
            recvr = InterruptibleRecv(pchRet3, 1, g_socks5_recv_timeout, sock);
            if (recvr != IntrRecvError::OK) {
                LogError("Error reading from proxy\n");
                return false;
            }
            int nRecv = pchRet3[0];
            recvr = InterruptibleRecv(pchRet3, nRecv, g_socks5_recv_timeout, sock);
            break;
        }
        default: {
            LogError("Error: malformed proxy response\n");
            return false;
        }
        }
        if (recvr != IntrRecvError::OK) {
            LogError("Error reading from proxy\n");
            return false;
        }
        if (InterruptibleRecv(pchRet3, 2, g_socks5_recv_timeout, sock) != IntrRecvError::OK) {
            LogError("Error reading from proxy\n");
            return false;
        }
        LogDebug(BCLog::NET, "SOCKS5 connected %s\n", strDest);
        return true;
    } catch (const std::runtime_error& e) {
        LogError("Error during SOCKS5 proxy handshake: %s\n", e.what());
        return false;
    }
}

std::unique_ptr<Sock> CreateSockOS(int domain, int type, int protocol)
{
    // Not IPv4, IPv6 or UNIX
    if (domain == AF_UNSPEC) return nullptr;

    // Create a socket in the specified address family.
    SOCKET hSocket = socket(domain, type, protocol);
    if (hSocket == INVALID_SOCKET) {
        return nullptr;
    }

    auto sock = std::make_unique<Sock>(hSocket);

    if (domain != AF_INET && domain != AF_INET6 && domain != AF_UNIX) {
        return sock;
    }

    // Ensure that waiting for I/O on this socket won't result in undefined
    // behavior.
    if (!sock->IsSelectable()) {
        LogPrintf("Cannot create connection: non-selectable socket created (fd >= FD_SETSIZE ?)\n");
        return nullptr;
    }

#ifdef SO_NOSIGPIPE
    int set = 1;
    // Set the no-sigpipe option on the socket for BSD systems, other UNIXes
    // should use the MSG_NOSIGNAL flag for every send.
    if (sock->SetSockOpt(SOL_SOCKET, SO_NOSIGPIPE, &set, sizeof(int)) == SOCKET_ERROR) {
        LogPrintf("Error setting SO_NOSIGPIPE on socket: %s, continuing anyway\n",
                  NetworkErrorString(WSAGetLastError()));
    }
#endif

    // Set the non-blocking option on the socket.
    if (!sock->SetNonBlocking()) {
        LogPrintf("Error setting socket to non-blocking: %s\n", NetworkErrorString(WSAGetLastError()));
        return nullptr;
    }

#ifdef HAVE_SOCKADDR_UN
    if (domain == AF_UNIX) return sock;
#endif

    if (protocol == IPPROTO_TCP) {
        // Set the no-delay option (disable Nagle's algorithm) on the TCP socket.
        const int on{1};
        if (sock->SetSockOpt(IPPROTO_TCP, TCP_NODELAY, &on, sizeof(on)) == SOCKET_ERROR) {
            LogDebug(BCLog::NET, "Unable to set TCP_NODELAY on a newly created socket, continuing anyway\n");
        }
    }

    return sock;
}

std::function<std::unique_ptr<Sock>(int, int, int)> CreateSock = CreateSockOS;

template<typename... Args>
static void LogConnectFailure(bool manual_connection, util::ConstevalFormatString<sizeof...(Args)> fmt, const Args&... args)
{
    std::string error_message = tfm::format(fmt, args...);
    if (manual_connection) {
        LogPrintf("%s\n", error_message);
    } else {
        LogDebug(BCLog::NET, "%s\n", error_message);
    }
}

static bool ConnectToSocket(const Sock& sock, struct sockaddr* sockaddr, socklen_t len, const std::string& dest_str, bool manual_connection)
{
    // Connect to `sockaddr` using `sock`.
    if (sock.Connect(sockaddr, len) == SOCKET_ERROR) {
        int nErr = WSAGetLastError();
        // WSAEINVAL is here because some legacy version of winsock uses it
        if (nErr == WSAEINPROGRESS || nErr == WSAEWOULDBLOCK || nErr == WSAEINVAL)
        {
            // Connection didn't actually fail, but is being established
            // asynchronously. Thus, use async I/O api (select/poll)
            // synchronously to check for successful connection with a timeout.
            const Sock::Event requested = Sock::RECV | Sock::SEND;
            Sock::Event occurred;
            if (!sock.Wait(std::chrono::milliseconds{nConnectTimeout}, requested, &occurred)) {
                LogPrintf("wait for connect to %s failed: %s\n",
                          dest_str,
                          NetworkErrorString(WSAGetLastError()));
                return false;
            } else if (occurred == 0) {
                LogPrintLevel(BCLog::NET, BCLog::Level::Debug, "connection attempt to %s timed out\n", dest_str);
                return false;
            }

            // Even if the wait was successful, the connect might not
            // have been successful. The reason for this failure is hidden away
            // in the SO_ERROR for the socket in modern systems. We read it into
            // sockerr here.
            int sockerr;
            socklen_t sockerr_len = sizeof(sockerr);
            if (sock.GetSockOpt(SOL_SOCKET, SO_ERROR, &sockerr, &sockerr_len) ==
                SOCKET_ERROR) {
                LogPrintf("getsockopt() for %s failed: %s\n", dest_str, NetworkErrorString(WSAGetLastError()));
                return false;
            }
            if (sockerr != 0) {
                LogConnectFailure(manual_connection,
                                  "connect() to %s failed after wait: %s",
                                  dest_str,
                                  NetworkErrorString(sockerr));
                return false;
            }
        }
#ifdef WIN32
        else if (WSAGetLastError() != WSAEISCONN)
#else
        else
#endif
        {
            LogConnectFailure(manual_connection, "connect() to %s failed: %s", dest_str, NetworkErrorString(WSAGetLastError()));
            return false;
        }
    }
    return true;
}

std::unique_ptr<Sock> ConnectDirectly(const CService& dest, bool manual_connection)
{
    auto sock = CreateSock(dest.GetSAFamily(), SOCK_STREAM, IPPROTO_TCP);
    if (!sock) {
        LogPrintLevel(BCLog::NET, BCLog::Level::Error, "Cannot create a socket for connecting to %s\n", dest.ToStringAddrPort());
        return {};
    }

    // Create a sockaddr from the specified service.
    struct sockaddr_storage sockaddr;
    socklen_t len = sizeof(sockaddr);
    if (!dest.GetSockAddr((struct sockaddr*)&sockaddr, &len)) {
        LogPrintf("Cannot get sockaddr for %s: unsupported network\n", dest.ToStringAddrPort());
        return {};
    }

    if (!ConnectToSocket(*sock, (struct sockaddr*)&sockaddr, len, dest.ToStringAddrPort(), manual_connection)) {
        return {};
    }

    return sock;
}

std::unique_ptr<Sock> Proxy::Connect() const
{
    if (!IsValid()) return {};

    if (!m_is_unix_socket) return ConnectDirectly(proxy, /*manual_connection=*/true);

#ifdef HAVE_SOCKADDR_UN
    auto sock = CreateSock(AF_UNIX, SOCK_STREAM, 0);
    if (!sock) {
        LogPrintLevel(BCLog::NET, BCLog::Level::Error, "Cannot create a socket for connecting to %s\n", m_unix_socket_path);
        return {};
    }

    const std::string path{m_unix_socket_path.substr(ADDR_PREFIX_UNIX.length())};

    struct sockaddr_un addrun;
    memset(&addrun, 0, sizeof(addrun));
    addrun.sun_family = AF_UNIX;
    // leave the last char in addrun.sun_path[] to be always '\0'
    memcpy(addrun.sun_path, path.c_str(), std::min(sizeof(addrun.sun_path) - 1, path.length()));
    socklen_t len = sizeof(addrun);

    if(!ConnectToSocket(*sock, (struct sockaddr*)&addrun, len, path, /*manual_connection=*/true)) {
        return {};
    }

    return sock;
#else
    return {};
#endif
}

bool SetProxy(enum Network net, const Proxy &addrProxy) {
    assert(net >= 0 && net < NET_MAX);
    if (!addrProxy.IsValid())
        return false;
    LOCK(g_proxyinfo_mutex);
    proxyInfo[net] = addrProxy;
    return true;
}

bool GetProxy(enum Network net, Proxy &proxyInfoOut) {
    assert(net >= 0 && net < NET_MAX);
    LOCK(g_proxyinfo_mutex);
    if (!proxyInfo[net].IsValid())
        return false;
    proxyInfoOut = proxyInfo[net];
    return true;
}

bool SetNameProxy(const Proxy &addrProxy) {
    if (!addrProxy.IsValid())
        return false;
    LOCK(g_proxyinfo_mutex);
    nameProxy = addrProxy;
    return true;
}

bool GetNameProxy(Proxy &nameProxyOut) {
    LOCK(g_proxyinfo_mutex);
    if(!nameProxy.IsValid())
        return false;
    nameProxyOut = nameProxy;
    return true;
}

bool HaveNameProxy() {
    LOCK(g_proxyinfo_mutex);
    return nameProxy.IsValid();
}

bool IsProxy(const CNetAddr &addr) {
    LOCK(g_proxyinfo_mutex);
    for (int i = 0; i < NET_MAX; i++) {
        if (addr == static_cast<CNetAddr>(proxyInfo[i].proxy))
            return true;
    }
    return false;
}

/**
 * Generate unique credentials for Tor stream isolation. Tor will create
 * separate circuits for SOCKS5 proxy connections with different credentials, which
 * makes it harder to correlate the connections.
 */
class TorStreamIsolationCredentialsGenerator
{
public:
    TorStreamIsolationCredentialsGenerator():
        m_prefix(GenerateUniquePrefix()) {
    }

    /** Return the next unique proxy credentials. */
    ProxyCredentials Generate() {
        ProxyCredentials auth;
        auth.username = auth.password = strprintf("%s%i", m_prefix, m_counter);
        ++m_counter;
        return auth;
    }

    /** Size of session prefix in bytes. */
    static constexpr size_t PREFIX_BYTE_LENGTH = 8;
private:
    const std::string m_prefix;
    std::atomic<uint64_t> m_counter;

    /** Generate a random prefix for each of the credentials returned by this generator.
     * This makes sure that different launches of the application (either successively or in parallel)
     * will not share the same circuits, as would be the case with a bare counter.
     */
    static std::string GenerateUniquePrefix() {
        std::array<uint8_t, PREFIX_BYTE_LENGTH> prefix_bytes;
        GetRandBytes(prefix_bytes);
        return HexStr(prefix_bytes) + "-";
    }
};

std::unique_ptr<Sock> ConnectThroughProxy(const Proxy& proxy,
                                          const std::string& dest,
                                          uint16_t port,
                                          bool& proxy_connection_failed)
{
    // first connect to proxy server
    auto sock = proxy.Connect();
    if (!sock) {
        proxy_connection_failed = true;
        return {};
    }

    // do socks negotiation
    if (proxy.m_tor_stream_isolation) {
        static TorStreamIsolationCredentialsGenerator generator;
        ProxyCredentials random_auth{generator.Generate()};
        if (!Socks5(dest, port, &random_auth, *sock)) {
            return {};
        }
    } else {
        if (!Socks5(dest, port, nullptr, *sock)) {
            return {};
        }
    }
    return sock;
}

CSubNet LookupSubNet(const std::string& subnet_str)
{
    CSubNet subnet;
    assert(!subnet.IsValid());
    if (!ContainsNoNUL(subnet_str)) {
        return subnet;
    }

    const size_t slash_pos{subnet_str.find_last_of('/')};
    const std::string str_addr{subnet_str.substr(0, slash_pos)};
    std::optional<CNetAddr> addr{LookupHost(str_addr, /*fAllowLookup=*/false)};

    if (addr.has_value()) {
        addr = static_cast<CNetAddr>(MaybeFlipIPv6toCJDNS(CService{addr.value(), /*port=*/0}));
        if (slash_pos != subnet_str.npos) {
            const std::string netmask_str{subnet_str.substr(slash_pos + 1)};
            if (const auto netmask{ToIntegral<uint8_t>(netmask_str)}) {
                // Valid number; assume CIDR variable-length subnet masking.
                subnet = CSubNet{addr.value(), *netmask};
            } else {
                // Invalid number; try full netmask syntax. Never allow lookup for netmask.
                const std::optional<CNetAddr> full_netmask{LookupHost(netmask_str, /*fAllowLookup=*/false)};
                if (full_netmask.has_value()) {
                    subnet = CSubNet{addr.value(), full_netmask.value()};
                }
            }
        } else {
            // Single IP subnet (<ipv4>/32 or <ipv6>/128).
            subnet = CSubNet{addr.value()};
        }
    }

    return subnet;
}

bool IsBadPort(uint16_t port)
{
    /* Don't forget to update doc/p2p-bad-ports.md if you change this list. */

    switch (port) {
    case 1:     // tcpmux
    case 7:     // echo
    case 9:     // discard
    case 11:    // systat
    case 13:    // daytime
    case 15:    // netstat
    case 17:    // qotd
    case 19:    // chargen
    case 20:    // ftp data
    case 21:    // ftp access
    case 22:    // ssh
    case 23:    // telnet
    case 25:    // smtp
    case 37:    // time
    case 42:    // name
    case 43:    // nicname
    case 53:    // domain
    case 69:    // tftp
    case 77:    // priv-rjs
    case 79:    // finger
    case 87:    // ttylink
    case 95:    // supdup
    case 101:   // hostname
    case 102:   // iso-tsap
    case 103:   // gppitnp
    case 104:   // acr-nema
    case 109:   // pop2
    case 110:   // pop3
    case 111:   // sunrpc
    case 113:   // auth
    case 115:   // sftp
    case 117:   // uucp-path
    case 119:   // nntp
    case 123:   // NTP
    case 135:   // loc-srv /epmap
    case 137:   // netbios
    case 139:   // netbios
    case 143:   // imap2
    case 161:   // snmp
    case 179:   // BGP
    case 389:   // ldap
    case 427:   // SLP (Also used by Apple Filing Protocol)
    case 465:   // smtp+ssl
    case 512:   // print / exec
    case 513:   // login
    case 514:   // shell
    case 515:   // printer
    case 526:   // tempo
    case 530:   // courier
    case 531:   // chat
    case 532:   // netnews
    case 540:   // uucp
    case 548:   // AFP (Apple Filing Protocol)
    case 554:   // rtsp
    case 556:   // remotefs
    case 563:   // nntp+ssl
    case 587:   // smtp (rfc6409)
    case 601:   // syslog-conn (rfc3195)
    case 636:   // ldap+ssl
    case 989:   // ftps-data
    case 990:   // ftps
    case 993:   // ldap+ssl
    case 995:   // pop3+ssl
    case 1719:  // h323gatestat
    case 1720:  // h323hostcall
    case 1723:  // pptp
    case 2049:  // nfs
    case 3306:  // MySQL
    case 3389:  // RDP / Windows Remote Desktop
    case 3659:  // apple-sasl / PasswordServer
    case 4045:  // lockd
    case 5060:  // sip
    case 5061:  // sips
    case 5432:  // PostgreSQL
    case 5900:  // VNC
    case 6000:  // X11
    case 6566:  // sane-port
    case 6665:  // Alternate IRC
    case 6666:  // Alternate IRC
    case 6667:  // Standard IRC
    case 6668:  // Alternate IRC
    case 6669:  // Alternate IRC
    case 6697:  // IRC + TLS
    case 10080: // Amanda
    case 27017: // MongoDB
        return true;
    }
    return false;
}

CService MaybeFlipIPv6toCJDNS(const CService& service)
{
    CService ret{service};
    if (ret.IsIPv6() && ret.HasCJDNSPrefix() && g_reachable_nets.Contains(NET_CJDNS)) {
        ret.m_net = NET_CJDNS;
    }
    return ret;
}
