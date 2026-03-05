// Copyright (c) 2018-2019 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>
#include <consensus/validation.h>
#include <hash.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <validation.h>

#include <algorithm>

#include <evo/deterministicmns.h>
#include <evo/specialtx.h>
#include <util/time.h>
#include <llmq/quorums_commitment.h>
#include <llmq/quorums_blockprocessor.h>
#include <llmq/quorums_chainlocks.h>
#include <llmq/quorums_btccheckpoints.h>
#include <logging.h>
#include <governance/governance.h>

class CCoinsViewCache;

bool ExtractBTCCReceipt(const CBlock& block, llmq::CBTCCheckpointSig& receipt)
{
    if (block.vtx.empty() || !block.vtx[0]) return false;
    std::vector<unsigned char> vchData;
    int nOut{-1};
    if (!GetSyscoinData(*block.vtx[0], vchData, nOut)) return false;

    auto pos = vchData.end();
    for (auto it = vchData.begin();;) {
        it = std::search(it, vchData.end(), std::begin(BTCCHECK_MAGIC_BYTES), std::end(BTCCHECK_MAGIC_BYTES));
        if (it == vchData.end()) break;
        pos = it;
        ++it;
    }
    if (pos == vchData.end()) return false;

    pos = std::next(pos, sizeof(BTCCHECK_MAGIC_BYTES));
    try {
        CDataStream ds(std::vector<unsigned char>(pos, vchData.end()), SER_NETWORK, PROTOCOL_VERSION);
        ds >> receipt;
    } catch (const std::exception&) {
        return false;
    }
    return true;
}

bool ExtractBTCPREVCommitment(const CBlock& block, uint256& btcPrevHash)
{
    if (block.vtx.empty() || !block.vtx[0]) return false;
    std::vector<unsigned char> vchData;
    int nOut{-1};
    if (!GetSyscoinData(*block.vtx[0], vchData, nOut)) return false;

    auto pos = vchData.end();
    for (auto it = vchData.begin();;) {
        it = std::search(it, vchData.end(), std::begin(BTCPREV_MAGIC_BYTES), std::end(BTCPREV_MAGIC_BYTES));
        if (it == vchData.end()) break;
        pos = it;
        ++it;
    }
    if (pos == vchData.end()) return false;

    pos = std::next(pos, sizeof(BTCPREV_MAGIC_BYTES));
    try {
        CDataStream ds(std::vector<unsigned char>(pos, vchData.end()), SER_NETWORK, PROTOCOL_VERSION);
        ds >> btcPrevHash;
    } catch (const std::exception&) {
        return false;
    }
    return true;
}

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
        default:
            return true;
        }
    } catch (const std::exception& e) {
        LogPrintf("%s -- failed: %s\n", __func__, e.what());
        return FormatSyscoinErrorMessage(state, "failed-check-special-tx", fJustCheck);
    }

    return FormatSyscoinErrorMessage(state, "bad-tx-type-check", fJustCheck);
}


