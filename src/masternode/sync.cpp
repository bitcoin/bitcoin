// Copyright (c) 2014-2023 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <masternode/sync.h>

#include <chainparams.h>
#include <governance/governance.h>
#include <netfulfilledman.h>
#include <netmessagemaker.h>
#include <node/ui_interface.h>
#include <shutdown.h>
#include <validation.h>
#include <util/time.h>
#include <util/translation.h>

class CMasternodeSync;
std::unique_ptr<CMasternodeSync> masternodeSync;

CMasternodeSync::CMasternodeSync(CConnman& _connman, const CGovernanceManager& govman) :
    nTimeAssetSyncStarted(GetTime()),
    nTimeLastBumped(GetTime()),
    connman(_connman),
    m_govman(govman)
{
}

void CMasternodeSync::Reset(bool fForce, bool fNotifyReset)
{
    // Avoid resetting the sync process if we just "recently" received a new block
    if (!fForce) {
        if (GetTime() - nTimeLastUpdateBlockTip < MASTERNODE_SYNC_RESET_SECONDS) {
            return;
        }
    }
    nCurrentAsset = MASTERNODE_SYNC_BLOCKCHAIN;
    nTriedPeerCount = 0;
    nTimeAssetSyncStarted = GetTime();
    nTimeLastBumped = GetTime();
    nTimeLastUpdateBlockTip = 0;
    fReachedBestHeader = false;
    if (fNotifyReset) {
        uiInterface.NotifyAdditionalDataSyncProgressChanged(-1);
    }
}

void CMasternodeSync::BumpAssetLastTime(const std::string& strFuncName)
{
    if (IsSynced()) return;
    nTimeLastBumped = GetTime();
    LogPrint(BCLog::MNSYNC, "CMasternodeSync::BumpAssetLastTime -- %s\n", strFuncName);
}

std::string CMasternodeSync::GetAssetName() const
{
    switch(nCurrentAsset)
    {
        case(MASTERNODE_SYNC_BLOCKCHAIN):   return "MASTERNODE_SYNC_BLOCKCHAIN";
        case(MASTERNODE_SYNC_GOVERNANCE):   return "MASTERNODE_SYNC_GOVERNANCE";
        case MASTERNODE_SYNC_FINISHED:      return "MASTERNODE_SYNC_FINISHED";
        default:                            return "UNKNOWN";
    }
}

void CMasternodeSync::SwitchToNextAsset()
{
    switch(nCurrentAsset)
    {
        case(MASTERNODE_SYNC_BLOCKCHAIN):
            LogPrintf("CMasternodeSync::SwitchToNextAsset -- Completed %s in %llds\n", GetAssetName(), GetTime() - nTimeAssetSyncStarted);
            nCurrentAsset = MASTERNODE_SYNC_GOVERNANCE;
            LogPrintf("CMasternodeSync::SwitchToNextAsset -- Starting %s\n", GetAssetName());
            break;
        case(MASTERNODE_SYNC_GOVERNANCE):
            LogPrintf("CMasternodeSync::SwitchToNextAsset -- Completed %s in %llds\n", GetAssetName(), GetTime() - nTimeAssetSyncStarted);
            nCurrentAsset = MASTERNODE_SYNC_FINISHED;
            uiInterface.NotifyAdditionalDataSyncProgressChanged(1);

            connman.ForEachNode(CConnman::AllNodes, [](CNode* pnode) {
                netfulfilledman->AddFulfilledRequest(pnode->addr, "full-sync");
            });
            LogPrintf("CMasternodeSync::SwitchToNextAsset -- Sync has finished\n");

            break;
    }
    nTriedPeerCount = 0;
    nTimeAssetSyncStarted = GetTime();
    BumpAssetLastTime("CMasternodeSync::SwitchToNextAsset");
}

std::string CMasternodeSync::GetSyncStatus() const
{
    switch (nCurrentAsset) {
        case MASTERNODE_SYNC_BLOCKCHAIN:    return _("Synchronizing blockchain...").translated;
        case MASTERNODE_SYNC_GOVERNANCE:    return _("Synchronizing governance objects...").translated;
        case MASTERNODE_SYNC_FINISHED:      return _("Synchronization finished").translated;
        default:                            return "";
    }
}

void CMasternodeSync::ProcessMessage(const CNode& peer, std::string_view msg_type, CDataStream& vRecv) const
{
    //Sync status count
    if (msg_type != NetMsgType::SYNCSTATUSCOUNT) return;

    //do not care about stats if sync process finished
    if (IsSynced()) return;

    int nItemID;
    int nCount;
    vRecv >> nItemID >> nCount;

    LogPrint(BCLog::MNSYNC, "SYNCSTATUSCOUNT -- got inventory count: nItemID=%d  nCount=%d  peer=%d\n", nItemID, nCount, peer.GetId());
}

