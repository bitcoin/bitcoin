// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <netaddress.h>
#include <hash.h>
#include <util/strencodings.h>
#include <tinyformat.h>

static const unsigned char pchIPv4[12] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xff, 0xff };
static const unsigned char pchOnionCat[] = {0xFD,0x87,0xD8,0x7E,0xEB,0x43};

// 0xFD + sha256("bitcoin")[0:5]
static const unsigned char g_internal_prefix[] = { 0xFD, 0x6B, 0x88, 0xC0, 0x87, 0x24 };

void CNetAddr::SetIP(const CNetAddr& ipIn)
{
    m_network_id = ipIn.m_network_id;
    ip = ipIn.ip;
}

void CNetAddr::SetRaw(Network network, const uint8_t *ip_in)
{
    switch(network)
    {
        case NET_IPV4:
            ip.assign(ip_in, ip_in+4);
            m_network_id = NetworkID::IPV4;
            break;
        case NET_IPV6:
            ip.assign(ip_in, ip_in+16);
            m_network_id = NetworkID::IPV6;
            break;
        default:
            assert(!"invalid network");
    }
}

/**
 * Try to make this a dummy address that maps the specified name into IPv6 like
 * so: (0xFD + %sha256("bitcoin")[0:5]) + %sha256(name)[0:10]. Such dummy
 * addresses have a prefix of fd6b:88c0:8724::/48 and are guaranteed to not be
 * publicly routable as it falls under RFC4193's fc00::/7 subnet allocated to
 * unique-local addresses.
 *
 * CAddrMan uses these fake addresses to keep track of which DNS seeds were
 * used.
 *
 * @returns Whether or not the operation was successful.
 *
 * @see CNetAddr::IsInternal(), CNetAddr::IsRFC4193()
 */
bool CNetAddr::SetInternal(const std::string &name)
{
    if (name.empty()) {
        return false;
    }
    unsigned char hash[32] = {};
    CSHA256().Write((const unsigned char*)name.data(), name.size()).Finalize(hash);
    ip.assign(hash, hash+16-sizeof(g_internal_prefix));
    m_network_id = NetworkID::NAME;
    return true;
}

/**
 * Try to make this a dummy address that maps the specified onion address into
 * IPv6 using OnionCat's range and encoding. Such dummy addresses have a prefix
 * of fd87:d87e:eb43::/48 and are guaranteed to not be publicly routable as they
 * fall under RFC4193's fc00::/7 subnet allocated to unique-local addresses.
 *
 * @returns Whether or not the operation was successful.
 *
 * @see CNetAddr::IsTor(), CNetAddr::IsRFC4193()
 */
bool CNetAddr::SetSpecial(const std::string &strName)
{
    if (strName.size()>6 && strName.substr(strName.size() - 6, 6) == ".onion") {
        std::vector<unsigned char> vchAddr = DecodeBase32(strName.substr(0, strName.size() - 6).c_str());
        if (vchAddr.size() == 10) {
            m_network_id = NetworkID::TORV2;
            ip.assign(vchAddr.begin(), vchAddr.end());
            return true;
        } else if (vchAddr.size() == 35) {
            m_network_id = NetworkID::TORV3;
            ip.assign(vchAddr.begin(), vchAddr.begin()+32);
            return true;
        }
    } else if (strName.size()>8 && strName.substr(strName.size() - 8, 8) == ".b32.i2p") {
        std::vector<unsigned char> vchAddr = DecodeBase32(strName.substr(0, strName.size() - 8).c_str());
        if (vchAddr.size() == 32) {
            m_network_id = NetworkID::I2P;
            ip.assign(vchAddr.begin(), vchAddr.end());
            return true;
        }
    }
    return false;
}

CNetAddr::CNetAddr(const struct in_addr& ipv4Addr)
{
    SetRaw(NET_IPV4, (const uint8_t*)&ipv4Addr);
}

CNetAddr::CNetAddr(const struct in6_addr& ipv6Addr, const uint32_t scope) : scopeId(scope)
{
    unsigned char *address = (unsigned char *)&ipv6Addr;
    if (*address == 0xfc) {
        m_network_id = NetworkID::CJDNS;
        ip.assign(address, address+16);
    } else {
        FromV1Serialization(address);
    }
}

