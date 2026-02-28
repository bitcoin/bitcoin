// Copyright (c) 2018-2019 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <llmq/quorums_init.h>

#include <llmq/quorums.h>
#include <llmq/quorums_blockprocessor.h>
#include <llmq/quorums_commitment.h>
#include <llmq/quorums_debug.h>
#include <llmq/quorums_dkgsessionmgr.h>
#include <llmq/quorums_signing.h>
#include <llmq/quorums_signing_shares.h>
#include <llmq/quorums_chainlocks.h>
#include <llmq/quorums_btccheckpoints.h>
#include <consensus/validation.h>
#include <dbwrapper.h>
#include <net.h>
namespace llmq
{

CBLSWorker* blsWorker;


void InitLLMQSystem(const DBParams& quorumCommitmentDB, const DBParams& quorumVectorDB, const DBParams& quorumSkDB, bool unitTests, CConnman& connman, BanMan& banman, PeerManager& peerman, ChainstateManager& chainman, bool fWipe)
{
    blsWorker = new CBLSWorker();

    quorumDKGDebugManager = new CDKGDebugManager();
    quorumBlockProcessor = new CQuorumBlockProcessor(quorumCommitmentDB, peerman, chainman);
    quorumDKGSessionManager = new CDKGSessionManager(*blsWorker, connman, peerman, chainman, unitTests, fWipe);
    quorumManager = new CQuorumManager(quorumVectorDB, quorumSkDB, *blsWorker, *quorumDKGSessionManager, chainman);
    quorumSigSharesManager = new CSigSharesManager(connman, peerman);
    quorumSigningManager = new CSigningManager(unitTests, peerman, chainman, fWipe);
    chainLocksHandler = new CChainLocksHandler(connman, peerman, chainman);
    btcCheckpointsHandler = new CBTCCheckpointsHandler(connman, peerman, chainman);
}

void DestroyLLMQSystem()
{
    delete btcCheckpointsHandler;
    btcCheckpointsHandler = nullptr;
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
    if (quorumSigningManager) {
        quorumSigningManager->StartWorkerThread();
    }
    if (chainLocksHandler) {
        chainLocksHandler->Start();
    }
    if (btcCheckpointsHandler) {
        btcCheckpointsHandler->Start();
    }
}

void StopLLMQSystem()
{
    if (btcCheckpointsHandler) {
        btcCheckpointsHandler->Stop();
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
    if (quorumSigningManager) {
        quorumSigningManager->StopWorkerThread();
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
    if (quorumSigningManager) {
        quorumSigningManager->InterruptWorkerThread();
    }
}

} // namespace llmq
