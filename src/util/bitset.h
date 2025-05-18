// Copyright (c) The Tortoisecoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef TORTOISECOIN_UTIL_BITSET_H
#define TORTOISECOIN_UTIL_BITSET_H

#include <util/check.h>

#include <array>
#include <bit>
#include <cstdint>
#include <limits>
#include <type_traits>

/* This file provides data types similar to std::bitset, but adds the following functionality:
 *
 * - Efficient iteration over all set bits (compatible with range-based for loops).
 * - Efficient search for the first and last set bit (First() and Last()).
 * - Efficient set subtraction: (a - b) implements "a and not b".
 * - Efficient non-strict subset/superset testing: IsSubsetOf() and IsSupersetOf().
 * - Efficient set overlap testing: a.Overlaps(b)
 * - Efficient construction of set containing 0..N-1 (S::Fill).
 * - Efficient construction of a single set (S::Singleton).
 * - Construction from initializer lists.
 *
 * Other differences:
 * - BitSet<N> is a bitset that supports at least N elements, but may support more (Size() reports
 *   the actual number). Because the actual number is unpredictable, there are no operations that
 *   affect all positions (like std::bitset's operator~, flip(), or all()).
 * - Various other unimplemented features.
 */

namespace bitset_detail {

/** Count the number of bits set in an unsigned integer type. */
template<typename I>
unsigned inline constexpr PopCount(I v)
{
    static_assert(std::is_integral_v<I> && std::is_unsigned_v<I> && std::numeric_limits<I>::radix == 2);
    constexpr auto BITS = std::numeric_limits<I>::digits;
    // Algorithms from https://en.wikipedia.org/wiki/Hamming_weight#Efficient_implementation.
    // These seem to be faster than std::popcount when compiling for non-SSE4 on x86_64.
    if constexpr (BITS <= 32) {
        v -= (v >> 1) & 0x55555555;
        v = (v & 0x33333333) + ((v >> 2) & 0x33333333);
        v = (v + (v >> 4)) & 0x0f0f0f0f;
        if constexpr (BITS > 8) v += v >> 8;
        if constexpr (BITS > 16) v += v >> 16;
        return v & 0x3f;
    } else {
        static_assert(BITS <= 64);
        v -= (v >> 1) & 0x5555555555555555;
        v = (v & 0x3333333333333333) + ((v >> 2) & 0x3333333333333333);
        v = (v + (v >> 4)) & 0x0f0f0f0f0f0f0f0f;
        return (v * uint64_t{0x0101010101010101}) >> 56;
    }
}

/** A bitset implementation backed by a single integer of type I. */
template<typename I>
class IntBitSet
{
    // Only binary, unsigned, integer, types allowed.
    static_assert(std::is_integral_v<I> && std::is_unsigned_v<I> && std::numeric_limits<I>::radix == 2);
    /** The maximum number of bits this bitset supports. */
    static constexpr unsigned MAX_SIZE = std::numeric_limits<I>::digits;
    /** Integer whose bits represent this bitset. */
    I m_val;
    /** Internal constructor with a given integer as contents. */
    IntBitSet(I val) noexcept : m_val{val} {}
    /** Dummy type to return using end(). Only used for comparing with Iterator. */
    class IteratorEnd
    {
        friend class IntBitSet;
        constexpr IteratorEnd() = default;
    public:
        constexpr IteratorEnd(const IteratorEnd&) = default;
    };
    /** Iterator type returned by begin(), which efficiently iterates all 1 positions. */
    class Iterator
    {
        friend class IntBitSet;
        I m_val; /**< The original integer's remaining bits. */
        unsigned m_pos; /** Last reported 1 position (if m_pos != 0). */
        constexpr Iterator(I val) noexcept : m_val(val), m_pos(0)
        {
            if (m_val != 0) m_pos = std::countr_zero(m_val);
        }
    public:
        /** Do not allow external code to construct an Iterator. */
        Iterator() = delete;
        // Copying is allowed.
        constexpr Iterator(const Iterator&) noexcept = default;
        constexpr Iterator& operator=(const Iterator&) noexcept = default;
        /** Test whether we are done (can only compare with IteratorEnd). */
        constexpr friend bool operator==(const Iterator& a, const IteratorEnd&) noexcept
        {
            return a.m_val == 0;
        }
        /** Progress to the next 1 bit (only if != IteratorEnd). */
        constexpr Iterator& operator++() noexcept
        {
            Assume(m_val != 0);
            m_val &= m_val - I{1U};
            if (m_val != 0) m_pos = std::countr_zero(m_val);
            return *this;
        }
        /** Get the current bit position (only if != IteratorEnd). */
        constexpr unsigned operator*() const noexcept
        {
            Assume(m_val != 0);
            return m_pos;
        }
    };

public:
    /** Construct an all-zero bitset. */
    constexpr IntBitSet() noexcept : m_val{0} {}
    /** Copy construct a bitset. */
    constexpr IntBitSet(const IntBitSet&) noexcept = default;
    /** Construct from a list of values. */
    constexpr IntBitSet(std::initializer_list<unsigned> ilist) noexcept : m_val(0)
    {
        for (auto pos : ilist) Set(pos);
    }
    /** Copy assign a bitset. */
    constexpr IntBitSet& operator=(const IntBitSet&) noexcept = default;
    /** Assign from a list of positions (which will be made true, all others false). */
    constexpr IntBitSet& operator=(std::initializer_list<unsigned> ilist) noexcept
    {
        m_val = 0;
        for (auto pos : ilist) Set(pos);
        return *this;
    }
    /** Construct a bitset with the singleton i. */
    static constexpr IntBitSet Singleton(unsigned i) noexcept
    {
        Assume(i < MAX_SIZE);
        return IntBitSet(I(1U) << i);
    }
    /** Construct a bitset with bits 0..count-1 (inclusive) set to 1. */
    static constexpr IntBitSet Fill(unsigned count) noexcept
    {
        IntBitSet ret;
        Assume(count <= MAX_SIZE);
        if (count) ret.m_val = I(~I{0}) >> (MAX_SIZE - count);
        return ret;
    }
    /** Set a bit to 1. */
    constexpr void Set(unsigned pos) noexcept
    {
        Assume(pos < MAX_SIZE);
        m_val |= I{1U} << pos;
    }
    /** Set a bit to the specified value. */
    constexpr void Set(unsigned pos, bool val) noexcept
    {
        Assume(pos < MAX_SIZE);
        m_val = (m_val & ~I(I{1U} << pos)) | (I(val) << pos);
    }
    /** Set a bit to 0. */
    constexpr void Reset(unsigned pos) noexcept
    {
        Assume(pos < MAX_SIZE);
        m_val &= ~I(I{1U} << pos);
    }
    /** Retrieve a bit at the given position. */
    constexpr bool operator[](unsigned pos) const noexcept
    {
        Assume(pos < MAX_SIZE);
        return (m_val >> pos) & 1U;
    }
    /** Compute the number of 1 bits in the bitset. */
    constexpr unsigned Count() const noexcept { return PopCount(m_val); }
    /** Return the number of bits that this object holds. */
    static constexpr unsigned Size() noexcept { return MAX_SIZE; }
    /** Check if all bits are 0. */
    constexpr bool None() const noexcept { return m_val == 0; }
    /** Check if any bits are 1. */
    constexpr bool Any() const noexcept { return !None(); }
    /** Return an object that iterates over all 1 bits (++ and * only allowed when != end()). */
    constexpr Iterator begin() const noexcept { return Iterator(m_val); }
    /** Return a dummy object to compare Iterators with. */
    constexpr IteratorEnd end() const noexcept { return IteratorEnd(); }
    /** Find the first element (requires Any()). */
    constexpr unsigned First() const noexcept
    {
        Assume(m_val != 0);
        return std::countr_zero(m_val);
    }
    /** Find the last element (requires Any()). */
    constexpr unsigned Last() const noexcept
    {
        Assume(m_val != 0);
        return std::bit_width(m_val) - 1;
    }
    /** Set this object's bits to be the binary AND between respective bits from this and a. */
    constexpr IntBitSet& operator|=(const IntBitSet& a) noexcept { m_val |= a.m_val; return *this; }
    /** Set this object's bits to be the binary OR between respective bits from this and a. */
    constexpr IntBitSet& operator&=(const IntBitSet& a) noexcept { m_val &= a.m_val; return *this; }
    /** Set this object's bits to be the binary AND NOT between respective bits from this and a. */
    constexpr IntBitSet& operator-=(const IntBitSet& a) noexcept { m_val &= ~a.m_val; return *this; }
    /** Set this object's bits to be the binary XOR between respective bits from this as a. */
    constexpr IntBitSet& operator^=(const IntBitSet& a) noexcept { m_val ^= a.m_val; return *this; }
    /** Check if the intersection between two sets is non-empty. */
    constexpr bool Overlaps(const IntBitSet& a) const noexcept { return m_val & a.m_val; }
    /** Return an object with the binary AND between respective bits from a and b. */
    friend constexpr IntBitSet operator&(const IntBitSet& a, const IntBitSet& b) noexcept { return I(a.m_val & b.m_val); }
    /** Return an object with the binary OR between respective bits from a and b. */
    friend constexpr IntBitSet operator|(const IntBitSet& a, const IntBitSet& b) noexcept { return I(a.m_val | b.m_val); }
    /** Return an object with the binary AND NOT between respective bits from a and b. */
    friend constexpr IntBitSet operator-(const IntBitSet& a, const IntBitSet& b) noexcept { return I(a.m_val & ~b.m_val); }
    /** Return an object with the binary XOR between respective bits from a and b. */
    friend constexpr IntBitSet operator^(const IntBitSet& a, const IntBitSet& b) noexcept { return I(a.m_val ^ b.m_val); }
    /** Check if bitset a and bitset b are identical. */
    friend constexpr bool operator==(const IntBitSet& a, const IntBitSet& b) noexcept = default;
    /** Check if bitset a is a superset of bitset b (= every 1 bit in b is also in a). */
    constexpr bool IsSupersetOf(const IntBitSet& a) const noexcept { return (a.m_val & ~m_val) == 0; }
    /** Check if bitset a is a subset of bitset b (= every 1 bit in a is also in b). */
    constexpr bool IsSubsetOf(const IntBitSet& a) const noexcept { return (m_val & ~a.m_val) == 0; }
    /** Swap two bitsets. */
    friend constexpr void swap(IntBitSet& a, IntBitSet& b) noexcept { std::swap(a.m_val, b.m_val); }
};

/** A bitset implementation backed by N integers of type I. */
template<typename I, unsigned N>
class MultiIntBitSet
{
    // Only binary, unsigned, integer, types allowed.
    static_assert(std::is_integral_v<I> && std::is_unsigned_v<I> && std::numeric_limits<I>::radix == 2);
    // Cannot be empty.
    static_assert(N > 0);
    /** The number of bits per integer. */
    static constexpr unsigned LIMB_BITS = std::numeric_limits<I>::digits;
    /** Number of elements this set type supports. */
    static constexpr unsigned MAX_SIZE = LIMB_BITS * N;
    // No overflow allowed here.
    static_assert(MAX_SIZE / LIMB_BITS == N);
    /** Array whose member integers store the bits of the set. */
    std::array<I, N> m_val;
    /** Dummy type to return using end(). Only used for comparing with Iterator. */
    class IteratorEnd
    {
        friend class MultiIntBitSet;
        constexpr IteratorEnd() = default;
    public:
        constexpr IteratorEnd(const IteratorEnd&) = default;
    };
    /** Iterator type returned by begin(), which efficiently iterates all 1 positions. */
    class Iterator
    {
        friend class MultiIntBitSet;
        const std::array<I, N>* m_ptr; /**< Pointer to array to fetch bits from. */
        I m_val; /**< The remaining bits of (*m_ptr)[m_idx]. */
        unsigned m_pos; /**< The last reported position. */
        unsigned m_idx; /**< The index in *m_ptr currently being iterated over. */
        constexpr Iterator(const std::array<I, N>& ref) noexcept : m_ptr(&ref), m_idx(0)
        {
            do {
                m_val = (*m_ptr)[m_idx];
                if (m_val) {
                    m_pos = std::countr_zero(m_val) + m_idx * LIMB_BITS;
                    break;
                }
                ++m_idx;
            } while(m_idx < N);
        }

