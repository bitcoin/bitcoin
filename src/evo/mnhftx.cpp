// Copyright (c) 2021-2022 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/validation.h>
#include <evo/mnhftx.h>
#include <evo/specialtx.h>
#include <llmq/commitment.h>
#include <llmq/signing.h>
#include <llmq/utils.h>
#include <llmq/quorums.h>

#include <chain.h>
#include <chainparams.h>
#include <validation.h>
#include <versionbits.h>

#include <algorithm>
#include <string>
#include <vector>

extern const std::string MNEHF_REQUESTID_PREFIX = "mnhf";
static const std::string DB_SIGNALS = "mnhf_s";

CMNHFManager::Signals CMNHFManager::GetSignalsStage(const CBlockIndex* const pindexPrev)
{
    Signals signals = GetFromCache(pindexPrev);
    const int height = pindexPrev->nHeight + 1;
    for (auto it = signals.begin(); it != signals.end(); ) {
        bool found{false};
        const auto signal_pindex = pindexPrev->GetAncestor(it->second);
        assert(signal_pindex != nullptr);
        const int64_t signal_time = signal_pindex->GetMedianTimePast();
        for (int index = 0; index < Consensus::MAX_VERSION_BITS_DEPLOYMENTS; ++index) {
            const auto& deployment = Params().GetConsensus().vDeployments[index];
            if (deployment.bit != it->first) continue;
            if (signal_time < deployment.nStartTime) {
                // new deployment is using the same bit as the old one
                LogPrintf("CMNHFManager::GetSignalsStage: mnhf signal bit=%d height:%d is expired at height=%d\n", it->first, it->second, height);
                it = signals.erase(it);
            } else {
                ++it;
            }
            found = true;
            break;
        }
        if (!found) {
            // no deployment means we buried it and aren't using the same bit (yet)
            LogPrintf("CMNHFManager::GetSignalsStage: mnhf signal bit=%d height:%d is not known at height=%d\n", it->first, it->second, height);
            it = signals.erase(it);
        }
    }
    return signals;
}

bool MNHFTx::Verify(const uint256& quorumHash, const uint256& msgHash, TxValidationState& state) const
{
    if (versionBit >= VERSIONBITS_NUM_BITS) {
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-mnhf-nbit-out-of-bounds");
    }

    const Consensus::LLMQType& llmqType = Params().GetConsensus().llmqTypeMnhf;
    const auto quorum = llmq::quorumManager->GetQuorum(llmqType, quorumHash);

    const uint256 requestId = ::SerializeHash(std::make_pair(MNEHF_REQUESTID_PREFIX, int64_t{versionBit}));
    const uint256 signHash = llmq::utils::BuildSignHash(llmqType, quorum->qc->quorumHash, requestId, msgHash);
    if (!sig.VerifyInsecure(quorum->qc->quorumPublicKey, signHash)) {
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-mnhf-invalid");
    }

    return true;
}

bool CheckMNHFTx(const CTransaction& tx, const CBlockIndex* pindexPrev, TxValidationState& state) EXCLUSIVE_LOCKS_REQUIRED(cs_main)
{
    if (tx.nVersion != 3 || tx.nType != TRANSACTION_MNHF_SIGNAL) {
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-mnhf-type");
    }

    MNHFTxPayload mnhfTx;
    if (!GetTxPayload(tx, mnhfTx)) {
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-mnhf-payload");
    }

    if (mnhfTx.nVersion == 0 || mnhfTx.nVersion > MNHFTxPayload::CURRENT_VERSION) {
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-mnhf-version");
    }

    const CBlockIndex* pindexQuorum = g_chainman.m_blockman.LookupBlockIndex(mnhfTx.signal.quorumHash);
    if (!pindexQuorum) {
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-mnhf-quorum-hash");
    }

    if (pindexQuorum != pindexPrev->GetAncestor(pindexQuorum->nHeight)) {
        // not part of active chain
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-mnhf-quorum-hash");
    }

    // Copy transaction except `quorumSig` field to calculate hash
    CMutableTransaction tx_copy(tx);
    auto payload_copy = mnhfTx;
    payload_copy.signal.sig = CBLSSignature();
    SetTxPayload(tx_copy, payload_copy);
    uint256 msgHash = tx_copy.GetHash();


    if (!mnhfTx.signal.Verify(mnhfTx.signal.quorumHash, msgHash, state)) {
        // set up inside Verify
        return false;
    }

    if (!Params().UpdateMNActivationParam(mnhfTx.signal.versionBit, pindexPrev->nHeight, pindexPrev->GetMedianTimePast(), /* fJustCheck= */ true )) {
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-mnhf-non-ehf");
    }

    return true;
}

