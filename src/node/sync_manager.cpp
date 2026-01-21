// Copyright (c) 2024-2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <node/sync_manager.h>

#include <chainparams.h>
#include <evo/deterministicmns.h>
#include <governance/governance.h>
#include <logging.h>
#include <masternode/sync.h>
#include <net.h>
#include <netfulfilledman.h>
#include <netmessagemaker.h>
#include <node/interface_ui.h>
#include <random.h>
#include <scheduler.h>
#include <shutdown.h>

class CConnman;

void SyncManager::Schedule(CScheduler& scheduler)
{
    scheduler.scheduleEvery(
        [this]() -> void {
            if (ShutdownRequested()) return;
            ProcessTick();
        },
        std::chrono::seconds{1});
}

void SyncManager::SendGovernanceSyncRequest(CNode* pnode) const
{
    CNetMsgMaker msgMaker(pnode->GetCommonVersion());
    CBloomFilter filter;
    m_connman.PushMessage(pnode, msgMaker.Make(NetMsgType::MNGOVERNANCESYNC, uint256(), filter));
}

void SyncManager::SendGovernanceObjectSyncRequest(CNode* pnode, const uint256& nHash, bool fUseFilter) const
{
    if (!pnode) return;

    LogPrint(BCLog::GOBJECT, "SyncManager::%s -- nHash %s peer=%d\n", __func__, nHash.ToString(), pnode->GetId());

    CBloomFilter filter = fUseFilter ? m_gov_manager.GetVoteBloomFilter(nHash) : CBloomFilter{};

    CNetMsgMaker msgMaker(pnode->GetCommonVersion());
    m_connman.PushMessage(pnode, msgMaker.Make(NetMsgType::MNGOVERNANCESYNC, nHash, filter));
}

int SyncManager::RequestGovernanceObjectVotes(const std::vector<CNode*>& vNodesCopy) const
{
    // Maximum number of nodes to request votes from for the same object hash on real networks
    // (mainnet, testnet, devnets). Keep this low to avoid unnecessary bandwidth usage.
    static constexpr size_t REALNET_PEERS_PER_HASH{3};
    // Maximum number of nodes to request votes from for the same object hash on regtest.
    // During testing, nodes are isolated to create conflicting triggers. Using the real
    // networks limit of 3 nodes often results in querying only "non-isolated" nodes, missing the
    // isolated ones we need to test. This high limit ensures all available nodes are queried.
    static constexpr size_t REGTEST_PEERS_PER_HASH{std::numeric_limits<size_t>::max()};

    if (vNodesCopy.empty()) return -1;

    int64_t nNow = GetTime();
    int nTimeout = 60 * 60;
    size_t nPeersPerHashMax = Params().IsMockableChain() ? REGTEST_PEERS_PER_HASH : REALNET_PEERS_PER_HASH;


    // This should help us to get some idea about an impact this can bring once deployed on mainnet.
    // Testnet is ~40 times smaller in masternode count, but only ~1000 masternodes usually vote,
    // so 1 obj on mainnet == ~10 objs or ~1000 votes on testnet. However we want to test a higher
    // number of votes to make sure it's robust enough, so aim at 2000 votes per masternode per request.
    // On mainnet nMaxObjRequestsPerNode is always set to 1.
    int nMaxObjRequestsPerNode = 1;
    size_t nProjectedVotes = 2000;
    if (Params().NetworkIDString() != CBaseChainParams::MAIN) {
        nMaxObjRequestsPerNode =
            std::max(1, int(nProjectedVotes /
                            std::max(1, (int)m_gov_manager.GetMNManager().GetListAtChainTip().GetValidMNsCount())));
    }

    static Mutex cs_recently;
    static std::map<uint256, std::map<CService, int64_t>> mapAskedRecently GUARDED_BY(cs_recently);
    LOCK(cs_recently);

    auto [vTriggerObjHashes, vOtherObjHashes] = m_gov_manager.FetchGovernanceObjectVotes(nMaxObjRequestsPerNode, nNow,
                                                                                         mapAskedRecently);

    if (vTriggerObjHashes.empty() && vOtherObjHashes.empty()) return -2;

    LogPrint(BCLog::GOBJECT, "%s -- start: vTriggerObjHashes %d vOtherObjHashes %d mapAskedRecently %d\n", __func__,
             vTriggerObjHashes.size(), vOtherObjHashes.size(), mapAskedRecently.size());

    Shuffle(vTriggerObjHashes.begin(), vTriggerObjHashes.end(), FastRandomContext());
    Shuffle(vOtherObjHashes.begin(), vOtherObjHashes.end(), FastRandomContext());

    for (int i = 0; i < nMaxObjRequestsPerNode; ++i) {
        uint256 nHashGovobj;

        // ask for triggers first
        if (!vTriggerObjHashes.empty()) {
            nHashGovobj = vTriggerObjHashes.back();
        } else {
            if (vOtherObjHashes.empty()) break;
            nHashGovobj = vOtherObjHashes.back();
        }
        bool fAsked = false;
        for (const auto& pnode : vNodesCopy) {
            // Don't try to sync any data from outbound non-relay "masternode" connections.
            // Inbound connection this early is most likely a "masternode" connection
            // initiated from another node, so skip it too.
            if (!pnode->CanRelay() || (m_connman.IsActiveMasternode() && pnode->IsInboundConn())) continue;
            // stop early to prevent setAskFor overflow
            {
                LOCK(::cs_main);
                size_t nProjectedSize = m_peer_manager->PeerGetRequestedObjectCount(pnode->GetId()) + nProjectedVotes;
                if (nProjectedSize > MAX_INV_SZ) continue;
            }
            // to early to ask the same node
            if (mapAskedRecently[nHashGovobj].count(pnode->addr)) continue;

            SendGovernanceObjectSyncRequest(pnode, nHashGovobj, true);
            mapAskedRecently[nHashGovobj][pnode->addr] = nNow + nTimeout;
            fAsked = true;
            // stop loop if max number of peers per obj was asked
            if (mapAskedRecently[nHashGovobj].size() >= nPeersPerHashMax) break;
        }
        // NOTE: this should match `if` above (the one before `while`)
        if (!vTriggerObjHashes.empty()) {
            vTriggerObjHashes.pop_back();
        } else {
            vOtherObjHashes.pop_back();
        }
        if (!fAsked) i--;
    }
    LogPrint(BCLog::GOBJECT, "%s -- end: vTriggerObjHashes %d vOtherObjHashes %d mapAskedRecently %d\n", __func__,
             vTriggerObjHashes.size(), vOtherObjHashes.size(), mapAskedRecently.size());

    return int(vTriggerObjHashes.size() + vOtherObjHashes.size());
}

