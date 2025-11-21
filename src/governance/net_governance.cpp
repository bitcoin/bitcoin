// Copyright (c) 2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <governance/net_governance.h>

#include <chainparams.h>
#include <governance/governance.h>
#include <logging.h>
#include <masternode/sync.h>
#include <net.h>
#include <netfulfilledman.h>
#include <netmessagemaker.h>
#include <node/interface_ui.h>
#include <scheduler.h>
#include <shutdown.h>

class CConnman;

void NetGovernance::Schedule(CScheduler& scheduler, CConnman& connman)
{
    scheduler.scheduleEvery(
        [this, &connman]() -> void {
            if (ShutdownRequested()) return;
            ProcessTick(connman);
        },
        std::chrono::seconds{1});

    // Code below is meant to be running only if governance validation is enabled
    if (!m_gov_manager.IsValid()) return;
    scheduler.scheduleEvery(
        [this, &connman]() -> void {
            if (!m_node_sync.IsSynced()) return;

            // CHECK OBJECTS WE'VE ASKED FOR, REMOVE OLD ENTRIES
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

void NetGovernance::SendGovernanceSyncRequest(CNode* pnode, CConnman& connman) const
{
    CNetMsgMaker msgMaker(pnode->GetCommonVersion());
    CBloomFilter filter;
    connman.PushMessage(pnode, msgMaker.Make(NetMsgType::MNGOVERNANCESYNC, uint256(), filter));
}

void NetGovernance::ProcessTick(CConnman& connman)
{
    assert(m_netfulfilledman.IsValid());

    static int nTick = 0;
    nTick++;

    const static int64_t nSyncStart = TicksSinceEpoch<std::chrono::milliseconds>(SystemClock::now());
    const static std::string strAllow = strprintf("allow-sync-%lld", nSyncStart);

    // reset the sync process if the last call to this function was more than 60 minutes ago (client was in sleep mode)
    static int64_t nTimeLastProcess = GetTime();
    if (!Params().IsMockableChain() && GetTime() - nTimeLastProcess > 60 * 60 && !connman.IsActiveMasternode()) {
        LogPrintf("Sync Tick -- WARNING: no actions for too long, restarting sync...\n");
        m_node_sync.Reset(true);
        nTimeLastProcess = GetTime();
        return;
    }

    if (GetTime() - nTimeLastProcess < MASTERNODE_SYNC_TICK_SECONDS) {
        // too early, nothing to do here
        return;
    }

    nTimeLastProcess = GetTime();
    const CConnman::NodesSnapshot snap{connman, /* cond = */ CConnman::FullyConnectedOnly};

    // gradually request the rest of the votes after sync finished
    if (m_node_sync.IsSynced()) {
        m_gov_manager.RequestGovernanceObjectVotes(snap.Nodes(), connman, m_peer_manager);
        return;
    }

    // Calculate "progress" for LOG reporting / GUI notification
    int attempt = m_node_sync.GetAttempt();
    int asset_id = m_node_sync.GetAssetID();
    double nSyncProgress = double(attempt + (asset_id - 1) * 8) / (8 * 4);
    LogPrint(BCLog::MNSYNC, "Sync Tick -- nTick %d asset_id %d nTriedPeerCount %d nSyncProgress %f\n", nTick, asset_id,
             attempt, nSyncProgress);
    uiInterface.NotifyAdditionalDataSyncProgressChanged(nSyncProgress);

    for (auto& pnode : snap.Nodes()) {
        CNetMsgMaker msgMaker(pnode->GetCommonVersion());

        // Don't try to sync any data from outbound non-relay "masternode" connections.
        // Inbound connection this early is most likely a "masternode" connection
        // initiated from another node, so skip it too.
        if (!pnode->CanRelay() || (connman.IsActiveMasternode() && pnode->IsInboundConn())) continue;

        {
            if ((pnode->HasPermission(NetPermissionFlags::NoBan) || pnode->IsManualConn()) &&
                !m_netfulfilledman.HasFulfilledRequest(pnode->addr, strAllow)) {
                m_netfulfilledman.RemoveAllFulfilledRequests(pnode->addr);
                m_netfulfilledman.AddFulfilledRequest(pnode->addr, strAllow);
                LogPrintf("Sync Tick -- skipping mnsync restrictions for peer=%d\n", pnode->GetId());
            }

            if (m_netfulfilledman.HasFulfilledRequest(pnode->addr, "full-sync")) {
                // We already fully synced from this node recently,
                // disconnect to free this connection slot for another peer.
                pnode->fDisconnect = true;
                LogPrintf("Sync Tick -- disconnecting from recently synced peer=%d\n", pnode->GetId());
                continue;
            }

            // SPORK : ALWAYS ASK FOR SPORKS AS WE SYNC

            if (!m_netfulfilledman.HasFulfilledRequest(pnode->addr, "spork-sync")) {
                // always get sporks first, only request once from each peer
                m_netfulfilledman.AddFulfilledRequest(pnode->addr, "spork-sync");
                // get current network sporks
                connman.PushMessage(pnode, msgMaker.Make(NetMsgType::GETSPORKS));
                LogPrint(BCLog::MNSYNC, "Sync Tick -- nTick %d asset_id %d -- requesting sporks from peer=%d\n", nTick,
                         asset_id, pnode->GetId());
            }

            if (asset_id == MASTERNODE_SYNC_BLOCKCHAIN) {
                int64_t nTimeSyncTimeout = snap.Nodes().size() > 3 ? MASTERNODE_SYNC_TICK_SECONDS
                                                                   : MASTERNODE_SYNC_TIMEOUT_SECONDS;
                if (m_node_sync.IsReachedBestHeader() && (GetTime() - m_node_sync.GetLastBump() > nTimeSyncTimeout)) {
                    // At this point we know that:
                    // a) there are peers (because we are looping on at least one of them);
                    // b) we waited for at least MASTERNODE_SYNC_TICK_SECONDS/MASTERNODE_SYNC_TIMEOUT_SECONDS
                    //    (depending on the number of connected peers) since we reached the headers tip the last
                    //    time (i.e. since fReachedBestHeader has been set to true);
                    // c) there were no blocks (UpdatedBlockTip, NotifyHeaderTip) or headers (AcceptedBlockHeader)
                    //    for at least MASTERNODE_SYNC_TICK_SECONDS/MASTERNODE_SYNC_TIMEOUT_SECONDS (depending on
                    //    the number of connected peers).
                    // We must be at the tip already, let's move to the next asset.
                    m_node_sync.SwitchToNextAsset();
                    uiInterface.NotifyAdditionalDataSyncProgressChanged(nSyncProgress);

                    if (gArgs.GetBoolArg("-syncmempool", DEFAULT_SYNC_MEMPOOL)) {
                        // Now that the blockchain is synced request the mempool from the connected outbound nodes if possible
                        for (auto pNodeTmp : snap.Nodes()) {
                            bool fRequestedEarlier = m_netfulfilledman.HasFulfilledRequest(pNodeTmp->addr,
                                                                                           "mempool-sync");
                            if (!pNodeTmp->IsInboundConn() && !fRequestedEarlier && !pNodeTmp->IsBlockRelayOnly()) {
                                m_netfulfilledman.AddFulfilledRequest(pNodeTmp->addr, "mempool-sync");
                                connman.PushMessage(pNodeTmp, msgMaker.Make(NetMsgType::MEMPOOL));
                                LogPrint(BCLog::MNSYNC, /* Continued */
                                         "Sync Tick -- nTick %d asset_id %d -- syncing mempool from peer=%d\n", nTick,
                                         asset_id, pNodeTmp->GetId());
                            }
                        }
                    }
                }
            }

            // GOVOBJ : SYNC GOVERNANCE ITEMS FROM OUR PEERS

            if (asset_id == MASTERNODE_SYNC_GOVERNANCE) {
                if (!m_gov_manager.IsValid()) {
                    m_node_sync.SwitchToNextAsset();
                    return;
                }
                LogPrint(BCLog::GOBJECT, "Sync Tick -- nTick %d asset_id %d last_bump %lld GetTime() %lld diff %lld\n",
                         nTick, asset_id, m_node_sync.GetLastBump(), GetTime(), GetTime() - m_node_sync.GetLastBump());

                // check for timeout first
                if (GetTime() - m_node_sync.GetLastBump() > MASTERNODE_SYNC_TIMEOUT_SECONDS) {
                    LogPrint(BCLog::MNSYNC, "Sync Tick -- nTick %d asset_id %d -- timeout\n", nTick, asset_id);
                    if (attempt == 0) {
                        LogPrintf("Sync Tick -- WARNING: failed to sync %s\n", m_node_sync.GetAssetName());
                        // it's kind of ok to skip this for now, hopefully we'll catch up later?
                    }
                    m_node_sync.SwitchToNextAsset();
                    return;
                }

                // only request obj sync once from each peer
                if (m_netfulfilledman.HasFulfilledRequest(pnode->addr, "governance-sync")) {
                    // will request votes on per-obj basis from each node in a separate loop below
                    // to avoid deadlocks here
                    continue;
                }
                m_netfulfilledman.AddFulfilledRequest(pnode->addr, "governance-sync");

                m_node_sync.BumpAttempt();

                SendGovernanceSyncRequest(pnode, connman);

                break; // this will cause each peer to get one request each six seconds for the various assets we need
            }
        }
    }


    if (asset_id != MASTERNODE_SYNC_GOVERNANCE) {
        // looped through all nodes and not syncing governance yet/already, release them
        return;
    }

    // request votes on per-obj basis from each node
    for (const auto& pnode : snap.Nodes()) {
        if (!m_netfulfilledman.HasFulfilledRequest(pnode->addr, "governance-sync")) {
            continue; // to early for this node
        }
        int nObjsLeftToAsk = m_gov_manager.RequestGovernanceObjectVotes(*pnode, connman, m_peer_manager);
        // check for data
        if (nObjsLeftToAsk == 0) {
            static int64_t nTimeNoObjectsLeft = 0;
            static int nLastTick = 0;
            static int nLastVotes = 0;
            if (nTimeNoObjectsLeft == 0) {
                // asked all objects for votes for the first time
                nTimeNoObjectsLeft = GetTime();
            }
            // make sure the condition below is checked only once per tick
            if (nLastTick == nTick) continue;
            if (GetTime() - nTimeNoObjectsLeft > MASTERNODE_SYNC_TIMEOUT_SECONDS &&
                m_gov_manager.GetVoteCount() - nLastVotes <
                    std::max(int(0.0001 * nLastVotes), MASTERNODE_SYNC_TICK_SECONDS)) {
                // We already asked for all objects, waited for MASTERNODE_SYNC_TIMEOUT_SECONDS
                // after that and less then 0.01% or MASTERNODE_SYNC_TICK_SECONDS
                // (i.e. 1 per second) votes were received during the last tick.
                // We can be pretty sure that we are done syncing.
                LogPrintf("Sync Tick -- nTick %d asset_id %d -- asked for all objects, nothing to do\n", nTick,
                          MASTERNODE_SYNC_GOVERNANCE);
                // reset nTimeNoObjectsLeft to be able to use the same condition on resync
                nTimeNoObjectsLeft = 0;
                m_node_sync.SwitchToNextAsset();
                return;
            }
            nLastTick = nTick;
            nLastVotes = m_gov_manager.GetVoteCount();
        }
    }
}
