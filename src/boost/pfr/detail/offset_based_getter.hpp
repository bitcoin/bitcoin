// Copyright (c) 2017-2018 Chris Beck
// Copyright (c) 2019-2021 Antony Polukhin
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PFR_DETAIL_OFFSET_BASED_GETTER_HPP
#define BOOST_PFR_DETAIL_OFFSET_BASED_GETTER_HPP
#pragma once

#include <boost/pfr/detail/config.hpp>

#include <type_traits>
#include <utility>
#include <boost/pfr/detail/sequence_tuple.hpp>
#include <boost/pfr/detail/rvalue_t.hpp>
#include <boost/pfr/detail/size_t_.hpp>


namespace boost { namespace pfr { namespace detail {

// Our own implementation of std::aligned_storage. On godbolt with MSVC, I have compilation errors
// using the standard version, it seems the compiler cannot generate default ctor.

template<std::size_t s, std::size_t a>
struct internal_aligned_storage {
   alignas(a) char storage_[s];
};

// Metafunction that replaces tuple<T1, T2, T3, ...> with
// tuple<std::aligned_storage_t<sizeof(T1), alignof(T1)>, std::aligned_storage<sizeof(T2), alignof(T2)>, ...>
//
// The point is, the new tuple is "layout compatible" in the sense that members have the same offsets,
// but this tuple is constexpr constructible.

template <typename T>
struct tuple_of_aligned_storage;

template <typename... Ts>
struct tuple_of_aligned_storage<sequence_tuple::tuple<Ts...>> {
  using type = sequence_tuple::tuple<internal_aligned_storage<sizeof(Ts),
#if defined(__GNUC__) && __GNUC__ < 8 && !defined(__x86_64__) && !defined(__CYGWIN__)
      // Before GCC-8 the `alignof` was returning the optimal alignment rather than the minimal one.
      // We have to adjust the alignemnt because otherwise we get the wrong offset.
      (alignof(Ts) > 4 ? 4 : alignof(Ts))
#else
      alignof(Ts)
#endif
  >...>;
};

// Note: If pfr has a typelist also, could also have an overload for that here

template <typename T>
using tuple_of_aligned_storage_t = typename tuple_of_aligned_storage<T>::type;

/***
 * Given a structure type and its sequence of members, we want to build a function
 * object "getter" that implements a version of `std::get` using offset arithmetic
 * and reinterpret_cast.
 *
 * typename U should be a user-defined struct
 * typename S should be a sequence_tuple which is layout compatible with U
 */

template <typename U, typename S>
class offset_based_getter {
  using this_t = offset_based_getter<U, S>;

  static_assert(sizeof(U) == sizeof(S), "====================> Boost.PFR: Member sequence does not indicate correct size for struct type! Maybe the user-provided type is not a SimpleAggregate?");
  static_assert(alignof(U) == alignof(S), "====================> Boost.PFR: Member sequence does not indicate correct alignment for struct type!");

  static_assert(!std::is_const<U>::value, "====================> Boost.PFR: const should be stripped from user-defined type when using offset_based_getter or overload resolution will be ambiguous later, this indicates an error within pfr");
  static_assert(!std::is_reference<U>::value, "====================> Boost.PFR: reference should be stripped from user-defined type when using offset_based_getter or overload resolution will be ambiguous later, this indicates an error within pfr");
  static_assert(!std::is_volatile<U>::value, "====================> Boost.PFR: volatile should be stripped from user-defined type when using offset_based_getter or overload resolution will be ambiguous later. this indicates an error within pfr");

  // Get type of idx'th member
  template <std::size_t idx>
  using index_t = typename sequence_tuple::tuple_element<idx, S>::type;

  // Get offset of idx'th member
  // Idea: Layout object has the same offsets as instance of S, so if S and U are layout compatible, then these offset
  // calculations are correct.
  template <std::size_t idx>
  static constexpr std::ptrdiff_t offset() noexcept {
    constexpr tuple_of_aligned_storage_t<S> layout{};
    return &sequence_tuple::get<idx>(layout).storage_[0] - &sequence_tuple::get<0>(layout).storage_[0];
  }

  // Encapsulates offset arithmetic and reinterpret_cast
  template <std::size_t idx>
  static index_t<idx> * get_pointer(U * u) noexcept {
    return reinterpret_cast<index_t<idx> *>(reinterpret_cast<char *>(u) + this_t::offset<idx>());
  }

  template <std::size_t idx>
  static const index_t<idx> * get_pointer(const U * u) noexcept {
    return reinterpret_cast<const index_t<idx> *>(reinterpret_cast<const char *>(u) + this_t::offset<idx>());
  }

  template <std::size_t idx>
  static volatile index_t<idx> * get_pointer(volatile U * u) noexcept {
    return reinterpret_cast<volatile index_t<idx> *>(reinterpret_cast<volatile char *>(u) + this_t::offset<idx>());
  }

  template <std::size_t idx>
  static const volatile index_t<idx> * get_pointer(const volatile U * u) noexcept {
    return reinterpret_cast<const volatile index_t<idx> *>(reinterpret_cast<const volatile char *>(u) + this_t::offset<idx>());
  }

public:
  template <std::size_t idx>
  index_t<idx> & get(U & u, size_t_<idx>) const noexcept {
    return *this_t::get_pointer<idx>(std::addressof(u));
  }

  template <std::size_t idx>
  index_t<idx> const & get(U const & u, size_t_<idx>) const noexcept {
    return *this_t::get_pointer<idx>(std::addressof(u));
  }

  template <std::size_t idx>
  index_t<idx> volatile & get(U volatile & u, size_t_<idx>) const noexcept {
    return *this_t::get_pointer<idx>(std::addressof(u));
  }

  template <std::size_t idx>
  index_t<idx> const volatile & get(U const volatile & u, size_t_<idx>) const noexcept {
    return *this_t::get_pointer<idx>(std::addressof(u));
  }

  // rvalues must not be used here, to avoid template instantiation bloats.
  template <std::size_t idx>
  index_t<idx> && get(rvalue_t<U> u, size_t_<idx>) const = delete;
};


}}} // namespace boost::pfr::detail

#endif // BOOST_PFR_DETAIL_OFFSET_LIST_HPP