unsigned int CNetAddr::GetByte(int n) const
{
    return ip.at(ip.size()-1-n);
}

bool CNetAddr::IsBindAny() const
{
    const int cmplen = IsIPv4() ? 4 : 16;
    for (int i = 0; i < cmplen; ++i) {
        if (GetByte(i)) return false;
    }

    return true;
}

bool CNetAddr::IsIPv4() const
{
    return m_network_id == NetworkID::IPV4;
}

bool CNetAddr::IsIPv6() const
{
    return m_network_id == NetworkID::IPV6;
}

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
    return IsIPv6() && (GetByte(15) == 0x20 && GetByte(14) == 0x01 && GetByte(13) == 0x0D && GetByte(12) == 0xB8);
}

bool CNetAddr::IsRFC3964() const
{
    return IsIPv6() && (GetByte(15) == 0x20 && GetByte(14) == 0x02);
}

bool CNetAddr::IsRFC6052() const
{
    static const unsigned char pchRFC6052[] = {0,0x64,0xFF,0x9B,0,0,0,0,0,0,0,0};
    return IsIPv6() && (memcmp(ip.data(), pchRFC6052, sizeof(pchRFC6052)) == 0);
}

bool CNetAddr::IsRFC4380() const
{
    return IsIPv6() && (GetByte(15) == 0x20 && GetByte(14) == 0x01 && GetByte(13) == 0 && GetByte(12) == 0);
}

bool CNetAddr::IsRFC4862() const
{
    static const unsigned char pchRFC4862[] = {0xFE,0x80,0,0,0,0,0,0};
    return IsIPv6() && (memcmp(ip.data(), pchRFC4862, sizeof(pchRFC4862)) == 0);
}

bool CNetAddr::IsRFC4193() const
{
    return m_network_id == NetworkID::CJDNS || (IsIPv6() && ((GetByte(15) & 0xFE) == 0xFC));
}

bool CNetAddr::IsRFC6145() const
{
    static const unsigned char pchRFC6145[] = {0,0,0,0,0,0,0,0,0xFF,0xFF,0,0};
    return IsIPv6() && (memcmp(ip.data(), pchRFC6145, sizeof(pchRFC6145)) == 0);
}

bool CNetAddr::IsRFC4843() const
{
    return IsIPv6() && (GetByte(15) == 0x20 && GetByte(14) == 0x01 && GetByte(13) == 0x00 && (GetByte(12) & 0xF0) == 0x10);
}

bool CNetAddr::IsRFC7343() const
{
    return IsIPv6() && (GetByte(15) == 0x20 && GetByte(14) == 0x01 && GetByte(13) == 0x00 && (GetByte(12) & 0xF0) == 0x20);
}

/**
 * @returns Whether or not this is a dummy address that maps an onion address
 *          into IPv6.
 *
 * @see CNetAddr::SetSpecial(const std::string &)
 */
bool CNetAddr::IsTor() const
{
    return m_network_id == NetworkID::TORV2;
}

bool CNetAddr::IsLocal() const
{
    // IPv4 loopback (127.0.0.0/8 or 0.0.0.0/8)
    if (IsIPv4() && (GetByte(3) == 127 || GetByte(3) == 0))
        return true;

    // IPv6 loopback (::1/128)
    static const unsigned char pchLocal[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1};
    if (IsIPv6() && memcmp(ip.data(), pchLocal, 16) == 0)
        return true;

    return false;
}

/**
 * @returns Whether or not this network address is a valid address that @a could
 *          be used to refer to an actual host.
 *
 * @note A valid address may or may not be publicly routable on the global
 *       internet. As in, the set of valid addresses is a superset of the set of
 *       publicly routable addresses.
 *
 * @see CNetAddr::IsRoutable()
 */