std::optional<uint8_t> extractEHFSignal(const CTransaction& tx)
{
    if (tx.nVersion != 3 || tx.nType != TRANSACTION_MNHF_SIGNAL) {
        // only interested in special TXs 'TRANSACTION_MNHF_SIGNAL'
        return std::nullopt;
    }

    MNHFTxPayload mnhfTx;
    if (!GetTxPayload(tx, mnhfTx)) {
        return std::nullopt;
    }
    return mnhfTx.signal.versionBit;
}

static bool extractSignals(const CBlock& block, const CBlockIndex* const pindex, std::vector<uint8_t>& new_signals, BlockValidationState& state)
{
    AssertLockHeld(cs_main);

    // we skip the coinbase
    for (size_t i = 1; i < block.vtx.size(); ++i) {
        const CTransaction& tx = *block.vtx[i];

        if (tx.nVersion != 3 || tx.nType != TRANSACTION_MNHF_SIGNAL) {
            // only interested in special TXs 'TRANSACTION_MNHF_SIGNAL'
            continue;
        }

        TxValidationState tx_state;
        if (!CheckMNHFTx(tx, pindex, tx_state)) {
            return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, tx_state.GetRejectReason(), tx_state.GetDebugMessage());
        }

        MNHFTxPayload mnhfTx;
        if (!GetTxPayload(tx, mnhfTx)) {
            return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-mnhf-tx-payload");
        }
        const uint8_t bit = mnhfTx.signal.versionBit;
        if (std::find(new_signals.begin(), new_signals.end(), bit) != new_signals.end()) {
            return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-mnhf-duplicates-in-block");
        }
        new_signals.push_back(bit);
    }

    return true;
}

bool CMNHFManager::ProcessBlock(const CBlock& block, const CBlockIndex* const pindex, bool fJustCheck, BlockValidationState& state)
{
    try {
        std::vector<uint8_t> new_signals;
        if (!extractSignals(block, pindex, new_signals, state)) {
            // state is set inside extractSignals
            return false;
        }
        Signals signals = GetSignalsStage(pindex->pprev);
        if (new_signals.empty()) {
            if (!fJustCheck) {
                AddToCache(signals, pindex);
            }
            return true;
        }

        int mined_height = pindex->nHeight;

        // Extra validation of signals to be sure that it can succeed
        for (const auto& versionBit : new_signals) {
            LogPrintf("CMNHFManager::ProcessBlock: add mnhf bit=%d block:%s number of known signals:%lld\n", versionBit, pindex->GetBlockHash().ToString(), signals.size());
            if (signals.find(versionBit) != signals.end()) {
                return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-mnhf-duplicate");
            }

            if (!Params().UpdateMNActivationParam(versionBit, mined_height, pindex->GetMedianTimePast(), true /* fJustCheck */)) {
                return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-mnhf-non-mn-fork");
            }
        }
        if (fJustCheck) {
            // We are done, no need actually update any params
            return true;
        }
        for (const auto& versionBit : new_signals) {
            signals.insert({versionBit, mined_height});

            if (!Params().UpdateMNActivationParam(versionBit, mined_height, pindex->GetMedianTimePast(), false /* fJustCheck */)) {
                // it should not ever fail - all checks are done above
                assert(false);
            }

        }

        AddToCache(signals, pindex);
        return true;
    } catch (const std::exception& e) {
        LogPrintf("CMNHFManager::ProcessBlock -- failed: %s\n", e.what());
        return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "failed-proc-mnhf-inblock");
    }
}

