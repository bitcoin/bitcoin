// Copyright (c) 2014-2016 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "main.h"
#include "activemasternode.h"
#include "masternode-sync.h"
#include "masternode-payments.h"
#include "masternode-budget.h"
#include "masternode.h"
#include "masternodeman.h"
#include "spork.h"
#include "util.h"
#include "addrman.h"

class CMasternodeSync;
CMasternodeSync masternodeSync;

CMasternodeSync::CMasternodeSync()
{
    Reset();
}

bool CMasternodeSync::IsSynced()
{
    return RequestedMasternodeAssets == MASTERNODE_SYNC_FINISHED;
}

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

void CMasternodeSync::Reset()
{   
    lastMasternodeList = GetTime();
    lastMasternodeWinner = GetTime();
    lastBudgetItem = GetTime();
    mapSeenSyncMNB.clear();
    mapSeenSyncMNW.clear();
    mapSeenSyncBudget.clear();
    lastFailure = 0;
    nCountFailures = 0;
    sumMasternodeList = 0;
    sumMasternodeWinner = 0;
    sumBudgetItemProp = 0;
    sumBudgetItemFin = 0;
    countMasternodeList = 0;
    countMasternodeWinner = 0;
    countBudgetItemProp = 0;
    countBudgetItemFin = 0;
    RequestedMasternodeAssets = MASTERNODE_SYNC_INITIAL;
    RequestedMasternodeAttempt = 0;
    nAssetSyncStarted = GetTime();
}

void CMasternodeSync::AddedMasternodeList(uint256 hash)
{
    if(mnodeman.mapSeenMasternodeBroadcast.count(hash)) {
        lastMasternodeList = GetTime();
        mapSeenSyncMNB[hash]++;
    } else {
        lastMasternodeList = GetTime();
        mapSeenSyncMNB.insert(make_pair(hash, 1));
    }
}

void CMasternodeSync::AddedMasternodeWinner(uint256 hash)
{
    if(mnpayments.mapMasternodePayeeVotes.count(hash)) {
        lastMasternodeWinner = GetTime();
        mapSeenSyncMNW[hash]++;
    } else {
        lastMasternodeWinner = GetTime();
        mapSeenSyncMNW.insert(make_pair(hash, 1));
    }
}

void CMasternodeSync::AddedBudgetItem(uint256 hash)
{
    if(budget.mapSeenMasternodeBudgetProposals.count(hash) || budget.mapSeenMasternodeBudgetVotes.count(hash) ||
        budget.mapSeenFinalizedBudgets.count(hash) || budget.mapSeenFinalizedBudgetVotes.count(hash)) {
        lastBudgetItem = GetTime();
        mapSeenSyncBudget[hash]++;
    } else {
        lastBudgetItem = GetTime();
        mapSeenSyncBudget.insert(make_pair(hash, 1));
    }
}

bool CMasternodeSync::IsBudgetPropEmpty()
{
    return sumBudgetItemProp==0 && countBudgetItemProp>0;
}

bool CMasternodeSync::IsBudgetFinEmpty()
{
    return sumBudgetItemFin==0 && countBudgetItemFin>0;
}

void CMasternodeSync::GetNextAsset()
{
    switch(RequestedMasternodeAssets)
    {
        case(MASTERNODE_SYNC_INITIAL):
        case(MASTERNODE_SYNC_FAILED): // should never be used here actually, use Reset() instead
            ClearFulfilledRequest();
            RequestedMasternodeAssets = MASTERNODE_SYNC_SPORKS;
            break;
        case(MASTERNODE_SYNC_SPORKS):
            lastMasternodeList = GetTime();
            RequestedMasternodeAssets = MASTERNODE_SYNC_LIST;
            break;
        case(MASTERNODE_SYNC_LIST):
            lastMasternodeWinner = GetTime();
            RequestedMasternodeAssets = MASTERNODE_SYNC_MNW;
            break;
        case(MASTERNODE_SYNC_MNW):
            lastBudgetItem = GetTime();
            RequestedMasternodeAssets = MASTERNODE_SYNC_BUDGET;
            break;
        case(MASTERNODE_SYNC_BUDGET):
            LogPrintf("CMasternodeSync::GetNextAsset - Sync has finished\n");
            RequestedMasternodeAssets = MASTERNODE_SYNC_FINISHED;
            uiInterface.NotifyAdditionalDataSyncProgressChanged(1);
            break;
    }
    RequestedMasternodeAttempt = 0;
    nAssetSyncStarted = GetTime();
}

