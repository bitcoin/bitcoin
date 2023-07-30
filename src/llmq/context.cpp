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
                         const std::unique_ptr<PeerManager>& peerman, bool unit_tests, bool wipe) :
    bls_worker{std::make_shared<CBLSWorker>()},
    dkg_debugman{std::make_unique<llmq::CDKGDebugManager>()},
    quorum_block_processor{[&]() -> llmq::CQuorumBlockProcessor* const {
        assert(llmq::quorumBlockProcessor == nullptr);
        llmq::quorumBlockProcessor = std::make_unique<llmq::CQuorumBlockProcessor>(evo_db, connman, peerman);
        return llmq::quorumBlockProcessor.get();
    }()},
    qdkgsman{std::make_unique<llmq::CDKGSessionManager>(connman, *bls_worker, *dkg_debugman, *quorum_block_processor, sporkman, peerman, unit_tests, wipe)},
    qman{[&]() -> llmq::CQuorumManager* const {
        assert(llmq::quorumManager == nullptr);
        llmq::quorumManager = std::make_unique<llmq::CQuorumManager>(evo_db, connman, *bls_worker, *quorum_block_processor, *qdkgsman, ::masternodeSync, peerman);
        return llmq::quorumManager.get();
    }()},
    sigman{std::make_unique<llmq::CSigningManager>(connman, *llmq::quorumManager, peerman, unit_tests, wipe)},
    shareman{std::make_unique<llmq::CSigSharesManager>(connman, *llmq::quorumManager, *sigman, peerman)},
    clhandler{[&]() -> llmq::CChainLocksHandler* const {
        assert(llmq::chainLocksHandler == nullptr);
        llmq::chainLocksHandler = std::make_unique<llmq::CChainLocksHandler>(mempool, connman, sporkman, *sigman, *shareman, *llmq::quorumManager, *::masternodeSync, peerman);
        return llmq::chainLocksHandler.get();
    }()},
    isman{[&]() -> llmq::CInstantSendManager* const {
        assert(llmq::quorumInstantSendManager == nullptr);
        llmq::quorumInstantSendManager = std::make_unique<llmq::CInstantSendManager>(mempool, connman, sporkman, *llmq::quorumManager, *sigman, *shareman, *llmq::chainLocksHandler, *::masternodeSync, peerman, unit_tests, wipe);
        return llmq::quorumInstantSendManager.get();
    }()}
{
    // NOTE: we use this only to wipe the old db, do NOT use it for anything else
    // TODO: remove it in some future version
    auto llmqDbTmp = std::make_unique<CDBWrapper>(unit_tests ? "" : (GetDataDir() / "llmq"), 1 << 20, unit_tests, true);
}

LLMQContext::~LLMQContext() {
    // LLMQContext doesn't own these objects, but still need to care of them for consistancy:
    llmq::quorumInstantSendManager.reset();
    llmq::chainLocksHandler.reset();
    llmq::quorumManager.reset();
    llmq::quorumBlockProcessor.reset();
    {
        LOCK(llmq::cs_llmq_vbc);
        llmq::llmq_versionbitscache.Clear();
    }
}

void LLMQContext::Interrupt() {
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
    qman->Stop();
    qdkgsman->StopThreads();
    bls_worker->Stop();
}
