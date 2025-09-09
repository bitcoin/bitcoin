// Copyright (c) 2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <masternode/active/notificationinterface.h>

#include <masternode/node.h>

ActiveNotificationInterface::ActiveNotificationInterface(CActiveMasternodeManager& mn_activeman) :
    m_mn_activeman{mn_activeman}
{
}

void ActiveNotificationInterface::UpdatedBlockTip(const CBlockIndex* pindexNew, const CBlockIndex* pindexFork,
                                                  bool fInitialDownload)
{
    if (fInitialDownload || pindexNew == pindexFork) // In IBD or blocks were disconnected without any new ones
        return;

    m_mn_activeman.UpdatedBlockTip(pindexNew, pindexFork, fInitialDownload);
}

std::unique_ptr<ActiveNotificationInterface> g_active_notification_interface;
