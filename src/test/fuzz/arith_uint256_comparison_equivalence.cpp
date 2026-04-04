// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <arith_uint256.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>

#include <cassert>

namespace {

struct OriginalArithUint256 : arith_uint256 {
    using arith_uint256::arith_uint256;

    explicit OriginalArithUint256(const arith_uint256& v) : arith_uint256(v) {}

    int CompareTo(const OriginalArithUint256& b) const
    {
        for (int i = WIDTH - 1; i >= 0; i--) {
            if (pn[i] < b.pn[i])
                return -1;
            if (pn[i] > b.pn[i])
                return 1;
        }
        return 0;
    }
};

} // namespace

FUZZ_TARGET(arith_uint256_comparison_equivalence)
{
    FuzzedDataProvider provider{buffer.data(), buffer.size()};
    OriginalArithUint256 a{ConsumeArithUInt256(provider)};
    const OriginalArithUint256 b{ConsumeArithUInt256(provider)};
    if (provider.ConsumeBool()) {
        // Exercise identical pointers.
        a = b;
    }

    const int old_result{a.CompareTo(b)};

    const auto comparison{a <=> b};
    const int new_result{comparison < 0 ? -1 : comparison > 0 ? 1 : 0};
    assert(old_result == new_result);

    // Verify all comparison operators match old behavior
    assert((a < b) == (old_result < 0));
    assert((a > b) == (old_result > 0));
    assert((a <= b) == (old_result <= 0));
    assert((a >= b) == (old_result >= 0));
    assert((a == b) == (old_result == 0));
    assert((a != b) == (old_result != 0));
}