void SyncManager::ProcessTick()
{
    assert(m_netfulfilledman.IsValid());

    static int nTick = 0;
    nTick++;

    const static int64_t nSyncStart = TicksSinceEpoch<std::chrono::milliseconds>(SystemClock::now());
    const static std::string strAllow = strprintf("allow-sync-%lld", nSyncStart);

    // reset the sync process if the last call to this function was more than 60 minutes ago (client was in sleep mode)
    static int64_t nTimeLastProcess = GetTime();
    if (!Params().IsMockableChain() && GetTime() - nTimeLastProcess > 60 * 60 && !m_connman.IsActiveMasternode()) {
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
    const CConnman::NodesSnapshot snap{m_connman, /* cond = */ CConnman::FullyConnectedOnly};

    // gradually request the rest of the votes after sync finished
    if (m_node_sync.IsSynced()) {
        RequestGovernanceObjectVotes(snap.Nodes());
        return;
    }

    // Calculate "progress" for LOG reporting / GUI notification
    int attempt = m_node_sync.GetAttempt();
    int asset_id = m_node_sync.GetAssetID();
    double nSyncProgress = double(attempt + (asset_id - 1) * 8) / (8 * 4);
    LogPrint(BCLog::MNSYNC, "Sync Tick -- nTick %d asset_id %d nTriedPeerCount %d nSyncProgress %f\n", nTick, asset_id,
             attempt, nSyncProgress);
    uiInterface.NotifyAdditionalDataSyncProgressChanged(nSyncProgress);

    // TODO: move switch-case out from this loop; logic & nodes code to be separated
    for (auto& pnode : snap.Nodes()) {
        CNetMsgMaker msgMaker(pnode->GetCommonVersion());

        // Don't try to sync any data from outbound non-relay "masternode" connections.
        // Inbound connection this early is most likely a "masternode" connection
        // initiated from another node, so skip it too.
        if (!pnode->CanRelay() || (m_connman.IsActiveMasternode() && pnode->IsInboundConn())) continue;

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
                m_connman.PushMessage(pnode, msgMaker.Make(NetMsgType::GETSPORKS));
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
                                m_connman.PushMessage(pNodeTmp, msgMaker.Make(NetMsgType::MEMPOOL));
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

                SendGovernanceSyncRequest(pnode);

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
        const std::vector<CNode*> vNodeCopy{pnode};
        int nObjsLeftToAsk = RequestGovernanceObjectVotes(vNodeCopy);
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

void SyncManager::ProcessMessage(CNode& peer, const std::string& msg_type, CDataStream& vRecv)
{
    //Sync status count
    if (msg_type != NetMsgType::SYNCSTATUSCOUNT) return;

    //do not care about stats if sync process finished
    if (m_node_sync.IsSynced()) return;

    int nItemID;
    int nCount;
    vRecv >> nItemID >> nCount;

    LogPrint(BCLog::MNSYNC, "SYNCSTATUSCOUNT -- got inventory count: nItemID=%d  nCount=%d  peer=%d\n", nItemID, nCount, peer.GetId());
}

void NodeSyncNotifierImpl::SyncReset()
{
    uiInterface.NotifyAdditionalDataSyncProgressChanged(-1);
}

void NodeSyncNotifierImpl::SyncFinished()
{
    assert(m_netfulfilledman.IsValid());

    uiInterface.NotifyAdditionalDataSyncProgressChanged(1);
    m_connman.ForEachNode(CConnman::AllNodes, [this](const CNode* pnode) {
        m_netfulfilledman.AddFulfilledRequest(pnode->addr, "full-sync");
    });
}
