// Copyright (c) 2017-2022 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <evo/cbtx.h>
#include <evo/deterministicmns.h>
#include <llmq/blockprocessor.h>
#include <llmq/commitment.h>
#include <llmq/utils.h>
#include <evo/simplifiedmns.h>
#include <evo/specialtx.h>
#include <consensus/validation.h>

#include <chain.h>
#include <chainparams.h>
#include <consensus/merkle.h>

bool CheckCbTx(const CTransaction& tx, const CBlockIndex* pindexPrev, CValidationState& state)
{
    if (tx.nType != TRANSACTION_COINBASE) {
        return state.Invalid(ValidationInvalidReason::CONSENSUS, false, REJECT_INVALID, "bad-cbtx-type");
    }

    if (!tx.IsCoinBase()) {
        return state.Invalid(ValidationInvalidReason::CONSENSUS, false, REJECT_INVALID, "bad-cbtx-invalid");
    }

    CCbTx cbTx;
    if (!GetTxPayload(tx, cbTx)) {
        return state.Invalid(ValidationInvalidReason::CONSENSUS, false, REJECT_INVALID, "bad-cbtx-payload");
    }

    if (cbTx.nVersion == 0 || cbTx.nVersion > CCbTx::CURRENT_VERSION) {
        return state.Invalid(ValidationInvalidReason::CONSENSUS, false, REJECT_INVALID, "bad-cbtx-version");
    }

    if (pindexPrev && pindexPrev->nHeight + 1 != cbTx.nHeight) {
        return state.Invalid(ValidationInvalidReason::CONSENSUS, false, REJECT_INVALID, "bad-cbtx-height");
    }

    if (pindexPrev) {
        bool fDIP0008Active = pindexPrev->nHeight >= Params().GetConsensus().DIP0008Height;
        if (fDIP0008Active && cbTx.nVersion < 2) {
            return state.Invalid(ValidationInvalidReason::CONSENSUS, false, REJECT_INVALID, "bad-cbtx-version");
        }
    }

    return true;
}

// This can only be done after the block has been fully processed, as otherwise we won't have the finished MN list
bool CheckCbTxMerkleRoots(const CBlock& block, const CBlockIndex* pindex, const llmq::CQuorumBlockProcessor& quorum_block_processor, CValidationState& state, const CCoinsViewCache& view)
{
    if (block.vtx[0]->nType != TRANSACTION_COINBASE) {
        return true;
    }

    static int64_t nTimePayload = 0;

    int64_t nTime1 = GetTimeMicros();

    CCbTx cbTx;
    if (!GetTxPayload(*block.vtx[0], cbTx)) {
        return state.Invalid(ValidationInvalidReason::CONSENSUS, false, REJECT_INVALID, "bad-cbtx-payload");
    }

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
            return state.Invalid(ValidationInvalidReason::CONSENSUS, false, REJECT_INVALID, "bad-cbtx-mnmerkleroot");
        }

        int64_t nTime3 = GetTimeMicros(); nTimeMerkleMNL += nTime3 - nTime2;
        LogPrint(BCLog::BENCHMARK, "          - CalcCbTxMerkleRootMNList: %.2fms [%.2fs]\n", 0.001 * (nTime3 - nTime2), nTimeMerkleMNL * 0.000001);

        if (cbTx.nVersion >= 2) {
            if (!CalcCbTxMerkleRootQuorums(block, pindex->pprev, quorum_block_processor, calculatedMerkleRoot, state)) {
                // pass the state returned by the function above
                return false;
            }
            if (calculatedMerkleRoot != cbTx.merkleRootQuorums) {
                return state.Invalid(ValidationInvalidReason::CONSENSUS, false, REJECT_INVALID, "bad-cbtx-quorummerkleroot");
            }
        }

        int64_t nTime4 = GetTimeMicros(); nTimeMerkleQuorum += nTime4 - nTime3;
        LogPrint(BCLog::BENCHMARK, "          - CalcCbTxMerkleRootQuorums: %.2fms [%.2fs]\n", 0.001 * (nTime4 - nTime3), nTimeMerkleQuorum * 0.000001);

    }

    return true;
}

