// Copyright (c) 2009-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NETADDRESS_H
#define BITCOIN_NETADDRESS_H

#include <compat/compat.h>
#include <crypto/siphash.h>
#include <prevector.h>
#include <random.h>
#include <serialize.h>
#include <tinyformat.h>
#include <util/strencodings.h>
#include <util/string.h>

#include <array>
#include <cstdint>
#include <ios>
#include <string>
#include <vector>

/**
 * A network type.
 * @note An address may belong to more than one network, for example `10.0.0.1`
 * belongs to both `NET_UNROUTABLE` and `NET_IPV4`.
 * Keep these sequential starting from 0 and `NET_MAX` as the last entry.
 * We have loops like `for (int i = 0; i < NET_MAX; ++i)` that expect to iterate
 * over all enum values and also `GetExtNetwork()` "extends" this enum by
 * introducing standalone constants starting from `NET_MAX`.
 */
enum Network {
    /// Addresses from these networks are not publicly routable on the global Internet.
    NET_UNROUTABLE = 0,

    /// IPv4
    NET_IPV4,

    /// IPv6
    NET_IPV6,

    /// TOR (v2 or v3)
    NET_ONION,

    /// I2P
    NET_I2P,

    /// CJDNS
    NET_CJDNS,

    /// A set of addresses that represent the hash of a string or FQDN. We use
    /// them in AddrMan to keep track of which DNS seeds were used.
    NET_INTERNAL,

    /// Dummy value to indicate the number of NET_* constants.
    NET_MAX,
};

/// Prefix of an IPv6 address when it contains an embedded IPv4 address.
/// Used when (un)serializing addresses in ADDRv1 format (pre-BIP155).
static const std::array<uint8_t, 12> IPV4_IN_IPV6_PREFIX{
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF};

/// Prefix of an IPv6 address when it contains an embedded TORv2 address.
/// Used when (un)serializing addresses in ADDRv1 format (pre-BIP155).
/// Such dummy IPv6 addresses are guaranteed to not be publicly routable as they
/// fall under RFC4193's fc00::/7 subnet allocated to unique-local addresses.
static const std::array<uint8_t, 6> TORV2_IN_IPV6_PREFIX{
    0xFD, 0x87, 0xD8, 0x7E, 0xEB, 0x43};

/// Prefix of an IPv6 address when it contains an embedded "internal" address.
/// Used when (un)serializing addresses in ADDRv1 format (pre-BIP155).
/// The prefix comes from 0xFD + SHA256("bitcoin")[0:5].
/// Such dummy IPv6 addresses are guaranteed to not be publicly routable as they
/// fall under RFC4193's fc00::/7 subnet allocated to unique-local addresses.
static const std::array<uint8_t, 6> INTERNAL_IN_IPV6_PREFIX{
    0xFD, 0x6B, 0x88, 0xC0, 0x87, 0x24 // 0xFD + sha256("bitcoin")[0:5].
};

/// All CJDNS addresses start with 0xFC. See
/// https://github.com/cjdelisle/cjdns/blob/master/doc/Whitepaper.md#pulling-it-all-together
static constexpr uint8_t CJDNS_PREFIX{0xFC};

/// Size of IPv4 address (in bytes).
static constexpr size_t ADDR_IPV4_SIZE = 4;

/// Size of IPv6 address (in bytes).
static constexpr size_t ADDR_IPV6_SIZE = 16;

/// Size of TORv3 address (in bytes). This is the length of just the address
/// as used in BIP155, without the checksum and the version byte.
static constexpr size_t ADDR_TORV3_SIZE = 32;

/// Size of I2P address (in bytes).
static constexpr size_t ADDR_I2P_SIZE = 32;

/// Size of CJDNS address (in bytes).
static constexpr size_t ADDR_CJDNS_SIZE = 16;

/// Size of "internal" (NET_INTERNAL) address (in bytes).
static constexpr size_t ADDR_INTERNAL_SIZE = 10;

/// SAM 3.1 and earlier do not support specifying ports and force the port to 0.
static constexpr uint16_t I2P_SAM31_PORT{0};

