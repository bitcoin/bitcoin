// Copyright (c) 2024 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://www.opensource.org/licenses/mit-license.php.

#if defined(HAVE_CONFIG_H)
#include <config/bitcoin-config.h>
#endif

#include <crypto/common.h>
#include <logging.h>
#include <netaddress.h>
#include <random.h>
#include <util/readwritefile.h>
#include <util/sock.h>
#include <util/strencodings.h>
#include <util/translation.h> // Remove this

#include <linux/route.h>

const std::function<std::string(const char*)> G_TRANSLATION_FUN = nullptr; // Remove this

// RFC 6887 Port Control Protocol (PCP) implementation.
// PCP uses network byte order (big-endian).
// References to sections and figures in the code below refer to https://datatracker.ietf.org/doc/html/rfc6887.

// Protocol constants.
//! Maximum packet size in bytes (see section 7).
constexpr size_t PCP_MAX_SIZE = 1100;
//! PCP uses a fixed server port number (see section 19.1).
constexpr uint16_t PCP_SERVER_PORT = 5351;
//! Version byte. 0 is NAT-PMP, 1 is forbidden, 2 for PCP RFC-6887.
constexpr uint8_t PCP_VERSION = 2;
//! PCP Request Header. See section 7.1
constexpr uint8_t PCP_REQUEST = 0x00; // R = 0
//! PCP Response Header. See section 7.2
constexpr uint8_t PCP_RESPONSE = 0x80; // R = 1
//! Map opcode. See section 19.2
constexpr uint8_t PCP_OP_MAP = 0x01;
//! TCP protocol number (IANA).
constexpr uint16_t PCP_PROTOCOL_TCP = 6;
//! Option: prefer failure to half-functional mapping. See section 13.2.
constexpr uint8_t PCP_OPTION_PREFER_FAILURE = 2;
//! Request header size in bytes (see section 7.1).
constexpr size_t PCP_REQUEST_HDR_SIZE = 24;
//! Response header size in bytes (see section 7.2).
constexpr size_t PCP_RESPONSE_HDR_SIZE = 24;
//! Option header size in bytes (see section 7.2).
constexpr size_t PCP_OPTION_HDR_SIZE = 4;
//! Map request size in bytes (see section 11.1).
constexpr size_t PCP_MAP_REQUEST_SIZE = 36;
//! Map response size in bytes (see section 11.1).
constexpr size_t PCP_MAP_RESPONSE_SIZE = 36;
//! Mapping nonce size in bytes (see section 11.1).
constexpr size_t PCP_MAP_NONCE_SIZE = 12;
//! Result code representing SUCCESS status (7.4).
constexpr uint8_t PCP_RESULT_SUCCESS = 0;

//! Mapping of PCP result code to string (7.4).
static const std::map<uint8_t, std::string> PCP_RESULT_STR{
    {0,  "SUCCESS"},
    {1,  "UNSUPP_VERSION"},
    {2,  "NOT_AUTHORIZED"},
    {3,  "MALFORMED_REQUEST"},
    {4,  "UNSUPP_OPCODE"},
    {5,  "UNSUPP_OPTION"},
    {6,  "MALFORMED_OPTION"},
    {7,  "NETWORK_FAILURE"},
    {8,  "NO_RESOURCES"},
    {9,  "UNSUPP_PROTOCOL"},
    {10, "USER_EX_QUOTA"},
    {11, "CANNOT_PROVIDE_EXTERNAL"},
    {12, "ADDRESS_MISMATCH"},
    {13, "EXCESSIVE_REMOTE_PEER"},
};

//! Human-readable string from result code.
std::string PCPResultString(uint8_t result_code)
{
    auto result_i = PCP_RESULT_STR.find(result_code);
    return strprintf("%s (code %d)", result_i == PCP_RESULT_STR.end() ? "(unknown)" : result_i->second,  result_code);
}

