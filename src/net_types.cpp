// Copyright (c) 2021-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <netgroup.h>

#include <hash.h>
#include <logging.h>
#include <util/asmap.h>

uint256 NetGroupManager::GetAsmapChecksum() const
{
    if (m_asmap.empty()) return {};
    return (HashWriter{} << m_asmap).GetHash();
}

std::vector<unsigned char> NetGroupManager::GetGroup(const CNetAddr& address) const
{
    std::vector<unsigned char> vchRet;
    
    // Try to get ASN first if asmap is available
    if (const uint32_t asn = GetMappedAS(address); asn != 0) {
        vchRet.push_back(NET_IPV6); // IPv4 and IPv6 with same ASN in same bucket
        for (int i = 0; i < 4; ++i) {
            vchRet.push_back(static_cast<unsigned char>((asn >> (8 * i)) & 0xFF));
        }
        return vchRet;
    }

    const uint32_t net_class = address.GetNetClass();
    vchRet.push_back(static_cast<unsigned char>(net_class));

    if (address.IsLocal() || !address.IsRoutable()) {
        // All local or unroutable addresses in same group
        return vchRet;
    }

    if (address.IsInternal()) {
        // Internal addresses get their own group
        const auto& prefix = INTERNAL_IN_IPV6_PREFIX;
        const auto addr_bytes = address.GetAddrBytes();
        vchRet.insert(vchRet.end(), addr_bytes.begin() + prefix.size(), 
                     addr_bytes.begin() + prefix.size() + ADDR_INTERNAL_SIZE);
        return vchRet;
    }

    if (address.HasLinkedIPv4()) {
        // IPv4 addresses use /16 groups
        const uint32_t ipv4 = address.GetLinkedIPv4();
        vchRet.push_back(static_cast<unsigned char>((ipv4 >> 24) & 0xFF));
        vchRet.push_back(static_cast<unsigned char>((ipv4 >> 16) & 0xFF));
        return vchRet;
    }

    // Handle other address types
    int nBits = 0;
    int nStartByte = 0;

    if (address.IsTor() || address.IsI2P()) {
        nBits = 4;
    } 
    else if (address.IsCJDNS()) {
        nBits = 12;
        nStartByte = 1; // Skip constant prefix byte
    } 
    else if (address.IsHeNet()) {
        nBits = 36;
    } 
    else {
        nBits = 32; // Default for other IPv6
    }

    // Add the relevant address bits
    const auto addr_bytes = address.GetAddrBytes();
    const size_t num_bytes = nBits / 8;
    
    if (num_bytes > 0) {
        vchRet.insert(vchRet.end(), addr_bytes.begin() + nStartByte, 
                     addr_bytes.begin() + nStartByte + num_bytes);
    }

    // Handle remaining bits
    if (const int remaining_bits = nBits % 8; remaining_bits > 0) {
        assert(num_bytes + nStartByte < addr_bytes.size());
        vchRet.push_back(addr_bytes[num_bytes + nStartByte] | ((1 << (8 - remaining_bits)) - 1));
    }

    return vchRet;
}

uint32_t NetGroupManager::GetMappedAS(const CNetAddr& address) const
{
    if (m_asmap.empty()) return 0;

    const uint32_t net_class = address.GetNetClass();
    if (net_class != NET_IPV4 && net_class != NET_IPV6) return 0;

    std::vector<bool> ip_bits(128);
    const auto addr_bytes = address.GetAddrBytes();

    if (address.HasLinkedIPv4()) {
        // Handle IPv4-mapped IPv6 addresses
        for (int byte_i = 0; byte_i < 12; ++byte_i) {
            const uint8_t byte = IPV4_IN_IPV6_PREFIX[byte_i];
            for (int bit_i = 0; bit_i < 8; ++bit_i) {
                ip_bits[byte_i * 8 + bit_i] = (byte >> (7 - bit_i)) & 1;
            }
        }
        
        const uint32_t ipv4 = address.GetLinkedIPv4();
        for (int i = 0; i < 32; ++i) {
            ip_bits[96 + i] = (ipv4 >> (31 - i)) & 1;
        }
    } else {
        // Handle full IPv6 addresses
        for (int byte_i = 0; byte_i < 16; ++byte_i) {
            const uint8_t byte = addr_bytes[byte_i];
            for (int bit_i = 0; bit_i < 8; ++bit_i) {
                ip_bits[byte_i * 8 + bit_i] = (byte >> (7 - bit_i)) & 1;
            }
        }
    }

    return Interpret(m_asmap, ip_bits);
}

void NetGroupManager::ASMapHealthCheck(const std::vector<CNetAddr>& clearnet_addrs) const
{
    std::set<uint32_t> clearnet_asns;
    int unmapped_count = 0;

    for (const auto& addr : clearnet_addrs) {
        if (const uint32_t asn = GetMappedAS(addr); asn != 0) {
            clearnet_asns.insert(asn);
        } else {
            ++unmapped_count;
        }
    }

    LogPrintf("ASMap Health Check: %i clearnet peers are mapped to %i ASNs with %i peers being unmapped\n", 
              clearnet_addrs.size(), clearnet_asns.size(), unmapped_count);
}

bool NetGroupManager::UsingASMap() const
{
    return !m_asmap.empty();
}
