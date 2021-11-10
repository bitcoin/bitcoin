// Copyright 2015-2017 Hans Dembinski
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_HISTOGRAM_LITERALS_HPP
#define BOOST_HISTOGRAM_LITERALS_HPP

#include <type_traits>

namespace boost {
namespace histogram {
namespace detail {
constexpr unsigned parse_number(unsigned n) { return n; }

template <class... Rest>
constexpr unsigned parse_number(unsigned n, char f, Rest... rest) {
  return parse_number(10u * n + static_cast<unsigned>(f - '0'), rest...);
}
} // namespace detail

namespace literals {
/// Suffix operator to generate literal compile-time numbers, 0_c, 12_c, etc.
template <char... digits>
auto operator"" _c() {
  return std::integral_constant<unsigned, detail::parse_number(0, digits...)>();
}
} // namespace literals
} // namespace histogram
} // namespace boost

#endif
