// Copyright (c) 2023-2024 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <evo/assetlocktx.h>
#include <evo/specialtx.h>

#include <llmq/commitment.h>
#include <llmq/quorums.h>

#include <chainparams.h>
#include <consensus/params.h>
#include <consensus/validation.h>
#include <deploymentstatus.h>
#include <logging.h>
#include <node/blockstorage.h>
#include <tinyformat.h>
#include <util/ranges_set.h>

#include <algorithm>

using node::BlockManager;

namespace llmq
{
// forward declaration to avoid circular dependency
uint256 BuildSignHash(Consensus::LLMQType llmqType, const uint256& quorumHash, const uint256& id, const uint256& msgHash);
} // llmq

/**
 *  Common code for Asset Lock and Asset Unlock
 */
bool CheckAssetLockUnlockTx(const BlockManager& blockman, const llmq::CQuorumManager& qman, const CTransaction& tx, gsl::not_null<const CBlockIndex*> pindexPrev, const std::optional<CRangesSet>& indexes, TxValidationState& state)
{
    switch (tx.nType) {
    case TRANSACTION_ASSET_LOCK:
        return CheckAssetLockTx(tx, state);
    case TRANSACTION_ASSET_UNLOCK:
        return CheckAssetUnlockTx(blockman, qman, tx, pindexPrev, indexes, state);
    default:
        return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-not-asset-locks-at-all");
    }
}

/**
 * Asset Lock Transaction
 */
bool CheckAssetLockTx(const CTransaction& tx, TxValidationState& state)
{
    if (tx.nType != TRANSACTION_ASSET_LOCK) {
        return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-assetlocktx-type");
    }

    CAmount returnAmount{0};
    for (const CTxOut& txout : tx.vout) {
        const CScript& script = txout.scriptPubKey;
        if (script.empty() || script[0] != OP_RETURN) continue;

        if (script.size() != 2 || script[1] != 0) return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-assetlocktx-non-empty-return");

        if (txout.nValue == 0 || !MoneyRange(txout.nValue)) return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-assetlocktx-opreturn-outofrange");

        // Should be only one OP_RETURN
        if (returnAmount > 0) return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-assetlocktx-multiple-return");
        returnAmount = txout.nValue;
    }

    if (returnAmount == 0) return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-assetlocktx-no-return");

    const auto opt_assetLockTx = GetTxPayload<CAssetLockPayload>(tx);
    if (!opt_assetLockTx.has_value()) {
        return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-assetlocktx-payload");
    }

    if (opt_assetLockTx->getVersion() == 0 || opt_assetLockTx->getVersion() > CAssetLockPayload::CURRENT_VERSION) {
        return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-assetlocktx-version");
    }

    if (opt_assetLockTx->getCreditOutputs().empty()) {
        return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-assetlocktx-emptycreditoutputs");
    }

    CAmount creditOutputsAmount = 0;
    for (const CTxOut& out : opt_assetLockTx->getCreditOutputs()) {
        if (out.nValue == 0 || !MoneyRange(out.nValue) || !MoneyRange(creditOutputsAmount + out.nValue)) {
            return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-assetlocktx-credit-outofrange");
        }

        creditOutputsAmount += out.nValue;
        if (!out.scriptPubKey.IsPayToPublicKeyHash()) {
            return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-assetlocktx-pubKeyHash");
        }
    }
    if (creditOutputsAmount != returnAmount) {
        return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-assetlocktx-creditamount");
    }

    return true;
}

std::string CAssetLockPayload::ToString() const
{
    std::string outputs{"["};
    for (const CTxOut& tx: creditOutputs) {
        outputs.append(tx.ToString());
        outputs.append(",");
    }
    outputs.back() = ']';
    return strprintf("CAssetLockPayload(nVersion=%d,creditOutputs=%s)", nVersion, outputs.c_str());
}

/**
 * Asset Unlock Transaction (withdrawals)
 */

const std::string ASSETUNLOCK_REQUESTID_PREFIX = "plwdtx";

