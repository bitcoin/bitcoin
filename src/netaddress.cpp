// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifdef HAVE_CONFIG_H
#include "config/bitcoin-config.h"
#endif

#include "netaddress.h"

#include "hash.h"
#include "tinyformat.h"
#include "utilstrencodings.h"

#include <algorithm>
#include <utility>

static const std::array<unsigned char, 12> IPV4_IP_PREFIX = {{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xff, 0xff }};
static const std::array<unsigned char, 6> ONION_IP_PREFIX = {{ 0xFD, 0x87, 0xD8, 0x7E, 0xEB, 0x43 }};

// 0xFD + sha256("bitcoin")[0:5]
static const std::array<unsigned char, 6> INTERNAL_IP_PREFIX = {{ 0xFD, 0x6B, 0x88, 0xC0, 0x87, 0x24 }};

static const std::string ONION_ADDRESS_SUFFIX {".onion"};

void CNetAddr::Init()
{
    ip.fill(0);
    scopeId = 0;
}

void CNetAddr::SetIP(CNetAddr ipIn)
{
    ip = ipIn.ip;
}

void CNetAddr::SetRawIPv4(const std::array<unsigned char, 4>& ip_in)
{
    assert(IPV4_IP_PREFIX.size() + ip_in.size() == ip.size());
    auto it = std::copy(IPV4_IP_PREFIX.begin(), IPV4_IP_PREFIX.end(), ip.begin());
    std::copy(ip_in.begin(), ip_in.end(), it);
}

void CNetAddr::SetRawIPv6(const std::array<unsigned char, 16>& ip_in)
{
    ip = ip_in;
}

bool CNetAddr::SetInternal(const std::string &name)
{
    if (name.empty()) {
        return false;
    }
    std::array<unsigned char, 32> hash {};
    CSHA256().Write(reinterpret_cast<const unsigned char*>(name.data()), name.size()).Finalize(hash.data());
    auto it = std::copy(INTERNAL_IP_PREFIX.begin(), INTERNAL_IP_PREFIX.end(), ip.begin());
    std::copy(hash.begin(), hash.begin() + (ip.size() - INTERNAL_IP_PREFIX.size()), it);
    return true;
}

bool CNetAddr::SetSpecial(const std::string &strName)
{
    if (strName.size() <= ONION_ADDRESS_SUFFIX.size() || !std::equal(ONION_ADDRESS_SUFFIX.begin(), ONION_ADDRESS_SUFFIX.end(), strName.end() - ONION_ADDRESS_SUFFIX.size())) {
        return false;
    }

    const auto vchAddr = DecodeBase32(strName.substr(0, strName.size() - ONION_ADDRESS_SUFFIX.size()).c_str());
    if (vchAddr.size() != ip.size() - ONION_IP_PREFIX.size()) {
        return false;
    }

    auto it = std::copy(ONION_IP_PREFIX.begin(), ONION_IP_PREFIX.end(), ip.begin());
    std::copy(vchAddr.begin(), vchAddr.end(), it);

    return true;
}

CNetAddr::CNetAddr()
{
    Init();
}

CNetAddr::CNetAddr(const struct in_addr& ipv4Addr)
{
    std::array<unsigned char, 4> ipAr;
    const auto* ipv4AddrBytes = reinterpret_cast<const unsigned char*>(&ipv4Addr);
    std::copy(ipv4AddrBytes, ipv4AddrBytes + ipAr.size(), ipAr.begin());
    SetRawIPv4(ipAr);
}

CNetAddr::CNetAddr(const struct in6_addr& ipv6Addr, const uint32_t scope)
{
    std::array<unsigned char, 16> ipAr;
    const auto* ipv6AddrBytes = reinterpret_cast<const unsigned char*>(&ipv6Addr);
    std::copy(ipv6AddrBytes, ipv6AddrBytes + ipAr.size(), ipAr.begin());
    SetRawIPv6(ipAr);
    scopeId = scope;
}

