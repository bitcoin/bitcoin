// Copyright (c) 2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <governance/net_governance.h>

#include <chainparams.h>
#include <governance/governance.h>
#include <masternode/sync.h>
#include <net.h>
#include <scheduler.h>

class CConnman;

void NetGovernance::Schedule(CScheduler& scheduler, CConnman& connman)
{
    if (!m_gov_manager.IsValid()) return;

    scheduler.scheduleEvery(
        [this, &connman]() -> void {
            if (!m_node_sync.IsSynced()) return;

            // CHECK OBJECTS WE'VE ASKED FOR, REMOVE OLD ENTRIES
            m_gov_manager.CleanOrphanObjects();
            m_gov_manager.RequestOrphanObjects(connman);

            // CHECK AND REMOVE - REPROCESS GOVERNANCE OBJECTS
            m_gov_manager.CheckAndRemove();
        },
        std::chrono::minutes{5});

    scheduler.scheduleEvery(
        [this]() -> void {
            auto relay_invs = m_gov_manager.FetchRelayInventory();
            for (const auto& inv : relay_invs) {
                m_peer_manager->PeerRelayInv(inv);
            }
        },
        // Tests need tighter timings to avoid timeouts, use more relaxed pacing otherwise
        Params().IsMockableChain() ? std::chrono::seconds{1} : std::chrono::seconds{5});
}
