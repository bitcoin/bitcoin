// Copyright (c) 2018-2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <evo/specialtxman.h>

#include <chainparams.h>
#include <consensus/validation.h>
#include <deploymentstatus.h>
#include <evo/assetlocktx.h>
#include <evo/cbtx.h>
#include <evo/creditpool.h>
#include <evo/deterministicmns.h>
#include <evo/mnhftx.h>
#include <evo/providertx.h>
#include <evo/simplifiedmns.h>
#include <hash.h>
#include <llmq/blockprocessor.h>
#include <llmq/commitment.h>
#include <primitives/block.h>
#include <validation.h>

static bool CheckSpecialTxInner(CDeterministicMNManager& dmnman, llmq::CQuorumSnapshotManager& qsnapman,
                                const ChainstateManager& chainman, const llmq::CQuorumManager& qman,
                                const CTransaction& tx, const CBlockIndex* pindexPrev, const CCoinsViewCache& view,
                                const std::optional<CRangesSet>& indexes, bool check_sigs, TxValidationState& state)
    EXCLUSIVE_LOCKS_REQUIRED(::cs_main)
{
    AssertLockHeld(cs_main);

    if (!tx.HasExtraPayloadField())
        return true;

    const auto& consensusParams = Params().GetConsensus();
    if (!DeploymentActiveAfter(pindexPrev, consensusParams, Consensus::DEPLOYMENT_DIP0003)) {
        return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-tx-type");
    }

    try {
        switch (tx.nType) {
        case TRANSACTION_PROVIDER_REGISTER:
            return CheckProRegTx(dmnman, tx, pindexPrev, state, view, check_sigs);
        case TRANSACTION_PROVIDER_UPDATE_SERVICE:
            return CheckProUpServTx(dmnman, tx, pindexPrev, state, check_sigs);
        case TRANSACTION_PROVIDER_UPDATE_REGISTRAR:
            return CheckProUpRegTx(dmnman, tx, pindexPrev, state, view, check_sigs);
        case TRANSACTION_PROVIDER_UPDATE_REVOKE:
            return CheckProUpRevTx(dmnman, tx, pindexPrev, state, check_sigs);
        case TRANSACTION_COINBASE:
            return CheckCbTx(tx, pindexPrev, state);
        case TRANSACTION_QUORUM_COMMITMENT:
            return llmq::CheckLLMQCommitment(dmnman, qsnapman, chainman, tx, pindexPrev, state);
        case TRANSACTION_MNHF_SIGNAL:
            return CheckMNHFTx(chainman, qman, tx, pindexPrev, state);
        case TRANSACTION_ASSET_LOCK:
            return CheckAssetLockUnlockTx(chainman.m_blockman, qman, tx, pindexPrev, indexes, state);
        case TRANSACTION_ASSET_UNLOCK:
            return CheckAssetLockUnlockTx(chainman.m_blockman, qman, tx, pindexPrev, indexes, state);
        }
    } catch (const std::exception& e) {
        LogPrintf("%s -- failed: %s\n", __func__, e.what());
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "failed-check-special-tx");
    }

    return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-tx-type-check");
}

bool CSpecialTxProcessor::CheckSpecialTx(const CTransaction& tx, const CBlockIndex* pindexPrev, const CCoinsViewCache& view, bool check_sigs, TxValidationState& state)
{
    AssertLockHeld(cs_main);
    return CheckSpecialTxInner(m_dmnman, m_qsnapman, m_chainman, m_qman, tx, pindexPrev, view, std::nullopt, check_sigs,
                               state);
}

[[nodiscard]] bool CSpecialTxProcessor::ProcessSpecialTx(const CTransaction& tx, const CBlockIndex* pindex, TxValidationState& state)
{
    if (!tx.HasExtraPayloadField()) {
        return true;
    }

    switch (tx.nType) {
    case TRANSACTION_ASSET_LOCK:
    case TRANSACTION_ASSET_UNLOCK:
        return true; // handled per block (during cb)
    case TRANSACTION_PROVIDER_REGISTER:
    case TRANSACTION_PROVIDER_UPDATE_SERVICE:
    case TRANSACTION_PROVIDER_UPDATE_REGISTRAR:
    case TRANSACTION_PROVIDER_UPDATE_REVOKE:
        return true; // handled in batches per block
    case TRANSACTION_COINBASE:
        return true; // nothing to do
    case TRANSACTION_QUORUM_COMMITMENT:
        return true; // handled per block
    case TRANSACTION_MNHF_SIGNAL:
        return true; // handled per block
    }

    return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-tx-type-proc");
}

