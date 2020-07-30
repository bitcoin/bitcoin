// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_PACKAGE_H
#define BITCOIN_PACKAGE_H

#include <uint256.h>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index/ordered_index.hpp>

#include <chrono>
#include <map>
#include <set>
#include <unordered_map>
#include <vector>

#include <stdint.h>

/** Data structure to keep cache and schedule package announcement from/to peers.
 *
 * Maintain a cache of package_id -> set of wtxids. As we cache data at the request
 * of untrusted peers we have to bound them (MAX_PEER_PACKAGE) to avoid risk of memory
 * DoS. We also expire package entry after PACKAGE_CACHE_DELAY to flush the cache and
 * avoid any late-stucking entry, which means package round-trip should have been achieved
 * with ever downstream peers before.
 *
 * XXX: round-robin announcements to avoid upstream peers overflowing our
 * downstream package bandwidth ?
 */
class PackageCache {

    //! Tag for the PackageTime-based index.
    struct ByTime {};
    //! Tag for the PackageId-based index.
    struct ById {};

    //! The ByTime index is sorted by (recvtime, id).
    using PackageTime = std::tuple<std::chrono::microseconds, const uint256&>;

    //! The ById index is sorted by id.
    using PackageId = const uint256;

    //! A package entry
    struct Package {
        //! Package id that was received.
        const uint256 m_package_id;
        //! List of wtxids included in this package.
        const std::vector<uint256> package_wtxids;
        //! What upstream peer sends this package.
        const uint64_t m_peer;
        //! When this package should be pruned for expiration.
        mutable std::chrono::microseconds m_time;
        //! List of downstream peers already announced to.
        mutable std::set<uint64_t> m_already_announced;

        //! Construct a new entry from scratch.
        Package(const uint256& package_id, std::vector<uint256> wtxids, uint64_t peer, std::chrono::microseconds exptime) :
            m_package_id(package_id), package_wtxids(wtxids), m_peer(peer), m_time(exptime) {}

        //! Extract the PackageTime from this Package.
        PackageTime ExtractTime() const { return PackageTime{m_time, m_package_id}; }

        //! Extract the PackageId from this Package.
        PackageId ExtractId() const { return PackageId{m_package_id}; }
    };

    //! Data type for the main data structure
    using Cache = boost::multi_index_container<
        Package,
        boost::multi_index::indexed_by<
            boost::multi_index::ordered_unique<
                boost::multi_index::tag<ByTime>,
                boost::multi_index::const_mem_fun<Package, PackageTime, &Package::ExtractTime>
            >,
            boost::multi_index::ordered_unique<
                boost::multi_index::tag<ById>,
                boost::multi_index::const_mem_fun<Package, PackageId, &Package::ExtractId>
            >
        >
    >;

    Cache m_cache;

    //! Per-peer statistics object.
    struct PeerInfo {
        size_t m_total = 0; //!< Total number of entries for this peer
    };

    std::unordered_map<uint64_t, PeerInfo> m_peerinfo;

    //! Wrapper around Index::...::erase that keeps m_peerinfo and m_associated_pkgs up to date.
    template<typename Tag>
    typename Cache::index<Tag>::type::iterator Erase(typename Cache::index<Tag>::type::iterator it)
    {
        auto peerit = m_peerinfo.find(it->m_peer);
        if (--peerit->second.m_total == 0) m_peerinfo.erase(peerit);
        for (const uint256& wtxid : it->package_wtxids) {
            auto pkgit = m_associated_pkgs.find(wtxid);
            pkgit->second.erase(it->m_package_id);
        }
        return m_cache.get<Tag>().erase(it);
    }

    //! Count how many packages are being cached for a peer.
    size_t CountPackage(uint64_t downstream_peer) const;

    //! Delete entry older than timeout.
    void FlushOldPackages(std::chrono::microseconds timeout);

    //! Per-wtxid associated packages.
    std::map<uint256, std::set<uint256>> m_associated_pkgs;

public:
    //! Construct a PackageCache.
    PackageCache();

    //! We received a new package, enter it.
    void ReceivedPackage(uint64_t upstream_peer, const uint256& package_id, std::vector<uint256> package_wtxids, std::chrono::microseconds recvtime);

    //! Select package ids not already announced to this downstream peer.
    void GetAnnouncable(uint64_t downstream_peer, std::vector<uint256>& package_ids);

    //! Get wtxids included inside a known package id.
    void GetPackageWtxids(uint64_t downstream_peer, const uint256& package_id, std::vector<uint256>& package_wtxids);

    //! Check if we know a package associated to this txid.
    bool HasPackage(uint256& wtxid);

    //! Add package known by this peer.
    void AddPackageKnown(uint64_t upstream_peer, uint256 package_id);
};

#endif // BITCOIN_PACKAGE_H
