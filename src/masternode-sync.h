// Copyright (c) 2014-2015 The Crown developers

// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef MASTERNODE_SYNC_H
#define MASTERNODE_SYNC_H

#define MASTERNODE_SYNC_INITIAL           0
#define MASTERNODE_SYNC_SPORKS            1
#define MASTERNODE_SYNC_LIST              2
#define MASTERNODE_SYNC_MNW               3
#define MASTERNODE_SYNC_BUDGET            4
#define MASTERNODE_SYNC_BUDGET_PROP       10
#define MASTERNODE_SYNC_BUDGET_FIN        11
#define MASTERNODE_SYNC_FAILED            998
#define MASTERNODE_SYNC_FINISHED          999

#define MASTERNODE_SYNC_TIMEOUT           5
#define MASTERNODE_SYNC_THRESHOLD         2

class CThroneSync;
extern CThroneSync throneSync;

//
// CThroneSync : Sync throne assets in stages
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

    // Time when current throne asset sync started
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
