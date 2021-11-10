/* Boost interval/ext/integer.hpp template implementation file
 *
 * Copyright 2003 Guillaume Melquiond
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or
 * copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_NUMERIC_INTERVAL_EXT_INTEGER_HPP
#define BOOST_NUMERIC_INTERVAL_EXT_INTEGER_HPP

#include <boost/numeric/interval/detail/interval_prototype.hpp>
#include <boost/numeric/interval/detail/test_input.hpp>

namespace boost {
namespace numeric {

template<class T, class Policies> inline
interval<T, Policies> operator+ (const interval<T, Policies>& x, int y)
{
  return x + static_cast<T>(y);
}

template<class T, class Policies> inline
interval<T, Policies> operator+ (int x, const interval<T, Policies>& y)
{
  return static_cast<T>(x) + y;
}

template<class T, class Policies> inline
interval<T, Policies> operator- (const interval<T, Policies>& x, int y)
{
  return x - static_cast<T>(y);
}

template<class T, class Policies> inline
interval<T, Policies> operator- (int x, const interval<T, Policies>& y)
{
  return static_cast<T>(x) - y;
}

template<class T, class Policies> inline
interval<T, Policies> operator* (const interval<T, Policies>& x, int y)
{
  return x * static_cast<T>(y);
}

template<class T, class Policies> inline
interval<T, Policies> operator* (int x, const interval<T, Policies>& y)
{
  return static_cast<T>(x) * y;
}

template<class T, class Policies> inline
interval<T, Policies> operator/ (const interval<T, Policies>& x, int y)
{
  return x / static_cast<T>(y);
}

template<class T, class Policies> inline
interval<T, Policies> operator/ (int x, const interval<T, Policies>& y)
{
  return static_cast<T>(x) / y;
}

} // namespace numeric
} // namespace boost

#endif // BOOST_NUMERIC_INTERVAL_EXT_INTEGER_HPP
