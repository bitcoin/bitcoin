// Copyright (c) 2022
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <node/txpackagetracker.h>

#include <common/bloom.h>
#include <logging.h>
#include <policy/policy.h>
#include <txorphanage.h>
#include <txrequest.h>
#include <util/hasher.h>

namespace node {
    /** How long to wait before requesting orphan ancpkginfo/parents from an additional peer.
     * Same as GETDATA_TX_INTERVAL. */
    static constexpr auto ORPHAN_ANCESTOR_GETDATA_INTERVAL{60s};

    /** Delay to add if an orphan resolution candidate is protecting a lot of memory in the
     * orphanage. */
    static constexpr auto ORPHANAGE_OVERLOAD_DELAY{1s};

    /** Maximum amount of orphans we may protect for an inbound peer when they first connect.
     * The number of "protection tokens" an inbound peer starts with. */
    static constexpr size_t MAX_ORPHANAGE_PROTECTION_INBOUND{50000};
    /** Maximum amount of orphans we may protect for an outbound peer when they first connect.
     * The number of "protection tokens" an inbound peer starts with. Equal to one maximum-size orphan. */
    static constexpr size_t MAX_ORPHANAGE_PROTECTION_OUTBOUND{400000};

    /** Number of orphanage bytes a peer may use before we start dropping their requests. */
    static constexpr size_t MAX_ORPHANAGE_USAGE{10*MAX_ORPHANAGE_PROTECTION_OUTBOUND};


class TxPackageTracker::Impl {
    /** Manages unvalidated tx data (orphan transactions for which we are downloading ancestors). */
    TxOrphanage m_orphanage;

    mutable Mutex m_mutex;
    struct RegistrationState {
        // All of the following bools will need to be true
        /** Whether this peer allows transaction relay from us. */
        bool m_txrelay{true};
        // Whether this peer sent a BIP339 wtxidrelay message.
        bool m_wtxid_relay{false};
        /** Whether this peer says they can do package relay. */
        bool m_sendpackages_received{false};
        /** Versions of package relay supported by this node.
         * This is a subset of PACKAGE_RELAY_SUPPORTED_VERSIONS. */
        PackageRelayVersions m_versions_in_common;
        bool CanRelayPackages() {
            return m_txrelay && m_wtxid_relay && m_sendpackages_received && m_versions_in_common != PKG_RELAY_NONE;
        }
    };
    /** Represents AncPkgInfo for which we are missing transaction data. */
    struct PackageToDownload {
        /** Who provided the ancpkginfo - this is the peer whose work queue to add this package when
         * all tx data is received. We expect to receive tx data from this peer. */
        const NodeId m_pkginfo_provider;

        /** When to stop trying to download this package if we haven't received tx data yet. */
        std::chrono::microseconds m_expiry;

        /** Representative wtxid, i.e. the orphan in an ancestor package. */
        const uint256 m_rep_wtxid;

        /** Map from wtxid to status (true indicates it is missing). This can be expanded to further
         * states such as "already in mempool/confirmed" in the future. */
        std::vector<std::pair<uint256, bool>> m_txdata_status;

        // Package info without wtxids doesn't make sense.
        PackageToDownload() = delete;
        // Constructor if you already know size.
        PackageToDownload(NodeId nodeid,
                          std::chrono::microseconds expiry,
                          const uint256& rep_wtxid,
                          const std::vector<std::pair<uint256, bool>>& txdata_status) :
            m_pkginfo_provider{nodeid},
            m_expiry{expiry},
            m_rep_wtxid{rep_wtxid},
            m_txdata_status{txdata_status}
        {}
        bool HasTransactionIn(const std::set<uint256>& wtxidset) const {
            for (const auto& keyval : m_txdata_status) {
                if (wtxidset.count(keyval.first) > 0) return true;
            }
            return false;
        }
        /** Returns wtxid of representative transaction (i.e. the orphan in an ancestor package). */
        uint256 RepresentativeWtxid() const { return m_rep_wtxid; }
        /** Combined hash of all wtxids in package. */
        uint256 GetPackageHash() const {
            std::vector<uint256> all_wtxids;
            std::transform(m_txdata_status.cbegin(), m_txdata_status.cend(), std::back_inserter(all_wtxids),
                [](const auto& mappair) { return mappair.first; });
            return GetCombinedHash(all_wtxids);
        }
    };

