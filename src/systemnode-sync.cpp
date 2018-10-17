// Copyright (c) 2014-2018 The Crown developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "main.h"
#include "activesystemnode.h"
#include "systemnode-sync.h"
#include "systemnode.h"
#include "systemnodeman.h"
#include "spork.h"
#include "util.h"
#include "addrman.h"

class CSystemnodeSync;
CSystemnodeSync systemnodeSync;

CSystemnodeSync::CSystemnodeSync()
{
    Reset();
}

bool CSystemnodeSync::IsSynced()
{
    return RequestedSystemnodeAssets == SYSTEMNODE_SYNC_FINISHED;
}

bool CSystemnodeSync::IsBlockchainSynced()
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

void CSystemnodeSync::Reset()
{   
    lastSystemnodeList = 0;
    lastSystemnodeWinner = 0;
    lastBudgetItem = 0;
    mapSeenSyncSNB.clear();
    mapSeenSyncSNW.clear();
    lastFailure = 0;
    nCountFailures = 0;
    sumSystemnodeList = 0;
    sumSystemnodeWinner = 0;
    sumBudgetItemProp = 0;
    sumBudgetItemFin = 0;
    countSystemnodeList = 0;
    countSystemnodeWinner = 0;
    countBudgetItemProp = 0;
    countBudgetItemFin = 0;
    RequestedSystemnodeAssets = SYSTEMNODE_SYNC_INITIAL;
    RequestedSystemnodeAttempt = 0;
    nAssetSyncStarted = GetTime();
}

void CSystemnodeSync::AddedSystemnodeList(uint256 hash)
{
    if(snodeman.mapSeenSystemnodeBroadcast.count(hash)) {
        if(mapSeenSyncSNB[hash] < SYSTEMNODE_SYNC_THRESHOLD) {
            lastSystemnodeList = GetTime();
            mapSeenSyncSNB[hash]++;
        }
    } else {
        lastSystemnodeList = GetTime();
        mapSeenSyncSNB.insert(make_pair(hash, 1));
    }
}

void CSystemnodeSync::AddedSystemnodeWinner(uint256 hash)
{
    if(systemnodePayments.mapSystemnodePayeeVotes.count(hash)) {
        if(mapSeenSyncSNW[hash] < SYSTEMNODE_SYNC_THRESHOLD) {
            lastSystemnodeWinner = GetTime();
            mapSeenSyncSNW[hash]++;
        }
    } else {
        lastSystemnodeWinner = GetTime();
        mapSeenSyncSNW.insert(make_pair(hash, 1));
    }
}

void CSystemnodeSync::GetNextAsset()
{
    switch(RequestedSystemnodeAssets)
    {
        case(SYSTEMNODE_SYNC_INITIAL):
        case(SYSTEMNODE_SYNC_FAILED): // should never be used here actually, use Reset() instead
            ClearFulfilledRequest();
            RequestedSystemnodeAssets = SYSTEMNODE_SYNC_SPORKS;
            break;
        case(SYSTEMNODE_SYNC_SPORKS):
            RequestedSystemnodeAssets = SYSTEMNODE_SYNC_LIST;
            break;
        case(SYSTEMNODE_SYNC_LIST):
            RequestedSystemnodeAssets = SYSTEMNODE_SYNC_SNW;
            break;
        case(SYSTEMNODE_SYNC_SNW):
            LogPrintf("CSystemnodeSync::GetNextAsset - Sync has finished\n");
            RequestedSystemnodeAssets = SYSTEMNODE_SYNC_FINISHED;
            break;
    }
    RequestedSystemnodeAttempt = 0;
    nAssetSyncStarted = GetTime();
}

std::string CSystemnodeSync::GetSyncStatus()
{
    switch (systemnodeSync.RequestedSystemnodeAssets) {
        case SYSTEMNODE_SYNC_INITIAL: return _("Synchronization pending...");
        case SYSTEMNODE_SYNC_SPORKS: return _("Synchronizing systemnode sporks...");
        case SYSTEMNODE_SYNC_LIST: return _("Synchronizing systemnodes...");
        case SYSTEMNODE_SYNC_SNW: return _("Synchronizing systemnode winners...");
        case SYSTEMNODE_SYNC_FAILED: return _("Systemnode synchronization failed");
        case SYSTEMNODE_SYNC_FINISHED: return _("Systemnode synchronization finished");
    }
    return "";
}

