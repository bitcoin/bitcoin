// Copyright (c) 2018-2024 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <llmq/context.h>

#include <dbwrapper.h>

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

LLMQContext::LLMQContext(CChainState& chainstate, CConnman& connman, CDeterministicMNManager& dmnman, CEvoDB& evo_db,
                         CMasternodeMetaMan& mn_metaman, CMNHFManager& mnhfman, CSporkManager& sporkman, CTxMemPool& mempool,
                         const CActiveMasternodeManager* const mn_activeman, const CMasternodeSync& mn_sync,
                         const std::unique_ptr<PeerManager>& peerman, bool unit_tests, bool wipe) :
    bls_worker{std::make_shared<CBLSWorker>()},
    dkg_debugman{std::make_unique<llmq::CDKGDebugManager>()},
    quorum_block_processor{[&]() -> llmq::CQuorumBlockProcessor* const {
        assert(llmq::quorumBlockProcessor == nullptr);
        llmq::quorumBlockProcessor = std::make_unique<llmq::CQuorumBlockProcessor>(chainstate, connman, dmnman, evo_db);
        return llmq::quorumBlockProcessor.get();
    }()},
    qdkgsman{std::make_unique<llmq::CDKGSessionManager>(*bls_worker, chainstate, connman, dmnman, *dkg_debugman, mn_metaman, *quorum_block_processor, mn_activeman, sporkman, unit_tests, wipe)},
    qman{[&]() -> llmq::CQuorumManager* const {
        assert(llmq::quorumManager == nullptr);
        llmq::quorumManager = std::make_unique<llmq::CQuorumManager>(*bls_worker, chainstate, connman, dmnman, *qdkgsman, evo_db, *quorum_block_processor, mn_activeman, mn_sync, sporkman);
        return llmq::quorumManager.get();
    }()},
    sigman{std::make_unique<llmq::CSigningManager>(connman, mn_activeman, *llmq::quorumManager, unit_tests, wipe)},
    shareman{std::make_unique<llmq::CSigSharesManager>(connman, *sigman, mn_activeman, *llmq::quorumManager, sporkman, peerman)},
    clhandler{[&]() -> llmq::CChainLocksHandler* const {
        assert(llmq::chainLocksHandler == nullptr);
        llmq::chainLocksHandler = std::make_unique<llmq::CChainLocksHandler>(chainstate, connman, *llmq::quorumManager, *sigman, *shareman, sporkman, mempool, mn_sync);
        return llmq::chainLocksHandler.get();
    }()},
    isman{[&]() -> llmq::CInstantSendManager* const {
        assert(llmq::quorumInstantSendManager == nullptr);
        llmq::quorumInstantSendManager = std::make_unique<llmq::CInstantSendManager>(*llmq::chainLocksHandler, chainstate, connman, *llmq::quorumManager, *sigman, *shareman, sporkman, mempool, mn_sync, unit_tests, wipe);
        return llmq::quorumInstantSendManager.get();
    }()},
    ehfSignalsHandler{std::make_unique<llmq::CEHFSignalsHandler>(chainstate, mnhfman, *sigman, *shareman, mempool, *llmq::quorumManager, sporkman, peerman)}
{
    // NOTE: we use this only to wipe the old db, do NOT use it for anything else
    // TODO: remove it in some future version
    auto llmqDbTmp = std::make_unique<CDBWrapper>(unit_tests ? "" : (GetDataDir() / "llmq"), 1 << 20, unit_tests, true);
}

LLMQContext::~LLMQContext() {
    // LLMQContext doesn't own these objects, but still need to care of them for consistency:
    llmq::quorumInstantSendManager.reset();
    llmq::chainLocksHandler.reset();
    llmq::quorumManager.reset();
    llmq::quorumBlockProcessor.reset();
}

void LLMQContext::Interrupt() {
    sigman->InterruptWorkerThread();
    shareman->InterruptWorkerThread();

    assert(isman == llmq::quorumInstantSendManager.get());
    llmq::quorumInstantSendManager->InterruptWorkerThread();
}

void LLMQContext::Start() {
    assert(quorum_block_processor == llmq::quorumBlockProcessor.get());
    assert(qman == llmq::quorumManager.get());
    assert(clhandler == llmq::chainLocksHandler.get());
    assert(isman == llmq::quorumInstantSendManager.get());

    bls_worker->Start();
    if (fMasternodeMode) {
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
    assert(quorum_block_processor == llmq::quorumBlockProcessor.get());
    assert(qman == llmq::quorumManager.get());
    assert(clhandler == llmq::chainLocksHandler.get());
    assert(isman == llmq::quorumInstantSendManager.get());

    llmq::quorumInstantSendManager->Stop();
    llmq::chainLocksHandler->Stop();

    shareman->StopWorkerThread();
    shareman->UnregisterAsRecoveredSigsListener();
    sigman->StopWorkerThread();
    qman->Stop();
    if (fMasternodeMode) {
        qdkgsman->StopThreads();
    }
    bls_worker->Stop();
}
