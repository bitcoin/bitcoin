// Copyright (c) 2017-2024 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <evo/cbtx.h>
#include <evo/deterministicmns.h>
#include <llmq/blockprocessor.h>
#include <llmq/chainlocks.h>
#include <llmq/commitment.h>
#include <llmq/options.h>
#include <node/blockstorage.h>
#include <evo/simplifiedmns.h>
#include <evo/specialtx.h>
#include <consensus/validation.h>

#include <chain.h>
#include <chainparams.h>
#include <consensus/merkle.h>
#include <deploymentstatus.h>
#include <validation.h>

bool CheckCbTx(const CTransaction& tx, const CBlockIndex* pindexPrev, TxValidationState& state)
{
    if (tx.nType != TRANSACTION_COINBASE) {
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-cbtx-type");
    }

    if (!tx.IsCoinBase()) {
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-cbtx-invalid");
    }

    const auto opt_cbTx = GetTxPayload<CCbTx>(tx);
    if (!opt_cbTx) {
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-cbtx-payload");
    }
    const auto& cbTx = *opt_cbTx;

    if (cbTx.nVersion == CCbTx::Version::INVALID || cbTx.nVersion >= CCbTx::Version::UNKNOWN) {
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-cbtx-version");
    }

    if (pindexPrev) {
        if (pindexPrev->nHeight + 1 != cbTx.nHeight) {
            return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-cbtx-height");
        }

        const bool fDIP0008Active{DeploymentActiveAt(*pindexPrev, Params().GetConsensus(), Consensus::DEPLOYMENT_DIP0008)};
        if (fDIP0008Active && cbTx.nVersion < CCbTx::Version::MERKLE_ROOT_QUORUMS) {
            return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-cbtx-version");
        }

        const bool isV20{DeploymentActiveAfter(pindexPrev, Params().GetConsensus(), Consensus::DEPLOYMENT_V20)};
        if ((isV20 && cbTx.nVersion < CCbTx::Version::CLSIG_AND_BALANCE) || (!isV20 && cbTx.nVersion >= CCbTx::Version::CLSIG_AND_BALANCE)) {
            return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-cbtx-version");
        }
    }

    return true;
}

// This can only be done after the block has been fully processed, as otherwise we won't have the finished MN list
bool CheckCbTxMerkleRoots(const CBlock& block, const CBlockIndex* pindex, const llmq::CQuorumBlockProcessor& quorum_block_processor, BlockValidationState& state, const CCoinsViewCache& view)
{
    if (block.vtx[0]->nType != TRANSACTION_COINBASE) {
        return true;
    }

    static int64_t nTimePayload = 0;

    int64_t nTime1 = GetTimeMicros();

    const auto opt_cbTx = GetTxPayload<CCbTx>(*block.vtx[0]);
    if (!opt_cbTx) {
        return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-cbtx-payload");
    }
    auto cbTx = *opt_cbTx;

    int64_t nTime2 = GetTimeMicros(); nTimePayload += nTime2 - nTime1;
    LogPrint(BCLog::BENCHMARK, "          - GetTxPayload: %.2fms [%.2fs]\n", 0.001 * (nTime2 - nTime1), nTimePayload * 0.000001);

    if (pindex) {
        static int64_t nTimeMerkleMNL = 0;
        static int64_t nTimeMerkleQuorum = 0;

        uint256 calculatedMerkleRoot;
        if (!CalcCbTxMerkleRootMNList(block, pindex->pprev, calculatedMerkleRoot, state, view)) {
            // pass the state returned by the function above
            return false;
        }
        if (calculatedMerkleRoot != cbTx.merkleRootMNList) {
            return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-cbtx-mnmerkleroot");
        }

        int64_t nTime3 = GetTimeMicros(); nTimeMerkleMNL += nTime3 - nTime2;
        LogPrint(BCLog::BENCHMARK, "          - CalcCbTxMerkleRootMNList: %.2fms [%.2fs]\n", 0.001 * (nTime3 - nTime2), nTimeMerkleMNL * 0.000001);

        if (cbTx.nVersion >= CCbTx::Version::MERKLE_ROOT_QUORUMS) {
            if (!CalcCbTxMerkleRootQuorums(block, pindex->pprev, quorum_block_processor, calculatedMerkleRoot, state)) {
                // pass the state returned by the function above
                return false;
            }
            if (calculatedMerkleRoot != cbTx.merkleRootQuorums) {
                return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-cbtx-quorummerkleroot");
            }
        }

        int64_t nTime4 = GetTimeMicros(); nTimeMerkleQuorum += nTime4 - nTime3;
        LogPrint(BCLog::BENCHMARK, "          - CalcCbTxMerkleRootQuorums: %.2fms [%.2fs]\n", 0.001 * (nTime4 - nTime3), nTimeMerkleQuorum * 0.000001);

    }

    return true;
}