unsigned int CNetAddr::GetByte(unsigned int n) const
{
    return *(ip.end() - (n+1));
}

bool CNetAddr::IsIPv4() const
{
    return std::equal(IPV4_IP_PREFIX.begin(), IPV4_IP_PREFIX.end(), ip.begin());
}

bool CNetAddr::IsIPv6() const
{
    return !IsIPv4() && !IsTor() && !IsInternal();
}

bool CNetAddr::IsRFC1918() const
{
    return IsIPv4() && (
        GetByte(3) == 10 ||
        (GetByte(3) == 192 && GetByte(2) == 168) ||
        (GetByte(3) == 172 && GetByte(2) >= 16 && GetByte(2) <= 31));
}

bool CNetAddr::IsRFC2544() const
{
    return IsIPv4() && GetByte(3) == 198 && (GetByte(2) == 18 || GetByte(2) == 19);
}

bool CNetAddr::IsRFC3927() const
{
    return IsIPv4() && GetByte(3) == 169 && GetByte(2) == 254;
}

bool CNetAddr::IsRFC6598() const
{
    return IsIPv4() && GetByte(3) == 100 && GetByte(2) >= 64 && GetByte(2) <= 127;
}

bool CNetAddr::IsRFC5737() const
{
    return IsIPv4() && (
        (GetByte(3) == 192 && GetByte(2) == 0 && GetByte(1) == 2) ||
        (GetByte(3) == 198 && GetByte(2) == 51 && GetByte(1) == 100) ||
        (GetByte(3) == 203 && GetByte(2) == 0 && GetByte(1) == 113));
}

bool CNetAddr::IsRFC3849() const
{
    return GetByte(15) == 0x20 && GetByte(14) == 0x01 && GetByte(13) == 0x0D && GetByte(12) == 0xB8;
}

bool CNetAddr::IsRFC3964() const
{
    return GetByte(15) == 0x20 && GetByte(14) == 0x02;
}

bool CNetAddr::IsRFC6052() const
{
    static const std::array<unsigned char, 12> RFC6052_IP_PREFIX {{ 0, 0x64, 0xFF, 0x9B, 0, 0, 0, 0, 0, 0, 0, 0}};
    return std::equal(RFC6052_IP_PREFIX.begin(), RFC6052_IP_PREFIX.end(), ip.begin());
}

bool CNetAddr::IsRFC4380() const
{
    return GetByte(15) == 0x20 && GetByte(14) == 0x01 && GetByte(13) == 0 && GetByte(12) == 0;
}

bool CNetAddr::IsRFC4862() const
{
    static const std::array<unsigned char, 8> RFC4862_IP_PREFIX {{ 0xFE, 0x80, 0, 0, 0, 0, 0, 0 }};
    return std::equal(RFC4862_IP_PREFIX.begin(), RFC4862_IP_PREFIX.end(), ip.begin());
}

bool CNetAddr::IsRFC4193() const
{
    return (GetByte(15) & 0xFE) == 0xFC;
}

bool CNetAddr::IsRFC6145() const
{
    static const std::array<unsigned char, 12> RFC6145_IP_PREFIX = {{ 0, 0, 0, 0, 0, 0, 0, 0, 0xFF, 0xFF, 0, 0 }};
    return std::equal(RFC6145_IP_PREFIX.begin(), RFC6145_IP_PREFIX.end(), ip.begin());
}

bool CNetAddr::IsRFC4843() const
{
    return GetByte(15) == 0x20 && GetByte(14) == 0x01 && GetByte(13) == 0x00 && (GetByte(12) & 0xF0) == 0x10;
}

bool CNetAddr::IsTor() const
{
    return std::equal(ONION_IP_PREFIX.begin(), ONION_IP_PREFIX.end(), ip.begin());
}