bool CNetAddr::IsValid() const
{
    // Cleanup 3-byte shifted addresses caused by garbage in size field
    // of addr messages from versions before 0.2.9 checksum.
    // Two consecutive addr messages look like this:
    // header20 vectorlen3 addr26 addr26 addr26 header20 vectorlen3 addr26 addr26 addr26...
    // so if the first length field is garbled, it reads the second batch
    // of addr misaligned by 3 bytes.
    if (IsIPv6() && memcmp(ip.data(), pchIPv4+3, sizeof(pchIPv4)-3) == 0) {
        return false;
    }

    // unspecified IPv6 address (::/128)
    unsigned char ipNone6[16] = {};
    if (IsIPv6() && memcmp(ip.data(), ipNone6, 16) == 0) {
        return false;
    }

    // documentation IPv6 address
    if (IsRFC3849())
        return false;

    if (IsInternal())
        return false;

    if (IsIPv4())
    {
        // INADDR_NONE
        uint32_t ipNone = INADDR_NONE;
        if (memcmp(ip.data(), &ipNone, 4) == 0) {
            return false;
        }

        // 0
        ipNone = 0;
        if (memcmp(ip.data(), &ipNone, 4) == 0) {
            return false;
        }
    }

    return true;
}

/**
 * @returns Whether or not this network address is publicly routable on the
 *          global internet.
 *
 * @note A routable address is always valid. As in, the set of routable addresses
 *       is a subset of the set of valid addresses.
 *
 * @see CNetAddr::IsValid()
 */
bool CNetAddr::IsRoutable() const
{
    return IsValid() && !(IsRFC1918() || IsRFC2544() || IsRFC3927() || IsRFC4862() || IsRFC6598() || IsRFC5737() || (IsRFC4193() && !IsTor()) || IsRFC4843() || IsRFC7343() || IsLocal() || IsInternal());
}

/**
 * @returns Whether or not this is a dummy address that maps a name into IPv6.
 *
 * @see CNetAddr::SetInternal(const std::string &)
 */
bool CNetAddr::IsInternal() const
{
    return m_network_id == NetworkID::NAME;
}

enum Network CNetAddr::GetNetwork() const
{
    if (IsInternal())
        return NET_INTERNAL;

    if (!IsRoutable())
        return NET_UNROUTABLE;

    if (IsIPv4())
        return NET_IPV4;

    if (IsTor())
        return NET_ONION;

    return NET_IPV6;
}