[[nodiscard]] bool CSpecialTxProcessor::UndoSpecialTx(const CTransaction& tx, const CBlockIndex* pindex)
{
    if (!tx.HasExtraPayloadField()) {
        return true;
    }

    switch (tx.nType) {
    case TRANSACTION_ASSET_LOCK:
    case TRANSACTION_ASSET_UNLOCK:
        return true; // handled per block (during cb)
    case TRANSACTION_PROVIDER_REGISTER:
    case TRANSACTION_PROVIDER_UPDATE_SERVICE:
    case TRANSACTION_PROVIDER_UPDATE_REGISTRAR:
    case TRANSACTION_PROVIDER_UPDATE_REVOKE:
        return true; // handled in batches per block
    case TRANSACTION_COINBASE:
        return true; // nothing to do
    case TRANSACTION_QUORUM_COMMITMENT:
        return true; // handled per block
    case TRANSACTION_MNHF_SIGNAL:
        return true; // handled per block
    }

    return false;
}

bool CSpecialTxProcessor::ProcessSpecialTxsInBlock(const CBlock& block, const CBlockIndex* pindex, const CCoinsViewCache& view, bool fJustCheck,
                                                   bool fCheckCbTxMerkleRoots, BlockValidationState& state, std::optional<MNListUpdates>& updatesRet)
{
    AssertLockHeld(cs_main);

    try {
        static int64_t nTimeLoop = 0;
        static int64_t nTimeQuorum = 0;
        static int64_t nTimeDMN = 0;
        static int64_t nTimeMerkle = 0;
        static int64_t nTimeCbTxCL = 0;
        static int64_t nTimeMnehf = 0;

        int64_t nTime1 = GetTimeMicros();

        const CCreditPool creditPool = m_cpoolman.GetCreditPool(pindex->pprev, m_consensus_params);
        if (DeploymentActiveAt(*pindex, m_consensus_params, Consensus::DEPLOYMENT_V20)) {
            LogPrint(BCLog::CREDITPOOL, "CSpecialTxProcessor::%s -- CCreditPool is %s\n", __func__, creditPool.ToString());
        }

        for (const auto& ptr_tx : block.vtx) {
            TxValidationState tx_state;
            // At this moment CheckSpecialTx() and ProcessSpecialTx() may fail by 2 possible ways:
            // consensus failures and "TX_BAD_SPECIAL"
            if (!CheckSpecialTxInner(m_dmnman, m_qsnapman, m_chainman, m_qman, *ptr_tx, pindex->pprev, view,
                                     creditPool.indexes, fCheckCbTxMerkleRoots, tx_state)) {
                assert(tx_state.GetResult() == TxValidationResult::TX_CONSENSUS || tx_state.GetResult() == TxValidationResult::TX_BAD_SPECIAL);
                return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, tx_state.GetRejectReason(),
                                 strprintf("Special Transaction check failed (tx hash %s) %s", ptr_tx->GetHash().ToString(), tx_state.GetDebugMessage()));
            }
            if (!ProcessSpecialTx(*ptr_tx, pindex, tx_state)) {
                assert(tx_state.GetResult() == TxValidationResult::TX_CONSENSUS || tx_state.GetResult() == TxValidationResult::TX_BAD_SPECIAL);
                return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, tx_state.GetRejectReason(),
                                 strprintf("Process Special Transaction failed (tx hash %s) %s", ptr_tx->GetHash().ToString(), tx_state.GetDebugMessage()));
            }
        }

        int64_t nTime2 = GetTimeMicros();
        nTimeLoop += nTime2 - nTime1;
        LogPrint(BCLog::BENCHMARK, "        - Loop: %.2fms [%.2fs]\n", 0.001 * (nTime2 - nTime1), nTimeLoop * 0.000001);

        if (!m_qblockman.ProcessBlock(block, pindex, state, fJustCheck, fCheckCbTxMerkleRoots)) {
            // pass the state returned by the function above
            return false;
        }

        int64_t nTime3 = GetTimeMicros();
        nTimeQuorum += nTime3 - nTime2;
        LogPrint(BCLog::BENCHMARK, "        - m_qblockman: %.2fms [%.2fs]\n", 0.001 * (nTime3 - nTime2), nTimeQuorum * 0.000001);


        CDeterministicMNList mn_list;
        if (DeploymentActiveAt(*pindex, m_consensus_params, Consensus::DEPLOYMENT_DIP0003)) {
            if (!m_dmnman.BuildNewListFromBlock(block, pindex->pprev, state, view, mn_list, m_qsnapman, true)) {
                // pass the state returned by the function above
                return false;
            }
            mn_list.SetBlockHash(pindex->GetBlockHash());

            if (!fJustCheck && !m_dmnman.ProcessBlock(block, pindex, state, view, m_qsnapman, mn_list, updatesRet)) {
                // pass the state returned by the function above
                return false;
            }
        }

        int64_t nTime4 = GetTimeMicros();
        nTimeDMN += nTime4 - nTime3;
        LogPrint(BCLog::BENCHMARK, "        - m_dmnman: %.2fms [%.2fs]\n", 0.001 * (nTime4 - nTime3), nTimeDMN * 0.000001);

        int64_t nTime5{nTime4};

        if (fCheckCbTxMerkleRoots && block.vtx[0]->nType == TRANSACTION_COINBASE) {
            int64_t nTime1_cbtx = GetTimeMicros();

            const auto opt_cbTx = GetTxPayload<CCbTx>(*block.vtx[0]);
            if (!opt_cbTx) {
                return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-cbtx-payload");
            }
            static int64_t nTimePayload = 0;

            int64_t nTime2_cbtx = GetTimeMicros();
            nTimePayload += nTime2_cbtx - nTime1_cbtx;
            LogPrint(BCLog::BENCHMARK, "          - GetTxPayload: %.2fms [%.2fs]\n",
                     0.001 * (nTime2_cbtx - nTime1_cbtx), nTimePayload * 0.000001);

            if (!CheckCbTxMerkleRoots(block, *opt_cbTx, pindex, m_qblockman, CSimplifiedMNList(mn_list), state)) {
                // pass the state returned by the function above
                return false;
            }

            nTime5 = GetTimeMicros();
            nTimeMerkle += nTime5 - nTime4;

            LogPrint(BCLog::BENCHMARK, "        - CheckCbTxMerkleRoots: %.2fms [%.2fs]\n", 0.001 * (nTime5 - nTime4),
                     nTimeMerkle * 0.000001);


            if (!CheckCbTxBestChainlock(*opt_cbTx, pindex, m_clhandler, state)) {
                // pass the state returned by the function above
                return false;
            }
        }

        int64_t nTime6 = GetTimeMicros();
        nTimeCbTxCL += nTime6 - nTime5;
        LogPrint(BCLog::BENCHMARK, "        - CheckCbTxBestChainlock: %.2fms [%.2fs]\n", 0.001 * (nTime6 - nTime5), nTimeCbTxCL * 0.000001);

        if (!m_mnhfman.ProcessBlock(block, pindex, fJustCheck, state)) {
            // pass the state returned by the function above
            return false;
        }

        int64_t nTime7 = GetTimeMicros();
        nTimeMnehf += nTime7 - nTime6;
        LogPrint(BCLog::BENCHMARK, "        - m_mnhfman: %.2fms [%.2fs]\n", 0.001 * (nTime7 - nTime6), nTimeMnehf * 0.000001);

        if (DeploymentActiveAfter(pindex, m_consensus_params, Consensus::DEPLOYMENT_V19) && bls::bls_legacy_scheme.load()) {
            // NOTE: The block next to the activation is the one that is using new rules.
            // V19 activated just activated, so we must switch to the new rules here.
            bls::bls_legacy_scheme.store(false);
            LogPrintf("CSpecialTxProcessor::%s -- bls_legacy_scheme=%d\n", __func__, bls::bls_legacy_scheme.load());
        }
    } catch (const std::exception& e) {
        LogPrintf("CSpecialTxProcessor::%s -- FAILURE! %s\n", __func__, e.what());
        return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "failed-procspectxsinblock");
    }

    return true;
}