    using PackageInfoRequestId = uint256;
    PackageInfoRequestId GetPackageInfoRequestId(NodeId nodeid, const uint256& wtxid, uint32_t version) {
        return (CHashWriter(SER_GETHASH, 0) << nodeid << wtxid << version).GetHash();
    }
    using PackageTxnsRequestId = uint256;
    PackageTxnsRequestId GetPackageTxnsRequestId(NodeId nodeid, const std::vector<uint256>& wtxids) {
        return (CHashWriter(SER_GETHASH, 0) << nodeid << GetCombinedHash(wtxids)).GetHash();
    }
    PackageTxnsRequestId GetPackageTxnsRequestId(NodeId nodeid, const std::vector<CTransactionRef>& pkgtxns) {
        return (CHashWriter(SER_GETHASH, 0) << nodeid << GetPackageHash(pkgtxns)).GetHash();
    }
    PackageTxnsRequestId GetPackageTxnsRequestId(NodeId nodeid, const uint256& combinedhash) {
        return (CHashWriter(SER_GETHASH, 0) << nodeid << combinedhash).GetHash();
    }
    /** List of all ancestor package info we're currently requesting txdata for, indexed by the
     * nodeid and getpkgtxns request we would have sent them. */
    std::map<PackageTxnsRequestId, PackageToDownload> pending_package_info GUARDED_BY(m_mutex);

    using PendingMap = decltype(pending_package_info);
    struct IteratorComparator {
        template<typename I>
        bool operator()(const I& a, const I& b) const { return &(*a) < &(*b); }
    };

    struct PeerInfo {
        /** Whether this is an inbound peer. Affects how much of the orphanage we may protect for
         * this peer. */
        bool m_is_inbound;
        // What package versions we agreed to relay.
        PackageRelayVersions m_versions_supported;
        /** Amount of orphans protected for this peer, in bytes. */
        size_t m_protected_orphans_size{0};
        /** Maximum amount of orphans that can be protected for this peer, in bytes. */
        size_t m_max_orphans_to_protect{0};
        /** Wtxids and sizes of the orphans in the orphanage who are protected for this peer. */
        std::map<uint256, size_t> m_protected_orphan_map;

        /** Iterators to items in pending_package_info originating from this peer. */
        std::set<PendingMap::iterator, IteratorComparator> m_package_info_provided;

        PeerInfo(bool is_inbound, PackageRelayVersions versions) :
            m_is_inbound{is_inbound},
            m_versions_supported{versions},
            m_max_orphans_to_protect{is_inbound ? MAX_ORPHANAGE_PROTECTION_INBOUND : MAX_ORPHANAGE_PROTECTION_OUTBOUND}
        {}
        bool SupportsVersion(PackageRelayVersions version) const { return m_versions_supported & version; }
        size_t AvailableProtectionTokens() {
            if (m_max_orphans_to_protect < m_protected_orphans_size) {
                Assume(false);
                return 0;
            }
            return m_max_orphans_to_protect - m_protected_orphans_size;
        }
    };

    /** Stores relevant information about the peer prior to verack. Upon completion of version
     * handshake, we use this information to decide whether we relay packages with this peer. */
    std::map<NodeId, RegistrationState> registration_states GUARDED_BY(m_mutex);

    /** Information for each peer we relay packages with. Membership in this map is equivalent to
     * whether or not we relay packages with a peer. */
    std::map<NodeId, PeerInfo> info_per_peer GUARDED_BY(m_mutex);

    /** Tracks orphans for which we need to request ancestor information. All hashes stored are
     * wtxids, i.e., the wtxid of the orphan. However, the is_wtxid field is used to indicate
     * whether we would request the ancestor information by wtxid (via package relay) or by txid
     * (via prevouts of the missing inputs). */
    TxRequestTracker orphan_request_tracker GUARDED_BY(m_mutex);

    /** Cache of package info requests sent. Used to identify unsolicited package info messages. */
    CRollingBloomFilter packageinfo_requested GUARDED_BY(m_mutex){50000, 0.000001};

public:
    Impl() = default;

