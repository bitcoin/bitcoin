// Copyright (c) 2024 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_COMMON_PCP_H
#define BITCOIN_COMMON_PCP_H

#include <netaddress.h>

#include <variant>

// RFC6886 NAT-PMP and RFC6887 Port Control Protocol (PCP) implementation.
// NAT-PMP and PCP use network byte order (big-endian).

//! Mapping nonce size in bytes (see RFC6887 section 11.1).
constexpr size_t PCP_MAP_NONCE_SIZE = 12;

//! PCP mapping nonce. Arbitrary data chosen by the client to identify a mapping.
typedef std::array<uint8_t, PCP_MAP_NONCE_SIZE> PCPMappingNonce;

//! Unsuccessful response to a port mapping.
enum class MappingError {
    NETWORK_ERROR,  ///< Any kind of network-level error.
    PROTOCOL_ERROR, ///< Any kind of protocol-level error, except unsupported version or no resources.
    UNSUPP_VERSION, ///< Unsupported protocol version.
    NO_RESOURCES,   ///< No resources available (port probably already mapped).
};

//! Successful response to a port mapping.
struct MappingResult {
    MappingResult(uint8_t version, const CService &internal_in, const CService &external_in, uint32_t lifetime_in):
        version(version), internal(internal_in), external(external_in), lifetime(lifetime_in) {}
    //! Protocol version, one of NATPMP_VERSION or PCP_VERSION.
    uint8_t version;
    //! Internal host:port.
    CService internal;
    //! External host:port.
    CService external;
    //! Granted lifetime of binding (seconds).
    uint32_t lifetime;

    //! Format mapping as string for logging.
    std::string ToString() const;
};

//! Try to open a port using RFC 6886 NAT-PMP. IPv4 only.
//!
//! * gateway: Destination address for PCP requests (usually the default gateway).
//! * port: Internal port, and desired external port.
//! * lifetime: Requested lifetime in seconds for mapping. The server may assign as shorter or longer lifetime. A lifetime of 0 deletes the mapping.
//! * num_tries: Number of tries in case of no response.
//!
//! Returns the external_ip:external_port of the mapping if successful, otherwise a MappingError.
std::variant<MappingResult, MappingError> NATPMPRequestPortMap(const CNetAddr &gateway, uint16_t port, uint32_t lifetime, int num_tries = 3, std::chrono::milliseconds timeout_per_try = std::chrono::milliseconds(1000));

//! Try to open a port using RFC 6887 Port Control Protocol (PCP). Handles IPv4 and IPv6.
//!
//! * nonce: Mapping cookie. Keep this the same over renewals.
//! * gateway: Destination address for PCP requests (usually the default gateway).
//! * bind: Specific local bind address for IPv6 pinholing. Set this as INADDR_ANY for IPv4.
//! * port: Internal port, and desired external port.
//! * lifetime: Requested lifetime in seconds for mapping. The server may assign as shorter or longer lifetime. A lifetime of 0 deletes the mapping.
//! * num_tries: Number of tries in case of no response.
//!
//! Returns the external_ip:external_port of the mapping if successful, otherwise a MappingError.
std::variant<MappingResult, MappingError> PCPRequestPortMap(const PCPMappingNonce &nonce, const CNetAddr &gateway, const CNetAddr &bind, uint16_t port, uint32_t lifetime, int num_tries = 3, std::chrono::milliseconds timeout_per_try = std::chrono::milliseconds(1000));

#endif // BITCOIN_COMMON_PCP_H
