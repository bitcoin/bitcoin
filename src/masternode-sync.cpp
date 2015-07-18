// Copyright (c) 2014-2015 The Dash developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "main.h"
#include "masternode-sync.h"
#include "masternode-payments.h"
#include "masternode.h"
#include "masternodeman.h"
#include "util.h"
#include "addrman.h"

class CMasternodeSync;
CMasternodeSync masternodeSync;

CMasternodeSync::CMasternodeSync()
{
    lastMasternodeList = 0;
    lastMasternodeWinner = 0;
    lastBudgetItem = 0;
    RequestedMasternodeAssets = MASTERNODE_SYNC_INITIAL;
    RequestedMasternodeAttempt = 0;
}

bool CMasternodeSync::IsSynced()
{
    return (RequestedMasternodeAssets == MASTERNODE_SYNC_FINISHED);
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
        case(MASTERNODE_SYNC_INITIAL):
            RequestedMasternodeAssets = MASTERNODE_SYNC_SPORKS;
            break;
        case(MASTERNODE_SYNC_SPORKS):
            RequestedMasternodeAssets = MASTERNODE_SYNC_LIST;
            break;
        case(MASTERNODE_SYNC_LIST):
            RequestedMasternodeAssets = MASTERNODE_SYNC_MNW;
            break;
        case(MASTERNODE_SYNC_MNW):
            RequestedMasternodeAssets = MASTERNODE_SYNC_BUDGET;
            break;
        case(MASTERNODE_SYNC_BUDGET):
            LogPrintf("CMasternodeSync::GetNextAsset - Sync has finished\n");
            RequestedMasternodeAssets = MASTERNODE_SYNC_FINISHED;
            break;
    }
    RequestedMasternodeAttempt = 0;
}

void CMasternodeSync::Process()
{
    static int c = 0;

    if(IsSynced()) {
        /* 
            Resync if we lose all masternodes from sleep/wake or failure to sync originally
        */
        if(mnodeman.CountEnabled() == 0) {
            RequestedMasternodeAssets = MASTERNODE_SYNC_INITIAL;
            RequestedMasternodeAttempt = 0;
        }
        return;
    }

    if(c++ % MASTERNODE_SYNC_TIMEOUT != 0) return;

    if(fDebug) LogPrintf("CMasternodeSync::Process() - RequestedMasternodeAssets %d c %d\n", RequestedMasternodeAssets, c);

    //request full mn list only if Masternodes.dat was updated quite a long time ago
    if(RequestedMasternodeAssets == MASTERNODE_SYNC_INITIAL) GetNextAsset();

    CBlockIndex* pindexPrev = chainActive.Tip();
    if(pindexPrev == NULL) return;

    LOCK(cs_vNodes);
    BOOST_FOREACH(CNode* pnode, vNodes)
    {
        if (pnode->nVersion >= MIN_POOL_PEER_PROTO_VERSION)
        {
            //set to syned
            if(Params().NetworkID() == CBaseChainParams::REGTEST && c >= 10) {
                LogPrintf("CMasternodeSync::Process - Sync has finished\n");
                RequestedMasternodeAssets = MASTERNODE_SYNC_FINISHED;
                RequestedMasternodeAttempt = 0;
            }

            if(RequestedMasternodeAssets == MASTERNODE_SYNC_SPORKS){
                if(pnode->HasFulfilledRequest("getspork")) continue;
                pnode->FulfilledRequest("getspork");

                if(RequestedMasternodeAttempt <= 2){
                    pnode->PushMessage("getsporks"); //get current network sporks
                    if(RequestedMasternodeAttempt == 2) GetNextAsset();
                    RequestedMasternodeAttempt++;
                }
                return;
            }

            //don't begin syncing until we're at a recent block
            if(pindexPrev->nTime + 600 < GetTime()) return;

            if(RequestedMasternodeAssets == MASTERNODE_SYNC_LIST){
                if(lastMasternodeList > 0 && lastMasternodeList < GetTime() - MASTERNODE_SYNC_TIMEOUT){ //hasn't received a new item in the last five seconds, so we'll move to the
                    GetNextAsset();
                    return;
                }

                if(pnode->HasFulfilledRequest("mnsync")) continue;
                pnode->FulfilledRequest("mnsync");

                if((lastMasternodeList == 0 || lastMasternodeList > GetTime() - MASTERNODE_SYNC_TIMEOUT)
                        && RequestedMasternodeAttempt <= 2){
                    mnodeman.DsegUpdate(pnode);
                    RequestedMasternodeAttempt++;
                }
                return;
            }

            if(RequestedMasternodeAssets == MASTERNODE_SYNC_MNW){
                if(lastMasternodeWinner > 0 && lastMasternodeWinner < GetTime() - MASTERNODE_SYNC_TIMEOUT){ //hasn't received a new item in the last five seconds, so we'll move to the
                    GetNextAsset();
                    return;
                }

                if(pnode->HasFulfilledRequest("mnwsync")) continue;
                pnode->FulfilledRequest("mnwsync");

                if((lastMasternodeWinner == 0 || lastMasternodeWinner > GetTime() - MASTERNODE_SYNC_TIMEOUT)
                        && RequestedMasternodeAttempt <= 2){
                    pnode->PushMessage("mnget"); //sync payees
                    RequestedMasternodeAttempt++;
                }
                return;
            }

            if(RequestedMasternodeAssets == MASTERNODE_SYNC_BUDGET){
                if(lastBudgetItem > 0 && lastBudgetItem < GetTime() - MASTERNODE_SYNC_TIMEOUT){ //hasn't received a new item in the last five seconds, so we'll move to the
                    GetNextAsset();
                    return;
                }

                if(pnode->HasFulfilledRequest("busync")) continue;
                pnode->FulfilledRequest("busync");

                if((lastBudgetItem == 0 || lastBudgetItem > GetTime() - MASTERNODE_SYNC_TIMEOUT)
                        && RequestedMasternodeAttempt <= 2){
                    uint256 n = 0;

                    pnode->PushMessage("mnvs", n); //sync masternode votes
                    RequestedMasternodeAttempt++;
                }
                return;
            }

        }
    }
}
