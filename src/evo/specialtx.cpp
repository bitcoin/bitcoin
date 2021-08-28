// Copyright (c) 2018-2019 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>
#include <consensus/validation.h>
#include <hash.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <validation.h>

#include <evo/cbtx.h>
#include <evo/deterministicmns.h>
#include <evo/specialtx.h>

#include <llmq/quorums_commitment.h>
#include <llmq/quorums_blockprocessor.h>
class CCoinsViewCache;
bool CheckSpecialTx(BlockManager &blockman, const CTransaction& tx, const CBlockIndex* pindexPrev, TxValidationState& state, CCoinsViewCache& view, bool fJustCheck)
{

    try {
        switch (tx.nVersion) {
        case SYSCOIN_TX_VERSION_MN_REGISTER:
            return CheckProRegTx(tx, pindexPrev, state, view, fJustCheck);
        case SYSCOIN_TX_VERSION_MN_UPDATE_SERVICE:
            return CheckProUpServTx(tx, pindexPrev, state, fJustCheck);
        case SYSCOIN_TX_VERSION_MN_UPDATE_REGISTRAR:
            return CheckProUpRegTx(tx, pindexPrev, state, view, fJustCheck);
        case SYSCOIN_TX_VERSION_MN_UPDATE_REVOKE:
            return CheckProUpRevTx(tx, pindexPrev, state, fJustCheck);
        case SYSCOIN_TX_VERSION_MN_COINBASE:
            return CheckCbTx(tx, pindexPrev, state, fJustCheck);
        case SYSCOIN_TX_VERSION_MN_QUORUM_COMMITMENT:
            return llmq::CheckLLMQCommitment(blockman, tx, pindexPrev, state, fJustCheck);
        default:
            return true;
        }
    } catch (const std::exception& e) {
        LogPrintf("%s -- failed: %s\n", __func__, e.what());
        return FormatSyscoinErrorMessage(state, "failed-check-special-tx", fJustCheck);
    }

    return FormatSyscoinErrorMessage(state, "bad-tx-type-check", fJustCheck);
}

bool IsSpecialTx(const CTransaction& tx)
{
    switch (tx.nVersion) {
    case SYSCOIN_TX_VERSION_MN_REGISTER:
    case SYSCOIN_TX_VERSION_MN_UPDATE_SERVICE:
    case SYSCOIN_TX_VERSION_MN_UPDATE_REGISTRAR:
    case SYSCOIN_TX_VERSION_MN_UPDATE_REVOKE:
    case SYSCOIN_TX_VERSION_MN_COINBASE:
    case SYSCOIN_TX_VERSION_MN_QUORUM_COMMITMENT:
        return true;
    default:
        return false;
    }

    return false;
}

bool ProcessSpecialTxsInBlock(BlockManager &blockman, const CBlock& block, const CBlockIndex* pindex, BlockValidationState& state, CCoinsViewCache& view, bool fJustCheck, bool fCheckCbTxMerleRoots)
{
    try {
        static int64_t nTimeLoop = 0;
        static int64_t nTimeQuorum = 0;
        static int64_t nTimeDMN = 0;
        static int64_t nTimeMerkle = 0;

        int64_t nTime1 = GetTimeMicros();

        for (const auto& ptr_tx : block.vtx) {
            TxValidationState txstate;
            if (!CheckSpecialTx(blockman, *ptr_tx, pindex->pprev, txstate, view, false)) {
                return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, txstate.GetRejectReason());
            }
        }

        int64_t nTime2 = GetTimeMicros(); nTimeLoop += nTime2 - nTime1;
        LogPrint(BCLog::BENCHMARK, "        - Loop: %.2fms [%.2fs]\n", 0.001 * (nTime2 - nTime1), nTimeLoop * 0.000001);

        if (!llmq::quorumBlockProcessor->ProcessBlock(block, pindex, state, fJustCheck)) {
            // pass the state returned by the function above
            return false;
        }

        int64_t nTime3 = GetTimeMicros(); nTimeQuorum += nTime3 - nTime2;
        LogPrint(BCLog::BENCHMARK, "        - quorumBlockProcessor: %.2fms [%.2fs]\n", 0.001 * (nTime3 - nTime2), nTimeQuorum * 0.000001);

        if (!deterministicMNManager || !deterministicMNManager->ProcessBlock(block, pindex, state, view, fJustCheck)) {
            // pass the state returned by the function above
            return false;
        }

        int64_t nTime4 = GetTimeMicros(); nTimeDMN += nTime4 - nTime3;
        LogPrint(BCLog::BENCHMARK, "        - deterministicMNManager: %.2fms [%.2fs]\n", 0.001 * (nTime4 - nTime3), nTimeDMN * 0.000001);

        if (fCheckCbTxMerleRoots && !CheckCbTxMerkleRoots(block, pindex, state, view)) {
            // pass the state returned by the function above
            return false;
        }

        int64_t nTime5 = GetTimeMicros(); nTimeMerkle += nTime5 - nTime4;
        LogPrint(BCLog::BENCHMARK, "        - CheckCbTxMerkleRoots: %.2fms [%.2fs]\n", 0.001 * (nTime5 - nTime4), nTimeMerkle * 0.000001);
    } catch (const std::exception& e) {
        return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "failed-procspectxsinblock");
    }

    return true;
}

bool UndoSpecialTxsInBlock(const CBlock& block, const CBlockIndex* pindex)
{
    try {
        if (!deterministicMNManager || !deterministicMNManager->UndoBlock(block, pindex)) {
            return false;
        }

        if (!llmq::quorumBlockProcessor->UndoBlock(block, pindex)) {
            return false;
        }
    } catch (const std::exception& e) {
        return error(strprintf("%s -- failed: %s\n", __func__, e.what()).c_str());
    }

    return true;
}

uint256 CalcTxInputsHash(const CTransaction& tx)
{
    CHashWriter hw(CLIENT_VERSION, SER_GETHASH);
    for (const auto& in : tx.vin) {
        hw << in.prevout;
    }
    return hw.GetHash();
}
