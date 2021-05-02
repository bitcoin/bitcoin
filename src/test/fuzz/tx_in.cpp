// Copyright (c) 2019 The XBit Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/validation.h>
#include <core_memusage.h>
#include <policy/policy.h>
#include <primitives/transaction.h>
#include <streams.h>
#include <test/fuzz/fuzz.h>
#include <version.h>

#include <cassert>

void test_one_input(const std::vector<uint8_t>& buffer)
{
    CDataStream ds(buffer, SER_NETWORK, INIT_PROTO_VERSION);
    CTxIn tx_in;
    try {
        int version;
        ds >> version;
        ds.SetVersion(version);
        ds >> tx_in;
    } catch (const std::ios_base::failure&) {
        return;
    }

    (void)GetTransactionInputWeight(tx_in);
    (void)GetVirtualTransactionInputSize(tx_in);
    (void)RecursiveDynamicUsage(tx_in);

    (void)tx_in.ToString();
}
