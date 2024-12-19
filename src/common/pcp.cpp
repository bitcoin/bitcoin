// Copyright (c) 2024-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://www.opensource.org/licenses/mit-license.php.

#include <common/pcp.h>

#include <common/netif.h>
#include <crypto/common.h>
#include <logging.h>
#include <netaddress.h>
#include <netbase.h>
#include <random.h>
#include <span.h>
#include <util/check.h>
#include <util/readwritefile.h>
#include <util/sock.h>
#include <util/strencodings.h>

namespace {

// RFC6886 NAT-PMP and RFC6887 Port Control Protocol (PCP) implementation.
// NAT-PMP and PCP use network byte order (big-endian).

// NAT-PMP (v0) protocol constants.
//! NAT-PMP uses a fixed server port number (RFC6887 section 1.1).
constexpr uint16_t NATPMP_SERVER_PORT = 5351;
//! Version byte for NATPMP (RFC6886 1.1)
constexpr uint8_t NATPMP_VERSION = 0;
//! Request opcode base (RFC6886 3).
constexpr uint8_t NATPMP_REQUEST = 0x00;
//! Response opcode base (RFC6886 3).
constexpr uint8_t NATPMP_RESPONSE = 0x80;
//! Get external address (RFC6886 3.2)
constexpr uint8_t NATPMP_OP_GETEXTERNAL = 0x00;
//! Map TCP port (RFC6886 3.3)
constexpr uint8_t NATPMP_OP_MAP_TCP = 0x02;
//! Shared request header size in bytes.
constexpr size_t NATPMP_REQUEST_HDR_SIZE = 2;
//! Shared response header (minimum) size in bytes.
constexpr size_t NATPMP_RESPONSE_HDR_SIZE = 8;
//! GETEXTERNAL request size in bytes, including header (RFC6886 3.2).
constexpr size_t NATPMP_GETEXTERNAL_REQUEST_SIZE = NATPMP_REQUEST_HDR_SIZE + 0;
//! GETEXTERNAL response size in bytes, including header (RFC6886 3.2).
constexpr size_t NATPMP_GETEXTERNAL_RESPONSE_SIZE = NATPMP_RESPONSE_HDR_SIZE + 4;
//! MAP request size in bytes, including header (RFC6886 3.3).
constexpr size_t NATPMP_MAP_REQUEST_SIZE = NATPMP_REQUEST_HDR_SIZE + 10;
//! MAP response size in bytes, including header (RFC6886 3.3).
constexpr size_t NATPMP_MAP_RESPONSE_SIZE = NATPMP_RESPONSE_HDR_SIZE + 8;

// Shared header offsets (RFC6886 3.2, 3.3), relative to start of packet.
//!  Offset of version field in packets.
constexpr size_t NATPMP_HDR_VERSION_OFS = 0;
//!  Offset of opcode field in packets
constexpr size_t NATPMP_HDR_OP_OFS = 1;
//!  Offset of result code in packets. Result codes are 16 bit in NAT-PMP instead of 8 bit in PCP.
constexpr size_t NATPMP_RESPONSE_HDR_RESULT_OFS = 2;

// GETEXTERNAL response offsets (RFC6886 3.2), relative to start of packet.
//!  Returned external address
constexpr size_t NATPMP_GETEXTERNAL_RESPONSE_IP_OFS = 8;

// MAP request offsets (RFC6886 3.3), relative to start of packet.
//!  Internal port to be mapped.
constexpr size_t NATPMP_MAP_REQUEST_INTERNAL_PORT_OFS = 4;
//!  Suggested external port for mapping.
constexpr size_t NATPMP_MAP_REQUEST_EXTERNAL_PORT_OFS = 6;
//!  Requested port mapping lifetime in seconds.
constexpr size_t NATPMP_MAP_REQUEST_LIFETIME_OFS = 8;

// MAP response offsets (RFC6886 3.3), relative to start of packet.
//!  Internal port for mapping (will match internal port of request).
constexpr size_t NATPMP_MAP_RESPONSE_INTERNAL_PORT_OFS = 8;
//!  External port for mapping.
constexpr size_t NATPMP_MAP_RESPONSE_EXTERNAL_PORT_OFS = 10;
//!  Created port mapping lifetime in seconds.
constexpr size_t NATPMP_MAP_RESPONSE_LIFETIME_OFS = 12;

// Relevant NETPMP result codes (RFC6886 3.5).
//! Result code representing success status.
constexpr uint8_t NATPMP_RESULT_SUCCESS = 0;
//! Result code representing unsupported version.
constexpr uint8_t NATPMP_RESULT_UNSUPP_VERSION = 1;
//! Result code representing lack of resources.
constexpr uint8_t NATPMP_RESULT_NO_RESOURCES = 4;

//! Mapping of NATPMP result code to string (RFC6886 3.5). Result codes <=2 match PCP.
const std::map<uint16_t, std::string> NATPMP_RESULT_STR{
    {0,  "SUCCESS"},
    {1,  "UNSUPP_VERSION"},
    {2,  "NOT_AUTHORIZED"},
    {3,  "NETWORK_FAILURE"},
    {4,  "NO_RESOURCES"},
    {5,  "UNSUPP_OPCODE"},
};

// PCP (v2) protocol constants.
//! Maximum packet size in bytes (RFC6887 section 7).
constexpr size_t PCP_MAX_SIZE = 1100;
//! PCP uses a fixed server port number (RFC6887 section 19.1). Shared with NAT-PMP.
constexpr uint16_t PCP_SERVER_PORT = NATPMP_SERVER_PORT;
//! Version byte. 0 is NAT-PMP (RFC6886), 1 is forbidden, 2 for PCP (RFC6887).
constexpr uint8_t PCP_VERSION = 2;
//! PCP Request Header. See RFC6887 section 7.1. Shared with NAT-PMP.
constexpr uint8_t PCP_REQUEST = NATPMP_REQUEST; // R = 0
//! PCP Response Header. See RFC6887 section 7.2. Shared with NAT-PMP.
constexpr uint8_t PCP_RESPONSE = NATPMP_RESPONSE; // R = 1
//! Map opcode. See RFC6887 section 19.2
constexpr uint8_t PCP_OP_MAP = 0x01;
//! TCP protocol number (IANA).
constexpr uint16_t PCP_PROTOCOL_TCP = 6;
//! Request and response header size in bytes (RFC6887 section 7.1).
constexpr size_t PCP_HDR_SIZE = 24;
//! Map request and response size in bytes (RFC6887 section 11.1).
constexpr size_t PCP_MAP_SIZE = 36;

// Header offsets shared between request and responses (RFC6887 7.1, 7.2), relative to start of packet.
//!  Version field (1 byte).
constexpr size_t PCP_HDR_VERSION_OFS = NATPMP_HDR_VERSION_OFS;
//!  Opcode field (1 byte).
constexpr size_t PCP_HDR_OP_OFS = NATPMP_HDR_OP_OFS;
//!  Requested lifetime (request), granted lifetime (response) (4 bytes).
constexpr size_t PCP_HDR_LIFETIME_OFS = 4;

// Request header offsets (RFC6887 7.1), relative to start of packet.
//!  PCP client's IP address (16 bytes).
constexpr size_t PCP_REQUEST_HDR_IP_OFS = 8;

// Response header offsets (RFC6887 7.2), relative to start of packet.
//!  Result code (1 byte).
constexpr size_t PCP_RESPONSE_HDR_RESULT_OFS = 3;

// MAP request/response offsets (RFC6887 11.1), relative to start of opcode-specific data.
//!  Mapping nonce (12 bytes).
constexpr size_t PCP_MAP_NONCE_OFS = 0;
//!  Protocol (1 byte).
constexpr size_t PCP_MAP_PROTOCOL_OFS = 12;
//!  Internal port for mapping (2 bytes).
constexpr size_t PCP_MAP_INTERNAL_PORT_OFS = 16;
//!  Suggested external port (request), assigned external port (response) (2 bytes).
constexpr size_t PCP_MAP_EXTERNAL_PORT_OFS = 18;
//!  Suggested external IP (request), assigned external IP (response) (16 bytes).
constexpr size_t PCP_MAP_EXTERNAL_IP_OFS = 20;

//! Result code representing success (RFC6887 7.4), shared with NAT-PMP.
constexpr uint8_t PCP_RESULT_SUCCESS = NATPMP_RESULT_SUCCESS;
//! Result code representing lack of resources (RFC6887 7.4).
constexpr uint8_t PCP_RESULT_NO_RESOURCES = 8;

//! Mapping of PCP result code to string (RFC6887 7.4). Result codes <=2 match NAT-PMP.
const std::map<uint8_t, std::string> PCP_RESULT_STR{
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

//! Return human-readable string from NATPMP result code.
std::string NATPMPResultString(uint16_t result_code)
{
    auto result_i = NATPMP_RESULT_STR.find(result_code);
    return strprintf("%s (code %d)", result_i == NATPMP_RESULT_STR.end() ? "(unknown)" : result_i->second,  result_code);
}

//! Return human-readable string from PCP result code.
std::string PCPResultString(uint8_t result_code)
{
    auto result_i = PCP_RESULT_STR.find(result_code);
    return strprintf("%s (code %d)", result_i == PCP_RESULT_STR.end() ? "(unknown)" : result_i->second,  result_code);
}

//! Wrap address in IPv6 according to RFC6887. wrapped_addr needs to be able to store 16 bytes.
[[nodiscard]] bool PCPWrapAddress(std::span<uint8_t> wrapped_addr, const CNetAddr &addr)
{
    Assume(wrapped_addr.size() == ADDR_IPV6_SIZE);
    if (addr.IsIPv4()) {
        struct in_addr addr4;
        if (!addr.GetInAddr(&addr4)) return false;
        // Section 5: "When the address field holds an IPv4 address, an IPv4-mapped IPv6 address [RFC4291] is used (::ffff:0:0/96)."
        std::memcpy(wrapped_addr.data(), IPV4_IN_IPV6_PREFIX.data(), IPV4_IN_IPV6_PREFIX.size());
        std::memcpy(wrapped_addr.data() + IPV4_IN_IPV6_PREFIX.size(), &addr4, ADDR_IPV4_SIZE);
        return true;
    } else if (addr.IsIPv6()) {
        struct in6_addr addr6;
        if (!addr.GetIn6Addr(&addr6)) return false;
        std::memcpy(wrapped_addr.data(), &addr6, ADDR_IPV6_SIZE);
        return true;
    } else {
        return false;
    }
}

//! Unwrap PCP-encoded address according to RFC6887.
CNetAddr PCPUnwrapAddress(std::span<const uint8_t> wrapped_addr)
{
    Assume(wrapped_addr.size() == ADDR_IPV6_SIZE);
    if (util::HasPrefix(wrapped_addr, IPV4_IN_IPV6_PREFIX)) {
        struct in_addr addr4;
        std::memcpy(&addr4, wrapped_addr.data() + IPV4_IN_IPV6_PREFIX.size(), ADDR_IPV4_SIZE);
        return CNetAddr(addr4);
    } else {
        struct in6_addr addr6;
        std::memcpy(&addr6, wrapped_addr.data(), ADDR_IPV6_SIZE);
        return CNetAddr(addr6);
    }
}

//! PCP or NAT-PMP send-receive loop.
std::optional<std::vector<uint8_t>> PCPSendRecv(Sock &sock, const std::string &protocol, std::span<const uint8_t> request, int num_tries,
        std::chrono::milliseconds timeout_per_try,
        std::function<bool(std::span<const uint8_t>)> check_packet)
{
    using namespace std::chrono;
    // UDP is a potentially lossy protocol, so we try to send again a few times.
    uint8_t response[PCP_MAX_SIZE];
    bool got_response = false;
    int recvsz = 0;
    for (int ntry = 0; !got_response && ntry < num_tries; ++ntry) {
        if (ntry > 0) {
            LogPrintLevel(BCLog::NET, BCLog::Level::Debug, "%s: Retrying (%d)\n", protocol, ntry);
        }
        // Dispatch packet to gateway.
        if (sock.Send(request.data(), request.size(), 0) != static_cast<ssize_t>(request.size())) {
            LogPrintLevel(BCLog::NET, BCLog::Level::Warning, "%s: Could not send request: %s\n", protocol, NetworkErrorString(WSAGetLastError()));
            return std::nullopt; // Network-level error, probably no use retrying.
        }

        // Wait for response(s) until we get a valid response, a network error, or time out.
        auto cur_time = time_point_cast<milliseconds>(MockableSteadyClock::now());
        auto deadline = cur_time + timeout_per_try;
        while ((cur_time = time_point_cast<milliseconds>(MockableSteadyClock::now())) < deadline) {
            Sock::Event occurred = 0;
            if (!sock.Wait(deadline - cur_time, Sock::RECV, &occurred)) {
                LogPrintLevel(BCLog::NET, BCLog::Level::Warning, "%s: Could not wait on socket: %s\n", protocol, NetworkErrorString(WSAGetLastError()));
                return std::nullopt; // Network-level error, probably no use retrying.
            }
            if (!occurred) {
                LogPrintLevel(BCLog::NET, BCLog::Level::Debug, "%s: Timeout\n", protocol);
                break; // Retry.
            }

            // Receive response.
            recvsz = sock.Recv(response, sizeof(response), MSG_DONTWAIT);
            if (recvsz < 0) {
                LogPrintLevel(BCLog::NET, BCLog::Level::Warning, "%s: Could not receive response: %s\n", protocol, NetworkErrorString(WSAGetLastError()));
                return std::nullopt; // Network-level error, probably no use retrying.
            }
            LogPrintLevel(BCLog::NET, BCLog::Level::Debug, "%s: Received response of %d bytes: %s\n", protocol, recvsz, HexStr(std::span(response, recvsz)));

            if (check_packet(std::span<uint8_t>(response, recvsz))) {
                got_response = true; // Got expected response, break from receive loop as well as from retry loop.
                break;
            }
        }
    }
    if (!got_response) {
        LogPrintLevel(BCLog::NET, BCLog::Level::Debug, "%s: Giving up after %d tries\n", protocol, num_tries);
        return std::nullopt;
    }
    return std::vector<uint8_t>(response, response + recvsz);
}

}

std::variant<MappingResult, MappingError> NATPMPRequestPortMap(const CNetAddr &gateway, uint16_t port, uint32_t lifetime, int num_tries, std::chrono::milliseconds timeout_per_try)
{
    struct sockaddr_storage dest_addr;
    socklen_t dest_addrlen = sizeof(struct sockaddr_storage);

    LogPrintLevel(BCLog::NET, BCLog::Level::Debug, "natpmp: Requesting port mapping port %d from gateway %s\n", port, gateway.ToStringAddr());

    // Validate gateway, make sure it's IPv4. NAT-PMP does not support IPv6.
    if (!CService(gateway, PCP_SERVER_PORT).GetSockAddr((struct sockaddr*)&dest_addr, &dest_addrlen)) return MappingError::NETWORK_ERROR;
    if (dest_addr.ss_family != AF_INET) return MappingError::NETWORK_ERROR;

    // Create IPv4 UDP socket
    auto sock{CreateSock(AF_INET, SOCK_DGRAM, IPPROTO_UDP)};
    if (!sock) {
        LogPrintLevel(BCLog::NET, BCLog::Level::Warning, "natpmp: Could not create UDP socket: %s\n", NetworkErrorString(WSAGetLastError()));
        return MappingError::NETWORK_ERROR;
    }

    // Associate UDP socket to gateway.
    if (sock->Connect((struct sockaddr*)&dest_addr, dest_addrlen) != 0) {
        LogPrintLevel(BCLog::NET, BCLog::Level::Warning, "natpmp: Could not connect to gateway: %s\n", NetworkErrorString(WSAGetLastError()));
        return MappingError::NETWORK_ERROR;
    }

    // Use getsockname to get the address toward the default gateway (the internal address).
    struct sockaddr_in internal;
    socklen_t internal_addrlen = sizeof(struct sockaddr_in);
    if (sock->GetSockName((struct sockaddr*)&internal, &internal_addrlen) != 0) {
        LogPrintLevel(BCLog::NET, BCLog::Level::Warning, "natpmp: Could not get sock name: %s\n", NetworkErrorString(WSAGetLastError()));
        return MappingError::NETWORK_ERROR;
    }

    // Request external IP address (RFC6886 section 3.2).
    std::vector<uint8_t> request(NATPMP_GETEXTERNAL_REQUEST_SIZE);
    request[NATPMP_HDR_VERSION_OFS] = NATPMP_VERSION;
    request[NATPMP_HDR_OP_OFS] = NATPMP_REQUEST | NATPMP_OP_GETEXTERNAL;

    auto recv_res = PCPSendRecv(*sock, "natpmp", request, num_tries, timeout_per_try,
        [&](const std::span<const uint8_t> response) -> bool {
            if (response.size() < NATPMP_GETEXTERNAL_RESPONSE_SIZE) {
                LogPrintLevel(BCLog::NET, BCLog::Level::Warning, "natpmp: Response too small\n");
                return false; // Wasn't response to what we expected, try receiving next packet.
            }
            if (response[NATPMP_HDR_VERSION_OFS] != NATPMP_VERSION || response[NATPMP_HDR_OP_OFS] != (NATPMP_RESPONSE | NATPMP_OP_GETEXTERNAL)) {
                LogPrintLevel(BCLog::NET, BCLog::Level::Warning, "natpmp: Response to wrong command\n");
                return false; // Wasn't response to what we expected, try receiving next packet.
            }
            return true;
        });

    struct in_addr external_addr;
    if (recv_res) {
        const std::span<const uint8_t> response = *recv_res;

        Assume(response.size() >= NATPMP_GETEXTERNAL_RESPONSE_SIZE);
        uint16_t result_code = ReadBE16(response.data() + NATPMP_RESPONSE_HDR_RESULT_OFS);
        if (result_code != NATPMP_RESULT_SUCCESS) {
            LogPrintLevel(BCLog::NET, BCLog::Level::Warning, "natpmp: Getting external address failed with result %s\n", NATPMPResultString(result_code));
            return MappingError::PROTOCOL_ERROR;
        }

        std::memcpy(&external_addr, response.data() + NATPMP_GETEXTERNAL_RESPONSE_IP_OFS, ADDR_IPV4_SIZE);
    } else {
        return MappingError::NETWORK_ERROR;
    }

    // Create TCP mapping request (RFC6886 section 3.3).
    request = std::vector<uint8_t>(NATPMP_MAP_REQUEST_SIZE);
    request[NATPMP_HDR_VERSION_OFS] = NATPMP_VERSION;
    request[NATPMP_HDR_OP_OFS] = NATPMP_REQUEST | NATPMP_OP_MAP_TCP;
    WriteBE16(request.data() + NATPMP_MAP_REQUEST_INTERNAL_PORT_OFS, port);
    WriteBE16(request.data() + NATPMP_MAP_REQUEST_EXTERNAL_PORT_OFS, port);
    WriteBE32(request.data() + NATPMP_MAP_REQUEST_LIFETIME_OFS, lifetime);

    recv_res = PCPSendRecv(*sock, "natpmp", request, num_tries, timeout_per_try,
        [&](const std::span<const uint8_t> response) -> bool {
            if (response.size() < NATPMP_MAP_RESPONSE_SIZE) {
                LogPrintLevel(BCLog::NET, BCLog::Level::Warning, "natpmp: Response too small\n");
                return false; // Wasn't response to what we expected, try receiving next packet.
            }
            if (response[0] != NATPMP_VERSION || response[1] != (NATPMP_RESPONSE | NATPMP_OP_MAP_TCP)) {
                LogPrintLevel(BCLog::NET, BCLog::Level::Warning, "natpmp: Response to wrong command\n");
                return false; // Wasn't response to what we expected, try receiving next packet.
            }
            uint16_t internal_port = ReadBE16(response.data() + NATPMP_MAP_RESPONSE_INTERNAL_PORT_OFS);
            if (internal_port != port) {
                LogPrintLevel(BCLog::NET, BCLog::Level::Warning, "natpmp: Response port doesn't match request\n");
                return false; // Wasn't response to what we expected, try receiving next packet.
            }
            return true;
        });

    if (recv_res) {
        const std::span<uint8_t> response = *recv_res;

        Assume(response.size() >= NATPMP_MAP_RESPONSE_SIZE);
        uint16_t result_code = ReadBE16(response.data() + NATPMP_RESPONSE_HDR_RESULT_OFS);
        if (result_code != NATPMP_RESULT_SUCCESS) {
            LogPrintLevel(BCLog::NET, BCLog::Level::Warning, "natpmp: Port mapping failed with result %s\n", NATPMPResultString(result_code));
            if (result_code == NATPMP_RESULT_NO_RESOURCES) {
                return MappingError::NO_RESOURCES;
            }
            return MappingError::PROTOCOL_ERROR;
        }

        uint32_t lifetime_ret = ReadBE32(response.data() + NATPMP_MAP_RESPONSE_LIFETIME_OFS);
        uint16_t external_port = ReadBE16(response.data() + NATPMP_MAP_RESPONSE_EXTERNAL_PORT_OFS);
        return MappingResult(NATPMP_VERSION, CService(internal.sin_addr, port), CService(external_addr, external_port), lifetime_ret);
    } else {
        return MappingError::NETWORK_ERROR;
    }
}

std::variant<MappingResult, MappingError> PCPRequestPortMap(const PCPMappingNonce &nonce, const CNetAddr &gateway, const CNetAddr &bind, uint16_t port, uint32_t lifetime, int num_tries, std::chrono::milliseconds timeout_per_try)
{
    struct sockaddr_storage dest_addr, bind_addr;
    socklen_t dest_addrlen = sizeof(struct sockaddr_storage), bind_addrlen = sizeof(struct sockaddr_storage);

    LogPrintLevel(BCLog::NET, BCLog::Level::Debug, "pcp: Requesting port mapping for addr %s port %d from gateway %s\n", bind.ToStringAddr(), port, gateway.ToStringAddr());

    // Validate addresses, make sure they're the same network family.
    if (!CService(gateway, PCP_SERVER_PORT).GetSockAddr((struct sockaddr*)&dest_addr, &dest_addrlen)) return MappingError::NETWORK_ERROR;
    if (!CService(bind, 0).GetSockAddr((struct sockaddr*)&bind_addr, &bind_addrlen)) return MappingError::NETWORK_ERROR;
    if (dest_addr.ss_family != bind_addr.ss_family) return MappingError::NETWORK_ERROR;

    // Create UDP socket (IPv4 or IPv6 based on provided gateway).
    auto sock{CreateSock(dest_addr.ss_family, SOCK_DGRAM, IPPROTO_UDP)};
    if (!sock) {
        LogPrintLevel(BCLog::NET, BCLog::Level::Warning, "pcp: Could not create UDP socket: %s\n", NetworkErrorString(WSAGetLastError()));
        return MappingError::NETWORK_ERROR;
    }

    // Make sure that we send from requested destination address, anything else will be
    // rejected by a security-conscious router.
    if (sock->Bind((struct sockaddr*)&bind_addr, bind_addrlen) != 0) {
        LogPrintLevel(BCLog::NET, BCLog::Level::Warning, "pcp: Could not bind to address: %s\n", NetworkErrorString(WSAGetLastError()));
        return MappingError::NETWORK_ERROR;
    }

    // Associate UDP socket to gateway.
    if (sock->Connect((struct sockaddr*)&dest_addr, dest_addrlen) != 0) {
        LogPrintLevel(BCLog::NET, BCLog::Level::Warning, "pcp: Could not connect to gateway: %s\n", NetworkErrorString(WSAGetLastError()));
        return MappingError::NETWORK_ERROR;
    }

    // Use getsockname to get the address toward the default gateway (the internal address),
    // in case we don't know what address to map
    // (this is only needed if bind is INADDR_ANY, but it doesn't hurt as an extra check).
    struct sockaddr_storage internal_addr;
    socklen_t internal_addrlen = sizeof(struct sockaddr_storage);
    if (sock->GetSockName((struct sockaddr*)&internal_addr, &internal_addrlen) != 0) {
        LogPrintLevel(BCLog::NET, BCLog::Level::Warning, "pcp: Could not get sock name: %s\n", NetworkErrorString(WSAGetLastError()));
        return MappingError::NETWORK_ERROR;
    }
    CService internal;
    if (!internal.SetSockAddr((struct sockaddr*)&internal_addr, internal_addrlen)) return MappingError::NETWORK_ERROR;
    LogPrintLevel(BCLog::NET, BCLog::Level::Debug, "pcp: Internal address after connect: %s\n", internal.ToStringAddr());

    // Build request packet. Make sure the packet is zeroed so that reserved fields are zero
    // as required by the spec (and not potentially leak data).
    // Make sure there's space for the request header and MAP specific request data.
    std::vector<uint8_t> request(PCP_HDR_SIZE + PCP_MAP_SIZE);
    // Fill in request header, See RFC6887 Figure 2.
    size_t ofs = 0;
    request[ofs + PCP_HDR_VERSION_OFS] = PCP_VERSION;
    request[ofs + PCP_HDR_OP_OFS] = PCP_REQUEST | PCP_OP_MAP;
    WriteBE32(request.data() + ofs + PCP_HDR_LIFETIME_OFS, lifetime);
    if (!PCPWrapAddress(std::span(request).subspan(ofs + PCP_REQUEST_HDR_IP_OFS, ADDR_IPV6_SIZE), internal)) return MappingError::NETWORK_ERROR;

    ofs += PCP_HDR_SIZE;

    // Fill in MAP request packet, See RFC6887 Figure 9.
    // Randomize mapping nonce (this is repeated in the response, to be able to
    // correlate requests and responses, and used to authenticate changes to the mapping).
    std::memcpy(request.data() + ofs + PCP_MAP_NONCE_OFS, nonce.data(), PCP_MAP_NONCE_SIZE);
    request[ofs + PCP_MAP_PROTOCOL_OFS] = PCP_PROTOCOL_TCP;
    WriteBE16(request.data() + ofs + PCP_MAP_INTERNAL_PORT_OFS, port);
    WriteBE16(request.data() + ofs + PCP_MAP_EXTERNAL_PORT_OFS, port);
    if (!PCPWrapAddress(std::span(request).subspan(ofs + PCP_MAP_EXTERNAL_IP_OFS, ADDR_IPV6_SIZE), bind)) return MappingError::NETWORK_ERROR;

    ofs += PCP_MAP_SIZE;
    Assume(ofs == request.size());

    // Receive loop.
    bool is_natpmp = false;
    auto recv_res = PCPSendRecv(*sock, "pcp", request, num_tries, timeout_per_try,
        [&](const std::span<const uint8_t> response) -> bool {
            // Unsupported version according to RFC6887 appendix A and RFC6886 section 3.5, can fall back to NAT-PMP.
            if (response.size() == NATPMP_RESPONSE_HDR_SIZE && response[PCP_HDR_VERSION_OFS] == NATPMP_VERSION && response[PCP_RESPONSE_HDR_RESULT_OFS] == NATPMP_RESULT_UNSUPP_VERSION) {
                is_natpmp = true;
                return true; // Let it through to caller.
            }
            if (response.size() < (PCP_HDR_SIZE + PCP_MAP_SIZE)) {
                LogPrintLevel(BCLog::NET, BCLog::Level::Warning, "pcp: Response too small\n");
                return false; // Wasn't response to what we expected, try receiving next packet.
            }
            if (response[PCP_HDR_VERSION_OFS] != PCP_VERSION || response[PCP_HDR_OP_OFS] != (PCP_RESPONSE | PCP_OP_MAP)) {
                LogPrintLevel(BCLog::NET, BCLog::Level::Warning, "pcp: Response to wrong command\n");
                return false; // Wasn't response to what we expected, try receiving next packet.
            }
            // Handle MAP opcode response. See RFC6887 Figure 10.
            // Check that returned mapping nonce matches our request.
            if (!std::ranges::equal(response.subspan(PCP_HDR_SIZE + PCP_MAP_NONCE_OFS, PCP_MAP_NONCE_SIZE), nonce)) {
                LogPrintLevel(BCLog::NET, BCLog::Level::Warning, "pcp: Mapping nonce mismatch\n");
                return false; // Wasn't response to what we expected, try receiving next packet.
            }
            uint8_t protocol = response[PCP_HDR_SIZE + 12];
            uint16_t internal_port = ReadBE16(response.data() + PCP_HDR_SIZE + 16);
            if (protocol != PCP_PROTOCOL_TCP || internal_port != port) {
                LogPrintLevel(BCLog::NET, BCLog::Level::Warning, "pcp: Response protocol or port doesn't match request\n");
                return false; // Wasn't response to what we expected, try receiving next packet.
            }
            return true;
        });

    if (!recv_res) {
        return MappingError::NETWORK_ERROR;
    }
    if (is_natpmp) {
        return MappingError::UNSUPP_VERSION;
    }

    const std::span<const uint8_t> response = *recv_res;
    // If we get here, we got a valid MAP response to our request.
    // Check to see if we got the result we expected.
    Assume(response.size() >= (PCP_HDR_SIZE + PCP_MAP_SIZE));
    uint8_t result_code = response[PCP_RESPONSE_HDR_RESULT_OFS];
    uint32_t lifetime_ret = ReadBE32(response.data() + PCP_HDR_LIFETIME_OFS);
    uint16_t external_port = ReadBE16(response.data() + PCP_HDR_SIZE + PCP_MAP_EXTERNAL_PORT_OFS);
    CNetAddr external_addr{PCPUnwrapAddress(response.subspan(PCP_HDR_SIZE + PCP_MAP_EXTERNAL_IP_OFS, ADDR_IPV6_SIZE))};
    if (result_code != PCP_RESULT_SUCCESS) {
        LogPrintLevel(BCLog::NET, BCLog::Level::Warning, "pcp: Mapping failed with result %s\n", PCPResultString(result_code));
        if (result_code == PCP_RESULT_NO_RESOURCES) {
            return MappingError::NO_RESOURCES;
        }
        return MappingError::PROTOCOL_ERROR;
    }

    return MappingResult(PCP_VERSION, CService(internal, port), CService(external_addr, external_port), lifetime_ret);
}

std::string MappingResult::ToString() const
{
    Assume(version == NATPMP_VERSION || version == PCP_VERSION);
    return strprintf("%s:%s -> %s (for %ds)",
            version == NATPMP_VERSION ? "natpmp" : "pcp",
            external.ToStringAddrPort(),
            internal.ToStringAddrPort(),
            lifetime
        );
}
