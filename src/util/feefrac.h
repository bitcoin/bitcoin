// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_UTIL_FEEFRAC_H
#define BITCOIN_UTIL_FEEFRAC_H

#include <util/check.h>
#include <util/overflow.h>

#include <compare>
#include <cstdint>
#include <span>
#include <utility>

/** Data structure storing a fee and size.
 *
 * The size of a FeeFrac cannot be zero unless the fee is also zero.
 */
struct FeeFrac
{
    /** Helper function for 32*64 signed multiplication, returning an unspecified but totally
     *  ordered type. This is a fallback version, separate so it can be tested on platforms where
     *  it isn't actually needed. */
    static inline std::pair<int64_t, uint32_t> MulFallback(int64_t a, int32_t b) noexcept
    {
        int64_t low = int64_t{static_cast<uint32_t>(a)} * b;
        int64_t high = (a >> 32) * b;
        return {high + (low >> 32), static_cast<uint32_t>(low)};
    }

    /** Helper function for 96/32 signed division, rounding towards negative infinity (if
     *  round_down) or positive infinity (if !round_down). This is a fallback version, separate so
     *  that it can be tested on platforms where it isn't actually needed.
     *
     * The exact behavior with negative n does not really matter, but this implementation chooses
     * to be consistent for testability reasons.
     *
     * The result must fit in an int64_t, and d must be strictly positive. */
    static inline int64_t DivFallback(std::pair<int64_t, uint32_t> n, int32_t d, bool round_down) noexcept
    {
        Assume(d > 0);
        // Compute quot_high = n.first / d, so the result becomes
        // (n.second + (n.first - quot_high * d) * 2**32) / d + (quot_high * 2**32), or
        // (n.second + (n.first % d) * 2**32) / d + (quot_high * 2**32).
        int64_t quot_high = n.first / d;
        // Evaluate the parenthesized expression above, so the result becomes
        // n_low / d + (quot_high * 2**32)
        int64_t n_low = ((n.first % d) << 32) + n.second;
        // Evaluate the division so the result becomes quot_low + quot_high * 2**32. It is possible
        // that the / operator here rounds in the wrong direction (if n_low is not a multiple of
        // size, and is (if round_down) negative, or (if !round_down) positive). If so, make a
        // correction.
        int64_t quot_low = n_low / d;
        int32_t mod_low = n_low % d;
        quot_low += (mod_low > 0) - (mod_low && round_down);
        // Combine and return the result
        return (quot_high << 32) + quot_low;
    }

#ifdef __SIZEOF_INT128__
    /** Helper function for 32*64 signed multiplication, returning an unspecified but totally
     *  ordered type. This is a version relying on __int128. */
    static inline __int128 Mul(int64_t a, int32_t b) noexcept
    {
        return __int128{a} * b;
    }

    /** Helper function for 96/32 signed division, rounding towards negative infinity (if
     *  round_down), or towards positive infinity (if !round_down). This is a
     *  version relying on __int128.
     *
     * The result must fit in an int64_t, and d must be strictly positive. */
    static inline int64_t Div(__int128 n, int32_t d, bool round_down) noexcept
    {
        Assume(d > 0);
        // Compute the division.
        int64_t quot = n / d;
        int32_t mod = n % d;
        // Correct result if the / operator above rounded in the wrong direction.
        return quot + ((mod > 0) - (mod && round_down));
    }
#else
    static constexpr auto Mul = MulFallback;
    static constexpr auto Div = DivFallback;
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

    /** Swap two FeeFracs. */
    friend inline void swap(FeeFrac& a, FeeFrac& b) noexcept
    {
        std::swap(a.fee, b.fee);
        std::swap(a.size, b.size);
    }

    /** Compute the fee for a given size `at_size` using this object's feerate.
     *
     * This effectively corresponds to evaluating (this->fee * at_size) / this->size, with the
     * result rounded towards negative infinity (if RoundDown) or towards positive infinity
     * (if !RoundDown).
     *
     * Requires this->size > 0, at_size >= 0, and that the correct result fits in a int64_t. This
     * is guaranteed to be the case when 0 <= at_size <= this->size.
     */
    template<bool RoundDown>
    int64_t EvaluateFee(int32_t at_size) const noexcept
    {
        Assume(size > 0);
        Assume(at_size >= 0);
        if (fee >= 0 && fee < 0x200000000) [[likely]] {
            // Common case where (this->fee * at_size) is guaranteed to fit in a uint64_t.
            if constexpr (RoundDown) {
                return (uint64_t(fee) * at_size) / uint32_t(size);
            } else {
                return CeilDiv(uint64_t(fee) * at_size, uint32_t(size));
            }
        } else {
            // Otherwise, use Mul and Div.
            return Div(Mul(fee, at_size), size, RoundDown);
        }
    }

public:
    /** Compute the fee for a given size `at_size` using this object's feerate, rounding down. */
    int64_t EvaluateFeeDown(int32_t at_size) const noexcept { return EvaluateFee<true>(at_size); }
    /** Compute the fee for a given size `at_size` using this object's feerate, rounding up. */
    int64_t EvaluateFeeUp(int32_t at_size) const noexcept { return EvaluateFee<false>(at_size); }
};

/** Compare the feerate diagrams implied by the provided sorted chunks data.
 *
 * The implied diagram for each starts at (0, 0), then contains for each chunk the cumulative fee
 * and size up to that chunk, and then extends infinitely to the right with a horizontal line.
 *
 * The caller must guarantee that the sum of the FeeFracs in either of the chunks' data set do not
 * overflow (so sum fees < 2^63, and sum sizes < 2^31).
 */
