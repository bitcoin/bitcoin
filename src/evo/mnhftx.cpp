// Copyright (c) 2021-2024 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/validation.h>
#include <deploymentstatus.h>
#include <evo/mnhftx.h>
#include <evo/specialtx.h>
#include <llmq/commitment.h>
#include <llmq/signing.h>
#include <llmq/quorums.h>
#include <node/blockstorage.h>

#include <chain.h>
#include <chainparams.h>
#include <validation.h>
#include <versionbits.h>

#include <algorithm>
#include <stack>
#include <string>
#include <vector>

static const std::string MNEHF_REQUESTID_PREFIX = "mnhf";
static const std::string DB_SIGNALS = "mnhf_s";

uint256 MNHFTxPayload::GetRequestId() const
{
    return ::SerializeHash(std::make_pair(MNEHF_REQUESTID_PREFIX, int64_t{signal.versionBit}));
}

CMutableTransaction MNHFTxPayload::PrepareTx() const
{
    CMutableTransaction tx;
    tx.nVersion = 3;
    tx.nType = SPECIALTX_TYPE;
    SetTxPayload(tx, *this);

    return tx;
}

CMNHFManager::CMNHFManager(CEvoDB& evoDb) :
    m_evoDb(evoDb)
{
    assert(globalInstance == nullptr);
    globalInstance = this;
}

CMNHFManager::~CMNHFManager()
{
    assert(globalInstance != nullptr);
    globalInstance = nullptr;
}

CMNHFManager::Signals CMNHFManager::GetSignalsStage(const CBlockIndex* const pindexPrev)
{
    Signals signals = GetForBlock(pindexPrev);
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

bool MNHFTx::Verify(const uint256& quorumHash, const uint256& requestId, const uint256& msgHash, TxValidationState& state) const
{
    if (versionBit >= VERSIONBITS_NUM_BITS) {
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-mnhf-nbit-out-of-bounds");
    }

    const Consensus::LLMQType& llmqType = Params().GetConsensus().llmqTypeMnhf;
    const auto quorum = llmq::quorumManager->GetQuorum(llmqType, quorumHash);

    const uint256 signHash = llmq::BuildSignHash(llmqType, quorum->qc->quorumHash, requestId, msgHash);
    if (!sig.VerifyInsecure(quorum->qc->quorumPublicKey, signHash)) {
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-mnhf-invalid");
    }

    return true;
}

bool CheckMNHFTx(const CTransaction& tx, const CBlockIndex* pindexPrev, TxValidationState& state)
{
    if (tx.nVersion != 3 || tx.nType != TRANSACTION_MNHF_SIGNAL) {
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-mnhf-type");
    }

    const auto opt_mnhfTx = GetTxPayload<MNHFTxPayload>(tx);
    if (!opt_mnhfTx) {
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-mnhf-payload");
    }
    auto& mnhfTx = *opt_mnhfTx;
    if (mnhfTx.nVersion == 0 || mnhfTx.nVersion > MNHFTxPayload::CURRENT_VERSION) {
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-mnhf-version");
    }

    const CBlockIndex* pindexQuorum = WITH_LOCK(::cs_main, return g_chainman.m_blockman.LookupBlockIndex(mnhfTx.signal.quorumHash));
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


    if (!mnhfTx.signal.Verify(mnhfTx.signal.quorumHash, mnhfTx.GetRequestId(), msgHash, state)) {
        // set up inside Verify
        return false;
    }

    if (!Params().IsValidMNActivation(mnhfTx.signal.versionBit, pindexPrev->GetMedianTimePast())) {
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

    const auto opt_mnhfTx = GetTxPayload<MNHFTxPayload>(tx);
    if (!opt_mnhfTx) {
        return std::nullopt;
    }
    return opt_mnhfTx->signal.versionBit;
}

static bool extractSignals(const CBlock& block, const CBlockIndex* const pindex, std::vector<uint8_t>& new_signals, BlockValidationState& state)
{
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

        const auto opt_mnhfTx = GetTxPayload<MNHFTxPayload>(tx);
        if (!opt_mnhfTx) {
            return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-mnhf-tx-payload");
        }
        const uint8_t bit = opt_mnhfTx->signal.versionBit;
        if (std::find(new_signals.begin(), new_signals.end(), bit) != new_signals.end()) {
            return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-mnhf-duplicates-in-block");
        }
        new_signals.push_back(bit);
    }

    return true;
}

std::optional<CMNHFManager::Signals> CMNHFManager::ProcessBlock(const CBlock& block, const CBlockIndex* const pindex, bool fJustCheck, BlockValidationState& state)
{
    try {
        std::vector<uint8_t> new_signals;
        if (!extractSignals(block, pindex, new_signals, state)) {
            // state is set inside extractSignals
            return std::nullopt;
        }
        Signals signals = GetSignalsStage(pindex->pprev);
        if (new_signals.empty()) {
            if (!fJustCheck) {
                AddToCache(signals, pindex);
            }
            LogPrint(BCLog::EHF, "CMNHFManager::ProcessBlock: no new signals; number of known signals: %d\n", signals.size());
            return signals;
        }

        const int mined_height = pindex->nHeight;

        // Extra validation of signals to be sure that it can succeed
        for (const auto& versionBit : new_signals) {
            LogPrintf("CMNHFManager::ProcessBlock: add mnhf bit=%d block:%s number of known signals:%lld\n", versionBit, pindex->GetBlockHash().ToString(), signals.size());
            if (signals.find(versionBit) != signals.end()) {
                state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-mnhf-duplicate");
                return std::nullopt;
            }

            if (!Params().IsValidMNActivation(versionBit, pindex->GetMedianTimePast())) {
                state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-mnhf-non-mn-fork");
                return std::nullopt;
            }
        }
        if (fJustCheck) {
            // We are done, no need actually update any params
            return signals;
        }
        for (const auto& versionBit : new_signals) {
            if (Params().IsValidMNActivation(versionBit, pindex->GetMedianTimePast())) {
                signals.insert({versionBit, mined_height});
            }

        }

        AddToCache(signals, pindex);
        return signals;
    } catch (const std::exception& e) {
        LogPrintf("CMNHFManager::ProcessBlock -- failed: %s\n", e.what());
        state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "failed-proc-mnhf-inblock");
        return std::nullopt;
    }
}