std::string OnionToString(Span<const uint8_t> addr);

/**
 * Network address.
 */
class CNetAddr
{
protected:
    /**
     * Raw representation of the network address.
     * In network byte order (big endian) for IPv4 and IPv6.
     */
    prevector<ADDR_IPV6_SIZE, uint8_t> m_addr{ADDR_IPV6_SIZE, 0x0};

    /**
     * Network to which this address belongs.
     */
    Network m_net{NET_IPV6};

    /**
     * Scope id if scoped/link-local IPV6 address.
     * See https://tools.ietf.org/html/rfc4007
     */
    uint32_t m_scope_id{0};

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
    void SetLegacyIPv6(Span<const uint8_t> ipv6);

    bool SetInternal(const std::string& name);

    /**
     * Parse a Tor or I2P address and set this object to it.
     * @param[in] addr Address to parse, for example
     * pg6mmjiyjmcrsslvykfwnntlaru7p5svn6y2ymmju6nubxndf4pscryd.onion or
     * ukeu3k5oycgaauneqgtnvselmt4yemvoilkln7jpvamvfx7dnkdq.b32.i2p.
     * @returns Whether the operation was successful.
     * @see CNetAddr::IsTor(), CNetAddr::IsI2P()
     */
    bool SetSpecial(const std::string& addr);

    bool IsBindAny() const; // INADDR_ANY equivalent
    [[nodiscard]] bool IsIPv4() const { return m_net == NET_IPV4; } // IPv4 mapped address (::FFFF:0:0/96, 0.0.0.0/0)
    [[nodiscard]] bool IsIPv6() const { return m_net == NET_IPV6; } // IPv6 address (not mapped IPv4, not Tor)
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
    [[nodiscard]] bool IsTor() const { return m_net == NET_ONION; }
    [[nodiscard]] bool IsI2P() const { return m_net == NET_I2P; }
    [[nodiscard]] bool IsCJDNS() const { return m_net == NET_CJDNS; }
    [[nodiscard]] bool HasCJDNSPrefix() const { return m_addr[0] == CJDNS_PREFIX; }
    bool IsLocal() const;
    bool IsRoutable() const;
    bool IsInternal() const;
    bool IsValid() const;

    /**
     * Whether this object is a privacy network.
     * TODO: consider adding IsCJDNS() here when more peers adopt CJDNS, see:
     * https://github.com/bitcoin/bitcoin/pull/27411#issuecomment-1497176155
     */
    [[nodiscard]] bool IsPrivacyNet() const { return IsTor() || IsI2P(); }

    /**
     * Check if the current object can be serialized in pre-ADDRv2/BIP155 format.
     */
    bool IsAddrV1Compatible() const;

    enum Network GetNetwork() const;
    std::string ToStringAddr() const;
    bool GetInAddr(struct in_addr* pipv4Addr) const;
    Network GetNetClass() const;

    //! For IPv4, mapped IPv4, SIIT translated IPv4, Teredo, 6to4 tunneled addresses, return the relevant IPv4 address as a uint32.
    uint32_t GetLinkedIPv4() const;
    //! Whether this address has a linked IPv4 address (see GetLinkedIPv4()).
    bool HasLinkedIPv4() const;

    std::vector<unsigned char> GetAddrBytes() const;
    int GetReachabilityFrom(const CNetAddr& paddrPartner) const;

    explicit CNetAddr(const struct in6_addr& pipv6Addr, const uint32_t scope = 0);
    bool GetIn6Addr(struct in6_addr* pipv6Addr) const;

    friend bool operator==(const CNetAddr& a, const CNetAddr& b);
    friend bool operator!=(const CNetAddr& a, const CNetAddr& b) { return !(a == b); }
    friend bool operator<(const CNetAddr& a, const CNetAddr& b);

    /**
     * Whether this address should be relayed to other peers even if we can't reach it ourselves.
     */
    bool IsRelayable() const
    {
        return IsIPv4() || IsIPv6() || IsTor() || IsI2P() || IsCJDNS();
    }

