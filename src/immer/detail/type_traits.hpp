//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#pragma once

#include <algorithm>
#include <iterator>
#include <memory>
#include <type_traits>
#include <utility>

namespace immer {
namespace detail {

template <typename... Ts>
struct make_void { using type = void; };

template <typename... Ts>
using void_t = typename make_void<Ts...>::type;

template <typename T, typename = void>
struct is_dereferenceable : std::false_type {};

template <typename T>
struct is_dereferenceable<T, void_t<decltype(*(std::declval<T&>()))>> :
    std::true_type {};

template <typename T>
constexpr bool is_dereferenceable_v = is_dereferenceable<T>::value;

template<typename T, typename U=T, typename = void>
struct is_equality_comparable : std::false_type {};

template<typename T, typename U>
struct is_equality_comparable
<T, U, std::enable_if_t
 <std::is_same
  <bool, decltype(std::declval<T&>() == std::declval<U&>())>::value>> :
    std::true_type {};

template<typename T, typename U=T>
constexpr bool is_equality_comparable_v = is_equality_comparable<T, U>::value;

template<typename T, typename U=T, typename = void>
struct is_inequality_comparable : std::false_type {};

template<typename T, typename U>
struct is_inequality_comparable
<T, U, std::enable_if_t
 <std::is_same
  <bool, decltype(std::declval<T&>() != std::declval<U&>())>::value>> :
    std::true_type {};

template<typename T, typename U=T>
constexpr bool is_inequality_comparable_v =
  is_inequality_comparable<T, U>::value;

template <typename T, typename = void>
struct is_preincrementable : std::false_type {};

template <typename T>
struct is_preincrementable
<T, std::enable_if_t
 <std::is_same<T&, decltype(++(std::declval<T&>()))>::value>> :
    std::true_type {};

template <typename T>
constexpr bool is_preincrementable_v = is_preincrementable<T>::value;

template <typename T, typename U=T, typename = void>
struct is_subtractable : std::false_type {};

template <typename T, typename U>
struct is_subtractable
<T, U, void_t<decltype(std::declval<T&>() - std::declval<U&>())>> :
    std::true_type {};

template <typename T, typename U = T>
constexpr bool is_subtractable_v = is_subtractable<T, U>::value;

namespace swappable {

using std::swap;

template <typename T, typename U, typename = void>
struct with : std::false_type {};

// Does not account for non-referenceable types
template <typename T, typename U>
struct with
<T, U, void_t<decltype(swap(std::declval<T&>(), std::declval<U&>())),
              decltype(swap(std::declval<U&>(), std::declval<T&>()))>> :
    std::true_type {};

}

template<typename T, typename U>
using is_swappable_with = swappable::with<T, U>;

template<typename T>
using is_swappable = is_swappable_with<T, T>;

template <typename T>
constexpr bool is_swappable_v = is_swappable_with<T&, T&>::value;

template <typename T, typename = void>
struct is_iterator : std::false_type {};

// See http://en.cppreference.com/w/cpp/concept/Iterator
template <typename T>
struct is_iterator
<T, void_t
 <std::enable_if_t
  <is_preincrementable_v<T>
   && is_dereferenceable_v<T>
   // accounts for non-referenceable types
   && std::is_copy_constructible<T>::value
   && std::is_copy_assignable<T>::value
   && std::is_destructible<T>::value
   && is_swappable_v<T>>,
  typename std::iterator_traits<T>::value_type,
  typename std::iterator_traits<T>::difference_type,
  typename std::iterator_traits<T>::reference,
  typename std::iterator_traits<T>::pointer,
  typename std::iterator_traits<T>::iterator_category>> :
    std::true_type {};

template<typename T>
constexpr bool is_iterator_v = is_iterator<T>::value;

template<typename T, typename U, typename = void>
struct compatible_sentinel : std::false_type {};

template<typename T, typename U>
struct compatible_sentinel
<T, U, std::enable_if_t
 <is_iterator_v<T>
  && is_equality_comparable_v<T, U>
  && is_inequality_comparable_v<T, U>>> :
    std::true_type {};

template<typename T, typename U>
constexpr bool compatible_sentinel_v = compatible_sentinel<T,U>::value;

template<typename T, typename = void>
struct is_forward_iterator : std::false_type {};

template<typename T>
struct is_forward_iterator
<T, std::enable_if_t
 <is_iterator_v<T> &&
  std::is_base_of
  <std::forward_iterator_tag,
   typename std::iterator_traits<T>::iterator_category>::value>> :
    std::true_type {};

template<typename T>
constexpr bool is_forward_iterator_v = is_forward_iterator<T>::value;

template<typename T, typename U, typename = void>
struct std_distance_supports : std::false_type {};

template<typename T, typename U>
struct std_distance_supports
<T, U, void_t<decltype(std::distance(std::declval<T>(), std::declval<U>()))>> :
  std::true_type {};

template<typename T, typename U>
constexpr bool std_distance_supports_v = std_distance_supports<T, U>::value;

template<typename T, typename U, typename V, typename = void>
struct std_uninitialized_copy_supports : std::false_type {};

template<typename T, typename U, typename V>
struct std_uninitialized_copy_supports
<T, U, V, void_t<decltype(std::uninitialized_copy(std::declval<T>(),
						  std::declval<U>(),
						  std::declval<V>()))>> :
  std::true_type {};

template<typename T, typename U, typename V>
constexpr bool std_uninitialized_copy_supports_v =
  std_uninitialized_copy_supports<T, U, V>::value;

}
}
