// Copyright (c) 2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <masternode/active/notificationinterface.h>

#include <governance/signing.h>
#include <llmq/ehf_signals.h>
#include <llmq/signing_shares.h>
#include <masternode/active/context.h>
#include <masternode/node.h>

ActiveNotificationInterface::ActiveNotificationInterface(ActiveContext& active_ctx, CActiveMasternodeManager& mn_activeman) :
    m_active_ctx{active_ctx},
    m_mn_activeman{mn_activeman}
{
}

ActiveNotificationInterface::~ActiveNotificationInterface() = default;

void ActiveNotificationInterface::UpdatedBlockTip(const CBlockIndex* pindexNew, const CBlockIndex* pindexFork,
                                                  bool fInitialDownload)
{
    if (fInitialDownload || pindexNew == pindexFork) // In IBD or blocks were disconnected without any new ones
        return;

    m_mn_activeman.UpdatedBlockTip(pindexNew, pindexFork, fInitialDownload);
    m_active_ctx.ehf_sighandler->UpdatedBlockTip(pindexNew);
    m_active_ctx.gov_signer->UpdatedBlockTip(pindexNew);
}

void ActiveNotificationInterface::NotifyRecoveredSig(const std::shared_ptr<const llmq::CRecoveredSig>& sig, bool proactive_relay)
{
    m_active_ctx.shareman->NotifyRecoveredSig(sig, proactive_relay);
}

std::unique_ptr<ActiveNotificationInterface> g_active_notification_interface;
