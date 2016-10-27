// Copyright (c) 2014-2015 The Crown developers
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

class CThroneSync;
CThroneSync masternodeSync;

CThroneSync::CThroneSync()
{
    Reset();
}

bool CThroneSync::IsSynced()
{
    return RequestedThroneAssets == THRONE_SYNC_FINISHED;
}

bool CThroneSync::IsBlockchainSynced()
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

void CThroneSync::Reset()
{   
    lastThroneList = 0;
    lastThroneWinner = 0;
    lastBudgetItem = 0;
    mapSeenSyncMNB.clear();
    mapSeenSyncMNW.clear();
    mapSeenSyncBudget.clear();
    lastFailure = 0;
    nCountFailures = 0;
    sumThroneList = 0;
    sumThroneWinner = 0;
    sumBudgetItemProp = 0;
    sumBudgetItemFin = 0;
    countThroneList = 0;
    countThroneWinner = 0;
    countBudgetItemProp = 0;
    countBudgetItemFin = 0;
    RequestedThroneAssets = THRONE_SYNC_INITIAL;
    RequestedThroneAttempt = 0;
    nAssetSyncStarted = GetTime();
}

void CThroneSync::AddedThroneList(uint256 hash)
{
    if(mnodeman.mapSeenThroneBroadcast.count(hash)) {
        if(mapSeenSyncMNB[hash] < THRONE_SYNC_THRESHOLD) {
            lastThroneList = GetTime();
            mapSeenSyncMNB[hash]++;
        }
    } else {
        lastThroneList = GetTime();
        mapSeenSyncMNB.insert(make_pair(hash, 1));
    }
}

void CThroneSync::AddedThroneWinner(uint256 hash)
{
    if(masternodePayments.mapThronePayeeVotes.count(hash)) {
        if(mapSeenSyncMNW[hash] < THRONE_SYNC_THRESHOLD) {
            lastThroneWinner = GetTime();
            mapSeenSyncMNW[hash]++;
        }
    } else {
        lastThroneWinner = GetTime();
        mapSeenSyncMNW.insert(make_pair(hash, 1));
    }
}

void CThroneSync::AddedBudgetItem(uint256 hash)
{
    if(budget.mapSeenThroneBudgetProposals.count(hash) || budget.mapSeenThroneBudgetVotes.count(hash) ||
            budget.mapSeenFinalizedBudgets.count(hash) || budget.mapSeenFinalizedBudgetVotes.count(hash)) {
        if(mapSeenSyncBudget[hash] < THRONE_SYNC_THRESHOLD) {
            lastBudgetItem = GetTime();
            mapSeenSyncBudget[hash]++;
        }
    } else {
        lastBudgetItem = GetTime();
        mapSeenSyncBudget.insert(make_pair(hash, 1));
    }
}

bool CThroneSync::IsBudgetPropEmpty()
{
    return sumBudgetItemProp==0 && countBudgetItemProp>0;
}

bool CThroneSync::IsBudgetFinEmpty()
{
    return sumBudgetItemFin==0 && countBudgetItemFin>0;
}

void CThroneSync::GetNextAsset()
{
    switch(RequestedThroneAssets)
    {
        case(THRONE_SYNC_INITIAL):
        case(THRONE_SYNC_FAILED): // should never be used here actually, use Reset() instead
            ClearFulfilledRequest();
            RequestedThroneAssets = THRONE_SYNC_SPORKS;
            break;
        case(THRONE_SYNC_SPORKS):
            RequestedThroneAssets = THRONE_SYNC_LIST;
            break;
        case(THRONE_SYNC_LIST):
            RequestedThroneAssets = THRONE_SYNC_MNW;
            break;
        case(THRONE_SYNC_MNW):
            RequestedThroneAssets = THRONE_SYNC_BUDGET;
            break;
        case(THRONE_SYNC_BUDGET):
            LogPrintf("CThroneSync::GetNextAsset - Sync has finished\n");
            RequestedThroneAssets = THRONE_SYNC_FINISHED;
            break;
    }
    RequestedThroneAttempt = 0;
    nAssetSyncStarted = GetTime();
}

std::string CThroneSync::GetSyncStatus()
{
    switch (masternodeSync.RequestedThroneAssets) {
        case THRONE_SYNC_INITIAL: return _("Synchronization pending...");
        case THRONE_SYNC_SPORKS: return _("Synchronizing sporks...");
        case THRONE_SYNC_LIST: return _("Synchronizing masternodes...");
        case THRONE_SYNC_MNW: return _("Synchronizing masternode winners...");
        case THRONE_SYNC_BUDGET: return _("Synchronizing budgets...");
        case THRONE_SYNC_FAILED: return _("Synchronization failed");
        case THRONE_SYNC_FINISHED: return _("Synchronization finished");
    }
    return "";
}

