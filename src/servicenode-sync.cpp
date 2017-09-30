// Copyright (c) 2014-2015 The Crown developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "main.h"
//#include "activeservicenode.h"
#include "servicenode-sync.h"
//#include "servicenode-payments.h"
//#include "servicenode-budget.h"
#include "servicenode.h"
#include "servicenodeman.h"
#include "spork.h"
#include "util.h"
#include "addrman.h"

class CServicenodeSync;
CServicenodeSync servicenodeSync;

CServicenodeSync::CServicenodeSync()
{
    Reset();
}

bool CServicenodeSync::IsSynced()
{
    return RequestedServicenodeAssets == SERVICENODE_SYNC_FINISHED;
}

bool CServicenodeSync::IsBlockchainSynced()
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

    TRY_LOCK(cs_main, lockMain);
    if(!lockMain) return false;

    CBlockIndex* pindex = chainActive.Tip();
    if(pindex == NULL) return false;


    if(pindex->nTime + 60*60 < GetTime())
        return false;

    fBlockchainSynced = true;

    return true;
}

void CServicenodeSync::Reset()
{   
    lastServicenodeList = 0;
    lastServicenodeWinner = 0;
    lastBudgetItem = 0;
    mapSeenSyncMNB.clear();
    mapSeenSyncMNW.clear();
    mapSeenSyncBudget.clear();
    lastFailure = 0;
    nCountFailures = 0;
    sumServicenodeList = 0;
    sumServicenodeWinner = 0;
    sumBudgetItemProp = 0;
    sumBudgetItemFin = 0;
    countServicenodeList = 0;
    countServicenodeWinner = 0;
    countBudgetItemProp = 0;
    countBudgetItemFin = 0;
    RequestedServicenodeAssets = SERVICENODE_SYNC_INITIAL;
    RequestedServicenodeAttempt = 0;
    nAssetSyncStarted = GetTime();
}

void CServicenodeSync::AddedServicenodeList(uint256 hash)
{
    if(snodeman.mapSeenServicenodeBroadcast.count(hash)) {
        if(mapSeenSyncMNB[hash] < SERVICENODE_SYNC_THRESHOLD) {
            lastServicenodeList = GetTime();
            mapSeenSyncMNB[hash]++;
        }
    } else {
        lastServicenodeList = GetTime();
        mapSeenSyncMNB.insert(make_pair(hash, 1));
    }
}

void CServicenodeSync::AddedServicenodeWinner(uint256 hash)
{
    //if(servicenodePayments.mapServicenodePayeeVotes.count(hash)) {
    //    if(mapSeenSyncMNW[hash] < SERVICENODE_SYNC_THRESHOLD) {
    //        lastServicenodeWinner = GetTime();
    //        mapSeenSyncMNW[hash]++;
    //    }
    //} else {
    //    lastServicenodeWinner = GetTime();
    //    mapSeenSyncMNW.insert(make_pair(hash, 1));
    //}
}

void CServicenodeSync::AddedBudgetItem(uint256 hash)
{
    //if(budget.mapSeenServicenodeBudgetProposals.count(hash) || budget.mapSeenServicenodeBudgetVotes.count(hash) ||
    //        budget.mapSeenFinalizedBudgets.count(hash) || budget.mapSeenFinalizedBudgetVotes.count(hash)) {
    //    if(mapSeenSyncBudget[hash] < SERVICENODE_SYNC_THRESHOLD) {
    //        lastBudgetItem = GetTime();
    //        mapSeenSyncBudget[hash]++;
    //    }
    //} else {
    //    lastBudgetItem = GetTime();
    //    mapSeenSyncBudget.insert(make_pair(hash, 1));
    //}
}

bool CServicenodeSync::IsBudgetPropEmpty()
{
    return sumBudgetItemProp==0 && countBudgetItemProp>0;
}

bool CServicenodeSync::IsBudgetFinEmpty()
{
    return sumBudgetItemFin==0 && countBudgetItemFin>0;
}

