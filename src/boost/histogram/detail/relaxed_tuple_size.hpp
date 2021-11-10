// Copyright 2019 Hans Dembinski
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_HISTOGRAM_DETAIL_RELAXED_TUPLE_SIZE_HPP
#define BOOST_HISTOGRAM_DETAIL_RELAXED_TUPLE_SIZE_HPP

#include <type_traits>

namespace boost {
namespace histogram {
namespace detail {

using dynamic_size = std::integral_constant<std::size_t, static_cast<std::size_t>(-1)>;

// Returns static size of tuple or dynamic_size
template <class T>
constexpr dynamic_size relaxed_tuple_size(const T&) noexcept {
  return {};
}

template <class... Ts>
constexpr std::integral_constant<std::size_t, sizeof...(Ts)> relaxed_tuple_size(
    const std::tuple<Ts...>&) noexcept {
  return {};
}

template <class T>
using relaxed_tuple_size_t = decltype(relaxed_tuple_size(std::declval<T>()));

} // namespace detail
} // namespace histogram
} // namespace boost

#endif
