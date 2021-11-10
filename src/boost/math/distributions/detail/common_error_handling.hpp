// Copyright John Maddock 2006, 2007.
// Copyright Paul A. Bristow 2006, 2007, 2012.

// Use, modification and distribution are subject to the
// Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MATH_DISTRIBUTIONS_COMMON_ERROR_HANDLING_HPP
#define BOOST_MATH_DISTRIBUTIONS_COMMON_ERROR_HANDLING_HPP

#include <boost/math/policies/error_handling.hpp>
#include <boost/math/special_functions/fpclassify.hpp>
// using boost::math::isfinite;
// using boost::math::isnan;

#ifdef _MSC_VER
# pragma warning(push)
# pragma warning(disable: 4702) // unreachable code (return after domain_error throw).
#endif

namespace boost{ namespace math{ namespace detail
{

template <class RealType, class Policy>
inline bool check_probability(const char* function, RealType const& prob, RealType* result, const Policy& pol)
{
   if((prob < 0) || (prob > 1) || !(boost::math::isfinite)(prob))
   {
      *result = policies::raise_domain_error<RealType>(
         function,
         "Probability argument is %1%, but must be >= 0 and <= 1 !", prob, pol);
      return false;
   }
   return true;
}

template <class RealType, class Policy>
inline bool check_df(const char* function, RealType const& df, RealType* result, const Policy& pol)
{ //  df > 0 but NOT +infinity allowed.
   if((df <= 0) || !(boost::math::isfinite)(df))
   {
      *result = policies::raise_domain_error<RealType>(
         function,
         "Degrees of freedom argument is %1%, but must be > 0 !", df, pol);
      return false;
   }
   return true;
}

template <class RealType, class Policy>
inline bool check_df_gt0_to_inf(const char* function, RealType const& df, RealType* result, const Policy& pol)
{  // df > 0 or +infinity are allowed.
   if( (df <= 0) || (boost::math::isnan)(df) )
   { // is bad df <= 0 or NaN or -infinity.
      *result = policies::raise_domain_error<RealType>(
         function,
         "Degrees of freedom argument is %1%, but must be > 0 !", df, pol);
      return false;
   }
   return true;
} // check_df_gt0_to_inf


template <class RealType, class Policy>
inline bool check_scale(
      const char* function,
      RealType scale,
      RealType* result,
      const Policy& pol)
{
   if((scale <= 0) || !(boost::math::isfinite)(scale))
   { // Assume scale == 0 is NOT valid for any distribution.
      *result = policies::raise_domain_error<RealType>(
         function,
         "Scale parameter is %1%, but must be > 0 !", scale, pol);
      return false;
   }
   return true;
}

template <class RealType, class Policy>
inline bool check_location(
      const char* function,
      RealType location,
      RealType* result,
      const Policy& pol)
{
   if(!(boost::math::isfinite)(location))
   {
      *result = policies::raise_domain_error<RealType>(
         function,
         "Location parameter is %1%, but must be finite!", location, pol);
      return false;
   }
   return true;
}

template <class RealType, class Policy>
inline bool check_x(
      const char* function,
      RealType x,
      RealType* result,
      const Policy& pol)
{
   // Note that this test catches both infinity and NaN.
   // Some distributions permit x to be infinite, so these must be tested 1st and return,
   // leaving this test to catch any NaNs.
   // See Normal, Logistic, Laplace and Cauchy for example.
   if(!(boost::math::isfinite)(x))
   {
      *result = policies::raise_domain_error<RealType>(
         function,
         "Random variate x is %1%, but must be finite!", x, pol);
      return false;
   }
   return true;
} // bool check_x

template <class RealType, class Policy>
inline bool check_x_not_NaN(
  const char* function,
  RealType x,
  RealType* result,
  const Policy& pol)
{
  // Note that this test catches only NaN.
  // Some distributions permit x to be infinite, leaving this test to catch any NaNs.
  // See Normal, Logistic, Laplace and Cauchy for example.
  if ((boost::math::isnan)(x))
  {
    *result = policies::raise_domain_error<RealType>(
      function,
      "Random variate x is %1%, but must be finite or + or - infinity!", x, pol);
    return false;
  }
  return true;
} // bool check_x_not_NaN

template <class RealType, class Policy>
inline bool check_x_gt0(
      const char* function,
      RealType x,
      RealType* result,
      const Policy& pol)
{
   if(x <= 0)
   {
      *result = policies::raise_domain_error<RealType>(
         function,
         "Random variate x is %1%, but must be > 0!", x, pol);
      return false;
   }

   return true;
   // Note that this test catches both infinity and NaN.
   // Some special cases permit x to be infinite, so these must be tested 1st,
   // leaving this test to catch any NaNs.  See Normal and cauchy for example.
} // bool check_x_gt0

template <class RealType, class Policy>
inline bool check_positive_x(
      const char* function,
      RealType x,
      RealType* result,
      const Policy& pol)
{
   if(!(boost::math::isfinite)(x) || (x < 0))
   {
      *result = policies::raise_domain_error<RealType>(
         function,
         "Random variate x is %1%, but must be finite and >= 0!", x, pol);
      return false;
   }
   return true;
   // Note that this test catches both infinity and NaN.
   // Some special cases permit x to be infinite, so these must be tested 1st,
   // leaving this test to catch any NaNs.  see Normal and cauchy for example.
}

template <class RealType, class Policy>
inline bool check_non_centrality(
      const char* function,
      RealType ncp,
      RealType* result,
      const Policy& pol)
{
   if((ncp < 0) || !(boost::math::isfinite)(ncp))
   { // Assume scale == 0 is NOT valid for any distribution.
      *result = policies::raise_domain_error<RealType>(
         function,
         "Non centrality parameter is %1%, but must be > 0 !", ncp, pol);
      return false;
   }
   return true;
}

template <class RealType, class Policy>
inline bool check_finite(
      const char* function,
      RealType x,
      RealType* result,
      const Policy& pol)
{
   if(!(boost::math::isfinite)(x))
   { // Assume scale == 0 is NOT valid for any distribution.
      *result = policies::raise_domain_error<RealType>(
         function,
         "Parameter is %1%, but must be finite !", x, pol);
      return false;
   }
   return true;
}

} // namespace detail
} // namespace math
} // namespace boost

#ifdef _MSC_VER
#  pragma warning(pop)
#endif

#endif // BOOST_MATH_DISTRIBUTIONS_COMMON_ERROR_HANDLING_HPP
