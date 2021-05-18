// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <netaddress.h>
#include <netbase.h>
#include <hash.h>
#include <utilasmap.h>
#include <utilstrencodings.h>
#include <tinyformat.h>

static const unsigned char pchIPv4[12] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xff, 0xff };
static const unsigned char pchOnionCat[] = {0xFD,0x87,0xD8,0x7E,0xEB,0x43};

// 0xFD + sha256("bitcoin")[0:5]
static const unsigned char g_internal_prefix[] = { 0xFD, 0x6B, 0x88, 0xC0, 0x87, 0x24 };

bool fAllowPrivateNet = DEFAULT_ALLOWPRIVATENET;

CNetAddr::CNetAddr()
{
    memset(ip, 0, sizeof(ip));
}

void CNetAddr::SetIP(const CNetAddr& ipIn)
{
    m_net = ipIn.m_net;
    memcpy(ip, ipIn.ip, sizeof(ip));
}

void CNetAddr::SetLegacyIPv6(const uint8_t ipv6[16])
{
    if (memcmp(ipv6, pchIPv4, sizeof(pchIPv4)) == 0) {
        m_net = NET_IPV4;
    } else if (memcmp(ipv6, pchOnionCat, sizeof(pchOnionCat)) == 0) {
        m_net = NET_ONION;
    } else if (memcmp(ipv6, g_internal_prefix, sizeof(g_internal_prefix)) == 0) {
        m_net = NET_INTERNAL;
    } else {
        m_net = NET_IPV6;
    }
    memcpy(ip, ipv6, 16);
}

void CNetAddr::SetRaw(Network network, const uint8_t *ip_in)
{
    switch(network)
    {
        case NET_IPV4:
            m_net = NET_IPV4;
            memcpy(ip, pchIPv4, 12);
            memcpy(ip+12, ip_in, 4);
            break;
        case NET_IPV6:
            SetLegacyIPv6(ip_in);
            break;
        default:
            assert(!"invalid network");
    }
}

bool CNetAddr::SetInternal(const std::string &name)
{
    if (name.empty()) {
        return false;
    }
    m_net = NET_INTERNAL;
    unsigned char hash[32] = {};
    CSHA256().Write((const unsigned char*)name.data(), name.size()).Finalize(hash);
    memcpy(ip, g_internal_prefix, sizeof(g_internal_prefix));
    memcpy(ip + sizeof(g_internal_prefix), hash, sizeof(ip) - sizeof(g_internal_prefix));
    return true;
}

bool CNetAddr::SetSpecial(const std::string &strName)
{
    if (strName.size()>6 && strName.substr(strName.size() - 6, 6) == ".onion") {
        std::vector<unsigned char> vchAddr = DecodeBase32(strName.substr(0, strName.size() - 6).c_str());
        if (vchAddr.size() != 16-sizeof(pchOnionCat))
            return false;
        m_net = NET_ONION;
        memcpy(ip, pchOnionCat, sizeof(pchOnionCat));
        for (unsigned int i=0; i<16-sizeof(pchOnionCat); i++)
            ip[i + sizeof(pchOnionCat)] = vchAddr[i];
        return true;
    }
    return false;
}

CNetAddr::CNetAddr(const struct in_addr& ipv4Addr)
{
    SetRaw(NET_IPV4, (const uint8_t*)&ipv4Addr);
}

CNetAddr::CNetAddr(const struct in6_addr& ipv6Addr, const uint32_t scope)
{
    SetRaw(NET_IPV6, (const uint8_t*)&ipv6Addr);
    scopeId = scope;
}

unsigned int CNetAddr::GetByte(int n) const
{
    return ip[15-n];
}

bool CNetAddr::IsBindAny() const
{
    const int cmplen = IsIPv4() ? 4 : 16;
    for (int i = 0; i < cmplen; ++i) {
        if (GetByte(i)) return false;
    }

    return true;
}

bool CNetAddr::IsIPv4() const { return m_net == NET_IPV4; }

bool CNetAddr::IsIPv6() const { return m_net == NET_IPV6; }

bool CNetAddr::IsRFC1918() const
{
    return IsIPv4() && (
        GetByte(3) == 10 ||
        (GetByte(3) == 192 && GetByte(2) == 168) ||
        (GetByte(3) == 172 && (GetByte(2) >= 16 && GetByte(2) <= 31)));
}

bool CNetAddr::IsRFC2544() const
{
    return IsIPv4() && GetByte(3) == 198 && (GetByte(2) == 18 || GetByte(2) == 19);
}

bool CNetAddr::IsRFC3927() const
{
    return IsIPv4() && (GetByte(3) == 169 && GetByte(2) == 254);
}

