// Copyright (c) 2014-2016 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "activemasternode.h"
#include "governance.h"
#include "main.h"
#include "masternode.h"
#include "masternode-payments.h"
#include "masternode-sync.h"
#include "masternodeman.h"
#include "spork.h"
#include "util.h"

class CMasternodeSync;
CMasternodeSync masternodeSync;

bool CMasternodeSync::IsBlockchainSynced()
{
    static bool fBlockchainSynced = false;
    static int64_t lastProcess = GetTime();

    // if the last call to this function was more than 60 minutes ago (client was in sleep mode) reset the sync process
    if(GetTime() - lastProcess > 60*60) {
        Reset();
        fBlockchainSynced = false;
    }
    lastProcess = GetTime();

    if(fBlockchainSynced) return true;

    if (fImporting || fReindex) return false;

    if(!pCurrentBlockIndex) return false;
    if(pCurrentBlockIndex->nTime + 60*60 < GetTime()) return false;

    fBlockchainSynced = true;

    return true;
}

void CMasternodeSync::Fail()
{
    nTimeLastFailure = GetTime();
    nRequestedMasternodeAssets = MASTERNODE_SYNC_FAILED;
}

void CMasternodeSync::Reset()
{
    nRequestedMasternodeAssets = MASTERNODE_SYNC_INITIAL;
    nRequestedMasternodeAttempt = 0;
    nTimeAssetSyncStarted = GetTime();
    nTimeLastMasternodeList = GetTime();
    nTimeLastMasternodeWinner = GetTime();
    nTimeLastBudgetItem = GetTime();
    nTimeLastFailure = 0;
    nCountFailures = 0;
    nSumMasternodeList = 0;
    nSumMasternodeWinner = 0;
    nSumBudgetItemProp = 0;
    nSumBudgetItemFin = 0;
    nCountMasternodeList = 0;
    nCountMasternodeWinner = 0;
    nCountBudgetItemProp = 0;
    nCountBudgetItemFin = 0;
    mapSeenSyncMNB.clear();
    mapSeenSyncMNW.clear();
    mapSeenSyncBudget.clear();
}

void CMasternodeSync::AddedMasternodeList(uint256 hash)
{
    if(mnodeman.mapSeenMasternodeBroadcast.count(hash)) {
        nTimeLastMasternodeList = GetTime();
        mapSeenSyncMNB[hash]++;
    } else {
        nTimeLastMasternodeList = GetTime();
        mapSeenSyncMNB.insert(std::make_pair(hash, 1));
    }
}

void CMasternodeSync::AddedMasternodeWinner(uint256 hash)
{
    if(mnpayments.mapMasternodePayeeVotes.count(hash)) {
        nTimeLastMasternodeWinner = GetTime();
        mapSeenSyncMNW[hash]++;
    } else {
        nTimeLastMasternodeWinner = GetTime();
        mapSeenSyncMNW.insert(std::make_pair(hash, 1));
    }
}

std::string CMasternodeSync::GetAssetName()
{
    switch(nRequestedMasternodeAssets)
    {
        case(MASTERNODE_SYNC_INITIAL):      return "MASTERNODE_SYNC_INITIAL";
        case(MASTERNODE_SYNC_SPORKS):       return "MASTERNODE_SYNC_SPORKS";
        case(MASTERNODE_SYNC_LIST):         return "MASTERNODE_SYNC_LIST";
        case(MASTERNODE_SYNC_MNW):          return "MASTERNODE_SYNC_MNW";
        case(MASTERNODE_SYNC_GOVERNANCE):   return "MASTERNODE_SYNC_GOVERNANCE";
        case(MASTERNODE_SYNC_FAILED):       return "MASTERNODE_SYNC_FAILED";
        case MASTERNODE_SYNC_FINISHED:      return "MASTERNODE_SYNC_FINISHED";
        default:                            return "UNKNOWN";
    }
}

