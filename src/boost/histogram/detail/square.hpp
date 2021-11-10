// Copyright 2018-2019 Hans Dembinski
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_HISTOGRAM_DETAIL_SQUARE_HPP
#define BOOST_HISTOGRAM_DETAIL_SQUARE_HPP

namespace boost {
namespace histogram {
namespace detail {

template <class T>
T square(T t) {
  return t * t;
}

} // namespace detail
} // namespace histogram
} // namespace boost

#endif