    enum class Encoding {
        V1,
        V2, //!< BIP155 encoding
    };
    struct SerParams {
        const Encoding enc;
        SER_PARAMS_OPFUNC
    };
    static constexpr SerParams V1{Encoding::V1};
    static constexpr SerParams V2{Encoding::V2};

    /**
     * Serialize to a stream.
     */
    template <typename Stream>
    void Serialize(Stream& s) const
    {
        if (s.template GetParams<SerParams>().enc == Encoding::V2) {
            SerializeV2Stream(s);
        } else {
            SerializeV1Stream(s);
        }
    }

    /**
     * Unserialize from a stream.
     */
    template <typename Stream>
    void Unserialize(Stream& s)
    {
        if (s.template GetParams<SerParams>().enc == Encoding::V2) {
            UnserializeV2Stream(s);
        } else {
            UnserializeV1Stream(s);
        }
    }

    /**
     * BIP155 network ids recognized by this software.
     */
    enum BIP155Network : uint8_t {
        IPV4 = 1,
        IPV6 = 2,
        TORV2 = 3,
        TORV3 = 4,
        I2P = 5,
        CJDNS = 6,
    };

    friend class CSubNet;

private:
    /**
     * Parse a Tor address and set this object to it.
     * @param[in] addr Address to parse, must be a valid C string, for example
     * pg6mmjiyjmcrsslvykfwnntlaru7p5svn6y2ymmju6nubxndf4pscryd.onion.
     * @returns Whether the operation was successful.
     * @see CNetAddr::IsTor()
     */
    bool SetTor(const std::string& addr);

    /**
     * Parse an I2P address and set this object to it.
     * @param[in] addr Address to parse, must be a valid C string, for example
     * ukeu3k5oycgaauneqgtnvselmt4yemvoilkln7jpvamvfx7dnkdq.b32.i2p.
     * @returns Whether the operation was successful.
     * @see CNetAddr::IsI2P()
     */
    bool SetI2P(const std::string& addr);

    /**
     * Size of CNetAddr when serialized as ADDRv1 (pre-BIP155) (in bytes).
     */
    static constexpr size_t V1_SERIALIZATION_SIZE = ADDR_IPV6_SIZE;

    /**
     * Maximum size of an address as defined in BIP155 (in bytes).
     * This is only the size of the address, not the entire CNetAddr object
     * when serialized.
     */
    static constexpr size_t MAX_ADDRV2_SIZE = 512;

    /**
     * Get the BIP155 network id of this address.
     * Must not be called for IsInternal() objects.
     * @returns BIP155 network id, except TORV2 which is no longer supported.
     */
    BIP155Network GetBIP155Network() const;

    /**
     * Set `m_net` from the provided BIP155 network id and size after validation.
     * @retval true the network was recognized, is valid and `m_net` was set
     * @retval false not recognised (from future?) and should be silently ignored
     * @throws std::ios_base::failure if the network is one of the BIP155 founding
     * networks (id 1..6) with wrong address size.
     */
    bool SetNetFromBIP155Network(uint8_t possible_bip155_net, size_t address_size);

    /**
     * Serialize in pre-ADDRv2/BIP155 format to an array.
     */
    void SerializeV1Array(uint8_t (&arr)[V1_SERIALIZATION_SIZE]) const
    {
        size_t prefix_size;

        switch (m_net) {
        case NET_IPV6:
            assert(m_addr.size() == sizeof(arr));
            memcpy(arr, m_addr.data(), m_addr.size());
            return;
        case NET_IPV4:
            prefix_size = sizeof(IPV4_IN_IPV6_PREFIX);
            assert(prefix_size + m_addr.size() == sizeof(arr));
            memcpy(arr, IPV4_IN_IPV6_PREFIX.data(), prefix_size);
            memcpy(arr + prefix_size, m_addr.data(), m_addr.size());
            return;
        case NET_INTERNAL:
            prefix_size = sizeof(INTERNAL_IN_IPV6_PREFIX);
            assert(prefix_size + m_addr.size() == sizeof(arr));
            memcpy(arr, INTERNAL_IN_IPV6_PREFIX.data(), prefix_size);
            memcpy(arr + prefix_size, m_addr.data(), m_addr.size());
            return;
        case NET_ONION:
        case NET_I2P:
        case NET_CJDNS:
            break;
        case NET_UNROUTABLE:
        case NET_MAX:
            assert(false);
        } // no default case, so the compiler can warn about missing cases

        // Serialize ONION, I2P and CJDNS as all-zeros.
        memset(arr, 0x0, V1_SERIALIZATION_SIZE);
    }

