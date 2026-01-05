// Copyright (c) 2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_MASTERNODE_ACTIVE_NOTIFICATIONINTERFACE_H
#define BITCOIN_MASTERNODE_ACTIVE_NOTIFICATIONINTERFACE_H

#include <validationinterface.h>

#include <memory>

class CActiveMasternodeManager;
struct ActiveContext;
namespace llmq {
class CRecoveredSig;
} // namespace llmq

class ActiveNotificationInterface final : public CValidationInterface
{
public:
    ActiveNotificationInterface() = delete;
    ActiveNotificationInterface(const ActiveNotificationInterface&) = delete;
    ActiveNotificationInterface& operator=(const ActiveNotificationInterface&) = delete;
    explicit ActiveNotificationInterface(ActiveContext& active_ctx);
    virtual ~ActiveNotificationInterface();

protected:
    // CValidationInterface
    void NotifyRecoveredSig(const std::shared_ptr<const llmq::CRecoveredSig>& sig, bool proactive_relay) override;
    void UpdatedBlockTip(const CBlockIndex* pindexNew, const CBlockIndex* pindexFork, bool fInitialDownload) override;

private:
    ActiveContext& m_active_ctx;
};

extern std::unique_ptr<ActiveNotificationInterface> g_active_notification_interface;

#endif // BITCOIN_MASTERNODE_ACTIVE_NOTIFICATIONINTERFACE_H
