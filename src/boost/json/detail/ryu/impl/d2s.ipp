// Copyright 2018 Ulf Adams
//
// The contents of this file may be used under the terms of the Apache License,
// Version 2.0.
//
//    (See accompanying file LICENSE-Apache or copy at
//     http://www.apache.org/licenses/LICENSE-2.0)
//
// Alternatively, the contents of this file may be used under the terms of
// the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE-Boost or copy at
//     https://www.boost.org/LICENSE_1_0.txt)
//
// Unless required by applicable law or agreed to in writing, this software
// is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.

// Runtime compiler options:
// -DRYU_DEBUG Generate verbose debugging output to stdout.
//
// -DRYU_ONLY_64_BIT_OPS Avoid using uint128_t or 64-bit intrinsics. Slower,
//     depending on your compiler.
//
// -DRYU_OPTIMIZE_SIZE Use smaller lookup tables. Instead of storing every
//     required power of 5, only store every 26th entry, and compute
//     intermediate values with a multiplication. This reduces the lookup table
//     size by about 10x (only one case, and only double) at the cost of some
//     performance. Currently requires MSVC intrinsics.

/*
    This is a derivative work
*/

#ifndef BOOST_JSON_DETAIL_RYU_IMPL_D2S_IPP
#define BOOST_JSON_DETAIL_RYU_IMPL_D2S_IPP

#include <boost/json/detail/ryu/ryu.hpp>
#include <cstdlib>
#include <cstring>

#ifdef RYU_DEBUG
#include <stdio.h>
#endif

// ABSL avoids uint128_t on Win32 even if __SIZEOF_INT128__ is defined.
// Let's do the same for now.
#if defined(__SIZEOF_INT128__) && !defined(_MSC_VER) && !defined(RYU_ONLY_64_BIT_OPS)
#define BOOST_JSON_RYU_HAS_UINT128
#elif defined(_MSC_VER) && !defined(RYU_ONLY_64_BIT_OPS) && defined(_M_X64)
#define BOOST_JSON_RYU_HAS_64_BIT_INTRINSICS
#endif

#include <boost/json/detail/ryu/detail/common.hpp>
#include <boost/json/detail/ryu/detail/digit_table.hpp>
#include <boost/json/detail/ryu/detail/d2s.hpp>
#include <boost/json/detail/ryu/detail/d2s_intrinsics.hpp>