    // Orphanage Wrapper Functions
    bool OrphanageHaveTx(const GenTxid& gtxid) { return m_orphanage.HaveTx(gtxid); }
    CTransactionRef GetTxToReconsider(NodeId peer) { return m_orphanage.GetTxToReconsider(peer); }
    int EraseOrphanTx(const uint256& txid) { return m_orphanage.EraseTx(txid); }
    void DisconnectedPeer(NodeId nodeid) EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
    {
        AssertLockNotHeld(m_mutex);
        LOCK(m_mutex);
        if (auto it{registration_states.find(nodeid)}; it != registration_states.end()) {
            registration_states.erase(it);
        }
        if (auto it{info_per_peer.find(nodeid)}; it != info_per_peer.end()) {
            std::set<uint256> pending_to_erase;
            for (const auto pkginfo_iter : it->second.m_package_info_provided) {
                if (pkginfo_iter != pending_package_info.end()) {
                    pending_to_erase.insert(GetPackageInfoRequestId(nodeid, pkginfo_iter->second.RepresentativeWtxid(), PKG_RELAY_ANCPKG));
                } else {
                    Assume(false);
                }
            }
            it->second.m_package_info_provided.clear();
            for (const auto& packageid : pending_to_erase) pending_package_info.erase(packageid);
            info_per_peer.erase(it);
        }
        orphan_request_tracker.DisconnectedPeer(nodeid);
        m_orphanage.EraseForPeer(nodeid);
    }
    void BlockConnected(const CBlock& block) EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
    {
        AssertLockNotHeld(m_mutex);
        const auto wtxids_erased{m_orphanage.EraseForBlock(block)};
        std::set<uint256> block_wtxids;
        std::set<uint256> conflicted_wtxids;
        for (const CTransactionRef& ptx : block.vtx) {
            block_wtxids.insert(ptx->GetWitnessHash());
        }
        for (const auto& wtxid : wtxids_erased) {
            if (block_wtxids.count(wtxid) == 0) {
                conflicted_wtxids.insert(wtxid);
            }
        }
        FinalizeTransactions(block_wtxids, conflicted_wtxids);
    }
    void LimitOrphans(unsigned int max_orphans) { m_orphanage.LimitOrphans(max_orphans); }
    void AddChildrenToWorkSet(const CTransaction& tx) { m_orphanage.AddChildrenToWorkSet(tx); }
    bool HaveTxToReconsider(NodeId peer) { return m_orphanage.HaveTxToReconsider(peer); }
    size_t OrphanageSize() { return m_orphanage.Size(); }
    PackageRelayVersions GetSupportedVersions() const
    {
        return PackageRelayVersions(PKG_RELAY_PKGTXNS | PKG_RELAY_ANCPKG);
    }

    void ReceivedVersion(NodeId nodeid) EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
    {
        AssertLockNotHeld(m_mutex);
        LOCK(m_mutex);
        if (registration_states.find(nodeid) != registration_states.end()) return;
        registration_states.insert(std::make_pair(nodeid, RegistrationState{}));
    }
    void ReceivedSendpackages(NodeId nodeid, PackageRelayVersions version) EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
    {
        AssertLockNotHeld(m_mutex);
        LOCK(m_mutex);
        const auto it = registration_states.find(nodeid);
        if (it == registration_states.end()) return;
        it->second.m_sendpackages_received = true;
        // Ignore versions we don't understand. Relay packages of versions that we both support.
        it->second.m_versions_in_common = PackageRelayVersions(GetSupportedVersions() & version);
    }

    bool ReceivedVerack(NodeId nodeid, bool inbound, bool txrelay, bool wtxidrelay) EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
    {
        AssertLockNotHeld(m_mutex);
        LOCK(m_mutex);
        const auto& it = registration_states.find(nodeid);
        if (it == registration_states.end()) return false;
        it->second.m_txrelay = txrelay;
        it->second.m_wtxid_relay = wtxidrelay;
        const bool final_state = it->second.CanRelayPackages();
        if (final_state) {
            // Should support at least one version of package relay.
            Assume(it->second.m_versions_in_common != PKG_RELAY_NONE);
            info_per_peer.insert(std::make_pair(nodeid, PeerInfo{inbound, it->second.m_versions_in_common}));
        }
        registration_states.erase(it);
        return final_state;
    }

    bool PeerSupportsVersion(NodeId nodeid, PackageRelayVersions versions) const EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
    {
        LOCK(m_mutex);
        auto it_peer_info = info_per_peer.find(nodeid);
        // Peer does not support package relay at all.
        if (it_peer_info == info_per_peer.end()) return false;
        return it_peer_info->second.SupportsVersion(versions);
    }

