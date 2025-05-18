// Copyright (c) 2019-2022 The Tortoisecoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/validation.h>
#include <core_memusage.h>
#include <policy/feerate.h>
#include <policy/policy.h>
#include <primitives/transaction.h>
#include <streams.h>
#include <test/fuzz/fuzz.h>

FUZZ_TARGET(tx_out)
{
    DataStream ds{buffer};
    CTxOut tx_out;
    try {
        ds >> tx_out;
    } catch (const std::ios_base::failure&) {
        return;
    }

    const CFeeRate dust_relay_fee{DUST_RELAY_TX_FEE};
    (void)GetDustThreshold(tx_out, dust_relay_fee);
    (void)IsDust(tx_out, dust_relay_fee);
    (void)RecursiveDynamicUsage(tx_out);

    (void)tx_out.ToString();
    (void)tx_out.IsNull();
    tx_out.SetNull();
    assert(tx_out.IsNull());
}