void CMasternodeSync::ProcessTick()
{
    static int nTick = 0;
    nTick++;

    const static int64_t nSyncStart = GetTimeMillis();
    const static std::string strAllow = strprintf("allow-sync-%lld", nSyncStart);

    // reset the sync process if the last call to this function was more than 60 minutes ago (client was in sleep mode)
    static int64_t nTimeLastProcess = GetTime();
    if(GetTime() - nTimeLastProcess > 60*60 && !fMasternodeMode) {
        LogPrintf("CMasternodeSync::ProcessTick -- WARNING: no actions for too long, restarting sync...\n");
        Reset(true);
        nTimeLastProcess = GetTime();
        return;
    }

    if(GetTime() - nTimeLastProcess < MASTERNODE_SYNC_TICK_SECONDS) {
        // too early, nothing to do here
        return;
    }

    nTimeLastProcess = GetTime();
    std::vector<CNode*> vNodesCopy = connman.CopyNodeVector(CConnman::FullyConnectedOnly);

    // gradually request the rest of the votes after sync finished
    if(IsSynced()) {
        m_govman.RequestGovernanceObjectVotes(vNodesCopy, connman);
        connman.ReleaseNodeVector(vNodesCopy);
        return;
    }

    // Calculate "progress" for LOG reporting / GUI notification
    double nSyncProgress = double(nTriedPeerCount + (nCurrentAsset - 1) * 8) / (8*4);
    LogPrint(BCLog::MNSYNC, "CMasternodeSync::ProcessTick -- nTick %d nCurrentAsset %d nTriedPeerCount %d nSyncProgress %f\n", nTick, nCurrentAsset, nTriedPeerCount, nSyncProgress);
    uiInterface.NotifyAdditionalDataSyncProgressChanged(nSyncProgress);

    for (auto& pnode : vNodesCopy)
    {
        CNetMsgMaker msgMaker(pnode->GetCommonVersion());

        // Don't try to sync any data from outbound non-relay "masternode" connections.
        // Inbound connection this early is most likely a "masternode" connection
        // initiated from another node, so skip it too.
        if (!pnode->CanRelay() || (fMasternodeMode && pnode->IsInboundConn())) continue;

        {
            if ((pnode->HasPermission(PF_NOBAN) || pnode->IsManualConn()) && !netfulfilledman->HasFulfilledRequest(pnode->addr, strAllow)) {
                netfulfilledman->RemoveAllFulfilledRequests(pnode->addr);
                netfulfilledman->AddFulfilledRequest(pnode->addr, strAllow);
                LogPrintf("CMasternodeSync::ProcessTick -- skipping mnsync restrictions for peer=%d\n", pnode->GetId());
            }

            if(netfulfilledman->HasFulfilledRequest(pnode->addr, "full-sync")) {
                // We already fully synced from this node recently,
                // disconnect to free this connection slot for another peer.
                pnode->fDisconnect = true;
                LogPrintf("CMasternodeSync::ProcessTick -- disconnecting from recently synced peer=%d\n", pnode->GetId());
                continue;
            }

            // SPORK : ALWAYS ASK FOR SPORKS AS WE SYNC

            if(!netfulfilledman->HasFulfilledRequest(pnode->addr, "spork-sync")) {
                // always get sporks first, only request once from each peer
                netfulfilledman->AddFulfilledRequest(pnode->addr, "spork-sync");
                // get current network sporks
                connman.PushMessage(pnode, msgMaker.Make(NetMsgType::GETSPORKS));
                LogPrint(BCLog::MNSYNC, "CMasternodeSync::ProcessTick -- nTick %d nCurrentAsset %d -- requesting sporks from peer=%d\n", nTick, nCurrentAsset, pnode->GetId());
            }

            if (nCurrentAsset == MASTERNODE_SYNC_BLOCKCHAIN) {
                int64_t nTimeSyncTimeout = vNodesCopy.size() > 3 ? MASTERNODE_SYNC_TICK_SECONDS : MASTERNODE_SYNC_TIMEOUT_SECONDS;
                if (fReachedBestHeader && (GetTime() - nTimeLastBumped > nTimeSyncTimeout)) {
                    // At this point we know that:
                    // a) there are peers (because we are looping on at least one of them);
                    // b) we waited for at least MASTERNODE_SYNC_TICK_SECONDS/MASTERNODE_SYNC_TIMEOUT_SECONDS
                    //    (depending on the number of connected peers) since we reached the headers tip the last
                    //    time (i.e. since fReachedBestHeader has been set to true);
                    // c) there were no blocks (UpdatedBlockTip, NotifyHeaderTip) or headers (AcceptedBlockHeader)
                    //    for at least MASTERNODE_SYNC_TICK_SECONDS/MASTERNODE_SYNC_TIMEOUT_SECONDS (depending on
                    //    the number of connected peers).
                    // We must be at the tip already, let's move to the next asset.
                    SwitchToNextAsset();
                    uiInterface.NotifyAdditionalDataSyncProgressChanged(nSyncProgress);

                    if (gArgs.GetBoolArg("-syncmempool", DEFAULT_SYNC_MEMPOOL)) {
                        // Now that the blockchain is synced request the mempool from the connected outbound nodes if possible
                        for (auto pNodeTmp : vNodesCopy) {
                            bool fRequestedEarlier = netfulfilledman->HasFulfilledRequest(pNodeTmp->addr, "mempool-sync");
                            if (pNodeTmp->nVersion >= 70216 && !pNodeTmp->IsInboundConn() && !fRequestedEarlier && !pNodeTmp->IsBlockRelayOnly()) {
                                netfulfilledman->AddFulfilledRequest(pNodeTmp->addr, "mempool-sync");
                                connman.PushMessage(pNodeTmp, msgMaker.Make(NetMsgType::MEMPOOL));
                                LogPrint(BCLog::MNSYNC, "CMasternodeSync::ProcessTick -- nTick %d nCurrentAsset %d -- syncing mempool from peer=%d\n", nTick, nCurrentAsset, pNodeTmp->GetId());
                            }
                        }
                    }
                }
            }

            // GOVOBJ : SYNC GOVERNANCE ITEMS FROM OUR PEERS

            if(nCurrentAsset == MASTERNODE_SYNC_GOVERNANCE) {
                if (fDisableGovernance) {
                    SwitchToNextAsset();
                    connman.ReleaseNodeVector(vNodesCopy);
                    return;
                }
                LogPrint(BCLog::GOBJECT, "CMasternodeSync::ProcessTick -- nTick %d nCurrentAsset %d nTimeLastBumped %lld GetTime() %lld diff %lld\n", nTick, nCurrentAsset, nTimeLastBumped, GetTime(), GetTime() - nTimeLastBumped);

                // check for timeout first
                if(GetTime() - nTimeLastBumped > MASTERNODE_SYNC_TIMEOUT_SECONDS) {
                    LogPrint(BCLog::MNSYNC, "CMasternodeSync::ProcessTick -- nTick %d nCurrentAsset %d -- timeout\n", nTick, nCurrentAsset);
                    if(nTriedPeerCount == 0) {
                        LogPrintf("CMasternodeSync::ProcessTick -- WARNING: failed to sync %s\n", GetAssetName());
                        // it's kind of ok to skip this for now, hopefully we'll catch up later?
                    }
                    SwitchToNextAsset();
                    connman.ReleaseNodeVector(vNodesCopy);
                    return;
                }

                // only request obj sync once from each peer
                if(netfulfilledman->HasFulfilledRequest(pnode->addr, "governance-sync")) {
                    // will request votes on per-obj basis from each node in a separate loop below
                    // to avoid deadlocks here
                    continue;
                }
                netfulfilledman->AddFulfilledRequest(pnode->addr, "governance-sync");

                nTriedPeerCount++;

                SendGovernanceSyncRequest(pnode);

                break; //this will cause each peer to get one request each six seconds for the various assets we need
            }
        }
    }


    if (nCurrentAsset != MASTERNODE_SYNC_GOVERNANCE) {
        // looped through all nodes and not syncing governance yet/already, release them
        connman.ReleaseNodeVector(vNodesCopy);
        return;
    }

    // request votes on per-obj basis from each node
    for (auto& pnode : vNodesCopy) {
        if(!netfulfilledman->HasFulfilledRequest(pnode->addr, "governance-sync")) {
            continue; // to early for this node
        }
        int nObjsLeftToAsk = m_govman.RequestGovernanceObjectVotes(*pnode, connman);
        // check for data
        if(nObjsLeftToAsk == 0) {
            static int64_t nTimeNoObjectsLeft = 0;
            static int nLastTick = 0;
            static int nLastVotes = 0;
            if(nTimeNoObjectsLeft == 0) {
                // asked all objects for votes for the first time
                nTimeNoObjectsLeft = GetTime();
            }
            // make sure the condition below is checked only once per tick
            if(nLastTick == nTick) continue;
            if(GetTime() - nTimeNoObjectsLeft > MASTERNODE_SYNC_TIMEOUT_SECONDS &&
                m_govman.GetVoteCount() - nLastVotes < std::max(int(0.0001 * nLastVotes), MASTERNODE_SYNC_TICK_SECONDS)
            ) {
                // We already asked for all objects, waited for MASTERNODE_SYNC_TIMEOUT_SECONDS
                // after that and less then 0.01% or MASTERNODE_SYNC_TICK_SECONDS
                // (i.e. 1 per second) votes were received during the last tick.
                // We can be pretty sure that we are done syncing.
                LogPrintf("CMasternodeSync::ProcessTick -- nTick %d nCurrentAsset %d -- asked for all objects, nothing to do\n", nTick, MASTERNODE_SYNC_GOVERNANCE);
                // reset nTimeNoObjectsLeft to be able to use the same condition on resync
                nTimeNoObjectsLeft = 0;
                SwitchToNextAsset();
                connman.ReleaseNodeVector(vNodesCopy);
                return;
            }
            nLastTick = nTick;
            nLastVotes = m_govman.GetVoteCount();
        }
    }

    // looped through all nodes, release them
    connman.ReleaseNodeVector(vNodesCopy);
}

