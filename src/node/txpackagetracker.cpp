// Copyright (c) 2022
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <node/txpackagetracker.h>

#include <common/bloom.h>
#include <logging.h>
#include <txorphanage.h>
#include <txrequest.h>
#include <util/hasher.h>

namespace node {
    /** How long to wait before requesting orphan ancpkginfo/parents from an additional peer.
     * Same as GETDATA_TX_INTERVAL. */
    static constexpr auto ORPHAN_ANCESTOR_GETDATA_INTERVAL{60s};

    /** Delay to add if an orphan resolution candidate is already using a lot of memory in the
     * orphanage. */
    static constexpr auto ORPHANAGE_OVERLOAD_DELAY{1s};

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
    using PackageInfoRequestId = uint256;
    PackageInfoRequestId GetPackageInfoRequestId(NodeId nodeid, const uint256& wtxid, uint32_t version) {
        return (CHashWriter(SER_GETHASH, 0) << nodeid << wtxid << version).GetHash();
    }

    struct PeerInfo {
        /** Whether this is an inbound peer. Affects how much of the orphanage we may protect for
         * this peer. */
        bool m_is_inbound;
        // What package versions we agreed to relay.
        PackageRelayVersions m_versions_supported;

        PeerInfo(bool is_inbound, PackageRelayVersions versions) :
            m_is_inbound{is_inbound},
            m_versions_supported{versions}
        {}
        bool SupportsVersion(PackageRelayVersions version) { return m_versions_supported & version; }
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
        return PKG_RELAY_ANCPKG;
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

    void AddOrphanTx(NodeId nodeid, const uint256& wtxid, const CTransactionRef& tx, bool is_preferred, std::chrono::microseconds reqtime)
        EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
    {
        AssertLockNotHeld(m_mutex);
        LOCK(m_mutex);
        // Skip if we weren't provided the tx and can't find the wtxid in the orphanage.
        if (tx == nullptr && !m_orphanage.HaveTx(GenTxid::Wtxid(wtxid))) return;

        // Skip if already requested in the (recent-ish) past.
        if (packageinfo_requested.contains(GetPackageInfoRequestId(nodeid, wtxid, PKG_RELAY_ANCPKG))) return;

        auto it_peer_info = info_per_peer.find(nodeid);
        if (it_peer_info != info_per_peer.end() && it_peer_info->second.SupportsVersion(PKG_RELAY_ANCPKG)) {
            // Package relay peer: is_wtxid=true because we will be requesting via ancpkginfo.
            LogPrint(BCLog::TXPACKAGES, "\nPotential orphan resolution for %s from package relay peer=%d\n", wtxid.ToString(), nodeid);
            orphan_request_tracker.ReceivedInv(nodeid, GenTxid::Wtxid(wtxid), is_preferred, reqtime);
        } else {
            // Even though this stores the orphan wtxid, is_wtxid=false because we will be requesting the parents via txid.
            LogPrint(BCLog::TXPACKAGES, "\nPotential orphan resolution for %s from legacy peer=%d\n", wtxid.ToString(), nodeid);
            orphan_request_tracker.ReceivedInv(nodeid, GenTxid::Txid(wtxid), is_preferred, reqtime);
        }

        if (tx != nullptr) {
            m_orphanage.AddTx(tx, nodeid);
        } else {
            m_orphanage.AddTx(m_orphanage.GetTx(wtxid), nodeid);
        }

    }
    size_t CountInFlight(NodeId nodeid) const EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
    {
        AssertLockNotHeld(m_mutex);
        LOCK(m_mutex);
        auto count{orphan_request_tracker.CountInFlight(nodeid)};
        return count;
    }
    size_t Count(NodeId nodeid) const EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
    {
        AssertLockNotHeld(m_mutex);
        LOCK(m_mutex);
        auto count{orphan_request_tracker.Count(nodeid)};
        return count;
    }

    std::vector<GenTxid> GetOrphanRequests(NodeId nodeid, std::chrono::microseconds current_time)
        EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
    {
        AssertLockNotHeld(m_mutex);
        LOCK(m_mutex);
        std::vector<std::pair<NodeId, GenTxid>> expired;
        auto tracker_requestable = orphan_request_tracker.GetRequestable(nodeid, current_time, &expired);
        for (const auto& entry : expired) {
            LogPrint(BCLog::TXPACKAGES, "\nTimeout of inflight %s %s from peer=%d\n", entry.second.IsWtxid() ? "ancpkginfo" : "orphan parent",
                entry.second.GetHash().ToString(), entry.first);
        }
        std::vector<GenTxid> results;
        for (const auto& gtxid : tracker_requestable) {
            if (gtxid.IsWtxid()) {
                Assume(info_per_peer.find(nodeid) != info_per_peer.end());
                // Add the orphan's wtxid as-is.
                LogPrint(BCLog::TXPACKAGES, "\nResolving orphan %s, requesting by ancpkginfo from peer=%d\n", gtxid.GetHash().ToString(), nodeid);
                results.emplace_back(gtxid);
                packageinfo_requested.insert(GetPackageInfoRequestId(nodeid, gtxid.GetHash(), PKG_RELAY_ANCPKG));
                orphan_request_tracker.RequestedTx(nodeid, gtxid.GetHash(), current_time + ORPHAN_ANCESTOR_GETDATA_INTERVAL);
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
        for (const auto& wtxid : valid) {
            orphan_request_tracker.ForgetTxHash(wtxid);
        }
        for (const auto& wtxid : invalid) {
            orphan_request_tracker.ForgetTxHash(wtxid);
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
        if (!packageinfo_requested.contains(GetPackageInfoRequestId(nodeid, wtxid, version))) {
            return false;
        }
        orphan_request_tracker.ReceivedResponse(nodeid, wtxid);
        return true;
    }
    void ForgetPkgInfo(NodeId nodeid, const uint256& rep_wtxid, PackageRelayVersions pkginfo_version) EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
    {
        AssertLockNotHeld(m_mutex);
        LOCK(m_mutex);
        if (pkginfo_version == PKG_RELAY_ANCPKG) {
            orphan_request_tracker.ReceivedResponse(nodeid, rep_wtxid);
        }
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
} // namespace node
