// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_UTIL_FEEFRAC_H
#define BITCOIN_UTIL_FEEFRAC_H

#include <assert.h>
#include <stdint.h>
#include <algorithm>
#include <compare>
#include <vector>

namespace {

inline std::pair<int64_t, uint64_t> Mul128(int64_t a, int64_t b)
{
#ifdef __SIZEOF_INT128__
    __int128 ret = (__int128)a * b;
    return {ret >> 64, ret};
#else
    uint64_t ll = (uint64_t)(uint32_t)a * (uint32_t)b;
    int64_t lh = (uint32_t)a * (b >> 32);
    int64_t hl = (a >> 32) * (uint32_t)b;
    int64_t hh = (a >> 32) * (b >> 32);
    uint64_t mid34 = (ll >> 32) + (uint32_t)lh + (uint32_t)hl;
    int64_t hi = hh + (lh >> 32) + (hl >> 32) + (mid34 >> 32);
    uint64_t lo = (mid34 << 32) + (uint32_t)ll;
    return {hi, lo};
#endif
}

} // namespace

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
 * comparison operators (==, !=, >, <, >=, <=, <=>) respect this ordering.
 *
 * The >> and << operators only compare feerate and treat equal feerate but different size as
 * equivalent. The empty FeeFrac is neither lower or higher in feerate than any other.
 * The FeeRateCompare function returns the three-way comparison for this order.
 */
struct FeeFrac
{
    /** Fee. */
    int64_t fee;
    /** Size. */
    int32_t size;

    /** Construct an IsEmpty() FeeFrac. */
    inline FeeFrac() noexcept : fee{0}, size{0} {}

    /** Construct a FeeFrac with specified fee and size. */
    inline FeeFrac(int64_t s, int32_t b) noexcept : fee{s}, size{b}
    {
        // If size==0, fee must be 0 as well.
        assert(size != 0 || fee == 0);
    }

    inline FeeFrac(const FeeFrac&) noexcept = default;
    inline FeeFrac& operator=(const FeeFrac&) noexcept = default;

    /** Check if this is empty (size and fee are 0). */
    bool inline IsEmpty() const noexcept {
        return size == 0;
    }

    /** Add size and size of another FeeFrac to this one. */
    void inline operator+=(const FeeFrac& other) noexcept
    {
        fee += other.fee;
        size += other.size;
    }

    /** Subtrack size and size of another FeeFrac from this one. */
    void inline operator-=(const FeeFrac& other) noexcept
    {
        fee -= other.fee;
        size -= other.size;
        assert(size != 0 || fee == 0);
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

    friend inline std::strong_ordering FeeRateCompare(const FeeFrac& a, const FeeFrac& b) noexcept
    {
        if (a.fee > INT32_MAX || a.fee < INT32_MIN || b.fee > INT32_MAX || b.fee < INT32_MIN) {
            auto a_cross = Mul128(a.fee, b.size);
            auto b_cross = Mul128(b.fee, a.size);
            return a_cross <=> b_cross;
        } else {
            auto a_cross = a.fee * b.size;
            auto b_cross = b.fee * a.size;
            return a_cross <=> b_cross;
        }
    }

    friend inline std::strong_ordering operator<=>(const FeeFrac& a, const FeeFrac& b) noexcept
    {
        auto feerate_cmp = FeeRateCompare(a, b);
        if (feerate_cmp != 0) return feerate_cmp; // NOLINT(modernize-use-nullptr)
        return b.size <=> a.size;
    }

    /** Check if a FeeFrac object has strictly lower feerate than another. */
    friend inline bool operator<<(const FeeFrac& a, const FeeFrac& b) noexcept
    {
        return FeeRateCompare(a, b) < 0; // NOLINT(modernize-use-nullptr)
    }

    /** Check if a FeeFrac object has strictly higher feerate than another. */
    friend inline bool operator>>(const FeeFrac& a, const FeeFrac& b) noexcept
    {
        return FeeRateCompare(a, b) > 0; // NOLINT(modernize-use-nullptr)
    }

    friend inline void swap(FeeFrac& a, FeeFrac& b) noexcept
    {
        std::swap(a.fee, b.fee);
        std::swap(a.size, b.size);
    }
};

void BuildDiagramFromUnsortedChunks(std::vector<FeeFrac>& chunks, std::vector<FeeFrac>& diagram);

#endif // BITCOIN_UTIL_FEEFRAC_H
