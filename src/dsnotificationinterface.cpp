// Copyright (c) 2014-2017 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "dsnotificationinterface.h"
#include "instantx.h"
#include "governance.h"
#include "masternodeman.h"
#include "masternode-payments.h"
#include "masternode-sync.h"
#include "privatesend-client.h"

void CDSNotificationInterface::InitializeCurrentBlockTip()
{
    LOCK(cs_main);
    UpdatedBlockTip(chainActive.Tip(), NULL, IsInitialBlockDownload());
}

void CDSNotificationInterface::UpdatedBlockTip(const CBlockIndex *pindexNew, const CBlockIndex *pindexFork, bool fInitialDownload)
{
    if (fInitialDownload || pindexNew == pindexFork) // In IBD or blocks were disconnected without any new ones
        return;

    mnodeman.UpdatedBlockTip(pindexNew);
    privateSendClient.UpdatedBlockTip(pindexNew);
    instantsend.UpdatedBlockTip(pindexNew);
    mnpayments.UpdatedBlockTip(pindexNew);
    governance.UpdatedBlockTip(pindexNew);
    masternodeSync.UpdatedBlockTip(pindexNew, fInitialDownload);
}

void CDSNotificationInterface::SyncTransaction(const CTransaction &tx, const CBlock *pblock)
{
    instantsend.SyncTransaction(tx, pblock);
    CPrivateSend::SyncTransaction(tx, pblock);
}