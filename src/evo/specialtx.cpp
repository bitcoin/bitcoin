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
#include <util/time.h>
#include <llmq/quorums_commitment.h>
#include <llmq/quorums_blockprocessor.h>
#include <logging.h>
class CCoinsViewCache;
bool CheckSpecialTx(node::BlockManager &blockman, const CTransaction& tx, const CBlockIndex* pindexPrev, TxValidationState& state, CCoinsViewCache& view, bool fJustCheck, bool check_sigs)
{

    try {
        switch (tx.nVersion) {
        case SYSCOIN_TX_VERSION_MN_REGISTER:
            return CheckProRegTx(tx, pindexPrev, state, view, fJustCheck, check_sigs);
        case SYSCOIN_TX_VERSION_MN_UPDATE_SERVICE:
            return CheckProUpServTx(tx, pindexPrev, state, fJustCheck, check_sigs);
        case SYSCOIN_TX_VERSION_MN_UPDATE_REGISTRAR:
            return CheckProUpRegTx(tx, pindexPrev, state, view, fJustCheck, check_sigs);
        case SYSCOIN_TX_VERSION_MN_UPDATE_REVOKE:
            return CheckProUpRevTx(tx, pindexPrev, state, fJustCheck, check_sigs);
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

bool ProcessSpecialTxsInBlock(node::BlockManager &blockman, const CBlock& block, const CBlockIndex* pindex, BlockValidationState& state, CCoinsViewCache& view, bool fJustCheck, bool check_sigs, bool ibd)
{
    try {
        static SteadyClock::duration nTimeLoop{};
        static SteadyClock::duration nTimeQuorum{};
        static SteadyClock::duration nTimeDMN{};

        auto nTime1 = SystemClock::now();

        for (const auto& ptr_tx : block.vtx) {
            TxValidationState txstate;
            if (!CheckSpecialTx(blockman, *ptr_tx, pindex->pprev, txstate, view, false, check_sigs)) {
                return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, txstate.GetRejectReason());
            }
        }

        auto nTime2 = SystemClock::now(); nTimeLoop += nTime2 - nTime1;
        LogPrint(BCLog::BENCHMARK, "        - Loop: %.2fms [%.2fs]\n",  Ticks<MillisecondsDouble>(nTime2 - nTime1), Ticks<SecondsDouble>(nTimeLoop));

        if (!llmq::quorumBlockProcessor->ProcessBlock(block, pindex, state, fJustCheck, check_sigs)) {
            // pass the state returned by the function above
            return false;
        }

        auto nTime3 = SystemClock::now(); nTimeQuorum += nTime3 - nTime2;
        LogPrint(BCLog::BENCHMARK, "        - quorumBlockProcessor: %.2fms [%.2fs]\n",  Ticks<MillisecondsDouble>(nTime3 - nTime2), Ticks<SecondsDouble>(nTimeQuorum));

        if (!deterministicMNManager || !deterministicMNManager->ProcessBlock(block, pindex, state, view, fJustCheck, ibd)) {
            // pass the state returned by the function above
            return false;
        }

        auto nTime4 = SystemClock::now(); nTimeDMN += nTime4 - nTime3;
        LogPrint(BCLog::BENCHMARK, "        - deterministicMNManager: %.2fms [%.2fs]\n",  Ticks<MillisecondsDouble>(nTime4 - nTime3), Ticks<SecondsDouble>(nTimeDMN));

    } catch (const std::exception& e) {
        return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "failed-procspectxsinblock");
    }

    return true;
}

bool UndoSpecialTxsInBlock(const CBlock& block, const CBlockIndex* pindex)
{
    try {
        if (!deterministicMNManager || !deterministicMNManager->UndoBlock(pindex)) {
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
    CHashWriter hw(SER_GETHASH, CLIENT_VERSION);
    for (const auto& in : tx.vin) {
        hw << in.prevout;
    }
    return hw.GetHash();
}
