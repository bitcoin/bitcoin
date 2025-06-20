// Copyright (c) 2018-2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <evo/specialtxman.h>

#include <chainparams.h>
#include <consensus/amount.h>
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
#include <llmq/chainlocks.h>
#include <llmq/clsig.h>
#include <llmq/commitment.h>
#include <llmq/quorums.h>
#include <primitives/block.h>
#include <validation.h>

static bool CheckCbTxBestChainlock(const CCbTx& cbTx, const CBlockIndex* pindex,
                                   const llmq::CChainLocksHandler& chainlock_handler, BlockValidationState& state)
{
    if (cbTx.nVersion < CCbTx::Version::CLSIG_AND_BALANCE) {
        return true;
    }

    static Mutex cached_mutex;
    static const CBlockIndex* cached_pindex GUARDED_BY(cached_mutex){nullptr};
    static std::optional<std::pair<CBLSSignature, uint32_t>> cached_chainlock GUARDED_BY(cached_mutex){std::nullopt};

    auto best_clsig = chainlock_handler.GetBestChainLock();
    if (best_clsig.getHeight() == pindex->nHeight - 1 && cbTx.bestCLHeightDiff == 0 &&
        cbTx.bestCLSignature == best_clsig.getSig()) {
        // matches our best clsig which still hold values for the previous block
        LOCK(cached_mutex);
        cached_chainlock = std::make_pair(cbTx.bestCLSignature, cbTx.bestCLHeightDiff);
        cached_pindex = pindex;
        return true;
    }

    std::optional<std::pair<CBLSSignature, uint32_t>> prevBlockCoinbaseChainlock{std::nullopt};
    if (LOCK(cached_mutex); cached_pindex == pindex->pprev) {
        prevBlockCoinbaseChainlock = cached_chainlock;
    }
    if (!prevBlockCoinbaseChainlock.has_value()) {
        prevBlockCoinbaseChainlock = GetNonNullCoinbaseChainlock(pindex->pprev);
    }
    // If std::optional prevBlockCoinbaseChainlock is empty, then up to the previous block, coinbase Chainlock is null.
    if (prevBlockCoinbaseChainlock.has_value()) {
        // Previous block Coinbase has a non-null Chainlock: current block's Chainlock must be non-null and at least as new as the previous one
        if (!cbTx.bestCLSignature.IsValid()) {
            // IsNull() doesn't exist for CBLSSignature: we assume that a non valid BLS sig is null
            return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-cbtx-null-clsig");
        }
        if (cbTx.bestCLHeightDiff > prevBlockCoinbaseChainlock.value().second + 1) {
            return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-cbtx-older-clsig");
        }
    }

    // IsNull() doesn't exist for CBLSSignature: we assume that a valid BLS sig is non-null
    if (cbTx.bestCLSignature.IsValid()) {
        int curBlockCoinbaseCLHeight = pindex->nHeight - static_cast<int>(cbTx.bestCLHeightDiff) - 1;
        if (best_clsig.getHeight() == curBlockCoinbaseCLHeight && best_clsig.getSig() == cbTx.bestCLSignature) {
            // matches our best (but outdated) clsig, no need to verify it again
            LOCK(cached_mutex);
            cached_chainlock = std::make_pair(cbTx.bestCLSignature, cbTx.bestCLHeightDiff);
            cached_pindex = pindex;
            return true;
        }
        uint256 curBlockCoinbaseCLBlockHash = pindex->GetAncestor(curBlockCoinbaseCLHeight)->GetBlockHash();
        if (chainlock_handler.VerifyChainLock(
                llmq::CChainLockSig(curBlockCoinbaseCLHeight, curBlockCoinbaseCLBlockHash, cbTx.bestCLSignature)) !=
            llmq::VerifyRecSigStatus::Valid) {
            return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-cbtx-invalid-clsig");
        }
        LOCK(cached_mutex);
        cached_chainlock = std::make_pair(cbTx.bestCLSignature, cbTx.bestCLHeightDiff);
        cached_pindex = pindex;
    } else if (cbTx.bestCLHeightDiff != 0) {
        // Null bestCLSignature is allowed only with bestCLHeightDiff = 0
        return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-cbtx-cldiff");
    }

    return true;
}

