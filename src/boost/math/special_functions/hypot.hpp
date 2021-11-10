//  (C) Copyright John Maddock 2005-2006.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MATH_HYPOT_INCLUDED
#define BOOST_MATH_HYPOT_INCLUDED

#ifdef _MSC_VER
#pragma once
#endif

#include <boost/math/tools/config.hpp>
#include <boost/math/tools/precision.hpp>
#include <boost/math/policies/error_handling.hpp>
#include <boost/math/special_functions/math_fwd.hpp>
#include <algorithm> // for swap
#include <cmath>

namespace boost{ namespace math{ namespace detail{

template <class T, class Policy>
T hypot_imp(T x, T y, const Policy& pol)
{
   //
   // Normalize x and y, so that both are positive and x >= y:
   //
   using std::fabs; using std::sqrt; // ADL of std names

   x = fabs(x);
   y = fabs(y);

#ifdef _MSC_VER
#pragma warning(push) 
#pragma warning(disable: 4127)
#endif
   // special case, see C99 Annex F:
   if(std::numeric_limits<T>::has_infinity
      && ((x == std::numeric_limits<T>::infinity())
      || (y == std::numeric_limits<T>::infinity())))
      return policies::raise_overflow_error<T>("boost::math::hypot<%1%>(%1%,%1%)", 0, pol);
#ifdef _MSC_VER
#pragma warning(pop)
#endif

   if(y > x)
      (std::swap)(x, y);

   if(x * tools::epsilon<T>() >= y)
      return x;

   T rat = y / x;
   return x * sqrt(1 + rat*rat);
} // template <class T> T hypot(T x, T y)

}

template <class T1, class T2>
inline typename tools::promote_args<T1, T2>::type 
   hypot(T1 x, T2 y)
{
   typedef typename tools::promote_args<T1, T2>::type result_type;
   return detail::hypot_imp(
      static_cast<result_type>(x), static_cast<result_type>(y), policies::policy<>());
}

template <class T1, class T2, class Policy>
inline typename tools::promote_args<T1, T2>::type 
   hypot(T1 x, T2 y, const Policy& pol)
{
   typedef typename tools::promote_args<T1, T2>::type result_type;
   return detail::hypot_imp(
      static_cast<result_type>(x), static_cast<result_type>(y), pol);
}

} // namespace math
} // namespace boost

#endif // BOOST_MATH_HYPOT_INCLUDED