void CThroneSync::ProcessMessage(CNode* pfrom, std::string& strCommand, CDataStream& vRecv)
{
    if (strCommand == "ssc") { //Sync status count
        int nItemID;
        int nCount;
        vRecv >> nItemID >> nCount;

        if(RequestedThroneAssets >= THRONE_SYNC_FINISHED) return;

        //this means we will receive no further communication
        switch(nItemID)
        {
            case(THRONE_SYNC_LIST):
                if(nItemID != RequestedThroneAssets) return;
                sumThroneList += nCount;
                countThroneList++;
                break;
            case(THRONE_SYNC_MNW):
                if(nItemID != RequestedThroneAssets) return;
                sumThroneWinner += nCount;
                countThroneWinner++;
                break;
            case(THRONE_SYNC_BUDGET_PROP):
                if(RequestedThroneAssets != THRONE_SYNC_BUDGET) return;
                sumBudgetItemProp += nCount;
                countBudgetItemProp++;
                break;
            case(THRONE_SYNC_BUDGET_FIN):
                if(RequestedThroneAssets != THRONE_SYNC_BUDGET) return;
                sumBudgetItemFin += nCount;
                countBudgetItemFin++;
                break;
        }
        
        LogPrintf("CThroneSync:ProcessMessage - ssc - got inventory count %d %d\n", nItemID, nCount);
    }
}

void CThroneSync::ClearFulfilledRequest()
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