//! Find IPv4 default gateway.
std::optional<CNetAddr> FindIPv4DefaultGateway()
{
    const auto& [read_ok, data] = ReadBinaryFile("/proc/net/route");
    if (!read_ok) {
        LogPrintLevel(BCLog::NET, BCLog::Level::Warning, "pcp: Could not read /proc/net/route to find default IPv4 gateway\n");
        return std::nullopt;
    }
    // [0]Iface [1]Destination [2]Gateway  [3]Flags [4]RefCnt [5]Use [6]Metric [7]Mask  [8]MTU [9]Window [10]IRTT
    for(std::string &line: SplitString(data, "\n")) {
        auto fields = SplitString(line, "\t");
        if (fields.size() != 11) continue;
        const std::optional<uint32_t> mask(ToIntegral<uint32_t>(fields[7], 16));
        const std::optional<uint32_t> gateway(ToIntegral<uint32_t>(fields[2], 16));
        const std::optional<uint32_t> flags(ToIntegral<uint32_t>(fields[3], 16));
        if (!mask || !gateway || !flags) continue; // Parse error
        if (mask.value() != 0x00000000 || (flags.value() & (RTF_GATEWAY | RTF_UP)) != (RTF_GATEWAY | RTF_UP)) continue; // Not default gw

        in_addr dest_addr = {};
        dest_addr.s_addr = gateway.value();
        return CNetAddr(dest_addr);
    }
    return std::nullopt;
}

//! Find IPv6 default gateway.
std::optional<CNetAddr> FindIPv6DefaultGateway()
{
    const auto& [read_ok, data] = ReadBinaryFile("/proc/net/ipv6_route");
    if (!read_ok) {
        LogPrintLevel(BCLog::NET, BCLog::Level::Warning, "pcp: Could not read /proc/net/ipv6_route to find default IPv6 gateway\n");
        return std::nullopt;
    }
    // 149 length (longer if interface name is longer)
    // destination                      prefix                                 nexthop                                                     flags     iface
    // 00000000000000000000000000000000 00 00000000000000000000000000000000 00 fe80000000000000da58d7fffe0077cd 00000064 00000007 00000000 00000003     eth0
    for(std::string &line: SplitString(data, "\n")) {
        if (line.size() < 149) continue;
        const std::optional<std::vector<uint8_t>> nexthop(TryParseHex<uint8_t>(line.substr(72, 32)));
        const std::optional<uint8_t> prefix(ToIntegral<uint8_t>(line.substr(33,  2), ADDR_IPV6_SIZE));
        const std::optional<uint32_t> flags(ToIntegral<uint32_t>(line.substr(132,  8), ADDR_IPV6_SIZE));
        const std::string iface(TrimString(line.substr(141)));
        if (!prefix || !nexthop | !flags || nexthop->size() != ADDR_IPV6_SIZE || iface.empty()) continue; // Parse error
        if (prefix.value() != 0 || (flags.value() & (RTF_GATEWAY | RTF_UP)) != (RTF_GATEWAY | RTF_UP)) continue; // Not default gw
        // Look up scope id for interface name string.
        int gateway_scope_id = if_nametoindex(iface.c_str());
        if (gateway_scope_id <= 0) {
            LogPrintLevel(BCLog::NET, BCLog::Level::Warning, "pcp: Could not parse gateway interface %s\n", iface.c_str());
            return std::nullopt;
        }

        in6_addr dest_addr = {};
        memcpy(&dest_addr, nexthop->data(), ADDR_IPV6_SIZE);
        return CNetAddr(dest_addr, gateway_scope_id);
    }
    return std::nullopt;
}

//! Wrap address in IPv6 according to RFC. wrapped_addr needs to be able to store 16 bytes.
bool PCPWrapAddress(uint8_t *wrapped_addr, const CNetAddr &addr)
{
    if (addr.IsIPv4()) {
        struct in_addr addr4;
        if (!addr.GetInAddr(&addr4)) return false;
        // Section 5: "When the address field holds an IPv4 address, an IPv4-mapped IPv6 address [RFC4291] is used (::ffff:0:0/96)."
        memcpy(wrapped_addr, IPV4_IN_IPV6_PREFIX.data(), IPV4_IN_IPV6_PREFIX.size());
        memcpy(wrapped_addr + IPV4_IN_IPV6_PREFIX.size(), &addr4, ADDR_IPV4_SIZE);
        return true;
    } else if (addr.IsIPv6()) {
        struct in6_addr addr6;
        if (!addr.GetIn6Addr(&addr6)) return false;
        memcpy(wrapped_addr, &addr6, ADDR_IPV6_SIZE);
        return true;
    } else {
        return false;
    }
}