    /**
     * Serialize in pre-ADDRv2/BIP155 format to a stream.
     */
    template <typename Stream>
    void SerializeV1Stream(Stream& s) const
    {
        uint8_t serialized[V1_SERIALIZATION_SIZE];

        SerializeV1Array(serialized);

        s << serialized;
    }

    /**
     * Serialize as ADDRv2 / BIP155.
     */
    template <typename Stream>
    void SerializeV2Stream(Stream& s) const
    {
        if (IsInternal()) {
            // Serialize NET_INTERNAL as embedded in IPv6. We need to
            // serialize such addresses from addrman.
            s << static_cast<uint8_t>(BIP155Network::IPV6);
            s << COMPACTSIZE(ADDR_IPV6_SIZE);
            SerializeV1Stream(s);
            return;
        }

        s << static_cast<uint8_t>(GetBIP155Network());
        s << m_addr;
    }

    /**
     * Unserialize from a pre-ADDRv2/BIP155 format from an array.
     *
     * This function is only called from UnserializeV1Stream() and is a wrapper
     * for SetLegacyIPv6(); however, we keep it for symmetry with
     * SerializeV1Array() to have pairs of ser/unser functions and to make clear
     * that if one is altered, a corresponding reverse modification should be
     * applied to the other.
     */
    void UnserializeV1Array(uint8_t (&arr)[V1_SERIALIZATION_SIZE])
    {
        // Use SetLegacyIPv6() so that m_net is set correctly. For example
        // ::FFFF:0102:0304 should be set as m_net=NET_IPV4 (1.2.3.4).
        SetLegacyIPv6(arr);
    }

    /**
     * Unserialize from a pre-ADDRv2/BIP155 format from a stream.
     */
    template <typename Stream>
    void UnserializeV1Stream(Stream& s)
    {
        uint8_t serialized[V1_SERIALIZATION_SIZE];

        s >> serialized;

        UnserializeV1Array(serialized);
    }

    /**
     * Unserialize from a ADDRv2 / BIP155 format.
     */
    template <typename Stream>
    void UnserializeV2Stream(Stream& s)
    {
        uint8_t bip155_net;
        s >> bip155_net;

        size_t address_size;
        s >> COMPACTSIZE(address_size);

        if (address_size > MAX_ADDRV2_SIZE) {
            throw std::ios_base::failure(strprintf(
                "Address too long: %u > %u", address_size, MAX_ADDRV2_SIZE));
        }

        m_scope_id = 0;

        if (SetNetFromBIP155Network(bip155_net, address_size)) {
            m_addr.resize(address_size);
            s >> Span{m_addr};

            if (m_net != NET_IPV6) {
                return;
            }

            // Do some special checks on IPv6 addresses.

            // Recognize NET_INTERNAL embedded in IPv6, such addresses are not
            // gossiped but could be coming from addrman, when unserializing from
            // disk.
            if (util::HasPrefix(m_addr, INTERNAL_IN_IPV6_PREFIX)) {
                m_net = NET_INTERNAL;
                memmove(m_addr.data(), m_addr.data() + INTERNAL_IN_IPV6_PREFIX.size(),
                        ADDR_INTERNAL_SIZE);
                m_addr.resize(ADDR_INTERNAL_SIZE);
                return;
            }

            if (!util::HasPrefix(m_addr, IPV4_IN_IPV6_PREFIX) &&
                !util::HasPrefix(m_addr, TORV2_IN_IPV6_PREFIX)) {
                return;
            }

            // IPv4 and TORv2 are not supposed to be embedded in IPv6 (like in V1
            // encoding). Unserialize as !IsValid(), thus ignoring them.
        } else {
            // If we receive an unknown BIP155 network id (from the future?) then
            // ignore the address - unserialize as !IsValid().
            s.ignore(address_size);
        }

        // Mimic a default-constructed CNetAddr object which is !IsValid() and thus
        // will not be gossiped, but continue reading next addresses from the stream.
        m_net = NET_IPV6;
        m_addr.assign(ADDR_IPV6_SIZE, 0x0);
    }
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
    /**
     * Construct an invalid subnet (empty, `Match()` always returns false).
     */
    CSubNet();

