// Copyright (c) 2018-2024 The Dash Core developers
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
#include <llmq/ehf_signals.h>
#include <llmq/instantsend.h>
#include <llmq/quorums.h>
#include <llmq/signing.h>
#include <llmq/signing_shares.h>
#include <masternode/sync.h>

LLMQContext::LLMQContext(CChainState& chainstate, CConnman& connman, CEvoDB& evo_db, CSporkManager& sporkman, CTxMemPool& mempool,
                         const std::unique_ptr<PeerManager>& peerman, bool unit_tests, bool wipe) :
    bls_worker{std::make_shared<CBLSWorker>()},
    dkg_debugman{std::make_unique<llmq::CDKGDebugManager>()},
    quorum_block_processor{[&]() -> llmq::CQuorumBlockProcessor* const {
        assert(llmq::quorumBlockProcessor == nullptr);
        llmq::quorumBlockProcessor = std::make_unique<llmq::CQuorumBlockProcessor>(chainstate, connman, evo_db);
        return llmq::quorumBlockProcessor.get();
    }()},
    qdkgsman{std::make_unique<llmq::CDKGSessionManager>(*bls_worker, chainstate, connman, *dkg_debugman, *quorum_block_processor, sporkman, unit_tests, wipe)},
    qman{[&]() -> llmq::CQuorumManager* const {
        assert(llmq::quorumManager == nullptr);
        llmq::quorumManager = std::make_unique<llmq::CQuorumManager>(*bls_worker, chainstate, connman, *qdkgsman, evo_db, *quorum_block_processor, ::masternodeSync);
        return llmq::quorumManager.get();
    }()},
    sigman{std::make_unique<llmq::CSigningManager>(connman, *llmq::quorumManager, unit_tests, wipe)},
    shareman{std::make_unique<llmq::CSigSharesManager>(connman, *llmq::quorumManager, *sigman, peerman)},
    clhandler{[&]() -> llmq::CChainLocksHandler* const {
        assert(llmq::chainLocksHandler == nullptr);
        llmq::chainLocksHandler = std::make_unique<llmq::CChainLocksHandler>(chainstate, connman, *::masternodeSync, *llmq::quorumManager, *sigman, *shareman, sporkman, mempool);
        return llmq::chainLocksHandler.get();
    }()},
    isman{[&]() -> llmq::CInstantSendManager* const {
        assert(llmq::quorumInstantSendManager == nullptr);
        llmq::quorumInstantSendManager = std::make_unique<llmq::CInstantSendManager>(*llmq::chainLocksHandler, chainstate, connman, *llmq::quorumManager, *sigman, *shareman, sporkman, mempool, *::masternodeSync, unit_tests, wipe);
        return llmq::quorumInstantSendManager.get();
    }()},
    ehfSignalsHandler{std::make_unique<llmq::CEHFSignalsHandler>(chainstate, connman, *sigman, *shareman, sporkman, *llmq::quorumManager, mempool)}
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
    qdkgsman->StartThreads();
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
    qdkgsman->StopThreads();
    bls_worker->Stop();
}
