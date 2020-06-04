// Copyright (c) 2018-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SYSCOIN_SPAN_H
#define SYSCOIN_SPAN_H

#include <type_traits>
#include <cstddef>
#include <algorithm>
#include <assert.h>

#ifdef DEBUG
#define CONSTEXPR_IF_NOT_DEBUG
#define ASSERT_IF_DEBUG(x) assert((x))
#else
#define CONSTEXPR_IF_NOT_DEBUG constexpr
#define ASSERT_IF_DEBUG(x)
#endif

/** A Span is an object that can refer to a contiguous sequence of objects.
 *
 * It implements a subset of C++20's std::span.
 */
template<typename C>
class Span
{
    C* m_data;
    std::ptrdiff_t m_size;

public:
    constexpr Span() noexcept : m_data(nullptr), m_size(0) {}

    /** Construct a span from a begin pointer and a size.
     *
     * This implements a subset of the iterator-based std::span constructor in C++20,
     * which is hard to implement without std::address_of.
     */
    template <typename T, typename std::enable_if<std::is_convertible<T (*)[], C (*)[]>::value, int>::type = 0>
    constexpr Span(T* begin, std::size_t size) noexcept : m_data(begin), m_size(size) {}

    /** Construct a span from a begin and end pointer.
     *
     * This implements a subset of the iterator-based std::span constructor in C++20,
     * which is hard to implement without std::address_of.
     */
    template <typename T, typename std::enable_if<std::is_convertible<T (*)[], C (*)[]>::value, int>::type = 0>
    CONSTEXPR_IF_NOT_DEBUG Span(T* begin, T* end) noexcept : m_data(begin), m_size(end - begin)
    {
        ASSERT_IF_DEBUG(end >= begin);
    }

    /** Implicit conversion of spans between compatible types.
     *
     *  Specifically, if a pointer to an array of type O can be implicitly converted to a pointer to an array of type
     *  C, then permit implicit conversion of Span<O> to Span<C>. This matches the behavior of the corresponding
     *  C++20 std::span constructor.
     *
     *  For example this means that a Span<T> can be converted into a Span<const T>.
     */
    template <typename O, typename std::enable_if<std::is_convertible<O (*)[], C (*)[]>::value, int>::type = 0>
    constexpr Span(const Span<O>& other) noexcept : m_data(other.m_data), m_size(other.m_size) {}

    /** Default copy constructor. */
    constexpr Span(const Span&) noexcept = default;

    /** Default assignment operator. */
    Span& operator=(const Span& other) noexcept = default;

    constexpr C* data() const noexcept { return m_data; }
    constexpr C* begin() const noexcept { return m_data; }
    constexpr C* end() const noexcept { return m_data + m_size; }
    CONSTEXPR_IF_NOT_DEBUG C& front() const noexcept
    {
        ASSERT_IF_DEBUG(size() > 0);
        return m_data[0];
    }
    CONSTEXPR_IF_NOT_DEBUG C& back() const noexcept
    {
        ASSERT_IF_DEBUG(size() > 0);
        return m_data[m_size - 1];
    }
    constexpr std::size_t size() const noexcept { return m_size; }
    CONSTEXPR_IF_NOT_DEBUG C& operator[](std::size_t pos) const noexcept
    {
        ASSERT_IF_DEBUG(size() > pos);
        return m_data[pos];
    }
    CONSTEXPR_IF_NOT_DEBUG Span<C> subspan(std::size_t offset) const noexcept
    {
        ASSERT_IF_DEBUG(size() >= offset);
        return Span<C>(m_data + offset, m_size - offset);
    }
    CONSTEXPR_IF_NOT_DEBUG Span<C> subspan(std::size_t offset, std::size_t count) const noexcept
    {
        ASSERT_IF_DEBUG(size() >= offset + count);
        return Span<C>(m_data + offset, count);
    }
    CONSTEXPR_IF_NOT_DEBUG Span<C> first(std::size_t count) const noexcept
    {
        ASSERT_IF_DEBUG(size() >= count);
        return Span<C>(m_data, count);
    }
    CONSTEXPR_IF_NOT_DEBUG Span<C> last(std::size_t count) const noexcept
    {
         ASSERT_IF_DEBUG(size() >= count);
         return Span<C>(m_data + m_size - count, count);
    }

    friend constexpr bool operator==(const Span& a, const Span& b) noexcept { return a.size() == b.size() && std::equal(a.begin(), a.end(), b.begin()); }
    friend constexpr bool operator!=(const Span& a, const Span& b) noexcept { return !(a == b); }
    friend constexpr bool operator<(const Span& a, const Span& b) noexcept { return std::lexicographical_compare(a.begin(), a.end(), b.begin(), b.end()); }
    friend constexpr bool operator<=(const Span& a, const Span& b) noexcept { return !(b < a); }
    friend constexpr bool operator>(const Span& a, const Span& b) noexcept { return (b < a); }
    friend constexpr bool operator>=(const Span& a, const Span& b) noexcept { return !(a < b); }

    template <typename O> friend class Span;
};

/** Create a span to a container exposing data() and size().
 *
 * This correctly deals with constness: the returned Span's element type will be
 * whatever data() returns a pointer to. If either the passed container is const,
 * or its element type is const, the resulting span will have a const element type.
 *
 * std::span will have a constructor that implements this functionality directly.
 */
template<typename A, int N>
constexpr Span<A> MakeSpan(A (&a)[N]) { return Span<A>(a, N); }

template<typename V>
constexpr Span<typename std::remove_pointer<decltype(std::declval<V>().data())>::type> MakeSpan(V& v) { return Span<typename std::remove_pointer<decltype(std::declval<V>().data())>::type>(v.data(), v.size()); }

/** Pop the last element off a span, and return a reference to that element. */
template <typename T>
T& SpanPopBack(Span<T>& span)
{
    size_t size = span.size();
    ASSERT_IF_DEBUG(size > 0);
    T& back = span[size - 1];
    span = Span<T>(span.data(), size - 1);
    return back;
}

#endif
