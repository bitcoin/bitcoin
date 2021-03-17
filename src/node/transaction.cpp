// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/validation.h>
#include <net.h>
#include <net_processing.h>
#include <node/context.h>
#include <validation.h>
#include <validationinterface.h>
#include <node/transaction.h>

#include <future>

static TransactionError HandleATMPError(const TxValidationState& state, std::string& err_string_out)
{
    err_string_out = state.ToString();
    if (state.IsInvalid()) {
        if (state.GetResult() == TxValidationResult::TX_MISSING_INPUTS) {
            return TransactionError::MISSING_INPUTS;
        }
        return TransactionError::MEMPOOL_REJECTED;
    } else {
        return TransactionError::MEMPOOL_ERROR;
    }
}

TransactionError BroadcastTransaction(NodeContext& node, const CTransactionRef tx, std::string& err_string, const CAmount& max_tx_fee, bool relay, bool wait_callback)
{
    // BroadcastTransaction can be called by either sendrawtransaction RPC or wallet RPCs.
    // node.peerman is assigned both before chain clients and before RPC server is accepting calls,
    // and reset after chain clients and RPC sever are stopped. node.peerman should never be null here.
    assert(node.peerman);
    assert(node.mempool);
    std::promise<void> promise;
    uint256 hashTx = tx->GetHash();
    bool callback_set = false;

    { // cs_main scope
    assert(node.chainman);
    LOCK(cs_main);
    assert(std::addressof(::ChainstateActive()) == std::addressof(node.chainman->ActiveChainstate()));
    // If the transaction is already confirmed in the chain, don't do anything
    // and return early.
    CCoinsViewCache &view = node.chainman->ActiveChainstate().CoinsTip();
    for (size_t o = 0; o < tx->vout.size(); o++) {
        const Coin& existingCoin = view.AccessCoin(COutPoint(hashTx, o));
        // IsSpent doesn't mean the coin is spent, it means the output doesn't exist.
        // So if the output does exist, then this transaction exists in the chain.
        if (!existingCoin.IsSpent()) return TransactionError::ALREADY_IN_CHAIN;
    }
    if (!node.mempool->exists(hashTx)) {
        // Transaction is not already in the mempool.
        if (max_tx_fee > 0) {
            // First, call ATMP with test_accept and check the fee. If ATMP
            // fails here, return error immediately.
            const MempoolAcceptResult result = AcceptToMemoryPool(node.chainman->ActiveChainstate(), *node.mempool, tx, false /* bypass_limits */,
                                                                  true /* test_accept */);
            if (result.m_result_type != MempoolAcceptResult::ResultType::VALID) {
                return HandleATMPError(result.m_state, err_string);
            } else if (result.m_base_fees.value() > max_tx_fee) {
                return TransactionError::MAX_FEE_EXCEEDED;
            }
        }
        // Try to submit the transaction to the mempool.
        const MempoolAcceptResult result = AcceptToMemoryPool(node.chainman->ActiveChainstate(), *node.mempool, tx, false /* bypass_limits */,
                                                              false /* test_accept */);
        if (result.m_result_type != MempoolAcceptResult::ResultType::VALID) {
            return HandleATMPError(result.m_state, err_string);
        }

        // Transaction was accepted to the mempool.

        if (wait_callback) {
            // For transactions broadcast from outside the wallet, make sure
            // that the wallet has been notified of the transaction before
            // continuing.
            //
            // This prevents a race where a user might call sendrawtransaction
            // with a transaction to/from their wallet, immediately call some
            // wallet RPC, and get a stale result because callbacks have not
            // yet been processed.
            CallFunctionInValidationInterfaceQueue([&promise] {
                promise.set_value();
            });
            callback_set = true;
        }
    }

    } // cs_main

    if (callback_set) {
        // Wait until Validation Interface clients have been notified of the
        // transaction entering the mempool.
        promise.get_future().wait();
    }

    if (relay) {
        // the mempool tracks locally submitted transactions to make a
        // best-effort of initial broadcast
        node.mempool->AddUnbroadcastTx(hashTx);

        LOCK(cs_main);
        node.peerman->RelayTransaction(hashTx, tx->GetWitnessHash());
    }

    return TransactionError::OK;
}
