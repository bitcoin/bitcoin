// Copyright 2019 Hans Dembinski
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_HISTOGRAM_DETAIL_INDEX_TRANSLATOR_HPP
#define BOOST_HISTOGRAM_DETAIL_INDEX_TRANSLATOR_HPP

#include <algorithm>
#include <boost/histogram/axis/traits.hpp>
#include <boost/histogram/axis/variant.hpp>
#include <boost/histogram/detail/relaxed_equal.hpp>
#include <boost/histogram/detail/relaxed_tuple_size.hpp>
#include <boost/histogram/fwd.hpp>
#include <boost/histogram/multi_index.hpp>
#include <boost/mp11/algorithm.hpp>
#include <boost/mp11/integer_sequence.hpp>
#include <cassert>
#include <initializer_list>
#include <tuple>
#include <vector>

namespace boost {
namespace histogram {
namespace detail {

template <class A>
struct index_translator {
  using index_type = axis::index_type;
  using multi_index_type = multi_index<relaxed_tuple_size_t<A>::value>;
  using cref = const A&;

  cref dst, src;
  bool pass_through[buffer_size<A>::value];

  index_translator(cref d, cref s) : dst{d}, src{s} { init(dst, src); }

  template <class T>
  void init(const T& a, const T& b) {
    std::transform(a.begin(), a.end(), b.begin(), pass_through,
                   [](const auto& a, const auto& b) {
                     return axis::visit(
                         [&](const auto& a) {
                           using U = std::decay_t<decltype(a)>;
                           return relaxed_equal{}(a, axis::get<U>(b));
                         },
                         a);
                   });
  }

  template <class... Ts>
  void init(const std::tuple<Ts...>& a, const std::tuple<Ts...>& b) {
    using Seq = mp11::mp_iota_c<sizeof...(Ts)>;
    mp11::mp_for_each<Seq>([&](auto I) {
      pass_through[I] = relaxed_equal{}(std::get<I>(a), std::get<I>(b));
    });
  }

  template <class T>
  static index_type translate(const T& dst, const T& src, index_type i) noexcept {
    assert(axis::traits::is_continuous<T>::value == false); // LCOV_EXCL_LINE: unreachable
    return dst.index(src.value(i));
  }

  template <class... Ts, class It>
  void impl(const std::tuple<Ts...>& a, const std::tuple<Ts...>& b, It i,
            index_type* j) const noexcept {
    using Seq = mp11::mp_iota_c<sizeof...(Ts)>;
    mp11::mp_for_each<Seq>([&](auto I) {
      if (pass_through[I])
        *(j + I) = *(i + I);
      else
        *(j + I) = this->translate(std::get<I>(a), std::get<I>(b), *(i + I));
    });
  }

  template <class T, class It>
  void impl(const T& a, const T& b, It i, index_type* j) const noexcept {
    const bool* p = pass_through;
    for (unsigned k = 0; k < a.size(); ++k, ++i, ++j, ++p) {
      if (*p)
        *j = *i;
      else {
        const auto& bk = b[k];
        axis::visit(
            [&](const auto& ak) {
              using U = std::decay_t<decltype(ak)>;
              *j = this->translate(ak, axis::get<U>(bk), *i);
            },
            a[k]);
      }
    }
  }

  template <class Indices>
  auto operator()(const Indices& seq) const noexcept {
    auto mi = multi_index_type::create(seq.size());
    impl(dst, src, seq.begin(), mi.begin());
    return mi;
  }
};

template <class Axes>
auto make_index_translator(const Axes& dst, const Axes& src) noexcept {
  return index_translator<Axes>{dst, src};
}

} // namespace detail
} // namespace histogram
} // namespace boost

#endif
