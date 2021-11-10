// Copyright 2021 Hans Dembinski
//
// Distributed under the Boost Software License, version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_HISTOGRAM_ACCUMULATORS_IS_THREAD_SAFE_HPP
#define BOOST_HISTOGRAM_ACCUMULATORS_IS_THREAD_SAFE_HPP

#include <boost/histogram/detail/priority.hpp>
#include <boost/histogram/fwd.hpp>
#include <type_traits>

namespace boost {
namespace histogram {
namespace detail {

template <class T>
constexpr bool is_thread_safe_impl(priority<0>) {
  return false;
}

template <class T>
constexpr auto is_thread_safe_impl(priority<1>) -> decltype(T::thread_safe()) {
  return T::thread_safe();
}

} // namespace detail

namespace accumulators {

template <class T>
struct is_thread_safe
    : std::integral_constant<bool,
                             detail::is_thread_safe_impl<T>(detail::priority<1>{})> {};

} // namespace accumulators
} // namespace histogram
} // namespace boost

#endif
