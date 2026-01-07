// Copyright (c) 2018-2026 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_LLMQ_CONTEXT_H
#define BITCOIN_LLMQ_CONTEXT_H

#include <llmq/options.h>

#include <memory>

class CBLSWorker;
class ChainstateManager;
class CDeterministicMNManager;
class CEvoDB;
class CMasternodeSync;
class CSporkManager;
class CTxMemPool;
class PeerManager;

namespace chainlock {
class Chainlocks;
}

namespace llmq {
class CInstantSendManager;
class CQuorumBlockProcessor;
class CQuorumManager;
class CQuorumSnapshotManager;
class CSigningManager;
} // namespace llmq
namespace util {
struct DbWrapperParams;
} // namespace util

struct LLMQContext {
public:
    LLMQContext() = delete;
    LLMQContext(const LLMQContext&) = delete;
    LLMQContext& operator=(const LLMQContext&) = delete;
    explicit LLMQContext(CDeterministicMNManager& dmnman, CEvoDB& evo_db, CSporkManager& sporkman,
                         chainlock::Chainlocks& chainlocks, CTxMemPool& mempool, ChainstateManager& chainman,
                         const CMasternodeSync& mn_sync, const util::DbWrapperParams& db_params, int8_t bls_threads,
                         int64_t max_recsigs_age);
    ~LLMQContext();

    /** Guaranteed if LLMQContext is initialized then all members are valid too
     *
     *  Please note, that members here should not be re-ordered without careful consideration,
     *  because initialization some of them requires other member initialized.
     *  For example, constructor `qman` requires `bls_worker`.
     */
    const std::shared_ptr<CBLSWorker> bls_worker;
    const std::unique_ptr<llmq::CQuorumSnapshotManager> qsnapman;
    const std::unique_ptr<llmq::CQuorumBlockProcessor> quorum_block_processor;
    const std::unique_ptr<llmq::CQuorumManager> qman;
    const std::unique_ptr<llmq::CSigningManager> sigman;
    const std::unique_ptr<llmq::CInstantSendManager> isman;
};

#endif // BITCOIN_LLMQ_CONTEXT_H
