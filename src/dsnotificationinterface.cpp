// Copyright (c) 2014-2017 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chainparams.h"
#include "dsnotificationinterface.h"
#include "instantx.h"
#include "governance.h"
#include "masternodeman.h"
#include "masternode-payments.h"
#include "masternode-sync.h"
#include "privatesend.h"
#ifdef ENABLE_WALLET
#include "privatesend-client.h"
#endif // ENABLE_WALLET
#include "txmempool.h"

void CDSNotificationInterface::InitializeCurrentBlockTip()
{
    LOCK(cs_main);
    UpdatedBlockTip(chainActive.Tip(), NULL, IsInitialBlockDownload());
}

void CDSNotificationInterface::AcceptedBlockHeader(const CBlockIndex *pindexNew)
{
    masternodeSync.AcceptedBlockHeader(pindexNew);
}

void CDSNotificationInterface::NotifyHeaderTip(const CBlockIndex *pindexNew, bool fInitialDownload)
{
    masternodeSync.NotifyHeaderTip(pindexNew, fInitialDownload, connman);
}

void CDSNotificationInterface::UpdatedBlockTip(const CBlockIndex *pindexNew, const CBlockIndex *pindexFork, bool fInitialDownload)
{
    if (pindexNew == pindexFork) // blocks were disconnected without any new ones
        return;

    masternodeSync.UpdatedBlockTip(pindexNew, fInitialDownload, connman);

    // DIP0001 updates

    bool fDIP0001ActiveAtTipTmp = fDIP0001ActiveAtTip;
    // Update global flags
    fDIP0001ActiveAtTip = (VersionBitsState(pindexNew, Params().GetConsensus(), Consensus::DEPLOYMENT_DIP0001, versionbitscache) == THRESHOLD_ACTIVE);
    fDIP0001WasLockedIn = fDIP0001ActiveAtTip || (VersionBitsState(pindexNew, Params().GetConsensus(), Consensus::DEPLOYMENT_DIP0001, versionbitscache) == THRESHOLD_LOCKED_IN);

    // Update min fees only if activation changed and we are using default fees
    if (fDIP0001ActiveAtTipTmp != fDIP0001ActiveAtTip) {
        if (!mapArgs.count("-minrelaytxfee")) {
            ::minRelayTxFee = CFeeRate(fDIP0001ActiveAtTip ? DEFAULT_DIP0001_MIN_RELAY_TX_FEE : DEFAULT_LEGACY_MIN_RELAY_TX_FEE);
            mempool.UpdateMinFee(::minRelayTxFee);
        }
#ifdef ENABLE_WALLET
        if (!mapArgs.count("-mintxfee")) {
            CWallet::minTxFee = CFeeRate(fDIP0001ActiveAtTip ? DEFAULT_DIP0001_TRANSACTION_MINFEE : DEFAULT_LEGACY_TRANSACTION_MINFEE);
        }
        if (!mapArgs.count("-fallbackfee")) {
            CWallet::fallbackFee = CFeeRate(fDIP0001ActiveAtTip ? DEFAULT_DIP0001_FALLBACK_FEE : DEFAULT_LEGACY_FALLBACK_FEE);
        }
#endif // ENABLE_WALLET
    }

    if (fInitialDownload)
        return;

    mnodeman.UpdatedBlockTip(pindexNew);
#ifdef ENABLE_WALLET
    privateSendClient.UpdatedBlockTip(pindexNew);
#endif // ENABLE_WALLET
    instantsend.UpdatedBlockTip(pindexNew);
    mnpayments.UpdatedBlockTip(pindexNew, connman);
    governance.UpdatedBlockTip(pindexNew, connman);
}

void CDSNotificationInterface::SyncTransaction(const CTransaction &tx, const CBlock *pblock)
{
    instantsend.SyncTransaction(tx, pblock);
    CPrivateSend::SyncTransaction(tx, pblock);
}
