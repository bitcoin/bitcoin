// Copyright (c) 2018-2026 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <llmq/context.h>

#include <bls/bls_worker.h>
#include <instantsend/instantsend.h>
#include <llmq/blockprocessor.h>
#include <llmq/quorumsman.h>
#include <llmq/signing.h>
#include <llmq/snapshot.h>
#include <validation.h>

LLMQContext::LLMQContext(CDeterministicMNManager& dmnman, CEvoDB& evo_db, CSporkManager& sporkman,
                         chainlock::Chainlocks& chainlocks, CTxMemPool& mempool, ChainstateManager& chainman,
                         const CMasternodeSync& mn_sync, const util::DbWrapperParams& db_params, int8_t bls_threads,
                         int64_t max_recsigs_age) :
    bls_worker{std::make_shared<CBLSWorker>()},
    qsnapman{std::make_unique<llmq::CQuorumSnapshotManager>(evo_db)},
    quorum_block_processor{std::make_unique<llmq::CQuorumBlockProcessor>(chainman.ActiveChainstate(), dmnman, evo_db,
                                                                         *qsnapman, bls_threads)},
    qman{std::make_unique<llmq::CQuorumManager>(*bls_worker, dmnman, evo_db, *quorum_block_processor, *qsnapman,
                                                chainman, db_params)},
    sigman{std::make_unique<llmq::CSigningManager>(*qman, db_params, max_recsigs_age)},
    isman{std::make_unique<llmq::CInstantSendManager>(chainlocks, chainman.ActiveChainstate(), *sigman, sporkman,
                                                      mempool, mn_sync, db_params)}
{
    // Have to start it early to let VerifyDB check ChainLock signatures in coinbase
    bls_worker->Start();
}

LLMQContext::~LLMQContext()
{
    bls_worker->Stop();
}
