// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/validation.h>
#include <net.h>
#include <txmempool.h>
#include <validation.h>
#include <validationinterface.h>
#include <node/transaction.h>
#include <util/validation.h>

#include <future>

const char* TransactionErrorString(const TransactionError err)
{
    switch (err) {
        case TransactionError::OK:
            return "No error";
        case TransactionError::MISSING_INPUTS:
            return "Missing inputs";
        case TransactionError::ALREADY_IN_CHAIN:
            return "Transaction already in block chain";
        case TransactionError::P2P_DISABLED:
            return "Peer-to-peer functionality missing or disabled";
        case TransactionError::MEMPOOL_REJECTED:
            return "Transaction rejected by AcceptToMemoryPool";
        case TransactionError::MEMPOOL_ERROR:
            return "AcceptToMemoryPool failed";
        case TransactionError::INVALID_PSBT:
            return "PSBT is not sane";
        case TransactionError::PSBT_MISMATCH:
            return "PSBTs not compatible (different transactions)";
        case TransactionError::SIGHASH_MISMATCH:
            return "Specified sighash value does not match existing value";

        case TransactionError::UNKNOWN_ERROR:
        default: break;
    }
    return "Unknown error";
}

bool BroadcastTransaction(const CTransactionRef tx, uint256& hashTx, TransactionError& error, std::string& err_string, const bool allowhighfees, const bool bypass_limits)
{
    std::promise<void> promise;
    hashTx = tx->GetHash();

    CAmount nMaxRawTxFee = maxTxFee;
    if (allowhighfees)
        nMaxRawTxFee = 0;

    { // cs_main scope
    LOCK(cs_main);
    CCoinsViewCache &view = *pcoinsTip;
    bool fHaveChain = false;
    for (size_t o = 0; !fHaveChain && o < tx->vout.size(); o++) {
        const Coin& existingCoin = view.AccessCoin(COutPoint(hashTx, o));
        fHaveChain = !existingCoin.IsSpent();
    }
    bool fHaveMempool = mempool.exists(hashTx);
    if (!fHaveMempool && !fHaveChain) {
        // push to local node and sync with wallets
        CValidationState state;
        bool fMissingInputs;
        if (!AcceptToMemoryPool(mempool, state, std::move(tx), &fMissingInputs,
                                bypass_limits, nMaxRawTxFee)) {
            if (state.IsInvalid()) {
                err_string = FormatStateMessage(state);
                error = TransactionError::MEMPOOL_REJECTED;
                return false;
            } else {
                if (fMissingInputs) {
                    error = TransactionError::MISSING_INPUTS;
                    return false;
                }
                err_string = FormatStateMessage(state);
                error = TransactionError::MEMPOOL_ERROR;
                return false;
            }
        } else {
            // If wallet is enabled, ensure that the wallet has been made aware
            // of the new transaction prior to returning. This prevents a race
            // where a user might call sendrawtransaction with a transaction
            // to/from their wallet, immediately call some wallet RPC, and get
            // a stale result because callbacks have not yet been processed.
            CallFunctionInValidationInterfaceQueue([&promise] {
                promise.set_value();
            });
        }
    } else if (fHaveChain) {
        error = TransactionError::ALREADY_IN_CHAIN;
        return false;
    } else {
        // Make sure we don't block forever if re-sending
        // a transaction already in mempool.
        promise.set_value();
    }

    } // cs_main

    promise.get_future().wait();

    if(!g_connman) {
        error = TransactionError::P2P_DISABLED;
        return false;
    }

    g_connman->RelayTransaction(*tx);

    return true;
    }
