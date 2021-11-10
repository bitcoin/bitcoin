// Copyright 2020 Hans Dembinski
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_HISTOGRAM_MULTI_INDEX_HPP
#define BOOST_HISTOGRAM_MULTI_INDEX_HPP

#include <algorithm>
#include <boost/histogram/detail/detect.hpp>
#include <boost/histogram/detail/nonmember_container_access.hpp>
#include <boost/histogram/fwd.hpp>
#include <boost/mp11/integer_sequence.hpp>
#include <boost/throw_exception.hpp>
#include <initializer_list>
#include <iterator>
#include <stdexcept>
#include <tuple>

namespace boost {
namespace histogram {

/** Holder for multiple axis indices.

  Adapts external iterable, tuple, or explicit list of indices to the same representation.
 */
template <std::size_t Size>
struct multi_index {
  using value_type = axis::index_type;
  using iterator = value_type*;
  using const_iterator = const value_type*;

  static multi_index create(std::size_t s) {
    if (s != size())
      BOOST_THROW_EXCEPTION(std::invalid_argument("size does not match static size"));
    return multi_index(priv_tag{});
  }

  template <class... Is>
  multi_index(axis::index_type i, Is... is)
      : multi_index(std::initializer_list<axis::index_type>{
            i, static_cast<axis::index_type>(is)...}) {}

  template <class... Is>
  multi_index(const std::tuple<axis::index_type, Is...>& is)
      : multi_index(is, mp11::make_index_sequence<(1 + sizeof...(Is))>{}) {}

  template <class Iterable, class = detail::requires_iterable<Iterable>>
  multi_index(const Iterable& is) {
    if (detail::size(is) != size())
      BOOST_THROW_EXCEPTION(std::invalid_argument("no. of axes != no. of indices"));
    using std::begin;
    using std::end;
    std::copy(begin(is), end(is), data_);
  }

  iterator begin() noexcept { return data_; }
  iterator end() noexcept { return data_ + size(); }
  const_iterator begin() const noexcept { return data_; }
  const_iterator end() const noexcept { return data_ + size(); }
  static constexpr std::size_t size() noexcept { return Size; }

private:
  struct priv_tag {};

  multi_index(priv_tag) {}

  template <class T, std::size_t... Is>
  multi_index(const T& is, mp11::index_sequence<Is...>)
      : multi_index(static_cast<axis::index_type>(std::get<Is>(is))...) {}

  axis::index_type data_[size()];
};

template <>
struct multi_index<static_cast<std::size_t>(-1)> {
  using value_type = axis::index_type;
  using iterator = value_type*;
  using const_iterator = const value_type*;

  static multi_index create(std::size_t s) { return multi_index(priv_tag{}, s); }

  template <class... Is>
  multi_index(axis::index_type i, Is... is)
      : multi_index(std::initializer_list<axis::index_type>{
            i, static_cast<axis::index_type>(is)...}) {}

  template <class... Is>
  multi_index(const std::tuple<axis::index_type, Is...>& is)
      : multi_index(is, mp11::make_index_sequence<(1 + sizeof...(Is))>{}) {}

  template <class Iterable, class = detail::requires_iterable<Iterable>>
  multi_index(const Iterable& is) : size_(detail::size(is)) {
    using std::begin;
    using std::end;
    std::copy(begin(is), end(is), data_);
  }

  iterator begin() noexcept { return data_; }
  iterator end() noexcept { return data_ + size_; }
  const_iterator begin() const noexcept { return data_; }
  const_iterator end() const noexcept { return data_ + size_; }
  std::size_t size() const noexcept { return size_; }

private:
  struct priv_tag {};

  multi_index(priv_tag, std::size_t s) : size_(s) {}

  template <class T, std::size_t... Ns>
  multi_index(const T& is, mp11::index_sequence<Ns...>)
      : multi_index(static_cast<axis::index_type>(std::get<Ns>(is))...) {}

  std::size_t size_ = 0;
  static constexpr std::size_t max_size_ = BOOST_HISTOGRAM_DETAIL_AXES_LIMIT;
  axis::index_type data_[max_size_];
};

} // namespace histogram
} // namespace boost

#endif