bool CNetAddr::IsLocal() const
{
    // IPv4 loopback
   if (IsIPv4() && (GetByte(3) == 127 || GetByte(3) == 0)) {
       return true;
   }

   // IPv6 loopback (::1/128)
   static const std::array<unsigned char, 16> LOOPBACK_IP {{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 }};
   return ip == LOOPBACK_IP;
}

bool CNetAddr::IsValid() const
{
    // Cleanup 3-byte shifted addresses caused by garbage in size field
    // of addr messages from versions before 0.2.9 checksum.
    // Two consecutive addr messages look like this:
    // header20 vectorlen3 addr26 addr26 addr26 header20 vectorlen3 addr26 addr26 addr26...
    // so if the first length field is garbled, it reads the second batch
    // of addr misaligned by 3 bytes.
    if (std::equal(IPV4_IP_PREFIX.begin() + 3, IPV4_IP_PREFIX.end(), ip.begin())) {
        return false;
    }

    // unspecified IPv6 address (::/128)
    if (std::all_of(ip.begin(), ip.end(), [](unsigned char x){ return x == 0; })) {
        return false;
    }

    // documentation IPv6 address
    if (IsRFC3849()) {
        return false;
    }

    if (IsInternal()) {
        return false;
    }

    if (IsIPv4()) {
        const auto ipv4_begin = ip.begin() + IPV4_IP_PREFIX.size();
        // INADDR_NONE
        if (std::all_of(ipv4_begin, ip.end(), [](unsigned char x){ return x == 0xff; })) {
            return false;
        }
        // 0
        if (std::all_of(ipv4_begin, ip.end(), [](unsigned char x){ return x == 0; })) {
            return false;
        }
    }

    return true;
}

bool CNetAddr::IsRoutable() const
{
    return IsValid() && !(IsRFC1918() || IsRFC2544() || IsRFC3927() || IsRFC4862() || IsRFC6598() || IsRFC5737() || (IsRFC4193() && !IsTor()) || IsRFC4843() || IsLocal() || IsInternal());
}

bool CNetAddr::IsInternal() const
{
   return std::equal(INTERNAL_IP_PREFIX.begin(), INTERNAL_IP_PREFIX.end(), ip.begin());
}

enum Network CNetAddr::GetNetwork() const
{
    if (IsInternal())   return NET_INTERNAL;
    if (!IsRoutable())  return NET_UNROUTABLE;
    if (IsIPv4())       return NET_IPV4;
    if (IsTor())        return NET_TOR;
    return NET_IPV6;
}

std::string CNetAddr::ToStringIP() const
{
    if (IsTor()) {
        return EncodeBase32(ip.data() + ONION_IP_PREFIX.size(), ip.size() - ONION_IP_PREFIX.size()) + ONION_ADDRESS_SUFFIX;
    }

    if (IsInternal()) {
        return EncodeBase32(ip.data() + INTERNAL_IP_PREFIX.size(), ip.size() - INTERNAL_IP_PREFIX.size()) + ".internal";
    }

    {
        CService serv(*this, 0);
        struct sockaddr_storage sockaddr;
        socklen_t socklen = sizeof(sockaddr);
        if (serv.GetSockAddr((struct sockaddr*)&sockaddr, &socklen)) {
            char name[1025] = "";
            if (!getnameinfo((const struct sockaddr*)&sockaddr, socklen, name, sizeof(name), nullptr, 0, NI_NUMERICHOST))
                return std::string(name);
        }
    }

    if (IsIPv4()) {
        return strprintf("%u.%u.%u.%u", GetByte(3), GetByte(2), GetByte(1), GetByte(0));
    }

    return strprintf("%x:%x:%x:%x:%x:%x:%x:%x",
                     GetByte(15) << 8 | GetByte(14), GetByte(13) << 8 | GetByte(12),
                     GetByte(11) << 8 | GetByte(10), GetByte(9) << 8 | GetByte(8),
                     GetByte(7) << 8 | GetByte(6), GetByte(5) << 8 | GetByte(4),
                     GetByte(3) << 8 | GetByte(2), GetByte(1) << 8 | GetByte(0));
}

