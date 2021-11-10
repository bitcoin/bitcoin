//  Copyright Thijs van den Berg, 2008.
//  Copyright John Maddock 2008.
//  Copyright Paul A. Bristow 2008, 2014.

//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// This module implements the Laplace distribution.
// Weisstein, Eric W. "Laplace Distribution." From MathWorld--A Wolfram Web Resource.
// http://mathworld.wolfram.com/LaplaceDistribution.html
// http://en.wikipedia.org/wiki/Laplace_distribution
//
// Abramowitz and Stegun 1972, p 930
// http://www.math.sfu.ca/~cbm/aands/page_930.htm

#ifndef BOOST_STATS_LAPLACE_HPP
#define BOOST_STATS_LAPLACE_HPP

#include <boost/math/distributions/detail/common_error_handling.hpp>
#include <boost/math/distributions/complement.hpp>
#include <boost/math/constants/constants.hpp>
#include <limits>

namespace boost{ namespace math{

#ifdef _MSC_VER
#  pragma warning(push)
#  pragma warning(disable:4127) // conditional expression is constant
#endif

template <class RealType = double, class Policy = policies::policy<> >
class laplace_distribution
{
public:
   // ----------------------------------
   // public Types
   // ----------------------------------
   typedef RealType value_type;
   typedef Policy policy_type;

   // ----------------------------------
   // Constructor(s)
   // ----------------------------------
   laplace_distribution(RealType l_location = 0, RealType l_scale = 1)
      : m_location(l_location), m_scale(l_scale)
   {
      RealType result;
      check_parameters("boost::math::laplace_distribution<%1%>::laplace_distribution()", &result);
   }


   // ----------------------------------
   // Public functions
   // ----------------------------------

   RealType location() const
   {
      return m_location;
   }

   RealType scale() const
   {
      return m_scale;
   }

