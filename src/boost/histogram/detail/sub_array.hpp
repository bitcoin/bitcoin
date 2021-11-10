// Copyright 2019 Hans Dembinski
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_HISTOGRAM_DETAIL_SUB_ARRAY_HPP
#define BOOST_HISTOGRAM_DETAIL_SUB_ARRAY_HPP

#include <algorithm>
#include <boost/throw_exception.hpp>
#include <stdexcept>

namespace boost {
namespace histogram {
namespace detail {

template <class T, std::size_t N>
class sub_array {
  constexpr bool swap_element_is_noexcept() noexcept {
    using std::swap;
    return noexcept(swap(std::declval<T&>(), std::declval<T&>()));
  }

public:
  using value_type = T;
  using size_type = std::size_t;
  using reference = T&;
  using const_reference = const T&;
  using pointer = T*;
  using const_pointer = const T*;
  using iterator = pointer;
  using const_iterator = const_pointer;

  sub_array() = default;

  explicit sub_array(std::size_t s) noexcept : size_(s) { assert(size_ <= N); }

  sub_array(std::size_t s, const T& value) noexcept(
      std::is_nothrow_assignable<T, const_reference>::value)
      : sub_array(s) {
    fill(value);
  }

  reference at(size_type pos) noexcept {
    if (pos >= size()) BOOST_THROW_EXCEPTION(std::out_of_range{"pos is out of range"});
    return data_[pos];
  }

  const_reference at(size_type pos) const noexcept {
    if (pos >= size()) BOOST_THROW_EXCEPTION(std::out_of_range{"pos is out of range"});
    return data_[pos];
  }

  reference operator[](size_type pos) noexcept { return data_[pos]; }
  const_reference operator[](size_type pos) const noexcept { return data_[pos]; }

  reference front() noexcept { return data_[0]; }
  const_reference front() const noexcept { return data_[0]; }

  reference back() noexcept { return data_[size_ - 1]; }
  const_reference back() const noexcept { return data_[size_ - 1]; }

  pointer data() noexcept { return static_cast<pointer>(data_); }
  const_pointer data() const noexcept { return static_cast<const_pointer>(data_); }

  iterator begin() noexcept { return data_; }
  const_iterator begin() const noexcept { return data_; }

  iterator end() noexcept { return begin() + size_; }
  const_iterator end() const noexcept { return begin() + size_; }

  const_iterator cbegin() noexcept { return data_; }
  const_iterator cbegin() const noexcept { return data_; }

  const_iterator cend() noexcept { return cbegin() + size_; }
  const_iterator cend() const noexcept { return cbegin() + size_; }

  constexpr size_type max_size() const noexcept { return N; }
  size_type size() const noexcept { return size_; }
  bool empty() const noexcept { return size_ == 0; }

  void fill(const_reference value) noexcept(
      std::is_nothrow_assignable<T, const_reference>::value) {
    std::fill(begin(), end(), value);
  }

  void swap(sub_array& other) noexcept(swap_element_is_noexcept()) {
    using std::swap;
    for (auto i = begin(), j = other.begin(); i != end(); ++i, ++j) swap(*i, *j);
  }

private:
  size_type size_ = 0;
  value_type data_[N];
};

template <class T, std::size_t N>
bool operator==(const sub_array<T, N>& a, const sub_array<T, N>& b) noexcept {
  return std::equal(a.begin(), a.end(), b.begin());
}

template <class T, std::size_t N>
bool operator!=(const sub_array<T, N>& a, const sub_array<T, N>& b) noexcept {
  return !(a == b);
}

} // namespace detail
} // namespace histogram
} // namespace boost

namespace std {
template <class T, std::size_t N>
void swap(::boost::histogram::detail::sub_array<T, N>& a,
          ::boost::histogram::detail::sub_array<T, N>& b) noexcept(noexcept(a.swap(b))) {
  a.swap(b);
}
} // namespace std

#endif