//! Unwrap PCP-encoded address.
CNetAddr PCPUnwrapAddress(const uint8_t *wrapped_addr)
{
    if (memcmp(wrapped_addr, IPV4_IN_IPV6_PREFIX.data(), IPV4_IN_IPV6_PREFIX.size()) == 0) {
        struct in_addr addr4;
        memcpy(&addr4, wrapped_addr + IPV4_IN_IPV6_PREFIX.size(), ADDR_IPV4_SIZE);
        return CNetAddr(addr4);
    } else {
        struct in6_addr addr6;
        memcpy(&addr6, wrapped_addr, ADDR_IPV6_SIZE);
        return CNetAddr(addr6);
    }
}

/// Description of a port mapping.
struct MappingResult {
    MappingResult(const CService &internal_in, const CService &external_in, uint32_t lifetime_in):
        internal(internal_in), external(external_in), lifetime(lifetime_in) {}
    //! Internal host:port.
    CService internal;
    //! External host:port.
    CService external;
    //! Granted lifetime of binding (seconds).
    uint32_t lifetime;
};

//! Try to open a port using RFC 6887 Port Control Protocol (PCP).
//!
//! * gateway: Destination address for PCP (usually the default gateway).
//! * bind: Specific local bind address for IPv6 pinholing. Set this as INADDR_ANY for IPv4.
//! * port: Internal port, and desired external port.
//! * lifetime: Requested lifetime in seconds for mapping. The server may assign as shorter or longer lifetime. A lifetime of 0 deletes the mapping.
//! * num_tries: Number of tries in case of no response.
//! * prefer_failure: Add PREFER_FAILURE option.
//!
//! Returns the external_ip:external_port of the mapping if successful, otherwise nullopt.
std::optional<MappingResult> PCPRequestPortMap(const CNetAddr &gateway, const CNetAddr &bind, uint16_t port, uint32_t lifetime, int num_tries = 3, bool option_prefer_failure = false)
{
    struct sockaddr_storage dest_addr, bind_addr;
    socklen_t dest_addrlen = sizeof(struct sockaddr_storage), bind_addrlen = sizeof(struct sockaddr_storage);

    LogPrintLevel(BCLog::NET, BCLog::Level::Warning, "pcp: Requesting port mapping for addr %s port %d from gateway %s\n", bind.ToStringAddr(), port, gateway.ToStringAddr());

    // Validate addresses, make sure they're the same network family.
    if (!CService(gateway, PCP_SERVER_PORT).GetSockAddr((struct sockaddr*)&dest_addr, &dest_addrlen)) return std::nullopt;
    if (!CService(bind, 0).GetSockAddr((struct sockaddr*)&bind_addr, &bind_addrlen)) return std::nullopt;
    if (dest_addr.ss_family != bind_addr.ss_family) return std::nullopt;

    // Create UDP socket (IPv4 or IPv6 based on provided gateway).
    SOCKET sock_fd = socket(dest_addr.ss_family, SOCK_DGRAM, IPPROTO_UDP);
    if (sock_fd == INVALID_SOCKET) {
        LogPrintLevel(BCLog::NET, BCLog::Level::Warning, "pcp: Could not create UDP socket: %s\n", NetworkErrorString(WSAGetLastError()));
        return std::nullopt;
    }
    Sock sock(sock_fd);

    // Make sure that we send from requested destination address, anything else will be
    // rejected by a security-conscious router.
    if (sock.Bind((struct sockaddr*)&bind_addr, bind_addrlen) != 0) {
        LogPrintLevel(BCLog::NET, BCLog::Level::Warning, "pcp: Could not bind to address: %s\n", NetworkErrorString(WSAGetLastError()));
        return std::nullopt;
    }

    // Associate UDP socket to gateway.
    if (sock.Connect((struct sockaddr*)&dest_addr, dest_addrlen) != 0) {
        LogPrintLevel(BCLog::NET, BCLog::Level::Warning, "pcp: Could not connect to gateway: %s\n", NetworkErrorString(WSAGetLastError()));
        return std::nullopt;
    }

    // Use getsockname to get the address toward the default gateway (the internal address),
    // in case we don't know what address to map
    // (this is only needed if bind is INADDR_ANY, but it doesn't hurt as an extra check).
    struct sockaddr_storage internal_addr;
    socklen_t internal_addrlen = sizeof(struct sockaddr_storage);
    if (sock.GetSockName((struct sockaddr*)&internal_addr, &internal_addrlen) != 0) {
        LogPrintLevel(BCLog::NET, BCLog::Level::Warning, "pcp: Could not get sock name: %s\n", NetworkErrorString(WSAGetLastError()));
        return std::nullopt;
    }
    CService internal;
    if (!internal.SetSockAddr((struct sockaddr*)&internal_addr)) return std::nullopt;
    LogPrintLevel(BCLog::NET, BCLog::Level::Debug, "pcp: Internal address after connect: %s\n", internal.ToStringAddr());

    // Build request packet. Make sure the packet is zeroed so that reserved fields are zero
    // as required by the spec (and not potentially leak data).
    // Make sure there's space for the request header, MAP specific request
    // data, and one option header.
    uint8_t request[PCP_REQUEST_HDR_SIZE + PCP_MAP_REQUEST_SIZE + PCP_OPTION_HDR_SIZE] = {};
    // Fill in request header, See Figure 2.
    size_t ofs = 0;
    request[ofs + 0] = PCP_VERSION;
    request[ofs + 1] = PCP_REQUEST | PCP_OP_MAP;
    WriteBE32(request + ofs + 4, lifetime);
    PCPWrapAddress(request + ofs + 8, internal);

    ofs += PCP_REQUEST_HDR_SIZE;

    // Fill in MAP request packet, See Figure 9.
    // Randomize mapping nonce (this is repeated in the response, to be able to
    // correlate requests and responses, and used to authenticate changes to the mapping).
    // TODO: remember this value when periodically renewing a mapping.
    GetRandBytes(Span(request + ofs + 0, PCP_MAP_NONCE_SIZE));
    request[ofs + 12] = PCP_PROTOCOL_TCP;
    WriteBE16(request + ofs + 16, port);
    WriteBE16(request + ofs + 18, port);
    PCPWrapAddress(request + ofs + 20, bind);

    ofs += PCP_MAP_REQUEST_SIZE;

    if (option_prefer_failure) {
        // Fill in option header. See Figure 4.
        // Prefer failure to a different external address mapping than we expect.
        // TODO: decide if we want to pas this option or rather just handle different addresses/ports than we expect,
        // and advertise those as local address. This would be needed to handle IPv4 port mapping anyway.
        request[ofs] = PCP_OPTION_PREFER_FAILURE;
        // This option takes no data, rest of option header can be left as zero bytes.

        ofs += PCP_OPTION_HDR_SIZE;
    }

    // UDP is a potentially lossy protocol, so we try to send again a few times.
    bool got_response = false;
    uint8_t response[PCP_MAX_SIZE];
    for (int ntry = 0; !got_response && ntry < num_tries; ++ntry) {
        if (ntry > 0) {
            LogPrintLevel(BCLog::NET, BCLog::Level::Debug, "pcp: Retrying (%d)\n", ntry);
        }
        // Dispatch packet to gateway.
        if (sock.Send(request, ofs, 0) != static_cast<ssize_t>(ofs)) {
            LogPrintLevel(BCLog::NET, BCLog::Level::Warning, "pcp: Could not send request: %s\n", NetworkErrorString(WSAGetLastError()));
            return std::nullopt; // Network-level error, probably no use retrying.
        }

        // Wait for response(s) until we get a valid response, a network error, or time out.
        while (true) {
            Sock::Event occured = 0;
            if (!sock.Wait(std::chrono::milliseconds(1000), Sock::RECV, &occured)) {
                LogPrintLevel(BCLog::NET, BCLog::Level::Warning, "pcp: Could not wait on socket: %s\n", NetworkErrorString(WSAGetLastError()));
                return std::nullopt; // Network-level error, probably no use retrying.
            }
            if (!occured) {
                LogPrintLevel(BCLog::NET, BCLog::Level::Debug, "pcp: Timeout\n");
                break; // Retry.
            }

            // Receive response.
            int recvsz = sock.Recv(response, sizeof(response), MSG_DONTWAIT);
            if (recvsz < 0) {
                LogPrintLevel(BCLog::NET, BCLog::Level::Warning, "pcp: Could not receive response: %s\n", NetworkErrorString(WSAGetLastError()));
                return std::nullopt; // Network-level error, probably no use retrying.
            }
            LogPrintLevel(BCLog::NET, BCLog::Level::Debug, "pcp: Received response of %d bytes: %s\n", recvsz, HexStr(Span(response, recvsz)));
            if (static_cast<size_t>(recvsz) < (PCP_RESPONSE_HDR_SIZE + PCP_MAP_RESPONSE_SIZE)) {
                LogPrintLevel(BCLog::NET, BCLog::Level::Warning, "pcp: Response too small\n");
                continue; // Wasn't response to what we expected, try receiving next packet.
            }
            if (response[0] != PCP_VERSION || response[1] != (PCP_RESPONSE | PCP_OP_MAP)) {
                LogPrintLevel(BCLog::NET, BCLog::Level::Warning, "pcp: Response to wrong command\n");
                continue; // Wasn't response to what we expected, try receiving next packet.
            }
            // Handle MAP opcode response. See Figure 10.
            // Check that returned mapping nonce matches our request.
            if (memcmp(response + PCP_RESPONSE_HDR_SIZE, request + PCP_REQUEST_HDR_SIZE, PCP_MAP_NONCE_SIZE) != 0) {
                LogPrintLevel(BCLog::NET, BCLog::Level::Warning, "pcp: Mapping nonce mismatch\n");
                continue; // Wasn't response to what we expected, try receiving next packet.
            }
            uint8_t protocol = response[PCP_RESPONSE_HDR_SIZE + 12];
            uint16_t internal_port = ReadBE16(response + PCP_RESPONSE_HDR_SIZE + 16);
            if (protocol != PCP_PROTOCOL_TCP || internal_port != port) {
                LogPrintLevel(BCLog::NET, BCLog::Level::Warning, "pcp: Response protocol or port doesn't match request\n");
                continue; // Wasn't response to what we expected, try receiving next packet.
            }
            got_response = true; // Got expected response, break from receive loop as well as from retry loop.
            break;
        }
    }
    if (!got_response) {
        LogPrintLevel(BCLog::NET, BCLog::Level::Debug, "pcp: Giving up after %d tries\n", num_tries);
        return std::nullopt;
    }
    // If we get here, we got a valid MAP response to our request.
    // Check to see if we got the result we expected.
    uint8_t result_code = response[3];
    uint32_t lifetime_ret = ReadBE32(response + 4);
    uint16_t external_port = ReadBE16(response + PCP_RESPONSE_HDR_SIZE + 18);
    CNetAddr external_addr{PCPUnwrapAddress(response + PCP_RESPONSE_HDR_SIZE + 20)};
    if (result_code != PCP_RESULT_SUCCESS) {
        LogPrintLevel(BCLog::NET, BCLog::Level::Warning, "pcp: Mapping failed with result %s\n", PCPResultString(result_code));
        return std::nullopt;
    }
    /*
    // Mapping was successful. Just to be sure, check that the external port and address match what we expect.
    if (external_port != port || memcmp(&external_addr, wrapped_addr, ADDR_IPV6_SIZE)) {
        LogPrintLevel(BCLog::NET, BCLog::Level::Warning, "pcp: We were assigned a different address (%s) or port (%s) than expected\n",
            CNetAddr(external_addr).ToStringAddr(), external_port);
    }
    */
    LogPrintLevel(BCLog::NET, BCLog::Level::Info, "pcp: Mapping successful: we got %s:%d for %d seconds.\n",
        external_addr.ToStringAddr(), external_port,
        lifetime_ret);

    return MappingResult(CService(internal, port), CService(external_addr, external_port), lifetime_ret);
}