static bool CheckSpecialTxInner(CDeterministicMNManager& dmnman, llmq::CQuorumSnapshotManager& qsnapman,
                                const ChainstateManager& chainman, const llmq::CQuorumManager& qman,
                                const CTransaction& tx, const CBlockIndex* pindexPrev, const CCoinsViewCache& view,
                                const std::optional<CRangesSet>& indexes, bool check_sigs, TxValidationState& state)
    EXCLUSIVE_LOCKS_REQUIRED(::cs_main)
{
    AssertLockHeld(::cs_main);

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
        case TRANSACTION_COINBASE: {
            if (!tx.IsCoinBase()) {
                return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-cbtx-invalid");
            }
            if (const auto opt_cbTx = GetTxPayload<CCbTx>(tx)) {
                return CheckCbTx(*opt_cbTx, pindexPrev, state);
            } else {
                return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-cbtx-payload");
            }
        }
        case TRANSACTION_QUORUM_COMMITMENT:
            return llmq::CheckLLMQCommitment(dmnman, qsnapman, chainman, tx, pindexPrev, state);
        case TRANSACTION_MNHF_SIGNAL:
            return CheckMNHFTx(chainman, qman, tx, pindexPrev, state);
        case TRANSACTION_ASSET_LOCK:
            return CheckAssetLockTx(tx, state);
        case TRANSACTION_ASSET_UNLOCK:
            return CheckAssetUnlockTx(chainman.m_blockman, qman, tx, pindexPrev, indexes, state);
        }
    } catch (const std::exception& e) {
        LogPrintf("%s -- failed: %s\n", __func__, e.what());
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "failed-check-special-tx");
    }

    return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-tx-type-check");
}

bool CSpecialTxProcessor::CheckSpecialTx(const CTransaction& tx, const CBlockIndex* pindexPrev, const CCoinsViewCache& view, bool check_sigs, TxValidationState& state)
{
    AssertLockHeld(::cs_main);
    return CheckSpecialTxInner(m_dmnman, m_qsnapman, m_chainman, m_qman, tx, pindexPrev, view, std::nullopt, check_sigs,
                               state);
}

