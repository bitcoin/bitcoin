// Copyright (c) 2017-2021 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <evo/cbtx.h>
#include <evo/deterministicmns.h>
#include <llmq/quorums_blockprocessor.h>
#include <llmq/quorums_commitment.h>
#include <evo/simplifiedmns.h>
#include <evo/specialtx.h>
#include <consensus/validation.h>

#include <chain.h>
#include <chainparams.h>
#include <consensus/merkle.h>

bool CheckCbTx(const CTransaction& tx, const CBlockIndex* pindexPrev, CValidationState& state)
{
    if (tx.nType != TRANSACTION_COINBASE) {
        return state.DoS(100, false, REJECT_INVALID, "bad-cbtx-type");
    }

    if (!tx.IsCoinBase()) {
        return state.DoS(100, false, REJECT_INVALID, "bad-cbtx-invalid");
    }

    CCbTx cbTx;
    if (!GetTxPayload(tx, cbTx)) {
        return state.DoS(100, false, REJECT_INVALID, "bad-cbtx-payload");
    }

    if (cbTx.nVersion == 0 || cbTx.nVersion > CCbTx::CURRENT_VERSION) {
        return state.DoS(100, false, REJECT_INVALID, "bad-cbtx-version");
    }

    if (pindexPrev && pindexPrev->nHeight + 1 != cbTx.nHeight) {
        return state.DoS(100, false, REJECT_INVALID, "bad-cbtx-height");
    }

    if (pindexPrev) {
        bool fDIP0008Active = pindexPrev->nHeight >= Params().GetConsensus().DIP0008Height;
        if (fDIP0008Active && cbTx.nVersion < 2) {
            return state.DoS(100, false, REJECT_INVALID, "bad-cbtx-version");
        }
    }

    return true;
}

