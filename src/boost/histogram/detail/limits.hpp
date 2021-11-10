// Copyright 2015-2019 Hans Dembinski
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_HISTOGRAM_DETAIL_LIMITS_HPP
#define BOOST_HISTOGRAM_DETAIL_LIMITS_HPP

#include <limits>

namespace boost {
namespace histogram {
namespace detail {

template <class T>
constexpr T lowest() {
  return std::numeric_limits<T>::lowest();
}

template <>
constexpr double lowest() {
  return -std::numeric_limits<double>::infinity();
}

template <>
constexpr float lowest() {
  return -std::numeric_limits<float>::infinity();
}

template <class T>
constexpr T highest() {
  return (std::numeric_limits<T>::max)();
}

template <>
constexpr double highest() {
  return std::numeric_limits<double>::infinity();
}

template <>
constexpr float highest() {
  return std::numeric_limits<float>::infinity();
}

} // namespace detail
} // namespace histogram
} // namespace boost

#endif
