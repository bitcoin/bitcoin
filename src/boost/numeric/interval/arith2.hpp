/* Boost interval/arith2.hpp template implementation file
 *
 * This header provides some auxiliary arithmetic
 * functions: fmod, sqrt, square, pov, inverse and
 * a multi-interval division.
 *
 * Copyright 2002-2003 Hervé Brönnimann, Guillaume Melquiond, Sylvain Pion
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or
 * copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_NUMERIC_INTERVAL_ARITH2_HPP
#define BOOST_NUMERIC_INTERVAL_ARITH2_HPP

#include <boost/config.hpp>
#include <boost/numeric/interval/detail/interval_prototype.hpp>
#include <boost/numeric/interval/detail/test_input.hpp>
#include <boost/numeric/interval/detail/bugs.hpp>
#include <boost/numeric/interval/detail/division.hpp>
#include <boost/numeric/interval/arith.hpp>
#include <boost/numeric/interval/policies.hpp>
#include <algorithm>
#include <cassert>
#include <boost/config/no_tr1/cmath.hpp>

namespace boost {
namespace numeric {

template<class T, class Policies> inline
interval<T, Policies> fmod(const interval<T, Policies>& x,
                           const interval<T, Policies>& y)
{
  if (interval_lib::detail::test_input(x, y))
    return interval<T, Policies>::empty();
  typename Policies::rounding rnd;
  typedef typename interval_lib::unprotect<interval<T, Policies> >::type I;
  T const &yb = interval_lib::user::is_neg(x.lower()) ? y.lower() : y.upper();
  T n = rnd.int_down(rnd.div_down(x.lower(), yb));
  return (const I&)x - n * (const I&)y;
}

template<class T, class Policies> inline
interval<T, Policies> fmod(const interval<T, Policies>& x, const T& y)
{
  if (interval_lib::detail::test_input(x, y))
    return interval<T, Policies>::empty();
  typename Policies::rounding rnd;
  typedef typename interval_lib::unprotect<interval<T, Policies> >::type I;
  T n = rnd.int_down(rnd.div_down(x.lower(), y));
  return (const I&)x - n * I(y);
}

template<class T, class Policies> inline
interval<T, Policies> fmod(const T& x, const interval<T, Policies>& y)
{
  if (interval_lib::detail::test_input(x, y))
    return interval<T, Policies>::empty();
  typename Policies::rounding rnd;
  typedef typename interval_lib::unprotect<interval<T, Policies> >::type I;
  T const &yb = interval_lib::user::is_neg(x) ? y.lower() : y.upper();
  T n = rnd.int_down(rnd.div_down(x, yb));
  return x - n * (const I&)y;
}

namespace interval_lib {

template<class T, class Policies> inline
interval<T, Policies> division_part1(const interval<T, Policies>& x,
                                     const interval<T, Policies>& y, bool& b)
{
  typedef interval<T, Policies> I;
  b = false;
  if (detail::test_input(x, y))
    return I::empty();
  if (zero_in(y))
    if (!user::is_zero(y.lower()))
      if (!user::is_zero(y.upper()))
        return detail::div_zero_part1(x, y, b);
      else
        return detail::div_negative(x, y.lower());
    else
      if (!user::is_zero(y.upper()))
        return detail::div_positive(x, y.upper());
      else
        return I::empty();
  else
    return detail::div_non_zero(x, y);
}

template<class T, class Policies> inline
interval<T, Policies> division_part2(const interval<T, Policies>& x,
                                     const interval<T, Policies>& y, bool b = true)
{
  if (!b) return interval<T, Policies>::empty();
  return detail::div_zero_part2(x, y);
}

template<class T, class Policies> inline
interval<T, Policies> multiplicative_inverse(const interval<T, Policies>& x)
{
  typedef interval<T, Policies> I;
  if (detail::test_input(x))
    return I::empty();
  T one = static_cast<T>(1);
  typename Policies::rounding rnd;
  if (zero_in(x)) {
    typedef typename Policies::checking checking;
    if (!user::is_zero(x.lower()))
      if (!user::is_zero(x.upper()))
        return I::whole();
      else
        return I(checking::neg_inf(), rnd.div_up(one, x.lower()), true);
    else
      if (!user::is_zero(x.upper()))
        return I(rnd.div_down(one, x.upper()), checking::pos_inf(), true);
      else
        return I::empty();
  } else 
    return I(rnd.div_down(one, x.upper()), rnd.div_up(one, x.lower()), true);
}

namespace detail {

template<class T, class Rounding> inline
T pow_dn(const T& x_, int pwr, Rounding& rnd) // x and pwr are positive
{
  T x = x_;
  T y = (pwr & 1) ? x_ : static_cast<T>(1);
  pwr >>= 1;
  while (pwr > 0) {
    x = rnd.mul_down(x, x);
    if (pwr & 1) y = rnd.mul_down(x, y);
    pwr >>= 1;
  }
  return y;
}

template<class T, class Rounding> inline
T pow_up(const T& x_, int pwr, Rounding& rnd) // x and pwr are positive
{
  T x = x_;
  T y = (pwr & 1) ? x_ : static_cast<T>(1);
  pwr >>= 1;
  while (pwr > 0) {
    x = rnd.mul_up(x, x);
    if (pwr & 1) y = rnd.mul_up(x, y);
    pwr >>= 1;
  }
  return y;
}

} // namespace detail
} // namespace interval_lib

template<class T, class Policies> inline
interval<T, Policies> pow(const interval<T, Policies>& x, int pwr)
{
  BOOST_USING_STD_MAX();
  using interval_lib::detail::pow_dn;
  using interval_lib::detail::pow_up;
  typedef interval<T, Policies> I;

  if (interval_lib::detail::test_input(x))
    return I::empty();

  if (pwr == 0)
    if (interval_lib::user::is_zero(x.lower())
        && interval_lib::user::is_zero(x.upper()))
      return I::empty();
    else
      return I(static_cast<T>(1));
  else if (pwr < 0)
    return interval_lib::multiplicative_inverse(pow(x, -pwr));

  typename Policies::rounding rnd;
  
  if (interval_lib::user::is_neg(x.upper())) {        // [-2,-1]
    T yl = pow_dn(static_cast<T>(-x.upper()), pwr, rnd);
    T yu = pow_up(static_cast<T>(-x.lower()), pwr, rnd);
    if (pwr & 1)     // [-2,-1]^1
      return I(-yu, -yl, true);
    else             // [-2,-1]^2
      return I(yl, yu, true);
  } else if (interval_lib::user::is_neg(x.lower())) { // [-1,1]
    if (pwr & 1) {   // [-1,1]^1
      return I(-pow_up(static_cast<T>(-x.lower()), pwr, rnd), pow_up(x.upper(), pwr, rnd), true);
    } else {         // [-1,1]^2
      return I(static_cast<T>(0), pow_up(max BOOST_PREVENT_MACRO_SUBSTITUTION(static_cast<T>(-x.lower()), x.upper()), pwr, rnd), true);
    }
  } else {                                // [1,2]
    return I(pow_dn(x.lower(), pwr, rnd), pow_up(x.upper(), pwr, rnd), true);
  }
}

template<class T, class Policies> inline
interval<T, Policies> sqrt(const interval<T, Policies>& x)
{
  typedef interval<T, Policies> I;
  if (interval_lib::detail::test_input(x) || interval_lib::user::is_neg(x.upper()))
    return I::empty();
  typename Policies::rounding rnd;
  T l = !interval_lib::user::is_pos(x.lower()) ? static_cast<T>(0) : rnd.sqrt_down(x.lower());
  return I(l, rnd.sqrt_up(x.upper()), true);
}

template<class T, class Policies> inline
interval<T, Policies> square(const interval<T, Policies>& x)
{
  typedef interval<T, Policies> I;
  if (interval_lib::detail::test_input(x))
    return I::empty();
  typename Policies::rounding rnd;
  const T& xl = x.lower();
  const T& xu = x.upper();
  if (interval_lib::user::is_neg(xu))
    return I(rnd.mul_down(xu, xu), rnd.mul_up(xl, xl), true);
  else if (interval_lib::user::is_pos(x.lower()))
    return I(rnd.mul_down(xl, xl), rnd.mul_up(xu, xu), true);
  else
    return I(static_cast<T>(0), (-xl > xu ? rnd.mul_up(xl, xl) : rnd.mul_up(xu, xu)), true);
}

namespace interval_lib {
namespace detail {

template< class I > inline
I root_aux(typename I::base_type const &x, int k) // x and k are bigger than one
{
  typedef typename I::base_type T;
  T tk(k);
  I y(static_cast<T>(1), x, true);
  for(;;) {
    T y0 = median(y);
    I yy = intersect(y, y0 - (pow(I(y0, y0, true), k) - x) / (tk * pow(y, k - 1)));
    if (equal(y, yy)) return y;
    y = yy;
  }
}

template< class I > inline // x is positive and k bigger than one
typename I::base_type root_aux_dn(typename I::base_type const &x, int k)
{
  typedef typename I::base_type T;
  typedef typename I::traits_type Policies;
  typename Policies::rounding rnd;
  T one(1);
  if (x > one) return root_aux<I>(x, k).lower();
  if (x == one) return one;
  return rnd.div_down(one, root_aux<I>(rnd.div_up(one, x), k).upper());
}

template< class I > inline // x is positive and k bigger than one
typename I::base_type root_aux_up(typename I::base_type const &x, int k)
{
  typedef typename I::base_type T;
  typedef typename I::traits_type Policies;
  typename Policies::rounding rnd;
  T one(1);
  if (x > one) return root_aux<I>(x, k).upper();
  if (x == one) return one;
  return rnd.div_up(one, root_aux<I>(rnd.div_down(one, x), k).lower());
}

} // namespace detail
} // namespace interval_lib

template< class T, class Policies > inline
interval<T, Policies> nth_root(interval<T, Policies> const &x, int k)
{
  typedef interval<T, Policies> I;
  if (interval_lib::detail::test_input(x)) return I::empty();
  assert(k > 0);
  if (k == 1) return x;
  typename Policies::rounding rnd;
  typedef typename interval_lib::unprotect<I>::type R;
  if (!interval_lib::user::is_pos(x.upper())) {
    if (interval_lib::user::is_zero(x.upper())) {
      T zero(0);
      if (!(k & 1) || interval_lib::user::is_zero(x.lower())) // [-1,0]^/2 or [0,0]
        return I(zero, zero, true);
      else               // [-1,0]^/3
        return I(-interval_lib::detail::root_aux_up<R>(-x.lower(), k), zero, true);
    } else if (!(k & 1)) // [-2,-1]^/2
      return I::empty();
    else {               // [-2,-1]^/3
      return I(-interval_lib::detail::root_aux_up<R>(-x.lower(), k),
               -interval_lib::detail::root_aux_dn<R>(-x.upper(), k), true);
    }
  }
  T u = interval_lib::detail::root_aux_up<R>(x.upper(), k);
  if (!interval_lib::user::is_pos(x.lower()))
    if (!(k & 1) || interval_lib::user::is_zero(x.lower())) // [-1,1]^/2 or [0,1]
      return I(static_cast<T>(0), u, true);
    else                 // [-1,1]^/3
      return I(-interval_lib::detail::root_aux_up<R>(-x.lower(), k), u, true);
  else                   // [1,2]
    return I(interval_lib::detail::root_aux_dn<R>(x.lower(), k), u, true);
}

} // namespace numeric
} // namespace boost

#endif // BOOST_NUMERIC_INTERVAL_ARITH2_HPP
