// Copyright (c) 2014-2019 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef SYSCOIN_MASTERNODE_MASTERNODESYNC_H
#define SYSCOIN_MASTERNODE_MASTERNODESYNC_H

#include <util/translation.h>
class CMasternodeSync;
class PeerManager;
class CBlockIndex;
class CConnman;
class CNode;
static const int MASTERNODE_SYNC_BLOCKCHAIN      = 1;
static const int MASTERNODE_SYNC_GOVERNANCE      = 4;
static const int MASTERNODE_SYNC_GOVOBJ          = 10;
static const int MASTERNODE_SYNC_GOVOBJ_VOTE     = 11;
static const int MASTERNODE_SYNC_FINISHED        = 999;

static const int MASTERNODE_SYNC_TICK_SECONDS    = 6;
static const int MASTERNODE_SYNC_TIMEOUT_SECONDS = 30; // our blocks are 2.5 minutes so 30 seconds should be fine
static const int MASTERNODE_SYNC_RESET_SECONDS = 600; // Reset fReachedBestHeader in CMasternodeSync::Reset if UpdateBlockTip hasn't been called for this seconds

extern CMasternodeSync masternodeSync;

//
// CMasternodeSync : Sync masternode assets in stages
//

class CMasternodeSync
{
private:
    mutable RecursiveMutex cs;
    // Keep track of current asset
    int nCurrentAsset;
    // Count peers we've requested the asset from
    int nTriedPeerCount;

    // Time when current masternode asset sync started
    int64_t nTimeAssetSyncStarted;
    // ... last bumped
    int64_t nTimeLastBumped;

    /// Set to true if best header is reached in CMasternodeSync::UpdatedBlockTip
    bool fReachedBestHeader{false};
    /// Last time UpdateBlockTip has been called
    int64_t nTimeLastUpdateBlockTip{0};

public:
    CMasternodeSync() { Reset(true, false); }


    static void SendGovernanceSyncRequest(CNode* pnode, CConnman& connman);

    bool IsBlockchainSynced() const {LOCK(cs); return nCurrentAsset > MASTERNODE_SYNC_BLOCKCHAIN; }
    bool IsSynced() const { LOCK(cs); return nCurrentAsset == MASTERNODE_SYNC_FINISHED; }
    void SetSyncMode(const int nMode) { LOCK(cs); nCurrentAsset = nMode; }

    int GetAssetID() const { LOCK(cs); return nCurrentAsset; }
    int GetAttempt() const { return nTriedPeerCount; }
    void BumpAssetLastTime(const std::string& strFuncName);
    int64_t GetAssetStartTime() { return nTimeAssetSyncStarted; }
    std::string GetAssetName() const;
    bilingual_str GetSyncStatus();

    void Reset(bool fForce = false, bool fNotifyReset = true);
    void SwitchToNextAsset(CConnman& connman);

    void ProcessMessage(CNode* pfrom, const std::string& strCommand, CDataStream& vRecv) const;
    void ProcessTick(CConnman& connman, const PeerManager& peerman);
    void AcceptedBlockHeader(const CBlockIndex *pindexNew);
    void NotifyHeaderTip(const CBlockIndex *pindexNew, bool fInitialDownload, CConnman& connman);
    void UpdatedBlockTip(const CBlockIndex *pindexNew, bool fInitialDownload);

    void DoMaintenance(CConnman &connman, const PeerManager& peerman);
};

#endif // SYSCOIN_MASTERNODE_MASTERNODESYNC_H
