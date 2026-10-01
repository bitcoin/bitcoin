// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/amount.h>
#include <consensus/tx_check.h>
#include <consensus/validation.h>
#include <policy/policy.h>
#include <primitives/transaction.h>
#include <privbcast/input.h>
#include <script/script.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>

#include <cassert>
#include <string>

FUZZ_TARGET(privbcast_input)
{
    FuzzedDataProvider provider{buffer.data(), buffer.size()};
    const CAmount max_burn{ConsumeMoney(provider)};
    const std::string text{provider.ConsumeRemainingBytesAsString()};

    const auto txs{privbcast::ParseTransactions(text)};
    if (!txs) return;
    assert(txs->size() == 1);
    const CTransaction& tx{*txs->front()};
    if (!privbcast::CheckForBroadcast(tx, max_burn)) return;

    // What passed is fit to broadcast as the interface describes it.
    TxValidationState state;
    assert(CheckTransaction(tx, state));
    assert(!tx.IsCoinBase());
    assert(GetTransactionWeight(tx) <= MAX_STANDARD_TX_WEIGHT);
    for (const CTxOut& out : tx.vout) {
        assert(!(out.scriptPubKey.IsUnspendable() || !out.scriptPubKey.HasValidOps()) || out.nValue <= max_burn);
    }
    // A higher bound accepts it too.
    assert(privbcast::CheckForBroadcast(tx, MAX_MONEY));
}