void CThroneSync::Process()
{
    static int tick = 0;

    if(tick++ % THRONE_SYNC_TIMEOUT != 0) return;

    if(IsSynced()) {
        /* 
            Resync if we lose all masternodes from sleep/wake or failure to sync originally
        */
        if(mnodeman.CountEnabled() == 0) {
            Reset();
        } else
            return;
    }

    //try syncing again
    if(RequestedThroneAssets == THRONE_SYNC_FAILED && lastFailure + (1*60) < GetTime()) {
        Reset();
    } else if (RequestedThroneAssets == THRONE_SYNC_FAILED) {
        return;
    }

    if(fDebug) LogPrintf("CThroneSync::Process() - tick %d RequestedThroneAssets %d\n", tick, RequestedThroneAssets);

    if(RequestedThroneAssets == THRONE_SYNC_INITIAL) GetNextAsset();

    // sporks synced but blockchain is not, wait until we're almost at a recent block to continue
    if(Params().NetworkID() != CBaseChainParams::REGTEST &&
            !IsBlockchainSynced() && RequestedThroneAssets > THRONE_SYNC_SPORKS) return;

    TRY_LOCK(cs_vNodes, lockRecv);
    if(!lockRecv) return;

    BOOST_FOREACH(CNode* pnode, vNodes)
    {
        if(Params().NetworkID() == CBaseChainParams::REGTEST){
            if(RequestedThroneAttempt <= 2) {
                pnode->PushMessage("getsporks"); //get current network sporks
            } else if(RequestedThroneAttempt < 4) {
                mnodeman.DsegUpdate(pnode); 
            } else if(RequestedThroneAttempt < 6) {
                int nMnCount = mnodeman.CountEnabled();
                pnode->PushMessage("mnget", nMnCount); //sync payees
                uint256 n = 0;
                pnode->PushMessage("mnvs", n); //sync masternode votes
            } else {
                RequestedThroneAssets = THRONE_SYNC_FINISHED;
            }
            RequestedThroneAttempt++;
            return;
        }

        //set to synced
        if(RequestedThroneAssets == THRONE_SYNC_SPORKS){
            if(pnode->HasFulfilledRequest("getspork")) continue;
            pnode->FulfilledRequest("getspork");

            pnode->PushMessage("getsporks"); //get current network sporks
            if(RequestedThroneAttempt >= 2) GetNextAsset();
            RequestedThroneAttempt++;
            
            return;
        }

        if (pnode->nVersion >= masternodePayments.GetMinThronePaymentsProto()) {

            if(RequestedThroneAssets == THRONE_SYNC_LIST) {
                if(fDebug) LogPrintf("CThroneSync::Process() - lastThroneList %lld (GetTime() - THRONE_SYNC_TIMEOUT) %lld\n", lastThroneList, GetTime() - THRONE_SYNC_TIMEOUT);
                if(lastThroneList > 0 && lastThroneList < GetTime() - THRONE_SYNC_TIMEOUT*2 && RequestedThroneAttempt >= THRONE_SYNC_THRESHOLD){ //hasn't received a new item in the last five seconds, so we'll move to the
                    GetNextAsset();
                    return;
                }

                if(pnode->HasFulfilledRequest("mnsync")) continue;
                pnode->FulfilledRequest("mnsync");

                // timeout
                if(lastThroneList == 0 &&
                (RequestedThroneAttempt >= THRONE_SYNC_THRESHOLD*3 || GetTime() - nAssetSyncStarted > THRONE_SYNC_TIMEOUT*5)) {
                    if(IsSporkActive(SPORK_8_THRONE_PAYMENT_ENFORCEMENT)) {
                        LogPrintf("CThroneSync::Process - ERROR - Sync has failed, will retry later\n");
                        RequestedThroneAssets = THRONE_SYNC_FAILED;
                        RequestedThroneAttempt = 0;
                        lastFailure = GetTime();
                        nCountFailures++;
                    } else {
                        GetNextAsset();
                    }
                    return;
                }

                if(RequestedThroneAttempt >= THRONE_SYNC_THRESHOLD*3) return;

                mnodeman.DsegUpdate(pnode);
                RequestedThroneAttempt++;
                return;
            }

            if(RequestedThroneAssets == THRONE_SYNC_MNW) {
                if(lastThroneWinner > 0 && lastThroneWinner < GetTime() - THRONE_SYNC_TIMEOUT*2 && RequestedThroneAttempt >= THRONE_SYNC_THRESHOLD){ //hasn't received a new item in the last five seconds, so we'll move to the
                    GetNextAsset();
                    return;
                }

                if(pnode->HasFulfilledRequest("mnwsync")) continue;
                pnode->FulfilledRequest("mnwsync");

                // timeout
                if(lastThroneWinner == 0 &&
                (RequestedThroneAttempt >= THRONE_SYNC_THRESHOLD*3 || GetTime() - nAssetSyncStarted > THRONE_SYNC_TIMEOUT*5)) {
                    if(IsSporkActive(SPORK_8_THRONE_PAYMENT_ENFORCEMENT)) {
                        LogPrintf("CThroneSync::Process - ERROR - Sync has failed, will retry later\n");
                        RequestedThroneAssets = THRONE_SYNC_FAILED;
                        RequestedThroneAttempt = 0;
                        lastFailure = GetTime();
                        nCountFailures++;
                    } else {
                        GetNextAsset();
                    }
                    return;
                }

                if(RequestedThroneAttempt >= THRONE_SYNC_THRESHOLD*3) return;

                CBlockIndex* pindexPrev = chainActive.Tip();
                if(pindexPrev == NULL) return;

                int nMnCount = mnodeman.CountEnabled();
                pnode->PushMessage("mnget", nMnCount); //sync payees
                RequestedThroneAttempt++;

                return;
            }
        }

        if (pnode->nVersion >= MIN_BUDGET_PEER_PROTO_VERSION) {

            if(RequestedThroneAssets == THRONE_SYNC_BUDGET){
                //we'll start rejecting votes if we accidentally get set as synced too soon
                if(lastBudgetItem > 0 && lastBudgetItem < GetTime() - THRONE_SYNC_TIMEOUT*2 && RequestedThroneAttempt >= THRONE_SYNC_THRESHOLD){ //hasn't received a new item in the last five seconds, so we'll move to the
                    //LogPrintf("CThroneSync::Process - HasNextFinalizedBudget %d nCountFailures %d IsBudgetPropEmpty %d\n", budget.HasNextFinalizedBudget(), nCountFailures, IsBudgetPropEmpty());
                    //if(budget.HasNextFinalizedBudget() || nCountFailures >= 2 || IsBudgetPropEmpty()) {
                        GetNextAsset();

                        //try to activate our masternode if possible
                        activeThrone.ManageStatus();
                    // } else { //we've failed to sync, this state will reject the next budget block
                    //     LogPrintf("CThroneSync::Process - ERROR - Sync has failed, will retry later\n");
                    //     RequestedThroneAssets = THRONE_SYNC_FAILED;
                    //     RequestedThroneAttempt = 0;
                    //     lastFailure = GetTime();
                    //     nCountFailures++;
                    // }
                    return;
                }

                // timeout
                if(lastBudgetItem == 0 &&
                (RequestedThroneAttempt >= THRONE_SYNC_THRESHOLD*3 || GetTime() - nAssetSyncStarted > THRONE_SYNC_TIMEOUT*5)) {
                    // maybe there is no budgets at all, so just finish syncing
                    GetNextAsset();
                    activeThrone.ManageStatus();
                    return;
                }

                if(pnode->HasFulfilledRequest("busync")) continue;
                pnode->FulfilledRequest("busync");

                if(RequestedThroneAttempt >= THRONE_SYNC_THRESHOLD*3) return;

                uint256 n = 0;
                pnode->PushMessage("mnvs", n); //sync masternode votes
                RequestedThroneAttempt++;
                
                return;
            }

        }
    }
}
