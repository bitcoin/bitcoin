// Copyright (c) 2018-2019 The Dash Core developers
// Copyright (c) 2019 The BitGreen Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <llmq/quorums_init.h>

#include <llmq/quorums.h>
#include <llmq/quorums_blockprocessor.h>
#include <llmq/quorums_commitment.h>
#include <llmq/quorums_chainlocks.h>
#include <llmq/quorums_debug.h>
#include <llmq/quorums_dkgsessionmgr.h>
#include <llmq/quorums_instantsend.h>
#include <llmq/quorums_signing.h>
#include <llmq/quorums_signing_shares.h>

#include <dbwrapper.h>
#include <scheduler.h>

namespace llmq
{

CBLSWorker* blsWorker;

CDBWrapper* llmqDb;

void InitLLMQSystem(CSpecialDB& specialDb, CScheduler* scheduler, bool unitTests, bool fWipe)
{
    llmqDb = new CDBWrapper(unitTests ? "" : (GetDataDir() / "llmq"), 1 << 20, unitTests, fWipe);
    blsWorker = new CBLSWorker();

    quorumDKGDebugManager = new CDKGDebugManager();
    quorumBlockProcessor = new CQuorumBlockProcessor(specialDb);
    quorumDKGSessionManager = new CDKGSessionManager(*llmqDb, *blsWorker);
    quorumManager = new CQuorumManager(specialDb, *blsWorker, *quorumDKGSessionManager);
    quorumSigSharesManager = new CSigSharesManager();
    quorumSigningManager = new CSigningManager(*llmqDb, unitTests);
    chainLocksHandler = new CChainLocksHandler(scheduler);
    quorumInstantSendManager = new CInstantSendManager(*llmqDb);
}

void DestroyLLMQSystem()
{
    delete quorumInstantSendManager;
    quorumInstantSendManager = nullptr;
    delete chainLocksHandler;
    chainLocksHandler = nullptr;
    delete quorumSigningManager;
    quorumSigningManager = nullptr;
    delete quorumSigSharesManager;
    quorumSigSharesManager = nullptr;
    delete quorumManager;
    quorumManager = nullptr;
    delete quorumDKGSessionManager;
    quorumDKGSessionManager = nullptr;
    delete quorumBlockProcessor;
    quorumBlockProcessor = nullptr;
    delete quorumDKGDebugManager;
    quorumDKGDebugManager = nullptr;
    delete blsWorker;
    blsWorker = nullptr;
    delete llmqDb;
    llmqDb = nullptr;
}

void StartLLMQSystem()
{
    if (blsWorker) {
        blsWorker->Start();
    }
    if (quorumDKGSessionManager) {
        quorumDKGSessionManager->StartMessageHandlerPool();
    }
    if (quorumSigSharesManager) {
        quorumSigSharesManager->RegisterAsRecoveredSigsListener();
        quorumSigSharesManager->StartWorkerThread();
    }
    if (chainLocksHandler) {
        chainLocksHandler->Start();
    }
    if (quorumInstantSendManager) {
        quorumInstantSendManager->Start();
    }
}

void StopLLMQSystem()
{
    if (quorumInstantSendManager) {
        quorumInstantSendManager->Stop();
    }
    if (chainLocksHandler) {
        chainLocksHandler->Stop();
    }
    if (quorumSigSharesManager) {
        quorumSigSharesManager->StopWorkerThread();
        quorumSigSharesManager->UnregisterAsRecoveredSigsListener();
    }
    if (quorumDKGSessionManager) {
        quorumDKGSessionManager->StopMessageHandlerPool();
    }
    if (blsWorker) {
        blsWorker->Stop();
    }
}

void InterruptLLMQSystem()
{
    if (quorumSigSharesManager) {
        quorumSigSharesManager->InterruptWorkerThread();
    }
    if (quorumInstantSendManager) {
        quorumInstantSendManager->InterruptWorkerThread();
    }
}

}
