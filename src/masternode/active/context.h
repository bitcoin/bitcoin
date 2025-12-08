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
class CGovernanceManager;
class CMasternodeMetaMan;
class CMasternodeSync;
class CMNHFManager;
class CSporkManager;
class CTxMemPool;
class GovernanceSigner;
class PeerManager;
struct LLMQContext;
namespace chainlock {
class ChainLockSigner;
} // namespace chainlock
namespace instantsend {
class InstantSendSigner;
} // namespace instantsend
namespace llmq {
class CDKGDebugManager;
class CDKGSessionManager;
class CEHFSignalsHandler;
class CSigSharesManager;
} // namespace llmq
namespace util {
struct DbWrapperParams;
} // namespace util

struct ActiveContext {
private:
    // TODO: Switch to references to members when migration is finished
    LLMQContext& m_llmq_ctx;

public:
    ActiveContext() = delete;
    ActiveContext(const ActiveContext&) = delete;
    ActiveContext& operator=(const ActiveContext&) = delete;
    explicit ActiveContext(CCoinJoinServer& cj_server, CConnman& connman, CDeterministicMNManager& dmnman,
                           CGovernanceManager& govman, ChainstateManager& chainman, CMasternodeMetaMan& mn_metaman,
                           CMNHFManager& mnhfman, CSporkManager& sporkman, CTxMemPool& mempool, LLMQContext& llmq_ctx,
                           PeerManager& peerman, const CActiveMasternodeManager& mn_activeman,
                           const CMasternodeSync& mn_sync, const util::DbWrapperParams& db_params, bool quorums_watch);
    ~ActiveContext();

    void Interrupt();
    void Start(CConnman& connman, PeerManager& peerman);
    void Stop();

    /*
     * Entities that are only utilized when masternode mode is enabled
     * and are accessible in their own right
     * TODO: Move CActiveMasternodeManager here when dependents have been migrated
     */
    CCoinJoinServer& m_cj_server;
    const std::unique_ptr<GovernanceSigner> gov_signer;
    const std::unique_ptr<llmq::CDKGDebugManager> dkgdbgman;
    const std::unique_ptr<llmq::CDKGSessionManager> qdkgsman;
    const std::unique_ptr<llmq::CSigSharesManager> shareman;
    const std::unique_ptr<llmq::CEHFSignalsHandler> ehf_sighandler;

private:
    /*
     * Entities that are registered with LLMQContext members are not accessible
     * and are managed with (Dis)connectSigner() in the (c/d)tor instead
     */
    const std::unique_ptr<chainlock::ChainLockSigner> cl_signer;
    const std::unique_ptr<instantsend::InstantSendSigner> is_signer;
};

#endif // BITCOIN_MASTERNODE_ACTIVE_CONTEXT_H
