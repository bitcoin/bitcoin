// Copyright 2015-2019 Hans Dembinski
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_HISTOGRAM_DETAIL_MAKE_DEFAULT_HPP
#define BOOST_HISTOGRAM_DETAIL_MAKE_DEFAULT_HPP

namespace boost {
namespace histogram {
namespace detail {

template <class T>
T make_default_impl(const T& t, decltype(t.get_allocator(), 0)) {
  return T(t.get_allocator());
}

template <class T>
T make_default_impl(const T&, float) {
  return T{};
}

template <class T>
T make_default(const T& t) {
  return make_default_impl(t, 0);
}

} // namespace detail
} // namespace histogram
} // namespace boost

#endif
