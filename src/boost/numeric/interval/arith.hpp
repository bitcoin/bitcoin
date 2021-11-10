/* Boost interval/arith.hpp template implementation file
 *
 * Copyright 2000 Jens Maurer
 * Copyright 2002-2003 Hervé Brönnimann, Guillaume Melquiond, Sylvain Pion
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or
 * copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_NUMERIC_INTERVAL_ARITH_HPP
#define BOOST_NUMERIC_INTERVAL_ARITH_HPP

#include <boost/config.hpp>
#include <boost/numeric/interval/interval.hpp>
#include <boost/numeric/interval/detail/bugs.hpp>
#include <boost/numeric/interval/detail/test_input.hpp>
#include <boost/numeric/interval/detail/division.hpp>
#include <algorithm>

namespace boost {
namespace numeric {

/*
 * Basic arithmetic operators
 */

template<class T, class Policies> inline
const interval<T, Policies>& operator+(const interval<T, Policies>& x)
{
  return x;
}

template<class T, class Policies> inline
interval<T, Policies> operator-(const interval<T, Policies>& x)
{
  if (interval_lib::detail::test_input(x))
    return interval<T, Policies>::empty();
  return interval<T, Policies>(-x.upper(), -x.lower(), true);
}

template<class T, class Policies> inline
interval<T, Policies>& interval<T, Policies>::operator+=(const interval<T, Policies>& r)
{
  if (interval_lib::detail::test_input(*this, r))
    set_empty();
  else {
    typename Policies::rounding rnd;
    set(rnd.add_down(low, r.low), rnd.add_up(up, r.up));
  }
  return *this;
}

template<class T, class Policies> inline
interval<T, Policies>& interval<T, Policies>::operator+=(const T& r)
{
  if (interval_lib::detail::test_input(*this, r))
    set_empty();
  else {
    typename Policies::rounding rnd;
    set(rnd.add_down(low, r), rnd.add_up(up, r));
  }
  return *this;
}

template<class T, class Policies> inline
interval<T, Policies>& interval<T, Policies>::operator-=(const interval<T, Policies>& r)
{
  if (interval_lib::detail::test_input(*this, r))
    set_empty();
  else {
    typename Policies::rounding rnd;
    set(rnd.sub_down(low, r.up), rnd.sub_up(up, r.low));
  }
  return *this;
}

template<class T, class Policies> inline
interval<T, Policies>& interval<T, Policies>::operator-=(const T& r)
{
  if (interval_lib::detail::test_input(*this, r))
    set_empty();
  else {
    typename Policies::rounding rnd;
    set(rnd.sub_down(low, r), rnd.sub_up(up, r));
  }
  return *this;
}

template<class T, class Policies> inline
interval<T, Policies>& interval<T, Policies>::operator*=(const interval<T, Policies>& r)
{
  return *this = *this * r;
}

template<class T, class Policies> inline
interval<T, Policies>& interval<T, Policies>::operator*=(const T& r)
{
  return *this = r * *this;
}

template<class T, class Policies> inline
interval<T, Policies>& interval<T, Policies>::operator/=(const interval<T, Policies>& r)
{
  return *this = *this / r;
}

template<class T, class Policies> inline
interval<T, Policies>& interval<T, Policies>::operator/=(const T& r)
{
  return *this = *this / r;
}

template<class T, class Policies> inline
interval<T, Policies> operator+(const interval<T, Policies>& x,
                                const interval<T, Policies>& y)
{
  if (interval_lib::detail::test_input(x, y))
    return interval<T, Policies>::empty();
  typename Policies::rounding rnd;
  return interval<T,Policies>(rnd.add_down(x.lower(), y.lower()),
                              rnd.add_up  (x.upper(), y.upper()), true);
}

template<class T, class Policies> inline
interval<T, Policies> operator+(const T& x, const interval<T, Policies>& y)
{
  if (interval_lib::detail::test_input(x, y))
    return interval<T, Policies>::empty();
  typename Policies::rounding rnd;
  return interval<T,Policies>(rnd.add_down(x, y.lower()),
                              rnd.add_up  (x, y.upper()), true);
}

template<class T, class Policies> inline
interval<T, Policies> operator+(const interval<T, Policies>& x, const T& y)
{ return y + x; }

template<class T, class Policies> inline
interval<T, Policies> operator-(const interval<T, Policies>& x,
                                const interval<T, Policies>& y)
{
  if (interval_lib::detail::test_input(x, y))
    return interval<T, Policies>::empty();
  typename Policies::rounding rnd;
  return interval<T,Policies>(rnd.sub_down(x.lower(), y.upper()),
                              rnd.sub_up  (x.upper(), y.lower()), true);
}

template<class T, class Policies> inline
interval<T, Policies> operator-(const T& x, const interval<T, Policies>& y)
{
  if (interval_lib::detail::test_input(x, y))
    return interval<T, Policies>::empty();
  typename Policies::rounding rnd;
  return interval<T,Policies>(rnd.sub_down(x, y.upper()),
                              rnd.sub_up  (x, y.lower()), true);
}

template<class T, class Policies> inline
interval<T, Policies> operator-(const interval<T, Policies>& x, const T& y)
{
  if (interval_lib::detail::test_input(x, y))
    return interval<T, Policies>::empty();
  typename Policies::rounding rnd;
  return interval<T,Policies>(rnd.sub_down(x.lower(), y),
                              rnd.sub_up  (x.upper(), y), true);
}

