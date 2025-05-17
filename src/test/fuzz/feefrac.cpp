// Copyright (c) 2024 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <arith_uint256.h>
#include <policy/feerate.h>
#include <util/feefrac.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>

#include <compare>
#include <cmath>
#include <cstdint>
#include <iostream>

namespace {

/** The maximum absolute value of an int64_t, as an arith_uint256 (2^63). */
const auto MAX_ABS_INT64 = arith_uint256{1} << 63;

/** Construct an arith_uint256 whose value equals abs(x). */
arith_uint256 Abs256(int64_t x)
{
    if (x >= 0) {
        // For positive numbers, pass through the value.
        return arith_uint256{static_cast<uint64_t>(x)};
    } else if (x > std::numeric_limits<int64_t>::min()) {
        // For negative numbers, negate first.
        return arith_uint256{static_cast<uint64_t>(-x)};
    } else {
        // Special case for x == -2^63 (for which -x results in integer overflow).
        return MAX_ABS_INT64;
    }
}

/** Construct an arith_uint256 whose value equals abs(x), for 96-bit x. */
arith_uint256 Abs256(std::pair<int64_t, uint32_t> x)
{
    if (x.first >= 0) {
        // x.first and x.second are both non-negative; sum their absolute values.
        return (Abs256(x.first) << 32) + Abs256(x.second);
    } else {
        // x.first is negative and x.second is non-negative; subtract the absolute values.
        return (Abs256(x.first) << 32) - Abs256(x.second);
    }
}

std::strong_ordering MulCompare(int64_t a1, int64_t a2, int64_t b1, int64_t b2)
{
    // Compute and compare signs.
    int sign_a = (a1 == 0 ? 0 : a1 < 0 ? -1 : 1) * (a2 == 0 ? 0 : a2 < 0 ? -1 : 1);
    int sign_b = (b1 == 0 ? 0 : b1 < 0 ? -1 : 1) * (b2 == 0 ? 0 : b2 < 0 ? -1 : 1);
    if (sign_a != sign_b) return sign_a <=> sign_b;

    // Compute absolute values of products.
    auto mul_abs_a = Abs256(a1) * Abs256(a2), mul_abs_b = Abs256(b1) * Abs256(b2);

    // Compute products of absolute values.
    if (sign_a < 0) {
        return mul_abs_b <=> mul_abs_a;
    } else {
        return mul_abs_a <=> mul_abs_b;
    }
}

} // namespace

FUZZ_TARGET(feefrac)
{
    FuzzedDataProvider provider(buffer.data(), buffer.size());

    int64_t f1 = provider.ConsumeIntegral<int64_t>();
    int32_t s1 = provider.ConsumeIntegral<int32_t>();
    if (s1 == 0) f1 = 0;
    FeeFrac fr1(f1, s1);
    assert(fr1.IsEmpty() == (s1 == 0));

    int64_t f2 = provider.ConsumeIntegral<int64_t>();
    int32_t s2 = provider.ConsumeIntegral<int32_t>();
    if (s2 == 0) f2 = 0;
    FeeFrac fr2(f2, s2);
    assert(fr2.IsEmpty() == (s2 == 0));

    // Feerate comparisons
    auto cmp_feerate = MulCompare(f1, s2, f2, s1);
    assert(FeeRateCompare(fr1, fr2) == cmp_feerate);
    assert((fr1 << fr2) == std::is_lt(cmp_feerate));
    assert((fr1 >> fr2) == std::is_gt(cmp_feerate));

    // Compare with manual invocation of FeeFrac::Mul.
    auto cmp_mul = FeeFrac::Mul(f1, s2) <=> FeeFrac::Mul(f2, s1);
    assert(cmp_mul == cmp_feerate);

    // Same, but using FeeFrac::MulFallback.
    auto cmp_fallback = FeeFrac::MulFallback(f1, s2) <=> FeeFrac::MulFallback(f2, s1);
    assert(cmp_fallback == cmp_feerate);

    // Total order comparisons
    auto cmp_total = std::is_eq(cmp_feerate) ? (s2 <=> s1) : cmp_feerate;
    assert((fr1 <=> fr2) == cmp_total);
    assert((fr1 < fr2) == std::is_lt(cmp_total));
    assert((fr1 > fr2) == std::is_gt(cmp_total));
    assert((fr1 <= fr2) == std::is_lteq(cmp_total));
    assert((fr1 >= fr2) == std::is_gteq(cmp_total));
    assert((fr1 == fr2) == std::is_eq(cmp_total));
    assert((fr1 != fr2) == std::is_neq(cmp_total));
}

