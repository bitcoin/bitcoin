// Copyright (c) 2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_MASTERNODE_ACTIVE_CONTEXT_H
#define BITCOIN_MASTERNODE_ACTIVE_CONTEXT_H

#include <memory>

class CChainState;
class CMasternodeSync;
class CSporkManager;
class CTxMemPool;
struct LLMQContext;
namespace chainlock {
class ChainLockSigner;
} // namespace chainlock
namespace instantsend {
class InstantSendSigner;
} // namespace instantsend

struct ActiveContext {
private:
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
    ActiveContext(CChainState& chainstate, LLMQContext& llmq_ctx, CSporkManager& sporkman, CTxMemPool& mempool,
                  const CMasternodeSync& mn_sync);
    ~ActiveContext();
};

#endif // BITCOIN_MASTERNODE_ACTIVE_CONTEXT_H
