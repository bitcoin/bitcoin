// Copyright (c) 2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <core_io.h>
#include <primitives/transaction.h>
#include <test/fuzz/fuzz.h>
#include <util/strencodings.h>

#include <cassert>
#include <cstdint>
#include <string>
#include <vector>

void test_one_input(const std::vector<uint8_t>& buffer)
{
    const std::string tx_hex = HexStr(buffer);
    CMutableTransaction mtx;
    const bool result_none = DecodeHexTx(mtx, tx_hex, false, false);
    const bool result_try_witness = DecodeHexTx(mtx, tx_hex, false, true);
    const bool result_try_witness_and_maybe_no_witness = DecodeHexTx(mtx, tx_hex, true, true);
    const bool result_try_no_witness = DecodeHexTx(mtx, tx_hex, true, false);
    assert(!result_none);
    if (result_try_witness_and_maybe_no_witness) {
        assert(result_try_no_witness || result_try_witness);
    }
    // if (result_try_no_witness) { // Uncomment when https://github.com/bitcoin/bitcoin/pull/17775 is merged
    if (result_try_witness) { // Remove stop-gap when https://github.com/bitcoin/bitcoin/pull/17775 is merged
        assert(result_try_witness_and_maybe_no_witness);
    }
}
