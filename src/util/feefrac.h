// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_UTIL_FEEFRAC_H
#define BITCOIN_UTIL_FEEFRAC_H

#include <stdint.h>
#include <compare>
#include <vector>
#include <span.h>
#include <util/check.h>

/** Data structure storing a fee and size, ordered by increasing fee/size.
 *
 * The size of a FeeFrac cannot be zero unless the fee is also zero.
 *
 * FeeFracs have a total ordering, first by increasing feerate (ratio of fee over size), and then
 * by decreasing size. The empty FeeFrac (fee and size both 0) sorts last. So for example, the
 * following FeeFracs are in sorted order:
 *
 * - fee=0 size=1 (feerate 0)
 * - fee=1 size=2 (feerate 0.5)
 * - fee=2 size=3 (feerate 0.667...)
 * - fee=2 size=2 (feerate 1)
 * - fee=1 size=1 (feerate 1)
 * - fee=3 size=2 (feerate 1.5)
 * - fee=2 size=1 (feerate 2)
 * - fee=0 size=0 (undefined feerate)
 *
 * A FeeFrac is considered "better" if it sorts after another, by this ordering. All standard
 * comparison operators (<=>, ==, !=, >, <, >=, <=) respect this ordering.
 *
 * The FeeRateCompare, and >> and << operators only compare feerate and treat equal feerate but
 * different size as equivalent. The empty FeeFrac is neither lower or higher in feerate than any
 * other.
 */
struct FeeFrac
{
    /** Fallback version for Mul (see below).
     *
     * Separate to permit testing on platforms where it isn't actually needed.
     */
    static inline std::pair<int64_t, uint32_t> MulFallback(int64_t a, int32_t b) noexcept
    {
        // Otherwise, emulate 96-bit multiplication using two 64-bit multiplies.
        int64_t low = int64_t{static_cast<uint32_t>(a)} * b;
        int64_t high = (a >> 32) * b;
        return {high + (low >> 32), static_cast<uint32_t>(low)};
    }

    // Compute a * b, returning an unspecified but totally ordered type.
#ifdef __SIZEOF_INT128__
    static inline __int128 Mul(int64_t a, int32_t b) noexcept
    {
        // If __int128 is available, use 128-bit wide multiply.
        return __int128{a} * b;
    }
#else
    static constexpr auto Mul = MulFallback;
#endif

    int64_t fee;
    int32_t size;

    /** Construct an IsEmpty() FeeFrac. */
    constexpr inline FeeFrac() noexcept : fee{0}, size{0} {}

    /** Construct a FeeFrac with specified fee and size. */
    constexpr inline FeeFrac(int64_t f, int32_t s) noexcept : fee{f}, size{s} {}

    constexpr inline FeeFrac(const FeeFrac&) noexcept = default;
    constexpr inline FeeFrac& operator=(const FeeFrac&) noexcept = default;

    /** Check if this is empty (size and fee are 0). */
    bool inline IsEmpty() const noexcept {
        return size == 0;
    }

    /** Add fee and size of another FeeFrac to this one. */
    void inline operator+=(const FeeFrac& other) noexcept
    {
        fee += other.fee;
        size += other.size;
    }

    /** Subtract fee and size of another FeeFrac from this one. */
    void inline operator-=(const FeeFrac& other) noexcept
    {
        fee -= other.fee;
        size -= other.size;
    }

    /** Sum fee and size. */
    friend inline FeeFrac operator+(const FeeFrac& a, const FeeFrac& b) noexcept
    {
        return {a.fee + b.fee, a.size + b.size};
    }

    /** Subtract both fee and size. */
    friend inline FeeFrac operator-(const FeeFrac& a, const FeeFrac& b) noexcept
    {
        return {a.fee - b.fee, a.size - b.size};
    }

    /** Check if two FeeFrac objects are equal (both same fee and same size). */
    friend inline bool operator==(const FeeFrac& a, const FeeFrac& b) noexcept
    {
        return a.fee == b.fee && a.size == b.size;
    }

    /** Compare two FeeFracs just by feerate. */
    friend inline std::weak_ordering FeeRateCompare(const FeeFrac& a, const FeeFrac& b) noexcept
    {
        auto cross_a = Mul(a.fee, b.size), cross_b = Mul(b.fee, a.size);
        return cross_a <=> cross_b;
    }

    /** Check if a FeeFrac object has strictly lower feerate than another. */
    friend inline bool operator<<(const FeeFrac& a, const FeeFrac& b) noexcept
    {
        auto cross_a = Mul(a.fee, b.size), cross_b = Mul(b.fee, a.size);
        return cross_a < cross_b;
    }

    /** Check if a FeeFrac object has strictly higher feerate than another. */
    friend inline bool operator>>(const FeeFrac& a, const FeeFrac& b) noexcept
    {
        auto cross_a = Mul(a.fee, b.size), cross_b = Mul(b.fee, a.size);
        return cross_a > cross_b;
    }

    /** Compare two FeeFracs. <, >, <=, and >= are auto-generated from this. */
    friend inline std::strong_ordering operator<=>(const FeeFrac& a, const FeeFrac& b) noexcept
    {
        auto cross_a = Mul(a.fee, b.size), cross_b = Mul(b.fee, a.size);
        if (cross_a == cross_b) return b.size <=> a.size;
        return cross_a <=> cross_b;
    }

    /** Swap two FeeFracs. */
    friend inline void swap(FeeFrac& a, FeeFrac& b) noexcept
    {
        std::swap(a.fee, b.fee);
        std::swap(a.size, b.size);
    }
};

/** Compare the feerate diagrams implied by the provided sorted chunks data.
 *
 * The implied diagram for each starts at (0, 0), then contains for each chunk the cumulative fee
 * and size up to that chunk, and then extends infinitely to the right with a horizontal line.
 *
 * The caller must guarantee that the sum of the FeeFracs in either of the chunks' data set do not
 * overflow (so sum fees < 2^63, and sum sizes < 2^31).
 */
std::partial_ordering CompareChunks(Span<const FeeFrac> chunks0, Span<const FeeFrac> chunks1);

#endif // BITCOIN_UTIL_FEEFRAC_H
