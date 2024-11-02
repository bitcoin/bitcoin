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
#include <memory>
#include <new>
#include <type_traits>

#include <immer/detail/type_traits.hpp>

#if defined(_MSC_VER)
#include <intrin.h> // for __lzcnt*
#endif

namespace immer {
namespace detail {

template <typename T>
const T* as_const(T* x)
{
    return x;
}

template <typename T>
const T& as_const(T& x)
{
    return x;
}

template <typename T>
using aligned_storage_for =
    typename std::aligned_storage<sizeof(T), alignof(T)>::type;

template <typename T>
T& auto_const_cast(const T& x)
{
    return const_cast<T&>(x);
}
template <typename T>
T&& auto_const_cast(const T&& x)
{
    return const_cast<T&&>(std::move(x));
}

template <class T>
inline auto destroy_at(T* p) noexcept
    -> std::enable_if_t<std::is_trivially_destructible<T>::value>
{
    p->~T();
}

template <class T>
inline auto destroy_at(T* p) noexcept
    -> std::enable_if_t<!std::is_trivially_destructible<T>::value>
{
    p->~T();
}

template <typename Iter1>
constexpr bool can_trivially_detroy = std::is_trivially_destructible<
    typename std::iterator_traits<Iter1>::value_type>::value;

template <class Iter>
auto destroy(Iter, Iter last) noexcept
    -> std::enable_if_t<can_trivially_detroy<Iter>, Iter>
{
    return last;
}
template <class Iter>
auto destroy(Iter first, Iter last) noexcept
    -> std::enable_if_t<!can_trivially_detroy<Iter>, Iter>
{
    for (; first != last; ++first)
        detail::destroy_at(std::addressof(*first));
    return first;
}

template <class Iter, class Size>
auto destroy_n(Iter first, Size n) noexcept
    -> std::enable_if_t<can_trivially_detroy<Iter>, Iter>
{
    return first + n;
}
template <class Iter, class Size>
auto destroy_n(Iter first, Size n) noexcept
    -> std::enable_if_t<!can_trivially_detroy<Iter>, Iter>
{
    for (; n > 0; (void) ++first, --n)
        detail::destroy_at(std::addressof(*first));
    return first;
}

template <typename Iter1, typename Iter2>
constexpr bool can_trivially_copy =
    std::is_same<typename std::iterator_traits<Iter1>::value_type,
                 typename std::iterator_traits<Iter2>::value_type>::value&&
        std::is_trivially_copyable<
            typename std::iterator_traits<Iter1>::value_type>::value;

template <typename Iter1, typename Iter2>
auto uninitialized_move(Iter1 first, Iter1 last, Iter2 out) noexcept
    -> std::enable_if_t<can_trivially_copy<Iter1, Iter2>, Iter2>
{
    return std::copy(first, last, out);
}
template <typename Iter1, typename Iter2>
auto uninitialized_move(Iter1 first, Iter1 last, Iter2 out)
    -> std::enable_if_t<!can_trivially_copy<Iter1, Iter2>, Iter2>

{
    using value_t = typename std::iterator_traits<Iter2>::value_type;
    auto current  = out;
    IMMER_TRY {
        for (; first != last; ++first, (void) ++current) {
            ::new (const_cast<void*>(static_cast<const volatile void*>(
                std::addressof(*current)))) value_t(std::move(*first));
        }
        return current;
    }
    IMMER_CATCH (...) {
        detail::destroy(out, current);
        IMMER_RETHROW;
    }
}

template <typename SourceIter, typename Sent, typename SinkIter>
auto uninitialized_copy(SourceIter first, Sent last, SinkIter out) noexcept
    -> std::enable_if_t<can_trivially_copy<SourceIter, SinkIter>, SinkIter>
{
    return std::copy(first, last, out);
}
template <typename SourceIter, typename Sent, typename SinkIter>
auto uninitialized_copy(SourceIter first, Sent last, SinkIter out)
    -> std::enable_if_t<!can_trivially_copy<SourceIter, SinkIter>, SinkIter>
{
    using value_t = typename std::iterator_traits<SinkIter>::value_type;
    auto current  = out;
    IMMER_TRY {
        for (; first != last; ++first, (void) ++current) {
            ::new (const_cast<void*>(static_cast<const volatile void*>(
                std::addressof(*current)))) value_t(*first);
        }
        return current;
    }
    IMMER_CATCH (...) {
        detail::destroy(out, current);
        IMMER_RETHROW;
    }
}

template <typename Heap, typename T, typename... Args>
T* make(Args&&... args)
{
    auto ptr = Heap::allocate(sizeof(T));
    IMMER_TRY {
        return new (ptr) T{std::forward<Args>(args)...};
    }
    IMMER_CATCH (...) {
        Heap::deallocate(sizeof(T), ptr);
        IMMER_RETHROW;
    }
}

struct not_supported_t
{};
struct empty_t
{};

template <typename T>
struct exact_t
{
    T value;
    exact_t(T v)
        : value{v} {};
};

template <typename T>
inline constexpr auto clz_(T) -> not_supported_t
{
    IMMER_UNREACHABLE;
    return {};
}
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
inline constexpr auto log2(T x) -> std::
    enable_if_t<!std::is_same<decltype(clz_(x)), not_supported_t>::value, T>
{
    return x == 0 ? 0 : sizeof(std::size_t) * 8 - 1 - clz_(x);
}

template <typename T>
inline constexpr auto log2(T x)
    -> std::enable_if_t<std::is_same<decltype(clz_(x)), not_supported_t>::value,
                        T>
{
    return log2_aux(x);
}

template <typename T>
constexpr T ipow(T num, unsigned int pow)
{
    return pow == 0 ? 1 : num * ipow(num, pow - 1);
}

template <bool b, typename F>
auto static_if(F&& f) -> std::enable_if_t<b>
{
    std::forward<F>(f)(empty_t{});
}
template <bool b, typename F>
auto static_if(F&& f) -> std::enable_if_t<!b>
{}

template <bool b, typename R = void, typename F1, typename F2>
auto static_if(F1&& f1, F2&& f2) -> std::enable_if_t<b, R>
{
    return std::forward<F1>(f1)(empty_t{});
}
template <bool b, typename R = void, typename F1, typename F2>
auto static_if(F1&& f1, F2&& f2) -> std::enable_if_t<!b, R>
{
    return std::forward<F2>(f2)(empty_t{});
}

template <typename T, T value>
struct constantly
{
    template <typename... Args>
    T operator()(Args&&...) const
    {
        return value;
    }
};

/*!
 * An alias to `std::distance`
 */
template <typename Iterator,
          typename Sentinel,
          std::enable_if_t<detail::std_distance_supports_v<Iterator, Sentinel>,
                           bool> = true>
typename std::iterator_traits<Iterator>::difference_type
distance(Iterator first, Sentinel last)
{
    return std::distance(first, last);
}

/*!
 * Equivalent of the `std::distance` applied to the sentinel-delimited
 * forward range @f$ [first, last) @f$
 */
template <typename Iterator,
          typename Sentinel,
          std::enable_if_t<
              (!detail::std_distance_supports_v<Iterator, Sentinel>) &&detail::
                      is_forward_iterator_v<Iterator> &&
                  detail::compatible_sentinel_v<Iterator, Sentinel> &&
                  (!detail::is_subtractable_v<Sentinel, Iterator>),
              bool> = true>
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
template <typename Iterator,
          typename Sentinel,
          std::enable_if_t<
              (!detail::std_distance_supports_v<Iterator, Sentinel>) &&detail::
                      is_forward_iterator_v<Iterator> &&
                  detail::compatible_sentinel_v<Iterator, Sentinel> &&
                  detail::is_subtractable_v<Sentinel, Iterator>,
              bool> = true>
typename std::iterator_traits<Iterator>::difference_type
distance(Iterator first, Sentinel last)
{
    return last - first;
}

} // namespace detail
} // namespace immer
