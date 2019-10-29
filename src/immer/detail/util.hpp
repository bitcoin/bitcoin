//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#pragma once

#include <immer/config.hpp>

#include <cstddef>
#include <new>
#include <type_traits>
#include <memory>

#include <immer/detail/type_traits.hpp>

#if defined(_MSC_VER)
#include <intrin.h> // for __lzcnt*
#endif

namespace immer {
namespace detail {

template <typename T>
using aligned_storage_for =
    typename std::aligned_storage<sizeof(T), alignof(T)>::type;

template <typename T>
T& auto_const_cast(const T& x) { return const_cast<T&>(x); }
template <typename T>
T&& auto_const_cast(const T&& x) { return const_cast<T&&>(std::move(x)); }

template <typename Iter1, typename Iter2>
auto uninitialized_move(Iter1 in1, Iter1 in2, Iter2 out)
{
    return std::uninitialized_copy(std::make_move_iterator(in1),
                                   std::make_move_iterator(in2),
                                   out);
}

template <class T>
void destroy(T* first, T* last)
{
    for (; first != last; ++first)
        first->~T();
}

template <class T, class Size>
void destroy_n(T* p, Size n)
{
    auto e = p + n;
    for (; p != e; ++p)
        p->~T();
}

template <typename Heap, typename T, typename... Args>
T* make(Args&& ...args)
{
    auto ptr = Heap::allocate(sizeof(T));
    try {
        return new (ptr) T{std::forward<Args>(args)...};
    } catch (...) {
        Heap::deallocate(sizeof(T), ptr);
        throw;
    }
}

struct not_supported_t {};
struct empty_t {};

template <typename T>
struct exact_t
{
    T value;
    exact_t(T v) : value{v} {};
};

template <typename T>
inline constexpr auto clz_(T) -> not_supported_t { IMMER_UNREACHABLE; return {}; }
#if defined(_MSC_VER)
// inline auto clz_(unsigned short x) { return __lzcnt16(x); }
// inline auto clz_(unsigned int x) { return __lzcnt(x); }
// inline auto clz_(unsigned __int64 x) { return __lzcnt64(x); }
#else
inline constexpr auto clz_(unsigned int x) { return __builtin_clz(x); }
inline constexpr auto clz_(unsigned long x) { return __builtin_clzl(x); }
inline constexpr auto clz_(unsigned long long x) { return __builtin_clzll(x); }
#endif

template <typename T>
inline constexpr T log2_aux(T x, T r = 0)
{
    return x <= 1 ? r : log2_aux(x >> 1, r + 1);
}

template <typename T>
inline constexpr auto log2(T x)
    -> std::enable_if_t<!std::is_same<decltype(clz_(x)), not_supported_t>::value, T>
{
    return x == 0 ? 0 : sizeof(std::size_t) * 8 - 1 - clz_(x);
}

template <typename T>
inline constexpr auto log2(T x)
    -> std::enable_if_t<std::is_same<decltype(clz_(x)), not_supported_t>::value, T>
{
    return log2_aux(x);
}

template <bool b, typename F>
auto static_if(F&& f) -> std::enable_if_t<b>
{ std::forward<F>(f)(empty_t{}); }
template <bool b, typename F>
auto static_if(F&& f) -> std::enable_if_t<!b>
{}

template <bool b, typename R=void, typename F1, typename F2>
auto static_if(F1&& f1, F2&& f2) -> std::enable_if_t<b, R>
{ return std::forward<F1>(f1)(empty_t{}); }
template <bool b, typename R=void, typename F1, typename F2>
auto static_if(F1&& f1, F2&& f2) -> std::enable_if_t<!b, R>
{ return std::forward<F2>(f2)(empty_t{}); }

template <typename T, T value>
struct constantly
{
    template <typename... Args>
    T operator() (Args&&...) const { return value; }
};

/*!
 * An alias to `std::distance`
 */
template <typename Iterator, typename Sentinel,
          std::enable_if_t
          <detail::std_distance_supports_v<Iterator,Sentinel>, bool> = true>
typename std::iterator_traits<Iterator>::difference_type
distance(Iterator first, Sentinel last)
{
    return std::distance(first, last);
}

/*!
 * Equivalent of the `std::distance` applied to the sentinel-delimited
 * forward range @f$ [first, last) @f$
 */
template <typename Iterator, typename Sentinel,
          std::enable_if_t
          <(!detail::std_distance_supports_v<Iterator,Sentinel>)
           && detail::is_forward_iterator_v<Iterator>
           && detail::compatible_sentinel_v<Iterator,Sentinel>
           && (!detail::is_subtractable_v<Sentinel, Iterator>), bool> = true>
typename std::iterator_traits<Iterator>::difference_type
distance(Iterator first, Sentinel last)
{
    std::size_t result = 0;
    while (first != last) {
        ++first;
        ++result;
    }
    return result;
}

/*!
 * Equivalent of the `std::distance` applied to the sentinel-delimited
 * random access range @f$ [first, last) @f$
 */
template <typename Iterator, typename Sentinel,
          std::enable_if_t
          <(!detail::std_distance_supports_v<Iterator,Sentinel>)
           && detail::is_forward_iterator_v<Iterator>
           && detail::compatible_sentinel_v<Iterator,Sentinel>
           && detail::is_subtractable_v<Sentinel, Iterator>, bool> = true>
typename std::iterator_traits<Iterator>::difference_type
distance(Iterator first, Sentinel last)
{
    return last - first;
}



/*!
 * An alias to `std::uninitialized_copy`
 */
template <typename Iterator, typename Sentinel, typename SinkIter,
          std::enable_if_t
          <detail::std_uninitialized_copy_supports_v
           <Iterator,Sentinel,SinkIter>, bool> = true>
SinkIter uninitialized_copy(Iterator first, Sentinel last, SinkIter d_first)
{
    return std::uninitialized_copy(first, last, d_first);
}

/*!
 * Equivalent of the `std::uninitialized_copy` applied to the
 * sentinel-delimited forward range @f$ [first, last) @f$
 */
template <typename SourceIter, typename Sent, typename SinkIter,
          std::enable_if_t
          <(!detail::std_uninitialized_copy_supports_v<SourceIter, Sent, SinkIter>)
           && detail::compatible_sentinel_v<SourceIter,Sent>
           && detail::is_forward_iterator_v<SinkIter>, bool> = true>
SinkIter uninitialized_copy(SourceIter first, Sent last, SinkIter d_first)
{
    auto current = d_first;
    try {
        while (first != last) {
            *current++ = *first;
            ++first;
        }
    } catch (...) {
        using Value = typename std::iterator_traits<SinkIter>::value_type;
        for (;d_first != current; ++d_first){
            d_first->~Value();
        }
        throw;
    }
    return current;
}

} // namespace detail
} // namespace immer