bool CSpecialTxProcessor::UndoSpecialTxsInBlock(const CBlock& block, const CBlockIndex* pindex, std::optional<MNListUpdates>& updatesRet)
{
    AssertLockHeld(cs_main);

    auto bls_legacy_scheme = bls::bls_legacy_scheme.load();

    try {
        if (!DeploymentActiveAt(*pindex, m_consensus_params, Consensus::DEPLOYMENT_V19) && !bls_legacy_scheme) {
            // NOTE: The block next to the activation is the one that is using new rules.
            // Removing the activation block here, so we must switch back to the old rules.
            bls::bls_legacy_scheme.store(true);
            LogPrintf("CSpecialTxProcessor::%s -- bls_legacy_scheme=%d\n", __func__, bls::bls_legacy_scheme.load());
        }

        for (int i = (int)block.vtx.size() - 1; i >= 0; --i) {
            const CTransaction& tx = *block.vtx[i];
            if (!UndoSpecialTx(tx, pindex)) {
                return false;
            }
        }

        if (!m_mnhfman.UndoBlock(block, pindex)) {
            return false;
        }

        if (!m_dmnman.UndoBlock(pindex, updatesRet)) {
            return false;
        }

        if (!m_qblockman.UndoBlock(block, pindex)) {
            return false;
        }
    } catch (const std::exception& e) {
        bls::bls_legacy_scheme.store(bls_legacy_scheme);
        LogPrintf("CSpecialTxProcessor::%s -- bls_legacy_scheme=%d\n", __func__, bls::bls_legacy_scheme.load());
        return error(strprintf("CSpecialTxProcessor::%s -- FAILURE! %s\n", __func__, e.what()).c_str());
    }

    return true;
}

