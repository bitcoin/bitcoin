// Copyright 2019 Hans Dembinski
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_HISTOGRAM_DETAIL_NONMEMBER_CONTAINER_ACCESS_HPP
#define BOOST_HISTOGRAM_DETAIL_NONMEMBER_CONTAINER_ACCESS_HPP

#if __cpp_lib_nonmember_container_access >= 201411

#include <iterator>

namespace boost {
namespace histogram {
namespace detail {

using std::data;
using std::size;

} // namespace detail
} // namespace histogram
} // namespace boost

#else // std implementations are not available

#include <initializer_list>
#include <type_traits>

namespace boost {
namespace histogram {
namespace detail {

template <class C>
constexpr auto data(C& c) -> decltype(c.data()) {
  return c.data();
}

template <class C>
constexpr auto data(const C& c) -> decltype(c.data()) {
  return c.data();
}

template <class T, std::size_t N>
constexpr T* data(T (&array)[N]) noexcept {
  return array;
}

template <class E>
constexpr const E* data(std::initializer_list<E> il) noexcept {
  return il.begin();
}

template <class C>
constexpr auto size(const C& c) -> decltype(c.size()) {
  return c.size();
}

template <class T, std::size_t N>
constexpr std::size_t size(const T (&)[N]) noexcept {
  return N;
}

} // namespace detail
} // namespace histogram
} // namespace boost

#endif

#endif // BOOST_HISTOGRAM_DETAIL_NONMEMBER_CONTAINER_ACCESS_HPP
