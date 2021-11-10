// Copyright 2015-2017 Hans Dembinski
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_HISTOGRAM_ACCUMULATORS_OSTREAM_HPP
#define BOOST_HISTOGRAM_ACCUMULATORS_OSTREAM_HPP

#include <boost/histogram/detail/counting_streambuf.hpp>
#include <boost/histogram/fwd.hpp>
#include <ios>

/**
  \file boost/histogram/accumulators/ostream.hpp
  Simple streaming operators for the builtin accumulator types.

  The text representation is not guaranteed to be stable between versions of
  Boost.Histogram. This header is only included by
  [boost/histogram/ostream.hpp](histogram/reference.html#header.boost.histogram.ostream_hpp).
  To use your own, include your own implementation instead of this header and do not
  include
  [boost/histogram/ostream.hpp](histogram/reference.html#header.boost.histogram.ostream_hpp).
 */

#ifndef BOOST_HISTOGRAM_DOXYGEN_INVOKED

namespace boost {
namespace histogram {

namespace detail {

template <class CharT, class Traits, class T>
std::basic_ostream<CharT, Traits>& handle_nonzero_width(
    std::basic_ostream<CharT, Traits>& os, const T& x) {
  const auto w = os.width();
  os.width(0);
  std::streamsize count = 0;
  {
    auto g = make_count_guard(os, count);
    os << x;
  }
  if (os.flags() & std::ios::left) {
    os << x;
    for (auto i = count; i < w; ++i) os << os.fill();
  } else {
    for (auto i = count; i < w; ++i) os << os.fill();
    os << x;
  }
  return os;
}

} // namespace detail

namespace accumulators {

template <class CharT, class Traits, class U, bool B>
std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& os,
                                              const count<U, B>& x) {
  return os << x.value();
}

template <class CharT, class Traits, class U>
std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& os,
                                              const sum<U>& x) {
  if (os.width() == 0) return os << "sum(" << x.large() << " + " << x.small() << ")";
  return detail::handle_nonzero_width(os, x);
}

template <class CharT, class Traits, class U>
std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& os,
                                              const weighted_sum<U>& x) {
  if (os.width() == 0)
    return os << "weighted_sum(" << x.value() << ", " << x.variance() << ")";
  return detail::handle_nonzero_width(os, x);
}

template <class CharT, class Traits, class U>
std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& os,
                                              const mean<U>& x) {
  if (os.width() == 0)
    return os << "mean(" << x.count() << ", " << x.value() << ", " << x.variance() << ")";
  return detail::handle_nonzero_width(os, x);
}

template <class CharT, class Traits, class U>
std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& os,
                                              const weighted_mean<U>& x) {
  if (os.width() == 0)
    return os << "weighted_mean(" << x.sum_of_weights() << ", " << x.value() << ", "
              << x.variance() << ")";
  return detail::handle_nonzero_width(os, x);
}

#include <boost/histogram/detail/ignore_deprecation_warning_begin.hpp>
template <class CharT, class Traits, class T>
std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& os,
                                              const thread_safe<T>& x) {
  os << static_cast<T>(x);
  return os;
}
#include <boost/histogram/detail/ignore_deprecation_warning_end.hpp>

} // namespace accumulators
} // namespace histogram
} // namespace boost

#endif // BOOST_HISTOGRAM_DOXYGEN_INVOKED

#endif