bool CalcCbTxMerkleRootMNList(const CBlock& block, const CBlockIndex* pindexPrev, uint256& merkleRootRet, BlockValidationState& state, const CCoinsViewCache& view)
{
    try {
        static std::atomic<int64_t> nTimeDMN = 0;
        static std::atomic<int64_t> nTimeSMNL = 0;
        static std::atomic<int64_t> nTimeMerkle = 0;

        int64_t nTime1 = GetTimeMicros();

        CDeterministicMNList tmpMNList;
        if (!deterministicMNManager->BuildNewListFromBlock(block, pindexPrev, state, view, tmpMNList, false)) {
            // pass the state returned by the function above
            return false;
        }

        int64_t nTime2 = GetTimeMicros(); nTimeDMN += nTime2 - nTime1;
        LogPrint(BCLog::BENCHMARK, "            - BuildNewListFromBlock: %.2fms [%.2fs]\n", 0.001 * (nTime2 - nTime1), nTimeDMN * 0.000001);

        CSimplifiedMNList sml(tmpMNList);

        int64_t nTime3 = GetTimeMicros(); nTimeSMNL += nTime3 - nTime2;
        LogPrint(BCLog::BENCHMARK, "            - CSimplifiedMNList: %.2fms [%.2fs]\n", 0.001 * (nTime3 - nTime2), nTimeSMNL * 0.000001);

        static Mutex cached_mutex;
        static CSimplifiedMNList smlCached GUARDED_BY(cached_mutex);
        static uint256 merkleRootCached GUARDED_BY(cached_mutex);
        static bool mutatedCached GUARDED_BY(cached_mutex) {false};

        LOCK(cached_mutex);
        if (sml == smlCached) {
            merkleRootRet = merkleRootCached;
            if (mutatedCached) {
                return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "mutated-cached-calc-cb-mnmerkleroot");
            }
            return true;
        }

        bool mutated = false;
        merkleRootRet = sml.CalcMerkleRoot(&mutated);

        int64_t nTime4 = GetTimeMicros(); nTimeMerkle += nTime4 - nTime3;
        LogPrint(BCLog::BENCHMARK, "            - CalcMerkleRoot: %.2fms [%.2fs]\n", 0.001 * (nTime4 - nTime3), nTimeMerkle * 0.000001);

        smlCached = std::move(sml);
        merkleRootCached = merkleRootRet;
        mutatedCached = mutated;

        if (mutated) {
            return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "mutated-calc-cb-mnmerkleroot");
        }

        return true;
    } catch (const std::exception& e) {
        LogPrintf("%s -- failed: %s\n", __func__, e.what());
        return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "failed-calc-cb-mnmerkleroot");
    }
}

using QcHashMap = std::map<Consensus::LLMQType, std::vector<uint256>>;
using QcIndexedHashMap = std::map<Consensus::LLMQType, std::map<int16_t, uint256>>;

/**
 * Handles the calculation or caching of qcHashes and qcIndexedHashes
 * @param pindexPrev The const CBlockIndex* (ie a block) of a block. Both the Quorum list and quorum rotation activation status will be retrieved based on this block.
 * @return nullopt if quorumCommitment was unable to be found, otherwise returns the qcHashes and qcIndexedHashes that were calculated or cached
 */
