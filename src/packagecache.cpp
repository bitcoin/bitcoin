// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <packagecache.h>

/** Maximum number of package per peer */
static constexpr int32_t MAX_PEER_PACKAGE = 1000;
/** How many microseconds to keep this package entry available for announcing/fetching. */
static constexpr std::chrono::microseconds PACKAGE_CACHE_DELAY{std::chrono::seconds{60}};

PackageCache::PackageCache() :
    m_cache(boost::make_tuple(
        boost::make_tuple(
            boost::multi_index::const_mem_fun<Package, PackageTime, &Package::ExtractTime>(),
            std::less<PackageTime>()
        ),
        boost::make_tuple(
            boost::multi_index::const_mem_fun<Package, PackageId, &Package::ExtractId>(),
            std::less<PackageId>()
        )
    )) {}

size_t PackageCache::CountPackage(uint64_t upstream_peer) const
{
    auto it = m_peerinfo.find(upstream_peer);
    if (it != m_peerinfo.end()) return it->second.m_total;
    return 0;
}

void PackageCache::FlushOldPackages(std::chrono::microseconds timeout)
{
    while (!m_cache.empty()) {
        auto it = m_cache.get<ByTime>().begin();
        if (it->m_time <= timeout) {
            Erase<ByTime>(it);
        } else {
            break;
        }
    }
}

void PackageCache::ReceivedPackage(uint64_t upstream_peer, const uint256& package_id, std:: vector<uint256> package_wtxids, std::chrono::microseconds recvtime)
{
    if (CountPackage(upstream_peer) >= MAX_PEER_PACKAGE) return;

    // If we already know about this package ignore it.
    auto it = m_cache.get<ById>().find(PackageId{package_id});
    if (it != m_cache.get<ById>().end()) return;

    auto ret = m_cache.get<ById>().emplace(package_id, package_wtxids, upstream_peer, recvtime + PACKAGE_CACHE_DELAY);
    if (ret.second) {
        ++m_peerinfo[upstream_peer].m_total;
        for (const uint256& wtxid: package_wtxids) {
            auto it = m_associated_pkgs.find(wtxid);
            if (it != m_associated_pkgs.end()) {
                it->second.emplace(package_id);
            }
        }
    }
}

static const uint256 UINT256_ZERO;

void PackageCache::GetAnnouncable(uint64_t downstream_peer, std::vector<uint256>& package_ids)
{
    auto it = m_cache.get<ById>().lower_bound(PackageId{UINT256_ZERO});
    while (it != m_cache.get<ById>().end()) {
        // If this downstream peer hasn't heard about it, mark it for announcement.
        if (it->m_already_announced.count(downstream_peer) == 0) {
            package_ids.push_back(it->m_package_id);
            it->m_already_announced.emplace(downstream_peer);
        }
        it++;
    }
}

void PackageCache::GetPackageWtxids(uint64_t downstream_peer, const uint256& package_id, std::vector<uint256>& package_wtxids)
{
    auto it = m_cache.get<ById>().find(PackageId{package_id});
    if (it != m_cache.get<ById>().end()) {
        // If downstream peer hasn't be announced package id don't yet walk away
        if (it->m_already_announced.count(downstream_peer) == 0) return;
        package_wtxids = it->package_wtxids;
    }
}

bool PackageCache::HasPackage(uint256& wtxid) {
    return m_associated_pkgs.count(wtxid) == 1;
}

void PackageCache::AddPackageKnown(uint64_t upstream_peer, uint256 package_id) {
    auto it = m_cache.get<ById>().find(PackageId{package_id});
    if (it != m_cache.get<ById>().end()) {
        it->m_already_announced.emplace(upstream_peer);
    }
}