std::string CNetAddr::ToString() const
{
    return ToStringIP();
}

bool operator==(const CNetAddr& a, const CNetAddr& b)
{
    return a.ip == b.ip;
}

bool operator!=(const CNetAddr& a, const CNetAddr& b)
{
    return !(a == b);
}

bool operator<(const CNetAddr& a, const CNetAddr& b)
{
    return a.ip < b.ip;
}

bool CNetAddr::GetInAddr(struct in_addr* pipv4Addr) const
{
    if (!IsIPv4()) {
        return false;
    }
    std::copy(ip.begin() + IPV4_IP_PREFIX.size(), ip.end(), reinterpret_cast<unsigned char*>(pipv4Addr));
    return true;
}

bool CNetAddr::GetIn6Addr(struct in6_addr* pipv6Addr) const
{
    std::copy(ip.begin(), ip.end(), reinterpret_cast<unsigned char*>(pipv6Addr));
    return true;
}

// get canonical identifier of an address' group
// no two connections will be attempted to addresses with the same group
std::vector<unsigned char> CNetAddr::GetGroup() const
{
    std::vector<unsigned char> vchRet;
    int nClass = NET_IPV6;
    int nStartByte = 0;
    int nBits = 16;

    // all local addresses belong to the same group
    if (IsLocal()) {
        nClass = 255;
        nBits = 0;
    }
    // all internal-usage addresses get their own group
    if (IsInternal()) {
        nClass = NET_INTERNAL;
        nStartByte = INTERNAL_IP_PREFIX.size();
        nBits = (ip.size() - INTERNAL_IP_PREFIX.size()) * 8;
    }
    // all other unroutable addresses belong to the same group
    else if (!IsRoutable()) {
        nClass = NET_UNROUTABLE;
        nBits = 0;
    }
    // for IPv4 addresses, '1' + the 16 higher-order bits of the IP
    // includes mapped IPv4, SIIT translated IPv4, and the well-known prefix
    else if (IsIPv4() || IsRFC6145() || IsRFC6052()) {
        nClass = NET_IPV4;
        nStartByte = IPV4_IP_PREFIX.size();
    }
    // for 6to4 tunnelled addresses, use the encapsulated IPv4 address
    else if (IsRFC3964()) {
        nClass = NET_IPV4;
        nStartByte = 2;
    }
    // for Teredo-tunnelled IPv6 addresses, use the encapsulated IPv4 address
    else if (IsRFC4380()) {
        vchRet.push_back(NET_IPV4);
        vchRet.push_back(GetByte(3) ^ 0xFF);
        vchRet.push_back(GetByte(2) ^ 0xFF);
        return vchRet;
    }
    else if (IsTor()) {
        nClass = NET_TOR;
        nStartByte = ONION_IP_PREFIX.size();
        nBits = 4;
    }
    // for he.net, use /36 groups
    else if (GetByte(15) == 0x20 && GetByte(14) == 0x01 && GetByte(13) == 0x04 && GetByte(12) == 0x70) {
        nBits = 36;
    }
    // for the rest of the IPv6 network, use /32 groups
    else {
        nBits = 32;
    }

    vchRet.push_back(nClass);
    while (nBits >= 8) {
        vchRet.push_back(GetByte(ip.size() - 1 - nStartByte));
        nStartByte++;
        nBits -= 8;
    }
    if (nBits > 0) {
        vchRet.push_back(GetByte(ip.size() - 1 - nStartByte) | ((1 << (8 - nBits)) - 1));
    }

    return vchRet;
}

uint64_t CNetAddr::GetHash() const
{
    uint256 hash = Hash(ip.begin(), ip.end());
    uint64_t nRet;
    memcpy(&nRet, &hash, sizeof(nRet));
    return nRet;
}