auto CachedGetQcHashesQcIndexedHashes(const CBlockIndex* pindexPrev, const llmq::CQuorumBlockProcessor& quorum_block_processor) ->
        std::optional<std::pair<QcHashMap /*qcHashes*/, QcIndexedHashMap /*qcIndexedHashes*/>> {
    auto quorums = quorum_block_processor.GetMinedAndActiveCommitmentsUntilBlock(pindexPrev);

    static Mutex cs_cache;
    static std::map<Consensus::LLMQType, std::vector<const CBlockIndex*>> quorums_cached GUARDED_BY(cs_cache);
    static QcHashMap qcHashes_cached GUARDED_BY(cs_cache);
    static QcIndexedHashMap qcIndexedHashes_cached GUARDED_BY(cs_cache);

    LOCK(cs_cache);

    if (quorums == quorums_cached) {
        return std::make_pair(qcHashes_cached, qcIndexedHashes_cached);
    }

    // Quorums set is different, reset cached values
    quorums_cached.clear();
    qcHashes_cached.clear();
    qcIndexedHashes_cached.clear();

    for (const auto& [llmqType, vecBlockIndexes] : quorums) {
        const auto& llmq_params_opt = Params().GetLLMQ(llmqType);
        assert(llmq_params_opt.has_value());
        bool rotation_enabled = llmq::IsQuorumRotationEnabled(llmq_params_opt.value(), pindexPrev);
        auto& vec_hashes = qcHashes_cached[llmqType];
        vec_hashes.reserve(vecBlockIndexes.size());
        auto& map_indexed_hashes = qcIndexedHashes_cached[llmqType];
        for (const auto& blockIndex : vecBlockIndexes) {
            uint256 dummyHash;
            llmq::CFinalCommitmentPtr pqc = quorum_block_processor.GetMinedCommitment(llmqType, blockIndex->GetBlockHash(), dummyHash);
            if (pqc == nullptr) {
                // this should never happen
                return std::nullopt;
            }
            auto qcHash = ::SerializeHash(*pqc);
            if (rotation_enabled) {
                map_indexed_hashes[pqc->quorumIndex] = qcHash;
            } else {
                vec_hashes.emplace_back(qcHash);
            }
        }
    }
    quorums_cached = quorums;
    return std::make_pair(qcHashes_cached, qcIndexedHashes_cached);
}

auto CalcHashCountFromQCHashes(const QcHashMap& qcHashes)
{
    size_t hash_count{0};
    for (const auto& [_, vec_hashes] : qcHashes) {
        hash_count += vec_hashes.size();
    }
    return hash_count;
}

bool CalcCbTxMerkleRootQuorums(const CBlock& block, const CBlockIndex* pindexPrev, const llmq::CQuorumBlockProcessor& quorum_block_processor, uint256& merkleRootRet, BlockValidationState& state)
{
    static int64_t nTimeMined = 0;
    static int64_t nTimeLoop = 0;
    static int64_t nTimeMerkle = 0;

    int64_t nTime1 = GetTimeMicros();

    auto retVal = CachedGetQcHashesQcIndexedHashes(pindexPrev, quorum_block_processor);
    if (!retVal) {
        return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "commitment-not-found");
    }
    // The returned quorums are in reversed order, so the most recent one is at index 0
    auto [qcHashes, qcIndexedHashes] = retVal.value();

    int64_t nTime2 = GetTimeMicros(); nTimeMined += nTime2 - nTime1;
    LogPrint(BCLog::BENCHMARK, "            - CachedGetQcHashesQcIndexedHashes: %.2fms [%.2fs]\n", 0.001 * (nTime2 - nTime1), nTimeMined * 0.000001);

    // now add the commitments from the current block, which are not returned by GetMinedAndActiveCommitmentsUntilBlock
    // due to the use of pindexPrev (we don't have the tip index here)
    for (size_t i = 1; i < block.vtx.size(); i++) {
        const auto& tx = block.vtx[i];

        if (tx->nVersion == 3 && tx->nType == TRANSACTION_QUORUM_COMMITMENT) {
            const auto opt_qc = GetTxPayload<llmq::CFinalCommitmentTxPayload>(*tx);
            if (!opt_qc) {
                return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-qc-payload-calc-cbtx-quorummerkleroot");
            }
            if (opt_qc->commitment.IsNull()) {
                // having null commitments is ok but we don't use them here, move to the next tx
                continue;
            }
            const auto& llmq_params_opt = Params().GetLLMQ(opt_qc->commitment.llmqType);
            if (!llmq_params_opt.has_value()) {
                return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-qc-commitment-type-calc-cbtx-quorummerkleroot");
            }
            const auto& llmq_params = llmq_params_opt.value();
            const auto qcHash = ::SerializeHash(opt_qc->commitment);
            if (llmq::IsQuorumRotationEnabled(llmq_params, pindexPrev)) {
                auto& map_indexed_hashes = qcIndexedHashes[opt_qc->commitment.llmqType];
                map_indexed_hashes[opt_qc->commitment.quorumIndex] = qcHash;
            } else {
                auto& vec_hashes = qcHashes[llmq_params.type];
                if (vec_hashes.size() == size_t(llmq_params.signingActiveQuorumCount)) {
                    // we pop the last entry, which is actually the oldest quorum as GetMinedAndActiveCommitmentsUntilBlock
                    // returned quorums in reversed order. This pop and later push can only work ONCE, but we rely on the
                    // fact that a block can only contain a single commitment for one LLMQ type
                    vec_hashes.pop_back();
                }
                vec_hashes.emplace_back(qcHash);
            }
        }
    }

    for (const auto& [llmqType, map_indexed_hashes] : qcIndexedHashes) {
        auto& vec_hashes = qcHashes[llmqType];
        for (const auto& [_, hash] : map_indexed_hashes) {
            vec_hashes.emplace_back(hash);
        }
    }

    std::vector<uint256> vec_hashes_final;
    vec_hashes_final.reserve(CalcHashCountFromQCHashes(qcHashes));

    for (const auto& [llmqType, vec_hashes] : qcHashes) {
        const auto& llmq_params_opt = Params().GetLLMQ(llmqType);
        assert(llmq_params_opt.has_value());
        if (vec_hashes.size() > size_t(llmq_params_opt->signingActiveQuorumCount)) {
            return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "excess-quorums-calc-cbtx-quorummerkleroot");
        }
        // Copy vec_hashes into vec_hashes_final
        std::copy(vec_hashes.begin(), vec_hashes.end(), std::back_inserter(vec_hashes_final));
    }
    std::sort(vec_hashes_final.begin(), vec_hashes_final.end());

    int64_t nTime3 = GetTimeMicros(); nTimeLoop += nTime3 - nTime2;
    LogPrint(BCLog::BENCHMARK, "            - Loop: %.2fms [%.2fs]\n", 0.001 * (nTime3 - nTime2), nTimeLoop * 0.000001);

    bool mutated = false;
    merkleRootRet = ComputeMerkleRoot(vec_hashes_final, &mutated);

    int64_t nTime4 = GetTimeMicros(); nTimeMerkle += nTime4 - nTime3;
    LogPrint(BCLog::BENCHMARK, "            - ComputeMerkleRoot: %.2fms [%.2fs]\n", 0.001 * (nTime4 - nTime3), nTimeMerkle * 0.000001);

    if (mutated) {
        return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "mutated-calc-cbtx-quorummerkleroot");
    }

    return true;
}

