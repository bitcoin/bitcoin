// Copyright (c) 2018-2025 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <llmq/context.h>

#include <bls/bls_worker.h>
#include <chainlock/chainlock.h>
#include <instantsend/instantsend.h>
#include <llmq/blockprocessor.h>
#include <llmq/debug.h>
#include <llmq/dkgsessionmgr.h>
#include <llmq/quorums.h>
#include <llmq/signing.h>
#include <llmq/snapshot.h>
#include <validation.h>

LLMQContext::LLMQContext(ChainstateManager& chainman, CDeterministicMNManager& dmnman, CEvoDB& evo_db,
                         CMasternodeMetaMan& mn_metaman, CMNHFManager& mnhfman, CSporkManager& sporkman,
                         CTxMemPool& mempool, const CActiveMasternodeManager* const mn_activeman,
                         const CMasternodeSync& mn_sync, const util::DbWrapperParams& db_params) :
    bls_worker{std::make_shared<CBLSWorker>()},
    dkg_debugman{std::make_unique<llmq::CDKGDebugManager>()},
    qsnapman{std::make_unique<llmq::CQuorumSnapshotManager>(evo_db)},
    quorum_block_processor{
        std::make_unique<llmq::CQuorumBlockProcessor>(chainman.ActiveChainstate(), dmnman, evo_db, *qsnapman)},
    qdkgsman{std::make_unique<llmq::CDKGSessionManager>(*bls_worker, chainman.ActiveChainstate(), dmnman, *dkg_debugman,
                                                        mn_metaman, *quorum_block_processor, *qsnapman, mn_activeman,
                                                        sporkman, db_params)},
    qman{std::make_unique<llmq::CQuorumManager>(*bls_worker, chainman.ActiveChainstate(), dmnman, *qdkgsman, evo_db,
                                                *quorum_block_processor, *qsnapman, mn_activeman, mn_sync, sporkman,
                                                db_params)},
    sigman{std::make_unique<llmq::CSigningManager>(*qman, db_params)},
    clhandler{std::make_unique<llmq::CChainLocksHandler>(chainman.ActiveChainstate(), *qman, sporkman, mempool, mn_sync)},
    isman{std::make_unique<llmq::CInstantSendManager>(*clhandler, chainman.ActiveChainstate(), *sigman, sporkman,
                                                      mempool, mn_sync, db_params)}
{
    // Have to start it early to let VerifyDB check ChainLock signatures in coinbase
    bls_worker->Start();
}

LLMQContext::~LLMQContext() {
    bls_worker->Stop();
}

void LLMQContext::Interrupt() {
}

void LLMQContext::Start(PeerManager& peerman)
{
    qman->Start();
    clhandler->Start(*isman);
}

void LLMQContext::Stop()
{
    clhandler->Stop();
    qman->Stop();
}