// TODO share with Discover()
//! Return all local non-loopback IPv4 and IPv6 network addresses.
std::vector<CNetAddr> GetLocalAddresses()
{
    std::vector<CNetAddr> addresses;
#ifdef WIN32
    char pszHostName[256] = "";
    if (gethostname(pszHostName, sizeof(pszHostName)) != SOCKET_ERROR)
    {
        addresses = LookupHost(pszHostName, 0, true);
    }
#elif (HAVE_DECL_GETIFADDRS && HAVE_DECL_FREEIFADDRS)
    struct ifaddrs* myaddrs;
    if (getifaddrs(&myaddrs) == 0)
    {
        for (struct ifaddrs* ifa = myaddrs; ifa != nullptr; ifa = ifa->ifa_next)
        {
            if (ifa->ifa_addr == nullptr) continue;
            if ((ifa->ifa_flags & IFF_UP) == 0) continue;
            if ((ifa->ifa_flags & IFF_LOOPBACK) != 0) continue;
            if (ifa->ifa_addr->sa_family == AF_INET)
            {
                struct sockaddr_in* s4 = (struct sockaddr_in*)(ifa->ifa_addr);
                addresses.emplace_back(s4->sin_addr);
            }
            else if (ifa->ifa_addr->sa_family == AF_INET6)
            {
                struct sockaddr_in6* s6 = (struct sockaddr_in6*)(ifa->ifa_addr);
                addresses.emplace_back(s6->sin6_addr);
            }
        }
        freeifaddrs(myaddrs);
    }
#endif
    return addresses;
}

