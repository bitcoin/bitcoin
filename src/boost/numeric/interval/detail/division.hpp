/* Boost interval/detail/division.hpp file
 *
 * Copyright 2003 Guillaume Melquiond, Sylvain Pion
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or
 * copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_NUMERIC_INTERVAL_DETAIL_DIVISION_HPP
#define BOOST_NUMERIC_INTERVAL_DETAIL_DIVISION_HPP

#include <boost/numeric/interval/detail/interval_prototype.hpp>
#include <boost/numeric/interval/detail/bugs.hpp>
#include <boost/numeric/interval/detail/test_input.hpp>
#include <boost/numeric/interval/rounded_arith.hpp>
#include <algorithm>

namespace boost {
namespace numeric {
namespace interval_lib {
namespace detail {

template<class T, class Policies> inline
interval<T, Policies> div_non_zero(const interval<T, Policies>& x,
                                   const interval<T, Policies>& y)
{
  // assert(!in_zero(y));
  typename Policies::rounding rnd;
  typedef interval<T, Policies> I;
  const T& xl = x.lower();
  const T& xu = x.upper();
  const T& yl = y.lower();
  const T& yu = y.upper();
  if (::boost::numeric::interval_lib::user::is_neg(xu))
    if (::boost::numeric::interval_lib::user::is_neg(yu))
      return I(rnd.div_down(xu, yl), rnd.div_up(xl, yu), true);
    else
      return I(rnd.div_down(xl, yl), rnd.div_up(xu, yu), true);
  else if (::boost::numeric::interval_lib::user::is_neg(xl))
    if (::boost::numeric::interval_lib::user::is_neg(yu))
      return I(rnd.div_down(xu, yu), rnd.div_up(xl, yu), true);
    else
      return I(rnd.div_down(xl, yl), rnd.div_up(xu, yl), true);
  else
    if (::boost::numeric::interval_lib::user::is_neg(yu))
      return I(rnd.div_down(xu, yu), rnd.div_up(xl, yl), true);
    else
      return I(rnd.div_down(xl, yu), rnd.div_up(xu, yl), true);
}

template<class T, class Policies> inline
interval<T, Policies> div_non_zero(const T& x, const interval<T, Policies>& y)
{
  // assert(!in_zero(y));
  typename Policies::rounding rnd;
  typedef interval<T, Policies> I;
  const T& yl = y.lower();
  const T& yu = y.upper();
  if (::boost::numeric::interval_lib::user::is_neg(x))
    return I(rnd.div_down(x, yl), rnd.div_up(x, yu), true);
  else
    return I(rnd.div_down(x, yu), rnd.div_up(x, yl), true);
}

template<class T, class Policies> inline
interval<T, Policies> div_positive(const interval<T, Policies>& x, const T& yu)
{
  // assert(::boost::numeric::interval_lib::user::is_pos(yu));
  if (::boost::numeric::interval_lib::user::is_zero(x.lower()) &&
      ::boost::numeric::interval_lib::user::is_zero(x.upper()))
    return x;
  typename Policies::rounding rnd;
  typedef interval<T, Policies> I;
  const T& xl = x.lower();
  const T& xu = x.upper();
  typedef typename Policies::checking checking;
  if (::boost::numeric::interval_lib::user::is_neg(xu))
    return I(checking::neg_inf(), rnd.div_up(xu, yu), true);
  else if (::boost::numeric::interval_lib::user::is_neg(xl))
    return I(checking::neg_inf(), checking::pos_inf(), true);
  else
    return I(rnd.div_down(xl, yu), checking::pos_inf(), true);
}

template<class T, class Policies> inline
interval<T, Policies> div_positive(const T& x, const T& yu)
{
  // assert(::boost::numeric::interval_lib::user::is_pos(yu));
  typedef interval<T, Policies> I;
  if (::boost::numeric::interval_lib::user::is_zero(x))
    return I(static_cast<T>(0), static_cast<T>(0), true);
  typename Policies::rounding rnd;
  typedef typename Policies::checking checking;
  if (::boost::numeric::interval_lib::user::is_neg(x))
    return I(checking::neg_inf(), rnd.div_up(x, yu), true);
  else
    return I(rnd.div_down(x, yu), checking::pos_inf(), true);
}

template<class T, class Policies> inline
interval<T, Policies> div_negative(const interval<T, Policies>& x, const T& yl)
{
  // assert(::boost::numeric::interval_lib::user::is_neg(yl));
  if (::boost::numeric::interval_lib::user::is_zero(x.lower()) &&
      ::boost::numeric::interval_lib::user::is_zero(x.upper()))
    return x;
  typename Policies::rounding rnd;
  typedef interval<T, Policies> I;
  const T& xl = x.lower();
  const T& xu = x.upper();
  typedef typename Policies::checking checking;
  if (::boost::numeric::interval_lib::user::is_neg(xu))
    return I(rnd.div_down(xu, yl), checking::pos_inf(), true);
  else if (::boost::numeric::interval_lib::user::is_neg(xl))
    return I(checking::neg_inf(), checking::pos_inf(), true);
  else
    return I(checking::neg_inf(), rnd.div_up(xl, yl), true);
}

template<class T, class Policies> inline
interval<T, Policies> div_negative(const T& x, const T& yl)
{
  // assert(::boost::numeric::interval_lib::user::is_neg(yl));
  typedef interval<T, Policies> I;
  if (::boost::numeric::interval_lib::user::is_zero(x))
    return I(static_cast<T>(0), static_cast<T>(0), true);
  typename Policies::rounding rnd;
  typedef typename Policies::checking checking;
  if (::boost::numeric::interval_lib::user::is_neg(x))
    return I(rnd.div_down(x, yl), checking::pos_inf(), true);
  else
    return I(checking::neg_inf(), rnd.div_up(x, yl), true);
}

template<class T, class Policies> inline
interval<T, Policies> div_zero(const interval<T, Policies>& x)
{
  if (::boost::numeric::interval_lib::user::is_zero(x.lower()) &&
      ::boost::numeric::interval_lib::user::is_zero(x.upper()))
    return x;
  else return interval<T, Policies>::whole();
}

template<class T, class Policies> inline
interval<T, Policies> div_zero(const T& x)
{
  if (::boost::numeric::interval_lib::user::is_zero(x))
    return interval<T, Policies>(static_cast<T>(0), static_cast<T>(0), true);
  else return interval<T, Policies>::whole();
}

template<class T, class Policies> inline
interval<T, Policies> div_zero_part1(const interval<T, Policies>& x,
                                     const interval<T, Policies>& y, bool& b)
{
  // assert(::boost::numeric::interval_lib::user::is_neg(y.lower()) && ::boost::numeric::interval_lib::user::is_pos(y.upper()));
  if (::boost::numeric::interval_lib::user::is_zero(x.lower()) && ::boost::numeric::interval_lib::user::is_zero(x.upper()))
    { b = false; return x; }
  typename Policies::rounding rnd;
  typedef interval<T, Policies> I;
  const T& xl = x.lower();
  const T& xu = x.upper();
  const T& yl = y.lower();
  const T& yu = y.upper();
  typedef typename Policies::checking checking;
  if (::boost::numeric::interval_lib::user::is_neg(xu))
    { b = true;  return I(checking::neg_inf(), rnd.div_up(xu, yu), true); }
  else if (::boost::numeric::interval_lib::user::is_neg(xl))
    { b = false; return I(checking::neg_inf(), checking::pos_inf(), true); }
  else
    { b = true;  return I(checking::neg_inf(), rnd.div_up(xl, yl), true); }
}

template<class T, class Policies> inline
interval<T, Policies> div_zero_part2(const interval<T, Policies>& x,
                                     const interval<T, Policies>& y)
{
  // assert(::boost::numeric::interval_lib::user::is_neg(y.lower()) && ::boost::numeric::interval_lib::user::is_pos(y.upper()) && (div_zero_part1(x, y, b), b));
  typename Policies::rounding rnd;
  typedef interval<T, Policies> I;
  typedef typename Policies::checking checking;
  if (::boost::numeric::interval_lib::user::is_neg(x.upper()))
    return I(rnd.div_down(x.upper(), y.lower()), checking::pos_inf(), true);
  else
    return I(rnd.div_down(x.lower(), y.upper()), checking::pos_inf(), true);
}

} // namespace detail
} // namespace interval_lib
} // namespace numeric
} // namespace boost

#endif // BOOST_NUMERIC_INTERVAL_DETAIL_DIVISION_HPP