bool CSpecialTxProcessor::CheckCreditPoolDiffForBlock(const CBlock& block, const CBlockIndex* pindex, const CAmount blockSubsidy, BlockValidationState& state)
{
    AssertLockHeld(cs_main);

    try {
        if (!DeploymentActiveAt(*pindex, m_consensus_params, Consensus::DEPLOYMENT_DIP0003)) return true;
        if (!DeploymentActiveAt(*pindex, m_consensus_params, Consensus::DEPLOYMENT_DIP0008)) return true;
        if (!DeploymentActiveAt(*pindex, m_consensus_params, Consensus::DEPLOYMENT_V20)) return true;

        auto creditPoolDiff = GetCreditPoolDiffForBlock(m_cpoolman, m_chainman.m_blockman, m_qman, block, pindex->pprev, m_consensus_params, blockSubsidy, state);
        if (!creditPoolDiff.has_value()) return false;

        // If we get there we have v20 activated and credit pool amount must be included in block CbTx
        if (block.vtx.empty()) {
            return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-missing-cbtx");
        }
        const auto& tx = *block.vtx[0];
        if (!tx.IsCoinBase() || !tx.IsSpecialTxVersion() || tx.nType != TRANSACTION_COINBASE) {
            return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-cbtx-type");
        }

        const auto opt_cbTx = GetTxPayload<CCbTx>(tx);
        if (!opt_cbTx) {
            return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-cbtx-payload");
        }
        CAmount target_balance{opt_cbTx->creditPoolBalance};
        // But it maybe not included yet in previous block yet; in this case value must be 0
        CAmount locked_calculated{creditPoolDiff->GetTotalLocked()};
        if (target_balance != locked_calculated) {
            LogPrintf("CSpecialTxProcessor::%s -- mismatched locked amount in CbTx: %lld against re-calculated: %lld\n", __func__, target_balance, locked_calculated);
            return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-cbtx-assetlocked-amount");
        }

    } catch (const std::exception& e) {
        LogPrintf("CSpecialTxProcessor::%s -- FAILURE! %s\n", __func__, e.what());
        return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "failed-checkcreditpooldiff");
    }

    return true;
}
