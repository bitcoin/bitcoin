// Copyright (c) 2017-2019 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <evo/cbtx.h>
#include <evo/deterministicmns.h>
#include <llmq/quorums_blockprocessor.h>
#include <llmq/quorums_commitment.h>
#include <evo/simplifiedmns.h>
#include <evo/specialtx.h>

#include <chainparams.h>
#include <consensus/merkle.h>
#include <univalue.h>
#include <validation.h>

bool CheckCbTx(const CTransaction& tx, const CBlockIndex* pindexPrev, TxValidationState& state, bool fJustCheck)
{
    if (tx.nVersion != SYSCOIN_TX_VERSION_MN_COINBASE) {
        return FormatSyscoinErrorMessage(state, "bad-cbtx-type", fJustCheck);
    }

    if (!tx.IsCoinBase()) {
        return FormatSyscoinErrorMessage(state, "bad-cbtx-invalid", fJustCheck);
    }

    CCbTx cbTx;
    if (!GetTxPayload(tx, cbTx)) {
        return FormatSyscoinErrorMessage(state, "bad-cbtx-payload", fJustCheck);
    }

    if (cbTx.nVersion == 0 || cbTx.nVersion > CCbTx::CURRENT_VERSION) {
        return FormatSyscoinErrorMessage(state, "bad-cbtx-version", fJustCheck);
    }

    if (pindexPrev && pindexPrev->nHeight + 1 != cbTx.nHeight) {
        return FormatSyscoinErrorMessage(state, "bad-cbtx-height", fJustCheck);
    }

    if (pindexPrev) {
        if (cbTx.nVersion < 2) {
            return FormatSyscoinErrorMessage(state, "bad-cbtx-version", fJustCheck);
        }
    }

    return true;
}
bool CheckCbTx(const CCbTx &cbTx, const CBlockIndex* pindexPrev, TxValidationState& state, bool fJustCheck)
{
    if (cbTx.nVersion == 0 || cbTx.nVersion > CCbTx::CURRENT_VERSION) {
        return FormatSyscoinErrorMessage(state, "bad-cbtx-version", fJustCheck);
    }

    if (pindexPrev && pindexPrev->nHeight + 1 != cbTx.nHeight) {
        return FormatSyscoinErrorMessage(state, "bad-cbtx-height", fJustCheck);
    }

    if (pindexPrev) {
        if (cbTx.nVersion < 2) {
            return FormatSyscoinErrorMessage(state, "bad-cbtx-version", fJustCheck);
        }
    }

    return true;
}
// This can only be done after the block has been fully processed, as otherwise we won't have the finished MN list
bool CheckCbTxMerkleRoots(const CBlock& block, const CBlockIndex* pindex, BlockValidationState& state, CCoinsViewCache& view)
{
    if (block.vtx[0]->nVersion != SYSCOIN_TX_VERSION_MN_COINBASE && block.vtx[0]->nVersion != SYSCOIN_TX_VERSION_MN_QUORUM_COMMITMENT) {
        return true;
    }

    static int64_t nTimePayload = 0;

    int64_t nTime1 = GetTimeMicros();

    CCbTx cbTx;
    if (!GetTxPayload(*block.vtx[0], cbTx)) {
        return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-cbtx-payload");
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
            return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-cbtx-mnmerkleroot");
        }

        int64_t nTime3 = GetTimeMicros(); nTimeMerkleMNL += nTime3 - nTime2;
        LogPrint(BCLog::BENCHMARK, "          - CalcCbTxMerkleRootMNList: %.2fms [%.2fs]\n", 0.001 * (nTime3 - nTime2), nTimeMerkleMNL * 0.000001);

        
        if (!CalcCbTxMerkleRootQuorums(block, pindex->pprev, calculatedMerkleRoot, state)) {
            // pass the state returned by the function above
            return false;
        }
        if (calculatedMerkleRoot != cbTx.merkleRootQuorums) {
            return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-cbtx-quorummerkleroot");
        }
        

        int64_t nTime4 = GetTimeMicros(); nTimeMerkleQuorum += nTime4 - nTime3;
        LogPrint(BCLog::BENCHMARK, "          - CalcCbTxMerkleRootQuorums: %.2fms [%.2fs]\n", 0.001 * (nTime4 - nTime3), nTimeMerkleQuorum * 0.000001);

    }

    return true;
}