bool CNetAddr::IsRFC6598() const
{
    return IsIPv4() && GetByte(3) == 100 && GetByte(2) >= 64 && GetByte(2) <= 127;
}

bool CNetAddr::IsRFC5737() const
{
    return IsIPv4() && ((GetByte(3) == 192 && GetByte(2) == 0 && GetByte(1) == 2) ||
        (GetByte(3) == 198 && GetByte(2) == 51 && GetByte(1) == 100) ||
        (GetByte(3) == 203 && GetByte(2) == 0 && GetByte(1) == 113));
}

bool CNetAddr::IsRFC3849() const
{
    return IsIPv6() && GetByte(15) == 0x20 && GetByte(14) == 0x01 &&
           GetByte(13) == 0x0D && GetByte(12) == 0xB8;
}

bool CNetAddr::IsRFC3964() const
{
    return IsIPv6() && GetByte(15) == 0x20 && GetByte(14) == 0x02;
}

bool CNetAddr::IsRFC6052() const
{
    static const unsigned char pchRFC6052[] = {0,0x64,0xFF,0x9B,0,0,0,0,0,0,0,0};
    return IsIPv6() && memcmp(ip, pchRFC6052, sizeof(pchRFC6052)) == 0;
}

bool CNetAddr::IsRFC4380() const
{
    return IsIPv6() && GetByte(15) == 0x20 && GetByte(14) == 0x01 && GetByte(13) == 0 &&
           GetByte(12) == 0;
}

bool CNetAddr::IsRFC4862() const
{
    static const unsigned char pchRFC4862[] = {0xFE,0x80,0,0,0,0,0,0};
    return IsIPv6() && memcmp(ip, pchRFC4862, sizeof(pchRFC4862)) == 0;
}

bool CNetAddr::IsRFC4193() const
{
    return IsIPv6() && (GetByte(15) & 0xFE) == 0xFC;
}

bool CNetAddr::IsRFC6145() const
{
    static const unsigned char pchRFC6145[] = {0,0,0,0,0,0,0,0,0xFF,0xFF,0,0};
    return IsIPv6() && memcmp(ip, pchRFC6145, sizeof(pchRFC6145)) == 0;
}

bool CNetAddr::IsRFC4843() const
{
    return IsIPv6() && GetByte(15) == 0x20 && GetByte(14) == 0x01 &&
           GetByte(13) == 0x00 && (GetByte(12) & 0xF0) == 0x10;
}

bool CNetAddr::IsRFC7343() const
{
    return IsIPv6() && GetByte(15) == 0x20 && GetByte(14) == 0x01 &&
           GetByte(13) == 0x00 && (GetByte(12) & 0xF0) == 0x20;
}

bool CNetAddr::IsTor() const { return m_net == NET_ONION; }

bool CNetAddr::IsHeNet() const
{
    return (GetByte(15) == 0x20 && GetByte(14) == 0x01 && GetByte(13) == 0x04 && GetByte(12) == 0x70);
}

bool CNetAddr::IsLocal() const
{
    // IPv4 loopback
   if (IsIPv4() && (GetByte(3) == 127 || GetByte(3) == 0))
       return true;

   // IPv6 loopback (::1/128)
   static const unsigned char pchLocal[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1};
   if (IsIPv6() && memcmp(ip, pchLocal, 16) == 0)
       return true;

   return false;
}

bool CNetAddr::IsValid() const
{
    // Cleanup 3-byte shifted addresses caused by garbage in size field
    // of addr messages from versions before 0.2.9 checksum.
    // Two consecutive addr messages look like this:
    // header20 vectorlen3 addr26 addr26 addr26 header20 vectorlen3 addr26 addr26 addr26...
    // so if the first length field is garbled, it reads the second batch
    // of addr misaligned by 3 bytes.
    if (IsIPv6() && memcmp(ip, pchIPv4+3, sizeof(pchIPv4)-3) == 0)
        return false;

    // unspecified IPv6 address (::/128)
    unsigned char ipNone6[16] = {};
    if (IsIPv6() && memcmp(ip, ipNone6, 16) == 0)
        return false;

    // documentation IPv6 address
    if (IsRFC3849())
        return false;

    if (IsInternal())
        return false;

    if (IsIPv4())
    {
        // INADDR_NONE
        uint32_t ipNone = INADDR_NONE;
        if (memcmp(ip+12, &ipNone, 4) == 0)
            return false;

        // 0
        ipNone = 0;
        if (memcmp(ip+12, &ipNone, 4) == 0)
            return false;
    }

    return true;
}

