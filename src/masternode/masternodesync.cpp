// Copyright (c) 2014-2020 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <governance/governance.h>
#include <init.h>
#include <validation.h>
#include <masternode/masternodesync.h>
#include <netfulfilledman.h>
#include <netmessagemaker.h>
#include <node/ui_interface.h>
#include <evo/deterministicmns.h>
#include <shutdown.h>
#include <util/translation.h>
#include <util/system.h>

class CMasternodeSync;
CMasternodeSync masternodeSync;

void CMasternodeSync::Reset(bool fForce, bool fNotifyReset)
{
    // Avoid resetting the sync process if we just "recently" received a new block
    if (fForce || (GetTime() - nTimeLastUpdateBlockTip > MASTERNODE_SYNC_RESET_SECONDS)) {
        SetSyncMode(MASTERNODE_SYNC_BLOCKCHAIN);
        nTriedPeerCount = 0;
        nTimeAssetSyncStarted = GetTime();
        nTimeLastBumped = GetTime();
        nTimeLastUpdateBlockTip = 0;
        fReachedBestHeader = false;
        if (fNotifyReset) {
            uiInterface.NotifyAdditionalDataSyncProgressChanged(-1);
        }
    }
}

void CMasternodeSync::BumpAssetLastTime(const std::string& strFuncName)
{
    if(IsSynced()) return;
    nTimeLastBumped = GetTime();
    LogPrint(BCLog::MNSYNC, "CMasternodeSync::BumpAssetLastTime -- %s\n", strFuncName);
}

std::string CMasternodeSync::GetAssetName() const
{
    switch(GetAssetID())
    {
        case(MASTERNODE_SYNC_BLOCKCHAIN):   return "MASTERNODE_SYNC_BLOCKCHAIN";
        case(MASTERNODE_SYNC_GOVERNANCE):   return "MASTERNODE_SYNC_GOVERNANCE";
        case MASTERNODE_SYNC_FINISHED:      return "MASTERNODE_SYNC_FINISHED";
        default:                            return "UNKNOWN";
    }
}

void CMasternodeSync::SwitchToNextAsset(CConnman& connman)
{
    switch(GetAssetID())
    {
        case(MASTERNODE_SYNC_BLOCKCHAIN):
            LogPrintf("CMasternodeSync::SwitchToNextAsset -- Completed %s in %llds\n", GetAssetName(), GetTime() - nTimeAssetSyncStarted);
            SetSyncMode(MASTERNODE_SYNC_GOVERNANCE);
            LogPrintf("CMasternodeSync::SwitchToNextAsset -- Starting %s\n", GetAssetName());
            break;
        case(MASTERNODE_SYNC_GOVERNANCE):
            LogPrintf("CMasternodeSync::SwitchToNextAsset -- Completed %s in %llds\n", GetAssetName(), GetTime() - nTimeAssetSyncStarted);
            SetSyncMode(MASTERNODE_SYNC_FINISHED);
            uiInterface.NotifyAdditionalDataSyncProgressChanged(1);

            connman.ForEachNode(AllNodes, [](CNode* pnode) {
                netfulfilledman.AddFulfilledRequest(pnode->addr, "full-sync");
            });
            LogPrintf("CMasternodeSync::SwitchToNextAsset -- Sync has finished\n");

            break;
    }
    nTriedPeerCount = 0;
    nTimeAssetSyncStarted = GetTime();
    BumpAssetLastTime("CMasternodeSync::SwitchToNextAsset");
}

bilingual_str CMasternodeSync::GetSyncStatus()
{
    switch (GetAssetID()) {
        case MASTERNODE_SYNC_BLOCKCHAIN:    return _("Synchronizing blockchain...");
        case MASTERNODE_SYNC_GOVERNANCE:    return _("Synchronizing governance objects...");
        case MASTERNODE_SYNC_FINISHED:      return _("Synchronization finished");
        default:                            return _("Unknown State");
    }
}

void CMasternodeSync::ProcessMessage(CNode* pfrom, const std::string& strCommand, CDataStream& vRecv) const
{
    if (strCommand == NetMsgType::SYNCSTATUSCOUNT) { //Sync status count

        //do not care about stats if sync process finished or failed
        if(IsSynced()) return;

        int nItemID;
        int nCount;
        vRecv >> nItemID >> nCount;

        LogPrint(BCLog::MNSYNC, "SYNCSTATUSCOUNT -- got inventory count: nItemID=%d  nCount=%d  peer=%d\n", nItemID, nCount, pfrom->GetId());
    }
}

