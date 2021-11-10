/* Boost interval/transc.hpp template implementation file
 *
 * Copyright 2000 Jens Maurer
 * Copyright 2002 Hervé Brönnimann, Guillaume Melquiond, Sylvain Pion
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or
 * copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_NUMERIC_INTERVAL_TRANSC_HPP
#define BOOST_NUMERIC_INTERVAL_TRANSC_HPP

#include <boost/config.hpp>
#include <boost/numeric/interval/detail/interval_prototype.hpp>
#include <boost/numeric/interval/detail/bugs.hpp>
#include <boost/numeric/interval/detail/test_input.hpp>
#include <boost/numeric/interval/rounding.hpp>
#include <boost/numeric/interval/constants.hpp>
#include <boost/numeric/interval/arith.hpp>
#include <boost/numeric/interval/arith2.hpp>
#include <algorithm>

namespace boost {
namespace numeric {

template<class T, class Policies> inline
interval<T, Policies> exp(const interval<T, Policies>& x)
{
  typedef interval<T, Policies> I;
  if (interval_lib::detail::test_input(x))
    return I::empty();
  typename Policies::rounding rnd;
  return I(rnd.exp_down(x.lower()), rnd.exp_up(x.upper()), true);
}

template<class T, class Policies> inline
interval<T, Policies> log(const interval<T, Policies>& x)
{
  typedef interval<T, Policies> I;
  if (interval_lib::detail::test_input(x) ||
      !interval_lib::user::is_pos(x.upper()))
    return I::empty();
  typename Policies::rounding rnd;
  typedef typename Policies::checking checking;
  T l = !interval_lib::user::is_pos(x.lower())
             ? checking::neg_inf() : rnd.log_down(x.lower());
  return I(l, rnd.log_up(x.upper()), true);
}

template<class T, class Policies> inline
interval<T, Policies> cos(const interval<T, Policies>& x)
{
  if (interval_lib::detail::test_input(x))
    return interval<T, Policies>::empty();
  typename Policies::rounding rnd;
  typedef interval<T, Policies> I;
  typedef typename interval_lib::unprotect<I>::type R;

  // get lower bound within [0, pi]
  const R pi2 = interval_lib::pi_twice<R>();
  R tmp = fmod((const R&)x, pi2);
  if (width(tmp) >= pi2.lower())
    return I(static_cast<T>(-1), static_cast<T>(1), true); // we are covering a full period
  if (tmp.lower() >= interval_lib::constants::pi_upper<T>())
    return -cos(tmp - interval_lib::pi<R>());
  T l = tmp.lower();
  T u = tmp.upper();

  BOOST_USING_STD_MIN();
  // separate into monotone subintervals
  if (u <= interval_lib::constants::pi_lower<T>())
    return I(rnd.cos_down(u), rnd.cos_up(l), true);
  else if (u <= pi2.lower())
    return I(static_cast<T>(-1), rnd.cos_up(min BOOST_PREVENT_MACRO_SUBSTITUTION(rnd.sub_down(pi2.lower(), u), l)), true);
  else
    return I(static_cast<T>(-1), static_cast<T>(1), true);
}

template<class T, class Policies> inline
interval<T, Policies> sin(const interval<T, Policies>& x)
{
  typedef interval<T, Policies> I;
  if (interval_lib::detail::test_input(x))
    return I::empty();
  typename Policies::rounding rnd;
  typedef typename interval_lib::unprotect<I>::type R;
  I r = cos((const R&)x - interval_lib::pi_half<R>());
  (void)&rnd;
  return r;
}

template<class T, class Policies> inline
interval<T, Policies> tan(const interval<T, Policies>& x)
{
  typedef interval<T, Policies> I;
  if (interval_lib::detail::test_input(x))
    return I::empty();
  typename Policies::rounding rnd;
  typedef typename interval_lib::unprotect<I>::type R;

  // get lower bound within [-pi/2, pi/2]
  const R pi = interval_lib::pi<R>();
  R tmp = fmod((const R&)x, pi);
  const T pi_half_d = interval_lib::constants::pi_half_lower<T>();
  if (tmp.lower() >= pi_half_d)
    tmp -= pi;
  if (tmp.lower() <= -pi_half_d || tmp.upper() >= pi_half_d)
    return I::whole();
  return I(rnd.tan_down(tmp.lower()), rnd.tan_up(tmp.upper()), true);
}

template<class T, class Policies> inline
interval<T, Policies> asin(const interval<T, Policies>& x)
{
  typedef interval<T, Policies> I;
  if (interval_lib::detail::test_input(x)
     || x.upper() < static_cast<T>(-1) || x.lower() > static_cast<T>(1))
    return I::empty();
  typename Policies::rounding rnd;
  T l = (x.lower() <= static_cast<T>(-1))
             ? -interval_lib::constants::pi_half_upper<T>()
             : rnd.asin_down(x.lower());
  T u = (x.upper() >= static_cast<T>(1) )
             ?  interval_lib::constants::pi_half_upper<T>()
             : rnd.asin_up  (x.upper());
  return I(l, u, true);
}

template<class T, class Policies> inline
interval<T, Policies> acos(const interval<T, Policies>& x)
{
  typedef interval<T, Policies> I;
  if (interval_lib::detail::test_input(x)
     || x.upper() < static_cast<T>(-1) || x.lower() > static_cast<T>(1))
    return I::empty();
  typename Policies::rounding rnd;
  T l = (x.upper() >= static_cast<T>(1) )
          ? static_cast<T>(0)
          : rnd.acos_down(x.upper());
  T u = (x.lower() <= static_cast<T>(-1))
          ? interval_lib::constants::pi_upper<T>()
          : rnd.acos_up  (x.lower());
  return I(l, u, true);
}

template<class T, class Policies> inline
interval<T, Policies> atan(const interval<T, Policies>& x)
{
  typedef interval<T, Policies> I;
  if (interval_lib::detail::test_input(x))
    return I::empty();
  typename Policies::rounding rnd;
  return I(rnd.atan_down(x.lower()), rnd.atan_up(x.upper()), true);
}

template<class T, class Policies> inline
interval<T, Policies> sinh(const interval<T, Policies>& x)
{
  typedef interval<T, Policies> I;
  if (interval_lib::detail::test_input(x))
    return I::empty();
  typename Policies::rounding rnd;
  return I(rnd.sinh_down(x.lower()), rnd.sinh_up(x.upper()), true);
}

template<class T, class Policies> inline
interval<T, Policies> cosh(const interval<T, Policies>& x)
{
  typedef interval<T, Policies> I;
  if (interval_lib::detail::test_input(x))
    return I::empty();
  typename Policies::rounding rnd;
  if (interval_lib::user::is_neg(x.upper()))
    return I(rnd.cosh_down(x.upper()), rnd.cosh_up(x.lower()), true);
  else if (!interval_lib::user::is_neg(x.lower()))
    return I(rnd.cosh_down(x.lower()), rnd.cosh_up(x.upper()), true);
  else
    return I(static_cast<T>(1), rnd.cosh_up(-x.lower() > x.upper() ? x.lower() : x.upper()), true);
}

template<class T, class Policies> inline
interval<T, Policies> tanh(const interval<T, Policies>& x)
{
  typedef interval<T, Policies> I;
  if (interval_lib::detail::test_input(x))
    return I::empty();
  typename Policies::rounding rnd;
  return I(rnd.tanh_down(x.lower()), rnd.tanh_up(x.upper()), true);
}

template<class T, class Policies> inline
interval<T, Policies> asinh(const interval<T, Policies>& x)
{
  typedef interval<T, Policies> I;
  if (interval_lib::detail::test_input(x))
    return I::empty();
  typename Policies::rounding rnd;
  return I(rnd.asinh_down(x.lower()), rnd.asinh_up(x.upper()), true);
}

template<class T, class Policies> inline
interval<T, Policies> acosh(const interval<T, Policies>& x)
{
  typedef interval<T, Policies> I;
  if (interval_lib::detail::test_input(x) || x.upper() < static_cast<T>(1))
    return I::empty();
  typename Policies::rounding rnd;
  T l = x.lower() <= static_cast<T>(1) ? static_cast<T>(0) : rnd.acosh_down(x.lower());
  return I(l, rnd.acosh_up(x.upper()), true);
}

template<class T, class Policies> inline
interval<T, Policies> atanh(const interval<T, Policies>& x)
{
  typedef interval<T, Policies> I;
  if (interval_lib::detail::test_input(x)
      || x.upper() < static_cast<T>(-1) || x.lower() > static_cast<T>(1))
    return I::empty();
  typename Policies::rounding rnd;
  typedef typename Policies::checking checking;
  T l = (x.lower() <= static_cast<T>(-1))
             ? checking::neg_inf() : rnd.atanh_down(x.lower());
  T u = (x.upper() >= static_cast<T>(1) )
             ? checking::pos_inf() : rnd.atanh_up  (x.upper());
  return I(l, u, true);
}

} // namespace numeric
} // namespace boost

#endif // BOOST_NUMERIC_INTERVAL_TRANSC_HPP