bool CMNHFManager::UndoBlock(const CBlock& block, const CBlockIndex* const pindex)
{
    std::vector<uint8_t> excluded_signals;
    BlockValidationState state;
    if (!extractSignals(block, pindex, excluded_signals, state)) {
        LogPrintf("%s: failed to extract signals\n", __func__);
        return false;
    }
    if (excluded_signals.empty()) {
        return true;
    }

    const Signals signals = GetFromCache(pindex);
    for (const auto& versionBit : excluded_signals) {
        LogPrintf("%s: exclude mnhf bit=%d block:%s number of known signals:%lld\n", __func__, versionBit, pindex->GetBlockHash().ToString(), signals.size());
        assert(signals.find(versionBit) != signals.end());

        bool update_ret = Params().UpdateMNActivationParam(versionBit, 0, pindex->GetMedianTimePast(), false /* fJustCheck */);
        assert(update_ret);
    }

    return true;
}

void CMNHFManager::UpdateChainParams(const CBlockIndex* const pindex, const CBlockIndex* const pindexOld)
{
    LogPrintf("%s: update chain params %s -> %s\n", __func__, pindexOld ? pindexOld->GetBlockHash().ToString() : "", pindex ? pindex->GetBlockHash().ToString() : "");
    Signals signals_old{GetFromCache(pindexOld)};
    for (const auto& signal: signals_old) {
        const uint8_t versionBit = signal.first;

        LogPrintf("%s: unload mnhf bit=%d block:%s number of known signals:%lld\n", __func__, versionBit, pindex->GetBlockHash().ToString(), signals_old.size());

        bool update_ret = Params().UpdateMNActivationParam(versionBit, 0, pindex->GetMedianTimePast(), /* fJustCheck= */ false);
        assert(update_ret);
    }

    Signals signals{GetFromCache(pindex)};
    for (const auto& signal: signals) {
        const uint8_t versionBit = signal.first;
        const int value = signal.second;

        LogPrintf("%s: load mnhf bit=%d block:%s number of known signals:%lld\n", __func__, versionBit, pindex->GetBlockHash().ToString(), signals.size());

        bool update_ret = Params().UpdateMNActivationParam(versionBit, value, pindex->GetMedianTimePast(), /* fJustCheck= */ false);
        assert(update_ret);
    }
}

CMNHFManager::Signals CMNHFManager::GetFromCache(const CBlockIndex* const pindex)
{
    if (pindex == nullptr) return {};
    const uint256& blockHash = pindex->GetBlockHash();
    Signals signals{};
    {
        LOCK(cs_cache);
        if (mnhfCache.get(blockHash, signals)) {
            LogPrintf("CMNHFManager::GetFromCache: mnhf get for block %s from cache: %lld signals\n", pindex->GetBlockHash().ToString(), signals.size());
            return signals;
        }
    }
    if (VersionBitsState(pindex->pprev, Params().GetConsensus(), Consensus::DEPLOYMENT_V20, versionbitscache) != ThresholdState::ACTIVE) {
        LOCK(cs_cache);
        mnhfCache.insert(blockHash, {});
        LogPrintf("CMNHFManager::GetFromCache: mnhf feature is disabled: return empty for block %s\n", pindex->GetBlockHash().ToString());
        return {};
    }
    if (!m_evoDb.Read(std::make_pair(DB_SIGNALS, blockHash), signals)) {
        LogPrintf("CMNHFManager::GetFromCache: failure: can't read MnEHF signals from db for %s\n", pindex->GetBlockHash().ToString());
    }
    LogPrintf("CMNHFManager::GetFromCache: mnhf for block %s read from evo: %lld\n", pindex->GetBlockHash().ToString(), signals.size());
    LOCK(cs_cache);
    mnhfCache.insert(blockHash, signals);
    return signals;
}

void CMNHFManager::AddToCache(const Signals& signals, const CBlockIndex* const pindex)
{
    const uint256& blockHash = pindex->GetBlockHash();
    {
        LOCK(cs_cache);
        LogPrintf("CMNHFManager::AddToCache: mnhf for block %s add to cache: %lld\n", pindex->GetBlockHash().ToString(), signals.size());
        mnhfCache.insert(blockHash, signals);
    }
    m_evoDb.Write(std::make_pair(DB_SIGNALS, blockHash), signals);
}

std::string MNHFTx::ToString() const
{
    return strprintf("MNHFTx(versionBit=%d, quorumHash=%s, sig=%s)",
                     versionBit, quorumHash.ToString(), sig.ToString());
}
std::string MNHFTxPayload::ToString() const
{
    return strprintf("MNHFTxPayload(nVersion=%d, signal=%s)",
                     nVersion, signal.ToString());
}