bool CNetAddr::IsRoutable() const
{
    if (!IsValid())
        return false;
    if (!fAllowPrivateNet && IsRFC1918())
        return false;
    return !(IsRFC2544() || IsRFC3927() || IsRFC4862() || IsRFC6598() || IsRFC5737() || (IsRFC4193() && !IsTor()) || IsRFC4843() || IsRFC7343() || IsLocal() || IsInternal());
}

bool CNetAddr::IsInternal() const
{
   return m_net == NET_INTERNAL;
}

enum Network CNetAddr::GetNetwork() const
{
    if (IsInternal())
        return NET_INTERNAL;

    if (!IsRoutable())
        return NET_UNROUTABLE;

    return m_net;
}

std::string CNetAddr::ToStringIP(bool fUseGetnameinfo) const
{
    if (IsTor())
        return EncodeBase32(&ip[6], 10) + ".onion";
    if (IsInternal())
        return EncodeBase32(ip + sizeof(g_internal_prefix), sizeof(ip) - sizeof(g_internal_prefix)) + ".internal";
    if (fUseGetnameinfo)
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
    if (IsIPv4())
        return strprintf("%u.%u.%u.%u", GetByte(3), GetByte(2), GetByte(1), GetByte(0));
    else
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
    return a.m_net == b.m_net && memcmp(a.ip, b.ip, 16) == 0;
}

bool operator<(const CNetAddr& a, const CNetAddr& b)
{
    return a.m_net < b.m_net || (a.m_net == b.m_net && memcmp(a.ip, b.ip, 16) < 0);
}

bool CNetAddr::GetInAddr(struct in_addr* pipv4Addr) const
{
    if (!IsIPv4())
        return false;
    memcpy(pipv4Addr, ip+12, 4);
    return true;
}

bool CNetAddr::GetIn6Addr(struct in6_addr* pipv6Addr) const
{
    if (!IsIPv6()) {
        return false;
    }
    memcpy(pipv6Addr, ip, 16);
    return true;
}

uint32_t CNetAddr::GetNetClass() const {
    uint32_t net_class = NET_IPV6;
    if (IsLocal()) {
        net_class = 255;
    }
    if (IsInternal()) {
        net_class = NET_INTERNAL;
    } else if (!IsRoutable()) {
        net_class = NET_UNROUTABLE;
    } else if (IsIPv4() || IsRFC6145() || IsRFC6052() || IsRFC3964() || IsRFC4380()) {
        net_class = NET_IPV4;
    } else if (IsTor()) {
        net_class = NET_ONION;
    }
    return net_class;
}

uint32_t CNetAddr::GetMappedAS(const std::vector<bool> &asmap) const {
    uint32_t net_class = GetNetClass();
    if (asmap.size() == 0 || (net_class != NET_IPV4 && net_class != NET_IPV6)) {
        return 0; // Indicates not found, safe because AS0 is reserved per RFC7607.
    }
    std::vector<bool> ip_bits(128);
    for (int8_t byte_i = 0; byte_i < 16; ++byte_i) {
        uint8_t cur_byte = GetByte(15 - byte_i);
        for (uint8_t bit_i = 0; bit_i < 8; ++bit_i) {
            ip_bits[byte_i * 8 + bit_i] = (cur_byte >> (7 - bit_i)) & 1;
        }
    }
    uint32_t mapped_as = Interpret(asmap, ip_bits);
    return mapped_as;
}

/**
 * Get the canonical identifier of our network group
 *
 * The groups are assigned in a way where it should be costly for an attacker to
 * obtain addresses with many different group identifiers, even if it is cheap
 * to obtain addresses with the same identifier.
 *
 * @note No two connections will be attempted to addresses with the same network
 *       group.
 */