    public:
        /** Do not allow external code to construct an Iterator. */
        Iterator() = delete;
        // Copying is allowed.
        constexpr Iterator(const Iterator&) noexcept = default;
        constexpr Iterator& operator=(const Iterator&) noexcept = default;
        /** Test whether we are done (can only compare with IteratorEnd). */
        friend constexpr bool operator==(const Iterator& a, const IteratorEnd&) noexcept
        {
            return a.m_idx == N;
        }
        /** Progress to the next 1 bit (only if != IteratorEnd). */
        constexpr Iterator& operator++() noexcept
        {
            Assume(m_idx < N);
            m_val &= m_val - I{1U};
            if (m_val == 0) {
                while (true) {
                    ++m_idx;
                    if (m_idx == N) break;
                    m_val = (*m_ptr)[m_idx];
                    if (m_val) {
                        m_pos = std::countr_zero(m_val) + m_idx * LIMB_BITS;
                        break;
                    }
                }
            } else {
                m_pos = std::countr_zero(m_val) + m_idx * LIMB_BITS;
            }
            return *this;
        }
        /** Get the current bit position (only if != IteratorEnd). */
        constexpr unsigned operator*() const noexcept
        {
            Assume(m_idx < N);
            return m_pos;
        }
    };

public:
    /** Construct an all-zero bitset. */
    constexpr MultiIntBitSet() noexcept : m_val{} {}
    /** Copy construct a bitset. */
    constexpr MultiIntBitSet(const MultiIntBitSet&) noexcept = default;
    /** Copy assign a bitset. */
    constexpr MultiIntBitSet& operator=(const MultiIntBitSet&) noexcept = default;
    /** Set a bit to 1. */
    void constexpr Set(unsigned pos) noexcept
    {
        Assume(pos < MAX_SIZE);
        m_val[pos / LIMB_BITS] |= I{1U} << (pos % LIMB_BITS);
    }
    /** Set a bit to the specified value. */
    void constexpr Set(unsigned pos, bool val) noexcept
    {
        Assume(pos < MAX_SIZE);
        m_val[pos / LIMB_BITS] = (m_val[pos / LIMB_BITS] & ~I(I{1U} << (pos % LIMB_BITS))) |
                                 (I{val} << (pos % LIMB_BITS));
    }
    /** Construct a bitset from a list of values. */
    constexpr MultiIntBitSet(std::initializer_list<unsigned> ilist) noexcept : m_val{}
    {
        for (auto pos : ilist) Set(pos);
    }
    /** Set a bitset to a list of values. */
    constexpr MultiIntBitSet& operator=(std::initializer_list<unsigned> ilist) noexcept
    {
        m_val.fill(0);
        for (auto pos : ilist) Set(pos);
        return *this;
    }
    /** Set a bit to 0. */
    void constexpr Reset(unsigned pos) noexcept
    {
        Assume(pos < MAX_SIZE);
        m_val[pos / LIMB_BITS] &= ~I(I{1U} << (pos % LIMB_BITS));
    }
    /** Retrieve a bit at the given position. */
    bool constexpr operator[](unsigned pos) const noexcept
    {
        Assume(pos < MAX_SIZE);
        return (m_val[pos / LIMB_BITS] >> (pos % LIMB_BITS)) & 1U;
    }
    /** Construct a bitset with the singleton pos. */
    static constexpr MultiIntBitSet Singleton(unsigned pos) noexcept
    {
        Assume(pos < MAX_SIZE);
        MultiIntBitSet ret;
        ret.m_val[pos / LIMB_BITS] = I{1U} << (pos % LIMB_BITS);
        return ret;
    }
    /** Construct a bitset with bits 0..count-1 (inclusive) set to 1. */
    static constexpr MultiIntBitSet Fill(unsigned count) noexcept
    {
        Assume(count <= MAX_SIZE);
        MultiIntBitSet ret;
        if (count) {
            unsigned i = 0;
            while (count > LIMB_BITS) {
                ret.m_val[i++] = I(~I{0});
                count -= LIMB_BITS;
            }
            ret.m_val[i] = I(~I{0}) >> (LIMB_BITS - count);
        }
        return ret;
    }
    /** Return the number of bits that this object holds. */
    static constexpr unsigned Size() noexcept { return MAX_SIZE; }
    /** Compute the number of 1 bits in the bitset. */
    unsigned constexpr Count() const noexcept
    {
        unsigned ret{0};
        for (I v : m_val) ret += PopCount(v);
        return ret;
    }
    /** Check if all bits are 0. */
    bool constexpr None() const noexcept
    {
        for (auto v : m_val) {
            if (v != 0) return false;
        }
        return true;
    }
    /** Check if any bits are 1. */
    bool constexpr Any() const noexcept { return !None(); }
    /** Return an object that iterates over all 1 bits (++ and * only allowed when != end()). */
    Iterator constexpr begin() const noexcept { return Iterator(m_val); }
    /** Return a dummy object to compare Iterators with. */
    IteratorEnd constexpr end() const noexcept { return IteratorEnd(); }
    /** Find the first element (requires Any()). */
    unsigned constexpr First() const noexcept
    {
        unsigned p = 0;
        while (m_val[p] == 0) {
            ++p;
            Assume(p < N);
        }
        return std::countr_zero(m_val[p]) + p * LIMB_BITS;
    }
    /** Find the last element (requires Any()). */
    unsigned constexpr Last() const noexcept
    {
        unsigned p = N - 1;
        while (m_val[p] == 0) {
            Assume(p > 0);
            --p;
        }
        return std::bit_width(m_val[p]) - 1 + p * LIMB_BITS;
    }
    /** Set this object's bits to be the binary OR between respective bits from this and a. */
    constexpr MultiIntBitSet& operator|=(const MultiIntBitSet& a) noexcept
    {
        for (unsigned i = 0; i < N; ++i) {
            m_val[i] |= a.m_val[i];
        }
        return *this;
    }
    /** Set this object's bits to be the binary AND between respective bits from this and a. */
    constexpr MultiIntBitSet& operator&=(const MultiIntBitSet& a) noexcept
    {
        for (unsigned i = 0; i < N; ++i) {
            m_val[i] &= a.m_val[i];
        }
        return *this;
    }
    /** Set this object's bits to be the binary AND NOT between respective bits from this and a. */
    constexpr MultiIntBitSet& operator-=(const MultiIntBitSet& a) noexcept
    {
        for (unsigned i = 0; i < N; ++i) {
            m_val[i] &= ~a.m_val[i];
        }
        return *this;
    }
    /** Set this object's bits to be the binary XOR between respective bits from this and a. */
    constexpr MultiIntBitSet& operator^=(const MultiIntBitSet& a) noexcept
    {
        for (unsigned i = 0; i < N; ++i) {
            m_val[i] ^= a.m_val[i];
        }
        return *this;
    }
    /** Check whether the intersection between two sets is non-empty. */
    constexpr bool Overlaps(const MultiIntBitSet& a) const noexcept
    {
        for (unsigned i = 0; i < N; ++i) {
            if (m_val[i] & a.m_val[i]) return true;
        }
        return false;
    }
    /** Return an object with the binary AND between respective bits from a and b. */
    friend constexpr MultiIntBitSet operator&(const MultiIntBitSet& a, const MultiIntBitSet& b) noexcept
    {
        MultiIntBitSet r;
        for (unsigned i = 0; i < N; ++i) {
            r.m_val[i] = a.m_val[i] & b.m_val[i];
        }
        return r;
    }
    /** Return an object with the binary OR between respective bits from a and b. */
    friend constexpr MultiIntBitSet operator|(const MultiIntBitSet& a, const MultiIntBitSet& b) noexcept
    {
        MultiIntBitSet r;
        for (unsigned i = 0; i < N; ++i) {
            r.m_val[i] = a.m_val[i] | b.m_val[i];
        }
        return r;
    }
    /** Return an object with the binary AND NOT between respective bits from a and b. */
    friend constexpr MultiIntBitSet operator-(const MultiIntBitSet& a, const MultiIntBitSet& b) noexcept
    {
        MultiIntBitSet r;
        for (unsigned i = 0; i < N; ++i) {
            r.m_val[i] = a.m_val[i] & ~b.m_val[i];
        }
        return r;
    }
    /** Return an object with the binary XOR between respective bits from a and b. */
    friend constexpr MultiIntBitSet operator^(const MultiIntBitSet& a, const MultiIntBitSet& b) noexcept
    {
        MultiIntBitSet r;
        for (unsigned i = 0; i < N; ++i) {
            r.m_val[i] = a.m_val[i] ^ b.m_val[i];
        }
        return r;
    }
    /** Check if bitset a is a superset of bitset b (= every 1 bit in b is also in a). */
    constexpr bool IsSupersetOf(const MultiIntBitSet& a) const noexcept
    {
        for (unsigned i = 0; i < N; ++i) {
            if (a.m_val[i] & ~m_val[i]) return false;
        }
        return true;
    }
    /** Check if bitset a is a subset of bitset b (= every 1 bit in a is also in b). */
    constexpr bool IsSubsetOf(const MultiIntBitSet& a) const noexcept
    {
        for (unsigned i = 0; i < N; ++i) {
            if (m_val[i] & ~a.m_val[i]) return false;
        }
        return true;
    }
    /** Check if bitset a and bitset b are identical. */
    friend constexpr bool operator==(const MultiIntBitSet& a, const MultiIntBitSet& b) noexcept = default;
    /** Swap two bitsets. */
    friend constexpr void swap(MultiIntBitSet& a, MultiIntBitSet& b) noexcept { std::swap(a.m_val, b.m_val); }
};

} // namespace bitset_detail

// BitSet dispatches to IntBitSet or MultiIntBitSet as appropriate for the requested minimum number
// of bits. Use IntBitSet up to 32-bit, or up to 64-bit on 64-bit platforms; above that, use a
// MultiIntBitSet of size_t.
template<unsigned BITS>
using BitSet = std::conditional_t<(BITS <= 32), bitset_detail::IntBitSet<uint32_t>,
               std::conditional_t<(BITS <= std::numeric_limits<size_t>::digits), bitset_detail::IntBitSet<size_t>,
               bitset_detail::MultiIntBitSet<size_t, (BITS + std::numeric_limits<size_t>::digits - 1) / std::numeric_limits<size_t>::digits>>>;

#endif // TORTOISECOIN_UTIL_BITSET_H
