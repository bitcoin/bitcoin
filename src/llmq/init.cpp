// Copyright (c) 2018-2022 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <llmq/init.h>

#include <llmq/quorums.h>
#include <llmq/blockprocessor.h>
#include <llmq/commitment.h>
#include <llmq/chainlocks.h>
#include <llmq/debug.h>
#include <llmq/dkgsessionmgr.h>
#include <llmq/instantsend.h>
#include <llmq/signing.h>
#include <llmq/signing_shares.h>
#include <llmq/utils.h>
#include <consensus/validation.h>

#include <dbwrapper.h>

namespace llmq
{

std::shared_ptr<CBLSWorker> blsWorker;

void InitLLMQSystem(CEvoDB& evoDb, CTxMemPool& mempool, CConnman& connman, CSporkManager& sporkManager, bool unitTests, bool fWipe)
{
    blsWorker = std::make_shared<CBLSWorker>();

    quorumDKGDebugManager = std::make_unique<CDKGDebugManager>();
    quorumBlockProcessor = std::make_unique<CQuorumBlockProcessor>(evoDb, connman);
    quorumDKGSessionManager = std::make_unique<CDKGSessionManager>(connman, *blsWorker, sporkManager, unitTests, fWipe);
    quorumManager = std::make_unique<CQuorumManager>(evoDb, connman, *blsWorker, *quorumDKGSessionManager);
    quorumSigSharesManager = std::make_unique<CSigSharesManager>(connman);
    quorumSigningManager = std::make_unique<CSigningManager>(connman, unitTests, fWipe);
    chainLocksHandler = std::make_unique<CChainLocksHandler>(mempool, connman, sporkManager);
    quorumInstantSendManager = std::make_unique<CInstantSendManager>(mempool, connman, sporkManager, unitTests, fWipe);

    // NOTE: we use this only to wipe the old db, do NOT use it for anything else
    // TODO: remove it in some future version
    auto llmqDbTmp = std::make_unique<CDBWrapper>(unitTests ? "" : (GetDataDir() / "llmq"), 1 << 20, unitTests, true);
}

void DestroyLLMQSystem()
{
    quorumInstantSendManager.reset();
    chainLocksHandler.reset();
    quorumSigningManager.reset();
    quorumSigSharesManager.reset();
    quorumManager.reset();
    quorumDKGSessionManager.reset();
    quorumBlockProcessor.reset();
    quorumDKGDebugManager.reset();
    blsWorker.reset();
    LOCK(cs_llmq_vbc);
    llmq_versionbitscache.Clear();
}

void StartLLMQSystem()
{
    if (blsWorker != nullptr) {
        blsWorker->Start();
    }
    if (quorumDKGSessionManager != nullptr) {
        quorumDKGSessionManager->StartThreads();
    }
    if (quorumManager != nullptr) {
        quorumManager->Start();
    }
    if (quorumSigSharesManager != nullptr) {
        quorumSigSharesManager->RegisterAsRecoveredSigsListener();
        quorumSigSharesManager->StartWorkerThread();
    }
    if (chainLocksHandler != nullptr) {
        chainLocksHandler->Start();
    }
    if (quorumInstantSendManager != nullptr) {
        quorumInstantSendManager->Start();
    }
}

void StopLLMQSystem()
{
    if (quorumInstantSendManager != nullptr) {
        quorumInstantSendManager->Stop();
    }
    if (chainLocksHandler != nullptr) {
        chainLocksHandler->Stop();
    }
    if (quorumSigSharesManager != nullptr) {
        quorumSigSharesManager->StopWorkerThread();
        quorumSigSharesManager->UnregisterAsRecoveredSigsListener();
    }
    if (quorumManager != nullptr) {
        quorumManager->Stop();
    }
    if (quorumDKGSessionManager != nullptr) {
        quorumDKGSessionManager->StopThreads();
    }
    if (blsWorker != nullptr) {
        blsWorker->Stop();
    }
}

void InterruptLLMQSystem()
{
    if (quorumSigSharesManager != nullptr) {
        quorumSigSharesManager->InterruptWorkerThread();
    }
    if (quorumInstantSendManager != nullptr) {
        quorumInstantSendManager->InterruptWorkerThread();
    }
}

} // namespace llmq