void CMasternodeSync::SendGovernanceSyncRequest(CNode* pnode) const
{
    CNetMsgMaker msgMaker(pnode->GetCommonVersion());

    CBloomFilter filter;

    connman.PushMessage(pnode, msgMaker.Make(NetMsgType::MNGOVERNANCESYNC, uint256(), filter));
}

void CMasternodeSync::AcceptedBlockHeader(const CBlockIndex *pindexNew)
{
    LogPrint(BCLog::MNSYNC, "CMasternodeSync::AcceptedBlockHeader -- pindexNew->nHeight: %d\n", pindexNew->nHeight);

    if (!IsBlockchainSynced()) {
        // Postpone timeout each time new block header arrives while we are still syncing blockchain
        BumpAssetLastTime("CMasternodeSync::AcceptedBlockHeader");
    }
}

void CMasternodeSync::NotifyHeaderTip(const CBlockIndex *pindexNew, bool fInitialDownload)
{
    if (pindexNew == nullptr) {
        return;
    }
    LogPrint(BCLog::MNSYNC, "CMasternodeSync::NotifyHeaderTip -- pindexNew->nHeight: %d fInitialDownload=%d\n", pindexNew->nHeight, fInitialDownload);
    if (IsSynced())
        return;

    if (!IsBlockchainSynced()) {
        // Postpone timeout each time new block arrives while we are still syncing blockchain
        BumpAssetLastTime("CMasternodeSync::NotifyHeaderTip");
    }
}