bool CSpecialTxProcessor::ProcessSpecialTxsInBlock(const CBlock& block, const CBlockIndex* pindex, const CCoinsViewCache& view, bool fJustCheck,
                                                   bool fCheckCbTxMerkleRoots, BlockValidationState& state, std::optional<MNListUpdates>& updatesRet)
{
    AssertLockHeld(::cs_main);

    try {
        static int64_t nTimeLoop = 0;
        static int64_t nTimeQuorum = 0;
        static int64_t nTimeDMN = 0;
        static int64_t nTimeMerkleMNL = 0;
        static int64_t nTimeMerkle = 0;
        static int64_t nTimeCbTxCL = 0;
        static int64_t nTimeMnehf = 0;
        static int64_t nTimePayload = 0;
        static int64_t nTimeCreditPool = 0;

        int64_t nTime1 = GetTimeMicros();

        std::optional<CCbTx> opt_cbTx{std::nullopt};
        if (fCheckCbTxMerkleRoots && block.vtx.size() > 0 && block.vtx[0]->nType == TRANSACTION_COINBASE) {
            const auto& tx = block.vtx[0];
            if (!tx->IsCoinBase()) {
                return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-cbtx-invalid");
            }
            if (opt_cbTx = GetTxPayload<CCbTx>(*tx); opt_cbTx) {
                TxValidationState tx_state;
                if (!CheckCbTx(*opt_cbTx, pindex->pprev, tx_state)) {
                    assert(tx_state.GetResult() == TxValidationResult::TX_CONSENSUS ||
                           tx_state.GetResult() == TxValidationResult::TX_BAD_SPECIAL);
                    return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, tx_state.GetRejectReason(),
                                         strprintf("Special Transaction check failed (tx hash %s) %s",
                                                   tx->GetHash().ToString(), tx_state.GetDebugMessage()));
                }
            } else {
                return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-cbtx-payload");
            }
        }
        if (fCheckCbTxMerkleRoots) {
            // To ensure that opt_cbTx is not missing when it's supposed to be
            if (DeploymentActiveAt(*pindex, m_consensus_params, Consensus::DEPLOYMENT_DIP0003) && !opt_cbTx.has_value()) {
                return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-cbtx-version");
            }
        }

        int64_t nTime2 = GetTimeMicros();
        nTimePayload += nTime2 - nTime1;
        LogPrint(BCLog::BENCHMARK, "      - GetTxPayload: %.2fms [%.2fs]\n", 0.001 * (nTime2 - nTime1),
                 nTimePayload * 0.000001);

        CRangesSet indexes;
        if (DeploymentActiveAt(*pindex, m_consensus_params, Consensus::DEPLOYMENT_V20)) {
            CCreditPool creditPool{m_cpoolman.GetCreditPool(pindex->pprev, m_consensus_params)};
            LogPrint(BCLog::CREDITPOOL, "CSpecialTxProcessor::%s -- CCreditPool is %s\n", __func__, creditPool.ToString());
            indexes = std::move(creditPool.indexes);
        }

        for (size_t i = 0; i < block.vtx.size(); ++i) {
            // we validated CCbTx above, starts from the 2nd transaction
            if (i == 0 && block.vtx[i]->nType == TRANSACTION_COINBASE) continue;

            const auto ptr_tx = block.vtx[i];
            TxValidationState tx_state;
            // At this moment CheckSpecialTx() may fail by 2 possible ways:
            // consensus failures and "TX_BAD_SPECIAL"
            if (!CheckSpecialTxInner(m_dmnman, m_qsnapman, m_chainman, m_qman, *ptr_tx, pindex->pprev, view, indexes,
                                     fCheckCbTxMerkleRoots, tx_state)) {
                assert(tx_state.GetResult() == TxValidationResult::TX_CONSENSUS || tx_state.GetResult() == TxValidationResult::TX_BAD_SPECIAL);
                return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, tx_state.GetRejectReason(),
                                 strprintf("Special Transaction check failed (tx hash %s) %s", ptr_tx->GetHash().ToString(), tx_state.GetDebugMessage()));
            }
        }

        int64_t nTime3 = GetTimeMicros();
        nTimeLoop += nTime3 - nTime2;
        LogPrint(BCLog::BENCHMARK, "      - Loop: %.2fms [%.2fs]\n", 0.001 * (nTime3 - nTime2), nTimeLoop * 0.000001);

        if (opt_cbTx.has_value()) {
            if (!CheckCreditPoolDiffForBlock(block, pindex, *opt_cbTx, state)) {
                return error("CSpecialTxProcessor: CheckCreditPoolDiffForBlock for block %s failed with %s",
                             pindex->GetBlockHash().ToString(), state.ToString());
            }
        }

        int64_t nTime4 = GetTimeMicros();
        nTimeCreditPool += nTime4 - nTime3;
        LogPrint(BCLog::BENCHMARK, "      - CheckCreditPoolDiffForBlock: %.2fms [%.2fs]\n", 0.001 * (nTime4 - nTime3),
                 nTimeCreditPool * 0.000001);

        if (!m_qblockman.ProcessBlock(block, pindex, state, fJustCheck, fCheckCbTxMerkleRoots)) {
            // pass the state returned by the function above
            return false;
        }

        int64_t nTime5 = GetTimeMicros();
        nTimeQuorum += nTime5 - nTime4;
        LogPrint(BCLog::BENCHMARK, "      - m_qblockman: %.2fms [%.2fs]\n", 0.001 * (nTime5 - nTime4),
                 nTimeQuorum * 0.000001);


        if (opt_cbTx.has_value()) {
            CDeterministicMNList mn_list;
            if (!m_dmnman.BuildNewListFromBlock(block, pindex->pprev, state, view, mn_list, m_qsnapman, true)) {
                // pass the state returned by the function above
                return false;
            }
            mn_list.SetBlockHash(pindex->GetBlockHash());

            if (!fJustCheck && !m_dmnman.ProcessBlock(block, pindex, state, view, m_qsnapman, mn_list, updatesRet)) {
                // pass the state returned by the function above
                return false;
            }

            int64_t nTime5_1 = GetTimeMicros();
            nTimeDMN += nTime5_1 - nTime5;

            LogPrint(BCLog::BENCHMARK, "      - m_dmnman: %.2fms [%.2fs]\n", 0.001 * (nTime5_1 - nTime5),
                     nTimeDMN * 0.000001);

            uint256 calculatedMerkleRoot;
            if (!CalcCbTxMerkleRootMNList(calculatedMerkleRoot, CSimplifiedMNList{std::move(mn_list)}, state)) {
                // pass the state returned by the function above
                return false;
            }
            if (calculatedMerkleRoot != opt_cbTx->merkleRootMNList) {
                return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-cbtx-mnmerkleroot");
            }

            int64_t nTime5_2 = GetTimeMicros();
            nTimeMerkleMNL += nTime5_2 - nTime5_1;
            LogPrint(BCLog::BENCHMARK, "      - CalcCbTxMerkleRootMNList: %.2fms [%.2fs]\n",
                     0.001 * (nTime5_2 - nTime5_1), nTimeMerkleMNL * 0.000001);
        }

        int64_t nTime6 = GetTimeMicros();
        if (opt_cbTx.has_value()) {
            if (!CheckCbTxMerkleRoots(block, *opt_cbTx, pindex, m_qblockman, state)) {
                // pass the state returned by the function above
                return false;
            }
        }

        int64_t nTime7 = GetTimeMicros();
        nTimeMerkle += nTime7 - nTime6;

        LogPrint(BCLog::BENCHMARK, "      - CheckCbTxMerkleRoots: %.2fms [%.2fs]\n", 0.001 * (nTime7 - nTime6),
                 nTimeMerkle * 0.000001);

        if (opt_cbTx.has_value()) {
            if (!CheckCbTxBestChainlock(*opt_cbTx, pindex, m_clhandler, state)) {
                // pass the state returned by the function above
                return false;
            }
        }

        int64_t nTime8 = GetTimeMicros();
        nTimeCbTxCL += nTime8 - nTime7;
        LogPrint(BCLog::BENCHMARK, "      - CheckCbTxBestChainlock: %.2fms [%.2fs]\n", 0.001 * (nTime8 - nTime7),
                 nTimeCbTxCL * 0.000001);

        if (!m_mnhfman.ProcessBlock(block, pindex, fJustCheck, state)) {
            // pass the state returned by the function above
            return false;
        }

        int64_t nTime9 = GetTimeMicros();
        nTimeMnehf += nTime9 - nTime8;
        LogPrint(BCLog::BENCHMARK, "      - m_mnhfman: %.2fms [%.2fs]\n", 0.001 * (nTime9 - nTime8), nTimeMnehf * 0.000001);

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
    AssertLockHeld(::cs_main);

    auto bls_legacy_scheme = bls::bls_legacy_scheme.load();

    try {
        if (!DeploymentActiveAt(*pindex, m_consensus_params, Consensus::DEPLOYMENT_V19) && !bls_legacy_scheme) {
            // NOTE: The block next to the activation is the one that is using new rules.
            // Removing the activation block here, so we must switch back to the old rules.
            bls::bls_legacy_scheme.store(true);
            LogPrintf("CSpecialTxProcessor::%s -- bls_legacy_scheme=%d\n", __func__, bls::bls_legacy_scheme.load());
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

bool CSpecialTxProcessor::CheckCreditPoolDiffForBlock(const CBlock& block, const CBlockIndex* pindex, const CCbTx& cbTx,
                                                      BlockValidationState& state)
{
    AssertLockHeld(::cs_main);

    if (!DeploymentActiveAt(*pindex, m_consensus_params, Consensus::DEPLOYMENT_DIP0008)) return true;
    if (!DeploymentActiveAt(*pindex, m_consensus_params, Consensus::DEPLOYMENT_V20)) return true;

    try {
        const CAmount blockSubsidy = GetBlockSubsidy(pindex, m_consensus_params);
        const auto creditPoolDiff = GetCreditPoolDiffForBlock(m_cpoolman, m_chainman.m_blockman, m_qman, block,
                                                              pindex->pprev, m_consensus_params, blockSubsidy, state);
        if (!creditPoolDiff.has_value()) return false;

        const CAmount target_balance{cbTx.creditPoolBalance};
        // But it maybe not included yet in previous block yet; in this case value must be 0
        const CAmount locked_calculated{creditPoolDiff->GetTotalLocked()};
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