std::string CMasternodeSync::GetSyncStatus()
{
    switch (masternodeSync.RequestedMasternodeAssets) {
        case MASTERNODE_SYNC_INITIAL: return _("Synchronization pending...");
        case MASTERNODE_SYNC_SPORKS: return _("Synchronizing sporks...");
        case MASTERNODE_SYNC_LIST: return _("Synchronizing masternodes...");
        case MASTERNODE_SYNC_MNW: return _("Synchronizing masternode winners...");
        case MASTERNODE_SYNC_BUDGET: return _("Synchronizing budgets...");
        case MASTERNODE_SYNC_FAILED: return _("Synchronization failed");
        case MASTERNODE_SYNC_FINISHED: return _("Synchronization finished");
    }
    return "";
}

void CMasternodeSync::ProcessMessage(CNode* pfrom, std::string& strCommand, CDataStream& vRecv)
{
    if (strCommand == NetMsgType::SYNCSTATUSCOUNT) { //Sync status count
        int nItemID;
        int nCount;
        vRecv >> nItemID >> nCount;

        if(RequestedMasternodeAssets >= MASTERNODE_SYNC_FINISHED) return;

        //this means we will receive no further communication
        switch(nItemID)
        {
            case(MASTERNODE_SYNC_LIST):
                if(nItemID != RequestedMasternodeAssets) return;
                sumMasternodeList += nCount;
                countMasternodeList++;
                break;
            case(MASTERNODE_SYNC_MNW):
                if(nItemID != RequestedMasternodeAssets) return;
                sumMasternodeWinner += nCount;
                countMasternodeWinner++;
                break;
            case(MASTERNODE_SYNC_BUDGET_PROP):
                if(RequestedMasternodeAssets != MASTERNODE_SYNC_BUDGET) return;
                sumBudgetItemProp += nCount;
                countBudgetItemProp++;
                break;
            case(MASTERNODE_SYNC_BUDGET_FIN):
                if(RequestedMasternodeAssets != MASTERNODE_SYNC_BUDGET) return;
                sumBudgetItemFin += nCount;
                countBudgetItemFin++;
                break;
        }
        
        LogPrintf("CMasternodeSync:ProcessMessage - ssc - got inventory count %d %d\n", nItemID, nCount);
    }
}

void CMasternodeSync::ClearFulfilledRequest()
{
    TRY_LOCK(cs_vNodes, lockRecv);
    if(!lockRecv) return;

    BOOST_FOREACH(CNode* pnode, vNodes)
    {
        pnode->ClearFulfilledRequest("getspork");
        pnode->ClearFulfilledRequest("mnsync");
        pnode->ClearFulfilledRequest("mnwsync");
        pnode->ClearFulfilledRequest("busync");
    }
}