// This can only be done after the block has been fully processed, as otherwise we won't have the finished MN list
bool CheckCbTxMerkleRoots(const CBlock& block, const CBlockIndex* pindex, CValidationState& state, const CCoinsViewCache& view)
{
    if (block.vtx[0]->nType != TRANSACTION_COINBASE) {
        return true;
    }

    static int64_t nTimePayload = 0;

    int64_t nTime1 = GetTimeMicros();

    CCbTx cbTx;
    if (!GetTxPayload(*block.vtx[0], cbTx)) {
        return state.DoS(100, false, REJECT_INVALID, "bad-cbtx-payload");
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
            return state.DoS(100, false, REJECT_INVALID, "bad-cbtx-mnmerkleroot");
        }

        int64_t nTime3 = GetTimeMicros(); nTimeMerkleMNL += nTime3 - nTime2;
        LogPrint(BCLog::BENCHMARK, "          - CalcCbTxMerkleRootMNList: %.2fms [%.2fs]\n", 0.001 * (nTime3 - nTime2), nTimeMerkleMNL * 0.000001);

        if (cbTx.nVersion >= 2) {
            if (!CalcCbTxMerkleRootQuorums(block, pindex->pprev, calculatedMerkleRoot, state)) {
                // pass the state returned by the function above
                return false;
            }
            if (calculatedMerkleRoot != cbTx.merkleRootQuorums) {
                return state.DoS(100, false, REJECT_INVALID, "bad-cbtx-quorummerkleroot");
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

        if (sml.mnList == smlCached.mnList) {
            merkleRootRet = merkleRootCached;
            if (mutatedCached) {
                return state.DoS(100, false, REJECT_INVALID, "mutated-cached-calc-cb-mnmerkleroot");
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
            return state.DoS(100, false, REJECT_INVALID, "mutated-calc-cb-mnmerkleroot");
        }

        return true;
    } catch (const std::exception& e) {
        LogPrintf("%s -- failed: %s\n", __func__, e.what());
        return state.DoS(100, false, REJECT_INVALID, "failed-calc-cb-mnmerkleroot");
    }
}

bool CalcCbTxMerkleRootQuorums(const CBlock& block, const CBlockIndex* pindexPrev, uint256& merkleRootRet, CValidationState& state)
{
    static int64_t nTimeMinedAndActive = 0;
    static int64_t nTimeMined = 0;
    static int64_t nTimeLoop = 0;
    static int64_t nTimeMerkle = 0;

    int64_t nTime1 = GetTimeMicros();

    static std::map<Consensus::LLMQType, std::vector<const CBlockIndex*>> quorumsCached;
    static std::map<Consensus::LLMQType, std::vector<uint256>> qcHashesCached;

    // The returned quorums are in reversed order, so the most recent one is at index 0
    auto quorums = llmq::quorumBlockProcessor->GetMinedAndActiveCommitmentsUntilBlock(pindexPrev);
    std::map<Consensus::LLMQType, std::vector<uint256>> qcHashes;
    size_t hashCount = 0;

    int64_t nTime2 = GetTimeMicros(); nTimeMinedAndActive += nTime2 - nTime1;
    LogPrint(BCLog::BENCHMARK, "            - GetMinedAndActiveCommitmentsUntilBlock: %.2fms [%.2fs]\n", 0.001 * (nTime2 - nTime1), nTimeMinedAndActive * 0.000001);

    if (quorums == quorumsCached) {
        qcHashes = qcHashesCached;
    } else {
        for (const auto& p : quorums) {
            auto& v = qcHashes[p.first];
            v.reserve(p.second.size());
            for (const auto& p2 : p.second) {
                uint256 minedBlockHash;
                llmq::CFinalCommitmentPtr qc = llmq::quorumBlockProcessor->GetMinedCommitment(p.first, p2->GetBlockHash(), minedBlockHash);
                if (qc == nullptr) return state.DoS(100, false, REJECT_INVALID, "commitment-not-found");
                v.emplace_back(::SerializeHash(*qc));
                hashCount++;
            }
        }
        quorumsCached = quorums;
        qcHashesCached = qcHashes;
    }

    int64_t nTime3 = GetTimeMicros(); nTimeMined += nTime3 - nTime2;
    LogPrint(BCLog::BENCHMARK, "            - GetMinedCommitment: %.2fms [%.2fs]\n", 0.001 * (nTime3 - nTime2), nTimeMined * 0.000001);

    // now add the commitments from the current block, which are not returned by GetMinedAndActiveCommitmentsUntilBlock
    // due to the use of pindexPrev (we don't have the tip index here)
    for (size_t i = 1; i < block.vtx.size(); i++) {
        auto& tx = block.vtx[i];

        if (tx->nVersion == 3 && tx->nType == TRANSACTION_QUORUM_COMMITMENT) {
            llmq::CFinalCommitmentTxPayload qc;
            if (!GetTxPayload(*tx, qc)) {
                return state.DoS(100, false, REJECT_INVALID, "bad-qc-payload-calc-cbtx-quorummerkleroot");
            }
            if (qc.commitment.IsNull()) {
                continue;
            }
            auto qcHash = ::SerializeHash(qc.commitment);
            const auto& llmq_params = llmq::GetLLMQParams(qc.commitment.llmqType);
            auto& v = qcHashes[llmq_params.type];
            if (v.size() == llmq_params.signingActiveQuorumCount) {
                // we pop the last entry, which is actually the oldest quorum as GetMinedAndActiveCommitmentsUntilBlock
                // returned quorums in reversed order. This pop and later push can only work ONCE, but we rely on the
                // fact that a block can only contain a single commitment for one LLMQ type
                v.pop_back();
            }
            v.emplace_back(qcHash);
            hashCount++;
            if (v.size() > llmq_params.signingActiveQuorumCount) {
                return state.DoS(100, false, REJECT_INVALID, "excess-quorums-calc-cbtx-quorummerkleroot");
            }
        }
    }

    std::vector<uint256> qcHashesVec;
    qcHashesVec.reserve(hashCount);

    for (const auto& p : qcHashes) {
        // Copy p.second into qcHashesVec
        std::copy(p.second.begin(), p.second.end(), std::back_inserter(qcHashesVec));
    }
    std::sort(qcHashesVec.begin(), qcHashesVec.end());

    int64_t nTime4 = GetTimeMicros(); nTimeLoop += nTime4 - nTime3;
    LogPrint(BCLog::BENCHMARK, "            - Loop: %.2fms [%.2fs]\n", 0.001 * (nTime4 - nTime3), nTimeLoop * 0.000001);

    bool mutated = false;
    merkleRootRet = ComputeMerkleRoot(qcHashesVec, &mutated);

    int64_t nTime5 = GetTimeMicros(); nTimeMerkle += nTime5 - nTime4;
    LogPrint(BCLog::BENCHMARK, "            - ComputeMerkleRoot: %.2fms [%.2fs]\n", 0.001 * (nTime5 - nTime4), nTimeMerkle * 0.000001);

    if (mutated) {
        return state.DoS(100, false, REJECT_INVALID, "mutated-calc-cbtx-quorummerkleroot");
    }

    return true;
}

std::string CCbTx::ToString() const
{
    return strprintf("CCbTx(nVersion=%d, nHeight=%d, merkleRootMNList=%s, merkleRootQuorums=%s)",
        nVersion, nHeight, merkleRootMNList.ToString(), merkleRootQuorums.ToString());
}