void CSystemnodeSync::ProcessMessage(CNode* pfrom, std::string& strCommand, CDataStream& vRecv)
{
    if (strCommand == "snssc") { //Sync status count
        int nItemID;
        int nCount;
        vRecv >> nItemID >> nCount;

        if(RequestedSystemnodeAssets >= SYSTEMNODE_SYNC_FINISHED) return;

        //this means we will receive no further communication
        switch(nItemID)
        {
            case(SYSTEMNODE_SYNC_LIST):
                if(nItemID != RequestedSystemnodeAssets) return;
                sumSystemnodeList += nCount;
                countSystemnodeList++;
                break;
            case(SYSTEMNODE_SYNC_SNW):
                if(nItemID != RequestedSystemnodeAssets) return;
                sumSystemnodeWinner += nCount;
                countSystemnodeWinner++;
                break;
        }
        
        LogPrintf("CSystemnodeSync:ProcessMessage - snssc - got inventory count %d %d\n", nItemID, nCount);
    }
}

void CSystemnodeSync::ClearFulfilledRequest()
{
    TRY_LOCK(cs_vNodes, lockRecv);
    if(!lockRecv) return;

    BOOST_FOREACH(CNode* pnode, vNodes)
    {
        pnode->ClearFulfilledRequest("sngetspork");
        pnode->ClearFulfilledRequest("snsync");
        pnode->ClearFulfilledRequest("snwsync");
    }
}