void CMasternodeSync::ProcessTick(CConnman& connman, const PeerManager& peerman)
{
    static int nTick = 0;
    nTick++;

    const static int64_t nSyncStart = GetTimeMillis();
    const static std::string strAllow = strprintf("allow-sync-%lld", nSyncStart);
    const int nMode = GetAssetID();
    // reset the sync process if the last call to this function was more than 60 minutes ago (client was in sleep mode)
    static int64_t nTimeLastProcess = GetTime();
    if(GetTime() - nTimeLastProcess > 60*60 && !fMasternodeMode) {
        LogPrint(BCLog::MNSYNC, "CMasternodeSync::ProcessTick -- WARNING: no actions for too long, restarting sync...\n");
        Reset(true);
        nTimeLastProcess = GetTime();
        return;
    }

    if(GetTime() - nTimeLastProcess < MASTERNODE_SYNC_TICK_SECONDS) {
        // too early, nothing to do here
        return;
    }

    nTimeLastProcess = GetTime();

    // gradually request the rest of the votes after sync finished
    if(IsSynced()) {
        std::vector<CNode*> vNodesCopy;
        connman.CopyNodeVector(vNodesCopy);
        governance.RequestGovernanceObjectVotes(vNodesCopy, connman, peerman);
        connman.ReleaseNodeVector(vNodesCopy);
        return;
    }

    // Calculate "progress" for LOG reporting / GUI notification
    double nSyncProgress = double(nTriedPeerCount + (nMode - 1) * 8) / (8*4);
    LogPrint(BCLog::MNSYNC, "CMasternodeSync::ProcessTick -- nTick %d nCurrentAsset %d nTriedPeerCount %d nSyncProgress %f\n", nTick, nMode, nTriedPeerCount, nSyncProgress);
    uiInterface.NotifyAdditionalDataSyncProgressChanged(nSyncProgress);

    std::vector<CNode*> vNodesCopy;
    connman.CopyNodeVector(vNodesCopy);

    for (CNode* pnode : vNodesCopy)
    {
        CNetMsgMaker msgMaker(pnode->GetCommonVersion());

        // Don't try to sync any data from outbound non-relay "masternode" connections.
        // Inbound connection this early is most likely a "masternode" connection
        // initiated from another node, so skip it too.
        if (!pnode->CanRelay() || (fMasternodeMode && pnode->IsInboundConn())) continue;

        // QUICK MODE (REGTEST ONLY!)
        if(fRegTest)
        {
            if (nMode == MASTERNODE_SYNC_BLOCKCHAIN) {
                connman.PushMessage(pnode, msgMaker.Make(NetMsgType::GETSPORKS)); //get current network sporks
                SwitchToNextAsset(connman);
            } else if (nMode == MASTERNODE_SYNC_GOVERNANCE) {
                SendGovernanceSyncRequest(pnode, connman);
                SwitchToNextAsset(connman);
            }
            connman.ReleaseNodeVector(vNodesCopy);
            return;
        }

        // NORMAL NETWORK MODE - TESTNET/MAINNET
        {
            if ((pnode->HasPermission(PF_NOBAN) || pnode->IsManualConn()) && !netfulfilledman.HasFulfilledRequest(pnode->addr, strAllow)) {
                netfulfilledman.RemoveAllFulfilledRequests(pnode->addr);
                netfulfilledman.AddFulfilledRequest(pnode->addr, strAllow);
                LogPrintf("CMasternodeSync::ProcessTick -- skipping mnsync restrictions for peer=%d\n", pnode->GetId());
            }

            if(netfulfilledman.HasFulfilledRequest(pnode->addr, "full-sync")) {
                // We already fully synced from this node recently,
                // disconnect to free this connection slot for another peer.
                pnode->fDisconnect = true;
                LogPrintf("CMasternodeSync::ProcessTick -- disconnecting from recently synced peer=%d\n", pnode->GetId());
                continue;
            }

            // SPORK : ALWAYS ASK FOR SPORKS AS WE SYNC

            if(!netfulfilledman.HasFulfilledRequest(pnode->addr, "spork-sync")) {
                // always get sporks first, only request once from each peer
                netfulfilledman.AddFulfilledRequest(pnode->addr, "spork-sync");
                // get current network sporks
                connman.PushMessage(pnode, msgMaker.Make(NetMsgType::GETSPORKS));
                LogPrint(BCLog::MNSYNC, "CMasternodeSync::ProcessTick -- nTick %d nMode %d -- requesting sporks from peer=%d\n", nTick, nMode, pnode->GetId());
            }

            if (nMode == MASTERNODE_SYNC_BLOCKCHAIN) {
                int64_t nTimeSyncTimeout = vNodesCopy.size() > 3 ? MASTERNODE_SYNC_TICK_SECONDS : MASTERNODE_SYNC_TIMEOUT_SECONDS;
                if (fReachedBestHeader && (GetTime() - nTimeLastBumped > nTimeSyncTimeout)) {
                    // At this point we know that:
                    // a) there are peers (because we are looping on at least one of them);
                    // b) we waited for at least MASTERNODE_SYNC_TICK_SECONDS/MASTERNODE_SYNC_TIMEOUT_SECONDS
                    //    (depending on the number of connected peers) since we reached the headers tip the last
                    //    time (i.e. since fReachedBestHeader has been set to true);
                    // c) there were no blocks (UpdatedBlockTip, NotifyHeaderTip)
                    //    for at least MASTERNODE_SYNC_TICK_SECONDS/MASTERNODE_SYNC_TIMEOUT_SECONDS (depending on
                    //    the number of connected peers).
                    // We must be at the tip already, let's move to the next asset.
                    SwitchToNextAsset(connman);
                    uiInterface.NotifyAdditionalDataSyncProgressChanged(nSyncProgress);
                    if (gArgs.GetBoolArg("-syncmempool", DEFAULT_SYNC_MEMPOOL)) {
                        // Now that the blockchain is synced request the mempool from the connected outbound nodes if possible
                        for (auto pNodeTmp : vNodesCopy) {
                            bool fRequestedEarlier = netfulfilledman.HasFulfilledRequest(pNodeTmp->addr, "mempool-sync");
                            if (pNodeTmp->nVersion >= 70216 && !pNodeTmp->IsInboundConn() && !fRequestedEarlier) {
                                netfulfilledman.AddFulfilledRequest(pNodeTmp->addr, "mempool-sync");
                                connman.PushMessage(pNodeTmp, msgMaker.Make(NetMsgType::MEMPOOL));
                                LogPrint(BCLog::MNSYNC, "CMasternodeSync::ProcessTick -- nTick %d nMode %d -- syncing mempool from peer=%d\n", nTick, nMode, pNodeTmp->GetId());
                            }
                        }
                    }
                }
            }

            // GOVOBJ : SYNC GOVERNANCE ITEMS FROM OUR PEERS

            if(nMode == MASTERNODE_SYNC_GOVERNANCE) {
                if (fDisableGovernance) {
                    SwitchToNextAsset(connman);
                    connman.ReleaseNodeVector(vNodesCopy);
                    return;
                }
                LogPrint(BCLog::GOBJECT, "CMasternodeSync::ProcessTick -- nTick %d nMode %d nTimeLastBumped %lld GetTime() %lld diff %lld\n", nTick, nMode, nTimeLastBumped, GetTime(), GetTime() - nTimeLastBumped);

                // check for timeout first
                if(GetTime() - nTimeLastBumped > MASTERNODE_SYNC_TIMEOUT_SECONDS) {
                    LogPrintf("CMasternodeSync::ProcessTick -- nTick %d nMode %d -- timeout\n", nTick, nMode);
                    if(nTriedPeerCount == 0) {
                        LogPrintf("CMasternodeSync::ProcessTick -- WARNING: failed to sync %s\n", GetAssetName());
                        // it's kind of ok to skip this for now, hopefully we'll catch up later?
                    }
                    SwitchToNextAsset(connman);
                    connman.ReleaseNodeVector(vNodesCopy);
                    return;
                }

                // only request obj sync once from each peer, then request votes on per-obj basis
                if(netfulfilledman.HasFulfilledRequest(pnode->addr, "governance-sync")) {
                    int nObjsLeftToAsk = governance.RequestGovernanceObjectVotes(pnode, connman, peerman);
                    static int64_t nTimeNoObjectsLeft = 0;
                    // check for data
                    if(nObjsLeftToAsk == 0) {
                        static int nLastTick = 0;
                        static int nLastVotes = 0;
                        if(nTimeNoObjectsLeft == 0) {
                            // asked all objects for votes for the first time
                            nTimeNoObjectsLeft = GetTime();
                        }
                        // make sure the condition below is checked only once per tick
                        if(nLastTick == nTick) continue;
                        if(GetTime() - nTimeNoObjectsLeft > MASTERNODE_SYNC_TIMEOUT_SECONDS &&
                            governance.GetVoteCount() - nLastVotes < std::max(int(0.0001 * nLastVotes), MASTERNODE_SYNC_TICK_SECONDS)
                        ) {
                            // We already asked for all objects, waited for MASTERNODE_SYNC_TIMEOUT_SECONDS
                            // after that and less then 0.01% or MASTERNODE_SYNC_TICK_SECONDS
                            // (i.e. 1 per second) votes were received during the last tick.
                            // We can be pretty sure that we are done syncing.
                            LogPrintf("CMasternodeSync::ProcessTick -- nTick %d nMode %d -- asked for all objects, nothing to do\n", nTick, nMode);
                            // reset nTimeNoObjectsLeft to be able to use the same condition on resync
                            nTimeNoObjectsLeft = 0;
                            SwitchToNextAsset(connman);
                            connman.ReleaseNodeVector(vNodesCopy);
                            return;
                        }
                        nLastTick = nTick;
                        nLastVotes = governance.GetVoteCount();
                    }
                    continue;
                }
                netfulfilledman.AddFulfilledRequest(pnode->addr, "governance-sync");

                nTriedPeerCount++;

                SendGovernanceSyncRequest(pnode, connman);

                connman.ReleaseNodeVector(vNodesCopy);
                return; //this will cause each peer to get one request each six seconds for the various assets we need
            }
        }
    }
    // looped through all nodes, release them
    connman.ReleaseNodeVector(vNodesCopy);
}

