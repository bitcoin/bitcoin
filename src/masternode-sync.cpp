// Copyright (c) 2014-2016 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "main.h"
#include "activemasternode.h"
#include "masternode-sync.h"
#include "masternode-payments.h"
#include "governance.h"
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

bool CMasternodeSync::IsBudgetPropEmpty()
{
    return sumBudgetItemProp==0 && countBudgetItemProp>0;
}

bool CMasternodeSync::IsBudgetFinEmpty()
{
    return sumBudgetItemFin==0 && countBudgetItemFin>0;
}

std::string CMasternodeSync::GetAssetName()
{
    switch(RequestedMasternodeAssets)
    {
        case(MASTERNODE_SYNC_INITIAL):
            return "MASTERNODE_SYNC_INITIAL";
        case(MASTERNODE_SYNC_FAILED): // should never be used here actually, use Reset() instead
            return "MASTERNODE_SYNC_FAILED";
        case(MASTERNODE_SYNC_SPORKS):
            return "MASTERNODE_SYNC_SPORKS";
        case(MASTERNODE_SYNC_LIST):
            return "MASTERNODE_SYNC_LIST";
        case(MASTERNODE_SYNC_MNW):
            return "MASTERNODE_SYNC_MNW";
        case(MASTERNODE_SYNC_GOVERNANCE):
            return "MASTERNODE_SYNC_GOVERNANCE";
    }
    return "error";
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
            RequestedMasternodeAssets = MASTERNODE_SYNC_GOVERNANCE;
            break;
        case(MASTERNODE_SYNC_GOVERNANCE):
            LogPrintf("CMasternodeSync::GetNextAsset - Sync has finished\n");
            RequestedMasternodeAssets = MASTERNODE_SYNC_FINISHED;
            uiInterface.NotifyAdditionalDataSyncProgressChanged(1);
            //try to activate our masternode if possible
            activeMasternode.ManageState();
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
        case MASTERNODE_SYNC_GOVERNANCE: return _("Synchronizing governance objects...");
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
            case(MASTERNODE_SYNC_GOVOBJ):
                if(RequestedMasternodeAssets != MASTERNODE_SYNC_GOVERNANCE) return;
                sumBudgetItemProp += nCount;
                countBudgetItemProp++;
                break;
            case(MASTERNODE_SYNC_GOVERNANCE_FIN):
                if(RequestedMasternodeAssets != MASTERNODE_SYNC_GOVERNANCE) return;
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
        pnode->ClearFulfilledRequest("spork-sync");
        pnode->ClearFulfilledRequest("masternode-winner-sync");
        pnode->ClearFulfilledRequest("governance-sync");
        pnode->ClearFulfilledRequest("masternode-sync");
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
            } else {
                //if syncing is complete and we have masternodes, return
                return;
            }
        }

        //try syncing again
        if(RequestedMasternodeAssets == MASTERNODE_SYNC_FAILED && lastFailure + (1*60) < GetTime()) {
            Reset();
        } else if (RequestedMasternodeAssets == MASTERNODE_SYNC_FAILED) {
            return;
        }
    }

    // INITIAL SYNC SETUP / LOG REPORTING
    double nSyncProgress = double(RequestedMasternodeAttempt + (RequestedMasternodeAssets - 1) * 8) / (8*4);
    LogPrintf("CMasternodeSync::Process() - tick %d RequestedMasternodeAttempt %d RequestedMasternodeAssets %d nSyncProgress %f\n", tick, RequestedMasternodeAttempt, RequestedMasternodeAssets, nSyncProgress);
    uiInterface.NotifyAdditionalDataSyncProgressChanged(nSyncProgress);

    // sporks synced but blockchain is not, wait until we're almost at a recent block to continue
    if(Params().NetworkIDString() != CBaseChainParams::REGTEST &&
            !IsBlockchainSynced() && RequestedMasternodeAssets > MASTERNODE_SYNC_SPORKS) return;

    TRY_LOCK(cs_vNodes, lockRecv);
    if(!lockRecv) return;

    if(RequestedMasternodeAssets == MASTERNODE_SYNC_INITIAL) GetNextAsset();

    BOOST_FOREACH(CNode* pnode, vNodes)
    {
        // QUICK MODE (REGTEST ONLY!)
        if(Params().NetworkIDString() == CBaseChainParams::REGTEST)
        {
            if(RequestedMasternodeAttempt <= 2) {
                pnode->PushMessage(NetMsgType::GETSPORKS); //get current network sporks
            } else if(RequestedMasternodeAttempt < 4) {
                mnodeman.DsegUpdate(pnode);
            } else if(RequestedMasternodeAttempt < 6) {
                int nMnCount = mnodeman.CountEnabled();
                pnode->PushMessage(NetMsgType::MNWINNERSSYNC, nMnCount); //sync payees
                uint256 n = uint256();
                pnode->PushMessage(NetMsgType::MNGOVERNANCESYNC, n); //sync masternode votes
            } else {
                RequestedMasternodeAssets = MASTERNODE_SYNC_FINISHED;
            }
            RequestedMasternodeAttempt++;
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
                if(RequestedMasternodeAssets == MASTERNODE_SYNC_SPORKS) GetNextAsset();

                continue; // always get sporks first, switch to the next node without waiting for the next tick
            }

            // MNLIST : SYNC MASTERNODE LIST FROM OTHER CONNECTED CLIENTS

            if(RequestedMasternodeAssets == MASTERNODE_SYNC_LIST) {
                // check for timeout first
                if(lastMasternodeList < GetTime() - MASTERNODE_SYNC_TIMEOUT) {
                    LogPrintf("CMasternodeSync::Process -- tick %d  asset %d -- timeout\n", tick, RequestedMasternodeAssets);
                    if(RequestedMasternodeAttempt == 0)
                        LogPrintf("CMasternodeSync::Process -- WARNING: failed to sync %s\n", GetAssetName());
                    GetNextAsset();
                    return;
                }

                // check for data
                // if we have enough masternodes in or list, switch to the next asset
                /* Note: Is this activing up? It's probably related to int CMasternodeMan::GetEstimatedMasternodes(int nBlock)
                   Surely doesn't work right for testnet currently */
                // try to fetch data from at least one peer though
                if(RequestedMasternodeAssets > 0 && nMnCount > mnodeman.GetEstimatedMasternodes(pCurrentBlockIndex->nHeight)*0.9) {
                    LogPrintf("CMasternodeSync::Process -- tick %d  asset %d -- found enough data\n", tick, RequestedMasternodeAssets);
                    GetNextAsset();
                    return;
                }

                // only request once from each peer
                if(pnode->HasFulfilledRequest("masternode-sync")) continue;
                pnode->FulfilledRequest("masternode-sync");

                if (pnode->nVersion < mnpayments.GetMinMasternodePaymentsProto()) continue;
                RequestedMasternodeAttempt++;

                mnodeman.DsegUpdate(pnode);

                return; //this will cause each peer to get one request each six seconds for the various assets we need
            }

            // MNW : SYNC MASTERNODE WINNERS FROM OTHER CONNECTED CLIENTS

            if(RequestedMasternodeAssets == MASTERNODE_SYNC_MNW) {
                // check for timeout first
                // This might take a lot longer than MASTERNODE_SYNC_TIMEOUT minutes due to new blocks,
                // but that should be OK and it should timeout eventually.
                if(lastMasternodeWinner < GetTime() - MASTERNODE_SYNC_TIMEOUT) {
                    LogPrintf("CMasternodeSync::Process -- tick %d  asset %d -- timeout\n", tick, RequestedMasternodeAssets);
                    if(RequestedMasternodeAttempt == 0)
                        LogPrintf("CMasternodeSync::Process -- WARNING: failed to sync %s\n", GetAssetName());
                    GetNextAsset();
                    return;
                }

                // check for data
                // if mnpayments already has enough blocks and votes, move to the next asset
                // try to fetch data from at least one peer though
                if(RequestedMasternodeAssets > 0 && mnpayments.IsEnoughData(nMnCount)) {
                    LogPrintf("CMasternodeSync::Process -- tick %d  asset %d -- found enough data\n", tick, RequestedMasternodeAssets);
                    GetNextAsset();
                    return;
                }

                // only request once from each peer
                if(pnode->HasFulfilledRequest("masternode-winner-sync")) continue;
                pnode->FulfilledRequest("masternode-winner-sync");

                if (pnode->nVersion < mnpayments.GetMinMasternodePaymentsProto()) continue;
                RequestedMasternodeAttempt++;

                pnode->PushMessage(NetMsgType::MNWINNERSSYNC, nMnCount); //sync payees


                return; //this will cause each peer to get one request each six seconds for the various assets we need
            }

            // GOVOBJ : SYNC GOVERNANCE ITEMS FROM OUR PEERS

            if(RequestedMasternodeAssets == MASTERNODE_SYNC_GOVERNANCE) {
                // check for timeout first
                if(lastBudgetItem < GetTime() - MASTERNODE_SYNC_TIMEOUT){
                    LogPrintf("CMasternodeSync::Process -- tick %d  asset %d -- timeout\n", tick, RequestedMasternodeAssets);
                    if(RequestedMasternodeAttempt == 0)
                        LogPrintf("CMasternodeSync::Process -- WARNING: failed to sync %s\n", GetAssetName());
                    GetNextAsset();
                    return;
                }

                // check for data
                // if(countBudgetItemProp > 0 && countBudgetItemFin)
                // {
                //     if(governance.CountProposalInventoryItems() >= (sumBudgetItemProp / countBudgetItemProp)*0.9)
                //     {
                //         if(governance.CountFinalizedInventoryItems() >= (sumBudgetItemFin / countBudgetItemFin)*0.9)
                //         {
                //             GetNextAsset();
                //             return;
                //         }
                //     }
                // }


                // only request once from each peer
                if(pnode->HasFulfilledRequest("governance-sync")) continue;
                pnode->FulfilledRequest("governance-sync");

                if (pnode->nVersion < MSG_GOVERNANCE_PEER_PROTO_VERSION) continue;
                RequestedMasternodeAttempt++;

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
