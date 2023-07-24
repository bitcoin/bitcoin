// Copyright (c) 2023 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <evo/assetlocktx.h>
#include <evo/specialtx.h>
#include <evo/creditpool.h>

#include <consensus/params.h>

#include <chainparams.h>
#include <logging.h>
#include <validation.h>

#include <llmq/commitment.h>
#include <llmq/signing.h>
#include <llmq/utils.h>
#include <llmq/quorums.h>

#include <algorithm>

/**
 *  Common code for Asset Lock and Asset Unlock
 */
bool CheckAssetLockUnlockTx(const CTransaction& tx, const CBlockIndex* pindexPrev, const CCreditPool& creditPool, TxValidationState& state)
{
    switch (tx.nType) {
    case TRANSACTION_ASSET_LOCK:
        return CheckAssetLockTx(tx, state);
    case TRANSACTION_ASSET_UNLOCK:
        return CheckAssetUnlockTx(tx, pindexPrev, creditPool, state);
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

    CAssetLockPayload assetLockTx;
    if (!GetTxPayload(tx, assetLockTx)) {
        return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-assetlocktx-payload");
    }

    if (assetLockTx.getVersion() == 0 || assetLockTx.getVersion() > CAssetLockPayload::CURRENT_VERSION) {
        return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-assetlocktx-version");
    }

    if (assetLockTx.getCreditOutputs().empty()) {
        return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-assetlocktx-emptycreditoutputs");
    }

    CAmount creditOutputsAmount = 0;
    for (const CTxOut& out : assetLockTx.getCreditOutputs()) {
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

bool CAssetUnlockPayload::VerifySig(const uint256& msgHash, const CBlockIndex* pindexTip, TxValidationState& state) const
{
    // That quourm hash must be active at `requestHeight`,
    // and at the quorumHash must be active in either the current or previous quorum cycle
    // and the sig must validate against that specific quorumHash.

    Consensus::LLMQType llmqType = Params().GetConsensus().llmqTypeAssetLocks;

    // We check at most 2 quorums
    const auto quorums = llmq::quorumManager->ScanQuorums(llmqType, pindexTip, 2);
    bool isActive = std::any_of(quorums.begin(), quorums.end(), [&](const auto &q) { return q->qc->quorumHash == quorumHash; });

    if (!isActive) {
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-assetunlock-not-active-quorum");
    }

    if (pindexTip->nHeight < requestedHeight || pindexTip->nHeight >= getHeightToExpiry()) {
        LogPrintf("Asset unlock tx %d with requested height %d could not be accepted on height: %d\n",
                index, requestedHeight, pindexTip->nHeight);
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-assetunlock-too-late");
    }

    const auto quorum = llmq::quorumManager->GetQuorum(llmqType, quorumHash);
    assert(quorum);

    const uint256 requestId = ::SerializeHash(std::make_pair(ASSETUNLOCK_REQUESTID_PREFIX, index));

    const uint256 signHash = llmq::utils::BuildSignHash(llmqType, quorum->qc->quorumHash, requestId, msgHash);
    if (quorumSig.VerifyInsecure(quorum->qc->quorumPublicKey, signHash)) {
        return true;
    }

    return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-assetunlock-not-verified");
}

bool CheckAssetUnlockTx(const CTransaction& tx, const CBlockIndex* pindexPrev, const CCreditPool& creditPool, TxValidationState& state)
{
    if (tx.nType != TRANSACTION_ASSET_UNLOCK) {
        return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-assetunlocktx-type");
    }

    if (!tx.vin.empty()) {
        return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-assetunlocktx-have-input");
    }

    if (tx.vout.size() > CAssetUnlockPayload::MAXIMUM_WITHDRAWALS) {
        return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-assetunlocktx-too-many-outs");
    }

    CAssetUnlockPayload assetUnlockTx;
    if (!GetTxPayload(tx, assetUnlockTx)) {
        return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-assetunlocktx-payload");
    }

    if (assetUnlockTx.getVersion() == 0 || assetUnlockTx.getVersion() > CAssetUnlockPayload::CURRENT_VERSION) {
        return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-assetunlocktx-version");
    }

    if (creditPool.indexes.Contains(assetUnlockTx.getIndex())) {
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-assetunlock-duplicated-index");
    }

    const CBlockIndex* pindexQuorum = WITH_LOCK(cs_main, return g_chainman.m_blockman.LookupBlockIndex(assetUnlockTx.getQuorumHash()));
    if (!pindexQuorum) {
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-assetunlock-quorum-hash");
    }

    // Copy transaction except `quorumSig` field to calculate hash
    CMutableTransaction tx_copy(tx);
    const CAssetUnlockPayload payload_copy{assetUnlockTx.getVersion(), assetUnlockTx.getIndex(), assetUnlockTx.getFee(), assetUnlockTx.getRequestedHeight(), assetUnlockTx.getQuorumHash(), CBLSSignature{}};
    SetTxPayload(tx_copy, payload_copy);

    uint256 msgHash = tx_copy.GetHash();

    return assetUnlockTx.VerifySig(msgHash, pindexPrev, state);
}

bool GetAssetUnlockFee(const CTransaction& tx, CAmount& txfee, TxValidationState& state)
{
    CAssetUnlockPayload assetUnlockTx;
    if (!GetTxPayload(tx, assetUnlockTx)) {
        return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-assetunlocktx-payload");
    }
    const CAmount txfee_aux = assetUnlockTx.getFee();
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
