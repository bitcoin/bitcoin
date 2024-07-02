// Copyright (c) 2019-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>
#include <coins.h>
#include <consensus/tx_check.h>
#include <consensus/tx_verify.h>
#include <consensus/validation.h>
#include <core_io.h>
#include <core_memusage.h>
#include <policy/policy.h>
#include <policy/settings.h>
#include <primitives/transaction.h>
#include <streams.h>
#include <test/fuzz/fuzz.h>
#include <univalue.h>
#include <util/chaintype.h>
#include <util/rbf.h>
#include <validation.h>

#include <cassert>

void initialize_transaction()
{
    SelectParams(ChainType::REGTEST);
}

FUZZ_TARGET(transaction, .init = initialize_transaction)
{
    DataStream ds{buffer};
    bool valid_tx = true;
    const CTransaction tx = [&] {
        try {
            return CTransaction(deserialize, TX_WITH_WITNESS, ds);
        } catch (const std::ios_base::failure&) {
            valid_tx = false;
            return CTransaction{CMutableTransaction{}};
        }
    }();
    bool valid_mutable_tx = true;
    DataStream ds_mtx{buffer};
    CMutableTransaction mutable_tx;
    try {
        ds_mtx >> TX_WITH_WITNESS(mutable_tx);
    } catch (const std::ios_base::failure&) {
        valid_mutable_tx = false;
    }
    assert(valid_tx == valid_mutable_tx);
    if (!valid_tx) {
        return;
    }

    {
        TxValidationState state_with_dupe_check;
        const bool res{CheckTransaction(tx, state_with_dupe_check)};
        Assert(res == state_with_dupe_check.IsValid());
    }

    const CFeeRate dust_relay_fee{DUST_RELAY_TX_FEE};
    std::string reason;
    const bool is_standard_with_permit_bare_multisig = IsStandardTx(tx, std::nullopt, /* permit_bare_multisig= */ true, dust_relay_fee, reason);
    const bool is_standard_without_permit_bare_multisig = IsStandardTx(tx, std::nullopt, /* permit_bare_multisig= */ false, dust_relay_fee, reason);
    if (is_standard_without_permit_bare_multisig) {
        assert(is_standard_with_permit_bare_multisig);
    }

    (void)tx.GetHash();
    (void)tx.GetTotalSize();
    try {
        (void)tx.GetValueOut();
    } catch (const std::runtime_error&) {
    }
    (void)tx.GetWitnessHash();
    (void)tx.HasWitness();
    (void)tx.IsCoinBase();
    (void)tx.IsNull();
    (void)tx.ToString();

    (void)EncodeHexTx(tx);
    (void)GetLegacySigOpCount(tx);
    (void)GetTransactionWeight(tx);
    (void)GetVirtualTransactionSize(tx);
    (void)IsFinalTx(tx, /* nBlockHeight= */ 1024, /* nBlockTime= */ 1024);
    (void)RecursiveDynamicUsage(tx);
    (void)SignalsOptInRBF(tx);

    CCoinsView coins_view;
    const CCoinsViewCache coins_view_cache(&coins_view);
    (void)HasNonStandardInput(tx, coins_view_cache);
    (void)IsWitnessStandard(tx, coins_view_cache);

    if (tx.GetTotalSize() < 250'000) { // Avoid high memory usage (with msan) due to json encoding
        {
            UniValue u{UniValue::VOBJ};
            TxToUniv(tx, /*block_hash=*/uint256::ZERO, /*entry=*/u);
        }
        {
            UniValue u{UniValue::VOBJ};
            TxToUniv(tx, /*block_hash=*/uint256::ONE, /*entry=*/u);
        }
    }
}
