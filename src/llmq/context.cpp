// Copyright (c) 2018-2024 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <llmq/context.h>

#include <dbwrapper.h>
#include <validation.h>

#include <llmq/blockprocessor.h>
#include <llmq/chainlocks.h>
#include <llmq/commitment.h>
#include <llmq/debug.h>
#include <llmq/dkgsessionmgr.h>
#include <llmq/ehf_signals.h>
#include <llmq/instantsend.h>
#include <llmq/quorums.h>
#include <llmq/signing.h>
#include <llmq/signing_shares.h>

LLMQContext::LLMQContext(ChainstateManager& chainman, CConnman& connman, CDeterministicMNManager& dmnman,
                         CEvoDB& evo_db, CMasternodeMetaMan& mn_metaman, CMNHFManager& mnhfman, CSporkManager& sporkman,
                         CTxMemPool& mempool, const CActiveMasternodeManager* const mn_activeman,
                         const CMasternodeSync& mn_sync, const std::unique_ptr<PeerManager>& peerman, bool unit_tests,
                         bool wipe) :
    is_masternode{mn_activeman != nullptr},
    bls_worker{std::make_shared<CBLSWorker>()},
    dkg_debugman{std::make_unique<llmq::CDKGDebugManager>()},
    quorum_block_processor{std::make_unique<llmq::CQuorumBlockProcessor>(chainman.ActiveChainstate(), dmnman, evo_db)},
    qdkgsman{std::make_unique<llmq::CDKGSessionManager>(*bls_worker, chainman.ActiveChainstate(), connman, dmnman,
                                                        *dkg_debugman, mn_metaman, *quorum_block_processor,
                                                        mn_activeman, sporkman, peerman, unit_tests, wipe)},
    qman{std::make_unique<llmq::CQuorumManager>(*bls_worker, chainman.ActiveChainstate(), connman, dmnman, *qdkgsman,
                                                evo_db, *quorum_block_processor, mn_activeman, mn_sync, sporkman,
                                                unit_tests, wipe)},
    sigman{std::make_unique<llmq::CSigningManager>(connman, mn_activeman, chainman.ActiveChainstate(), *qman, peerman,
                                                   unit_tests, wipe)},
    shareman{std::make_unique<llmq::CSigSharesManager>(connman, *sigman, mn_activeman, *qman, sporkman, peerman)},
    clhandler{[&]() -> llmq::CChainLocksHandler* const {
        assert(llmq::chainLocksHandler == nullptr);
        llmq::chainLocksHandler = std::make_unique<llmq::CChainLocksHandler>(chainman.ActiveChainstate(), *qman,
                                                                             *sigman, *shareman, sporkman, mempool,
                                                                             mn_sync, is_masternode);
        return llmq::chainLocksHandler.get();
    }()},
    isman{[&]() -> llmq::CInstantSendManager* const {
        assert(llmq::quorumInstantSendManager == nullptr);
        llmq::quorumInstantSendManager = std::make_unique<llmq::CInstantSendManager>(*llmq::chainLocksHandler,
                                                                                     chainman.ActiveChainstate(),
                                                                                     connman, *qman, *sigman, *shareman,
                                                                                     sporkman, mempool, mn_sync, peerman,
                                                                                     is_masternode, unit_tests, wipe);
        return llmq::quorumInstantSendManager.get();
    }()},
    ehfSignalsHandler{std::make_unique<llmq::CEHFSignalsHandler>(chainman, mnhfman, *sigman, *shareman, *qman)}
{
}

LLMQContext::~LLMQContext() {
    // LLMQContext doesn't own these objects, but still need to care of them for consistency:
    llmq::quorumInstantSendManager.reset();
    llmq::chainLocksHandler.reset();
}

void LLMQContext::Interrupt() {
    sigman->InterruptWorkerThread();
    shareman->InterruptWorkerThread();

    assert(isman == llmq::quorumInstantSendManager.get());
    llmq::quorumInstantSendManager->InterruptWorkerThread();
}

void LLMQContext::Start() {
    assert(clhandler == llmq::chainLocksHandler.get());
    assert(isman == llmq::quorumInstantSendManager.get());

    bls_worker->Start();
    if (is_masternode) {
        qdkgsman->StartThreads();
    }
    qman->Start();
    shareman->RegisterAsRecoveredSigsListener();
    shareman->StartWorkerThread();
    sigman->StartWorkerThread();

    llmq::chainLocksHandler->Start();
    llmq::quorumInstantSendManager->Start();
}

void LLMQContext::Stop() {
    assert(clhandler == llmq::chainLocksHandler.get());
    assert(isman == llmq::quorumInstantSendManager.get());

    llmq::quorumInstantSendManager->Stop();
    llmq::chainLocksHandler->Stop();

    shareman->StopWorkerThread();
    shareman->UnregisterAsRecoveredSigsListener();
    sigman->StopWorkerThread();
    qman->Stop();
    if (is_masternode) {
        qdkgsman->StopThreads();
    }
    bls_worker->Stop();
}