bool CheckCbTxBestChainlock(const CBlock& block, const CBlockIndex* pindex, const llmq::CChainLocksHandler& chainlock_handler, BlockValidationState& state)
{
    if (block.vtx[0]->nType != TRANSACTION_COINBASE) {
        return true;
    }

    const auto opt_cbTx = GetTxPayload<CCbTx>(*block.vtx[0]);
    if (!opt_cbTx) {
        return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-cbtx-payload");
    }

    if (opt_cbTx->nVersion < CCbTx::Version::CLSIG_AND_BALANCE) {
        return true;
    }

    auto best_clsig = chainlock_handler.GetBestChainLock();
    if (best_clsig.getHeight() == pindex->nHeight - 1 && opt_cbTx->bestCLHeightDiff == 0 && opt_cbTx->bestCLSignature == best_clsig.getSig()) {
        // matches our best clsig which still hold values for the previous block
        return true;
    }

    auto prevBlockCoinbaseChainlock = GetNonNullCoinbaseChainlock(pindex);
    // If std::optional prevBlockCoinbaseChainlock is empty, then up to the previous block, coinbase Chainlock is null.
    if (prevBlockCoinbaseChainlock.has_value()) {
        // Previous block Coinbase has a non-null Chainlock: current block's Chainlock must be non-null and at least as new as the previous one
        if (!opt_cbTx->bestCLSignature.IsValid()) {
            // IsNull() doesn't exist for CBLSSignature: we assume that a non valid BLS sig is null
            return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-cbtx-null-clsig");
        }
        int prevBlockCoinbaseCLHeight = pindex->nHeight - static_cast<int>(prevBlockCoinbaseChainlock.value().second) - 1;
        int curBlockCoinbaseCLHeight = pindex->nHeight - static_cast<int>(opt_cbTx->bestCLHeightDiff) - 1;
        if (curBlockCoinbaseCLHeight < prevBlockCoinbaseCLHeight) {
            return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-cbtx-older-clsig");
        }
    }

    // IsNull() doesn't exist for CBLSSignature: we assume that a valid BLS sig is non-null
    if (opt_cbTx->bestCLSignature.IsValid()) {
        int curBlockCoinbaseCLHeight = pindex->nHeight - static_cast<int>(opt_cbTx->bestCLHeightDiff) - 1;
        if (best_clsig.getHeight() == curBlockCoinbaseCLHeight && best_clsig.getSig() == opt_cbTx->bestCLSignature) {
            // matches our best (but outdated) clsig, no need to verify it again
            return true;
        }
        uint256 curBlockCoinbaseCLBlockHash = pindex->GetAncestor(curBlockCoinbaseCLHeight)->GetBlockHash();
        if (!chainlock_handler.VerifyChainLock(llmq::CChainLockSig(curBlockCoinbaseCLHeight, curBlockCoinbaseCLBlockHash, opt_cbTx->bestCLSignature))) {
            return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-cbtx-invalid-clsig");
        }
    } else if (opt_cbTx->bestCLHeightDiff != 0) {
        // Null bestCLSignature is allowed only with bestCLHeightDiff = 0
        return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-cbtx-cldiff");
    }

    return true;
}

