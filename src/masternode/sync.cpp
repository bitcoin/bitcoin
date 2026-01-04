// Copyright (c) 2014-2025 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <masternode/sync.h>

#include <chain.h>
#include <logging.h>
#include <util/time.h>
#include <util/translation.h>

CMasternodeSync::CMasternodeSync(std::unique_ptr<NodeSyncNotifier>&& sync_notifier) :
    nTimeAssetSyncStarted{GetTime()},
    nTimeLastBumped{GetTime()},
    m_sync_notifier{std::move(sync_notifier)}
{
    assert(m_sync_notifier != nullptr);
}

CMasternodeSync::~CMasternodeSync() = default;

void CMasternodeSync::Reset(bool fForce, bool fNotifyReset)
{
    // Avoid resetting the sync process if we just "recently" received a new block
    if (!fForce) {
        if (GetTime() - nTimeLastUpdateBlockTip < MASTERNODE_SYNC_RESET_SECONDS) {
            return;
        }
    }
    nCurrentAsset = MASTERNODE_SYNC_BLOCKCHAIN;
    nTriedPeerCount = 0;
    nTimeAssetSyncStarted = GetTime();
    nTimeLastBumped = GetTime();
    nTimeLastUpdateBlockTip = 0;
    fReachedBestHeader = false;
    if (fNotifyReset) {
        m_sync_notifier->SyncReset();
    }
}

void CMasternodeSync::BumpAssetLastTime(const std::string& strFuncName)
{
    if (IsSynced()) return;
    nTimeLastBumped = GetTime();
    LogPrint(BCLog::MNSYNC, "CMasternodeSync::BumpAssetLastTime -- %s\n", strFuncName);
}

std::string CMasternodeSync::GetAssetName() const
{
    switch(nCurrentAsset)
    {
        case(MASTERNODE_SYNC_BLOCKCHAIN):   return "MASTERNODE_SYNC_BLOCKCHAIN";
        case(MASTERNODE_SYNC_GOVERNANCE):   return "MASTERNODE_SYNC_GOVERNANCE";
        case MASTERNODE_SYNC_FINISHED:      return "MASTERNODE_SYNC_FINISHED";
        default:                            return "UNKNOWN";
    }
}

void CMasternodeSync::SwitchToNextAsset()
{
    switch(nCurrentAsset)
    {
        case(MASTERNODE_SYNC_BLOCKCHAIN):
            LogPrintf("CMasternodeSync::SwitchToNextAsset -- Completed %s in %llds\n", GetAssetName(), GetTime() - nTimeAssetSyncStarted);
            nCurrentAsset = MASTERNODE_SYNC_GOVERNANCE;
            LogPrintf("CMasternodeSync::SwitchToNextAsset -- Starting %s\n", GetAssetName());
            break;
        case(MASTERNODE_SYNC_GOVERNANCE):
            LogPrintf("CMasternodeSync::SwitchToNextAsset -- Completed %s in %llds\n", GetAssetName(), GetTime() - nTimeAssetSyncStarted);
            nCurrentAsset = MASTERNODE_SYNC_FINISHED;
            m_sync_notifier->SyncFinished();
            LogPrintf("CMasternodeSync::SwitchToNextAsset -- Sync has finished\n");

            break;
    }
    nTriedPeerCount = 0;
    nTimeAssetSyncStarted = GetTime();
    BumpAssetLastTime("CMasternodeSync::SwitchToNextAsset");
}

std::string CMasternodeSync::GetSyncStatus() const
{
    switch (nCurrentAsset) {
        case MASTERNODE_SYNC_BLOCKCHAIN:    return _("Synchronizing blockchain…").translated;
        case MASTERNODE_SYNC_GOVERNANCE:    return _("Synchronizing governance objects…").translated;
        case MASTERNODE_SYNC_FINISHED:      return _("Synchronization finished").translated;
        default:                            return "";
    }
}

void CMasternodeSync::AcceptedBlockHeader(const CBlockIndex *pindexNew)
{
    LogPrint(BCLog::MNSYNC, "CMasternodeSync::AcceptedBlockHeader -- pindexNew->nHeight: %d\n", pindexNew->nHeight);

    if (!IsBlockchainSynced()) {
        // Postpone timeout each time new block header arrives while we are still syncing blockchain
        BumpAssetLastTime("CMasternodeSync::AcceptedBlockHeader");
    }
}

void CMasternodeSync::NotifyHeaderTip(const CBlockIndex *pindexNew, bool fInitialDownload)
{
    if (pindexNew == nullptr) {
        return;
    }
    LogPrint(BCLog::MNSYNC, "CMasternodeSync::NotifyHeaderTip -- pindexNew->nHeight: %d fInitialDownload=%d\n", pindexNew->nHeight, fInitialDownload);
    if (IsSynced())
        return;

    if (!IsBlockchainSynced()) {
        // Postpone timeout each time new block arrives while we are still syncing blockchain
        BumpAssetLastTime("CMasternodeSync::NotifyHeaderTip");
    }
}

void CMasternodeSync::UpdatedBlockTip(const CBlockIndex *pindexTip, const CBlockIndex *pindexNew, bool fInitialDownload)
{
    LogPrint(BCLog::MNSYNC, "CMasternodeSync::UpdatedBlockTip -- pindexNew->nHeight: %d fInitialDownload=%d\n", pindexNew->nHeight, fInitialDownload);
    nTimeLastUpdateBlockTip = GetTime<std::chrono::seconds>().count();

    if (IsSynced())
        return;

    if (!IsBlockchainSynced()) {
        // Postpone timeout each time new block arrives while we are still syncing blockchain
        BumpAssetLastTime("CMasternodeSync::UpdatedBlockTip");
    }

    if (fInitialDownload) {
        // switched too early
        if (IsBlockchainSynced()) {
            Reset(true);
        }

        // no need to check any further while still in IBD mode
        return;
    }

    // Note: since we sync headers first, it should be ok to use this
    if (pindexTip == nullptr) return;
    bool fReachedBestHeaderNew = pindexNew->GetBlockHash() == pindexTip->GetBlockHash();

    if (fReachedBestHeader && !fReachedBestHeaderNew) {
        // Switching from true to false means that we previously stuck syncing headers for some reason,
        // probably initial timeout was not enough,
        // because there is no way we can update tip not having best header
        Reset(true);
    }

    fReachedBestHeader = fReachedBestHeaderNew;
    LogPrint(BCLog::MNSYNC, "CMasternodeSync::UpdatedBlockTip -- pindexNew->nHeight: %d pindexTip->nHeight: %d fInitialDownload=%d fReachedBestHeader=%d\n",
                pindexNew->nHeight, pindexTip->nHeight, fInitialDownload, fReachedBestHeader);
}
