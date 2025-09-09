// Copyright (c) 2023-2024 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <llmq/ehf_signals.h>
#include <llmq/quorums.h>
#include <llmq/commitment.h>

#include <chainparams.h>
#include <consensus/validation.h>
#include <deploymentstatus.h>
#include <evo/mnhftx.h>
#include <index/txindex.h> // g_txindex
#include <primitives/transaction.h>
#include <validation.h>

namespace llmq {
CEHFSignalsHandler::CEHFSignalsHandler(ChainstateManager& chainman, CMNHFManager& mnhfman, CSigningManager& sigman,
                                       CSigSharesManager& shareman, const CQuorumManager& qman) :
    m_chainman(chainman),
    mnhfman(mnhfman),
    sigman(sigman),
    shareman(shareman),
    qman(qman)
{
    sigman.RegisterRecoveredSigsListener(this);
}

CEHFSignalsHandler::~CEHFSignalsHandler()
{
    sigman.UnregisterRecoveredSigsListener(this);
}

void CEHFSignalsHandler::UpdatedBlockTip(const CBlockIndex* const pindexNew)
{
    if (!DeploymentActiveAfter(pindexNew, Params().GetConsensus(), Consensus::DEPLOYMENT_V20)) return;

    const auto ehfSignals = mnhfman.GetSignalsStage(pindexNew);
    for (const auto& deployment : Params().GetConsensus().vDeployments) {
        // Skip deployments that do not use dip0023
        if (!deployment.useEHF) continue;
        // Try to sign only activable deployments that haven't been mined yet
        if (ehfSignals.find(deployment.bit) == ehfSignals.end() && Params().IsValidMNActivation(deployment.bit, pindexNew->GetMedianTimePast())) {
            trySignEHFSignal(deployment.bit, pindexNew);
        }
    }
}

void CEHFSignalsHandler::trySignEHFSignal(int bit, const CBlockIndex* const pindex)
{
    MNHFTxPayload mnhfPayload;
    mnhfPayload.signal.versionBit = bit;
    const uint256 requestId = mnhfPayload.GetRequestId();

    const Consensus::LLMQType& llmqType = Params().GetConsensus().llmqTypeMnhf;
    const auto& llmq_params_opt = Params().GetLLMQ(llmqType);
    if (!llmq_params_opt.has_value()) {
        return;
    }
    if (sigman.HasRecoveredSigForId(llmqType, requestId)) {
        WITH_LOCK(cs, ids.insert(requestId));

        LogPrint(BCLog::EHF, "CEHFSignalsHandler::trySignEHFSignal: already signed bit=%d at height=%d id=%s\n", bit, pindex->nHeight, requestId.ToString());
        // no need to sign same message one more time
        return;
    }

    const auto quorum = llmq::SelectQuorumForSigning(llmq_params_opt.value(), m_chainman.ActiveChain(), qman, requestId);
    if (!quorum) {
        LogPrintf("CEHFSignalsHandler::trySignEHFSignal no quorum for id=%s\n", requestId.ToString());
        return;
    }

    LogPrint(BCLog::EHF, "CEHFSignalsHandler::trySignEHFSignal: bit=%d at height=%d id=%s\n", bit, pindex->nHeight, requestId.ToString());
    mnhfPayload.signal.quorumHash = quorum->qc->quorumHash;
    const uint256 msgHash = mnhfPayload.PrepareTx().GetHash();

    WITH_LOCK(cs, ids.insert(requestId));
    sigman.AsyncSignIfMember(llmqType, shareman, requestId, msgHash, quorum->qc->quorumHash, false, true);
}

MessageProcessingResult CEHFSignalsHandler::HandleNewRecoveredSig(const CRecoveredSig& recoveredSig)
{
    if (g_txindex) {
        g_txindex->BlockUntilSyncedToCurrentChain();
    }

    if (WITH_LOCK(cs, return ids.find(recoveredSig.getId()) == ids.end())) {
        // Do nothing, it's not for this handler
        return {};
    }

    MessageProcessingResult ret;
    const auto ehfSignals = mnhfman.GetSignalsStage(WITH_LOCK(::cs_main, return m_chainman.ActiveTip()));
    MNHFTxPayload mnhfPayload;
    for (const auto& deployment : Params().GetConsensus().vDeployments) {
        // skip deployments that do not use dip0023 or that have already been mined
        if (!deployment.useEHF || ehfSignals.find(deployment.bit) != ehfSignals.end()) continue;

        mnhfPayload.signal.versionBit = deployment.bit;
        const uint256 expectedId = mnhfPayload.GetRequestId();
        LogPrint(BCLog::EHF, "CEHFSignalsHandler::HandleNewRecoveredSig expecting ID=%s received=%s\n", expectedId.ToString(), recoveredSig.getId().ToString());
        if (recoveredSig.getId() != expectedId) {
            // wrong deployment! Check the next one
            continue;
        }

        mnhfPayload.signal.quorumHash = recoveredSig.getQuorumHash();
        mnhfPayload.signal.sig = recoveredSig.sig.Get();

        CMutableTransaction tx = mnhfPayload.PrepareTx();

        {
            CTransactionRef tx_to_sent = MakeTransactionRef(std::move(tx));
            LogPrintf("CEHFSignalsHandler::HandleNewRecoveredSig Special EHF TX is created hash=%s\n", tx_to_sent->GetHash().ToString());
            LOCK(::cs_main);
            const MempoolAcceptResult result = m_chainman.ProcessTransaction(tx_to_sent);
            if (result.m_result_type == MempoolAcceptResult::ResultType::VALID) {
                ret.m_transactions.push_back(tx_to_sent->GetHash());
            } else {
                LogPrintf("CEHFSignalsHandler::HandleNewRecoveredSig -- AcceptToMemoryPool failed: %s\n", result.m_state.ToString());
            }
        }
        break;
    }
    return ret;
}
} // namespace llmq
