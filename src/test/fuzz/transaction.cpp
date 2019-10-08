// Copyright (c) 2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <coins.h>
#include <consensus/tx_verify.h>
#include <consensus/validation.h>
#include <core_io.h>
#include <core_memusage.h>
#include <policy/policy.h>
#include <primitives/transaction.h>
#include <streams.h>
#include <test/fuzz/fuzz.h>
#include <validation.h>
#include <version.h>

#include <cassert>

void test_one_input(const std::vector<uint8_t>& buffer)
{
    CDataStream ds(buffer, SER_NETWORK, INIT_PROTO_VERSION);
    try {
        int nVersion;
        ds >> nVersion;
        ds.SetVersion(nVersion);
    } catch (const std::ios_base::failure& e) {
        return;
    }
    bool valid = true;
    const CTransaction tx = [&] {
        try {
            return CTransaction(deserialize, ds);
        } catch (const std::ios_base::failure& e) {
            valid = false;
            return CTransaction();
        }
    }();
    if (!valid) {
        return;
    }

    CValidationState state_with_dupe_check;
    const bool valid_with_dupe_check = CheckTransaction(tx, state_with_dupe_check);
    CValidationState state_without_dupe_check;
    const bool valid_without_dupe_check = CheckTransaction(tx, state_without_dupe_check);
    if (valid_with_dupe_check) {
        assert(valid_without_dupe_check);
    }

    std::string reason;
    const bool is_standard_with_permit_bare_multisig = IsStandardTx(tx, reason);
    const bool is_standard_without_permit_bare_multisig = IsStandardTx(tx, reason);
    if (is_standard_without_permit_bare_multisig) {
        assert(is_standard_with_permit_bare_multisig);
    }

    (void)tx.GetHash();
    (void)tx.GetTotalSize();
    try {
        (void)tx.GetValueOut();
    } catch (const std::runtime_error&) {
    }
    (void)tx.IsCoinBase();
    (void)tx.IsNull();
    (void)tx.ToString();

    (void)EncodeHexTx(tx);
    (void)GetLegacySigOpCount(tx);
    (void)IsFinalTx(tx, /* nBlockHeight= */ 1024, /* nBlockTime= */ 1024);
    (void)IsStandardTx(tx, reason);
    (void)RecursiveDynamicUsage(tx);
}