bool CMNHFManager::UndoBlock(const CBlock& block, const CBlockIndex* const pindex)
{
    std::vector<uint8_t> excluded_signals;
    BlockValidationState state;
    if (!extractSignals(block, pindex, excluded_signals, state)) {
        LogPrintf("CMNHFManager::%s: failed to extract signals\n", __func__);
        return false;
    }
    if (excluded_signals.empty()) {
        return true;
    }

    const Signals signals = GetForBlock(pindex);
    for (const auto& versionBit : excluded_signals) {
        LogPrintf("%s: exclude mnhf bit=%d block:%s number of known signals:%lld\n", __func__, versionBit, pindex->GetBlockHash().ToString(), signals.size());
        assert(signals.find(versionBit) != signals.end());
        assert(Params().IsValidMNActivation(versionBit, pindex->GetMedianTimePast()));
    }

    return true;
}

CMNHFManager::Signals CMNHFManager::GetForBlock(const CBlockIndex* pindex)
{
    if (pindex == nullptr) return {};

    std::stack<const CBlockIndex *> to_calculate;

    std::optional<CMNHFManager::Signals> signalsTmp;
    while (!(signalsTmp = GetFromCache(pindex)).has_value()) {
        to_calculate.push(pindex);
        pindex = pindex->pprev;
    }

    const Consensus::Params& consensusParams{Params().GetConsensus()};
    while (!to_calculate.empty()) {
        const CBlockIndex* pindex_top{to_calculate.top()};
        CBlock block;
        if (!ReadBlockFromDisk(block, pindex_top, consensusParams)) {
            throw std::runtime_error("failed-getehfforblock-read");
        }
        BlockValidationState state;
        signalsTmp = ProcessBlock(block, pindex_top, false, state);
        if (!signalsTmp.has_value()) {
            LogPrintf("%s: process block failed due to %s\n", __func__, state.ToString());
            throw std::runtime_error("failed-getehfforblock-construct");
        }

        to_calculate.pop();
    }
    return *signalsTmp;
}

std::optional<CMNHFManager::Signals> CMNHFManager::GetFromCache(const CBlockIndex* const pindex)
{
    Signals signals{};
    if (pindex == nullptr) return signals;

    // TODO: remove this check of phashBlock to nullptr
    // This check is needed only because unit test 'versionbits_tests.cpp'
    // lets `phashBlock` to be nullptr
    if (pindex->phashBlock == nullptr) return signals;


    const uint256& blockHash = pindex->GetBlockHash();
    {
        LOCK(cs_cache);
        if (mnhfCache.get(blockHash, signals)) {
            return signals;
        }
    }
    {
        LOCK(cs_cache);
        if (ThresholdState::ACTIVE != v20_activation.State(pindex->pprev, Params().GetConsensus(), Consensus::DEPLOYMENT_V20)) {
            mnhfCache.insert(blockHash, signals);
            return signals;
        }
    }
    if (m_evoDb.Read(std::make_pair(DB_SIGNALS, blockHash), signals)) {
        LOCK(cs_cache);
        mnhfCache.insert(blockHash, signals);
        return signals;
    }
    return std::nullopt;
}

void CMNHFManager::AddToCache(const Signals& signals, const CBlockIndex* const pindex)
{
    assert(pindex != nullptr);
    const uint256& blockHash = pindex->GetBlockHash();
    {
        LOCK(cs_cache);
        mnhfCache.insert(blockHash, signals);
    }
    {
        LOCK(cs_cache);
        if (ThresholdState::ACTIVE != v20_activation.State(pindex->pprev, Params().GetConsensus(), Consensus::DEPLOYMENT_V20)) return;
    }
    m_evoDb.Write(std::make_pair(DB_SIGNALS, blockHash), signals);
}

void CMNHFManager::AddSignal(const CBlockIndex* const pindex, int bit)
{
    auto signals = GetForBlock(pindex->pprev);
    signals.emplace(bit, pindex->nHeight);
    AddToCache(signals, pindex);
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
