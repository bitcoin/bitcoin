// Copyright (c) 2017-2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <version.h>

#include <consensus/consensus.h>
#include <consensus/tx_check.h>

#include <primitives/transaction.h>
#include <consensus/validation.h>

bool CheckTransaction(const CTransaction& tx, TxValidationState& state)
{
    bool allowEmptyTxIn = false;
    bool allowEmptyTxOut = false;
    if (tx.nType == TRANSACTION_QUORUM_COMMITMENT || tx.nType == TRANSACTION_MNHF_SIGNAL) {
        allowEmptyTxIn = true;
        allowEmptyTxOut = true;
    }
    if (tx.nType == TRANSACTION_ASSET_UNLOCK) {
        allowEmptyTxIn = true;
    }

    // Basic checks that don't depend on any context
    if (!allowEmptyTxIn && tx.vin.empty())
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-txns-vin-empty");
    if (!allowEmptyTxOut && tx.vout.empty())
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-txns-vout-empty");
    // Size limits
    if (::GetSerializeSize(tx, PROTOCOL_VERSION) > MAX_LEGACY_BLOCK_SIZE)
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-txns-oversize");
    if (tx.vExtraPayload.size() > MAX_TX_EXTRA_PAYLOAD)
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-txns-payload-oversize");

    // Check for negative or overflow output values (see CVE-2010-5139)
    CAmount nValueOut = 0;
    for (const auto& txout : tx.vout) {
        if (txout.nValue < 0)
            return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-txns-vout-negative");
        if (txout.nValue > MAX_MONEY)
            return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-txns-vout-toolarge");
        nValueOut += txout.nValue;
        if (!MoneyRange(nValueOut))
            return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-txns-txouttotal-toolarge");
    }

    // Check for duplicate inputs (see CVE-2018-17144)
    // While Consensus::CheckTxInputs does check if all inputs of a tx are available, and UpdateCoins marks all inputs
    // of a tx as spent, it does not check if the tx has duplicate inputs.
    // Failure to run this check will result in either a crash or an inflation bug, depending on the implementation of
    // the underlying coins database.
    std::set<COutPoint> vInOutPoints;
    for (const auto& txin : tx.vin) {
        if (!vInOutPoints.insert(txin.prevout).second)
            return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-txns-inputs-duplicate");
    }

    if (tx.IsCoinBase()) {
        size_t minCbSize = 2;
        if (tx.nType == TRANSACTION_COINBASE) {
            // With the introduction of CbTx, coinbase scripts are not required anymore to hold a valid block height
            minCbSize = 1;
        }
        if (tx.vin[0].scriptSig.size() < minCbSize || tx.vin[0].scriptSig.size() > 100)
            return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-cb-length");
    } else {
        for (const auto& txin : tx.vin)
            if (txin.prevout.IsNull())
                return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-txns-prevout-null");
    }

    return true;
}
