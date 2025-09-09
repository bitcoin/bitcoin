// Copyright (c) 2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <masternode/active/notificationinterface.h>

#include <llmq/ehf_signals.h>
#include <masternode/active/context.h>
#include <masternode/node.h>

ActiveNotificationInterface::ActiveNotificationInterface(ActiveContext& active_ctx, CActiveMasternodeManager& mn_activeman) :
    m_active_ctx{active_ctx},
    m_mn_activeman{mn_activeman}
{
}

void ActiveNotificationInterface::UpdatedBlockTip(const CBlockIndex* pindexNew, const CBlockIndex* pindexFork,
                                                  bool fInitialDownload)
{
    if (fInitialDownload || pindexNew == pindexFork) // In IBD or blocks were disconnected without any new ones
        return;

    m_mn_activeman.UpdatedBlockTip(pindexNew, pindexFork, fInitialDownload);
    m_active_ctx.ehf_sighandler->UpdatedBlockTip(pindexNew);
}

std::unique_ptr<ActiveNotificationInterface> g_active_notification_interface;