    void AddOrphanTx(NodeId nodeid, const uint256& wtxid, const CTransactionRef& tx, bool is_preferred, std::chrono::microseconds reqtime)
        EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
    {
        AssertLockNotHeld(m_mutex);
        LOCK(m_mutex);
        // Skip if we weren't provided the tx and can't find the wtxid in the orphanage.
        if (tx == nullptr && !m_orphanage.HaveTx(GenTxid::Wtxid(wtxid))) return;
        CTransactionRef ptx = tx ? tx : m_orphanage.GetTx(wtxid);

        // Skip if already requested in the (recent-ish) past.
        if (packageinfo_requested.contains(GetPackageInfoRequestId(nodeid, wtxid, PKG_RELAY_ANCPKG))) return;

        // Skip if this peer is already using a lot of space in the orphanage.
        if (m_orphanage.BytesFromPeer(nodeid) > MAX_ORPHANAGE_USAGE) return;

        auto it_peer_info = info_per_peer.find(nodeid);
        if (it_peer_info != info_per_peer.end() && it_peer_info->second.SupportsVersion(PKG_RELAY_ANCPKG)) {
            // If we cannot protect this transaction, delay the request to use other peers'
            // protection tokens.
            Assume(it_peer_info->second.m_protected_orphan_map.count(wtxid) == 0);
            if (it_peer_info->second.AvailableProtectionTokens() < ptx->GetTotalSize()) {
                reqtime += ORPHANAGE_OVERLOAD_DELAY;
            }
            // Package relay peer: is_wtxid=true because we will be requesting via ancpkginfo.
            LogPrint(BCLog::TXPACKAGES, "\nPotential orphan resolution for %s from package relay peer=%d\n", wtxid.ToString(), nodeid);
            orphan_request_tracker.ReceivedInv(nodeid, GenTxid::Wtxid(wtxid), is_preferred, reqtime);
        } else {
            // Even though this stores the orphan wtxid, is_wtxid=false because we will be requesting the parents via txid.
            LogPrint(BCLog::TXPACKAGES, "\nPotential orphan resolution for %s from legacy peer=%d\n", wtxid.ToString(), nodeid);
            orphan_request_tracker.ReceivedInv(nodeid, GenTxid::Txid(wtxid), is_preferred, reqtime);
        }

        m_orphanage.AddTx(ptx, nodeid);
        // Protection may or may not be possible. If the orphan is unprotected, it's possible it
        // will be evicted by the time we try to download ancestors. If we cannot protect now but
        // still have it later, we will attempt to protect it again.
        MaybeProtectOrphan(nodeid, wtxid);
    }
    void MaybeProtectOrphan(NodeId nodeid, const uint256& wtxid) EXCLUSIVE_LOCKS_REQUIRED(m_mutex)
    {
        AssertLockHeld(m_mutex);
        auto peer_info_it = info_per_peer.find(nodeid);
        // Skip if this isn't a package relay peer.
        if (peer_info_it == info_per_peer.end()) return;
        const auto orphan_tx = m_orphanage.GetTx(wtxid);
        // Skip if the orphanage doesn't have this transaction.
        if (!orphan_tx) return;
        const auto orphan_size{orphan_tx->GetTotalSize()};
        // Skip if this peer is already protecting this orphan.
        if (peer_info_it->second.m_protected_orphan_map.count(wtxid) > 0) return;
        // Skip if this peer is already protecting too many orphans.
        if (peer_info_it->second.AvailableProtectionTokens() < orphan_size) return;

        // Protect this orphan.
        LogPrint(BCLog::TXPACKAGES, "\nProtecting orphan %s for peer=%d\n", wtxid.ToString(), nodeid);
        m_orphanage.ProtectOrphan(wtxid);
        peer_info_it->second.m_protected_orphans_size += orphan_tx->GetTotalSize();
        peer_info_it->second.m_protected_orphan_map.emplace(wtxid, orphan_size);
    }
    void MaybeUnprotectOrphan(NodeId nodeid, const uint256& wtxid) EXCLUSIVE_LOCKS_REQUIRED(m_mutex)
    {
        AssertLockHeld(m_mutex);
        auto peer_info_it = info_per_peer.find(nodeid);
        if (peer_info_it == info_per_peer.end()) return;
        auto orphan_map_it = peer_info_it->second.m_protected_orphan_map.find(wtxid);
        // Skip if this peer isn't protecting this orphan.
        if (orphan_map_it == peer_info_it->second.m_protected_orphan_map.end()) return;
        const auto orphan_size = orphan_map_it->second;

        // Unprotect this orphan.
        LogPrint(BCLog::TXPACKAGES, "\nUndoing protection for orphan %s for peer=%d\n", wtxid.ToString(), nodeid);
        m_orphanage.UndoProtectOrphan(wtxid);
        Assume(peer_info_it->second.m_protected_orphans_size >= orphan_size);
        peer_info_it->second.m_protected_orphans_size -= orphan_size;
        peer_info_it->second.m_protected_orphan_map.erase(wtxid);
    }
    size_t CountInFlight(NodeId nodeid) const EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
    {
        AssertLockNotHeld(m_mutex);
        LOCK(m_mutex);
        auto count{orphan_request_tracker.CountInFlight(nodeid)};
        if (auto it{info_per_peer.find(nodeid)}; it != info_per_peer.end()) {
            count += it->second.m_package_info_provided.size();
        }
        return count;
    }
    size_t Count(NodeId nodeid) const EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
    {
        AssertLockNotHeld(m_mutex);
        LOCK(m_mutex);
        auto count{orphan_request_tracker.Count(nodeid)};
        if (auto it{info_per_peer.find(nodeid)}; it != info_per_peer.end()) {
            count += it->second.m_package_info_provided.size();
        }
        return count;
    }

