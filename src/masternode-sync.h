// Copyright (c) 2014-2016 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef MASTERNODE_SYNC_H
#define MASTERNODE_SYNC_H

#include "chain.h"
#include "net.h"

#include <univalue.h>

class CMasternodeSync;

static const int MASTERNODE_SYNC_INITIAL         = 0;
static const int MASTERNODE_SYNC_SPORKS          = 1;
static const int MASTERNODE_SYNC_LIST            = 2;
static const int MASTERNODE_SYNC_MNW             = 3;
static const int MASTERNODE_SYNC_GOVERNANCE      = 4;
static const int MASTERNODE_SYNC_GOVOBJ          = 10;
static const int MASTERNODE_SYNC_GOVERNANCE_FIN  = 11;
static const int MASTERNODE_SYNC_FAILED          = 998;
static const int MASTERNODE_SYNC_FINISHED        = 999;

static const int MASTERNODE_SYNC_TIMEOUT_SECONDS = 30; // our blocks are 2.5 minutes so 30 seconds should be fine

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
    int64_t nTimeLastMasternodeWinner;
    int64_t nTimeLastBudgetItem;
    // ... or failed
    int64_t nTimeLastFailure;

    // How many times we failed
    int nCountFailures;

    // Sum of all data counts
    int nSumMasternodeList;
    int nSumMasternodeWinner;
    int nSumBudgetItemProp;
    int nSumBudgetItemFin;

    // Number of peers that reported data counts
    int nCountMasternodeList;
    int nCountMasternodeWinner;
    int nCountBudgetItemProp;
    int nCountBudgetItemFin;

    // Keep track of current block index
    const CBlockIndex *pCurrentBlockIndex;

    void Fail();
    void ClearFulfilledRequest();

public:
    std::map<uint256, int> mapSeenSyncMNB;
    std::map<uint256, int> mapSeenSyncMNW;
    std::map<uint256, int> mapSeenSyncBudget;

    CMasternodeSync() { Reset(); }

    void AddedMasternodeList(uint256 hash);
    void AddedMasternodeWinner(uint256 hash);
    void AddedBudgetItem(uint256 hash);

    bool IsFailed() { return nRequestedMasternodeAssets == MASTERNODE_SYNC_FAILED; }
    bool IsSynced() { return nRequestedMasternodeAssets == MASTERNODE_SYNC_FINISHED; }
    bool IsBlockchainSynced();

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