FUZZ_TARGET(feefrac_div_fallback)
{
    // Verify the behavior of FeeFrac::DivFallback over all possible inputs.

    // Construct a 96-bit signed value num, a positive 31-bit value den, and rounding mode.
    FuzzedDataProvider provider(buffer.data(), buffer.size());
    auto num_high = provider.ConsumeIntegral<int64_t>();
    auto num_low = provider.ConsumeIntegral<uint32_t>();
    std::pair<int64_t, uint32_t> num{num_high, num_low};
    auto den = provider.ConsumeIntegralInRange<int32_t>(1, std::numeric_limits<int32_t>::max());
    auto round_down = provider.ConsumeBool();

    // Predict the sign of the actual result.
    bool is_negative = num_high < 0;
    // Evaluate absolute value using arith_uint256. If the actual result is negative and we are
    // rounding down, or positive and we are rounding up, the absolute value of the quotient is
    // the rounded-up quotient of the absolute values.
    auto num_abs = Abs256(num);
    auto den_abs = Abs256(den);
    auto quot_abs = (is_negative == round_down) ?
        (num_abs + den_abs - 1) / den_abs :
        num_abs / den_abs;

    // If the result is not representable by an int64_t, bail out.
    if ((is_negative && quot_abs > MAX_ABS_INT64) || (!is_negative && quot_abs >= MAX_ABS_INT64)) {
        return;
    }

    // Verify the behavior of FeeFrac::DivFallback.
    auto res = FeeFrac::DivFallback(num, den, round_down);
    assert(res == 0 || (res < 0) == is_negative);
    assert(Abs256(res) == quot_abs);

    // Compare approximately with floating-point.
    long double expect = round_down ? std::floor(num_high * 4294967296.0L + num_low) / den
                                    : std::ceil(num_high * 4294967296.0L + num_low) / den;
    // Expect to be accurate within 50 bits of precision, +- 1 sat.
    if (expect == 0.0L) {
        assert(res >= -1 && res <= 1);
    } else if (expect > 0.0L) {
        assert(res >= expect * 0.999999999999999L - 1.0L);
        assert(res <= expect * 1.000000000000001L + 1.0L);
    } else {
        assert(res >= expect * 1.000000000000001L - 1.0L);
        assert(res <= expect * 0.999999999999999L + 1.0L);
    }
}

FUZZ_TARGET(feefrac_mul_div)
{
    // Verify the behavior of:
    // - The combination of FeeFrac::Mul + FeeFrac::Div.
    // - The combination of FeeFrac::MulFallback + FeeFrac::DivFallback.
    // - FeeFrac::Evaluate.

    // Construct a 32-bit signed multiplicand, a 64-bit signed multiplicand, a positive 31-bit
    // divisor, and a rounding mode.
    FuzzedDataProvider provider(buffer.data(), buffer.size());
    auto mul32 = provider.ConsumeIntegral<int32_t>();
    auto mul64 = provider.ConsumeIntegral<int64_t>();
    auto div = provider.ConsumeIntegralInRange<int32_t>(1, std::numeric_limits<int32_t>::max());
    auto round_down = provider.ConsumeBool();

    // Predict the sign of the overall result.
    bool is_negative = ((mul32 < 0) && (mul64 > 0)) || ((mul32 > 0) && (mul64 < 0));
    // Evaluate absolute value using arith_uint256. If the actual result is negative and we are
    // rounding down or positive and we rounding up, the absolute value of the quotient is the
    // rounded-up quotient of the absolute values.
    auto prod_abs = Abs256(mul32) * Abs256(mul64);
    auto div_abs = Abs256(div);
    auto quot_abs = (is_negative == round_down) ?
        (prod_abs + div_abs - 1) / div_abs :
        prod_abs / div_abs;

    // If the result is not representable by an int64_t, bail out.
    if ((is_negative && quot_abs > MAX_ABS_INT64) || (!is_negative && quot_abs >= MAX_ABS_INT64)) {
        // If 0 <= mul32 <= div, then the result is guaranteed to be representable. In the context
        // of the Evaluate{Down,Up} calls below, this corresponds to 0 <= at_size <= feefrac.size.
        assert(mul32 < 0 || mul32 > div);
        return;
    }

    // Verify the behavior of FeeFrac::Mul + FeeFrac::Div.
    auto res = FeeFrac::Div(FeeFrac::Mul(mul64, mul32), div, round_down);
    assert(res == 0 || (res < 0) == is_negative);
    assert(Abs256(res) == quot_abs);

    // Verify the behavior of FeeFrac::MulFallback + FeeFrac::DivFallback.
    auto res_fallback = FeeFrac::DivFallback(FeeFrac::MulFallback(mul64, mul32), div, round_down);
    assert(res == res_fallback);

    // Compare approximately with floating-point.
    long double expect = round_down ? std::floor(static_cast<long double>(mul32) * mul64 / div)
                                    : std::ceil(static_cast<long double>(mul32) * mul64 / div);
    // Expect to be accurate within 50 bits of precision, +- 1 sat.
    if (expect == 0.0L) {
        assert(res >= -1 && res <= 1);
    } else if (expect > 0.0L) {
        assert(res >= expect * 0.999999999999999L - 1.0L);
        assert(res <= expect * 1.000000000000001L + 1.0L);
    } else {
        assert(res >= expect * 1.000000000000001L - 1.0L);
        assert(res <= expect * 0.999999999999999L + 1.0L);
    }

    // Verify the behavior of FeeFrac::Evaluate{Down,Up}.
    if (mul32 >= 0) {
        auto res_fee = round_down ?
            FeeFrac{mul64, div}.EvaluateFeeDown(mul32) :
            FeeFrac{mul64, div}.EvaluateFeeUp(mul32);
        assert(res == res_fee);

        // Compare approximately with CFeeRate.
        if (mul64 < std::numeric_limits<int64_t>::max() / 1000 &&
            mul64 > std::numeric_limits<int64_t>::min() / 1000 &&
            quot_abs < arith_uint256{std::numeric_limits<int64_t>::max() / 1000}) {
            CFeeRate feerate(mul64, div);
            CAmount feerate_fee{feerate.GetFee(mul32)};
            auto allowed_gap = static_cast<int64_t>(mul32 / 1000 + 3 + round_down);
            assert(feerate_fee - res_fee >= -allowed_gap);
            assert(feerate_fee - res_fee <= allowed_gap);
        }
    }
}