    void ExpirePackageToDownload(NodeId nodeid, std::chrono::microseconds current_time)
        EXCLUSIVE_LOCKS_REQUIRED(m_mutex)
    {
        AssertLockHeld(m_mutex);
        auto peer_info_it = info_per_peer.find(nodeid);
        if (peer_info_it == info_per_peer.end()) return;
        std::set<PackageTxnsRequestId> to_expire;
        for (const auto& pkginfo_iter : peer_info_it->second.m_package_info_provided) {
            const auto& packageinfo = pkginfo_iter->second;
            if (packageinfo.m_expiry < current_time) {
                LogPrint(BCLog::TXPACKAGES, "\nExpiring package info for tx %s from peer=%d\n",
                         packageinfo.RepresentativeWtxid().ToString(), nodeid);
                to_expire.insert(pkginfo_iter->first);
            }
        }
        for (const auto& packageid : to_expire) {
            auto pending_iter = pending_package_info.find(packageid);
            Assume(pending_iter != pending_package_info.end());
            if (pending_iter != pending_package_info.end()) {
                for (const auto& [wtxid, missing] : pending_iter->second.m_txdata_status) {
                    if (!missing) MaybeUnprotectOrphan(nodeid, wtxid);
                }
                peer_info_it->second.m_package_info_provided.erase(pending_iter);
                pending_package_info.erase(pending_iter);
            }
        }
    }
    std::vector<GenTxid> GetOrphanRequests(NodeId nodeid, std::chrono::microseconds current_time)
        EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
    {
        AssertLockNotHeld(m_mutex);
        LOCK(m_mutex);
        // Expire packages we were trying to download tx data for
        ExpirePackageToDownload(nodeid, current_time);
        std::vector<std::pair<NodeId, GenTxid>> expired;
        auto tracker_requestable = orphan_request_tracker.GetRequestable(nodeid, current_time, &expired);
        for (const auto& entry : expired) {
            LogPrint(BCLog::TXPACKAGES, "\nTimeout of inflight %s %s from peer=%d\n", entry.second.IsWtxid() ? "ancpkginfo" : "orphan parent",
                entry.second.GetHash().ToString(), entry.first);
        }
        // Get getdata requests we should send
        std::vector<GenTxid> results;
        for (const auto& gtxid : tracker_requestable) {
            if (gtxid.IsWtxid()) {
                Assume(info_per_peer.find(nodeid) != info_per_peer.end());
                // Add the orphan's wtxid as-is.
                LogPrint(BCLog::TXPACKAGES, "\nResolving orphan %s, requesting by ancpkginfo from peer=%d\n", gtxid.GetHash().ToString(), nodeid);
                results.emplace_back(gtxid);
                packageinfo_requested.insert(GetPackageInfoRequestId(nodeid, gtxid.GetHash(), PKG_RELAY_ANCPKG));
                orphan_request_tracker.RequestedTx(nodeid, gtxid.GetHash(), current_time + ORPHAN_ANCESTOR_GETDATA_INTERVAL);
                MaybeProtectOrphan(nodeid, gtxid.GetHash());
            } else {
                LogPrint(BCLog::TXPACKAGES, "\nResolving orphan %s, requesting by txids of parents from peer=%d\n", gtxid.GetHash().ToString(), nodeid);
                const auto ptx = m_orphanage.GetTx(gtxid.GetHash());
                if (!ptx) {
                    // We can't request ancpkginfo and we have no way of knowing what the missing
                    // parents are (it could also be that the orphan has already been resolved).
                    // Give up.
                    orphan_request_tracker.ForgetTxHash(gtxid.GetHash());
                    LogPrint(BCLog::TXPACKAGES, "\nForgetting orphan %s from peer=%d\n", gtxid.GetHash().ToString(), nodeid);
                    continue;
                }
                // Add the orphan's parents. Net processing will filter out what we already have.
                // Deduplicate parent txids, so that we don't have to loop over
                // the same parent txid more than once down below.
                std::vector<uint256> unique_parents;
                unique_parents.reserve(ptx->vin.size());
                for (const auto& txin : ptx->vin) {
                    // We start with all parents, and then remove duplicates below.
                    unique_parents.push_back(txin.prevout.hash);
                }
                std::sort(unique_parents.begin(), unique_parents.end());
                unique_parents.erase(std::unique(unique_parents.begin(), unique_parents.end()), unique_parents.end());
                for (const auto& txid : unique_parents) {
                    results.emplace_back(GenTxid::Txid(txid));
                }
                // Mark the orphan as requested. Limitation: we aren't tracking these txids in
                // relation to the orphan's wtxid anywhere. If we get a NOTFOUND for the parent(s),
                // we won't automatically know that it corresponds to this orphan (i.e. won't be
                // able to call ReceivedResponse()). We will need to wait until it expires before
                // requesting from somebody else.
                orphan_request_tracker.RequestedTx(nodeid, gtxid.GetHash(), current_time + ORPHAN_ANCESTOR_GETDATA_INTERVAL);
            }
        }
        if (!results.empty()) LogPrint(BCLog::TXPACKAGES, "\nRequesting %u items from peer=%d\n", results.size(), nodeid);
        return results;
    }
    void FinalizeTransactions(const std::set<uint256>& valid, const std::set<uint256>& invalid) EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
    {
        AssertLockNotHeld(m_mutex);
        LOCK(m_mutex);
        // Do a linear search of all packages. This operation should not be expensive as we don't
        // expect to be relaying more than 1 package per peer. Nonetheless, process sets together
        // to be more efficient.
        std::set<PackageTxnsRequestId> to_erase;
        for (const auto& [packageid, packageinfo] : pending_package_info) {
            const auto& rep_wtxid = packageinfo.RepresentativeWtxid();
            if (valid.count(rep_wtxid) > 0 || invalid.count(rep_wtxid) > 0) {
                // We have already made a final decision on the transaction of interest.
                // There is no need to request more information from other peers.
                to_erase.insert(packageid);
                orphan_request_tracker.ForgetTxHash(rep_wtxid);
                MaybeUnprotectOrphan(packageinfo.m_pkginfo_provider, rep_wtxid);
            } else if (packageinfo.HasTransactionIn(invalid)) {
                // This package info is known to contain an invalid transaction; don't continue
                // trying to download or validate it.
                to_erase.insert(packageid);
                // However, as it's possible for this information to be incorrect (e.g. a peer
                // purposefully trying to get us to reject the orphan by providing package info
                // containing an invalid transaction), don't prevent further orphan resolution
                // attempts with other peers.
            } else {
                // FIXME: Some packages may need less txdata now.
                // It's fine not to do this *for now* since we always request all missing txdata
                // from the same peer.
            }
        }
        for (const auto& packageid : to_erase) {
            auto pending_iter = pending_package_info.find(packageid);
            Assume(pending_iter != pending_package_info.end());
            if (pending_iter != pending_package_info.end()) {
                auto peer_info_it = info_per_peer.find(pending_iter->second.m_pkginfo_provider);
                Assume(peer_info_it != info_per_peer.end());
                if (peer_info_it != info_per_peer.end()) {
                    for (const auto& [wtxid, missing] : pending_iter->second.m_txdata_status) {
                        if (!missing) MaybeUnprotectOrphan(pending_iter->second.m_pkginfo_provider, wtxid);
                    }
                    peer_info_it->second.m_package_info_provided.erase(pending_iter);
                }
                pending_package_info.erase(pending_iter);
            }
        }
    }
    bool PkgInfoAllowed(NodeId nodeid, const uint256& wtxid, PackageRelayVersions version) EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
    {
        AssertLockNotHeld(m_mutex);
        LOCK(m_mutex);
        auto peerinfo_it = info_per_peer.find(nodeid);
        if (peerinfo_it == info_per_peer.end()) {
            return false;
        } else if (!peerinfo_it->second.SupportsVersion(version)) {
            return false;
        }
        auto peer_info = info_per_peer.find(nodeid)->second;
        const auto packageid{GetPackageInfoRequestId(nodeid, wtxid, version)};
        if (!packageinfo_requested.contains(packageid)) {
            return false;
        }
        // They already responded to this request.
        for (const auto& pkginfo_iter : peer_info.m_package_info_provided) {
            if (wtxid == pkginfo_iter->second.m_rep_wtxid) return false;
        }
        return true;
    }
    void ForgetPkgInfo(NodeId nodeid, const uint256& rep_wtxid, PackageRelayVersions pkginfo_version) EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
    {
        AssertLockNotHeld(m_mutex);
        LOCK(m_mutex);
        if (pkginfo_version == PKG_RELAY_ANCPKG) {
            orphan_request_tracker.ReceivedResponse(nodeid, rep_wtxid);
            MaybeUnprotectOrphan(nodeid, rep_wtxid);
        }
    }

