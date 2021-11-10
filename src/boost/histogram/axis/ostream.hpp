// Copyright 2015-2017 Hans Dembinski
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// String representations here evaluate correctly in Python.

#ifndef BOOST_HISTOGRAM_AXIS_OSTREAM_HPP
#define BOOST_HISTOGRAM_AXIS_OSTREAM_HPP

#include <boost/histogram/axis/regular.hpp>
#include <boost/histogram/detail/counting_streambuf.hpp>
#include <boost/histogram/detail/priority.hpp>
#include <boost/histogram/detail/type_name.hpp>
#include <boost/histogram/fwd.hpp>
#include <boost/throw_exception.hpp>
#include <cassert>
#include <iomanip>
#include <iosfwd>
#include <sstream>
#include <stdexcept>
#include <type_traits>

/**
  \file boost/histogram/axis/ostream.hpp
  Simple streaming operators for the builtin axis types.

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

template <class OStream, class T>
auto ostream_any_impl(OStream& os, const T& t, priority<1>) -> decltype(os << t) {
  return os << t;
}

template <class OStream, class T>
OStream& ostream_any_impl(OStream& os, const T&, priority<0>) {
  return os << type_name<T>();
}

template <class OStream, class T>
OStream& ostream_any(OStream& os, const T& t) {
  return ostream_any_impl(os, t, priority<1>{});
}

template <class OStream, class... Ts>
OStream& ostream_any_quoted(OStream& os, const std::basic_string<Ts...>& s) {
  return os << std::quoted(s);
}

template <class OStream, class T>
OStream& ostream_any_quoted(OStream& os, const T& t) {
  return ostream_any(os, t);
}

template <class... Ts, class T>
std::basic_ostream<Ts...>& ostream_metadata(std::basic_ostream<Ts...>& os, const T& t,
                                            const char* prefix = ", ") {
  std::streamsize count = 0;
  {
    auto g = make_count_guard(os, count);
    ostream_any(os, t);
  }
  if (!count) return os;
  os << prefix << "metadata=";
  return ostream_any_quoted(os, t);
}

template <class OStream>
void ostream_options(OStream& os, const unsigned bits) {
  bool first = true;
  os << ", options=";

#define BOOST_HISTOGRAM_AXIS_OPTION_OSTREAM(x) \
  if (bits & axis::option::x) {                \
    if (first)                                 \
      first = false;                           \
    else {                                     \
      os << " | ";                             \
    }                                          \
    os << #x;                                  \
  }

  BOOST_HISTOGRAM_AXIS_OPTION_OSTREAM(underflow);
  BOOST_HISTOGRAM_AXIS_OPTION_OSTREAM(overflow);
  BOOST_HISTOGRAM_AXIS_OPTION_OSTREAM(circular);
  BOOST_HISTOGRAM_AXIS_OPTION_OSTREAM(growth);

#undef BOOST_HISTOGRAM_AXIS_OPTION_OSTREAM

  if (first) os << "none";
}

} // namespace detail

namespace axis {

template <class T>
class polymorphic_bin;

template <class... Ts>
std::basic_ostream<Ts...>& operator<<(std::basic_ostream<Ts...>& os, const null_type&) {
  return os; // do nothing
}

template <class... Ts, class U>
std::basic_ostream<Ts...>& operator<<(std::basic_ostream<Ts...>& os,
                                      const interval_view<U>& i) {
  return os << "[" << i.lower() << ", " << i.upper() << ")";
}

template <class... Ts, class U>
std::basic_ostream<Ts...>& operator<<(std::basic_ostream<Ts...>& os,
                                      const polymorphic_bin<U>& i) {
  if (i.is_discrete()) return os << static_cast<double>(i);
  return os << "[" << i.lower() << ", " << i.upper() << ")";
}

namespace transform {
template <class... Ts>
std::basic_ostream<Ts...>& operator<<(std::basic_ostream<Ts...>& os, const id&) {
  return os;
}

template <class... Ts>
std::basic_ostream<Ts...>& operator<<(std::basic_ostream<Ts...>& os, const log&) {
  return os << "transform::log{}";
}

template <class... Ts>
std::basic_ostream<Ts...>& operator<<(std::basic_ostream<Ts...>& os, const sqrt&) {
  return os << "transform::sqrt{}";
}

template <class... Ts>
std::basic_ostream<Ts...>& operator<<(std::basic_ostream<Ts...>& os, const pow& p) {
  return os << "transform::pow{" << p.power << "}";
}
} // namespace transform

template <class... Ts, class... Us>
std::basic_ostream<Ts...>& operator<<(std::basic_ostream<Ts...>& os,
                                      const regular<Us...>& a) {
  os << "regular(";
  const auto pos = os.tellp();
  os << a.transform();
  if (os.tellp() > pos) os << ", ";
  os << a.size() << ", " << a.value(0) << ", " << a.value(a.size());
  detail::ostream_metadata(os, a.metadata());
  detail::ostream_options(os, a.options());
  return os << ")";
}

template <class... Ts, class... Us>
std::basic_ostream<Ts...>& operator<<(std::basic_ostream<Ts...>& os,
                                      const integer<Us...>& a) {
  os << "integer(" << a.value(0) << ", " << a.value(a.size());
  detail::ostream_metadata(os, a.metadata());
  detail::ostream_options(os, a.options());
  return os << ")";
}

template <class... Ts, class... Us>
std::basic_ostream<Ts...>& operator<<(std::basic_ostream<Ts...>& os,
                                      const variable<Us...>& a) {
  os << "variable(" << a.value(0);
  for (index_type i = 1, n = a.size(); i <= n; ++i) { os << ", " << a.value(i); }
  detail::ostream_metadata(os, a.metadata());
  detail::ostream_options(os, a.options());
  return os << ")";
}

template <class... Ts, class... Us>
std::basic_ostream<Ts...>& operator<<(std::basic_ostream<Ts...>& os,
                                      const category<Us...>& a) {
  os << "category(";
  for (index_type i = 0, n = a.size(); i < n; ++i) {
    detail::ostream_any_quoted(os, a.value(i));
    os << (i == (a.size() - 1) ? "" : ", ");
  }
  detail::ostream_metadata(os, a.metadata());
  detail::ostream_options(os, a.options());
  return os << ")";
}

template <class... Ts, class M>
std::basic_ostream<Ts...>& operator<<(std::basic_ostream<Ts...>& os,
                                      const boolean<M>& a) {
  os << "boolean(";
  detail::ostream_metadata(os, a.metadata(), "");
  return os << ")";
}

template <class... Ts, class... Us>
std::basic_ostream<Ts...>& operator<<(std::basic_ostream<Ts...>& os,
                                      const variant<Us...>& v) {
  visit([&os](const auto& x) { detail::ostream_any(os, x); }, v);
  return os;
}

} // namespace axis
} // namespace histogram
} // namespace boost

#endif // BOOST_HISTOGRAM_DOXYGEN_INVOKED

#endif
