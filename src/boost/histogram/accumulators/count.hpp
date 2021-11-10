// Copyright 2019 Hans Dembinski
//
// Distributed under the Boost Software License, version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_HISTOGRAM_ACCUMULATORS_COUNT_HPP
#define BOOST_HISTOGRAM_ACCUMULATORS_COUNT_HPP

#include <boost/core/nvp.hpp>
#include <boost/histogram/detail/atomic_number.hpp>
#include <boost/histogram/fwd.hpp> // for count<>
#include <type_traits>             // for std::common_type

namespace boost {
namespace histogram {
namespace accumulators {

/**
  Wraps a C++ arithmetic type with optionally thread-safe increments and adds.

  This adaptor optionally uses atomic operations to make concurrent increments and
  additions thread-safe for the stored arithmetic value, which can be integral or
  floating point. For small histograms, the performance will still be poor because of
  False Sharing, see https://en.wikipedia.org/wiki/False_sharing for details.

  Warning: Assignment is not thread-safe in this implementation, so don't assign
  concurrently.

  This wrapper class can be used as a base class by users to add arbitrary metadata to
  each bin of a histogram.

  When weighted samples are accumulated and high precision is required, use
  `accumulators::sum` instead (at the cost of lower performance). If a local variance
  estimate for the weight distribution should be computed as well (generally needed for a
  detailed statistical analysis), use `accumulators::weighted_sum`.

  @tparam T C++ builtin arithmetic type (integer or floating point).
  @tparam ThreadSafe Set to true to make increments and adds thread-safe.
*/
template <class ValueType, bool ThreadSafe>
class count {
  using internal_type =
      std::conditional_t<ThreadSafe, detail::atomic_number<ValueType>, ValueType>;

public:
  using value_type = ValueType;
  using const_reference = const value_type&;

  count() noexcept = default;

  /// Initialize count to value and allow implicit conversion
  count(const_reference value) noexcept : value_{value} {}

  /// Allow implicit conversion from other count
  template <class T, bool B>
  count(const count<T, B>& c) noexcept : count{c.value()} {}

  /// Increment count by one
  count& operator++() noexcept {
    ++value_;
    return *this;
  }

  /// Increment count by value
  count& operator+=(const_reference value) noexcept {
    value_ += value;
    return *this;
  }

  /// Add another count
  count& operator+=(const count& s) noexcept {
    value_ += s.value_;
    return *this;
  }

  /// Scale by value
  count& operator*=(const_reference value) noexcept {
    value_ *= value;
    return *this;
  }

  bool operator==(const count& rhs) const noexcept { return value_ == rhs.value_; }

  bool operator!=(const count& rhs) const noexcept { return !operator==(rhs); }

  /// Return count
  value_type value() const noexcept { return value_; }

  // conversion to value_type must be explicit
  explicit operator value_type() const noexcept { return value_; }

  template <class Archive>
  void serialize(Archive& ar, unsigned /* version */) {
    auto v = value();
    ar& make_nvp("value", v);
    value_ = v;
  }

  static constexpr bool thread_safe() noexcept { return ThreadSafe; }

  // begin: extra operators to make count behave like a regular number

  count& operator*=(const count& rhs) noexcept {
    value_ *= rhs.value_;
    return *this;
  }

  count operator*(const count& rhs) const noexcept {
    count x = *this;
    x *= rhs;
    return x;
  }

  count& operator/=(const count& rhs) noexcept {
    value_ /= rhs.value_;
    return *this;
  }

  count operator/(const count& rhs) const noexcept {
    count x = *this;
    x /= rhs;
    return x;
  }

  bool operator<(const count& rhs) const noexcept { return value_ < rhs.value_; }

  bool operator>(const count& rhs) const noexcept { return value_ > rhs.value_; }

  bool operator<=(const count& rhs) const noexcept { return value_ <= rhs.value_; }

  bool operator>=(const count& rhs) const noexcept { return value_ >= rhs.value_; }

  friend bool operator==(const_reference x, const count& rhs) noexcept {
    return x == rhs.value_;
  }

  friend bool operator!=(const_reference x, const count& rhs) noexcept {
    return x != rhs.value_;
  }

  friend bool operator<(const_reference x, const count& rhs) noexcept {
    return x < rhs.value_;
  }

  friend bool operator>(const_reference x, const count& rhs) noexcept {
    return x > rhs.value_;
  }

  friend bool operator<=(const_reference x, const count& rhs) noexcept {
    return x <= rhs.value_;
  }
  friend bool operator>=(const_reference x, const count& rhs) noexcept {
    return x >= rhs.value_;
  }

  // end: extra operators

private:
  internal_type value_{};
};

} // namespace accumulators
} // namespace histogram
} // namespace boost

#ifndef BOOST_HISTOGRAM_DOXYGEN_INVOKED
namespace std {
template <class T, class U, bool B1, bool B2>
struct common_type<boost::histogram::accumulators::count<T, B1>,
                   boost::histogram::accumulators::count<U, B2>> {
  using type = boost::histogram::accumulators::count<common_type_t<T, U>, (B1 || B2)>;
};
} // namespace std
#endif

#endif