std::partial_ordering CompareChunks(std::span<const FeeFrac> chunks0, std::span<const FeeFrac> chunks1);

/** Tagged wrapper around FeeFrac to avoid unit confusion. */
template<typename Tag>
struct FeePerUnit : public FeeFrac
{
    // Inherit FeeFrac constructors.
    using FeeFrac::FeeFrac;

    /** Convert a FeeFrac to a FeePerUnit. */
    static FeePerUnit FromFeeFrac(const FeeFrac& feefrac) noexcept
    {
        return {feefrac.fee, feefrac.size};
    }
};

// FeePerUnit instance for satoshi / vbyte.
struct VSizeTag {};
using FeePerVSize = FeePerUnit<VSizeTag>;

// FeePerUnit instance for satoshi / WU.
struct WeightTag {};
using FeePerWeight = FeePerUnit<WeightTag>;

/** Wrapper around FeeFrac & derived types, which adds a feerate-based ordering which treats
 *  equal-feerate but distinct-size FeeFracs as equals.
 *
 *  This is not included inside FeeFrac itself, because it is not a total ordering (as would be
 *  expected for built-in comparison operators).
 */
template<std::derived_from<FeeFrac> T>
class ByRatio
{
    const T& m_feefrac;

public:
    constexpr ByRatio(const T& feefrac) noexcept : m_feefrac{feefrac} {}

    friend bool operator==(const ByRatio& a, const ByRatio& b) noexcept
    {
        auto cross_a = T::Mul(a.m_feefrac.fee, b.m_feefrac.size);
        auto cross_b = T::Mul(b.m_feefrac.fee, a.m_feefrac.size);
        return cross_a == cross_b;
    }

    // Note that we can use std::strong_ordering here, because even though FeeFrac{1,2} and
    // FeeFrac{2,4} are distinct as FeeFracs, they are indistinguishable from ByRatio's perspective
    // (operator== also treats them as equal).
    friend std::strong_ordering operator<=>(const ByRatio& a, const ByRatio& b) noexcept
    {
        auto cross_a = T::Mul(a.m_feefrac.fee, b.m_feefrac.size);
        auto cross_b = T::Mul(b.m_feefrac.fee, a.m_feefrac.size);
        return cross_a <=> cross_b;
    }

    // Specialized versions for efficiency. GCC 15+ and Clang 11+ produce operator<=>-derived
    // versions that are equally efficient as this at -O2, but earlier versions do not.
    friend bool operator<(const ByRatio& a, const ByRatio& b) noexcept
    {
        auto cross_a = T::Mul(a.m_feefrac.fee, b.m_feefrac.size);
        auto cross_b = T::Mul(b.m_feefrac.fee, a.m_feefrac.size);
        return cross_a < cross_b;
    }
    friend bool operator>(const ByRatio& a, const ByRatio& b) noexcept
    {
        auto cross_a = T::Mul(a.m_feefrac.fee, b.m_feefrac.size);
        auto cross_b = T::Mul(b.m_feefrac.fee, a.m_feefrac.size);
        return cross_a > cross_b;
    }
    friend bool operator<=(const ByRatio& a, const ByRatio& b) noexcept
    {
        auto cross_a = T::Mul(a.m_feefrac.fee, b.m_feefrac.size);
        auto cross_b = T::Mul(b.m_feefrac.fee, a.m_feefrac.size);
        return cross_a <= cross_b;
    }
    friend bool operator>=(const ByRatio& a, const ByRatio& b) noexcept
    {
        auto cross_a = T::Mul(a.m_feefrac.fee, b.m_feefrac.size);
        auto cross_b = T::Mul(b.m_feefrac.fee, a.m_feefrac.size);
        return cross_a >= cross_b;
    }
};

/** Wrapper around FeeFrac & derived types, which adds a total ordering which first sorts by feerate
 *  and then by reversed size (i.e., larger sizes come first).
 *
 *  This is not included inside FeeFrac itself, because it is not the most natural behavior, so it
 *  is better to make code using it invoke this explicitly.
 *
 *  The empty FeeFrac (fee and size both 0) sorts last. So for example, the following FeeFracs are
 *  in sorted order:
 *
 *   - fee=0 size=1 (feerate 0)
 *   - fee=1 size=2 (feerate 0.5)
 *   - fee=2 size=3 (feerate 0.667...)
 *   - fee=2 size=2 (feerate 1)
 *   - fee=1 size=1 (feerate 1)
 *   - fee=3 size=2 (feerate 1.5)
 *   - fee=2 size=1 (feerate 2)
 *   - fee=0 size=0 (undefined feerate)
 */
template<std::derived_from<FeeFrac> T>
class ByRatioNegSize
{
    const T& m_feefrac;

public:
    constexpr ByRatioNegSize(const T& feefrac) noexcept : m_feefrac{feefrac} {}

    friend bool operator==(const ByRatioNegSize& a, const ByRatioNegSize& b) noexcept
    {
        return a.m_feefrac == b.m_feefrac;
    }

    friend std::strong_ordering operator<=>(const ByRatioNegSize& a, const ByRatioNegSize& b) noexcept
    {
        auto cross_a = T::Mul(a.m_feefrac.fee, b.m_feefrac.size);
        auto cross_b = T::Mul(b.m_feefrac.fee, a.m_feefrac.size);
        auto cmp = cross_a <=> cross_b;
        if (cmp != 0) return cmp;
        return b.m_feefrac.size <=> a.m_feefrac.size;
    }

    // Support conversion back to underlying FeeFrac, which allows using std::max().
    operator const T&() const noexcept { return m_feefrac; }
};

#endif // BITCOIN_UTIL_FEEFRAC_H