void CServicenodeSync::GetNextAsset()
{
    switch(RequestedServicenodeAssets)
    {
        case(SERVICENODE_SYNC_INITIAL):
        case(SERVICENODE_SYNC_FAILED): // should never be used here actually, use Reset() instead
            ClearFulfilledRequest();
            RequestedServicenodeAssets = SERVICENODE_SYNC_SPORKS;
            break;
        case(SERVICENODE_SYNC_SPORKS):
            RequestedServicenodeAssets = SERVICENODE_SYNC_LIST;
            break;
        case(SERVICENODE_SYNC_LIST):
            RequestedServicenodeAssets = SERVICENODE_SYNC_MNW;
            break;
        case(SERVICENODE_SYNC_MNW):
            RequestedServicenodeAssets = SERVICENODE_SYNC_BUDGET;
            break;
        case(SERVICENODE_SYNC_BUDGET):
            LogPrintf("CServicenodeSync::GetNextAsset - Sync has finished\n");
            RequestedServicenodeAssets = SERVICENODE_SYNC_FINISHED;
            break;
    }
    RequestedServicenodeAttempt = 0;
    nAssetSyncStarted = GetTime();
}

std::string CServicenodeSync::GetSyncStatus()
{
    switch (servicenodeSync.RequestedServicenodeAssets) {
        case SERVICENODE_SYNC_INITIAL: return _("Synchronization pending...");
        case SERVICENODE_SYNC_SPORKS: return _("Synchronizing sporks...");
        case SERVICENODE_SYNC_LIST: return _("Synchronizing servicenodes...");
        case SERVICENODE_SYNC_MNW: return _("Synchronizing servicenode winners...");
        case SERVICENODE_SYNC_BUDGET: return _("Synchronizing budgets...");
        case SERVICENODE_SYNC_FAILED: return _("Synchronization failed");
        case SERVICENODE_SYNC_FINISHED: return _("Synchronization finished");
    }
    return "";
}

void CServicenodeSync::ProcessMessage(CNode* pfrom, std::string& strCommand, CDataStream& vRecv)
{
    if (strCommand == "ssc") { //Sync status count
        int nItemID;
        int nCount;
        vRecv >> nItemID >> nCount;

        if(RequestedServicenodeAssets >= SERVICENODE_SYNC_FINISHED) return;

        //this means we will receive no further communication
        switch(nItemID)
        {
            case(SERVICENODE_SYNC_LIST):
                if(nItemID != RequestedServicenodeAssets) return;
                sumServicenodeList += nCount;
                countServicenodeList++;
                break;
            case(SERVICENODE_SYNC_MNW):
                if(nItemID != RequestedServicenodeAssets) return;
                sumServicenodeWinner += nCount;
                countServicenodeWinner++;
                break;
            case(SERVICENODE_SYNC_BUDGET_PROP):
                if(RequestedServicenodeAssets != SERVICENODE_SYNC_BUDGET) return;
                sumBudgetItemProp += nCount;
                countBudgetItemProp++;
                break;
            case(SERVICENODE_SYNC_BUDGET_FIN):
                if(RequestedServicenodeAssets != SERVICENODE_SYNC_BUDGET) return;
                sumBudgetItemFin += nCount;
                countBudgetItemFin++;
                break;
        }
        
        LogPrintf("CServicenodeSync:ProcessMessage - ssc - got inventory count %d %d\n", nItemID, nCount);
    }
}

void CServicenodeSync::ClearFulfilledRequest()
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

