// Copyright 2019 Hans Dembinski
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_HISTOGRAM_DETAIL_ACCUMULATOR_TRAITS_HPP
#define BOOST_HISTOGRAM_DETAIL_ACCUMULATOR_TRAITS_HPP

#include <boost/histogram/detail/priority.hpp>
#include <boost/histogram/fwd.hpp>
#include <tuple>
#include <type_traits>

namespace boost {

// forward declare accumulator_set so that it can be matched below
namespace accumulators {
template <class, class, class>
struct accumulator_set;
}

namespace histogram {
namespace detail {

template <bool WeightSupport, class... Ts>
struct accumulator_traits_holder {
  static constexpr bool weight_support = WeightSupport;
  using args = std::tuple<Ts...>;
};

// member function pointer with weight_type as first argument is better match
template <class R, class T, class U, class... Ts>
accumulator_traits_holder<true, Ts...> accumulator_traits_impl_call_op(
    R (T::*)(boost::histogram::weight_type<U>, Ts...));

template <class R, class T, class U, class... Ts>
accumulator_traits_holder<true, Ts...> accumulator_traits_impl_call_op(
    R (T::*)(boost::histogram::weight_type<U>&, Ts...));

template <class R, class T, class U, class... Ts>
accumulator_traits_holder<true, Ts...> accumulator_traits_impl_call_op(
    R (T::*)(boost::histogram::weight_type<U>&&, Ts...));

template <class R, class T, class U, class... Ts>
accumulator_traits_holder<true, Ts...> accumulator_traits_impl_call_op(
    R (T::*)(const boost::histogram::weight_type<U>&, Ts...));

// member function pointer only considered if all specializations above fail
template <class R, class T, class... Ts>
accumulator_traits_holder<false, Ts...> accumulator_traits_impl_call_op(R (T::*)(Ts...));

template <class T>
auto accumulator_traits_impl(T&, priority<2>)
    -> decltype(accumulator_traits_impl_call_op(&T::operator()));

template <class T>
auto accumulator_traits_impl(T&, priority<2>)
    -> decltype(std::declval<T&>() += std::declval<boost::histogram::weight_type<int>>(),
                accumulator_traits_holder<true>{});

// fallback for simple arithmetic types that do not implement adding a weight_type
template <class T>
auto accumulator_traits_impl(T&, priority<1>)
    -> decltype(std::declval<T&>() += 0, accumulator_traits_holder<true>{});

template <class T>
auto accumulator_traits_impl(T&, priority<0>) -> accumulator_traits_holder<false>;

// for boost.accumulators compatibility
template <class S, class F, class W>
accumulator_traits_holder<false, S> accumulator_traits_impl(
    boost::accumulators::accumulator_set<S, F, W>&, priority<2>) {
  static_assert(std::is_same<W, void>::value,
                "accumulator_set with weights is not directly supported, please use "
                "a wrapper class that implements the Accumulator concept");
}

template <class T>
using accumulator_traits =
    decltype(accumulator_traits_impl(std::declval<T&>(), priority<2>{}));

} // namespace detail
} // namespace histogram
} // namespace boost

#endif
