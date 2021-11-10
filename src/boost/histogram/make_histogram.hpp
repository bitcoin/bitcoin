// Copyright 2015-2018 Hans Dembinski
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_HISTOGRAM_MAKE_HISTOGRAM_HPP
#define BOOST_HISTOGRAM_MAKE_HISTOGRAM_HPP

/**
  \file boost/histogram/make_histogram.hpp
  Collection of factory functions to conveniently create histograms.
*/

#include <boost/histogram/accumulators/weighted_sum.hpp>
#include <boost/histogram/detail/detect.hpp>
#include <boost/histogram/histogram.hpp>
#include <boost/histogram/storage_adaptor.hpp>
#include <boost/histogram/unlimited_storage.hpp> // = default_storage
#include <boost/mp11/utility.hpp>
#include <tuple>
#include <vector>

namespace boost {
namespace histogram {

/**
  Make histogram from compile-time axis configuration and custom storage.
  @param storage Storage or container with standard interface (any vector, array, or map).
  @param axis First axis instance.
  @param axes Other axis instances.
*/
template <class Storage, class Axis, class... Axes,
          class = detail::requires_storage_or_adaptible<Storage>,
          class = detail::requires_axis<Axis>>
auto make_histogram_with(Storage&& storage, Axis&& axis, Axes&&... axes) {
  auto a = std::make_tuple(std::forward<Axis>(axis), std::forward<Axes>(axes)...);
  using U = std::decay_t<Storage>;
  using S = mp11::mp_if<detail::is_storage<U>, U, storage_adaptor<U>>;
  return histogram<decltype(a), S>(std::move(a), S(std::forward<Storage>(storage)));
}

/**
  Make histogram from compile-time axis configuration and default storage.
  @param axis First axis instance.
  @param axes Other axis instances.
*/
template <class Axis, class... Axes, class = detail::requires_axis<Axis>>
auto make_histogram(Axis&& axis, Axes&&... axes) {
  return make_histogram_with(default_storage(), std::forward<Axis>(axis),
                             std::forward<Axes>(axes)...);
}

/**
  Make histogram from compile-time axis configuration and weight-counting storage.
  @param axis First axis instance.
  @param axes Other axis instances.
*/
template <class Axis, class... Axes, class = detail::requires_axis<Axis>>
auto make_weighted_histogram(Axis&& axis, Axes&&... axes) {
  return make_histogram_with(weight_storage(), std::forward<Axis>(axis),
                             std::forward<Axes>(axes)...);
}

/**
  Make histogram from iterable range and custom storage.
  @param storage Storage or container with standard interface (any vector, array, or map).
  @param iterable Iterable range of axis objects.
*/
template <class Storage, class Iterable,
          class = detail::requires_storage_or_adaptible<Storage>,
          class = detail::requires_sequence_of_any_axis<Iterable>>
auto make_histogram_with(Storage&& storage, Iterable&& iterable) {
  using U = std::decay_t<Storage>;
  using S = mp11::mp_if<detail::is_storage<U>, U, storage_adaptor<U>>;
  using It = std::decay_t<Iterable>;
  using A = mp11::mp_if<detail::is_indexable_container<It>, It,
                        std::vector<mp11::mp_first<It>>>;
  return histogram<A, S>(std::forward<Iterable>(iterable),
                         S(std::forward<Storage>(storage)));
}

/**
  Make histogram from iterable range and default storage.
  @param iterable Iterable range of axis objects.
*/
template <class Iterable, class = detail::requires_sequence_of_any_axis<Iterable>>
auto make_histogram(Iterable&& iterable) {
  return make_histogram_with(default_storage(), std::forward<Iterable>(iterable));
}

/**
  Make histogram from iterable range and weight-counting storage.
  @param iterable Iterable range of axis objects.
*/
template <class Iterable, class = detail::requires_sequence_of_any_axis<Iterable>>
auto make_weighted_histogram(Iterable&& iterable) {
  return make_histogram_with(weight_storage(), std::forward<Iterable>(iterable));
}

/**
  Make histogram from iterator interval and custom storage.
  @param storage Storage or container with standard interface (any vector, array, or map).
  @param begin Iterator to range of axis objects.
  @param end   Iterator to range of axis objects.
*/
template <class Storage, class Iterator,
          class = detail::requires_storage_or_adaptible<Storage>,
          class = detail::requires_iterator<Iterator>>
auto make_histogram_with(Storage&& storage, Iterator begin, Iterator end) {
  using T = std::decay_t<decltype(*begin)>;
  return make_histogram_with(std::forward<Storage>(storage), std::vector<T>(begin, end));
}

/**
  Make histogram from iterator interval and default storage.
  @param begin Iterator to range of axis objects.
  @param end   Iterator to range of axis objects.
*/
template <class Iterator, class = detail::requires_iterator<Iterator>>
auto make_histogram(Iterator begin, Iterator end) {
  return make_histogram_with(default_storage(), begin, end);
}

/**
  Make histogram from iterator interval and weight-counting storage.
  @param begin Iterator to range of axis objects.
  @param end   Iterator to range of axis objects.
*/
template <class Iterator, class = detail::requires_iterator<Iterator>>
auto make_weighted_histogram(Iterator begin, Iterator end) {
  return make_histogram_with(weight_storage(), begin, end);
}

} // namespace histogram
} // namespace boost

#endif