int main(int argc, char **argv)
{
    // Needed to get LogPrintf working.
    LogInstance().m_print_to_file = false;
    LogInstance().m_print_to_console = true;
    LogInstance().m_log_timestamps = true;
    LogInstance().m_log_time_micros = false;
    LogInstance().m_log_sourcelocations = false;
    LogInstance().m_always_print_category_level = true;
    LogInstance().SetLogLevel(BCLog::Level::Debug);
    LogInstance().StartLogging();

    LogInstance().EnableCategory(BCLog::NET);
    LogInstance().SetCategoryLogLevel("net", "trace");

    LogPrintLevel(BCLog::NET, BCLog::Level::Warning, "*************************************************\n");
    LogPrintLevel(BCLog::NET, BCLog::Level::Warning, "PCP port mapping / pinholing test\n");
    LogPrintLevel(BCLog::NET, BCLog::Level::Warning, "*************************************************\n");
    LogPrintLevel(BCLog::NET, BCLog::Level::Warning, "This will try to open port 1234 on all publicly routed IPv6 addresses, as well as create a IPv4 port mapping for a duration of 100 seconds.\n");
    LogPrintLevel(BCLog::NET, BCLog::Level::Warning, "Assuming any mappings succeed, you can test them by, on the listening host, doing:\n");
    LogPrintLevel(BCLog::NET, BCLog::Level::Warning, "    nc -v6l 1234\n");
    LogPrintLevel(BCLog::NET, BCLog::Level::Warning, "Then from a different external host, try to connect:\n");
    LogPrintLevel(BCLog::NET, BCLog::Level::Warning, "    nc -v6 <addr> 1234\n");
    LogPrintLevel(BCLog::NET, BCLog::Level::Warning, "\n");

    // IPv4
    std::optional<CNetAddr> gateway4 = FindIPv4DefaultGateway();
    if (!gateway4) {
        LogPrintLevel(BCLog::NET, BCLog::Level::Warning, "pcp: Could not determine IPv4 default gateway\n");
        exit(1);
    }
    LogPrintLevel(BCLog::NET, BCLog::Level::Debug, "pcp: gateway [IPv4]: %s\n", gateway4->ToStringAddr());

    struct in_addr inaddr_any;
    inaddr_any.s_addr = htonl(INADDR_ANY);
    PCPRequestPortMap(*gateway4, CNetAddr(inaddr_any), 1234, 100);

    // Collect routable local IPv6 addresses.
    std::vector<CNetAddr> my_addrs;
    for (const auto &net_addr: GetLocalAddresses()) {
        if (net_addr.IsRoutable() && net_addr.IsIPv6())
            my_addrs.push_back(net_addr);
    }
    if (my_addrs.empty()) {
        LogPrintLevel(BCLog::NET, BCLog::Level::Warning, "pcp: Did not find any routable local IPv6 address\n");
        exit(1);
    }

    // IPv6
    std::optional<CNetAddr> gateway = FindIPv6DefaultGateway();
    if (!gateway) {
        LogPrintLevel(BCLog::NET, BCLog::Level::Warning, "pcp: Could not determine IPv6 default gateway\n");
        exit(1);
    }

    LogPrintLevel(BCLog::NET, BCLog::Level::Debug, "pcp: gateway [IPv6]: %s\n", gateway->ToStringAddr());

    // Try open pinholes for all addresses.
    for (const auto &addr: my_addrs) {
        PCPRequestPortMap(*gateway, addr, 1234, 100);
    }

    return 0;
}
