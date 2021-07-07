// Copyright (c) 2009-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <netaddress.h>
#include <netbase.h>

#include <string>
#include <type_traits>
#include <vector>

#ifndef BITCOIN_NET_PERMISSIONS_H
#define BITCOIN_NET_PERMISSIONS_H

struct bilingual_str;

extern const std::vector<std::string> NET_PERMISSIONS_DOC;

/** Default for -whitelistrelay. */
constexpr bool DEFAULT_WHITELISTRELAY = true;
/** Default for -whitelistforcerelay. */
constexpr bool DEFAULT_WHITELISTFORCERELAY = false;

enum class NetPermissionFlags : uint32_t {
    None = 0,
    // Can query bloomfilter even if -peerbloomfilters is false
    BloomFilter = (1U << 1),
    // Relay and accept transactions from this peer, even if -blocksonly is true
    // This peer is also not subject to limits on how many transaction INVs are tracked
    Relay = (1U << 3),
    // Always relay transactions from this peer, even if already in mempool
    // Keep parameter interaction: forcerelay implies relay
    ForceRelay = (1U << 2) | Relay,
    // Allow getheaders during IBD and block-download after maxuploadtarget limit
    Download = (1U << 6),
    // Can't be banned/disconnected/discouraged for misbehavior
    NoBan = (1U << 4) | Download,
    // Can query the mempool
    Mempool = (1U << 5),
    // Can request addrs without hitting a privacy-preserving cache, and send us
    // unlimited amounts of addrs.
    Addr = (1U << 7),

    // True if the user did not specifically set fine-grained permissions with
    // the -whitebind or -whitelist configuration options.
    Implicit = (1U << 31),
    All = BloomFilter | ForceRelay | Relay | NoBan | Mempool | Download | Addr,
};
static inline constexpr NetPermissionFlags operator|(NetPermissionFlags a, NetPermissionFlags b)
{
    using t = typename std::underlying_type<NetPermissionFlags>::type;
    return static_cast<NetPermissionFlags>(static_cast<t>(a) | static_cast<t>(b));
}

class NetPermissions
{
public:
    NetPermissionFlags m_flags;
    static std::vector<std::string> ToStrings(NetPermissionFlags flags);
    static inline bool HasFlag(NetPermissionFlags flags, NetPermissionFlags f)
    {
        using t = typename std::underlying_type<NetPermissionFlags>::type;
        return (static_cast<t>(flags) & static_cast<t>(f)) == static_cast<t>(f);
    }
    static inline void AddFlag(NetPermissionFlags& flags, NetPermissionFlags f)
    {
        flags = flags | f;
    }
    //! ClearFlag is only called with `f` == NetPermissionFlags::Implicit.
    //! If that should change in the future, be aware that ClearFlag should not
    //! be called with a subflag of a multiflag, e.g. NetPermissionFlags::Relay
    //! or NetPermissionFlags::Download, as that would leave `flags` in an
    //! invalid state corresponding to none of the existing flags.
    static inline void ClearFlag(NetPermissionFlags& flags, NetPermissionFlags f)
    {
        assert(f == NetPermissionFlags::Implicit);
        using t = typename std::underlying_type<NetPermissionFlags>::type;
        flags = static_cast<NetPermissionFlags>(static_cast<t>(flags) & ~static_cast<t>(f));
    }
};

class NetWhitebindPermissions : public NetPermissions
{
public:
    static bool TryParse(const std::string& str, NetWhitebindPermissions& output, bilingual_str& error);
    CService m_service;
};

class NetWhitelistPermissions : public NetPermissions
{
public:
    static bool TryParse(const std::string& str, NetWhitelistPermissions& output, ConnectionDirection& output_connection_direction, bilingual_str& error);
    CSubNet m_subnet;
};

#endif // BITCOIN_NET_PERMISSIONS_H