BOOST_JSON_NS_BEGIN
namespace detail {

namespace ryu {
namespace detail {

// We need a 64x128-bit multiplication and a subsequent 128-bit shift.
// Multiplication:
//   The 64-bit factor is variable and passed in, the 128-bit factor comes
//   from a lookup table. We know that the 64-bit factor only has 55
//   significant bits (i.e., the 9 topmost bits are zeros). The 128-bit
//   factor only has 124 significant bits (i.e., the 4 topmost bits are
//   zeros).
// Shift:
//   In principle, the multiplication result requires 55 + 124 = 179 bits to
//   represent. However, we then shift this value to the right by j, which is
//   at least j >= 115, so the result is guaranteed to fit into 179 - 115 = 64
//   bits. This means that we only need the topmost 64 significant bits of
//   the 64x128-bit multiplication.
//
// There are several ways to do this:
// 1. Best case: the compiler exposes a 128-bit type.
//    We perform two 64x64-bit multiplications, add the higher 64 bits of the
//    lower result to the higher result, and shift by j - 64 bits.
//
//    We explicitly cast from 64-bit to 128-bit, so the compiler can tell
//    that these are only 64-bit inputs, and can map these to the best
//    possible sequence of assembly instructions.
//    x64 machines happen to have matching assembly instructions for
//    64x64-bit multiplications and 128-bit shifts.
//
// 2. Second best case: the compiler exposes intrinsics for the x64 assembly
//    instructions mentioned in 1.
//
// 3. We only have 64x64 bit instructions that return the lower 64 bits of
//    the result, i.e., we have to use plain C.
//    Our inputs are less than the full width, so we have three options:
//    a. Ignore this fact and just implement the intrinsics manually.
//    b. Split both into 31-bit pieces, which guarantees no internal overflow,
//       but requires extra work upfront (unless we change the lookup table).
//    c. Split only the first factor into 31-bit pieces, which also guarantees
//       no internal overflow, but requires extra work since the intermediate
//       results are not perfectly aligned.
#if defined(BOOST_JSON_RYU_HAS_UINT128)

// Best case: use 128-bit type.
inline
std::uint64_t
    mulShift(
    const std::uint64_t m,
    const std::uint64_t* const mul,
    const std::int32_t j) noexcept
{
    const uint128_t b0 = ((uint128_t) m) * mul[0];
    const uint128_t b2 = ((uint128_t) m) * mul[1];
    return (std::uint64_t) (((b0 >> 64) + b2) >> (j - 64));
}

inline
uint64_t
mulShiftAll(
    const std::uint64_t m,
    const std::uint64_t* const mul,
    std::int32_t const j,
    std::uint64_t* const vp,
    std::uint64_t* const vm,
    const std::uint32_t mmShift) noexcept
{
//  m <<= 2;
//  uint128_t b0 = ((uint128_t) m) * mul[0]; // 0
//  uint128_t b2 = ((uint128_t) m) * mul[1]; // 64
//
//  uint128_t hi = (b0 >> 64) + b2;
//  uint128_t lo = b0 & 0xffffffffffffffffull;
//  uint128_t factor = (((uint128_t) mul[1]) << 64) + mul[0];
//  uint128_t vpLo = lo + (factor << 1);
//  *vp = (std::uint64_t) ((hi + (vpLo >> 64)) >> (j - 64));
//  uint128_t vmLo = lo - (factor << mmShift);
//  *vm = (std::uint64_t) ((hi + (vmLo >> 64) - (((uint128_t) 1ull) << 64)) >> (j - 64));
//  return (std::uint64_t) (hi >> (j - 64));
    *vp = mulShift(4 * m + 2, mul, j);
    *vm = mulShift(4 * m - 1 - mmShift, mul, j);
    return mulShift(4 * m, mul, j);
}

#elif defined(BOOST_JSON_RYU_HAS_64_BIT_INTRINSICS)

inline
std::uint64_t
mulShift(
    const std::uint64_t m,
    const std::uint64_t* const mul,
    const std::int32_t j) noexcept
{
    // m is maximum 55 bits
    std::uint64_t high1;                                   // 128
    std::uint64_t const low1 = umul128(m, mul[1], &high1); // 64
    std::uint64_t high0;                                   // 64
    umul128(m, mul[0], &high0);                            // 0
    std::uint64_t const sum = high0 + low1;
    if (sum < high0)
        ++high1; // overflow into high1
    return shiftright128(sum, high1, j - 64);
}

inline
std::uint64_t
mulShiftAll(
    const std::uint64_t m,
    const std::uint64_t* const mul,
    const std::int32_t j,
    std::uint64_t* const vp,
    std::uint64_t* const vm,
    const std::uint32_t mmShift) noexcept
{
    *vp = mulShift(4 * m + 2, mul, j);
    *vm = mulShift(4 * m - 1 - mmShift, mul, j);
    return mulShift(4 * m, mul, j);
}

#else // !defined(BOOST_JSON_RYU_HAS_UINT128) && !defined(BOOST_JSON_RYU_HAS_64_BIT_INTRINSICS)

inline
std::uint64_t
mulShiftAll(
    std::uint64_t m,
    const std::uint64_t* const mul,
    const std::int32_t j,
    std::uint64_t* const vp,
    std::uint64_t* const vm,
    const std::uint32_t mmShift)
{
    m <<= 1;
    // m is maximum 55 bits
    std::uint64_t tmp;
    std::uint64_t const lo = umul128(m, mul[0], &tmp);
    std::uint64_t hi;
    std::uint64_t const mid = tmp + umul128(m, mul[1], &hi);
    hi += mid < tmp; // overflow into hi

    const std::uint64_t lo2 = lo + mul[0];
    const std::uint64_t mid2 = mid + mul[1] + (lo2 < lo);
    const std::uint64_t hi2 = hi + (mid2 < mid);
    *vp = shiftright128(mid2, hi2, (std::uint32_t)(j - 64 - 1));

    if (mmShift == 1)
    {
        const std::uint64_t lo3 = lo - mul[0];
        const std::uint64_t mid3 = mid - mul[1] - (lo3 > lo);
        const std::uint64_t hi3 = hi - (mid3 > mid);
        *vm = shiftright128(mid3, hi3, (std::uint32_t)(j - 64 - 1));
    }
    else
    {
        const std::uint64_t lo3 = lo + lo;
        const std::uint64_t mid3 = mid + mid + (lo3 < lo);
        const std::uint64_t hi3 = hi + hi + (mid3 < mid);
        const std::uint64_t lo4 = lo3 - mul[0];
        const std::uint64_t mid4 = mid3 - mul[1] - (lo4 > lo3);
        const std::uint64_t hi4 = hi3 - (mid4 > mid3);
        *vm = shiftright128(mid4, hi4, (std::uint32_t)(j - 64));
    }

    return shiftright128(mid, hi, (std::uint32_t)(j - 64 - 1));
}

#endif // BOOST_JSON_RYU_HAS_64_BIT_INTRINSICS

inline
std::uint32_t
decimalLength17(
    const std::uint64_t v)
{
    // This is slightly faster than a loop.
    // The average output length is 16.38 digits, so we check high-to-low.
    // Function precondition: v is not an 18, 19, or 20-digit number.
    // (17 digits are sufficient for round-tripping.)
    BOOST_ASSERT(v < 100000000000000000L);
    if (v >= 10000000000000000L) { return 17; }
    if (v >= 1000000000000000L) { return 16; }
    if (v >= 100000000000000L) { return 15; }
    if (v >= 10000000000000L) { return 14; }
    if (v >= 1000000000000L) { return 13; }
    if (v >= 100000000000L) { return 12; }
    if (v >= 10000000000L) { return 11; }
    if (v >= 1000000000L) { return 10; }
    if (v >= 100000000L) { return 9; }
    if (v >= 10000000L) { return 8; }
    if (v >= 1000000L) { return 7; }
    if (v >= 100000L) { return 6; }
    if (v >= 10000L) { return 5; }
    if (v >= 1000L) { return 4; }
    if (v >= 100L) { return 3; }
    if (v >= 10L) { return 2; }
    return 1;
}

// A floating decimal representing m * 10^e.
struct floating_decimal_64
{
    std::uint64_t mantissa;
    // Decimal exponent's range is -324 to 308
    // inclusive, and can fit in a short if needed.
    std::int32_t exponent;
};

inline
floating_decimal_64
d2d(
    const std::uint64_t ieeeMantissa,
    const std::uint32_t ieeeExponent)
{
    std::int32_t e2;
    std::uint64_t m2;
    if (ieeeExponent == 0)
    {
        // We subtract 2 so that the bounds computation has 2 additional bits.
        e2 = 1 - DOUBLE_BIAS - DOUBLE_MANTISSA_BITS - 2;
        m2 = ieeeMantissa;
    }
    else
    {
        e2 = (std::int32_t)ieeeExponent - DOUBLE_BIAS - DOUBLE_MANTISSA_BITS - 2;
        m2 = (1ull << DOUBLE_MANTISSA_BITS) | ieeeMantissa;
    }
    const bool even = (m2 & 1) == 0;
    const bool acceptBounds = even;

#ifdef RYU_DEBUG
    printf("-> %" PRIu64 " * 2^%d\n", m2, e2 + 2);
#endif

    // Step 2: Determine the interval of valid decimal representations.
    const std::uint64_t mv = 4 * m2;
    // Implicit bool -> int conversion. True is 1, false is 0.
    const std::uint32_t mmShift = ieeeMantissa != 0 || ieeeExponent <= 1;
    // We would compute mp and mm like this:
    // uint64_t mp = 4 * m2 + 2;
    // uint64_t mm = mv - 1 - mmShift;

    // Step 3: Convert to a decimal power base using 128-bit arithmetic.
    std::uint64_t vr, vp, vm;
    std::int32_t e10;
    bool vmIsTrailingZeros = false;
    bool vrIsTrailingZeros = false;
    if (e2 >= 0) {
        // I tried special-casing q == 0, but there was no effect on performance.
        // This expression is slightly faster than max(0, log10Pow2(e2) - 1).
        const std::uint32_t q = log10Pow2(e2) - (e2 > 3);
        e10 = (std::int32_t)q;
        const std::int32_t k = DOUBLE_POW5_INV_BITCOUNT + pow5bits((int32_t)q) - 1;
        const std::int32_t i = -e2 + (std::int32_t)q + k;
#if defined(BOOST_JSON_RYU_OPTIMIZE_SIZE)
        uint64_t pow5[2];
        double_computeInvPow5(q, pow5);
        vr = mulShiftAll(m2, pow5, i, &vp, &vm, mmShift);
#else
        vr = mulShiftAll(m2, DOUBLE_POW5_INV_SPLIT()[q], i, &vp, &vm, mmShift);
#endif
#ifdef RYU_DEBUG
        printf("%" PRIu64 " * 2^%d / 10^%u\n", mv, e2, q);
        printf("V+=%" PRIu64 "\nV =%" PRIu64 "\nV-=%" PRIu64 "\n", vp, vr, vm);
#endif
        if (q <= 21)
        {
            // This should use q <= 22, but I think 21 is also safe. Smaller values
            // may still be safe, but it's more difficult to reason about them.
            // Only one of mp, mv, and mm can be a multiple of 5, if any.
            const std::uint32_t mvMod5 = ((std::uint32_t)mv) - 5 * ((std::uint32_t)div5(mv));
            if (mvMod5 == 0)
            {
                vrIsTrailingZeros = multipleOfPowerOf5(mv, q);
            }
            else if (acceptBounds)
            {
                // Same as min(e2 + (~mm & 1), pow5Factor(mm)) >= q
                // <=> e2 + (~mm & 1) >= q && pow5Factor(mm) >= q
                // <=> true && pow5Factor(mm) >= q, since e2 >= q.
                vmIsTrailingZeros = multipleOfPowerOf5(mv - 1 - mmShift, q);
            }
            else
            {
                // Same as min(e2 + 1, pow5Factor(mp)) >= q.
                vp -= multipleOfPowerOf5(mv + 2, q);
            }
        }
    }
    else
    {
        // This expression is slightly faster than max(0, log10Pow5(-e2) - 1).
        const std::uint32_t q = log10Pow5(-e2) - (-e2 > 1);
        e10 = (std::int32_t)q + e2;
        const std::int32_t i = -e2 - (std::int32_t)q;
        const std::int32_t k = pow5bits(i) - DOUBLE_POW5_BITCOUNT;
        const std::int32_t j = (std::int32_t)q - k;
#if defined(BOOST_JSON_RYU_OPTIMIZE_SIZE)
        std::uint64_t pow5[2];
        double_computePow5(i, pow5);
        vr = mulShiftAll(m2, pow5, j, &vp, &vm, mmShift);
#else
        vr = mulShiftAll(m2, DOUBLE_POW5_SPLIT()[i], j, &vp, &vm, mmShift);
#endif
#ifdef RYU_DEBUG
        printf("%" PRIu64 " * 5^%d / 10^%u\n", mv, -e2, q);
        printf("%u %d %d %d\n", q, i, k, j);
        printf("V+=%" PRIu64 "\nV =%" PRIu64 "\nV-=%" PRIu64 "\n", vp, vr, vm);
#endif
        if (q <= 1)
        {
            // {vr,vp,vm} is trailing zeros if {mv,mp,mm} has at least q trailing 0 bits.
            // mv = 4 * m2, so it always has at least two trailing 0 bits.
            vrIsTrailingZeros = true;
            if (acceptBounds)
            {
                // mm = mv - 1 - mmShift, so it has 1 trailing 0 bit iff mmShift == 1.
                vmIsTrailingZeros = mmShift == 1;
            }
            else
            {
                // mp = mv + 2, so it always has at least one trailing 0 bit.
                --vp;
            }
        }
        else if (q < 63)
        {
            // TODO(ulfjack): Use a tighter bound here.
            // We want to know if the full product has at least q trailing zeros.
            // We need to compute min(p2(mv), p5(mv) - e2) >= q
            // <=> p2(mv) >= q && p5(mv) - e2 >= q
            // <=> p2(mv) >= q (because -e2 >= q)
            vrIsTrailingZeros = multipleOfPowerOf2(mv, q);
#ifdef RYU_DEBUG
            printf("vr is trailing zeros=%s\n", vrIsTrailingZeros ? "true" : "false");
#endif
        }
    }
#ifdef RYU_DEBUG
    printf("e10=%d\n", e10);
    printf("V+=%" PRIu64 "\nV =%" PRIu64 "\nV-=%" PRIu64 "\n", vp, vr, vm);
    printf("vm is trailing zeros=%s\n", vmIsTrailingZeros ? "true" : "false");
    printf("vr is trailing zeros=%s\n", vrIsTrailingZeros ? "true" : "false");
#endif

    // Step 4: Find the shortest decimal representation in the interval of valid representations.
    std::int32_t removed = 0;
    std::uint8_t lastRemovedDigit = 0;
    std::uint64_t output;
    // On average, we remove ~2 digits.
    if (vmIsTrailingZeros || vrIsTrailingZeros)
    {
        // General case, which happens rarely (~0.7%).
        for (;;)
        {
            const std::uint64_t vpDiv10 = div10(vp);
            const std::uint64_t vmDiv10 = div10(vm);
            if (vpDiv10 <= vmDiv10)
                break;
            const std::uint32_t vmMod10 = ((std::uint32_t)vm) - 10 * ((std::uint32_t)vmDiv10);
            const std::uint64_t vrDiv10 = div10(vr);
            const std::uint32_t vrMod10 = ((std::uint32_t)vr) - 10 * ((std::uint32_t)vrDiv10);
            vmIsTrailingZeros &= vmMod10 == 0;
            vrIsTrailingZeros &= lastRemovedDigit == 0;
            lastRemovedDigit = (uint8_t)vrMod10;
            vr = vrDiv10;
            vp = vpDiv10;
            vm = vmDiv10;
            ++removed;
        }
#ifdef RYU_DEBUG
        printf("V+=%" PRIu64 "\nV =%" PRIu64 "\nV-=%" PRIu64 "\n", vp, vr, vm);
        printf("d-10=%s\n", vmIsTrailingZeros ? "true" : "false");
#endif
        if (vmIsTrailingZeros)
        {
            for (;;)
            {
                const std::uint64_t vmDiv10 = div10(vm);
                const std::uint32_t vmMod10 = ((std::uint32_t)vm) - 10 * ((std::uint32_t)vmDiv10);
                if (vmMod10 != 0)
                    break;
                const std::uint64_t vpDiv10 = div10(vp);
                const std::uint64_t vrDiv10 = div10(vr);
                const std::uint32_t vrMod10 = ((std::uint32_t)vr) - 10 * ((std::uint32_t)vrDiv10);
                vrIsTrailingZeros &= lastRemovedDigit == 0;
                lastRemovedDigit = (uint8_t)vrMod10;
                vr = vrDiv10;
                vp = vpDiv10;
                vm = vmDiv10;
                ++removed;
            }
        }
#ifdef RYU_DEBUG
        printf("%" PRIu64 " %d\n", vr, lastRemovedDigit);
        printf("vr is trailing zeros=%s\n", vrIsTrailingZeros ? "true" : "false");
#endif
        if (vrIsTrailingZeros && lastRemovedDigit == 5 && vr % 2 == 0)
        {
            // Round even if the exact number is .....50..0.
            lastRemovedDigit = 4;
        }
        // We need to take vr + 1 if vr is outside bounds or we need to round up.
        output = vr + ((vr == vm && (!acceptBounds || !vmIsTrailingZeros)) || lastRemovedDigit >= 5);
    }
    else
    {
        // Specialized for the common case (~99.3%). Percentages below are relative to this.
        bool roundUp = false;
        const std::uint64_t vpDiv100 = div100(vp);
        const std::uint64_t vmDiv100 = div100(vm);
        if (vpDiv100 > vmDiv100)
        {
            // Optimization: remove two digits at a time (~86.2%).
            const std::uint64_t vrDiv100 = div100(vr);
            const std::uint32_t vrMod100 = ((std::uint32_t)vr) - 100 * ((std::uint32_t)vrDiv100);
            roundUp = vrMod100 >= 50;
            vr = vrDiv100;
            vp = vpDiv100;
            vm = vmDiv100;
            removed += 2;
        }
        // Loop iterations below (approximately), without optimization above:
        // 0: 0.03%, 1: 13.8%, 2: 70.6%, 3: 14.0%, 4: 1.40%, 5: 0.14%, 6+: 0.02%
        // Loop iterations below (approximately), with optimization above:
        // 0: 70.6%, 1: 27.8%, 2: 1.40%, 3: 0.14%, 4+: 0.02%
        for (;;)
        {
            const std::uint64_t vpDiv10 = div10(vp);
            const std::uint64_t vmDiv10 = div10(vm);
            if (vpDiv10 <= vmDiv10)
                break;
            const std::uint64_t vrDiv10 = div10(vr);
            const std::uint32_t vrMod10 = ((std::uint32_t)vr) - 10 * ((std::uint32_t)vrDiv10);
            roundUp = vrMod10 >= 5;
            vr = vrDiv10;
            vp = vpDiv10;
            vm = vmDiv10;
            ++removed;
        }
#ifdef RYU_DEBUG
        printf("%" PRIu64 " roundUp=%s\n", vr, roundUp ? "true" : "false");
        printf("vr is trailing zeros=%s\n", vrIsTrailingZeros ? "true" : "false");
#endif
        // We need to take vr + 1 if vr is outside bounds or we need to round up.
        output = vr + (vr == vm || roundUp);
    }
    const std::int32_t exp = e10 + removed;

#ifdef RYU_DEBUG
    printf("V+=%" PRIu64 "\nV =%" PRIu64 "\nV-=%" PRIu64 "\n", vp, vr, vm);
    printf("O=%" PRIu64 "\n", output);
    printf("EXP=%d\n", exp);
#endif

    floating_decimal_64 fd;
    fd.exponent = exp;
    fd.mantissa = output;
    return fd;
}

inline
int
to_chars(
    const floating_decimal_64 v,
    const bool sign,
    char* const result)
{
    // Step 5: Print the decimal representation.
    int index = 0;
    if (sign)
        result[index++] = '-';

    std::uint64_t output = v.mantissa;
    std::uint32_t const olength = decimalLength17(output);

#ifdef RYU_DEBUG
    printf("DIGITS=%" PRIu64 "\n", v.mantissa);
    printf("OLEN=%u\n", olength);
    printf("EXP=%u\n", v.exponent + olength);
#endif

    // Print the decimal digits.
    // The following code is equivalent to:
    // for (uint32_t i = 0; i < olength - 1; ++i) {
    //   const uint32_t c = output % 10; output /= 10;
    //   result[index + olength - i] = (char) ('0' + c);
    // }
    // result[index] = '0' + output % 10;

    std::uint32_t i = 0;
    // We prefer 32-bit operations, even on 64-bit platforms.
    // We have at most 17 digits, and uint32_t can store 9 digits.
    // If output doesn't fit into uint32_t, we cut off 8 digits,
    // so the rest will fit into uint32_t.
    if ((output >> 32) != 0)
    {
        // Expensive 64-bit division.
        std::uint64_t const q = div1e8(output);
        std::uint32_t output2 = ((std::uint32_t)output) - 100000000 * ((std::uint32_t)q);
        output = q;

        const std::uint32_t c = output2 % 10000;
        output2 /= 10000;
        const std::uint32_t d = output2 % 10000;
        const std::uint32_t c0 = (c % 100) << 1;
        const std::uint32_t c1 = (c / 100) << 1;
        const std::uint32_t d0 = (d % 100) << 1;
        const std::uint32_t d1 = (d / 100) << 1;
        std::memcpy(result + index + olength - i - 1, DIGIT_TABLE() + c0, 2);
        std::memcpy(result + index + olength - i - 3, DIGIT_TABLE() + c1, 2);
        std::memcpy(result + index + olength - i - 5, DIGIT_TABLE() + d0, 2);
        std::memcpy(result + index + olength - i - 7, DIGIT_TABLE() + d1, 2);
        i += 8;
    }
    uint32_t output2 = (std::uint32_t)output;
    while (output2 >= 10000)
    {
#ifdef __clang__ // https://bugs.llvm.org/show_bug.cgi?id=38217
        const uint32_t c = output2 - 10000 * (output2 / 10000);
#else
        const uint32_t c = output2 % 10000;
#endif
        output2 /= 10000;
        const uint32_t c0 = (c % 100) << 1;
        const uint32_t c1 = (c / 100) << 1;
        memcpy(result + index + olength - i - 1, DIGIT_TABLE() + c0, 2);
        memcpy(result + index + olength - i - 3, DIGIT_TABLE() + c1, 2);
        i += 4;
    }
    if (output2 >= 100) {
        const uint32_t c = (output2 % 100) << 1;
        output2 /= 100;
        memcpy(result + index + olength - i - 1, DIGIT_TABLE() + c, 2);
        i += 2;
    }
    if (output2 >= 10) {
        const uint32_t c = output2 << 1;
        // We can't use memcpy here: the decimal dot goes between these two digits.
        result[index + olength - i] = DIGIT_TABLE()[c + 1];
        result[index] = DIGIT_TABLE()[c];
    }
    else {
        result[index] = (char)('0' + output2);
    }

    // Print decimal point if needed.
    if (olength > 1) {
        result[index + 1] = '.';
        index += olength + 1;
    }
    else {
        ++index;
    }

    // Print the exponent.
    result[index++] = 'E';
    int32_t exp = v.exponent + (int32_t)olength - 1;
    if (exp < 0) {
        result[index++] = '-';
        exp = -exp;
    }

    if (exp >= 100) {
        const int32_t c = exp % 10;
        memcpy(result + index, DIGIT_TABLE() + 2 * (exp / 10), 2);
        result[index + 2] = (char)('0' + c);
        index += 3;
    }
    else if (exp >= 10) {
        memcpy(result + index, DIGIT_TABLE() + 2 * exp, 2);
        index += 2;
    }
    else {
        result[index++] = (char)('0' + exp);
    }

    return index;
}

static inline bool d2d_small_int(const uint64_t ieeeMantissa, const uint32_t ieeeExponent,
  floating_decimal_64* const v) {
  const uint64_t m2 = (1ull << DOUBLE_MANTISSA_BITS) | ieeeMantissa;
  const int32_t e2 = (int32_t) ieeeExponent - DOUBLE_BIAS - DOUBLE_MANTISSA_BITS;

  if (e2 > 0) {
    // f = m2 * 2^e2 >= 2^53 is an integer.
    // Ignore this case for now.
    return false;
  }

  if (e2 < -52) {
    // f < 1.
    return false;
  }

  // Since 2^52 <= m2 < 2^53 and 0 <= -e2 <= 52: 1 <= f = m2 / 2^-e2 < 2^53.
  // Test if the lower -e2 bits of the significand are 0, i.e. whether the fraction is 0.
  const uint64_t mask = (1ull << -e2) - 1;
  const uint64_t fraction = m2 & mask;
  if (fraction != 0) {
    return false;
  }

  // f is an integer in the range [1, 2^53).
  // Note: mantissa might contain trailing (decimal) 0's.
  // Note: since 2^53 < 10^16, there is no need to adjust decimalLength17().
  v->mantissa = m2 >> -e2;
  v->exponent = 0;
  return true;
}

} // detail

int
d2s_buffered_n(
    double f,
    char* result) noexcept
{
    using namespace detail;
    // Step 1: Decode the floating-point number, and unify normalized and subnormal cases.
    std::uint64_t const bits = double_to_bits(f);

#ifdef RYU_DEBUG
    printf("IN=");
    for (std::int32_t bit = 63; bit >= 0; --bit) {
        printf("%d", (int)((bits >> bit) & 1));
    }
    printf("\n");
#endif

    // Decode bits into sign, mantissa, and exponent.
    const bool ieeeSign = ((bits >> (DOUBLE_MANTISSA_BITS + DOUBLE_EXPONENT_BITS)) & 1) != 0;
    const std::uint64_t ieeeMantissa = bits & ((1ull << DOUBLE_MANTISSA_BITS) - 1);
    const std::uint32_t ieeeExponent = (std::uint32_t)((bits >> DOUBLE_MANTISSA_BITS) & ((1u << DOUBLE_EXPONENT_BITS) - 1));
    // Case distinction; exit early for the easy cases.
    if (ieeeExponent == ((1u << DOUBLE_EXPONENT_BITS) - 1u) || (ieeeExponent == 0 && ieeeMantissa == 0)) {
        return copy_special_str(result, ieeeSign, ieeeExponent != 0, ieeeMantissa != 0);
    }

    floating_decimal_64 v;
    const bool isSmallInt = d2d_small_int(ieeeMantissa, ieeeExponent, &v);
    if (isSmallInt) {
        // For small integers in the range [1, 2^53), v.mantissa might contain trailing (decimal) zeros.
        // For scientific notation we need to move these zeros into the exponent.
        // (This is not needed for fixed-point notation, so it might be beneficial to trim
        // trailing zeros in to_chars only if needed - once fixed-point notation output is implemented.)
        for (;;) {
            std::uint64_t const q = div10(v.mantissa);
            std::uint32_t const r = ((std::uint32_t) v.mantissa) - 10 * ((std::uint32_t) q);
            if (r != 0)
                break;
            v.mantissa = q;
            ++v.exponent;
        }
    }
    else {
        v = d2d(ieeeMantissa, ieeeExponent);
    }

    return to_chars(v, ieeeSign, result);
}

} // ryu

} // detail
BOOST_JSON_NS_END

#endif