std::vector<unsigned char> CNetAddr::GetGroup(const std::vector<bool> &asmap) const
{
    std::vector<unsigned char> vchRet;
    uint32_t net_class = GetNetClass();
    // If non-empty asmap is supplied and the address is IPv4/IPv6,
    // return ASN to be used for bucketing.
    uint32_t asn = GetMappedAS(asmap);
    if (asn != 0) { // Either asmap was empty, or address has non-asmappable net class (e.g. TOR).
        vchRet.push_back(NET_IPV6); // IPv4 and IPv6 with same ASN should be in the same bucket
        for (int i = 0; i < 4; i++) {
            vchRet.push_back((asn >> (8 * i)) & 0xFF);
        }
        return vchRet;
    }

    vchRet.push_back(net_class);
    int nStartByte = 0;
    int nBits = 16;

    // all local addresses belong to the same group
    if (IsLocal())
    {
        nBits = 0;
    }
    // all internal-usage addresses get their own group
    if (IsInternal())
    {
        nStartByte = sizeof(g_internal_prefix);
        nBits = (sizeof(ip) - sizeof(g_internal_prefix)) * 8;
    }
    // all other unroutable addresses belong to the same group
    else if (!IsRoutable())
    {
        nBits = 0;
    }
    // for IPv4 addresses, '1' + the 16 higher-order bits of the IP
    // includes mapped IPv4, SIIT translated IPv4, and the well-known prefix
    else if (IsIPv4() || IsRFC6145() || IsRFC6052())
    {
        nStartByte = 12;
    }
    // for 6to4 tunnelled addresses, use the encapsulated IPv4 address
    else if (IsRFC3964())
    {
        nStartByte = 2;
    }
    // for Teredo-tunnelled IPv6 addresses, use the encapsulated IPv4 address
    else if (IsRFC4380())
    {
        vchRet.push_back(GetByte(3) ^ 0xFF);
        vchRet.push_back(GetByte(2) ^ 0xFF);
        return vchRet;
    }
    else if (IsTor())
    {
        nStartByte = 6;
        nBits = 4;
    } else if (IsHeNet()) {
        // for he.net, use /36 groups
        nBits = 36;
    // for the rest of the IPv6 network, use /32 groups
    else
        nBits = 32;

    while (nBits >= 8)
    {
        vchRet.push_back(GetByte(15 - nStartByte));
        nStartByte++;
        nBits -= 8;
    }
    if (nBits > 0)
        vchRet.push_back(GetByte(15 - nStartByte) | ((1 << (8 - nBits)) - 1));

    return vchRet;
}

uint64_t CNetAddr::GetHash() const
{
    uint256 hash = Hash(&ip[0], &ip[16]);
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
    if (addr == nullptr)
        return NET_UNKNOWN;
    if (addr->IsRFC4380())
        return NET_TEREDO;
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

    if (!IsRoutable() || IsInternal())
        return REACH_UNREACHABLE;

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
    case NET_ONION:
        switch(ourNet) {
        default:         return REACH_DEFAULT;
        case NET_IPV4:   return REACH_IPV4; // Tor users can connect to IPv4 as well
        case NET_ONION:    return REACH_PRIVATE;
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
        case NET_ONION:     return REACH_PRIVATE; // either from Tor, or don't care about our address
        }
    }
}

CService::CService() : port(0)
{
}

CService::CService(const CNetAddr& cip, unsigned short portIn) : CNetAddr(cip), port(portIn)
{
}

CService::CService(const struct in_addr& ipv4Addr, unsigned short portIn) : CNetAddr(ipv4Addr), port(portIn)
{
}

CService::CService(const struct in6_addr& ipv6Addr, unsigned short portIn) : CNetAddr(ipv6Addr), port(portIn)
{
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
        *this = CService(*(const struct sockaddr_in*)paddr);
        return true;
    case AF_INET6:
        *this = CService(*(const struct sockaddr_in6*)paddr);
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
    return static_cast<CNetAddr>(a) == static_cast<CNetAddr>(b) && a.port == b.port;
}

bool operator<(const CService& a, const CService& b)
{
    return static_cast<CNetAddr>(a) < static_cast<CNetAddr>(b) || (static_cast<CNetAddr>(a) == static_cast<CNetAddr>(b) && a.port < b.port);
}

bool CService::GetSockAddr(struct sockaddr* paddr, socklen_t *addrlen) const
{
    if (IsIPv4()) {
        if (*addrlen < (socklen_t)sizeof(struct sockaddr_in))
            return false;
        *addrlen = sizeof(struct sockaddr_in);
        struct sockaddr_in *paddrin = (struct sockaddr_in*)paddr;
        memset(paddrin, 0, *addrlen);
        if (!GetInAddr(&paddrin->sin_addr))
            return false;
        paddrin->sin_family = AF_INET;
        paddrin->sin_port = htons(port);
        return true;
    }
    if (IsIPv6()) {
        if (*addrlen < (socklen_t)sizeof(struct sockaddr_in6))
            return false;
        *addrlen = sizeof(struct sockaddr_in6);
        struct sockaddr_in6 *paddrin6 = (struct sockaddr_in6*)paddr;
        memset(paddrin6, 0, *addrlen);
        if (!GetIn6Addr(&paddrin6->sin6_addr))
            return false;
        paddrin6->sin6_scope_id = scopeId;
        paddrin6->sin6_family = AF_INET6;
        paddrin6->sin6_port = htons(port);
        return true;
    }
    return false;
}