void CMasternodeSync::Process()
{
    static int tick = 0;
    if(tick++ % 6 != 0) return;

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
            } else
                return;
        }

        //try syncing again
        if(RequestedMasternodeAssets == MASTERNODE_SYNC_FAILED && lastFailure + (1*60) < GetTime()) {
            Reset();
        } else if (RequestedMasternodeAssets == MASTERNODE_SYNC_FAILED) {
            return;
        }
    
        if(fDebug) LogPrintf("CMasternodeSync::Process() - tick %d RequestedMasternodeAssets %d\n", tick, RequestedMasternodeAssets);
    }

    double nSyncProgress = double(RequestedMasternodeAttempt + (RequestedMasternodeAssets - 1) * 8) / (8*4);
    LogPrintf("CMasternodeSync::Process() - tick %d RequestedMasternodeAttempt %d RequestedMasternodeAssets %d nSyncProgress %f\n", tick, RequestedMasternodeAttempt, RequestedMasternodeAssets, nSyncProgress);
    uiInterface.NotifyAdditionalDataSyncProgressChanged(nSyncProgress);

    if(RequestedMasternodeAssets == MASTERNODE_SYNC_INITIAL) GetNextAsset();

    // sporks synced but blockchain is not, wait until we're almost at a recent block to continue
    if(Params().NetworkIDString() != CBaseChainParams::REGTEST &&
            !IsBlockchainSynced() && RequestedMasternodeAssets > MASTERNODE_SYNC_SPORKS) return;

    TRY_LOCK(cs_vNodes, lockRecv);
    if(!lockRecv) return;

    BOOST_FOREACH(CNode* pnode, vNodes)
    {
        /* 
            REGTEST QUICK MODE
        */ 
        if(Params().NetworkIDString() == CBaseChainParams::REGTEST){
            if(RequestedMasternodeAttempt <= 2) {
                pnode->PushMessage(NetMsgType::GETSPORKS); //get current network sporks
            } else if(RequestedMasternodeAttempt < 4) {
                mnodeman.DsegUpdate(pnode); 
            } else if(RequestedMasternodeAttempt < 6) {
                int nMnCount = mnodeman.CountEnabled();
                pnode->PushMessage(NetMsgType::MNWINNERSSYNC, nMnCount); //sync payees
                uint256 n = uint256();
                pnode->PushMessage(NetMsgType::MNBUDGETVOTESYNC, n); //sync masternode votes
            } else {
                RequestedMasternodeAssets = MASTERNODE_SYNC_FINISHED;
            }
            RequestedMasternodeAttempt++;
            return;
        }

        /*
            NORMAL NETWORK MODE - TESTNET/MAINNET
        */

        // ALWAYS ASK FOR SPORKS AS WE SYNC (we skip this mode now)
        if(!pnode->HasFulfilledRequest("getspork"))
        {
            pnode->FulfilledRequest("getspork");
            pnode->PushMessage(NetMsgType::GETSPORKS); //get current network sporks
        }

        //we always ask for sporks, so just skip this
        if(RequestedMasternodeAssets == MASTERNODE_SYNC_SPORKS){
            GetNextAsset();
            return;
        }

        if (pnode->nVersion >= mnpayments.GetMinMasternodePaymentsProto()) {

            // MODE : MASTERNODE_SYNC_LIST
            if(RequestedMasternodeAssets == MASTERNODE_SYNC_LIST) {

                // shall we move onto the next asset?
                if(nMnCount > mnodeman.GetEstimatedMasternodes(pCurrentBlockIndex->nHeight)*0.9)
                {
                    GetNextAsset();
                    return;
                }

                if(lastMasternodeList < GetTime() - MASTERNODE_SYNC_TIMEOUT){ //hasn't received a new item in the last five seconds, so we'll move to the
                    GetNextAsset();
                    return;
                }

                // requesting is the last thing we do (incase we needed to move to the next asset and we've requested from each peer already)

                if(pnode->HasFulfilledRequest("mnsync")) continue;
                pnode->FulfilledRequest("mnsync");

                //see if we've synced the masternode list
                /* note: Is this activing up? It's probably related to int CMasternodeMan::GetEstimatedMasternodes(int nBlock) */
 
                mnodeman.DsegUpdate(pnode);
                RequestedMasternodeAttempt++;

                return; //this will cause each peer to get one request each six seconds for the various assets we need
            }

            // MODE : MASTERNODE_SYNC_MNW
            if(RequestedMasternodeAssets == MASTERNODE_SYNC_MNW) {

                // Shall we move onto the next asset?
                // --
                // This might take a lot longer than 2 minutes due to new blocks, but that's OK. It will eventually time out if needed
                if(lastMasternodeWinner < GetTime() - MASTERNODE_SYNC_TIMEOUT){ //hasn't received a new item in the last five seconds, so we'll move to the
                    GetNextAsset();
                    return;
                }

                // target blocks count
                // we store nMnCount*1.25 payments blocks so nMnCount*1.2 should be enough most of the time
                if(mnpayments.GetBlockCount() > nMnCount*1.2)
                {   
                    // target votes, max ten per item. 6 average should be fine
                    if(mnpayments.GetVoteCount() > nMnCount*1.2*6)
                    {
                        GetNextAsset();
                        return;
                    }
                }

                // requesting is the last thing we do (incase we needed to move to the next asset and we've requested from each peer already)

                if(pnode->HasFulfilledRequest("mnwsync")) continue;
                pnode->FulfilledRequest("mnwsync");

                int nMnCount = mnodeman.CountEnabled();
                pnode->PushMessage(NetMsgType::MNWINNERSSYNC, nMnCount); //sync payees
                RequestedMasternodeAttempt++;


                return; //this will cause each peer to get one request each six seconds for the various assets we need
            }

            // MODE : MASTERNODE_SYNC_BUDGET
            if(RequestedMasternodeAssets == MASTERNODE_SYNC_BUDGET){
                // shall we move onto the next asset
                if(countBudgetItemProp > 0 && countBudgetItemFin)
                {
                    if(budget.CountProposalInventoryItems() >= (sumBudgetItemProp / countBudgetItemProp)*0.9)
                    {
                        if(budget.CountFinalizedInventoryItems() >= (sumBudgetItemFin / countBudgetItemFin)*0.9)
                        {
                            GetNextAsset();
                            return;
                        }
                    }
                }

                //we'll start rejecting votes if we accidentally get set as synced too soon, this allows plenty of time
                if(lastBudgetItem < GetTime() - MASTERNODE_SYNC_TIMEOUT){
                    GetNextAsset();

                    //try to activate our masternode if possible
                    activeMasternode.ManageStatus();
                    return;
                }

                // requesting is the last thing we do, incase we needed to move to the next asset and we've requested from each peer already

                if(pnode->HasFulfilledRequest("busync")) continue;
                pnode->FulfilledRequest("busync");

                uint256 n = uint256();
                pnode->PushMessage(NetMsgType::MNBUDGETVOTESYNC, n); //sync masternode votes
                RequestedMasternodeAttempt++;

                return; //this will cause each peer to get one request each six seconds for the various assets we need
            }

        }
    }
}

void CMasternodeSync::UpdatedBlockTip(const CBlockIndex *pindex)
{
    pCurrentBlockIndex = pindex;
}
