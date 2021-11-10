// Copyright 2019 Hans Dembinski
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_HISTOGRAM_DETAIL_ARGUMENT_TRAITS_HPP
#define BOOST_HISTOGRAM_DETAIL_ARGUMENT_TRAITS_HPP

#include <boost/histogram/fwd.hpp>
#include <boost/mp11/algorithm.hpp>
#include <boost/mp11/integral.hpp>
#include <boost/mp11/list.hpp>
#include <tuple>

namespace boost {
namespace histogram {
namespace detail {

template <class T>
struct is_weight_impl : mp11::mp_false {};

template <class T>
struct is_weight_impl<weight_type<T>> : mp11::mp_true {};

template <class T>
using is_weight = is_weight_impl<T>;

template <class T>
struct is_sample_impl : mp11::mp_false {};

template <class T>
struct is_sample_impl<sample_type<T>> : mp11::mp_true {};

template <class T>
using is_sample = is_sample_impl<T>;

template <int Idx, class L>
struct sample_args_impl {
  using type = mp11::mp_first<std::decay_t<mp11::mp_at_c<L, (Idx >= 0 ? Idx : 0)>>>;
};

template <class L>
struct sample_args_impl<-1, L> {
  using type = std::tuple<>;
};

template <std::size_t NArgs, std::size_t Start, int WeightPos, int SamplePos,
          class SampleArgs>
struct argument_traits_holder {
  using nargs = mp11::mp_size_t<NArgs>;
  using start = mp11::mp_size_t<Start>;
  using wpos = mp11::mp_int<WeightPos>;
  using spos = mp11::mp_int<SamplePos>;
  using sargs = SampleArgs;
};

template <class... Ts>
struct argument_traits_impl {
  using list_ = mp11::mp_list<Ts...>;
  static constexpr std::size_t size_ = sizeof...(Ts);
  static constexpr std::size_t weight_ = mp11::mp_find_if<list_, is_weight>::value;
  static constexpr std::size_t sample_ = mp11::mp_find_if<list_, is_sample>::value;
  static constexpr int spos_ = (sample_ < size_ ? static_cast<int>(sample_) : -1);
  static constexpr int wpos_ = (weight_ < size_ ? static_cast<int>(weight_) : -1);

  using type =
      argument_traits_holder<(size_ - (weight_ < size_) - (sample_ < size_)),
                             (weight_ < size_ && sample_ < size_ &&
                                      (weight_ + sample_ < 2)
                                  ? 2
                                  : ((weight_ == 0 || sample_ == 0) ? 1 : 0)),
                             wpos_, spos_, typename sample_args_impl<spos_, list_>::type>;
};

template <class... Ts>
using argument_traits = typename argument_traits_impl<Ts...>::type;

} // namespace detail
} // namespace histogram
} // namespace boost

#endif
