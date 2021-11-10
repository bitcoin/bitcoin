// Copyright 2019 Hans Dembinski
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_HISTOGRAM_DETAIL_OPTIONAL_INDEX_HPP
#define BOOST_HISTOGRAM_DETAIL_OPTIONAL_INDEX_HPP

#include <cassert>
#include <cstdint>

namespace boost {
namespace histogram {
namespace detail {

constexpr auto invalid_index = ~static_cast<std::size_t>(0);

// integer with a persistent invalid state, similar to NaN
struct optional_index {
  std::size_t value;

  optional_index& operator=(std::size_t x) noexcept {
    value = x;
    return *this;
  }

  optional_index& operator+=(std::intptr_t x) noexcept {
    assert(x >= 0 || static_cast<std::size_t>(-x) <= value);
    if (value != invalid_index) { value += x; }
    return *this;
  }

  optional_index& operator+=(const optional_index& x) noexcept {
    if (value != invalid_index) return operator+=(x.value);
    value = invalid_index;
    return *this;
  }

  operator std::size_t() const noexcept { return value; }

  friend bool operator<=(std::size_t x, optional_index idx) noexcept {
    return x <= idx.value;
  }
};

constexpr inline bool is_valid(const std::size_t) noexcept { return true; }

inline bool is_valid(const optional_index x) noexcept { return x.value != invalid_index; }

} // namespace detail
} // namespace histogram
} // namespace boost

#endif
