// Copyright 2019 Hans Dembinski
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_HISTOGRAM_DETAIL_TRY_CAST_HPP
#define BOOST_HISTOGRAM_DETAIL_TRY_CAST_HPP

#include <boost/config.hpp> // BOOST_NORETURN
#include <boost/throw_exception.hpp>
#include <type_traits>

namespace boost {
namespace histogram {
namespace detail {

template <class T, class U>
constexpr T* ptr_cast(U*) noexcept {
  return nullptr;
}

template <class T>
constexpr T* ptr_cast(T* p) noexcept {
  return p;
}

template <class T>
constexpr const T* ptr_cast(const T* p) noexcept {
  return p;
}

template <class T, class E, class U>
BOOST_NORETURN T try_cast_impl(std::false_type, std::false_type, U&&) {
  BOOST_THROW_EXCEPTION(E("type cast error"));
}

// converting cast
template <class T, class E, class U>
T try_cast_impl(std::false_type, std::true_type, U&& u) noexcept {
  return static_cast<T>(u); // cast to avoid warnings
}

// pass-through cast
template <class T, class E>
T&& try_cast_impl(std::true_type, std::true_type, T&& t) noexcept {
  return std::forward<T>(t);
}

// cast fails at runtime with exception E instead of compile-time, T must be a value
template <class T, class E, class U>
T try_cast(U&& u) noexcept(std::is_convertible<U, T>::value) {
  return try_cast_impl<T, E>(std::is_same<U, T>{}, std::is_convertible<U, T>{},
                             std::forward<U>(u));
}

} // namespace detail
} // namespace histogram
} // namespace boost

#endif
