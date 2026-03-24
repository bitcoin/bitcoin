// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <local_addresses.h>

#include <logging.h>
#include <net.h>

std::unique_ptr<LocalAddressManager> g_localaddressman{std::make_unique<LocalAddressManager>()};

[[nodiscard]] std::optional<CService> LocalAddressManager::Get(const CAddress& addr, const Network& connected_through) const
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
            const int nReachability{local_addr.GetReachabilityFrom(addr)};
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
        LogInfo("AddLocal(%s,%i)\n", addr.ToStringAddrPort(), nScore);
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

void LocalAddressManager::Remove(const CService& addr)
{
    LOCK(m_mutex);
    if (fLogIPs) {
        LogInfo("RemoveLocal(%s)\n", addr.ToStringAddrPort());
    }
    m_addresses.erase(addr);
}

bool LocalAddressManager::Seen(const CService& addr)
{
    LOCK(m_mutex);
    const auto it = m_addresses.find(addr);
    if (it == m_addresses.end()) return false;
    ++it->second.nScore;
    return true;
}

bool LocalAddressManager::Contains(const CService& addr) const
{
    LOCK(m_mutex);
    return m_addresses.contains(addr);
}

int LocalAddressManager::GetnScore(const CService& addr) const
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
