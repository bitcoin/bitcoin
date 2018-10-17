// Copyright (c) 2014-2015 The Dash developers
// Copyright (c) 2014-2018 The Crown developers
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

    TRY_LOCK(cs_main, lockMain);
    if(!lockMain) return false;

    CBlockIndex* pindex = chainActive.Tip();
    if(pindex == NULL) return false;


    if(!GetBoolArg("-jumpstart", false) && pindex->nTime + 60*60 < GetTime())
        return false;

    fBlockchainSynced = true;

    return true;
}

void CMasternodeSync::Reset()
{   
    lastMasternodeList = 0;
    lastMasternodeWinner = 0;
    lastBudgetItem = 0;
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
        if(mapSeenSyncMNB[hash] < MASTERNODE_SYNC_THRESHOLD) {
            lastMasternodeList = GetTime();
            mapSeenSyncMNB[hash]++;
        }
    } else {
        lastMasternodeList = GetTime();
        mapSeenSyncMNB.insert(make_pair(hash, 1));
    }
}

void CMasternodeSync::AddedMasternodeWinner(uint256 hash)
{
    if(masternodePayments.mapMasternodePayeeVotes.count(hash)) {
        if(mapSeenSyncMNW[hash] < MASTERNODE_SYNC_THRESHOLD) {
            lastMasternodeWinner = GetTime();
            mapSeenSyncMNW[hash]++;
        }
    } else {
        lastMasternodeWinner = GetTime();
        mapSeenSyncMNW.insert(make_pair(hash, 1));
    }
}

