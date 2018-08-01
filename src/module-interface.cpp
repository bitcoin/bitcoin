// Copyright (c) 2014-2017 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>
#include <init.h>
#include <module-interface.h>
#include <governance.h>
#include <masternodeman.h>
#include <masternode-payments.h>
#include <masternode-sync.h>
#include <privatesend.h>

void CDSNotificationInterface::InitializeCurrentBlockTip()
{
    LOCK(cs_main);
    UpdatedBlockTip(chainActive.Tip(), nullptr, IsInitialBlockDownload());
}

void CDSNotificationInterface::NewPoWValidBlock(const CBlockIndex *pindex, const std::shared_ptr<const CBlock> &block)
{
    masternodeSync.NewPoWValidBlock(pindex, IsInitialBlockDownload(), connman);
}

void CDSNotificationInterface::UpdatedBlockTip(const CBlockIndex *pindexNew, const CBlockIndex *pindexFork, bool fInitialDownload)
{
    if (pindexNew == pindexFork) // blocks were disconnected without any new ones
        return;

    masternodeSync.UpdatedBlockTip(pindexNew, fInitialDownload, connman);

    if (fInitialDownload)
        return;

    if (fLiteMode)
        return;

    mnodeman.UpdatedBlockTip(pindexNew);
    CPrivateSend::UpdatedBlockTip(pindexNew);
    g_module_interface->UpdatedBlockTip(pindexNew);
    mnpayments.UpdatedBlockTip(pindexNew, connman);
    governance.UpdatedBlockTip(pindexNew, connman);
}

void CDSNotificationInterface::TransactionAddedToMempool(const CTransactionRef &tx)
{
    CPrivateSend::TransactionAddedToMempool(tx);
}