bool CalcCbTxBestChainlock(const llmq::CChainLocksHandler& chainlock_handler, const CBlockIndex* pindexPrev, uint32_t& bestCLHeightDiff, CBLSSignature& bestCLSignature)
{
    auto best_clsig = chainlock_handler.GetBestChainLock();
    if (best_clsig.getHeight() == pindexPrev->nHeight) {
        // Our best CL is the newest one possible
        bestCLHeightDiff = 0;
        bestCLSignature = best_clsig.getSig();
        return true;
    }

    auto prevBlockCoinbaseChainlock = GetNonNullCoinbaseChainlock(pindexPrev);
    if (prevBlockCoinbaseChainlock.has_value()) {
        // Previous block Coinbase contains a non-null CL: We must insert the same sig or a better (newest) one
        if (best_clsig.IsNull()) {
            // We don't know any CL, therefore inserting the CL of the previous block
            bestCLHeightDiff = prevBlockCoinbaseChainlock->second + 1;
            bestCLSignature = prevBlockCoinbaseChainlock->first;
            return true;
        }

        // We check if our best CL is newer than the one from previous block Coinbase
        int curCLHeight = best_clsig.getHeight();
        int prevCLHeight = pindexPrev->nHeight - static_cast<int>(prevBlockCoinbaseChainlock->second) - 1;
        if (curCLHeight < prevCLHeight) {
            // Our best CL isn't newer: inserting CL from previous block
            bestCLHeightDiff = prevBlockCoinbaseChainlock->second + 1;
            bestCLSignature = prevBlockCoinbaseChainlock->first;
        }
        else {
            // Our best CL is newer
            bestCLHeightDiff = pindexPrev->nHeight - best_clsig.getHeight();
            bestCLSignature = best_clsig.getSig();
        }

        return true;
    }
    else {
        // Previous block Coinbase has no CL. We can either insert null or any valid CL
        if (best_clsig.IsNull()) {
            // We don't know any CL, therefore inserting a null CL
            bestCLHeightDiff = 0;
            bestCLSignature.Reset();
            return false;
        }

        // Inserting our best CL
        bestCLHeightDiff = pindexPrev->nHeight - best_clsig.getHeight();
        bestCLSignature = chainlock_handler.GetBestChainLock().getSig();

        return true;
    }
}


std::string CCbTx::ToString() const
{
    return strprintf("CCbTx(nVersion=%d, nHeight=%d, merkleRootMNList=%s, merkleRootQuorums=%s, bestCLHeightDiff=%d, bestCLSig=%s, creditPoolBalance=%d.%08d)",
        static_cast<uint16_t>(nVersion), nHeight, merkleRootMNList.ToString(), merkleRootQuorums.ToString(), bestCLHeightDiff, bestCLSignature.ToString(),
        creditPoolBalance / COIN, creditPoolBalance % COIN);
}

std::optional<CCbTx> GetCoinbaseTx(const CBlockIndex* pindex)
{
    if (pindex == nullptr) {
        return std::nullopt;
    }

    CBlock block;
    if (!ReadBlockFromDisk(block, pindex, Params().GetConsensus())) {
        return std::nullopt;
    }

    CTransactionRef cbTx = block.vtx[0];
    return GetTxPayload<CCbTx>(*cbTx);
}

std::optional<std::pair<CBLSSignature, uint32_t>> GetNonNullCoinbaseChainlock(const CBlockIndex* pindex)
{
    auto opt_cbtx = GetCoinbaseTx(pindex);

    if (!opt_cbtx.has_value()) {
        return std::nullopt;
    }

    CCbTx& cbtx = opt_cbtx.value();

    if (cbtx.nVersion < CCbTx::Version::CLSIG_AND_BALANCE) {
        return std::nullopt;
    }

    if (!cbtx.bestCLSignature.IsValid()) {
        return std::nullopt;
    }

    return std::make_pair(cbtx.bestCLSignature, cbtx.bestCLHeightDiff);
}