void CMasternodeSync::AddedBudgetItem(uint256 hash)
{
    if(budget.mapSeenMasternodeBudgetProposals.count(hash) || budget.mapSeenMasternodeBudgetVotes.count(hash) ||
            budget.mapSeenBudgetDrafts.count(hash) || budget.mapSeenBudgetDraftVotes.count(hash)) {
        if(mapSeenSyncBudget[hash] < MASTERNODE_SYNC_THRESHOLD) {
            lastBudgetItem = GetTime();
            mapSeenSyncBudget[hash]++;
        }
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
    nAssetSyncStarted = GetTime();
}

std::string CMasternodeSync::GetSyncStatus()
{
    switch (masternodeSync.RequestedMasternodeAssets) {
        case MASTERNODE_SYNC_INITIAL: return _("Synchronization pending...");
        case MASTERNODE_SYNC_SPORKS: return _("Synchronizing masternode sporks...");
        case MASTERNODE_SYNC_LIST: return _("Synchronizing masternodes...");
        case MASTERNODE_SYNC_MNW: return _("Synchronizing masternode winners...");
        case MASTERNODE_SYNC_BUDGET: return _("Synchronizing masternode budgets...");
        case MASTERNODE_SYNC_FAILED: return _("Masternode synchronization failed");
        case MASTERNODE_SYNC_FINISHED: return _("Masternode synchronization finished");
    }
    return "";
}

void CMasternodeSync::ProcessMessage(CNode* pfrom, std::string& strCommand, CDataStream& vRecv)
{
    if (strCommand == "ssc") { //Sync status count
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

    if(tick++ % MASTERNODE_SYNC_TIMEOUT != 0) return;

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
    if(RequestedMasternodeAssets == MASTERNODE_SYNC_FAILED && lastFailure + (1*60) < GetTime()) {
        Reset();
    } else if (RequestedMasternodeAssets == MASTERNODE_SYNC_FAILED) {
        return;
    }

    if(fDebug) LogPrintf("CMasternodeSync::Process() - tick %d RequestedMasternodeAssets %d\n", tick, RequestedMasternodeAssets);

    if(RequestedMasternodeAssets == MASTERNODE_SYNC_INITIAL) GetNextAsset();

    // sporks synced but blockchain is not, wait until we're almost at a recent block to continue
    if(Params().NetworkID() != CBaseChainParams::REGTEST &&
            !IsBlockchainSynced() && RequestedMasternodeAssets > MASTERNODE_SYNC_SPORKS) return;

    TRY_LOCK(cs_vNodes, lockRecv);
    if(!lockRecv) return;

    BOOST_FOREACH(CNode* pnode, vNodes)
    {
        if(Params().NetworkID() == CBaseChainParams::REGTEST){
            if(RequestedMasternodeAttempt <= 2) {
                pnode->PushMessage("getsporks"); //get current network sporks
            } else if(RequestedMasternodeAttempt < 4) {
                mnodeman.DsegUpdate(pnode); 
            } else if(RequestedMasternodeAttempt < 6) {
                int nMnCount = mnodeman.CountEnabled();
                pnode->PushMessage("mnget", nMnCount); //sync payees
                uint256 n = uint256();
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

        if (pnode->nVersion >= masternodePayments.GetMinMasternodePaymentsProto()) {

            if(RequestedMasternodeAssets == MASTERNODE_SYNC_LIST) {
                if(fDebug) LogPrintf("CMasternodeSync::Process() - lastMasternodeList %lld (GetTime() - MASTERNODE_SYNC_TIMEOUT) %lld\n", lastMasternodeList, GetTime() - MASTERNODE_SYNC_TIMEOUT);
                if(lastMasternodeList > 0 && lastMasternodeList < GetTime() - MASTERNODE_SYNC_TIMEOUT*2 && RequestedMasternodeAttempt >= MASTERNODE_SYNC_THRESHOLD){ //hasn't received a new item in the last five seconds, so we'll move to the
                    GetNextAsset();
                    return;
                }

                if(pnode->HasFulfilledRequest("mnsync")) continue;
                pnode->FulfilledRequest("mnsync");

                // timeout
                if(lastMasternodeList == 0 &&
                (RequestedMasternodeAttempt >= MASTERNODE_SYNC_THRESHOLD*3 || GetTime() - nAssetSyncStarted > MASTERNODE_SYNC_TIMEOUT*5)) {
                    if(IsSporkActive(SPORK_8_MASTERNODE_PAYMENT_ENFORCEMENT)) {
                        LogPrintf("CMasternodeSync::Process - ERROR - Sync has failed, will retry later\n");
                        RequestedMasternodeAssets = MASTERNODE_SYNC_FAILED;
                        RequestedMasternodeAttempt = 0;
                        lastFailure = GetTime();
                        nCountFailures++;
                    } else {
                        GetNextAsset();
                    }
                    return;
                }

                if(RequestedMasternodeAttempt >= MASTERNODE_SYNC_THRESHOLD*3) return;

                mnodeman.DsegUpdate(pnode);
                RequestedMasternodeAttempt++;
                return;
            }

            if(RequestedMasternodeAssets == MASTERNODE_SYNC_MNW) {
                if(lastMasternodeWinner > 0 && lastMasternodeWinner < GetTime() - MASTERNODE_SYNC_TIMEOUT*2 && RequestedMasternodeAttempt >= MASTERNODE_SYNC_THRESHOLD){ //hasn't received a new item in the last five seconds, so we'll move to the
                    GetNextAsset();
                    return;
                }

                if(pnode->HasFulfilledRequest("mnwsync")) continue;
                pnode->FulfilledRequest("mnwsync");

                // timeout
                if(lastMasternodeWinner == 0 &&
                (RequestedMasternodeAttempt >= MASTERNODE_SYNC_THRESHOLD*3 || GetTime() - nAssetSyncStarted > MASTERNODE_SYNC_TIMEOUT*5)) {
                    if(IsSporkActive(SPORK_8_MASTERNODE_PAYMENT_ENFORCEMENT)) {
                        LogPrintf("CMasternodeSync::Process - ERROR - Sync has failed, will retry later\n");
                        RequestedMasternodeAssets = MASTERNODE_SYNC_FAILED;
                        RequestedMasternodeAttempt = 0;
                        lastFailure = GetTime();
                        nCountFailures++;
                    } else {
                        GetNextAsset();
                    }
                    return;
                }

                if(RequestedMasternodeAttempt >= MASTERNODE_SYNC_THRESHOLD*3) return;

                CBlockIndex* pindexPrev = chainActive.Tip();
                if(pindexPrev == NULL) return;

                int nMnCount = mnodeman.CountEnabled();
                pnode->PushMessage("mnget", nMnCount); //sync payees
                RequestedMasternodeAttempt++;

                return;
            }
        }

        if (pnode->nVersion >= MIN_BUDGET_PEER_PROTO_VERSION) {

            if(RequestedMasternodeAssets == MASTERNODE_SYNC_BUDGET){
                //we'll start rejecting votes if we accidentally get set as synced too soon
                if(lastBudgetItem > 0 && lastBudgetItem < GetTime() - MASTERNODE_SYNC_TIMEOUT*2 && RequestedMasternodeAttempt >= MASTERNODE_SYNC_THRESHOLD){ //hasn't received a new item in the last five seconds, so we'll move to the
                    //LogPrintf("CMasternodeSync::Process - HasNextFinalizedBudget %d nCountFailures %d IsBudgetPropEmpty %d\n", budget.HasNextFinalizedBudget(), nCountFailures, IsBudgetPropEmpty());
                    //if(budget.HasNextFinalizedBudget() || nCountFailures >= 2 || IsBudgetPropEmpty()) {
                        GetNextAsset();

                        //try to activate our masternode if possible
                        activeMasternode.ManageStatus();
                    // } else { //we've failed to sync, this state will reject the next budget block
                    //     LogPrintf("CMasternodeSync::Process - ERROR - Sync has failed, will retry later\n");
                    //     RequestedMasternodeAssets = MASTERNODE_SYNC_FAILED;
                    //     RequestedMasternodeAttempt = 0;
                    //     lastFailure = GetTime();
                    //     nCountFailures++;
                    // }
                    return;
                }

                // timeout
                if(lastBudgetItem == 0 &&
                (RequestedMasternodeAttempt >= MASTERNODE_SYNC_THRESHOLD*3 || GetTime() - nAssetSyncStarted > MASTERNODE_SYNC_TIMEOUT*5)) {
                    // maybe there is no budgets at all, so just finish syncing
                    GetNextAsset();
                    activeMasternode.ManageStatus();
                    return;
                }

                if(pnode->HasFulfilledRequest("busync")) continue;
                pnode->FulfilledRequest("busync");

                if(RequestedMasternodeAttempt >= MASTERNODE_SYNC_THRESHOLD*3) return;

                uint256 n = uint256();
                pnode->PushMessage("mnvs", n); //sync masternode votes
                RequestedMasternodeAttempt++;
                
                return;
            }

        }
    }
}
