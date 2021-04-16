// Copyright (c) 2018-2019 The Dash Core developers
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
#include <llmq/quorums_utils.h>
#include <consensus/validation.h>

#include <dbwrapper.h>

namespace llmq
{

CBLSWorker* blsWorker;

CDBWrapper* llmqDb;

void InitLLMQSystem(CEvoDB& evoDb, bool unitTests, bool fWipe)
{
    llmqDb = new CDBWrapper(unitTests ? "" : (GetDataDir() / "llmq"), 1 << 20, unitTests, fWipe);
    blsWorker = new CBLSWorker();

    quorumDKGDebugManager = new CDKGDebugManager();
    quorumBlockProcessor = new CQuorumBlockProcessor(evoDb);
    quorumDKGSessionManager = new CDKGSessionManager(*llmqDb, *blsWorker);
    quorumManager = new CQuorumManager(evoDb, *blsWorker, *quorumDKGSessionManager);
    quorumSigSharesManager = new CSigSharesManager();
    quorumSigningManager = new CSigningManager(*llmqDb, unitTests);
    chainLocksHandler = new CChainLocksHandler();
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
    LOCK(cs_llmq_vbc);
    llmq_versionbitscache.Clear();
}

void StartLLMQSystem()
{
    if (blsWorker) {
        blsWorker->Start();
    }
    if (quorumDKGSessionManager) {
        quorumDKGSessionManager->StartThreads();
    }
    if (quorumManager) {
        quorumManager->Start();
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
    if (quorumManager) {
        quorumManager->Stop();
    }
    if (quorumDKGSessionManager) {
        quorumDKGSessionManager->StopThreads();
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

} // namespace llmq
