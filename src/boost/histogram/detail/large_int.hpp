// Copyright 2018-2019 Hans Dembinski
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_HISTOGRAM_DETAIL_LARGE_INT_HPP
#define BOOST_HISTOGRAM_DETAIL_LARGE_INT_HPP

#include <boost/histogram/detail/operators.hpp>
#include <boost/histogram/detail/safe_comparison.hpp>
#include <boost/mp11/algorithm.hpp>
#include <boost/mp11/function.hpp>
#include <boost/mp11/list.hpp>
#include <boost/mp11/utility.hpp>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <limits>
#include <type_traits>
#include <utility>
#include <vector>

namespace boost {
namespace histogram {
namespace detail {

template <class T>
using is_unsigned_integral = mp11::mp_and<std::is_integral<T>, std::is_unsigned<T>>;

template <class T>
bool safe_increment(T& t) {
  if (t < (std::numeric_limits<T>::max)()) {
    ++t;
    return true;
  }
  return false;
}

template <class T, class U>
bool safe_radd(T& t, const U& u) {
  static_assert(is_unsigned_integral<T>::value, "T must be unsigned integral type");
  static_assert(is_unsigned_integral<U>::value, "T must be unsigned integral type");
  if (static_cast<T>((std::numeric_limits<T>::max)() - t) >= u) {
    t += static_cast<T>(u); // static_cast to suppress conversion warning
    return true;
  }
  return false;
}

// An integer type which can grow arbitrarily large (until memory is exhausted).
// Use boost.multiprecision.cpp_int in your own code, it is much more sophisticated.
// We use it only to reduce coupling between boost libs.
template <class Allocator>
struct large_int : totally_ordered<large_int<Allocator>, large_int<Allocator>>,
                   partially_ordered<large_int<Allocator>, void> {
  explicit large_int(const Allocator& a = {}) : data(1, 0, a) {}
  explicit large_int(std::uint64_t v, const Allocator& a = {}) : data(1, v, a) {}

  large_int(const large_int&) = default;
  large_int& operator=(const large_int&) = default;
  large_int(large_int&&) = default;
  large_int& operator=(large_int&&) = default;

  large_int& operator=(std::uint64_t o) {
    data = decltype(data)(1, o);
    return *this;
  }

  large_int& operator++() {
    assert(data.size() > 0u);
    std::size_t i = 0;
    while (!safe_increment(data[i])) {
      data[i] = 0;
      ++i;
      if (i == data.size()) {
        data.push_back(1);
        break;
      }
    }
    return *this;
  }

  large_int& operator+=(const large_int& o) {
    if (this == &o) {
      auto tmp{o};
      return operator+=(tmp);
    }
    bool carry = false;
    std::size_t i = 0;
    for (std::uint64_t oi : o.data) {
      auto& di = maybe_extend(i);
      if (carry) {
        if (safe_increment(oi))
          carry = false;
        else {
          ++i;
          continue;
        }
      }
      if (!safe_radd(di, oi)) {
        add_remainder(di, oi);
        carry = true;
      }
      ++i;
    }
    while (carry) {
      auto& di = maybe_extend(i);
      if (safe_increment(di)) break;
      di = 0;
      ++i;
    }
    return *this;
  }

  large_int& operator+=(std::uint64_t o) {
    assert(data.size() > 0u);
    if (safe_radd(data[0], o)) return *this;
    add_remainder(data[0], o);
    // carry the one, data may grow several times
    std::size_t i = 1;
    while (true) {
      auto& di = maybe_extend(i);
      if (safe_increment(di)) break;
      di = 0;
      ++i;
    }
    return *this;
  }

  explicit operator double() const noexcept {
    assert(data.size() > 0u);
    double result = static_cast<double>(data[0]);
    std::size_t i = 0;
    while (++i < data.size())
      result += static_cast<double>(data[i]) * std::pow(2.0, i * 64);
    return result;
  }

  bool operator<(const large_int& o) const noexcept {
    assert(data.size() > 0u);
    assert(o.data.size() > 0u);
    // no leading zeros allowed
    assert(data.size() == 1 || data.back() > 0u);
    assert(o.data.size() == 1 || o.data.back() > 0u);
    if (data.size() < o.data.size()) return true;
    if (data.size() > o.data.size()) return false;
    auto s = data.size();
    while (s > 0u) {
      --s;
      if (data[s] < o.data[s]) return true;
      if (data[s] > o.data[s]) return false;
    }
    return false; // args are equal
  }

  bool operator==(const large_int& o) const noexcept {
    assert(data.size() > 0u);
    assert(o.data.size() > 0u);
    // no leading zeros allowed
    assert(data.size() == 1 || data.back() > 0u);
    assert(o.data.size() == 1 || o.data.back() > 0u);
    if (data.size() != o.data.size()) return false;
    return std::equal(data.begin(), data.end(), o.data.begin());
  }

  template <class U>
  std::enable_if_t<std::is_integral<U>::value, bool> operator<(
      const U& o) const noexcept {
    assert(data.size() > 0u);
    return data.size() == 1 && safe_less()(data[0], o);
  }

  template <class U>
  std::enable_if_t<std::is_integral<U>::value, bool> operator>(
      const U& o) const noexcept {
    assert(data.size() > 0u);
    assert(data.size() == 1 || data.back() > 0u); // no leading zeros allowed
    return data.size() > 1 || safe_less()(o, data[0]);
  }

  template <class U>
  std::enable_if_t<std::is_integral<U>::value, bool> operator==(
      const U& o) const noexcept {
    assert(data.size() > 0u);
    return data.size() == 1 && safe_equal()(data[0], o);
  }

  template <class U>
  std::enable_if_t<std::is_floating_point<U>::value, bool> operator<(
      const U& o) const noexcept {
    return operator double() < o;
  }

  template <class U>
  std::enable_if_t<std::is_floating_point<U>::value, bool> operator>(
      const U& o) const noexcept {
    return operator double() > o;
  }

  template <class U>
  std::enable_if_t<std::is_floating_point<U>::value, bool> operator==(
      const U& o) const noexcept {
    return operator double() == o;
  }

  template <class U>
  std::enable_if_t<
      (!std::is_arithmetic<U>::value && std::is_convertible<U, double>::value), bool>
  operator<(const U& o) const noexcept {
    return operator double() < o;
  }

  template <class U>
  std::enable_if_t<
      (!std::is_arithmetic<U>::value && std::is_convertible<U, double>::value), bool>
  operator>(const U& o) const noexcept {
    return operator double() > o;
  }

  template <class U>
  std::enable_if_t<
      (!std::is_arithmetic<U>::value && std::is_convertible<U, double>::value), bool>
  operator==(const U& o) const noexcept {
    return operator double() == o;
  }

  std::uint64_t& maybe_extend(std::size_t i) {
    while (i >= data.size()) data.push_back(0);
    return data[i];
  }

  static void add_remainder(std::uint64_t& d, const std::uint64_t o) noexcept {
    assert(d > 0u);
    // in decimal system it would look like this:
    // 8 + 8 = 6 = 8 - (9 - 8) - 1
    // 9 + 1 = 0 = 9 - (9 - 1) - 1
    auto tmp = (std::numeric_limits<std::uint64_t>::max)();
    tmp -= o;
    --d -= tmp;
  }

  template <class Archive>
  void serialize(Archive& ar, unsigned /* version */) {
    ar& make_nvp("data", data);
  }

  std::vector<std::uint64_t, Allocator> data;
};

} // namespace detail
} // namespace histogram
} // namespace boost

#endif
