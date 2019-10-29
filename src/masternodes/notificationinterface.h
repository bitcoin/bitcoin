// Copyright (c) 2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITGREEN_MASTERNODES_NOTIFICATIONINTERFACE_H
#define BITGREEN_MASTERNODES_NOTIFICATIONINTERFACE_H

#include <validationinterface.h>

class CBlockIndex;

class CMNNotificationInterface : public CValidationInterface
{
public:
    CMNNotificationInterface(CConnman& connmanIn): connman(connmanIn) {}
    virtual ~CMNNotificationInterface() = default;

    // a small helper to initialize current block height in sub-modules on startup
    void InitializeCurrentBlockTip();

protected:
    // CValidationInterface
    void AcceptedBlockHeader(const CBlockIndex *pindexNew) ;
    void NotifyHeaderTip(const CBlockIndex *pindexNew, bool fInitialDownload) ;
    void UpdatedBlockTip(const CBlockIndex *pindexNew, const CBlockIndex *pindexFork, bool fInitialDownload) ;
    void SyncTransaction(const CTransaction &tx, const CBlockIndex *pindex, int posInBlock) ;
    void NotifyMasternodeListChanged(bool undo, const CDeterministicMNList& oldMNList, const CDeterministicMNListDiff& diff) ;
    void NotifyChainLock(const CBlockIndex* pindex) ;

private:
    CConnman& connman;
};

extern CMNNotificationInterface* g_mn_notification_interface;

#endif // BITGREEN_MASTERNODES_NOTIFICATIONINTERFACE_H