template<class T, class Policies> inline
interval<T, Policies> operator*(const interval<T, Policies>& x,
                                const interval<T, Policies>& y)
{
  BOOST_USING_STD_MIN();
  BOOST_USING_STD_MAX();
  typedef interval<T, Policies> I;
  if (interval_lib::detail::test_input(x, y))
    return I::empty();
  typename Policies::rounding rnd;
  const T& xl = x.lower();
  const T& xu = x.upper();
  const T& yl = y.lower();
  const T& yu = y.upper();

  if (interval_lib::user::is_neg(xl))
    if (interval_lib::user::is_pos(xu))
      if (interval_lib::user::is_neg(yl))
        if (interval_lib::user::is_pos(yu)) // M * M
          return I(min BOOST_PREVENT_MACRO_SUBSTITUTION(rnd.mul_down(xl, yu), rnd.mul_down(xu, yl)),
                   max BOOST_PREVENT_MACRO_SUBSTITUTION(rnd.mul_up  (xl, yl), rnd.mul_up  (xu, yu)), true);
        else                    // M * N
          return I(rnd.mul_down(xu, yl), rnd.mul_up(xl, yl), true);
      else
        if (interval_lib::user::is_pos(yu)) // M * P
          return I(rnd.mul_down(xl, yu), rnd.mul_up(xu, yu), true);
        else                    // M * Z
          return I(static_cast<T>(0), static_cast<T>(0), true);
    else
      if (interval_lib::user::is_neg(yl))
        if (interval_lib::user::is_pos(yu)) // N * M
          return I(rnd.mul_down(xl, yu), rnd.mul_up(xl, yl), true);
        else                    // N * N
          return I(rnd.mul_down(xu, yu), rnd.mul_up(xl, yl), true);
      else
        if (interval_lib::user::is_pos(yu)) // N * P
          return I(rnd.mul_down(xl, yu), rnd.mul_up(xu, yl), true);
        else                    // N * Z
          return I(static_cast<T>(0), static_cast<T>(0), true);
  else
    if (interval_lib::user::is_pos(xu))
      if (interval_lib::user::is_neg(yl))
        if (interval_lib::user::is_pos(yu)) // P * M
          return I(rnd.mul_down(xu, yl), rnd.mul_up(xu, yu), true);
        else                    // P * N
          return I(rnd.mul_down(xu, yl), rnd.mul_up(xl, yu), true);
      else
        if (interval_lib::user::is_pos(yu)) // P * P
          return I(rnd.mul_down(xl, yl), rnd.mul_up(xu, yu), true);
        else                    // P * Z
          return I(static_cast<T>(0), static_cast<T>(0), true);
    else                        // Z * ?
      return I(static_cast<T>(0), static_cast<T>(0), true);
}

template<class T, class Policies> inline
interval<T, Policies> operator*(const T& x, const interval<T, Policies>& y)
{ 
  typedef interval<T, Policies> I;
  if (interval_lib::detail::test_input(x, y))
    return I::empty();
  typename Policies::rounding rnd;
  const T& yl = y.lower();
  const T& yu = y.upper();
  // x is supposed not to be infinite
  if (interval_lib::user::is_neg(x))
    return I(rnd.mul_down(x, yu), rnd.mul_up(x, yl), true);
  else if (interval_lib::user::is_zero(x))
    return I(static_cast<T>(0), static_cast<T>(0), true);
  else
    return I(rnd.mul_down(x, yl), rnd.mul_up(x, yu), true);
}

template<class T, class Policies> inline
interval<T, Policies> operator*(const interval<T, Policies>& x, const T& y)
{ return y * x; }

template<class T, class Policies> inline
interval<T, Policies> operator/(const interval<T, Policies>& x,
                                const interval<T, Policies>& y)
{
  if (interval_lib::detail::test_input(x, y))
    return interval<T, Policies>::empty();
  if (zero_in(y))
    if (!interval_lib::user::is_zero(y.lower()))
      if (!interval_lib::user::is_zero(y.upper()))
        return interval_lib::detail::div_zero(x);
      else
        return interval_lib::detail::div_negative(x, y.lower());
    else
      if (!interval_lib::user::is_zero(y.upper()))
        return interval_lib::detail::div_positive(x, y.upper());
      else
        return interval<T, Policies>::empty();
  else
    return interval_lib::detail::div_non_zero(x, y);
}

template<class T, class Policies> inline
interval<T, Policies> operator/(const T& x, const interval<T, Policies>& y)
{
  if (interval_lib::detail::test_input(x, y))
    return interval<T, Policies>::empty();
  if (zero_in(y))
    if (!interval_lib::user::is_zero(y.lower()))
      if (!interval_lib::user::is_zero(y.upper()))
        return interval_lib::detail::div_zero<T, Policies>(x);
      else
        return interval_lib::detail::div_negative<T, Policies>(x, y.lower());
    else
      if (!interval_lib::user::is_zero(y.upper()))
        return interval_lib::detail::div_positive<T, Policies>(x, y.upper());
      else
        return interval<T, Policies>::empty();
  else
    return interval_lib::detail::div_non_zero(x, y);
}

template<class T, class Policies> inline
interval<T, Policies> operator/(const interval<T, Policies>& x, const T& y)
{
  if (interval_lib::detail::test_input(x, y) || interval_lib::user::is_zero(y))
    return interval<T, Policies>::empty();
  typename Policies::rounding rnd;
  const T& xl = x.lower();
  const T& xu = x.upper();
  if (interval_lib::user::is_neg(y))
    return interval<T, Policies>(rnd.div_down(xu, y), rnd.div_up(xl, y), true);
  else
    return interval<T, Policies>(rnd.div_down(xl, y), rnd.div_up(xu, y), true);
}

} // namespace numeric
} // namespace boost

#endif // BOOST_NUMERIC_INTERVAL_ARITH_HPP
