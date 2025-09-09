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
#include <llmq/ehf_signals.h>
#include <llmq/quorums.h>
#include <llmq/signing.h>
#include <llmq/signing_shares.h>
#include <llmq/snapshot.h>
#include <validation.h>

LLMQContext::LLMQContext(ChainstateManager& chainman, CDeterministicMNManager& dmnman, CEvoDB& evo_db,
                         CMasternodeMetaMan& mn_metaman, CMNHFManager& mnhfman, CSporkManager& sporkman,
                         CTxMemPool& mempool, const CActiveMasternodeManager* const mn_activeman,
                         const CMasternodeSync& mn_sync, bool unit_tests, bool wipe) :
    is_masternode{mn_activeman != nullptr},
    bls_worker{std::make_shared<CBLSWorker>()},
    dkg_debugman{std::make_unique<llmq::CDKGDebugManager>()},
    qsnapman{std::make_unique<llmq::CQuorumSnapshotManager>(evo_db)},
    quorum_block_processor{
        std::make_unique<llmq::CQuorumBlockProcessor>(chainman.ActiveChainstate(), dmnman, evo_db, *qsnapman)},
    qdkgsman{std::make_unique<llmq::CDKGSessionManager>(*bls_worker, chainman.ActiveChainstate(), dmnman, *dkg_debugman,
                                                        mn_metaman, *quorum_block_processor, *qsnapman, mn_activeman,
                                                        sporkman, unit_tests, wipe)},
    qman{std::make_unique<llmq::CQuorumManager>(*bls_worker, chainman.ActiveChainstate(), dmnman, *qdkgsman, evo_db,
                                                *quorum_block_processor, *qsnapman, mn_activeman, mn_sync, sporkman,
                                                unit_tests, wipe)},
    sigman{std::make_unique<llmq::CSigningManager>(mn_activeman, chainman.ActiveChainstate(), *qman, unit_tests, wipe)},
    shareman{std::make_unique<llmq::CSigSharesManager>(*sigman, mn_activeman, *qman, sporkman)},
    clhandler{std::make_unique<llmq::CChainLocksHandler>(chainman.ActiveChainstate(), *qman, *sigman, *shareman,
                                                         sporkman, mempool, mn_sync, is_masternode)},
    isman{std::make_unique<llmq::CInstantSendManager>(*clhandler, chainman.ActiveChainstate(), *qman, *sigman, sporkman,
                                                      mempool, mn_sync, unit_tests, wipe)},
    ehfSignalsHandler{std::make_unique<llmq::CEHFSignalsHandler>(chainman, mnhfman, *sigman, *shareman, *qman)}
{
    // Have to start it early to let VerifyDB check ChainLock signatures in coinbase
    bls_worker->Start();
}

LLMQContext::~LLMQContext() {
    bls_worker->Stop();
}

void LLMQContext::Interrupt() {
    isman->InterruptWorkerThread();
    shareman->InterruptWorkerThread();
    sigman->InterruptWorkerThread();
}

void LLMQContext::Start(CConnman& connman, PeerManager& peerman)
{
    if (is_masternode) {
        qdkgsman->StartThreads(connman, peerman);
    }
    qman->Start();
    sigman->StartWorkerThread(peerman);
    shareman->RegisterAsRecoveredSigsListener();
    shareman->StartWorkerThread(connman, peerman);
    clhandler->Start(*isman);
    isman->Start(peerman);
}

void LLMQContext::Stop() {
    isman->Stop();
    clhandler->Stop();
    shareman->StopWorkerThread();
    shareman->UnregisterAsRecoveredSigsListener();
    sigman->StopWorkerThread();
    qman->Stop();
    if (is_masternode) {
        qdkgsman->StopThreads();
    }
}