bool CalcCbTxMerkleRootMNList(const CBlock& block, const CBlockIndex* pindexPrev, uint256& merkleRootRet, CValidationState& state, const CCoinsViewCache& view)
{
    LOCK(deterministicMNManager->cs);

    try {
        static int64_t nTimeDMN = 0;
        static int64_t nTimeSMNL = 0;
        static int64_t nTimeMerkle = 0;

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

        static CSimplifiedMNList smlCached;
        static uint256 merkleRootCached;
        static bool mutatedCached{false};

        if (sml == smlCached) {
            merkleRootRet = merkleRootCached;
            if (mutatedCached) {
                return state.Invalid(ValidationInvalidReason::CONSENSUS, false, REJECT_INVALID, "mutated-cached-calc-cb-mnmerkleroot");
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
            return state.Invalid(ValidationInvalidReason::CONSENSUS, false, REJECT_INVALID, "mutated-calc-cb-mnmerkleroot");
        }

        return true;
    } catch (const std::exception& e) {
        LogPrintf("%s -- failed: %s\n", __func__, e.what());
        return state.Invalid(ValidationInvalidReason::CONSENSUS, false, REJECT_INVALID, "failed-calc-cb-mnmerkleroot");
    }
}

using QcHashMap = std::map<Consensus::LLMQType, std::vector<uint256>>;
using QcIndexedHashMap = std::map<Consensus::LLMQType, std::map<int16_t, uint256>>;

/**
 * Handles the calculation or caching of qcHashes and qcIndexedHashes
 * @param pindexPrev The const CBlockIndex* (ie a block) of a block. Both the Quorum list and quorum rotation actiavtion status will be retrieved based on this block.
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
        const auto& llmq_params_opt = llmq::GetLLMQParams(llmqType);
        assert(llmq_params_opt.has_value());
        bool rotation_enabled = llmq::utils::IsQuorumRotationEnabled(llmq_params_opt.value(), pindexPrev);
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

bool CalcCbTxMerkleRootQuorums(const CBlock& block, const CBlockIndex* pindexPrev, const llmq::CQuorumBlockProcessor& quorum_block_processor, uint256& merkleRootRet, CValidationState& state)
{
    static int64_t nTimeMined = 0;
    static int64_t nTimeLoop = 0;
    static int64_t nTimeMerkle = 0;

    int64_t nTime1 = GetTimeMicros();

    auto retVal = CachedGetQcHashesQcIndexedHashes(pindexPrev, quorum_block_processor);
    if (!retVal) {
        return state.Invalid(ValidationInvalidReason::CONSENSUS, false, REJECT_INVALID, "commitment-not-found");
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
            llmq::CFinalCommitmentTxPayload qc;
            if (!GetTxPayload(*tx, qc)) {
                return state.Invalid(ValidationInvalidReason::CONSENSUS, false, REJECT_INVALID, "bad-qc-payload-calc-cbtx-quorummerkleroot");
            }
            if (qc.commitment.IsNull()) {
                // having null commitments is ok but we don't use them here, move to the next tx
                continue;
            }
            const auto& llmq_params_opt = llmq::GetLLMQParams(qc.commitment.llmqType);
            if (!llmq_params_opt.has_value()) {
                return state.Invalid(ValidationInvalidReason::CONSENSUS, false, REJECT_INVALID, "bad-qc-commitment-type-calc-cbtx-quorummerkleroot");
            }
            const auto& llmq_params = llmq_params_opt.value();
            auto qcHash = ::SerializeHash(qc.commitment);
            if (llmq::utils::IsQuorumRotationEnabled(llmq_params, pindexPrev)) {
                auto& map_indexed_hashes = qcIndexedHashes[qc.commitment.llmqType];
                map_indexed_hashes[qc.commitment.quorumIndex] = qcHash;
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
        const auto& llmq_params_opt = llmq::GetLLMQParams(llmqType);
        assert(llmq_params_opt.has_value());
        if (vec_hashes.size() > size_t(llmq_params_opt->signingActiveQuorumCount)) {
            return state.Invalid(ValidationInvalidReason::CONSENSUS, false, REJECT_INVALID, "excess-quorums-calc-cbtx-quorummerkleroot");
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
        return state.Invalid(ValidationInvalidReason::CONSENSUS, false, REJECT_INVALID, "mutated-calc-cbtx-quorummerkleroot");
    }

    return true;
}

std::string CCbTx::ToString() const
{
    return strprintf("CCbTx(nVersion=%d, nHeight=%d, merkleRootMNList=%s, merkleRootQuorums=%s)",
        nVersion, nHeight, merkleRootMNList.ToString(), merkleRootQuorums.ToString());
}
