// Copyright (c) 2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SYSCOIN_DSNOTIFICATIONINTERFACE_H
#define SYSCOIN_DSNOTIFICATIONINTERFACE_H

#include <validationinterface.h>
class ChainstateManager;
class PeerManager;
class CConnman;
class CDSNotificationInterface : public CValidationInterface
{
public:
    explicit CDSNotificationInterface(CConnman &connmanIn, PeerManager& peerManIn): peerman(peerManIn), connman(connmanIn) {}
    virtual ~CDSNotificationInterface() = default;

    // a small helper to initialize current block height in sub-modules on startup
    void InitializeCurrentBlockTip(ChainstateManager& chainman);

protected:
    // CValidationInterface
    void NotifyHeaderTip(const CBlockIndex *pindexNew) override;
    void UpdatedBlockTip(const CBlockIndex *pindexNew, const CBlockIndex *pindexFork, ChainstateManager& chainman, bool fInitialDownload) override;
    void NotifyMasternodeListChanged(bool undo, const CDeterministicMNList& oldMNList, const CDeterministicMNListDiff& diff) override;

private:
    PeerManager& peerman;
    CConnman& connman;
};

#endif // SYSCOIN_DSNOTIFICATIONINTERFACE_H