bool CAssetUnlockPayload::VerifySig(const llmq::CQuorumManager& qman, const uint256& msgHash, gsl::not_null<const CBlockIndex*> pindexTip, TxValidationState& state) const
{
    // That quourm hash must be active at `requestHeight`,
    // and at the quorumHash must be active in either the current or previous quorum cycle
    // and the sig must validate against that specific quorumHash.


    Consensus::LLMQType llmqType = Params().GetConsensus().llmqTypePlatform;

    const auto& llmq_params_opt = Params().GetLLMQ(llmqType);
    assert(llmq_params_opt.has_value());

    // We check all active quorums + 1 the latest inactive
    const int quorums_to_scan = llmq_params_opt->signingActiveQuorumCount + 1;
    const auto quorums = qman.ScanQuorums(llmqType, pindexTip, quorums_to_scan);

    if (bool isActive = std::any_of(quorums.begin(), quorums.end(), [&](const auto &q) { return q->qc->quorumHash == quorumHash; }); !isActive) {
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-assetunlock-too-old-quorum");
    }

    if (static_cast<uint32_t>(pindexTip->nHeight) < requestedHeight || pindexTip->nHeight >= getHeightToExpiry()) {
        LogPrint(BCLog::CREDITPOOL, "Asset unlock tx %d with requested height %d could not be accepted on height: %d\n",
                index, requestedHeight, pindexTip->nHeight);
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-assetunlock-too-late");
    }

    const auto quorum = qman.GetQuorum(llmqType, quorumHash);
    // quorum must be valid at this point. Let's check and throw error just in case
    if (!quorum) {
        LogPrintf("%s: ERROR! No quorum for credit pool found for hash=%s\n", __func__, quorumHash.ToString());
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-assetunlock-quorum-internal-error");
    }

    const uint256 requestId = ::SerializeHash(std::make_pair(ASSETUNLOCK_REQUESTID_PREFIX, index));

    if (const uint256 signHash = llmq::BuildSignHash(llmqType, quorum->qc->quorumHash, requestId, msgHash);
            quorumSig.VerifyInsecure(quorum->qc->quorumPublicKey, signHash)) {
        return true;
    }

    return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-assetunlock-not-verified");
}

bool CheckAssetUnlockTx(const BlockManager& blockman, const llmq::CQuorumManager& qman, const CTransaction& tx, gsl::not_null<const CBlockIndex*> pindexPrev, const std::optional<CRangesSet>& indexes, TxValidationState& state)
{
    // Some checks depends from blockchain status also, such as `known indexes` and `withdrawal limits`
    // They are omitted here and done by CCreditPool
    if (tx.nType != TRANSACTION_ASSET_UNLOCK) {
        return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-assetunlocktx-type");
    }

    if (!tx.vin.empty()) {
        return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-assetunlocktx-have-input");
    }

    if (tx.vout.size() > CAssetUnlockPayload::MAXIMUM_WITHDRAWALS) {
        return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-assetunlocktx-too-many-outs");
    }

    const auto opt_assetUnlockTx = GetTxPayload<CAssetUnlockPayload>(tx);
    if (!opt_assetUnlockTx) {
        return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-assetunlocktx-payload");
    }
    auto& assetUnlockTx = *opt_assetUnlockTx;

    if (assetUnlockTx.getVersion() == 0 || assetUnlockTx.getVersion() > CAssetUnlockPayload::CURRENT_VERSION) {
        return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-assetunlocktx-version");
    }

    if (indexes != std::nullopt && indexes->Contains(assetUnlockTx.getIndex())) {
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-assetunlock-duplicated-index");
    }

    if (LOCK(::cs_main); blockman.LookupBlockIndex(assetUnlockTx.getQuorumHash()) == nullptr) {
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-assetunlock-quorum-hash");
    }

    // Copy transaction except `quorumSig` field to calculate hash
    CMutableTransaction tx_copy(tx);
    const CAssetUnlockPayload payload_copy{assetUnlockTx.getVersion(), assetUnlockTx.getIndex(), assetUnlockTx.getFee(), assetUnlockTx.getRequestedHeight(), assetUnlockTx.getQuorumHash(), CBLSSignature{}};
    SetTxPayload(tx_copy, payload_copy);

    uint256 msgHash = tx_copy.GetHash();

    return assetUnlockTx.VerifySig(qman, msgHash, pindexPrev, state);
}

bool GetAssetUnlockFee(const CTransaction& tx, CAmount& txfee, TxValidationState& state)
{
    const auto opt_assetUnlockTx = GetTxPayload<CAssetUnlockPayload>(tx);
    if (!opt_assetUnlockTx) {
        return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-assetunlocktx-payload");
    }
    const CAmount txfee_aux = opt_assetUnlockTx->getFee();
    if (txfee_aux == 0 || !MoneyRange(txfee_aux)) {
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-txns-assetunlock-fee-outofrange");
    }
    txfee = txfee_aux;
    return true;
}

std::string CAssetUnlockPayload::ToString() const
{
    return strprintf("CAssetUnlockPayload(nVersion=%d,index=%d,fee=%d.%08d,requestedHeight=%d,quorumHash=%d,quorumSig=%s",
            nVersion, index, fee / COIN, fee % COIN, requestedHeight, quorumHash.GetHex(), quorumSig.ToString().c_str());
}
