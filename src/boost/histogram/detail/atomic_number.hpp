// Copyright 2021 Hans Dembinski
//
// Distributed under the Boost Software License, version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_HISTOGRAM_DETAIL_ATOMIC_NUMBER_HPP
#define BOOST_HISTOGRAM_DETAIL_ATOMIC_NUMBER_HPP

#include <atomic>
#include <boost/histogram/detail/priority.hpp>
#include <type_traits>

namespace boost {
namespace histogram {
namespace detail {

// copyable arithmetic type with thread-safe operator++ and operator+=
template <class T>
struct atomic_number : std::atomic<T> {
  static_assert(std::is_arithmetic<T>(), "");

  using base_t = std::atomic<T>;
  using std::atomic<T>::atomic;

  atomic_number() noexcept = default;
  atomic_number(const atomic_number& o) noexcept : std::atomic<T>{o.load()} {}
  atomic_number& operator=(const atomic_number& o) noexcept {
    this->store(o.load());
    return *this;
  }

  // not defined for float
  atomic_number& operator++() noexcept {
    increment_impl(static_cast<base_t&>(*this), priority<1>{});
    return *this;
  }

  // operator is not defined for floating point before C++20
  atomic_number& operator+=(const T& x) noexcept {
    add_impl(static_cast<base_t&>(*this), x, priority<1>{});
    return *this;
  }

  // not thread-safe
  atomic_number& operator*=(const T& x) noexcept {
    this->store(this->load() * x);
    return *this;
  }

private:
  // for integral types
  template <class U = T>
  auto increment_impl(std::atomic<U>& a, priority<1>) noexcept -> decltype(++a) {
    return ++a;
  }

  // fallback implementation for floating point types
  template <class U = T>
  void increment_impl(std::atomic<U>&, priority<0>) noexcept {
    this->operator+=(static_cast<U>(1));
  }

  // always available for integral types, in C++20 also available for float
  template <class U = T>
  static auto add_impl(std::atomic<U>& a, const U& x, priority<1>) noexcept
      -> decltype(a += x) {
    return a += x;
  }

  // pre-C++20 fallback implementation for floating point
  template <class U = T>
  static void add_impl(std::atomic<U>& a, const U& x, priority<0>) noexcept {
    U expected = a.load();
    // if another tread changed `expected` in the meantime, compare_exchange returns
    // false and updates `expected`; we then loop and try to update again;
    // see https://en.cppreference.com/w/cpp/atomic/atomic/compare_exchange
    while (!a.compare_exchange_weak(expected, expected + x))
      ;
  }
};
} // namespace detail
} // namespace histogram
} // namespace boost

#endif
