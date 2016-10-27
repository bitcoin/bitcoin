// Copyright (c) 2014-2015 The Crown developers

// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef THRONE_SYNC_H
#define THRONE_SYNC_H

#define THRONE_SYNC_INITIAL           0
#define THRONE_SYNC_SPORKS            1
#define THRONE_SYNC_LIST              2
#define THRONE_SYNC_MNW               3
#define THRONE_SYNC_BUDGET            4
#define THRONE_SYNC_BUDGET_PROP       10
#define THRONE_SYNC_BUDGET_FIN        11
#define THRONE_SYNC_FAILED            998
#define THRONE_SYNC_FINISHED          999

#define THRONE_SYNC_TIMEOUT           5
#define THRONE_SYNC_THRESHOLD         2

class CThroneSync;
extern CThroneSync masternodeSync;

//
// CThroneSync : Sync masternode assets in stages
//

class CThroneSync
{
public:
    std::map<uint256, int> mapSeenSyncMNB;
    std::map<uint256, int> mapSeenSyncMNW;
    std::map<uint256, int> mapSeenSyncBudget;

    int64_t lastThroneList;
    int64_t lastThroneWinner;
    int64_t lastBudgetItem;
    int64_t lastFailure;
    int nCountFailures;

    // sum of all counts
    int sumThroneList;
    int sumThroneWinner;
    int sumBudgetItemProp;
    int sumBudgetItemFin;
    // peers that reported counts
    int countThroneList;
    int countThroneWinner;
    int countBudgetItemProp;
    int countBudgetItemFin;

    // Count peers we've requested the list from
    int RequestedThroneAssets;
    int RequestedThroneAttempt;

    // Time when current masternode asset sync started
    int64_t nAssetSyncStarted;

    CThroneSync();

    void AddedThroneList(uint256 hash);
    void AddedThroneWinner(uint256 hash);
    void AddedBudgetItem(uint256 hash);
    void GetNextAsset();
    std::string GetSyncStatus();
    void ProcessMessage(CNode* pfrom, std::string& strCommand, CDataStream& vRecv);
    bool IsBudgetFinEmpty();
    bool IsBudgetPropEmpty();

    void Reset();
    void Process();
    bool IsSynced();
    bool IsBlockchainSynced();
    void ClearFulfilledRequest();
};

#endif