void CMasternodeSync::SwitchToNextAsset()
{
    switch(nRequestedMasternodeAssets)
    {
        case(MASTERNODE_SYNC_FAILED):
            throw std::runtime_error("Can't switch to next asset from failed, should use Reset() first!");
            break;
        case(MASTERNODE_SYNC_INITIAL):
            ClearFulfilledRequest();
            nRequestedMasternodeAssets = MASTERNODE_SYNC_SPORKS;
            break;
        case(MASTERNODE_SYNC_SPORKS):
            nTimeLastMasternodeList = GetTime();
            nRequestedMasternodeAssets = MASTERNODE_SYNC_LIST;
            break;
        case(MASTERNODE_SYNC_LIST):
            nTimeLastMasternodeWinner = GetTime();
            nRequestedMasternodeAssets = MASTERNODE_SYNC_MNW;
            break;
        case(MASTERNODE_SYNC_MNW):
            nTimeLastBudgetItem = GetTime();
            nRequestedMasternodeAssets = MASTERNODE_SYNC_GOVERNANCE;
            break;
        case(MASTERNODE_SYNC_GOVERNANCE):
            LogPrintf("CMasternodeSync::SwitchToNextAsset -- Sync has finished\n");
            nRequestedMasternodeAssets = MASTERNODE_SYNC_FINISHED;
            uiInterface.NotifyAdditionalDataSyncProgressChanged(1);
            //try to activate our masternode if possible
            activeMasternode.ManageState();
            break;
    }
    nRequestedMasternodeAttempt = 0;
    nTimeAssetSyncStarted = GetTime();
}

std::string CMasternodeSync::GetSyncStatus()
{
    switch (masternodeSync.nRequestedMasternodeAssets) {
        case MASTERNODE_SYNC_INITIAL:       return _("Synchronization pending...");
        case MASTERNODE_SYNC_SPORKS:        return _("Synchronizing sporks...");
        case MASTERNODE_SYNC_LIST:          return _("Synchronizing masternodes...");
        case MASTERNODE_SYNC_MNW:           return _("Synchronizing masternode winners...");
        case MASTERNODE_SYNC_GOVERNANCE:    return _("Synchronizing governance objects...");
        case MASTERNODE_SYNC_FAILED:        return _("Synchronization failed");
        case MASTERNODE_SYNC_FINISHED:      return _("Synchronization finished");
        default:                            return "";
    }
}

void CMasternodeSync::ProcessMessage(CNode* pfrom, std::string& strCommand, CDataStream& vRecv)
{
    if (strCommand == NetMsgType::SYNCSTATUSCOUNT) { //Sync status count
        int nItemID;
        int nCount;
        vRecv >> nItemID >> nCount;

        //do not care about stats if sync process finished or failed
        if(IsSynced() || IsFailed()) return;

        switch(nItemID)
        {
            case(MASTERNODE_SYNC_LIST):
                if(nItemID != nRequestedMasternodeAssets) return;
                nSumMasternodeList += nCount;
                nCountMasternodeList++;
                break;
            case(MASTERNODE_SYNC_MNW):
                if(nItemID != nRequestedMasternodeAssets) return;
                nSumMasternodeWinner += nCount;
                nCountMasternodeWinner++;
                break;
            case(MASTERNODE_SYNC_GOVOBJ):
                if(nRequestedMasternodeAssets != MASTERNODE_SYNC_GOVERNANCE) return;
                nSumBudgetItemProp += nCount;
                nCountBudgetItemProp++;
                break;
            case(MASTERNODE_SYNC_GOVERNANCE_FIN):
                if(nRequestedMasternodeAssets != MASTERNODE_SYNC_GOVERNANCE) return;
                nSumBudgetItemFin += nCount;
                nCountBudgetItemFin++;
                break;
        }

        LogPrintf("CMasternodeSync::ProcessMessage -- SYNCSTATUSCOUNT -- got inventory count: nItemID=%d  nCount=%d\n", nItemID, nCount);
    }
}

void CMasternodeSync::ClearFulfilledRequest()
{
    TRY_LOCK(cs_vNodes, lockRecv);
    if(!lockRecv) return;

    BOOST_FOREACH(CNode* pnode, vNodes)
    {
        pnode->ClearFulfilledRequest("spork-sync");
        pnode->ClearFulfilledRequest("masternode-winner-sync");
        pnode->ClearFulfilledRequest("governance-sync");
        pnode->ClearFulfilledRequest("masternode-sync");
    }
}

