// Copyright (c) 2017-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/tx_check.h>

#include <consensus/amount.h>
#include <consensus/consensus.h>
#include <consensus/validation.h>
#include <primitives/transaction.h>
#include <script/script.h>
#include <serialize.h>

#include <algorithm>
#include <cstddef>
#include <string>
#include <vector>

bool CheckTransaction(const CTransaction& tx, TxValidationState& state)
{
    // Basic checks that don't depend on any context
    if (tx.vin.empty())
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-txns-vin-empty");
    if (tx.vout.empty())
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-txns-vout-empty");
    // Size limits (this doesn't take the witness into account, as that hasn't been checked for malleability)
    if (::GetSerializeSize(TX_NO_WITNESS(tx)) * WITNESS_SCALE_FACTOR > MAX_BLOCK_WEIGHT) {
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-txns-oversize");
    }

    // Check for negative or overflow output values (see CVE-2010-5139)
    CAmount nValueOut = 0;
    for (const auto& txout : tx.vout)
    {
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
    //
    // This runs for every transaction of every connected block, so it is on the initial block download
    // critical path. Two strategies are used, both of which avoid the per-transaction node allocations of
    // a std::set:
    //
    // * Small transactions (essentially all of them) are checked by comparing every pair of inputs. The
    //   output index is compared first, which is cheap and rejects nearly all pairs without touching the
    //   32-byte transaction id.
    // * Large transactions fall back to sorting a contiguous buffer, which is O(n log n) instead of the
    //   O(n^2) of the pairwise scan.
    static constexpr size_t MAX_PAIRWISE_INPUTS{96};
    if (tx.vin.size() <= MAX_PAIRWISE_INPUTS) {
        for (size_t i{0}; i < tx.vin.size(); ++i) {
            const COutPoint& prevout{tx.vin[i].prevout};
            for (size_t j{0}; j < i; ++j) {
                const COutPoint& other{tx.vin[j].prevout};
                if (prevout.n == other.n && prevout.hash == other.hash)
                    return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-txns-inputs-duplicate");
            }
        }
    } else {
        std::vector<COutPoint> sorted;
        sorted.reserve(tx.vin.size());
        for (const auto& txin : tx.vin) sorted.push_back(txin.prevout);
        std::sort(sorted.begin(), sorted.end());
        if (std::adjacent_find(sorted.begin(), sorted.end()) != sorted.end())
            return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-txns-inputs-duplicate");
    }

    if (tx.IsCoinBase())
    {
        if (tx.vin[0].scriptSig.size() < 2 || tx.vin[0].scriptSig.size() > 100)
            return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-cb-length");
    }
    else
    {
        for (const auto& txin : tx.vin)
            if (txin.prevout.IsNull())
                return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-txns-prevout-null");
    }

    return true;
}