void CMasternodeSync::UpdatedBlockTip(const CBlockIndex *pindexNew, bool fInitialDownload)
{
    LogPrint(BCLog::MNSYNC, "CMasternodeSync::UpdatedBlockTip -- pindexNew->nHeight: %d fInitialDownload=%d\n", pindexNew->nHeight, fInitialDownload);
    nTimeLastUpdateBlockTip = GetTime<std::chrono::seconds>().count();

    if (IsSynced())
        return;

    if (!IsBlockchainSynced()) {
        // Postpone timeout each time new block arrives while we are still syncing blockchain
        BumpAssetLastTime("CMasternodeSync::UpdatedBlockTip");
    }

    if (fInitialDownload) {
        // switched too early
        if (IsBlockchainSynced()) {
            Reset(true);
        }

        // no need to check any further while still in IBD mode
        return;
    }

    // Note: since we sync headers first, it should be ok to use this
    CBlockIndex* pindexTip = WITH_LOCK(cs_main, return pindexBestHeader);
    if (pindexTip == nullptr) return;
    bool fReachedBestHeaderNew = pindexNew->GetBlockHash() == pindexTip->GetBlockHash();

    if (fReachedBestHeader && !fReachedBestHeaderNew) {
        // Switching from true to false means that we previously stuck syncing headers for some reason,
        // probably initial timeout was not enough,
        // because there is no way we can update tip not having best header
        Reset(true);
    }

    fReachedBestHeader = fReachedBestHeaderNew;
    LogPrint(BCLog::MNSYNC, "CMasternodeSync::UpdatedBlockTip -- pindexNew->nHeight: %d pindexTip->nHeight: %d fInitialDownload=%d fReachedBestHeader=%d\n",
                pindexNew->nHeight, pindexTip->nHeight, fInitialDownload, fReachedBestHeader);
}

void CMasternodeSync::DoMaintenance()
{
    if (ShutdownRequested()) return;

    ProcessTick();
}
