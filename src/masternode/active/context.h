// Copyright (c) 2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_MASTERNODE_ACTIVE_CONTEXT_H
#define BITCOIN_MASTERNODE_ACTIVE_CONTEXT_H

#include <memory>

class CActiveMasternodeManager;
class ChainstateManager;
class CCoinJoinServer;
class CConnman;
class CDeterministicMNManager;
class CDSTXManager;
class CMasternodeMetaMan;
class CMasternodeSync;
class CSporkManager;
class CTxMemPool;
class PeerManager;
struct LLMQContext;
namespace chainlock {
class ChainLockSigner;
} // namespace chainlock
namespace instantsend {
class InstantSendSigner;
} // namespace instantsend

struct ActiveContext {
private:
    // TODO: Switch to references to members when migration is finished
    LLMQContext& m_llmq_ctx;

    /*
     * Entities that are registered with LLMQContext members are not accessible
     * and are managed with (Dis)connectSigner() in the (c/d)tor instead
     */
    const std::unique_ptr<chainlock::ChainLockSigner> cl_signer;
    const std::unique_ptr<instantsend::InstantSendSigner> is_signer;

public:
    ActiveContext() = delete;
    ActiveContext(const ActiveContext&) = delete;
    ActiveContext(ChainstateManager& chainman, CConnman& connman, CDeterministicMNManager& dmnman, CDSTXManager& dstxman,
                  CMasternodeMetaMan& mn_metaman, LLMQContext& llmq_ctx, CSporkManager& sporkman, CTxMemPool& mempool,
                  PeerManager& peerman, const CActiveMasternodeManager& mn_activeman, const CMasternodeSync& mn_sync);
    ~ActiveContext();

    /*
     * Entities that are only utilized when masternode mode is enabled
     * and are accessible in their own right
     * TODO: Move CActiveMasternodeManager here when dependents have been migrated
     */
    const std::unique_ptr<CCoinJoinServer> cj_server;
};

#endif // BITCOIN_MASTERNODE_ACTIVE_CONTEXT_H
