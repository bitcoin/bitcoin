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
#include <iterator>
#include <limits>

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

namespace {
size_t CompactSizeLen(uint64_t n)
{
    if (n < 253) return 1;
    if (n <= std::numeric_limits<uint16_t>::max()) return 3;
    if (n <= std::numeric_limits<uint32_t>::max()) return 5;
    return 9;
}

size_t MaxSerializedBTCCReceiptSize()
{
    const auto& llmq_params = Params().GetConsensus().llmqTypeChainLocks;
    const size_t max_signers = static_cast<size_t>(llmq_params.signingActiveQuorumCount);
    return sizeof(int32_t) + uint256::size() + CBLSSignature::SerSize +
           CompactSizeLen(max_signers) + ((max_signers + 7) / 8);
}

template <typename T, typename ParseFn>
bool ExtractUniqueTaggedTailObject(const std::vector<unsigned char>& vchData,
                                   const uint8_t (&magic)[4],
                                   size_t max_payload_size,
                                   T& out,
                                   ParseFn&& parse_fn)
{
    if (vchData.size() < sizeof(magic)) {
        return false;
    }

    const size_t search_start_offset = vchData.size() > sizeof(magic) + max_payload_size ?
                                       vchData.size() - sizeof(magic) - max_payload_size :
                                       0;
    const auto search_begin = vchData.begin() + search_start_offset;
    auto search_end = vchData.end();
    bool found{false};
    T parsed{};
    while (search_begin != search_end) {
        const auto it = std::find_end(search_begin, search_end, std::begin(magic), std::end(magic));
        if (it == search_end) break;
        const auto payload_begin = std::next(it, sizeof(magic));
        const size_t payload_size = std::distance(payload_begin, vchData.end());
        if (payload_size > max_payload_size) return false;

        const Span<const unsigned char> payload{vchData.data() + std::distance(vchData.begin(), payload_begin), payload_size};
        T candidate{};
        if (parse_fn(payload, candidate)) {
            // Multiple decodable tails are ambiguous and thus invalid.
            if (found) return false;
            parsed = std::move(candidate);
            found = true;
        }
        search_end = it;
    }
    if (!found) return false;
    out = std::move(parsed);
    return true;
}

} // namespace

bool ExtractBTCCReceipt(const std::vector<unsigned char>& vchData, llmq::CBTCCheckpointSig& receipt)
{
    return ExtractUniqueTaggedTailObject(vchData, BTCCHECK_MAGIC_BYTES,
                                         MaxSerializedBTCCReceiptSize(), receipt,
                                         [](Span<const unsigned char> payload, llmq::CBTCCheckpointSig& candidate) {
                                             try {
                                                 SpanReader ds(SER_NETWORK, PROTOCOL_VERSION, payload);
                                                 ds >> candidate;
                                                 return ds.empty();
                                             } catch (const std::exception&) {
                                                 return false;
                                             }
                                         });
}

bool ExtractBTCCReceipt(const CBlock& block, llmq::CBTCCheckpointSig& receipt)
{
    if (block.vtx.empty() || !block.vtx[0]) return false;
    std::vector<unsigned char> vchData;
    int nOut{-1};
    if (!GetSyscoinData(*block.vtx[0], vchData, nOut)) return false;
    return ExtractBTCCReceipt(vchData, receipt);
}

bool ExtractBTCPREVCommitment(const CBlock& block, uint256& btcPrevHash)
{
    if (block.vtx.empty() || !block.vtx[0]) return false;
    std::vector<unsigned char> vchData;
    int nOut{-1};
    if (!GetSyscoinData(*block.vtx[0], vchData, nOut)) return false;
    constexpr size_t BTCPREV_PAYLOAD_SIZE{32};
    return ExtractUniqueTaggedTailObject(vchData, BTCPREV_MAGIC_BYTES,
                                         BTCPREV_PAYLOAD_SIZE, btcPrevHash,
                                         [](Span<const unsigned char> payload, uint256& candidate) {
                                             if (payload.size() != BTCPREV_PAYLOAD_SIZE) return false;
                                             try {
                                                 SpanReader ds(SER_NETWORK, PROTOCOL_VERSION, payload);
                                                 ds >> candidate;
                                                 return ds.empty();
                                             } catch (const std::exception&) {
                                                 return false;
                                             }
                                         });
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
        // Schedule: one checkpoint per 10-block absolute epoch (+2 sign, +7 carrier).
        // Carrier is required only when its referenced sign height (h-5) is post-activation.
        {
            const auto& consensus = Params().GetConsensus();
            const int32_t height = pindex->nHeight;
            const int start = consensus.nCLReceiptStartBlock;
            const int32_t expected = height - BTCCHECK_PROP_BUFFER;
            const bool receiptRequired = height >= BTCCHECK_PROP_BUFFER &&
                                        expected >= start &&
                                        ((height % BTCCHECK_PERIOD) == BTCCHECK_CARRIER_OFFSET);
            if (receiptRequired) {
                std::vector<unsigned char> vchData;
                int nOut{-1};
                if (!GetSyscoinData(*block.vtx[0], vchData, nOut)) {
                    LogPrintf("%s -- bad-btcc-output at height=%d block=%s\n", __func__, height, pindex->GetBlockHash().ToString());
                    return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-btcc-output");
                }

                // SYSCOIN: consensus-validated, lagged BTC checkpoint attestation embedded in the same payload.
                // Required on carrier heights whose referenced sign height is post-activation (null allowed).
                llmq::CBTCCheckpointSig btcc;
                if (!ExtractBTCCReceipt(vchData, btcc)) {
                    LogPrintf("%s -- bad-btcc-missing at height=%d block=%s data_size=%u\n", __func__, height, pindex->GetBlockHash().ToString(), static_cast<unsigned>(vchData.size()));
                    return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-btcc-missing");
                }

                // If BTCCHECK is non-null, it must match the (h-5) ancestor referenced by this carrier block.
                if (!btcc.IsNull()) {
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

bool UndoSpecialTxsInBlock(const CBlock& block, const CBlockIndex* pindex, CDeterministicMNListNEVMAddressDiff& diffNEVM, bool bUpdateSpecialTxState, bool bReplay)
{
    try {
        if(bUpdateSpecialTxState) {
            if (!deterministicMNManager || !deterministicMNManager->UndoBlock(pindex, diffNEVM)) {
                return false;
            }
            if (!llmq::quorumBlockProcessor->UndoBlock(block, pindex)) {
                return false;
            }
            // replay doesn't connect block which writes governance SB to cache again
            if(!bReplay) {
                if (!governance->UndoBlock(pindex)) {
                    return false;
                }
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