void CSystemnodeSync::Process()
{
    static int tick = 0;

    if(tick++ % SYSTEMNODE_SYNC_TIMEOUT != 0) return;

    if(IsSynced()) {
        /* 
            Resync if we lose all systemnodes from sleep/wake or failure to sync originally
        */
        if(snodeman.CountEnabled() == 0) {
            Reset();
        } else
            return;
    }

    //try syncing again
    if(RequestedSystemnodeAssets == SYSTEMNODE_SYNC_FAILED && lastFailure + (1*60) < GetTime()) {
        Reset();
    } else if (RequestedSystemnodeAssets == SYSTEMNODE_SYNC_FAILED) {
        return;
    }

    if(fDebug) LogPrintf("CSystemnodeSync::Process() - tick %d RequestedSystemnodeAssets %d\n", tick, RequestedSystemnodeAssets);

    if(RequestedSystemnodeAssets == SYSTEMNODE_SYNC_INITIAL) GetNextAsset();

    // sporks synced but blockchain is not, wait until we're almost at a recent block to continue
    if(Params().NetworkID() != CBaseChainParams::REGTEST &&
            !IsBlockchainSynced() && RequestedSystemnodeAssets > SYSTEMNODE_SYNC_SPORKS) return;

    TRY_LOCK(cs_vNodes, lockRecv);
    if(!lockRecv) return;

    BOOST_FOREACH(CNode* pnode, vNodes)
    {
        if(Params().NetworkID() == CBaseChainParams::REGTEST){
            if(RequestedSystemnodeAttempt <= 2) {
                pnode->PushMessage("getsporks"); //get current network sporks
            } else if(RequestedSystemnodeAttempt < 4) {
                snodeman.DsegUpdate(pnode); 
            } else if(RequestedSystemnodeAttempt < 6) {
                int nMnCount = snodeman.CountEnabled();
                pnode->PushMessage("snget", nMnCount); //sync payees
                uint256 n = uint256();
                pnode->PushMessage("snvs", n); //sync systemnode votes
            } else {
                RequestedSystemnodeAssets = SYSTEMNODE_SYNC_FINISHED;
            }
            RequestedSystemnodeAttempt++;
            return;
        }

        //set to synced
        if(RequestedSystemnodeAssets == SYSTEMNODE_SYNC_SPORKS){
            if(pnode->HasFulfilledRequest("sngetspork")) continue;
            pnode->FulfilledRequest("sngetspork");

            pnode->PushMessage("getsporks"); //get current network sporks
            if(RequestedSystemnodeAttempt >= 2) GetNextAsset();
            RequestedSystemnodeAttempt++;
            
            return;
        }

        if (pnode->nVersion >= systemnodePayments.GetMinSystemnodePaymentsProto()) {

            if(RequestedSystemnodeAssets == SYSTEMNODE_SYNC_LIST) {
                if(fDebug) LogPrintf("CSystemnodeSync::Process() - lastSystemnodeList %lld (GetTime() - SYSTEMNODE_SYNC_TIMEOUT) %lld\n", lastSystemnodeList, GetTime() - SYSTEMNODE_SYNC_TIMEOUT);
                if(lastSystemnodeList > 0 && lastSystemnodeList < GetTime() - SYSTEMNODE_SYNC_TIMEOUT*2 && RequestedSystemnodeAttempt >= SYSTEMNODE_SYNC_THRESHOLD){ //hasn't received a new item in the last five seconds, so we'll move to the
                    GetNextAsset();
                    return;
                }

                if(pnode->HasFulfilledRequest("snsync")) continue;
                pnode->FulfilledRequest("snsync");

                // timeout
                if(lastSystemnodeList == 0 &&
                (RequestedSystemnodeAttempt >= SYSTEMNODE_SYNC_THRESHOLD*3 || GetTime() - nAssetSyncStarted > SYSTEMNODE_SYNC_TIMEOUT*5)) {
                    if(IsSporkActive(SPORK_14_SYSTEMNODE_PAYMENT_ENFORCEMENT)) {
                        LogPrintf("CSystemnodeSync::Process - ERROR - Sync has failed, will retry later\n");
                        RequestedSystemnodeAssets = SYSTEMNODE_SYNC_FAILED;
                        RequestedSystemnodeAttempt = 0;
                        lastFailure = GetTime();
                        nCountFailures++;
                    } else {
                        GetNextAsset();
                    }
                    return;
                }

                if(RequestedSystemnodeAttempt >= SYSTEMNODE_SYNC_THRESHOLD*3) return;

                snodeman.DsegUpdate(pnode);
                RequestedSystemnodeAttempt++;
                return;
            }

            if(RequestedSystemnodeAssets == SYSTEMNODE_SYNC_SNW) {
                if(lastSystemnodeWinner > 0 && lastSystemnodeWinner < GetTime() - SYSTEMNODE_SYNC_TIMEOUT*2 && RequestedSystemnodeAttempt >= SYSTEMNODE_SYNC_THRESHOLD){ //hasn't received a new item in the last five seconds, so we'll move to the
                    GetNextAsset();
                    return;
                }

                if(pnode->HasFulfilledRequest("snwsync")) continue;
                pnode->FulfilledRequest("snwsync");

                // timeout
                if(lastSystemnodeWinner == 0 &&
                (RequestedSystemnodeAttempt >= SYSTEMNODE_SYNC_THRESHOLD*3 || GetTime() - nAssetSyncStarted > SYSTEMNODE_SYNC_TIMEOUT*5)) {
                    if(IsSporkActive(SPORK_14_SYSTEMNODE_PAYMENT_ENFORCEMENT)) {
                        LogPrintf("CSystemnodeSync::Process - ERROR - Sync has failed, will retry later\n");
                        RequestedSystemnodeAssets = SYSTEMNODE_SYNC_FAILED;
                        RequestedSystemnodeAttempt = 0;
                        lastFailure = GetTime();
                        nCountFailures++;
                    } else {
                        GetNextAsset();
                    }
                    return;
                }

                if(RequestedSystemnodeAttempt >= SYSTEMNODE_SYNC_THRESHOLD*3) return;

                CBlockIndex* pindexPrev = chainActive.Tip();
                if(pindexPrev == NULL) return;

                int nSnCount = snodeman.CountEnabled();
                pnode->PushMessage("snget", nSnCount); //sync payees
                RequestedSystemnodeAttempt++;

                return;
            }
        }
    }
}
