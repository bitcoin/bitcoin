// Copyright (c) 2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_SPAN_H
#define BITCOIN_SPAN_H

#include <type_traits>
#include <cstddef>
#include <algorithm>

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
    constexpr Span(C* data, std::ptrdiff_t size) noexcept : m_data(data), m_size(size) {}
    constexpr Span(C* data, C* end) noexcept : m_data(data), m_size(end - data) {}

    constexpr C* data() const noexcept { return m_data; }
    constexpr C* begin() const noexcept { return m_data; }
    constexpr C* end() const noexcept { return m_data + m_size; }
    constexpr std::ptrdiff_t size() const noexcept { return m_size; }
    constexpr C& operator[](std::ptrdiff_t pos) const noexcept { return m_data[pos]; }

    constexpr Span<C> subspan(std::ptrdiff_t offset) const noexcept { return Span<C>(m_data + offset, m_size - offset); }
    constexpr Span<C> subspan(std::ptrdiff_t offset, std::ptrdiff_t count) const noexcept { return Span<C>(m_data + offset, count); }
    constexpr Span<C> first(std::ptrdiff_t count) const noexcept { return Span<C>(m_data, count); }
    constexpr Span<C> last(std::ptrdiff_t count) const noexcept { return Span<C>(m_data + m_size - count, count); }

    friend constexpr bool operator==(const Span& a, const Span& b) noexcept { return a.size() == b.size() && std::equal(a.begin(), a.end(), b.begin()); }
    friend constexpr bool operator!=(const Span& a, const Span& b) noexcept { return !(a == b); }
    friend constexpr bool operator<(const Span& a, const Span& b) noexcept { return std::lexicographical_compare(a.begin(), a.end(), b.begin(), b.end()); }
    friend constexpr bool operator<=(const Span& a, const Span& b) noexcept { return !(b < a); }
    friend constexpr bool operator>(const Span& a, const Span& b) noexcept { return (b < a); }
    friend constexpr bool operator>=(const Span& a, const Span& b) noexcept { return !(a < b); }
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

#endif
