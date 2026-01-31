// Copyright (c) 2021-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <netgroup.h>

#include <hash.h>
#include <logging.h>
#include <uint256.h>
#include <util/asmap.h>

#include <cstddef>

uint256 NetGroupManager::GetAsmapVersion() const
{
    return AsmapVersion(m_asmap);
}

std::vector<unsigned char> NetGroupManager::GetGroup(const CNetAddr& address) const
{
    std::vector<unsigned char> vchRet;
    // If non-empty asmap is supplied and the address is IPv4/IPv6,
    // return ASN to be used for bucketing.
    uint32_t asn = GetMappedAS(address);
    if (asn != 0) { // Either asmap was empty, or address has non-asmappable net class (e.g. TOR).
        vchRet.push_back(NET_IPV6); // IPv4 and IPv6 with same ASN should be in the same bucket
        for (int i = 0; i < 4; i++) {
            vchRet.push_back((asn >> (8 * i)) & 0xFF);
        }
        return vchRet;
    }

    vchRet.push_back(address.GetNetClass());
    int nStartByte{0};
    int nBits{0};

    if (address.IsLocal()) {
        // all local addresses belong to the same group
    } else if (address.IsInternal()) {
        // All internal-usage addresses get their own group.
        // Skip over the INTERNAL_IN_IPV6_PREFIX returned by CAddress::GetAddrBytes().
        nStartByte = INTERNAL_IN_IPV6_PREFIX.size();
        nBits = ADDR_INTERNAL_SIZE * 8;
    } else if (!address.IsRoutable()) {
        // all other unroutable addresses belong to the same group
    } else if (address.HasLinkedIPv4()) {
        // IPv4 addresses (and mapped IPv4 addresses) use /16 groups
        uint32_t ipv4 = address.GetLinkedIPv4();
        vchRet.push_back((ipv4 >> 24) & 0xFF);
        vchRet.push_back((ipv4 >> 16) & 0xFF);
        return vchRet;
    } else if (address.IsTor() || address.IsI2P()) {
        nBits = 4;
    } else if (address.IsCJDNS()) {
        // Treat in the same way as Tor and I2P because the address in all of
        // them is "random" bytes (derived from a public key). However in CJDNS
        // the first byte is a constant (see CJDNS_PREFIX), so the random bytes
        // come after it. Thus skip the constant 8 bits at the start.
        nBits = 12;
    } else if (address.IsHeNet()) {
        // for he.net, use /36 groups
        nBits = 36;
    } else {
        // for the rest of the IPv6 network, use /32 groups
        nBits = 32;
    }

    // Push our address onto vchRet.
    auto addr_bytes = address.GetAddrBytes();
    const size_t num_bytes = nBits / 8;
    vchRet.insert(vchRet.end(), addr_bytes.begin() + nStartByte, addr_bytes.begin() + nStartByte + num_bytes);
    nBits %= 8;
    // ...for the last byte, push nBits and for the rest of the byte push 1's
    if (nBits > 0) {
        assert(num_bytes < addr_bytes.size());
        vchRet.push_back(addr_bytes[num_bytes + nStartByte] | ((1 << (8 - nBits)) - 1));
    }

    return vchRet;
}

uint32_t NetGroupManager::GetMappedAS(const CNetAddr& address) const
{
    uint32_t net_class = address.GetNetClass();
    if (m_asmap.empty() || (net_class != NET_IPV4 && net_class != NET_IPV6)) {
        return 0; // Indicates not found, safe because AS0 is reserved per RFC7607.
    }
    std::vector<std::byte> ip_bytes(16);
    if (address.HasLinkedIPv4()) {
        // For lookup, treat as if it was just an IPv4 address (IPV4_IN_IPV6_PREFIX + IPv4 bits)
        std::copy_n(std::as_bytes(std::span{IPV4_IN_IPV6_PREFIX}).begin(),
                    IPV4_IN_IPV6_PREFIX.size(), ip_bytes.begin());
        uint32_t ipv4 = address.GetLinkedIPv4();
        for (int i = 0; i < 4; ++i) {
            ip_bytes[12 + i] = std::byte((ipv4 >> (24 - i * 8)) & 0xFF);
        }
    } else {
        // Use all 128 bits of the IPv6 address otherwise
        assert(address.IsIPv6());
        auto addr_bytes = address.GetAddrBytes();
        assert(addr_bytes.size() == ip_bytes.size());
        std::copy_n(std::as_bytes(std::span{addr_bytes}).begin(),
                    addr_bytes.size(), ip_bytes.begin());
    }
    uint32_t mapped_as = Interpret(m_asmap, ip_bytes);
    return mapped_as;
}

void NetGroupManager::ASMapHealthCheck(const std::vector<CNetAddr>& clearnet_addrs) const {
    std::set<uint32_t> clearnet_asns{};
    int unmapped_count{0};

    for (const auto& addr : clearnet_addrs) {
        uint32_t asn = GetMappedAS(addr);
        if (asn == 0) {
            ++unmapped_count;
            continue;
        }
        clearnet_asns.insert(asn);
    }

    LogInfo("ASMap Health Check: %i clearnet peers are mapped to %i ASNs with %i peers being unmapped\n", clearnet_addrs.size(), clearnet_asns.size(), unmapped_count);
}

bool NetGroupManager::UsingASMap() const {
    return m_asmap.size() > 0;
}