void CMasternodeSync::ProcessTick()
{
    static int nTick = 0;
    if(nTick++ % 6 != 0) return;
    if(!pCurrentBlockIndex) return;

    //the actual count of masternodes we have currently
    int nMnCount = mnodeman.CountEnabled();

    // RESET SYNCING INCASE OF FAILURE
    {
        if(IsSynced()) {
            /*
                Resync if we lose all masternodes from sleep/wake or failure to sync originally
            */
            if(nMnCount == 0) {
                Reset();
            } else {
                //if syncing is complete and we have masternodes, return
                return;
            }
        }

        //try syncing again
        if(IsFailed()) {
            if(nTimeLastFailure + (1*60) < GetTime()) { // 1 minute cooldown after failed sync
                Reset();
            }
            return;
        }
    }

    // INITIAL SYNC SETUP / LOG REPORTING
    double nSyncProgress = double(nRequestedMasternodeAttempt + (nRequestedMasternodeAssets - 1) * 8) / (8*4);
    LogPrintf("CMasternodeSync::Process -- nTick %d nRequestedMasternodeAssets %d nRequestedMasternodeAttempt %d nSyncProgress %f\n", nTick, nRequestedMasternodeAssets, nRequestedMasternodeAttempt, nSyncProgress);
    uiInterface.NotifyAdditionalDataSyncProgressChanged(nSyncProgress);

    // sporks synced but blockchain is not, wait until we're almost at a recent block to continue
    if(Params().NetworkIDString() != CBaseChainParams::REGTEST &&
            !IsBlockchainSynced() && nRequestedMasternodeAssets > MASTERNODE_SYNC_SPORKS) return;

    TRY_LOCK(cs_vNodes, lockRecv);
    if(!lockRecv) return;

    if(nRequestedMasternodeAssets == MASTERNODE_SYNC_INITIAL) {
        SwitchToNextAsset();
    }

    BOOST_FOREACH(CNode* pnode, vNodes)
    {
        // QUICK MODE (REGTEST ONLY!)
        if(Params().NetworkIDString() == CBaseChainParams::REGTEST)
        {
            if(nRequestedMasternodeAttempt <= 2) {
                pnode->PushMessage(NetMsgType::GETSPORKS); //get current network sporks
            } else if(nRequestedMasternodeAttempt < 4) {
                mnodeman.DsegUpdate(pnode);
            } else if(nRequestedMasternodeAttempt < 6) {
                int nMnCount = mnodeman.CountEnabled();
                pnode->PushMessage(NetMsgType::MNWINNERSSYNC, nMnCount); //sync payees
                uint256 n = uint256();
                pnode->PushMessage(NetMsgType::MNGOVERNANCESYNC, n); //sync masternode votes
            } else {
                nRequestedMasternodeAssets = MASTERNODE_SYNC_FINISHED;
            }
            nRequestedMasternodeAttempt++;
            return;
        }

        // NORMAL NETWORK MODE - TESTNET/MAINNET
        {
            // SPORK : ALWAYS ASK FOR SPORKS AS WE SYNC (we skip this mode now)

            if(!pnode->HasFulfilledRequest("spork-sync")) {
                // only request once from each peer
                pnode->FulfilledRequest("spork-sync");

                // get current network sporks
                pnode->PushMessage(NetMsgType::GETSPORKS);

                // we always ask for sporks, so just skip this
                if(nRequestedMasternodeAssets == MASTERNODE_SYNC_SPORKS) SwitchToNextAsset();

                continue; // always get sporks first, switch to the next node without waiting for the next tick
            }

            // MNLIST : SYNC MASTERNODE LIST FROM OTHER CONNECTED CLIENTS

            if(nRequestedMasternodeAssets == MASTERNODE_SYNC_LIST) {
                // check for timeout first
                if(nTimeLastMasternodeList < GetTime() - MASTERNODE_SYNC_TIMEOUT_SECONDS) {
                    LogPrintf("CMasternodeSync::Process -- nTick %d nRequestedMasternodeAssets %d -- timeout\n", nTick, nRequestedMasternodeAssets);
                    if (nRequestedMasternodeAttempt == 0) {
                        LogPrintf("CMasternodeSync::Process -- WARNING: failed to sync %s\n", GetAssetName());
                        // there is no way we can continue without masternode list, fail here and try later
                        Fail();
                        return;
                    }
                    SwitchToNextAsset();
                    return;
                }

                // check for data
                // if we have enough masternodes in or list, switch to the next asset
                /* Note: Is this activing up? It's probably related to int CMasternodeMan::GetEstimatedMasternodes(int nBlock)
                   Surely doesn't work right for testnet currently */
                // try to fetch data from at least two peers though
                if(nRequestedMasternodeAttempt > 1 && nMnCount > mnodeman.GetEstimatedMasternodes(pCurrentBlockIndex->nHeight)*0.9) {
                    LogPrintf("CMasternodeSync::Process -- nTick %d nRequestedMasternodeAssets %d -- found enough data\n", nTick, nRequestedMasternodeAssets);
                    SwitchToNextAsset();
                    return;
                }

                // only request once from each peer
                if(pnode->HasFulfilledRequest("masternode-sync")) continue;
                pnode->FulfilledRequest("masternode-sync");

                if (pnode->nVersion < mnpayments.GetMinMasternodePaymentsProto()) continue;
                nRequestedMasternodeAttempt++;

                mnodeman.DsegUpdate(pnode);

                return; //this will cause each peer to get one request each six seconds for the various assets we need
            }

            // MNW : SYNC MASTERNODE WINNERS FROM OTHER CONNECTED CLIENTS

            if(nRequestedMasternodeAssets == MASTERNODE_SYNC_MNW) {
                // check for timeout first
                // This might take a lot longer than MASTERNODE_SYNC_TIMEOUT_SECONDS minutes due to new blocks,
                // but that should be OK and it should timeout eventually.
                if(nTimeLastMasternodeWinner < GetTime() - MASTERNODE_SYNC_TIMEOUT_SECONDS) {
                    LogPrintf("CMasternodeSync::Process -- nTick %d nRequestedMasternodeAssets %d -- timeout\n", nTick, nRequestedMasternodeAssets);
                    if (nRequestedMasternodeAttempt == 0) {
                        LogPrintf("CMasternodeSync::Process -- WARNING: failed to sync %s\n", GetAssetName());
                        // probably not a good idea to proceed without winner list
                        Fail();
                        return;
                    }
                    SwitchToNextAsset();
                    return;
                }

                // check for data
                // if mnpayments already has enough blocks and votes, switch to the next asset
                // try to fetch data from at least two peers though
                if(nRequestedMasternodeAttempt > 1 && mnpayments.IsEnoughData(nMnCount)) {
                    LogPrintf("CMasternodeSync::Process -- nTick %d nRequestedMasternodeAssets %d -- found enough data\n", nTick, nRequestedMasternodeAssets);
                    SwitchToNextAsset();
                    return;
                }

                // only request once from each peer
                if(pnode->HasFulfilledRequest("masternode-winner-sync")) continue;
                pnode->FulfilledRequest("masternode-winner-sync");

                if(pnode->nVersion < mnpayments.GetMinMasternodePaymentsProto()) continue;
                nRequestedMasternodeAttempt++;

                pnode->PushMessage(NetMsgType::MNWINNERSSYNC, nMnCount); //sync payees


                return; //this will cause each peer to get one request each six seconds for the various assets we need
            }

            // GOVOBJ : SYNC GOVERNANCE ITEMS FROM OUR PEERS

            if(nRequestedMasternodeAssets == MASTERNODE_SYNC_GOVERNANCE) {
                // check for timeout first
                if(nTimeLastBudgetItem < GetTime() - MASTERNODE_SYNC_TIMEOUT_SECONDS){
                    LogPrintf("CMasternodeSync::Process -- nTick %d nRequestedMasternodeAssets %d -- timeout\n", nTick, nRequestedMasternodeAssets);
                    if(nRequestedMasternodeAttempt == 0) {
                        LogPrintf("CMasternodeSync::Process -- WARNING: failed to sync %s\n", GetAssetName());
                        // it's kind of ok to skip this for now, hopefully we'll catch up later?
                    }
                    SwitchToNextAsset();
                    return;
                }

                // check for data
                // if(nCountBudgetItemProp > 0 && nCountBudgetItemFin)
                // {
                //     if(governance.CountProposalInventoryItems() >= (nSumBudgetItemProp / nCountBudgetItemProp)*0.9)
                //     {
                //         if(governance.CountFinalizedInventoryItems() >= (nSumBudgetItemFin / nCountBudgetItemFin)*0.9)
                //         {
                //             SwitchToNextAsset();
                //             return;
                //         }
                //     }
                // }

                // only request once from each peer
                if(pnode->HasFulfilledRequest("governance-sync")) continue;
                pnode->FulfilledRequest("governance-sync");

                if (pnode->nVersion < MSG_GOVERNANCE_PEER_PROTO_VERSION) continue;
                nRequestedMasternodeAttempt++;

                pnode->PushMessage(NetMsgType::MNGOVERNANCESYNC, uint256()); //sync masternode votes

                return; //this will cause each peer to get one request each six seconds for the various assets we need
            }
        }
    }
}

void CMasternodeSync::UpdatedBlockTip(const CBlockIndex *pindex)
{
    pCurrentBlockIndex = pindex;
}


void CMasternodeSync::AddedBudgetItem(uint256 hash)
{
    // skip this for now
    return;
}