// private extensions to enum Network, only returned by GetExtNetwork,
// and only used in GetReachabilityFrom
static const int NET_UNKNOWN = NET_MAX + 0;
static const int NET_TEREDO  = NET_MAX + 1;
int static GetExtNetwork(const CNetAddr *addr)
{
    if (addr == nullptr)    return NET_UNKNOWN;
    if (addr->IsRFC4380())  return NET_TEREDO;
    return addr->GetNetwork();
}

/** Calculates a metric for how reachable (*this) is from a given partner */
int CNetAddr::GetReachabilityFrom(const CNetAddr *paddrPartner) const
{
    enum Reachability {
        REACH_UNREACHABLE,
        REACH_DEFAULT,
        REACH_TEREDO,
        REACH_IPV6_WEAK,
        REACH_IPV4,
        REACH_IPV6_STRONG,
        REACH_PRIVATE
    };

    if (!IsRoutable() || IsInternal()) {
        return REACH_UNREACHABLE;
    }

    int ourNet = GetExtNetwork(this);
    int theirNet = GetExtNetwork(paddrPartner);
    bool fTunnel = IsRFC3964() || IsRFC6052() || IsRFC6145();

    switch(theirNet) {
    case NET_IPV4:
        switch(ourNet) {
        default:       return REACH_DEFAULT;
        case NET_IPV4: return REACH_IPV4;
        }
    case NET_IPV6:
        switch(ourNet) {
        default:         return REACH_DEFAULT;
        case NET_TEREDO: return REACH_TEREDO;
        case NET_IPV4:   return REACH_IPV4;
        case NET_IPV6:   return fTunnel ? REACH_IPV6_WEAK : REACH_IPV6_STRONG; // only prefer giving our IPv6 address if it's not tunnelled
        }
    case NET_TOR:
        switch(ourNet) {
        default:         return REACH_DEFAULT;
        case NET_IPV4:   return REACH_IPV4; // Tor users can connect to IPv4 as well
        case NET_TOR:    return REACH_PRIVATE;
        }
    case NET_TEREDO:
        switch(ourNet) {
        default:          return REACH_DEFAULT;
        case NET_TEREDO:  return REACH_TEREDO;
        case NET_IPV6:    return REACH_IPV6_WEAK;
        case NET_IPV4:    return REACH_IPV4;
        }
    case NET_UNKNOWN:
    case NET_UNROUTABLE:
    default:
        switch(ourNet) {
        default:          return REACH_DEFAULT;
        case NET_TEREDO:  return REACH_TEREDO;
        case NET_IPV6:    return REACH_IPV6_WEAK;
        case NET_IPV4:    return REACH_IPV4;
        case NET_TOR:     return REACH_PRIVATE; // either from Tor, or don't care about our address
        }
    }
}

void CService::Init()
{
    port = 0;
}

CService::CService()
{
    Init();
}

CService::CService(const CNetAddr& cip, unsigned short portIn) : CNetAddr(cip), port(portIn)
{
    // Empty
}

CService::CService(const struct in_addr& ipv4Addr, unsigned short portIn) : CNetAddr(ipv4Addr), port(portIn)
{
    // Empty
}

CService::CService(const struct in6_addr& ipv6Addr, unsigned short portIn) : CNetAddr(ipv6Addr), port(portIn)
{
    // Empty
}

CService::CService(const struct sockaddr_in& addr) : CNetAddr(addr.sin_addr), port(ntohs(addr.sin_port))
{
    assert(addr.sin_family == AF_INET);
}

CService::CService(const struct sockaddr_in6 &addr) : CNetAddr(addr.sin6_addr, addr.sin6_scope_id), port(ntohs(addr.sin6_port))
{
   assert(addr.sin6_family == AF_INET6);
}

