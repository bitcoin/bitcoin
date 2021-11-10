//  Copyright John Maddock 2007-8.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MATH_REAL_TYPE_CONCEPT_HPP
#define BOOST_MATH_REAL_TYPE_CONCEPT_HPP

#include <cmath>
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4100)
#pragma warning(disable: 4510)
#pragma warning(disable: 4610)
#endif
#ifdef _MSC_VER
#pragma warning(pop)
#endif
#include <boost/math/tools/config.hpp>
#include <boost/math/tools/precision.hpp>


namespace boost{ namespace math{ namespace concepts{

template <class RealType>
struct RealTypeConcept
{
   template <class Other>
   void check_binary_ops(Other o)
   {
      RealType r(o);
      r = o;
      r -= o;
      r += o;
      r *= o;
      r /= o;
      r = r - o;
      r = o - r;
      r = r + o;
      r = o + r;
      r = o * r;
      r = r * o;
      r = r / o;
      r = o / r;
      bool b;
      b = r == o;
      suppress_unused_variable_warning(b);
      b = o == r;
      suppress_unused_variable_warning(b);
      b = r != o;
      suppress_unused_variable_warning(b);
      b = o != r;
      suppress_unused_variable_warning(b);
      b = r <= o;
      suppress_unused_variable_warning(b);
      b = o <= r;
      suppress_unused_variable_warning(b);
      b = r >= o;
      suppress_unused_variable_warning(b);
      b = o >= r;
      suppress_unused_variable_warning(b);
      b = r < o;
      suppress_unused_variable_warning(b);
      b = o < r;
      suppress_unused_variable_warning(b);
      b = r > o;
      suppress_unused_variable_warning(b);
      b = o > r;
      suppress_unused_variable_warning(b);
   }

   void constraints()
   {
      BOOST_MATH_STD_USING

      RealType r;
      check_binary_ops(r);
      check_binary_ops(0.5f);
      check_binary_ops(0.5);
      //check_binary_ops(0.5L);
      check_binary_ops(1);
      //check_binary_ops(1u);
      check_binary_ops(1L);
      //check_binary_ops(1uL);
      check_binary_ops(1LL);
      RealType r2 = +r;
      r2 = -r;

      r2 = fabs(r);
      r2 = abs(r);
      r2 = ceil(r);
      r2 = floor(r);
      r2 = exp(r);
      r2 = pow(r, r2);
      r2 = sqrt(r);
      r2 = log(r);
      r2 = cos(r);
      r2 = sin(r);
      r2 = tan(r);
      r2 = asin(r);
      r2 = acos(r);
      r2 = atan(r);
      int i;
      r2 = ldexp(r, i);
      r2 = frexp(r, &i);
      i = boost::math::tools::digits<RealType>();
      r2 = boost::math::tools::max_value<RealType>();
      r2 = boost::math::tools::min_value<RealType>();
      r2 = boost::math::tools::log_max_value<RealType>();
      r2 = boost::math::tools::log_min_value<RealType>();
      r2 = boost::math::tools::epsilon<RealType>();
   }
}; // struct DistributionConcept


}}} // namespaces

#endif

