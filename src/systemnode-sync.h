// Copyright (c) 2014-2018 The Crown developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SYSTEMNODE_SYNC_H
#define SYSTEMNODE_SYNC_H

#define SYSTEMNODE_SYNC_INITIAL           0
#define SYSTEMNODE_SYNC_SPORKS            1
#define SYSTEMNODE_SYNC_LIST              2
#define SYSTEMNODE_SYNC_SNW               3
#define SYSTEMNODE_SYNC_FAILED            998
#define SYSTEMNODE_SYNC_FINISHED          999

#define SYSTEMNODE_SYNC_TIMEOUT           5
#define SYSTEMNODE_SYNC_THRESHOLD         2

class CSystemnodeSync;
extern CSystemnodeSync systemnodeSync;

//
// CSystemnodeSync : Sync systemnode assets in stages
//

class CSystemnodeSync
{
public:
    std::map<uint256, int> mapSeenSyncSNB;
    std::map<uint256, int> mapSeenSyncSNW;

    int64_t lastSystemnodeList;
    int64_t lastSystemnodeWinner;
    int64_t lastBudgetItem;
    int64_t lastFailure;
    int nCountFailures;

    // sum of all counts
    int sumSystemnodeList;
    int sumSystemnodeWinner;
    int sumBudgetItemProp;
    int sumBudgetItemFin;
    // peers that reported counts
    int countSystemnodeList;
    int countSystemnodeWinner;
    int countBudgetItemProp;
    int countBudgetItemFin;

    // Count peers we've requested the list from
    int RequestedSystemnodeAssets;
    int RequestedSystemnodeAttempt;

    // Time when current systemnode asset sync started
    int64_t nAssetSyncStarted;

    CSystemnodeSync();

    void AddedSystemnodeList(uint256 hash);
    void AddedSystemnodeWinner(uint256 hash);
    void GetNextAsset();
    std::string GetSyncStatus();
    void ProcessMessage(CNode* pfrom, std::string& strCommand, CDataStream& vRecv);

    void Reset();
    void Process();
    bool IsSynced();
    bool IsBlockchainSynced();
    void ClearFulfilledRequest();
};

#endif
