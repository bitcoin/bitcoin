// Copyright (c) 2014-2019 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef MASTERNODE_SYNC_H
#define MASTERNODE_SYNC_H

#include <chain.h>
#include <net.h>

class CMasternodeSync;

static const int MASTERNODE_SYNC_INITIAL         = 0; // sync just started, was reset recently or still in IDB
static const int MASTERNODE_SYNC_WAITING         = 1; // waiting after initial to see if we can get more headers/blocks
static const int MASTERNODE_SYNC_GOVERNANCE      = 4;
static const int MASTERNODE_SYNC_GOVOBJ          = 10;
static const int MASTERNODE_SYNC_GOVOBJ_VOTE     = 11;
static const int MASTERNODE_SYNC_FINISHED        = 999;

static const int MASTERNODE_SYNC_TICK_SECONDS    = 6;
static const int MASTERNODE_SYNC_TIMEOUT_SECONDS = 30; // our blocks are 2.5 minutes so 30 seconds should be fine

extern CMasternodeSync masternodeSync;

//
// CMasternodeSync : Sync masternode assets in stages
//

class CMasternodeSync
{
private:
    // Keep track of current asset
    int nCurrentAsset;
    // Count peers we've requested the asset from
    int nTriedPeerCount;

    // Time when current masternode asset sync started
    int64_t nTimeAssetSyncStarted;
    // ... last bumped
    int64_t nTimeLastBumped;

public:
    CMasternodeSync() { Reset(); }


    void SendGovernanceSyncRequest(CNode* pnode, CConnman& connman);

    bool IsBlockchainSynced() { return nCurrentAsset > MASTERNODE_SYNC_WAITING; }
    bool IsSynced() { return nCurrentAsset == MASTERNODE_SYNC_FINISHED; }

    int GetAssetID() { return nCurrentAsset; }
    int GetAttempt() { return nTriedPeerCount; }
    void BumpAssetLastTime(const std::string& strFuncName);
    int64_t GetAssetStartTime() { return nTimeAssetSyncStarted; }
    std::string GetAssetName();
    std::string GetSyncStatus();

    void Reset();
    void SwitchToNextAsset(CConnman& connman);

    void ProcessMessage(CNode* pfrom, const std::string& strCommand, CDataStream& vRecv);
    void ProcessTick(CConnman& connman);

    void AcceptedBlockHeader(const CBlockIndex *pindexNew);
    void NotifyHeaderTip(const CBlockIndex *pindexNew, bool fInitialDownload, CConnman& connman);
    void UpdatedBlockTip(const CBlockIndex *pindexNew, bool fInitialDownload, CConnman& connman);

    void DoMaintenance(CConnman &connman);
};

#endif
