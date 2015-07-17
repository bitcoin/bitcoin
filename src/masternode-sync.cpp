// Copyright (c) 2014-2015 The Dash developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "main.h"
#include "init.h"

#include "masternode-sync.h"
#include "masternode.h"
#include "util.h"
#include "addrman.h"

class CMasternodeSync;
CMasternodeSync masternodeSync;

CMasternodeSync::CMasternodeSync()
{
    c = 0;
    lastMasternodeList = 0;
    lastMasternodeWinner = 0;
    lastBudgetItem = 0;
    RequestedMasternodeAssets = 0;
    RequestedMasternodeAttempt = 0;
}

bool CMasternodeSync::IsSynced()
{
    return (RequestedMasternodeAssets == MASTERNODE_LIST_SYNCED);
}

void CMasternodeSync::AddedMasternodeList()
{
    lastMasternodeList = GetTime();
}

void CMasternodeSync::AddedMasternodeWinner()
{
    lastMasternodeWinner = GetTime();
}

void CMasternodeSync::AddedBudgetItem()
{
    lastBudgetItem = GetTime();
}

void CMasternodeSync::GetNextAsset()
{
    switch(RequestedMasternodeAssets)
    {
        case(MASTERNODE_INITIAL):
            RequestedMasternodeAssets = MASTERNODE_SPORK_SETTINGS;
            break;
        case(MASTERNODE_SPORK_SETTINGS):
            RequestedMasternodeAssets = MASTERNODE_SYNC_LIST;
            break;
        case(MASTERNODE_SYNC_LIST):
            RequestedMasternodeAssets = MASTERNODE_SYNC_MNW;
            break;
        case(MASTERNODE_SYNC_MNW):
            RequestedMasternodeAssets = MASTERNODE_SYNC_BUDGET;
            break;
        case(MASTERNODE_SYNC_BUDGET):
            RequestedMasternodeAssets = MASTERNODE_LIST_SYNCED;
            break;
    }
}

void CMasternodeSync::Process() 
{
    if(IsSynced()) return;

    //request full mn list only if Masternodes.dat was updated quite a long time ago
    if(RequestedMasternodeAssets == MASTERNODE_INITIAL) GetNextAsset();

    CBlockIndex* pindexPrev = chainActive.Tip();
    if(pindexPrev == NULL) return;

    //try to sync the Masternode list and payment list every 5 seconds from at least 3 nodes
    if(c % 5 == 0){
        LOCK(cs_vNodes);
        BOOST_FOREACH(CNode* pnode, vNodes)
        {
            if (pnode->nVersion >= MIN_POOL_PEER_PROTO_VERSION) 
            {

                if(RequestedMasternodeAssets == MASTERNODE_SPORK_SETTINGS){
                    if(pnode->HasFulfilledRequest("getspork")) continue;
                    pnode->FulfilledRequest("getspork");

                    if(RequestedMasternodeAttempt <= 2){
                        pnode->PushMessage("getsporks"); //get current network sporks
                        if(RequestedMasternodeAttempt == 2) GetNextAsset();
                        RequestedMasternodeAttempt++;
                    }
                    return;
                }

                if(IsInitialBlockDownload()) return;

                if(RequestedMasternodeAssets == MASTERNODE_SYNC_LIST){
                    if(lastMasternodeList > 0 && lastMasternodeList < GetTime() - 5){ //hasn't received a new item in the last five seconds, so we'll move to the 
                        GetNextAsset();
                        return;
                    }

                    if(pnode->HasFulfilledRequest("mnsync")) continue;
                    pnode->FulfilledRequest("mnsync");

                    if((lastMasternodeList == 0 || lastMasternodeList > GetTime() - 5) && RequestedMasternodeAttempt <= 4){
                        mnodeman.DsegUpdate(pnode);
                        pnode->PushMessage("getsporks"); //get current network sporks
                        AddedMasternodeList();
                        RequestedMasternodeAttempt++;
                    }
                    return;
                }

                if(RequestedMasternodeAssets == MASTERNODE_SYNC_MNW){
                    if(lastMasternodeWinner > 0 && lastMasternodeWinner < GetTime() - 5){ //hasn't received a new item in the last five seconds, so we'll move to the 
                        GetNextAsset();
                        return;
                    }

                    if(pnode->HasFulfilledRequest("mnwsync")) continue;
                    pnode->FulfilledRequest("mnwsync");

                    if((lastMasternodeWinner == 0 || lastMasternodeWinner > GetTime() - 5) && RequestedMasternodeAttempt <= 6){
                        pnode->PushMessage("mnget"); //sync payees
                        AddedMasternodeWinner();
                        RequestedMasternodeAttempt++;
                    }
                    return;
                }
                
                if(RequestedMasternodeAssets == MASTERNODE_SYNC_BUDGET){
                    if(lastBudgetItem > 0 && lastBudgetItem < GetTime() - 5){ //hasn't received a new item in the last five seconds, so we'll move to the 
                        GetNextAsset();
                        return;
                    }

                    if(pnode->HasFulfilledRequest("busync")) continue;
                    pnode->FulfilledRequest("busync");

                    if((lastBudgetItem == 0 || lastBudgetItem > GetTime() - 5) && RequestedMasternodeAttempt <= 8){
                        uint256 n = 0;

                        pnode->PushMessage("mnvs", n); //sync masternode votes
                        AddedBudgetItem();
                        RequestedMasternodeAttempt++;
                    }
                    return;
                }

            }
        }
    }
    c++;
}