std::string CNetAddr::ToStringIP() const
{
    if (IsTor())
        return EncodeBase32(ip.data(), 10) + ".onion";
    if (IsInternal())
        return EncodeBase32(ip.data(), 16 - sizeof(g_internal_prefix)) + ".internal";
    CService serv(*this, 0);
    struct sockaddr_storage sockaddr;
    socklen_t socklen = sizeof(sockaddr);
    if (serv.GetSockAddr((struct sockaddr*)&sockaddr, &socklen)) {
        char name[1025] = "";
        if (!getnameinfo((const struct sockaddr*)&sockaddr, socklen, name, sizeof(name), nullptr, 0, NI_NUMERICHOST))
            return std::string(name);
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
    return a.m_network_id == b.m_network_id && a.ip == b.ip;
}

bool operator<(const CNetAddr& a, const CNetAddr& b)
{
    if (a.m_network_id == b.m_network_id) {
        return a.ip < b.ip;
    } else {
        return a.m_network_id < b.m_network_id;
    }
}

/**
 * Try to get our IPv4 address.
 *
 * @param[out] pipv4Addr The in_addr struct to which to copy.
 *
 * @returns Whether or not the operation was successful, in particular, whether
 *          or not our address was an IPv4 address.
 *
 * @see CNetAddr::IsIPv4()
 */
bool CNetAddr::GetInAddr(struct in_addr* pipv4Addr) const
{
    if (!IsIPv4())
        return false;
    memcpy(pipv4Addr, ip.data(), 4);
    return true;
}

/**
 * Try to get our IPv6 address.
 *
 * @param[out] pipv6Addr The in6_addr struct to which to copy.
 *
 * @returns Whether or not the operation was successful, in particular, whether
 *          or not our address was an IPv6 address.
 *
 * @see CNetAddr::IsIPv6()
 */
bool CNetAddr::GetIn6Addr(struct in6_addr* pipv6Addr) const
{
    if (!IsIPv6()) {
        return false;
    }
    memcpy(pipv6Addr, ip.data(), 16);
    return true;
}

void CNetAddr::FromV1Serialization(unsigned char *buff) {
    if (memcmp(buff, pchIPv4, sizeof(pchIPv4)) == 0) {
        // IPv4-in-IPv6
        SetRaw(NET_IPV4, (const uint8_t *)buff+sizeof(pchIPv4));
    } else if (memcmp(buff, pchOnionCat, sizeof(pchOnionCat)) == 0) {
        // OnionCat
        ip.assign(buff+sizeof(pchOnionCat), buff+16);
        m_network_id = NetworkID::TORV2;
    } else if (memcmp(buff, g_internal_prefix, sizeof(g_internal_prefix)) == 0) {
        // Custom Internal
        ip.assign(buff+sizeof(g_internal_prefix), buff+16);
        m_network_id = NetworkID::NAME;
    } else { // Actually IPv6
        SetRaw(NET_IPV6, (const uint8_t*)buff);
    }
}

bool CNetAddr::GetV1Serialization(unsigned char *buff) const {
    switch (m_network_id) {
    case NetworkID::IPV4:
        memcpy(buff, pchIPv4, sizeof(pchIPv4));
        memcpy(buff+sizeof(pchIPv4), ip.data(), 16-sizeof(pchIPv4));
        return true;
    case NetworkID::TORV2:
        memcpy(buff, pchOnionCat, sizeof(pchOnionCat));
        memcpy(buff+sizeof(pchOnionCat), ip.data(), 16-sizeof(pchOnionCat));
        return true;
    case NetworkID::NAME:
        memcpy(buff, g_internal_prefix, sizeof(g_internal_prefix));
        memcpy(buff+sizeof(g_internal_prefix), ip.data(), 16-sizeof(g_internal_prefix));
        return true;
    case NetworkID::IPV6:
        memcpy(buff, ip.data(), 16);
        return true;
    default:
        return false;
    }
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
std::vector<unsigned char> CNetAddr::GetGroup() const
{
    std::vector<unsigned char> vchRet;
    int nClass = NET_IPV6;
    int nStartByte = 0;
    int nBits = 16;

    // all local addresses belong to the same group
    if (IsLocal())
    {
        nClass = 255;
        nBits = 0;
    }
    // all internal-usage addresses get their own group
    if (IsInternal())
    {
        nClass = NET_INTERNAL;
        nBits = (16 - sizeof(g_internal_prefix)) * 8;
    }
    // all other unroutable addresses belong to the same group
    else if (!IsRoutable())
    {
        nClass = NET_UNROUTABLE;
        nBits = 0;
    }
    // for IPv4 addresses, '1' + the 16 higher-order bits of the IP
    // includes mapped IPv4, SIIT translated IPv4, and the well-known prefix
    else if (IsIPv4())
    {
        nClass = NET_IPV4;
    }
    else if (IsRFC6145() || IsRFC6052())
    {
        nClass = NET_IPV4;
        nStartByte = 12;
    }
    // for 6to4 tunnelled addresses, use the encapsulated IPv4 address
    else if (IsRFC3964())
    {
        nClass = NET_IPV4;
        nStartByte = 2;
    }
    // for Teredo-tunnelled IPv6 addresses, use the encapsulated IPv4 address
    else if (IsRFC4380())
    {
        vchRet.push_back(NET_IPV4);
        vchRet.push_back(GetByte(3) ^ 0xFF);
        vchRet.push_back(GetByte(2) ^ 0xFF);
        return vchRet;
    }
    else if (IsTor())
    {
        nClass = NET_ONION;
        nBits = 4;
    }
    // for he.net, use /36 groups
    else if (GetByte(15) == 0x20 && GetByte(14) == 0x01 && GetByte(13) == 0x04 && GetByte(12) == 0x70)
        nBits = 36;
    // for the rest of the IPv6 network, use /32 groups
    else
        nBits = 32;

    vchRet.push_back(nClass);

    // push our ip onto vchRet byte by byte...
    while (nBits >= 8)
    {
        vchRet.push_back(ip.at(nStartByte));
        nStartByte++;
        nBits -= 8;
    }
    // ...for the last byte, push nBits and for the rest of the byte push 1's
    if (nBits > 0)
        vchRet.push_back(ip.at(nStartByte) | ((1 << (8 - nBits)) - 1));

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

/**
 * Obtain the IPv4/6 socket address this represents.
 *
 * @param[out] paddr The obtained socket address.
 * @param[in,out] addrlen The size, in bytes, of the address structure pointed
 *                        to by paddr. The value that's pointed to by this
 *                        parameter might change after calling this function if
 *                        the size of the corresponding address structure
 *                        changed.
 *
 * @returns Whether or not the operation was successful.
 */
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

/**
 * @returns An identifier unique to this service's address and port number.
 */
std::vector<unsigned char> CService::GetKey() const
{
     std::vector<unsigned char> vKey;
     vKey.resize(16);
     if (!GetV1Serialization(vKey.data())) {
         vKey.assign(ip.begin(), ip.end());
     }
     vKey.push_back(port / 0x100);
     vKey.push_back(port & 0x0FF);
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
    } else {
        return "[" + ToStringIP() + "]:" + ToStringPort();
    }
}

std::string CService::ToString() const
{
    return ToStringIPPort();
}

CSubNet::CSubNet():
    valid(false)
{
    memset(netmask, 0, sizeof(netmask));
}

CSubNet::CSubNet(const CNetAddr &addr, int32_t mask)
{
    valid = (addr.m_network_id == NetworkID::IPV4 || addr.m_network_id == NetworkID::IPV6);
    network = addr;
    // Default to /32 (IPv4) or /128 (IPv6), i.e. match single address
    memset(netmask, 255, sizeof(netmask));

    // IPv4 addresses start at offset 12, and first 12 bytes must match, so just offset n
    const int alens = network.ip.size();

    int32_t n = mask;
    if(n >= 0 && n <= (alens*8)) // Only valid if in range of bits of address
    {
        // Clear bits [n..127]
        for (; n < 128; ++n)
            netmask[n>>3] &= ~(1<<(7-(n&7)));
    } else {
        valid = false;
    }

    // Normalize network according to netmask
    for(size_t x=0; x<alens; ++x)
        network.ip.at(x) &= netmask[x];
}

CSubNet::CSubNet(const CNetAddr &addr, const CNetAddr &mask)
{
    valid = (addr.m_network_id == NetworkID::IPV4 || addr.m_network_id == NetworkID::IPV6);
    network = addr;
    // Default to /32 (IPv4) or /128 (IPv6), i.e. match single address
    memset(netmask, 255, sizeof(netmask));

    // IPv4 addresses start at offset 12, and first 12 bytes must match, so just offset n
    const int alens = network.ip.size();

    for(int x=0; x<alens; ++x)
        netmask[x] = mask.ip.at(x);
    for(int x=alens; x<16; ++x)
        netmask[x] = 0;

    // Normalize network according to netmask
    for(size_t x=0; x<network.ip.size(); ++x)
        network.ip.at(x) &= netmask[x];
}

CSubNet::CSubNet(const CNetAddr &addr):
    valid(addr.IsValid())
{
    valid &= (addr.m_network_id == NetworkID::IPV4 || addr.m_network_id == NetworkID::IPV6);

    memset(netmask, 0, sizeof(netmask));
    memset(netmask, 255, addr.ip.size());

    network = addr;
}

/**
 * @returns True if this subnet is valid, the specified address is valid, and
 *          the specified address belongs in this subnet.
 */
bool CSubNet::Match(const CNetAddr &addr) const
{
    // Construct a ::FFFF:0:0 with NetworkID::IPV6 as m_network_id
    std::vector<unsigned char> blank_ipv4_mapped(std::begin(pchIPv4), std::end(pchIPv4));
    blank_ipv4_mapped.resize(16, 0);
    CNetAddr blank_ipv4_mapped_address;
    blank_ipv4_mapped_address.SetRaw(NET_IPV6, blank_ipv4_mapped.data());

    // We consider IPv4 as a subnet of IPv6. Therefore, any IPv6 subnet that
    // contains ::FFFF:0:0 also contains all of IPv4.
    if (network.IsIPv6() && addr.IsIPv4() && Match(blank_ipv4_mapped_address)) {
        return true;
    }
    if (!valid || !addr.IsValid() || addr.m_network_id != network.m_network_id)
        return false;
    for(size_t x=0; x<addr.ip.size(); ++x)
        if ((addr.ip.at(x) & netmask[x]) != network.ip.at(x))
            return false;
    return true;
}

/**
 * @returns The number of 1-bits in the prefix of the specified subnet mask. If
 *          the specified subnet mask is not a valid one, -1.
 */
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
    int n = 0;
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
            strNetmask = strprintf("%u.%u.%u.%u", netmask[0], netmask[1], netmask[2], netmask[3]);
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