bool CService::SetSockAddr(const struct sockaddr *paddr)
{
    switch (paddr->sa_family) {
    case AF_INET:
        *this = CService(*reinterpret_cast<const struct sockaddr_in*>(paddr));
        return true;
    case AF_INET6:
        *this = CService(*reinterpret_cast<const struct sockaddr_in6*>(paddr));
        return true;
    default:
        return false;
    }
}

unsigned short CService::GetPort() const
{
    return port;
}

bool operator==(const CService& a, const CService& b)
{
    return (CNetAddr)a == (CNetAddr)b && a.port == b.port;
}

bool operator!=(const CService& a, const CService& b)
{
    return (CNetAddr)a != (CNetAddr)b || a.port != b.port;
}

bool operator<(const CService& a, const CService& b)
{
    return (CNetAddr)a < (CNetAddr)b || ((CNetAddr)a == (CNetAddr)b && a.port < b.port);
}

bool CService::GetSockAddr(struct sockaddr* paddr, socklen_t *addrlen) const
{
    if (IsIPv4()) {
        if (*addrlen < (socklen_t)sizeof(struct sockaddr_in)) {
            return false;
        }
        *addrlen = sizeof(struct sockaddr_in);
        auto *paddrin = reinterpret_cast<struct sockaddr_in*>(paddr);
        memset(paddrin, 0, *addrlen);
        if (!GetInAddr(&paddrin->sin_addr)) {
            return false;
        }
        paddrin->sin_family = AF_INET;
        paddrin->sin_port = htons(port);
        return true;
    }
    if (IsIPv6()) {
        if (*addrlen < (socklen_t)sizeof(struct sockaddr_in6)) {
            return false;
        }
        *addrlen = sizeof(struct sockaddr_in6);
        auto *paddrin6 = reinterpret_cast<struct sockaddr_in6*>(paddr);
        memset(paddrin6, 0, *addrlen);
        if (!GetIn6Addr(&paddrin6->sin6_addr)) {
            return false;
        }
        paddrin6->sin6_scope_id = scopeId;
        paddrin6->sin6_family = AF_INET6;
        paddrin6->sin6_port = htons(port);
        return true;
    }
    return false;
}

std::vector<unsigned char> CService::GetKey() const
{
     std::vector<unsigned char> vKey;
     vKey.resize(18);
     std::copy(ip.begin(), ip.end(), vKey.begin());
     vKey[16] = port / 0x100;
     vKey[17] = port & 0x0FF;
     return vKey;
}

std::string CService::ToStringPort() const
{
    return strprintf("%u", port);
}

std::string CService::ToStringIPPort() const
{
    if (IsIPv4() || IsTor() || IsInternal()) {
        return ToStringIP() + ":" + ToStringPort();
    }
    return "[" + ToStringIP() + "]:" + ToStringPort();
}

std::string CService::ToString() const
{
    return ToStringIPPort();
}

CSubNet::CSubNet()
: netmask {}
, valid {false}
{
    // Empty
}

CSubNet::CSubNet(const CNetAddr &addr, int32_t mask)
{
    valid = true;
    network = addr;
    // Default to /32 (IPv4) or /128 (IPv6), i.e. match single address
    std::fill(netmask.begin(), netmask.end(), 255);

    // IPv4 addresses start at offset 12, and first 12 bytes must match, so just offset n
    const int astartofs = network.IsIPv4() ? 12 : 0;

    int32_t n = mask;
    if (n >= 0 && n <= (128 - astartofs*8)) { // Only valid if in range of bits of address
        n += astartofs*8;
        // Clear bits [n..127]
        for (; n < 128; ++n) {
            netmask[n>>3] &= ~(1<<(7-(n&7)));
        }
    } else {
        valid = false;
    }

    // Normalize network according to netmask
    std::transform(network.ip.begin(), network.ip.end(), netmask.begin(), network.ip.begin(),
        [](unsigned char ip_byte, unsigned char mask_byte) { return ip_byte & mask_byte; });
}