    /**
     * Construct from a given network start and number of bits (CIDR mask).
     * @param[in] addr Network start. Must be IPv4 or IPv6, otherwise an invalid subnet is
     * created.
     * @param[in] mask CIDR mask, must be in [0, 32] for IPv4 addresses and in [0, 128] for
     * IPv6 addresses. Otherwise an invalid subnet is created.
     */
    CSubNet(const CNetAddr& addr, uint8_t mask);

    /**
     * Construct from a given network start and mask.
     * @param[in] addr Network start. Must be IPv4 or IPv6, otherwise an invalid subnet is
     * created.
     * @param[in] mask Network mask, must be of the same type as `addr` and not contain 0-bits
     * followed by 1-bits. Otherwise an invalid subnet is created.
     */
    CSubNet(const CNetAddr& addr, const CNetAddr& mask);

    /**
     * Construct a single-host subnet.
     * @param[in] addr The sole address to be contained in the subnet, can also be non-IPv[46].
     */
    explicit CSubNet(const CNetAddr& addr);

    bool Match(const CNetAddr& addr) const;

    std::string ToString() const;
    bool IsValid() const;

    friend bool operator==(const CSubNet& a, const CSubNet& b);
    friend bool operator!=(const CSubNet& a, const CSubNet& b) { return !(a == b); }
    friend bool operator<(const CSubNet& a, const CSubNet& b);
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
    bool GetSockAddr(struct sockaddr* paddr, socklen_t* addrlen) const;
    bool SetSockAddr(const struct sockaddr* paddr);
    /**
     * Get the address family
     * @returns AF_UNSPEC if unspecified
     */
    [[nodiscard]] sa_family_t GetSAFamily() const;
    friend bool operator==(const CService& a, const CService& b);
    friend bool operator!=(const CService& a, const CService& b) { return !(a == b); }
    friend bool operator<(const CService& a, const CService& b);
    std::vector<unsigned char> GetKey() const;
    std::string ToStringAddrPort() const;

    CService(const struct in6_addr& ipv6Addr, uint16_t port);
    explicit CService(const struct sockaddr_in6& addr);

    SERIALIZE_METHODS(CService, obj)
    {
        READWRITE(AsBase<CNetAddr>(obj), Using<BigEndianFormatter<2>>(obj.port));
    }

    friend class CServiceHash;
    friend CService MaybeFlipIPv6toCJDNS(const CService& service);
};

class CServiceHash
{
public:
    CServiceHash()
        : m_salt_k0{FastRandomContext().rand64()},
          m_salt_k1{FastRandomContext().rand64()}
    {
    }

    CServiceHash(uint64_t salt_k0, uint64_t salt_k1) : m_salt_k0{salt_k0}, m_salt_k1{salt_k1} {}

    size_t operator()(const CService& a) const noexcept
    {
        CSipHasher hasher(m_salt_k0, m_salt_k1);
        hasher.Write(a.m_net);
        hasher.Write(a.port);
        hasher.Write(a.m_addr);
        return static_cast<size_t>(hasher.Finalize());
    }

private:
    const uint64_t m_salt_k0;
    const uint64_t m_salt_k1;
};

#endif // BITCOIN_NETADDRESS_H