void CServicenodeSync::Process()
{
    static int tick = 0;

    if(tick++ % SERVICENODE_SYNC_TIMEOUT != 0) return;

    if(IsSynced()) {
        /* 
            Resync if we lose all servicenodes from sleep/wake or failure to sync originally
        */
        if(snodeman.CountEnabled() == 0) {
            Reset();
        } else
            return;
    }

    //try syncing again
    if(RequestedServicenodeAssets == SERVICENODE_SYNC_FAILED && lastFailure + (1*60) < GetTime()) {
        Reset();
    } else if (RequestedServicenodeAssets == SERVICENODE_SYNC_FAILED) {
        return;
    }

    if(fDebug) LogPrintf("CServicenodeSync::Process() - tick %d RequestedServicenodeAssets %d\n", tick, RequestedServicenodeAssets);

    if(RequestedServicenodeAssets == SERVICENODE_SYNC_INITIAL) GetNextAsset();

    // sporks synced but blockchain is not, wait until we're almost at a recent block to continue
    if(Params().NetworkID() != CBaseChainParams::REGTEST &&
            !IsBlockchainSynced() && RequestedServicenodeAssets > SERVICENODE_SYNC_SPORKS) return;

    TRY_LOCK(cs_vNodes, lockRecv);
    if(!lockRecv) return;

    BOOST_FOREACH(CNode* pnode, vNodes)
    {
        if(Params().NetworkID() == CBaseChainParams::REGTEST){
            if(RequestedServicenodeAttempt <= 2) {
                pnode->PushMessage("getsporks"); //get current network sporks
            } else if(RequestedServicenodeAttempt < 4) {
                snodeman.DsegUpdate(pnode); 
            } else if(RequestedServicenodeAttempt < 6) {
                int nMnCount = snodeman.CountEnabled();
                pnode->PushMessage("mnget", nMnCount); //sync payees
                uint256 n = uint256();
                pnode->PushMessage("mnvs", n); //sync servicenode votes
            } else {
                RequestedServicenodeAssets = SERVICENODE_SYNC_FINISHED;
            }
            RequestedServicenodeAttempt++;
            return;
        }

        //set to synced
        if(RequestedServicenodeAssets == SERVICENODE_SYNC_SPORKS){
            if(pnode->HasFulfilledRequest("getspork")) continue;
            pnode->FulfilledRequest("getspork");

            pnode->PushMessage("getsporks"); //get current network sporks
            if(RequestedServicenodeAttempt >= 2) GetNextAsset();
            RequestedServicenodeAttempt++;
            
            return;
        }

        //if (pnode->nVersion >= servicenodePayments.GetMinServicenodePaymentsProto()) {

        //    if(RequestedServicenodeAssets == SERVICENODE_SYNC_LIST) {
        //        if(fDebug) LogPrintf("CServicenodeSync::Process() - lastServicenodeList %lld (GetTime() - SERVICENODE_SYNC_TIMEOUT) %lld\n", lastServicenodeList, GetTime() - SERVICENODE_SYNC_TIMEOUT);
        //        if(lastServicenodeList > 0 && lastServicenodeList < GetTime() - SERVICENODE_SYNC_TIMEOUT*2 && RequestedServicenodeAttempt >= SERVICENODE_SYNC_THRESHOLD){ //hasn't received a new item in the last five seconds, so we'll move to the
        //            GetNextAsset();
        //            return;
        //        }

        //        if(pnode->HasFulfilledRequest("mnsync")) continue;
        //        pnode->FulfilledRequest("mnsync");

        //        // timeout
        //        if(lastServicenodeList == 0 &&
        //        (RequestedServicenodeAttempt >= SERVICENODE_SYNC_THRESHOLD*3 || GetTime() - nAssetSyncStarted > SERVICENODE_SYNC_TIMEOUT*5)) {
        //            if(IsSporkActive(SPORK_8_SERVICENODE_PAYMENT_ENFORCEMENT)) {
        //                LogPrintf("CServicenodeSync::Process - ERROR - Sync has failed, will retry later\n");
        //                RequestedServicenodeAssets = SERVICENODE_SYNC_FAILED;
        //                RequestedServicenodeAttempt = 0;
        //                lastFailure = GetTime();
        //                nCountFailures++;
        //            } else {
        //                GetNextAsset();
        //            }
        //            return;
        //        }

        //        if(RequestedServicenodeAttempt >= SERVICENODE_SYNC_THRESHOLD*3) return;

        //        snodeman.DsegUpdate(pnode);
        //        RequestedServicenodeAttempt++;
        //        return;
        //    }

        //    if(RequestedServicenodeAssets == SERVICENODE_SYNC_MNW) {
        //        if(lastServicenodeWinner > 0 && lastServicenodeWinner < GetTime() - SERVICENODE_SYNC_TIMEOUT*2 && RequestedServicenodeAttempt >= SERVICENODE_SYNC_THRESHOLD){ //hasn't received a new item in the last five seconds, so we'll move to the
        //            GetNextAsset();
        //            return;
        //        }

        //        if(pnode->HasFulfilledRequest("mnwsync")) continue;
        //        pnode->FulfilledRequest("mnwsync");

        //        // timeout
        //        if(lastServicenodeWinner == 0 &&
        //        (RequestedServicenodeAttempt >= SERVICENODE_SYNC_THRESHOLD*3 || GetTime() - nAssetSyncStarted > SERVICENODE_SYNC_TIMEOUT*5)) {
        //            if(IsSporkActive(SPORK_8_SERVICENODE_PAYMENT_ENFORCEMENT)) {
        //                LogPrintf("CServicenodeSync::Process - ERROR - Sync has failed, will retry later\n");
        //                RequestedServicenodeAssets = SERVICENODE_SYNC_FAILED;
        //                RequestedServicenodeAttempt = 0;
        //                lastFailure = GetTime();
        //                nCountFailures++;
        //            } else {
        //                GetNextAsset();
        //            }
        //            return;
        //        }

        //        if(RequestedServicenodeAttempt >= SERVICENODE_SYNC_THRESHOLD*3) return;

        //        CBlockIndex* pindexPrev = chainActive.Tip();
        //        if(pindexPrev == NULL) return;

        //        int nMnCount = snodeman.CountEnabled();
        //        pnode->PushMessage("mnget", nMnCount); //sync payees
        //        RequestedServicenodeAttempt++;

        //        return;
        //    }
        //}

        if (pnode->nVersion >= MIN_BUDGET_PEER_PROTO_VERSION) {

            if(RequestedServicenodeAssets == SERVICENODE_SYNC_BUDGET){
                //we'll start rejecting votes if we accidentally get set as synced too soon
                if(lastBudgetItem > 0 && lastBudgetItem < GetTime() - SERVICENODE_SYNC_TIMEOUT*2 && RequestedServicenodeAttempt >= SERVICENODE_SYNC_THRESHOLD){ //hasn't received a new item in the last five seconds, so we'll move to the
                    //LogPrintf("CServicenodeSync::Process - HasNextFinalizedBudget %d nCountFailures %d IsBudgetPropEmpty %d\n", budget.HasNextFinalizedBudget(), nCountFailures, IsBudgetPropEmpty());
                    //if(budget.HasNextFinalizedBudget() || nCountFailures >= 2 || IsBudgetPropEmpty()) {
                        GetNextAsset();

                        //try to activate our servicenode if possible
                        // TODO uncomment later
                        //activeServicenode.ManageStatus();
                    // } else { //we've failed to sync, this state will reject the next budget block
                    //     LogPrintf("CServicenodeSync::Process - ERROR - Sync has failed, will retry later\n");
                    //     RequestedServicenodeAssets = SERVICENODE_SYNC_FAILED;
                    //     RequestedServicenodeAttempt = 0;
                    //     lastFailure = GetTime();
                    //     nCountFailures++;
                    // }
                    return;
                }

                // timeout
                if(lastBudgetItem == 0 &&
                (RequestedServicenodeAttempt >= SERVICENODE_SYNC_THRESHOLD*3 || GetTime() - nAssetSyncStarted > SERVICENODE_SYNC_TIMEOUT*5)) {
                    // maybe there is no budgets at all, so just finish syncing
                    GetNextAsset();
                    //activeServicenode.ManageStatus();
                    return;
                }

                if(pnode->HasFulfilledRequest("busync")) continue;
                pnode->FulfilledRequest("busync");

                if(RequestedServicenodeAttempt >= SERVICENODE_SYNC_THRESHOLD*3) return;

                uint256 n = uint256();
                pnode->PushMessage("mnvs", n); //sync servicenode votes
                RequestedServicenodeAttempt++;
                
                return;
            }

        }
    }
}
