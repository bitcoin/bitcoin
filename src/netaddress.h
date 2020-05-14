// Copyright (c) 2009-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NETADDRESS_H
#define BITCOIN_NETADDRESS_H

#if defined(HAVE_CONFIG_H)
#include <config/bitcoin-config.h>
#endif

#include <compat.h>
#include <serialize.h>

#include <cstdint>
#include <string>
#include <vector>

/**
 * A network type.
 * @note An address may belong to more than one network, for example `10.0.0.1`
 * belongs to both `NET_UNROUTABLE` and `NET_IPV4`.
 * Keep these sequential starting from 0 and `NET_MAX` as the last entry.
 * We have loops like `for (int i = 0; i < NET_MAX; i++)` that expect to iterate
 * over all enum values and also `GetExtNetwork()` "extends" this enum by
 * introducing standalone constants starting from `NET_MAX`.
 */
enum Network
{
    /// Addresses from these networks are not publicly routable on the global Internet.
    NET_UNROUTABLE = 0,

    /// IPv4
    NET_IPV4,

    /// IPv6
    NET_IPV6,

    /// TORv2
    NET_ONION,

    /// A set of dummy addresses that map a name to an IPv6 address. These
    /// addresses belong to RFC4193's fc00::/7 subnet (unique-local addresses).
    /// We use them to map a string or FQDN to an IPv6 address in CAddrMan to
    /// keep track of which DNS seeds were used.
    NET_INTERNAL,

    /// Dummy value to indicate the number of NET_* constants.
    NET_MAX,
};

/**
 * Network address.
 */
class CNetAddr
{
    protected:
        /**
         * Network to which this address belongs.
         */
        Network m_net{NET_IPV6};

        unsigned char ip[16]; // in network byte order
        uint32_t scopeId{0}; // for scoped/link-local ipv6 addresses

    public:
        CNetAddr();
        explicit CNetAddr(const struct in_addr& ipv4Addr);
        void SetIP(const CNetAddr& ip);

        /**
         * Set from a legacy IPv6 address.
         * Legacy IPv6 address may be a normal IPv6 address, or another address
         * (e.g. IPv4) disguised as IPv6. This encoding is used in the legacy
         * `addr` encoding.
         */
        void SetLegacyIPv6(const uint8_t ipv6[16]);

        /**
         * Set raw IPv4 or IPv6 address (in network byte order)
         * @note Only NET_IPV4 and NET_IPV6 are allowed for network.
         */
        void SetRaw(Network network, const uint8_t *data);

        bool SetInternal(const std::string& name);

        bool SetSpecial(const std::string &strName); // for Tor addresses
        bool IsBindAny() const; // INADDR_ANY equivalent
        bool IsIPv4() const;    // IPv4 mapped address (::FFFF:0:0/96, 0.0.0.0/0)
        bool IsIPv6() const;    // IPv6 address (not mapped IPv4, not Tor)
        bool IsRFC1918() const; // IPv4 private networks (10.0.0.0/8, 192.168.0.0/16, 172.16.0.0/12)
        bool IsRFC2544() const; // IPv4 inter-network communications (198.18.0.0/15)
        bool IsRFC6598() const; // IPv4 ISP-level NAT (100.64.0.0/10)
        bool IsRFC5737() const; // IPv4 documentation addresses (192.0.2.0/24, 198.51.100.0/24, 203.0.113.0/24)
        bool IsRFC3849() const; // IPv6 documentation address (2001:0DB8::/32)
        bool IsRFC3927() const; // IPv4 autoconfig (169.254.0.0/16)
        bool IsRFC3964() const; // IPv6 6to4 tunnelling (2002::/16)
        bool IsRFC4193() const; // IPv6 unique local (FC00::/7)
        bool IsRFC4380() const; // IPv6 Teredo tunnelling (2001::/32)
        bool IsRFC4843() const; // IPv6 ORCHID (deprecated) (2001:10::/28)
        bool IsRFC7343() const; // IPv6 ORCHIDv2 (2001:20::/28)
        bool IsRFC4862() const; // IPv6 autoconfig (FE80::/64)
        bool IsRFC6052() const; // IPv6 well-known prefix for IPv4-embedded address (64:FF9B::/96)
        bool IsRFC6145() const; // IPv6 IPv4-translated address (::FFFF:0:0:0/96) (actually defined in RFC2765)
        bool IsHeNet() const;   // IPv6 Hurricane Electric - https://he.net (2001:0470::/36)
        bool IsTor() const;
        bool IsLocal() const;
        bool IsRoutable() const;
        bool IsInternal() const;
        bool IsValid() const;
        enum Network GetNetwork() const;
        std::string ToString() const;
        std::string ToStringIP() const;
        unsigned int GetByte(int n) const;
        uint64_t GetHash() const;
        bool GetInAddr(struct in_addr* pipv4Addr) const;
        uint32_t GetNetClass() const;