bool ProcessSpecialTxsInBlock(ChainstateManager &chainman, const CBlock& block, const CBlockIndex* pindex, BlockValidationState& state, CDeterministicMNListNEVMAddressDiff &diff, CCoinsViewCache& view, bool fJustCheck, bool check_sigs, bool ibd)
{
    try {
        static SteadyClock::duration nTimeLoop{};
        static SteadyClock::duration nTimeQuorum{};

        auto nTime1 = SystemClock::now();
        llmq::CFinalCommitmentTxPayload qcTx;
        for (const auto& ptr_tx : block.vtx) {
            TxValidationState txstate;
            if (!CheckSpecialTx(chainman.m_blockman, *ptr_tx, pindex->pprev, txstate, view, false, check_sigs)) {
                return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, txstate.GetRejectReason());
            }
        }

        auto nTime2 = SystemClock::now(); nTimeLoop += nTime2 - nTime1;
        LogPrint(BCLog::BENCHMARK, "        - Loop: %.2fms [%.2fs]\n",  Ticks<MillisecondsDouble>(nTime2 - nTime1), Ticks<SecondsDouble>(nTimeLoop));

        // SYSCOIN: consensus-validated, lagged BTC checkpoint attestation embedded in the coinbase Syscoin-data payload.
        // Schedule: one checkpoint per 10-block epoch counted from activation height.
        // Carrier at epoch offset +7. If non-null, must verify against the ancestor at height (h-5).
        {
            const auto& consensus = Params().GetConsensus();
            const int32_t height = pindex->nHeight;
            const int start = consensus.nCLReceiptStartBlock;
            const bool receiptRequired = height >= start && ((height % BTCCHECK_PERIOD) == BTCCHECK_CARRIER_OFFSET) && height >= BTCCHECK_PROP_BUFFER;
            if (receiptRequired) {
                std::vector<unsigned char> vchData;
                int nOut{-1};
                if (!GetSyscoinData(*block.vtx[0], vchData, nOut)) {
                    LogPrintf("%s -- bad-btcc-output at height=%d block=%s\n", __func__, height, pindex->GetBlockHash().ToString());
                    return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-btcc-output");
                }

                // SYSCOIN: consensus-validated, lagged BTC checkpoint attestation embedded in the same payload.
                // Required every 10 blocks post activation (null allowed).
                llmq::CBTCCheckpointSig btcc;
                if (!ExtractBTCCReceipt(block, btcc)) {
                    LogPrintf("%s -- bad-btcc-missing at height=%d block=%s data_size=%u\n", __func__, height, pindex->GetBlockHash().ToString(), static_cast<unsigned>(vchData.size()));
                    return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-btcc-missing");
                }

                // If BTCCHECK is non-null, it must match the (h-5) ancestor referenced by this carrier block.
                if (!btcc.IsNull()) {
                    const int32_t expected = height - BTCCHECK_PROP_BUFFER;
                    if (btcc.nHeight != expected) {
                        LogPrintf("%s -- bad-btcc-height at height=%d block=%s btcc_height=%d expected=%d\n",
                                __func__, height, pindex->GetBlockHash().ToString(), btcc.nHeight, expected);
                        return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-btcc-height");
                    }
                    const CBlockIndex* pindexReceipt = pindex->GetAncestor(expected);
                    if (!pindexReceipt) {
                        LogPrintf("%s -- bad-btcc-ancestor at height=%d block=%s expected=%d\n",
                                __func__, height, pindex->GetBlockHash().ToString(), expected);
                        return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-btcc-ancestor");
                    }
                    if (btcc.sysHash != pindexReceipt->GetBlockHash()) {
                        LogPrintf("%s -- bad-btcc-syshash at height=%d block=%s btcc_sys=%s expected_sys=%s\n",
                                __func__, height, pindex->GetBlockHash().ToString(),
                                btcc.sysHash.ToString(), pindexReceipt->GetBlockHash().ToString());
                        return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-btcc-syshash");
                    }
                    if (pindexReceipt->btcpPrevCommitment.IsNull()) {
                        LogPrintf("%s -- bad-btcc-btcp at height=%d block=%s expected_height=%d sys=%s\n",
                                __func__, height, pindex->GetBlockHash().ToString(), expected, btcc.sysHash.ToString());
                        return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-btcc-btcp");
                    }

                    if (check_sigs) {
                        if (!llmq::btcCheckpointsHandler) {
                            LogPrintf("%s -- bad-btcc-nohandler at height=%d block=%s\n", __func__, height, pindex->GetBlockHash().ToString());
                            return state.Invalid(BlockValidationResult::BLOCK_CHAINLOCK, "bad-btcc-nohandler");
                        }
                        if (!llmq::btcCheckpointsHandler->VerifyAggregatedBTCCheckpoint(btcc, pindexReceipt)) {
                            LogPrintf("%s -- bad-btcc-sig at height=%d block=%s btcc_height=%d btcc_sys=%s\n",
                                    __func__, height, pindex->GetBlockHash().ToString(), btcc.nHeight, btcc.sysHash.ToString());
                            return state.Invalid(BlockValidationResult::BLOCK_CHAINLOCK, "bad-btcc-sig");
                        }
                    }
                }
            }
        }

        if (!llmq::quorumBlockProcessor->ProcessBlock(block, pindex, state, qcTx, fJustCheck, check_sigs)) {
            // pass the state returned by the function above
            return false;
        }

        auto nTime3 = SystemClock::now(); nTimeQuorum += nTime3 - nTime2;
        LogPrint(BCLog::BENCHMARK, "        - quorumBlockProcessor: %.2fms [%.2fs]\n",  Ticks<MillisecondsDouble>(nTime3 - nTime2), Ticks<SecondsDouble>(nTimeQuorum));

        if (!deterministicMNManager || !deterministicMNManager->ProcessBlock(block, pindex, state, view, qcTx, diff, fJustCheck, ibd)) {
            // pass the state returned by the function above
            return false;
        }
    } catch (const std::exception& e) {
        return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "failed-procspectxsinblock");
    }

    return true;
}

bool UndoSpecialTxsInBlock(const CBlock& block, const CBlockIndex* pindex, CDeterministicMNListNEVMAddressDiff& diffNEVM, bool bReverify, bool bReplay)
{
    try {
        if(bReverify) {
            if (!deterministicMNManager || !deterministicMNManager->UndoBlock(pindex, diffNEVM)) {
                return false;
            }
            if (!llmq::quorumBlockProcessor->UndoBlock(block, pindex)) {
                return false;
            }
        }
        // replay doesn't connect block which writes governance SB to cache again
        if(!bReplay) {
            if (!governance->UndoBlock(pindex)) {
                return false;
            }
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
