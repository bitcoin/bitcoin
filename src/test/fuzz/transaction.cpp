// Copyright (c) 2019-2020 The Widecoin Core developers
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
#include <util/rbf.h>
#include <validation.h>
#include <version.h>

#include <cassert>

void initialize()
{
    SelectParams(CBaseChainParams::REGTEST);
}

void test_one_input(const std::vector<uint8_t>& buffer)
{
    CDataStream ds(buffer, SER_NETWORK, INIT_PROTO_VERSION);
    try {
        int nVersion;
        ds >> nVersion;
        ds.SetVersion(nVersion);
    } catch (const std::ios_base::failure&) {
        return;
    }
    bool valid_tx = true;
    const CTransaction tx = [&] {
        try {
            return CTransaction(deserialize, ds);
        } catch (const std::ios_base::failure&) {
            valid_tx = false;
            return CTransaction();
        }
    }();
    bool valid_mutable_tx = true;
    CDataStream ds_mtx(buffer, SER_NETWORK, INIT_PROTO_VERSION);
    CMutableTransaction mutable_tx;
    try {
        int nVersion;
        ds_mtx >> nVersion;
        ds_mtx.SetVersion(nVersion);
        ds_mtx >> mutable_tx;
    } catch (const std::ios_base::failure&) {
        valid_mutable_tx = false;
    }
    assert(valid_tx == valid_mutable_tx);
    if (!valid_tx) {
        return;
    }

    TxValidationState state_with_dupe_check;
    (void)CheckTransaction(tx, state_with_dupe_check);

    const CFeeRate dust_relay_fee{DUST_RELAY_TX_FEE};
    std::string reason;
    const bool is_standard_with_permit_bare_multisig = IsStandardTx(tx, /* permit_bare_multisig= */ true, dust_relay_fee, reason);
    const bool is_standard_without_permit_bare_multisig = IsStandardTx(tx, /* permit_bare_multisig= */ false, dust_relay_fee, reason);
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
    (void)IsStandardTx(tx, reason);
    (void)RecursiveDynamicUsage(tx);
    (void)SignalsOptInRBF(tx);

    CCoinsView coins_view;
    const CCoinsViewCache coins_view_cache(&coins_view);
    (void)AreInputsStandard(tx, coins_view_cache, false);
    (void)AreInputsStandard(tx, coins_view_cache, true);
    (void)IsWitnessStandard(tx, coins_view_cache);

    UniValue u(UniValue::VOBJ);
    // ValueFromAmount(i) not defined when i == std::numeric_limits<int64_t>::min()
    bool skip_tx_to_univ = false;
    for (const CTxOut& txout : tx.vout) {
        if (txout.nValue == std::numeric_limits<int64_t>::min()) {
            skip_tx_to_univ = true;
        }
    }
    if (!skip_tx_to_univ) {
        TxToUniv(tx, /* hashBlock */ {}, u);
        static const uint256 u256_max(uint256S("ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"));
        TxToUniv(tx, u256_max, u);
    }
}
