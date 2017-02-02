// Copyright (c) 2014-2017 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef MASTERNODE_SYNC_H
#define MASTERNODE_SYNC_H

#include "chain.h"
#include "net.h"

#include <univalue.h>

class CMasternodeSync;

static const int MASTERNODE_SYNC_FAILED          = -1;
static const int MASTERNODE_SYNC_INITIAL         = 0;
static const int MASTERNODE_SYNC_SPORKS          = 1;
static const int MASTERNODE_SYNC_LIST            = 2;
static const int MASTERNODE_SYNC_MNW             = 3;
static const int MASTERNODE_SYNC_GOVERNANCE      = 4;
static const int MASTERNODE_SYNC_GOVOBJ          = 10;
static const int MASTERNODE_SYNC_GOVOBJ_VOTE     = 11;
static const int MASTERNODE_SYNC_FINISHED        = 999;

static const int MASTERNODE_SYNC_TICK_SECONDS    = 6;
static const int MASTERNODE_SYNC_TIMEOUT_SECONDS = 30; // our blocks are 2.5 minutes so 30 seconds should be fine

static const int MASTERNODE_SYNC_ENOUGH_PEERS    = 6;

extern CMasternodeSync masternodeSync;

//
// CMasternodeSync : Sync masternode assets in stages
//

class CMasternodeSync
{
private:
    // Keep track of current asset
    int nRequestedMasternodeAssets;
    // Count peers we've requested the asset from
    int nRequestedMasternodeAttempt;

    // Time when current masternode asset sync started
    int64_t nTimeAssetSyncStarted;

    // Last time when we received some masternode asset ...
    int64_t nTimeLastMasternodeList;
    int64_t nTimeLastPaymentVote;
    int64_t nTimeLastGovernanceItem;
    // ... or failed
    int64_t nTimeLastFailure;

    // How many times we failed
    int nCountFailures;

    // Keep track of current block index
    const CBlockIndex *pCurrentBlockIndex;

    bool CheckNodeHeight(CNode* pnode, bool fDisconnectStuckNodes = false);
    void Fail();
    void ClearFulfilledRequests();

public:
    CMasternodeSync() { Reset(); }

    void AddedMasternodeList() { nTimeLastMasternodeList = GetTime(); }
    void AddedPaymentVote() { nTimeLastPaymentVote = GetTime(); }
    void AddedGovernanceItem() { nTimeLastGovernanceItem = GetTime(); };

    void SendGovernanceSyncRequest(CNode* pnode);

    bool IsFailed() { return nRequestedMasternodeAssets == MASTERNODE_SYNC_FAILED; }
    bool IsBlockchainSynced(bool fBlockAccepted = false);
    bool IsMasternodeListSynced() { return nRequestedMasternodeAssets > MASTERNODE_SYNC_LIST; }
    bool IsWinnersListSynced() { return nRequestedMasternodeAssets > MASTERNODE_SYNC_MNW; }
    bool IsSynced() { return nRequestedMasternodeAssets == MASTERNODE_SYNC_FINISHED; }

    int GetAssetID() { return nRequestedMasternodeAssets; }
    int GetAttempt() { return nRequestedMasternodeAttempt; }
    std::string GetAssetName();
    std::string GetSyncStatus();

    void Reset();
    void SwitchToNextAsset();

    void ProcessMessage(CNode* pfrom, std::string& strCommand, CDataStream& vRecv);
    void ProcessTick();

    void UpdatedBlockTip(const CBlockIndex *pindex);
};

#endif