        //! For IPv4, mapped IPv4, SIIT translated IPv4, Teredo, 6to4 tunneled addresses, return the relevant IPv4 address as a uint32.
        uint32_t GetLinkedIPv4() const;
        //! Whether this address has a linked IPv4 address (see GetLinkedIPv4()).
        bool HasLinkedIPv4() const;

        // The AS on the BGP path to the node we use to diversify
        // peers in AddrMan bucketing based on the AS infrastructure.
        // The ip->AS mapping depends on how asmap is constructed.
        uint32_t GetMappedAS(const std::vector<bool> &asmap) const;

        std::vector<unsigned char> GetGroup(const std::vector<bool> &asmap) const;
        std::vector<unsigned char> GetAddrBytes() const { return {std::begin(ip), std::end(ip)}; }
        int GetReachabilityFrom(const CNetAddr *paddrPartner = nullptr) const;

        explicit CNetAddr(const struct in6_addr& pipv6Addr, const uint32_t scope = 0);
        bool GetIn6Addr(struct in6_addr* pipv6Addr) const;

        friend bool operator==(const CNetAddr& a, const CNetAddr& b);
        friend bool operator!=(const CNetAddr& a, const CNetAddr& b) { return !(a == b); }
        friend bool operator<(const CNetAddr& a, const CNetAddr& b);

        /**
         * Serialize to a stream.
         */
        template <typename Stream>
        void Serialize(Stream& s) const
        {
            s << ip;
        }

        /**
         * Unserialize from a stream.
         */
        template <typename Stream>
        void Unserialize(Stream& s)
        {
            unsigned char ip_temp[sizeof(ip)];
            s >> ip_temp;
            // Use SetLegacyIPv6() so that m_net is set correctly. For example
            // ::FFFF:0102:0304 should be set as m_net=NET_IPV4 (1.2.3.4).
            SetLegacyIPv6(ip_temp);
        }

        friend class CSubNet;
};

class CSubNet
{
    protected:
        /// Network (base) address
        CNetAddr network;
        /// Netmask, in network byte order
        uint8_t netmask[16];
        /// Is this value valid? (only used to signal parse errors)
        bool valid;

    public:
        CSubNet();
        CSubNet(const CNetAddr &addr, int32_t mask);
        CSubNet(const CNetAddr &addr, const CNetAddr &mask);

        //constructor for single ip subnet (<ipv4>/32 or <ipv6>/128)
        explicit CSubNet(const CNetAddr &addr);

        bool Match(const CNetAddr &addr) const;

        std::string ToString() const;
        bool IsValid() const;

        friend bool operator==(const CSubNet& a, const CSubNet& b);
        friend bool operator!=(const CSubNet& a, const CSubNet& b) { return !(a == b); }
        friend bool operator<(const CSubNet& a, const CSubNet& b);

        SERIALIZE_METHODS(CSubNet, obj) { READWRITE(obj.network, obj.netmask, obj.valid); }
};

/** A combination of a network address (CNetAddr) and a (TCP) port */
class CService : public CNetAddr
{
    protected:
        uint16_t port; // host order

    public:
        CService();
        CService(const CNetAddr& ip, uint16_t port);
        CService(const struct in_addr& ipv4Addr, uint16_t port);
        explicit CService(const struct sockaddr_in& addr);
        uint16_t GetPort() const;
        bool GetSockAddr(struct sockaddr* paddr, socklen_t *addrlen) const;
        bool SetSockAddr(const struct sockaddr* paddr);
        friend bool operator==(const CService& a, const CService& b);
        friend bool operator!=(const CService& a, const CService& b) { return !(a == b); }
        friend bool operator<(const CService& a, const CService& b);
        std::vector<unsigned char> GetKey() const;
        std::string ToString() const;
        std::string ToStringPort() const;
        std::string ToStringIPPort() const;

        CService(const struct in6_addr& ipv6Addr, uint16_t port);
        explicit CService(const struct sockaddr_in6& addr);

        SERIALIZE_METHODS(CService, obj)
        {
            READWRITEAS(CNetAddr, obj);
            READWRITE(Using<BigEndianFormatter<2>>(obj.port));
        }
};

bool SanityCheckASMap(const std::vector<bool>& asmap);

#endif // BITCOIN_NETADDRESS_H
