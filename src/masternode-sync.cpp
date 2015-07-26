// Copyright (c) 2014-2015 The Dash developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "main.h"
#include "masternode-sync.h"
#include "masternode-payments.h"
#include "masternode-budget.h"
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
    return RequestedMasternodeAssets == MASTERNODE_SYNC_FINISHED;
}

bool CMasternodeSync::IsListSyncStarted()
{
    return RequestedMasternodeAssets >= MASTERNODE_SYNC_LIST;
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
        case(MASTERNODE_SYNC_FAILED):
            lastMasternodeList = 0;
            lastMasternodeWinner = 0;
            lastBudgetItem = 0;
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
    static int tick = 0;

    if(tick++ % MASTERNODE_SYNC_TIMEOUT != 0) return;

    CBlockIndex* pindexPrev = chainActive.Tip();
    if(pindexPrev == NULL) return;

    if(IsSynced()) {
        /* 
            Resync if we lose all masternodes from sleep/wake or failure to sync originally
        */
        if(mnodeman.CountEnabled() == 0) {
            LOCK(cs_vNodes);
            BOOST_FOREACH(CNode* pnode, vNodes)
            {
                pnode->ClearFulfilledRequest("getspork");
                pnode->ClearFulfilledRequest("mnsync");
                pnode->ClearFulfilledRequest("mnwsync");
                pnode->ClearFulfilledRequest("busync");
            }

            RequestedMasternodeAssets = MASTERNODE_SYNC_INITIAL;
        } else
            return;
    }

    //try syncing again in an hour
    if(RequestedMasternodeAssets == MASTERNODE_SYNC_FAILED && lastFailure + (60*60) < GetTime()) {
        GetNextAsset();
    } else if (RequestedMasternodeAssets == MASTERNODE_SYNC_FAILED) {
        return;
    }

    if(fDebug) LogPrintf("CMasternodeSync::Process() - tick %d RequestedMasternodeAssets %d\n", tick, RequestedMasternodeAssets);

    if(RequestedMasternodeAssets == MASTERNODE_SYNC_INITIAL) GetNextAsset();

    LOCK(cs_vNodes);
    BOOST_FOREACH(CNode* pnode, vNodes)
    {
        if(Params().NetworkID() == CBaseChainParams::REGTEST){
            if(RequestedMasternodeAttempt <= 2) {
                pnode->PushMessage("getsporks"); //get current network sporks
            } else if(RequestedMasternodeAttempt < 4) {
                mnodeman.DsegUpdate(pnode); 
            } else if(RequestedMasternodeAttempt < 6) {
                int nMnCount = mnodeman.CountEnabled()*2;
                pnode->PushMessage("mnget", nMnCount); //sync payees
                uint256 n = 0;
                pnode->PushMessage("mnvs", n); //sync masternode votes
            } else {
                RequestedMasternodeAssets = MASTERNODE_SYNC_FINISHED;
            }
            RequestedMasternodeAttempt++;
            return;
        }

        //set to synced
        if(RequestedMasternodeAssets == MASTERNODE_SYNC_SPORKS){
            if(pnode->HasFulfilledRequest("getspork")) continue;
            pnode->FulfilledRequest("getspork");

            pnode->PushMessage("getsporks"); //get current network sporks
            if(RequestedMasternodeAttempt >= 2) GetNextAsset();
            RequestedMasternodeAttempt++;
            
            return;
        }

        if(IsInitialBlockDownload()) return;

        //don't begin syncing until we're almost at a recent block
        if(pindexPrev->nHeight + 4 < pindexBestHeader->nHeight || pindexPrev->nTime + 600 < GetTime()) return;

        if (pnode->nVersion >= masternodePayments.GetMinMasternodePaymentsProto()) {

            if(RequestedMasternodeAssets == MASTERNODE_SYNC_LIST) {
                if(fDebug) LogPrintf("CMasternodeSync::Process() - lastMasternodeList %lld (GetTime() - MASTERNODE_SYNC_TIMEOUT) %lld\n", lastMasternodeList, GetTime() - MASTERNODE_SYNC_TIMEOUT);
                if(lastMasternodeList > 0 && lastMasternodeList < GetTime() - MASTERNODE_SYNC_TIMEOUT){ //hasn't received a new item in the last five seconds, so we'll move to the
                    GetNextAsset();
                    return;
                }

                if(pnode->HasFulfilledRequest("mnsync")) continue;
                pnode->FulfilledRequest("mnsync");

                if((lastMasternodeList == 0 || lastMasternodeList > GetTime() - MASTERNODE_SYNC_TIMEOUT)
                        && RequestedMasternodeAttempt <= 4){
                    mnodeman.DsegUpdate(pnode);
                    RequestedMasternodeAttempt++;
                }
                return;
            }

            if(RequestedMasternodeAssets == MASTERNODE_SYNC_MNW) {
                if(lastMasternodeWinner > 0 && lastMasternodeWinner < GetTime() - MASTERNODE_SYNC_TIMEOUT){ //hasn't received a new item in the last five seconds, so we'll move to the
                    GetNextAsset();
                    return;
                }

                if(pnode->HasFulfilledRequest("mnwsync")) continue;
                pnode->FulfilledRequest("mnwsync");

                if((lastMasternodeWinner == 0 || lastMasternodeWinner > GetTime() - MASTERNODE_SYNC_TIMEOUT)
                        && RequestedMasternodeAttempt <= 4){

                    CBlockIndex* pindexPrev = chainActive.Tip();
                    if(pindexPrev == NULL) return;

                    int nMnCount = mnodeman.CountEnabled()*2;
                    pnode->PushMessage("mnget", nMnCount); //sync payees
                    RequestedMasternodeAttempt++;
                }
                return;
            }
        }

        if (pnode->nVersion >= MIN_BUDGET_PEER_PROTO_VERSION) {

            if(RequestedMasternodeAssets == MASTERNODE_SYNC_BUDGET){
                //we'll start rejecting votes if we accidentally get set as synced too soon
                if(lastBudgetItem > 0 && lastBudgetItem < GetTime() - MASTERNODE_SYNC_TIMEOUT*3){ //hasn't received a new item in the last five seconds, so we'll move to the
                    if(budget.HasNextFinalizedBudget()) {
                        GetNextAsset();
                    } else { //we've failed to sync, this state will reject the next budget block
                        LogPrintf("CMasternodeSync::Process - ERROR - Sync has failed, will retry later\n");
                        RequestedMasternodeAssets = MASTERNODE_SYNC_FAILED;
                        RequestedMasternodeAttempt = 0;
                        lastFailure = GetTime();
                    }
                    return;
                }

                if(pnode->HasFulfilledRequest("busync")) continue;
                pnode->FulfilledRequest("busync");

                if((lastBudgetItem == 0 || lastBudgetItem > GetTime() - MASTERNODE_SYNC_TIMEOUT)
                        && RequestedMasternodeAttempt <= 4){
                    uint256 n = 0;
                    pnode->PushMessage("mnvs", n); //sync masternode votes
                    RequestedMasternodeAttempt++;
                }
                return;
            }

        }
    }
}
