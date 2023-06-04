// Copyright (c) 2018-2023 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <llmq/context.h>

#include <consensus/validation.h>
#include <dbwrapper.h>

#include <llmq/blockprocessor.h>
#include <llmq/chainlocks.h>
#include <llmq/commitment.h>
#include <llmq/debug.h>
#include <llmq/dkgsessionmgr.h>
#include <llmq/instantsend.h>
#include <llmq/quorums.h>
#include <llmq/signing.h>
#include <llmq/signing_shares.h>
#include <llmq/utils.h>
#include <masternode/sync.h>

LLMQContext::LLMQContext(CEvoDB& evo_db, CTxMemPool& mempool, CConnman& connman, CSporkManager& sporkman,
                         const std::unique_ptr<PeerManager>& peerman, bool unit_tests, bool wipe) {
    Create(evo_db, mempool, connman, sporkman, peerman, unit_tests, wipe);

    /* Context aliases to globals used by the LLMQ system */
    quorum_block_processor = llmq::quorumBlockProcessor.get();
    qman = llmq::quorumManager.get();
    clhandler = llmq::chainLocksHandler.get();
    isman = llmq::quorumInstantSendManager.get();
}

LLMQContext::~LLMQContext() {
    isman = nullptr;
    clhandler = nullptr;
    qman = nullptr;
    quorum_block_processor = nullptr;

    Destroy();
}

void LLMQContext::Create(CEvoDB& evo_db, CTxMemPool& mempool, CConnman& connman, CSporkManager& sporkman,
                         const std::unique_ptr<PeerManager>& peerman, bool unit_tests, bool wipe) {
    bls_worker = std::make_shared<CBLSWorker>();

    dkg_debugman = std::make_unique<llmq::CDKGDebugManager>();
    llmq::quorumBlockProcessor = std::make_unique<llmq::CQuorumBlockProcessor>(evo_db, connman, peerman);
    qdkgsman = std::make_unique<llmq::CDKGSessionManager>(connman, *bls_worker, *dkg_debugman, *llmq::quorumBlockProcessor, sporkman, peerman, unit_tests, wipe);
    llmq::quorumManager = std::make_unique<llmq::CQuorumManager>(evo_db, connman, *bls_worker, *llmq::quorumBlockProcessor, *qdkgsman, ::masternodeSync, peerman);
    sigman = std::make_unique<llmq::CSigningManager>(connman, *llmq::quorumManager, peerman, unit_tests, wipe);
    shareman = std::make_unique<llmq::CSigSharesManager>(connman, *llmq::quorumManager, *sigman, peerman);
    llmq::chainLocksHandler = std::make_unique<llmq::CChainLocksHandler>(mempool, connman, sporkman, *sigman, *shareman, *llmq::quorumManager, *::masternodeSync, peerman);
    llmq::quorumInstantSendManager = std::make_unique<llmq::CInstantSendManager>(mempool, connman, sporkman, *llmq::quorumManager, *sigman, *shareman, *llmq::chainLocksHandler, *::masternodeSync, peerman, unit_tests, wipe);

    // NOTE: we use this only to wipe the old db, do NOT use it for anything else
    // TODO: remove it in some future version
    auto llmqDbTmp = std::make_unique<CDBWrapper>(unit_tests ? "" : (GetDataDir() / "llmq"), 1 << 20, unit_tests, true);
}

void LLMQContext::Destroy() {
    llmq::quorumInstantSendManager.reset();
    llmq::chainLocksHandler.reset();
    shareman.reset();
    sigman.reset();
    llmq::quorumManager.reset();
    qdkgsman.reset();
    llmq::quorumBlockProcessor.reset();
    dkg_debugman.reset();
    bls_worker.reset();
    {
        LOCK(llmq::cs_llmq_vbc);
        llmq::llmq_versionbitscache.Clear();
    }
}

void LLMQContext::Interrupt() {
    if (shareman != nullptr) {
        shareman->InterruptWorkerThread();
    }
    if (llmq::quorumInstantSendManager != nullptr) {
        llmq::quorumInstantSendManager->InterruptWorkerThread();
    }
}

void LLMQContext::Start() {
    if (bls_worker != nullptr) {
        bls_worker->Start();
    }
    if (qdkgsman != nullptr) {
        qdkgsman->StartThreads();
    }
    if (llmq::quorumManager != nullptr) {
        llmq::quorumManager->Start();
    }
    if (shareman != nullptr) {
        shareman->RegisterAsRecoveredSigsListener();
        shareman->StartWorkerThread();
    }
    if (llmq::chainLocksHandler != nullptr) {
        llmq::chainLocksHandler->Start();
    }
    if (llmq::quorumInstantSendManager != nullptr) {
        llmq::quorumInstantSendManager->Start();
    }
}

void LLMQContext::Stop() {
    if (llmq::quorumInstantSendManager != nullptr) {
        llmq::quorumInstantSendManager->Stop();
    }
    if (llmq::chainLocksHandler != nullptr) {
        llmq::chainLocksHandler->Stop();
    }
    if (shareman != nullptr) {
        shareman->StopWorkerThread();
        shareman->UnregisterAsRecoveredSigsListener();
    }
    if (llmq::quorumManager != nullptr) {
        llmq::quorumManager->Stop();
    }
    if (qdkgsman != nullptr) {
        qdkgsman->StopThreads();
    }
    if (bls_worker != nullptr) {
        bls_worker->Stop();
    }
}
