// Copyright (c) 2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_MASTERNODE_ACTIVE_NOTIFICATIONINTERFACE_H
#define BITCOIN_MASTERNODE_ACTIVE_NOTIFICATIONINTERFACE_H

#include <validationinterface.h>

class CActiveMasternodeManager;

class ActiveNotificationInterface final : public CValidationInterface
{
public:
    ActiveNotificationInterface() = delete;
    ActiveNotificationInterface(const ActiveNotificationInterface&) = delete;
    explicit ActiveNotificationInterface(CActiveMasternodeManager& mn_activeman);
    virtual ~ActiveNotificationInterface() = default;

protected:
    // CValidationInterface
    void UpdatedBlockTip(const CBlockIndex* pindexNew, const CBlockIndex* pindexFork, bool fInitialDownload) override;

private:
    CActiveMasternodeManager& m_mn_activeman;
};

extern std::unique_ptr<ActiveNotificationInterface> g_active_notification_interface;

#endif // BITCOIN_MASTERNODE_ACTIVE_NOTIFICATIONINTERFACE_H
