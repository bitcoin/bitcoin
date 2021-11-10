// Copyright 2015-2019 Hans Dembinski
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_HISTOGRAM_DETAIL_RELAXED_EQUAL_HPP
#define BOOST_HISTOGRAM_DETAIL_RELAXED_EQUAL_HPP

#include <boost/histogram/detail/priority.hpp>
#include <type_traits>

namespace boost {
namespace histogram {
namespace detail {

struct relaxed_equal {
  template <class T, class U>
  constexpr auto impl(const T& t, const U& u, priority<1>) const noexcept
      -> decltype(t == u) const {
    return t == u;
  }

  // consider T and U not equal, if there is no operator== defined for them
  template <class T, class U>
  constexpr bool impl(const T&, const U&, priority<0>) const noexcept {
    return false;
  }

  // consider two T equal if they are stateless
  template <class T>
  constexpr bool impl(const T&, const T&, priority<0>) const noexcept {
    return std::is_empty<T>::value;
  }

  template <class T, class U>
  constexpr bool operator()(const T& t, const U& u) const noexcept {
    return impl(t, u, priority<1>{});
  }
};

} // namespace detail
} // namespace histogram
} // namespace boost

#endif
