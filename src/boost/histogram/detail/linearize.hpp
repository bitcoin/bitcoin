// Copyright 2019 Hans Dembinski
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_HISTOGRAM_DETAIL_LINEARIZE_HPP
#define BOOST_HISTOGRAM_DETAIL_LINEARIZE_HPP

#include <boost/histogram/axis/option.hpp>
#include <boost/histogram/axis/traits.hpp>
#include <boost/histogram/axis/variant.hpp>
#include <boost/histogram/detail/optional_index.hpp>
#include <boost/histogram/fwd.hpp>
#include <boost/histogram/multi_index.hpp>
#include <cassert>

namespace boost {
namespace histogram {
namespace detail {

// initial offset to out must be set;
// this faster code can be used if all axes are inclusive
template <class Opts>
std::size_t linearize(Opts, std::size_t& out, const std::size_t stride,
                      const axis::index_type size, const axis::index_type idx) {
  constexpr bool u = Opts::test(axis::option::underflow);
  constexpr bool o = Opts::test(axis::option::overflow);
  assert(idx >= (u ? -1 : 0));
  assert(idx < (o ? size + 1 : size));
  assert(idx >= 0 || static_cast<std::size_t>(-idx * stride) <= out);
  out += idx * stride;
  return size + u + o;
}

// initial offset to out must be set
// this slower code must be used if not all axes are inclusive
template <class Opts>
std::size_t linearize(Opts, optional_index& out, const std::size_t stride,
                      const axis::index_type size, const axis::index_type idx) {
  constexpr bool u = Opts::test(axis::option::underflow);
  constexpr bool o = Opts::test(axis::option::overflow);
  assert(idx >= -1);
  assert(idx < size + 1);
  const bool is_valid = (u || idx >= 0) && (o || idx < size);
  if (is_valid)
    out += idx * stride;
  else
    out = invalid_index;
  return size + u + o;
}

template <class Index, class Axis, class Value>
std::size_t linearize(Index& out, const std::size_t stride, const Axis& ax,
                      const Value& v) {
  // mask options to reduce no. of template instantiations
  constexpr auto opts = axis::traits::get_options<Axis>{} &
                        (axis::option::underflow | axis::option::overflow);
  return linearize(opts, out, stride, ax.size(), axis::traits::index(ax, v));
}

// initial offset of out must be zero
template <class Index, class Axis, class Value>
std::size_t linearize_growth(Index& out, axis::index_type& shift,
                             const std::size_t stride, Axis& a, const Value& v) {
  axis::index_type idx;
  std::tie(idx, shift) = axis::traits::update(a, v);
  constexpr bool u = axis::traits::get_options<Axis>::test(axis::option::underflow);
  if (u) ++idx;
  if (std::is_same<Index, std::size_t>::value) {
    assert(idx < axis::traits::extent(a));
    out += idx * stride;
  } else {
    if (0 <= idx && idx < axis::traits::extent(a))
      out += idx * stride;
    else
      out = invalid_index;
  }
  return axis::traits::extent(a);
}

// initial offset of out must be zero
template <class A>
std::size_t linearize_index(optional_index& out, const std::size_t stride, const A& ax,
                            const axis::index_type idx) noexcept {
  const auto opt = axis::traits::get_options<A>();
  const axis::index_type begin = opt & axis::option::underflow ? -1 : 0;
  const axis::index_type end = opt & axis::option::overflow ? ax.size() + 1 : ax.size();
  const axis::index_type extent = end - begin;
  // i may be arbitrarily out of range
  if (begin <= idx && idx < end)
    out += (idx - begin) * stride;
  else
    out = invalid_index;
  return extent;
}

template <class A, std::size_t N>
optional_index linearize_indices(const A& axes, const multi_index<N>& indices) noexcept {
  assert(axes_rank(axes) == detail::size(indices));

  optional_index idx{0}; // offset not used by linearize_index
  auto stride = static_cast<std::size_t>(1);
  using std::begin;
  auto i = begin(indices);
  for_each_axis(axes,
                [&](const auto& a) { stride *= linearize_index(idx, stride, a, *i++); });
  return idx;
}

template <class Index, class... Ts, class Value>
std::size_t linearize(Index& o, const std::size_t s, const axis::variant<Ts...>& a,
                      const Value& v) {
  return axis::visit([&o, &s, &v](const auto& a) { return linearize(o, s, a, v); }, a);
}

template <class Index, class... Ts, class Value>
std::size_t linearize_growth(Index& o, axis::index_type& sh, const std::size_t st,
                             axis::variant<Ts...>& a, const Value& v) {
  return axis::visit([&](auto& a) { return linearize_growth(o, sh, st, a, v); }, a);
}

} // namespace detail
} // namespace histogram
} // namespace boost

#endif // BOOST_HISTOGRAM_DETAIL_LINEARIZE_HPP
