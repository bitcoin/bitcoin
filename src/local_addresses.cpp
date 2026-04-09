// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <local_addresses.h>

#include <logging.h>
#include <net.h>

std::unique_ptr<LocalAddressManager> g_localaddressman{std::make_unique<LocalAddressManager>()};

// private extensions to enum Network, only returned by GetExtNetwork,
// and only used in GetReachabilityFrom
static const int NET_TEREDO = NET_MAX;
int static GetExtNetwork(const CNetAddr& addr)
{
    if (addr.IsRFC4380())
        return NET_TEREDO;
    return addr.GetNetwork();
}

[[nodiscard]] std::optional<CService> LocalAddressManager::Get(const CNetAddr& addr, const Network& connected_through) const
{
    if (!fListen) return std::nullopt;

    std::optional<CService> ret;
    int nBestScore = -1;
    int nBestReachability = -1;
    {
        LOCK(m_mutex);
        for (const auto& [local_addr, local_service_info] : m_addresses) {
            // For privacy reasons, don't advertise our privacy-network address
            // to other networks and don't advertise our other-network address
            // to privacy networks.
            if (Network local_addr_net = local_addr.GetNetwork();
                local_addr_net != connected_through && (IsPrivacyNetwork(local_addr_net) || IsPrivacyNetwork(connected_through))) {
                continue;
            }
            const int nScore{local_service_info.nScore};
            const int nReachability{GetReachability(local_addr, addr)};
            if (nReachability > nBestReachability || (nReachability == nBestReachability && nScore > nBestScore)) {
                ret.emplace(CService{local_addr, local_service_info.nPort});
                nBestReachability = nReachability;
                nBestScore = nScore;
            }
        }
    }
    return ret;
}

void LocalAddressManager::Clear()
{
    LOCK(m_mutex);
    return m_addresses.clear();
}

bool LocalAddressManager::Add(const CService& addr_, int nScore)
{
    CService addr{MaybeFlipIPv6toCJDNS(addr_)};

    if (!addr.IsRoutable())
        return false;

    if (!fDiscover && nScore < LOCAL_MANUAL)
        return false;

    if (!g_reachable_nets.Contains(addr))
        return false;

    if (fLogIPs) {
        LogInfo("LocalAddressManager: Adding %s (Score: %i)\n", addr.ToStringAddrPort(), nScore);
    }

    {
        LOCK(m_mutex);
        const auto [it, is_newly_added] = m_addresses.emplace(addr, LocalServiceInfo());
        LocalServiceInfo &info = it->second;
        if (is_newly_added || nScore >= info.nScore) {
            info.nScore = nScore + (is_newly_added ? 0 : 1);
            info.nPort = addr.GetPort();
        }
    }

    return true;
}

void LocalAddressManager::Remove(const CNetAddr& addr)
{
    LOCK(m_mutex);
    if (fLogIPs) {
        LogInfo("LocalAddressManager: Removing %s\n", addr.ToStringAddr());
    }
    m_addresses.erase(addr);
}

bool LocalAddressManager::Seen(const CNetAddr& addr)
{
    LOCK(m_mutex);
    const auto it = m_addresses.find(addr);
    if (it == m_addresses.end()) return false;
    ++it->second.nScore;
    return true;
}

bool LocalAddressManager::Contains(const CNetAddr& addr) const
{
    LOCK(m_mutex);
    return m_addresses.contains(addr);
}

int LocalAddressManager::GetnScore(const CNetAddr& addr) const
{
    LOCK(m_mutex);
    const auto it = m_addresses.find(addr);
    return (it != m_addresses.end()) ? it->second.nScore : 0;
}

LocalAddressManager::map_type LocalAddressManager::GetAll() const
{
    LOCK(m_mutex);
    return m_addresses;
}

int LocalAddressManager::GetReachability(const CNetAddr& addr, const CNetAddr& paddrPartner)
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

    if (!addr.IsRoutable() || addr.IsInternal())
        return REACH_UNREACHABLE;

    int ourNet = GetExtNetwork(addr);
    int theirNet = GetExtNetwork(paddrPartner);
    bool fTunnel = addr.IsRFC3964() || addr.IsRFC6052() || addr.IsRFC6145();

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
    case NET_I2P:
        switch (ourNet) {
        case NET_I2P: return REACH_PRIVATE;
        default: return REACH_DEFAULT;
        }
    case NET_CJDNS:
        switch (ourNet) {
        case NET_CJDNS: return REACH_PRIVATE;
        default: return REACH_DEFAULT;
        }
    case NET_TEREDO:
        switch(ourNet) {
        default:          return REACH_DEFAULT;
        case NET_TEREDO:  return REACH_TEREDO;
        case NET_IPV6:    return REACH_IPV6_WEAK;
        case NET_IPV4:    return REACH_IPV4;
        }
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
