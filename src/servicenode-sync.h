// Copyright (c) 2014-2015 The Crown developers

// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef SERVICENODE_SYNC_H
#define SERVICENODE_SYNC_H

#define SERVICENODE_SYNC_INITIAL           0
#define SERVICENODE_SYNC_SPORKS            1
#define SERVICENODE_SYNC_LIST              2
#define SERVICENODE_SYNC_MNW               3
#define SERVICENODE_SYNC_BUDGET            4
#define SERVICENODE_SYNC_BUDGET_PROP       10
#define SERVICENODE_SYNC_BUDGET_FIN        11
#define SERVICENODE_SYNC_FAILED            998
#define SERVICENODE_SYNC_FINISHED          999

#define SERVICENODE_SYNC_TIMEOUT           5
#define SERVICENODE_SYNC_THRESHOLD         2

class CServicenodeSync;
extern CServicenodeSync servicenodeSync;

//
// CServicenodeSync : Sync servicenode assets in stages
//

class CServicenodeSync
{
public:
    std::map<uint256, int> mapSeenSyncMNB;
    std::map<uint256, int> mapSeenSyncMNW;
    std::map<uint256, int> mapSeenSyncBudget;

    int64_t lastServicenodeList;
    int64_t lastServicenodeWinner;
    int64_t lastBudgetItem;
    int64_t lastFailure;
    int nCountFailures;

    // sum of all counts
    int sumServicenodeList;
    int sumServicenodeWinner;
    int sumBudgetItemProp;
    int sumBudgetItemFin;
    // peers that reported counts
    int countServicenodeList;
    int countServicenodeWinner;
    int countBudgetItemProp;
    int countBudgetItemFin;

    // Count peers we've requested the list from
    int RequestedServicenodeAssets;
    int RequestedServicenodeAttempt;

    // Time when current servicenode asset sync started
    int64_t nAssetSyncStarted;

    CServicenodeSync();

    void AddedServicenodeList(uint256 hash);
    void AddedServicenodeWinner(uint256 hash);
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