    std::variant<PackageToValidate, std::vector<uint256>> ReceivedAncPkgInfo(NodeId nodeid,
            const uint256& rep_wtxid, const std::vector<std::pair<uint256, bool>>& txdata_status, std::chrono::microseconds expiry)
        EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
    {
        AssertLockNotHeld(m_mutex);
        LOCK(m_mutex);
        std::vector<uint256> wtxids_to_request;
        auto peer_info_it = info_per_peer.find(nodeid);
        if (peer_info_it == info_per_peer.end()) return wtxids_to_request;
        // We haven't fully resolved this orphan yet - we still need to download the txdata for each
        // ancestor - so don't call ForgetTxHash(), as it is not guaranteed we will get all the
        // information from this peer. Also don't call ReceivedResponse(), as doing so would trigger
        // the orphan_request_tracker to select other candidate peers for orphan resolution. Stay
        // in the REQUESTED, not COMPLETED, state.
        //
        // Instead, reset the timeout (another ORPHAN_ANCESTOR_GETDATA_INTERVAL) to give this peer
        // more time to respond to our second round of requests. After that timeout, the
        // orphan_request_tracker will select additional candidate peers for orphan resolution.
        orphan_request_tracker.ResetRequestTimeout(nodeid, rep_wtxid, ORPHAN_ANCESTOR_GETDATA_INTERVAL);
        for (const auto& [wtxid, missing] : txdata_status) {
            if (missing && m_orphanage.HaveTx(GenTxid::Wtxid(wtxid))) {
                // Try to protect all transactions in our orphanage if we haven't already. Do this
                // before we decide whether or not we want to request this transaction. We don't
                // need to redownload orphans we are protecting.
                MaybeProtectOrphan(nodeid, wtxid);
            }
            // All missing transactions and unprotected orphans must be requested again. We cannot
            // guarantee that an unprotected orphan will remain in our orphanage, and it won't make
            // sense to try to download something a third time.
            if ((missing && !m_orphanage.HaveTx(GenTxid::Wtxid(wtxid))) ||
                 peer_info_it->second.m_protected_orphan_map.count(wtxid) == 0) {
                wtxids_to_request.push_back(wtxid);
            }
        }
        if (wtxids_to_request.empty()) {
            std::vector<CTransactionRef> unvalidated_txns;
            for (const auto& [wtxid, _] : txdata_status) {
                if (const auto ptx{m_orphanage.GetTx(wtxid)}) unvalidated_txns.push_back(ptx);
            }
            const auto pkgtxnsid{GetPackageTxnsRequestId(nodeid, unvalidated_txns)};
            // All transactions must be AlreadyHave (according to peerman) or in our orphanage.
            // Return a PackageToValidate immediately.
            return PackageToValidate{nodeid, rep_wtxid, pkgtxnsid, unvalidated_txns};
        } else {
            // Transactions are missing. Return the list to request and remember this request by the
            // combined hash of the wtxids.
            const auto pkgtxnsid{GetPackageTxnsRequestId(nodeid, wtxids_to_request)};
            const auto [it, success] = pending_package_info.emplace(pkgtxnsid, PackageToDownload{nodeid, expiry, rep_wtxid, txdata_status});
            if (success) peer_info_it->second.m_package_info_provided.emplace(it);
            return wtxids_to_request;
        }
    }
    void ReceivedNotFound(NodeId nodeid, const uint256& hash) EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
    {
        AssertLockNotHeld(m_mutex);
        LOCK(m_mutex);
        auto peer_info_it = info_per_peer.find(nodeid);
        if (peer_info_it == info_per_peer.end()) return;
        const auto pending_iter{pending_package_info.find(GetPackageTxnsRequestId(nodeid, hash))};
        if (pending_iter != pending_package_info.end()) {
            auto& pendingpackage{pending_iter->second};
            LogPrint(BCLog::TXPACKAGES, "\nReceived notfound for package (tx %s) from peer=%d\n", pendingpackage.RepresentativeWtxid().ToString(), nodeid);
            // Give up on downloading this package
            for (const auto& [wtxid, missing] : pendingpackage.m_txdata_status) {
                if (!missing) MaybeUnprotectOrphan(nodeid, wtxid);
            }
            peer_info_it->second.m_package_info_provided.erase(pending_iter);
            pending_package_info.erase(pending_iter);
        }
    }
    std::optional<PackageToValidate> ReceivedPkgTxns(NodeId nodeid, const std::vector<CTransactionRef>& package_txns)
        EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
    {
        AssertLockNotHeld(m_mutex);
        LOCK(m_mutex);
        auto peer_info_it = info_per_peer.find(nodeid);
        if (peer_info_it == info_per_peer.end()) return std::nullopt;
        const auto pending_iter{pending_package_info.find(GetPackageTxnsRequestId(nodeid, package_txns))};
        if (pending_iter == pending_package_info.end()) {
            // For whatever reason, we've been sent a pkgtxns that doesn't correspond to a pending
            // package. It's possible we already admitted all the transactions, or this response
            // arrived past the request expiry. Drop it on the ground.
            return std::nullopt;
        }
        std::vector<CTransactionRef> unvalidated_txdata(package_txns.cbegin(), package_txns.cend());
        auto& pendingpackage{pending_iter->second};
        LogPrint(BCLog::TXPACKAGES, "\nReceived tx data for package (tx %s) from peer=%d\n", pendingpackage.RepresentativeWtxid().ToString(), nodeid);
        // Add the other orphanage transactions before updating pending packages map.
        for (const auto& [wtxid, _] : pendingpackage.m_txdata_status) {
            if (m_orphanage.HaveTx(GenTxid::Wtxid(wtxid))) {
                unvalidated_txdata.push_back(m_orphanage.GetTx(wtxid));
            }
            MaybeUnprotectOrphan(nodeid, wtxid);
        }
        return PackageToValidate{pendingpackage.m_pkginfo_provider, pendingpackage.RepresentativeWtxid(),
                                 pendingpackage.GetPackageHash(), unvalidated_txdata};
    }
};

TxPackageTracker::TxPackageTracker() : m_impl{std::make_unique<TxPackageTracker::Impl>()} {}
TxPackageTracker::~TxPackageTracker() = default;

bool TxPackageTracker::OrphanageHaveTx(const GenTxid& gtxid) { return m_impl->OrphanageHaveTx(gtxid); }
CTransactionRef TxPackageTracker::GetTxToReconsider(NodeId peer) { return m_impl->GetTxToReconsider(peer); }
int TxPackageTracker::EraseOrphanTx(const uint256& txid) { return m_impl->EraseOrphanTx(txid); }
void TxPackageTracker::DisconnectedPeer(NodeId peer) { m_impl->DisconnectedPeer(peer); }
void TxPackageTracker::BlockConnected(const CBlock& block) { m_impl->BlockConnected(block); }
void TxPackageTracker::LimitOrphans(unsigned int max_orphans) { m_impl->LimitOrphans(max_orphans); }
void TxPackageTracker::AddChildrenToWorkSet(const CTransaction& tx) { m_impl->AddChildrenToWorkSet(tx); }
bool TxPackageTracker::HaveTxToReconsider(NodeId peer) { return m_impl->HaveTxToReconsider(peer); }
size_t TxPackageTracker::OrphanageSize() { return m_impl->OrphanageSize(); }
PackageRelayVersions TxPackageTracker::GetSupportedVersions() const { return m_impl->GetSupportedVersions(); }
void TxPackageTracker::ReceivedVersion(NodeId nodeid) { m_impl->ReceivedVersion(nodeid); }
void TxPackageTracker::ReceivedSendpackages(NodeId nodeid, PackageRelayVersions version) { m_impl->ReceivedSendpackages(nodeid, version); }
bool TxPackageTracker::ReceivedVerack(NodeId nodeid, bool inbound, bool txrelay, bool wtxidrelay) {
    return m_impl->ReceivedVerack(nodeid, inbound, txrelay, wtxidrelay);
}
bool TxPackageTracker::PeerSupportsVersion(NodeId nodeid, PackageRelayVersions versions) const { return m_impl->PeerSupportsVersion(nodeid, versions); }
void TxPackageTracker::AddOrphanTx(NodeId nodeid, const uint256& wtxid, const CTransactionRef& tx, bool is_preferred, std::chrono::microseconds reqtime)
{
    m_impl->AddOrphanTx(nodeid, wtxid, tx, is_preferred, reqtime);
}
size_t TxPackageTracker::CountInFlight(NodeId nodeid) const { return m_impl->CountInFlight(nodeid); }
size_t TxPackageTracker::Count(NodeId nodeid) const { return m_impl->Count(nodeid); }
std::vector<GenTxid> TxPackageTracker::GetOrphanRequests(NodeId nodeid, std::chrono::microseconds current_time) {
    return m_impl->GetOrphanRequests(nodeid, current_time);
}
void TxPackageTracker::FinalizeTransactions(const std::set<uint256>& valid, const std::set<uint256>& invalid)
{
    m_impl->FinalizeTransactions(valid, invalid);
}
bool TxPackageTracker::PkgInfoAllowed(NodeId nodeid, const uint256& wtxid, PackageRelayVersions version)
{
    return m_impl->PkgInfoAllowed(nodeid, wtxid, version);
}
void TxPackageTracker::ForgetPkgInfo(NodeId nodeid, const uint256& rep_wtxid, PackageRelayVersions pkginfo_version)
{
    m_impl->ForgetPkgInfo(nodeid, rep_wtxid, pkginfo_version);
}
std::variant<TxPackageTracker::PackageToValidate, std::vector<uint256>> TxPackageTracker::ReceivedAncPkgInfo(NodeId nodeid,
        const uint256& rep_wtxid, const std::vector<std::pair<uint256, bool>>& txdata_status, std::chrono::microseconds expiry)
{
    return m_impl->ReceivedAncPkgInfo(nodeid, rep_wtxid, txdata_status, expiry);
}
void TxPackageTracker::ReceivedNotFound(NodeId nodeid, const uint256& hash) { m_impl->ReceivedNotFound(nodeid, hash); }
std::optional<TxPackageTracker::PackageToValidate> TxPackageTracker::ReceivedPkgTxns(NodeId nodeid,
    const std::vector<CTransactionRef>& package_txns)
{
    return m_impl->ReceivedPkgTxns(nodeid, package_txns);
}
} // namespace node