   bool check_parameters(const char* function, RealType* result) const
   {
         if(false == detail::check_scale(function, m_scale, result, Policy())) return false;
         if(false == detail::check_location(function, m_location, result, Policy())) return false;
         return true;
   }

private:
   RealType m_location;
   RealType m_scale;
}; // class laplace_distribution

//
// Convenient type synonym for double.
typedef laplace_distribution<double> laplace;

//
// Non-member functions.
template <class RealType, class Policy>
inline const std::pair<RealType, RealType> range(const laplace_distribution<RealType, Policy>&)
{
   if (std::numeric_limits<RealType>::has_infinity)
  {  // Can use infinity.
     return std::pair<RealType, RealType>(-std::numeric_limits<RealType>::infinity(), std::numeric_limits<RealType>::infinity()); // - to + infinity.
  }
  else
  { // Can only use max_value.
    using boost::math::tools::max_value;
    return std::pair<RealType, RealType>(-max_value<RealType>(), max_value<RealType>()); // - to + max value.
  }

}

template <class RealType, class Policy>
inline const std::pair<RealType, RealType> support(const laplace_distribution<RealType, Policy>&)
{
  if (std::numeric_limits<RealType>::has_infinity)
  { // Can Use infinity.
     return std::pair<RealType, RealType>(-std::numeric_limits<RealType>::infinity(), std::numeric_limits<RealType>::infinity()); // - to + infinity.
  }
  else
  { // Can only use max_value.
    using boost::math::tools::max_value;
    return std::pair<RealType, RealType>(-max_value<RealType>(), max_value<RealType>()); // - to + max value.
  }
}

template <class RealType, class Policy>
inline RealType pdf(const laplace_distribution<RealType, Policy>& dist, const RealType& x)
{
   BOOST_MATH_STD_USING // for ADL of std functions

   // Checking function argument
   RealType result = 0;
   const char* function = "boost::math::pdf(const laplace_distribution<%1%>&, %1%))";

   // Check scale and location.
   if (false == dist.check_parameters(function, &result)) return result;
   // Special pdf values.
   if((boost::math::isinf)(x))
   {
      return 0; // pdf + and - infinity is zero.
   }
   if (false == detail::check_x(function, x, &result, Policy())) return result;

   // General case
   RealType scale( dist.scale() );
   RealType location( dist.location() );

   RealType exponent = x - location;
   if (exponent>0) exponent = -exponent;
   exponent /= scale;

   result = exp(exponent);
   result /= 2 * scale;

   return result;
} // pdf

template <class RealType, class Policy>
inline RealType cdf(const laplace_distribution<RealType, Policy>& dist, const RealType& x)
{
   BOOST_MATH_STD_USING  // For ADL of std functions.

   RealType result = 0;
   // Checking function argument.
   const char* function = "boost::math::cdf(const laplace_distribution<%1%>&, %1%)";
   // Check scale and location.
   if (false == dist.check_parameters(function, &result)) return result;

   // Special cdf values:
   if((boost::math::isinf)(x))
   {
     if(x < 0) return 0; // -infinity.
     return 1; // + infinity.
   }
   if (false == detail::check_x(function, x, &result, Policy())) return result;

   // General cdf  values
   RealType scale( dist.scale() );
   RealType location( dist.location() );

   if (x < location)
   {
      result = exp( (x-location)/scale )/2;
   }
   else
   {
      result = 1 - exp( (location-x)/scale )/2;
   }
   return result;
} // cdf


template <class RealType, class Policy>
inline RealType quantile(const laplace_distribution<RealType, Policy>& dist, const RealType& p)
{
   BOOST_MATH_STD_USING // for ADL of std functions.

   // Checking function argument
   RealType result = 0;
   const char* function = "boost::math::quantile(const laplace_distribution<%1%>&, %1%)";
   if (false == dist.check_parameters(function, &result)) return result;
   if(false == detail::check_probability(function, p, &result, Policy())) return result;

   // Extreme values of p:
   if(p == 0)
   {
      result = policies::raise_overflow_error<RealType>(function,
        "probability parameter is 0, but must be > 0!", Policy());
      return -result; // -std::numeric_limits<RealType>::infinity();
   }
  
   if(p == 1)
   {
      result = policies::raise_overflow_error<RealType>(function,
        "probability parameter is 1, but must be < 1!", Policy());
      return result; // std::numeric_limits<RealType>::infinity();
   }
   // Calculate Quantile
   RealType scale( dist.scale() );
   RealType location( dist.location() );

   if (p - 0.5 < 0.0)
      result = location + scale*log( static_cast<RealType>(p*2) );
   else
      result = location - scale*log( static_cast<RealType>(-p*2 + 2) );

   return result;
} // quantile


template <class RealType, class Policy>
inline RealType cdf(const complemented2_type<laplace_distribution<RealType, Policy>, RealType>& c)
{
   // Calculate complement of cdf.
   BOOST_MATH_STD_USING // for ADL of std functions

   RealType scale = c.dist.scale();
   RealType location = c.dist.location();
   RealType x = c.param;
   RealType result = 0;

   // Checking function argument.
   const char* function = "boost::math::cdf(const complemented2_type<laplace_distribution<%1%>, %1%>&)";

   // Check scale and location.
   //if(false == detail::check_scale(function, scale, result, Policy())) return false;
   //if(false == detail::check_location(function, location, result, Policy())) return false;
    if (false == c.dist.check_parameters(function, &result)) return result;

   // Special cdf values.
   if((boost::math::isinf)(x))
   {
     if(x < 0) return 1; // cdf complement -infinity is unity.
     return 0; // cdf complement +infinity is zero.
   }
   if(false == detail::check_x(function, x, &result, Policy()))return result;

   // Cdf interval value.
   if (-x < -location)
   {
      result = exp( (-x+location)/scale )/2;
   }
   else
   {
      result = 1 - exp( (-location+x)/scale )/2;
   }
   return result;
} // cdf complement


template <class RealType, class Policy>
inline RealType quantile(const complemented2_type<laplace_distribution<RealType, Policy>, RealType>& c)
{
   BOOST_MATH_STD_USING // for ADL of std functions.

   // Calculate quantile.
   RealType scale = c.dist.scale();
   RealType location = c.dist.location();
   RealType q = c.param;
   RealType result = 0;

   // Checking function argument.
   const char* function = "quantile(const complemented2_type<laplace_distribution<%1%>, %1%>&)";
   if (false == c.dist.check_parameters(function, &result)) return result;
   
   // Extreme values.
   if(q == 0)
   {
       return std::numeric_limits<RealType>::infinity();
   }
   if(q == 1)
   {
       return -std::numeric_limits<RealType>::infinity();
   }
   if(false == detail::check_probability(function, q, &result, Policy())) return result;

   if (0.5 - q < 0.0)
      result = location + scale*log( static_cast<RealType>(-q*2 + 2) );
   else
      result = location - scale*log( static_cast<RealType>(q*2) );


   return result;
} // quantile

template <class RealType, class Policy>
inline RealType mean(const laplace_distribution<RealType, Policy>& dist)
{
   return dist.location();
}

template <class RealType, class Policy>
inline RealType standard_deviation(const laplace_distribution<RealType, Policy>& dist)
{
   return constants::root_two<RealType>() * dist.scale();
}

template <class RealType, class Policy>
inline RealType mode(const laplace_distribution<RealType, Policy>& dist)
{
   return dist.location();
}

template <class RealType, class Policy>
inline RealType median(const laplace_distribution<RealType, Policy>& dist)
{
   return dist.location();
}

template <class RealType, class Policy>
inline RealType skewness(const laplace_distribution<RealType, Policy>& /*dist*/)
{
   return 0;
}

template <class RealType, class Policy>
inline RealType kurtosis(const laplace_distribution<RealType, Policy>& /*dist*/)
{
   return 6;
}

template <class RealType, class Policy>
inline RealType kurtosis_excess(const laplace_distribution<RealType, Policy>& /*dist*/)
{
   return 3;
}

template <class RealType, class Policy>
inline RealType entropy(const laplace_distribution<RealType, Policy> & dist)
{
   using std::log;
   return log(2*dist.scale()*constants::e<RealType>());
}

#ifdef _MSC_VER
#  pragma warning(pop)
#endif

} // namespace math
} // namespace boost

// This include must be at the end, *after* the accessors
// for this distribution have been defined, in order to
// keep compilers that support two-phase lookup happy.
#include <boost/math/distributions/detail/derived_accessors.hpp>

#endif // BOOST_STATS_LAPLACE_HPP