CSubNet::CSubNet(const CNetAddr &addr, const CNetAddr &mask)
{
    valid = true;
    network = addr;
    // Default to /32 (IPv4) or /128 (IPv6), i.e. match single address
    std::fill(netmask.begin(), netmask.end(), 255);

    // IPv4 addresses start at offset 12, and first 12 bytes must match, so just offset n
    const size_t astartofs = network.IsIPv4() ? IPV4_IP_PREFIX.size() : 0;

    std::copy(mask.ip.begin() + astartofs, mask.ip.end(), netmask.begin() + astartofs);

    // Normalize network according to netmask
    std::transform(network.ip.begin(), network.ip.end(), netmask.begin(), network.ip.begin(),
        [](unsigned char ip_byte, unsigned char mask_byte) { return ip_byte & mask_byte; });
}

CSubNet::CSubNet(const CNetAddr &addr)
: valid(addr.IsValid())
{
    std::fill(netmask.begin(), netmask.end(), 255);
    network = addr;
}

bool CSubNet::Match(const CNetAddr &addr) const
{
    if (!valid || !addr.IsValid()) {
        return false;
    }
    for (size_t x = 0; x < addr.ip.size(); ++x) {
        if ((addr.ip[x] & netmask[x]) != network.ip[x]) {
            return false;
        }
    }
    return true;
}

static inline int NetmaskBits(unsigned char x)
{
    switch(x) {
    case 0x00: return 0;
    case 0x80: return 1;
    case 0xc0: return 2;
    case 0xe0: return 3;
    case 0xf0: return 4;
    case 0xf8: return 5;
    case 0xfc: return 6;
    case 0xfe: return 7;
    case 0xff: return 8;
    default: return -1;
    }
}

std::string CSubNet::ToString() const
{
    /* Parse binary 1{n}0{N-n} to see if mask can be represented as /n */
    int cidr = 0;
    bool valid_cidr = true;
    size_t n = network.IsIPv4() ? IPV4_IP_PREFIX.size() : 0;
    for (; n < netmask.size() && netmask[n] == 0xff; ++n) {
        cidr += 8;
    }
    if (n < netmask.size()) {
        int bits = NetmaskBits(netmask[n]);
        if (bits < 0) {
            valid_cidr = false;
        }
        else {
            cidr += bits;
        }
        ++n;
    }
    for (; n < netmask.size() && valid_cidr; ++n) {
        if (netmask[n] != 0x00) {
            valid_cidr = false;
        }
    }

    /* Format output */
    std::string strNetmask;
    if (valid_cidr) {
        strNetmask = strprintf("%u", cidr);
    } else {
        if (network.IsIPv4()) {
            strNetmask = strprintf("%u.%u.%u.%u", netmask[12], netmask[13], netmask[14], netmask[15]);
        }
        else {
            strNetmask = strprintf("%x:%x:%x:%x:%x:%x:%x:%x",
                             netmask[0] << 8 | netmask[1], netmask[2] << 8 | netmask[3],
                             netmask[4] << 8 | netmask[5], netmask[6] << 8 | netmask[7],
                             netmask[8] << 8 | netmask[9], netmask[10] << 8 | netmask[11],
                             netmask[12] << 8 | netmask[13], netmask[14] << 8 | netmask[15]);
        }
    }

    return network.ToString() + "/" + strNetmask;
}

bool CSubNet::IsValid() const
{
    return valid;
}

bool operator==(const CSubNet& a, const CSubNet& b)
{
    return a.valid == b.valid && a.network == b.network && a.netmask == b.netmask;
}

bool operator!=(const CSubNet& a, const CSubNet& b)
{
    return !(a==b);
}

bool operator<(const CSubNet& a, const CSubNet& b)
{
    return a.network < b.network || (a.network == b.network && a.netmask < b.netmask);
}