void CMasternodeSync::SendGovernanceSyncRequest(CNode* pnode, CConnman& connman)
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

void CMasternodeSync::NotifyHeaderTip(const CBlockIndex *pindexNew, bool fInitialDownload, CConnman& connman)
{
    LogPrint(BCLog::MNSYNC, "CMasternodeSync::NotifyHeaderTip -- pindexNew->nHeight: %d fInitialDownload=%d\n", pindexNew->nHeight, fInitialDownload);

    if (IsSynced() || !pindexBestHeader)
        return;

    if (!IsBlockchainSynced()) {
        // Postpone timeout each time new block arrives while we are still syncing blockchain
        BumpAssetLastTime("CMasternodeSync::NotifyHeaderTip");
    }
}

void CMasternodeSync::UpdatedBlockTip(const CBlockIndex *pindexNew, bool fInitialDownload)
{
    LogPrint(BCLog::MNSYNC, "CMasternodeSync::UpdatedBlockTip -- pindexNew->nHeight: %d fInitialDownload=%d\n", pindexNew->nHeight, fInitialDownload);

    nTimeLastUpdateBlockTip = GetAdjustedTime();

    if (IsSynced() || !pindexNew)
        return;
    {
        LOCK(cs_main);
        if(!pindexBestHeader) {
            return;
        }
    }

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
    bool fReachedBestHeaderNew;
    int32_t bestHeight;
    {
        LOCK(cs_main);
        // Note: since we sync headers first, it should be ok to use this
        fReachedBestHeaderNew = pindexNew->GetBlockHash() == pindexBestHeader->GetBlockHash();
        bestHeight = pindexBestHeader->nHeight;
    }

    if (fReachedBestHeader && !fReachedBestHeaderNew) {
        // Switching from true to false means that we previously stuck syncing headers for some reason,
        // probably initial timeout was not enough,
        // because there is no way we can update tip not having best header
        Reset(true);
    }

    fReachedBestHeader = fReachedBestHeaderNew;

    LogPrint(BCLog::MNSYNC, "CMasternodeSync::UpdatedBlockTip -- pindexNew->nHeight: %d pindexBestHeader->nHeight: %d fInitialDownload=%d fReachedBestHeader=%d\n",
                pindexNew->nHeight, bestHeight, fInitialDownload, fReachedBestHeader);
}

void CMasternodeSync::DoMaintenance(CConnman &connman, const PeerManager& peerman)
{
    if (ShutdownRequested()) return;

    ProcessTick(connman, peerman);
}