std::vector<unsigned char> CService::GetKey() const
{
    auto key = GetAddrBytes();
    key.push_back(port / 0x100);
    key.push_back(port & 0x0FF);
    return key;
}

std::string CService::ToStringPort() const
{
    return strprintf("%u", port);
}

std::string CService::ToStringIPPort(bool fUseGetnameinfo) const
{
    if (IsIPv4() || IsTor() || IsInternal()) {
        return ToStringIP(fUseGetnameinfo) + ":" + ToStringPort();
    } else {
        return "[" + ToStringIP(fUseGetnameinfo) + "]:" + ToStringPort();
    }
}

std::string CService::ToString(bool fUseGetnameinfo) const
{
    return ToStringIPPort(fUseGetnameinfo);
}

void CService::SetPort(unsigned short portIn)
{
    port = portIn;
}

CSubNet::CSubNet():
    valid(false)
{
    memset(netmask, 0, sizeof(netmask));
}

CSubNet::CSubNet(const CNetAddr &addr, int32_t mask)
{
    valid = true;
    network = addr;
    // Default to /32 (IPv4) or /128 (IPv6), i.e. match single address
    memset(netmask, 255, sizeof(netmask));

    // IPv4 addresses start at offset 12, and first 12 bytes must match, so just offset n
    const int astartofs = network.IsIPv4() ? 12 : 0;

    int32_t n = mask;
    if(n >= 0 && n <= (128 - astartofs*8)) // Only valid if in range of bits of address
    {
        n += astartofs*8;
        // Clear bits [n..127]
        for (; n < 128; ++n)
            netmask[n>>3] &= ~(1<<(7-(n&7)));
    } else
        valid = false;

    // Normalize network according to netmask
    for(int x=0; x<16; ++x)
        network.ip[x] &= netmask[x];
}

CSubNet::CSubNet(const CNetAddr &addr, const CNetAddr &mask)
{
    valid = true;
    network = addr;
    // Default to /32 (IPv4) or /128 (IPv6), i.e. match single address
    memset(netmask, 255, sizeof(netmask));

    // IPv4 addresses start at offset 12, and first 12 bytes must match, so just offset n
    const int astartofs = network.IsIPv4() ? 12 : 0;

    for(int x=astartofs; x<16; ++x)
        netmask[x] = mask.ip[x];

    // Normalize network according to netmask
    for(int x=0; x<16; ++x)
        network.ip[x] &= netmask[x];
}

CSubNet::CSubNet(const CNetAddr &addr):
    valid(addr.IsValid())
{
    memset(netmask, 255, sizeof(netmask));
    network = addr;
}

bool CSubNet::Match(const CNetAddr &addr) const
{
    if (!valid || !addr.IsValid() || network.m_net != addr.m_net)
        return false;
    for(int x=0; x<16; ++x)
        if ((addr.ip[x] & netmask[x]) != network.ip[x])
            return false;
    return true;
}

static inline int NetmaskBits(uint8_t x)
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
    int n = network.IsIPv4() ? 12 : 0;
    for (; n < 16 && netmask[n] == 0xff; ++n)
        cidr += 8;
    if (n < 16) {
        int bits = NetmaskBits(netmask[n]);
        if (bits < 0)
            valid_cidr = false;
        else
            cidr += bits;
        ++n;
    }
    for (; n < 16 && valid_cidr; ++n)
        if (netmask[n] != 0x00)
            valid_cidr = false;

    /* Format output */
    std::string strNetmask;
    if (valid_cidr) {
        strNetmask = strprintf("%u", cidr);
    } else {
        if (network.IsIPv4())
            strNetmask = strprintf("%u.%u.%u.%u", netmask[12], netmask[13], netmask[14], netmask[15]);
        else
            strNetmask = strprintf("%x:%x:%x:%x:%x:%x:%x:%x",
                             netmask[0] << 8 | netmask[1], netmask[2] << 8 | netmask[3],
                             netmask[4] << 8 | netmask[5], netmask[6] << 8 | netmask[7],
                             netmask[8] << 8 | netmask[9], netmask[10] << 8 | netmask[11],
                             netmask[12] << 8 | netmask[13], netmask[14] << 8 | netmask[15]);
    }

    return network.ToString() + "/" + strNetmask;
}

bool CSubNet::IsValid() const
{
    return valid;
}

bool operator==(const CSubNet& a, const CSubNet& b)
{
    return a.valid == b.valid && a.network == b.network && !memcmp(a.netmask, b.netmask, 16);
}

bool operator<(const CSubNet& a, const CSubNet& b)
{
    return (a.network < b.network || (a.network == b.network && memcmp(a.netmask, b.netmask, 16) < 0));
}
