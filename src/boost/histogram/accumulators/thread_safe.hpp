// Copyright 2019 Hans Dembinski
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_HISTOGRAM_ACCUMULATORS_THREAD_SAFE_HPP
#define BOOST_HISTOGRAM_ACCUMULATORS_THREAD_SAFE_HPP

#include <atomic>
#include <boost/core/nvp.hpp>
#include <boost/histogram/fwd.hpp>
#include <type_traits>

namespace boost {
namespace histogram {
namespace accumulators {

// cannot use new mechanism with accumulators::thread_safe
template <class T>
struct is_thread_safe<thread_safe<T>> : std::true_type {};

/** Thread-safe adaptor for builtin integral numbers.

  This adaptor uses atomic operations to make concurrent increments and additions safe for
  the stored value.

  On common computing platforms, the adapted integer has the same size and
  alignment as underlying type. The atomicity is implemented with a special CPU
  instruction. On exotic platforms the size of the adapted number may be larger and/or the
  type may have different alignment, which means it cannot be tightly packed into arrays.

  @tparam T type to adapt, must be an integral type.
 */
template <class T>
class [[deprecated("use count<T, true> instead; "
                   "thread_safe<T> will be removed in boost-1.79")]] thread_safe
    : public std::atomic<T> {
public:
  using value_type = T;
  using super_t = std::atomic<T>;

  thread_safe() noexcept : super_t(static_cast<T>(0)) {}
  // non-atomic copy and assign is allowed, because storage is locked in this case
  thread_safe(const thread_safe& o) noexcept : super_t(o.load()) {}
  thread_safe& operator=(const thread_safe& o) noexcept {
    super_t::store(o.load());
    return *this;
  }

  thread_safe(value_type arg) : super_t(arg) {}
  thread_safe& operator=(value_type arg) {
    super_t::store(arg);
    return *this;
  }

  thread_safe& operator+=(const thread_safe& arg) {
    operator+=(arg.load());
    return *this;
  }
  thread_safe& operator+=(value_type arg) {
    super_t::fetch_add(arg, std::memory_order_relaxed);
    return *this;
  }
  thread_safe& operator++() {
    operator+=(static_cast<value_type>(1));
    return *this;
  }

  template <class Archive>
  void serialize(Archive & ar, unsigned /* version */) {
    auto value = super_t::load();
    ar& make_nvp("value", value);
    super_t::store(value);
  }
};

} // namespace accumulators
} // namespace histogram
} // namespace boost

#endif