bool CalcCbTxMerkleRootMNList(const CBlock& block, const CBlockIndex* pindexPrev, uint256& merkleRootRet, BlockValidationState& state,  CCoinsViewCache& view, const llmq::CFinalCommitmentTxPayload *qcIn)
{
    if(!deterministicMNManager)
        return false;
    LOCK(deterministicMNManager->cs);

    try {
        static int64_t nTimeDMN = 0;
        static int64_t nTimeSMNL = 0;
        static int64_t nTimeMerkle = 0;

        int64_t nTime1 = GetTimeMicros();

        CDeterministicMNList tmpMNList;
        if (!deterministicMNManager->BuildNewListFromBlock(block, pindexPrev, state, view, tmpMNList, false, qcIn)) {
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

bool CalcCbTxMerkleRootQuorums(const CBlock& block, const CBlockIndex* pindexPrev, uint256& merkleRootRet, BlockValidationState& state, const llmq::CFinalCommitmentTxPayload *qcIn)
{
    AssertLockHeld(cs_main);
    static int64_t nTimeMinedAndActive = 0;
    static int64_t nTimeMined = 0;
    static int64_t nTimeLoop = 0;
    static int64_t nTimeMerkle = 0;

    int64_t nTime1 = GetTimeMicros();

    static std::map<uint8_t, std::vector<const CBlockIndex*>> quorumsCached;
    static std::map<uint8_t, std::vector<uint256>> qcHashesCached;

    // The returned quorums are in reversed order, so the most recent one is at index 0
    std::map<uint8_t, std::vector<const CBlockIndex*>> quorums;
    llmq::quorumBlockProcessor->GetMinedAndActiveCommitmentsUntilBlock(pindexPrev, quorums);
    std::map<uint8_t, std::vector<uint256>> qcHashes;
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
                if (qc == nullptr) return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "commitment-not-found");
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
    const bool &IsQCIn = qcIn && !qcIn->IsNull();
    // qcIn passed in by createnewblock, but connectblock will pass in null, use gettxpayload there if version is for mn quorum
    if (IsQCIn || (block.vtx[0] && block.vtx[0]->nVersion == SYSCOIN_TX_VERSION_MN_QUORUM_COMMITMENT)) {
        llmq::CFinalCommitmentTxPayload qc;
        if(IsQCIn)
            qc = *qcIn;
        else if (!GetTxPayload(*block.vtx[0], qc)) {
            return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-qc-payload-calc-cbtx-quorummerkleroot");
        }
        for(const auto& commitment: qc.commitments) {
            if (commitment.IsNull()) {
                continue;
            }
            auto qcHash = ::SerializeHash(commitment);
            const auto& params = Params().GetConsensus().llmqs.at(commitment.llmqType);
            auto& v = qcHashes[params.type];
            if (v.size() == (size_t)params.signingActiveQuorumCount) {
                // we pop the last entry, which is actually the oldest quorum as GetMinedAndActiveCommitmentsUntilBlock
                // returned quorums in reversed order. This pop and later push can only work ONCE, but we rely on the
                // fact that a block can only contain a single commitment for one LLMQ type
                v.pop_back();
            }
            v.emplace_back(qcHash);
            hashCount++;
            if (v.size() > (size_t)params.signingActiveQuorumCount) {
                return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "excess-quorums-calc-cbtx-quorummerkleroot");
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
        return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "mutated-calc-cbtx-quorummerkleroot");
    }

    return true;
}

std::string CCbTx::ToString() const
{
    return strprintf("CCbTx(nVersion=%d, nHeight=%d, merkleRootMNList=%s, merkleRootQuorums=%s)",
        nVersion, nHeight, merkleRootMNList.ToString(), merkleRootQuorums.ToString());
}
