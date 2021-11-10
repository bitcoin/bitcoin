//  Copyright John Maddock 2006.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_STATS_WEIBULL_HPP
#define BOOST_STATS_WEIBULL_HPP

// http://www.itl.nist.gov/div898/handbook/eda/section3/eda3668.htm
// http://mathworld.wolfram.com/WeibullDistribution.html

#include <boost/math/distributions/fwd.hpp>
#include <boost/math/special_functions/gamma.hpp>
#include <boost/math/special_functions/log1p.hpp>
#include <boost/math/special_functions/expm1.hpp>
#include <boost/math/distributions/detail/common_error_handling.hpp>
#include <boost/math/distributions/complement.hpp>

#include <utility>

namespace boost{ namespace math
{
namespace detail{

template <class RealType, class Policy>
inline bool check_weibull_shape(
      const char* function,
      RealType shape,
      RealType* result, const Policy& pol)
{
   if((shape <= 0) || !(boost::math::isfinite)(shape))
   {
      *result = policies::raise_domain_error<RealType>(
         function,
         "Shape parameter is %1%, but must be > 0 !", shape, pol);
      return false;
   }
   return true;
}

template <class RealType, class Policy>
inline bool check_weibull_x(
      const char* function,
      RealType const& x,
      RealType* result, const Policy& pol)
{
   if((x < 0) || !(boost::math::isfinite)(x))
   {
      *result = policies::raise_domain_error<RealType>(
         function,
         "Random variate is %1% but must be >= 0 !", x, pol);
      return false;
   }
   return true;
}

template <class RealType, class Policy>
inline bool check_weibull(
      const char* function,
      RealType scale,
      RealType shape,
      RealType* result, const Policy& pol)
{
   return check_scale(function, scale, result, pol) && check_weibull_shape(function, shape, result, pol);
}

} // namespace detail

template <class RealType = double, class Policy = policies::policy<> >
class weibull_distribution
{
public:
   typedef RealType value_type;
   typedef Policy policy_type;

   weibull_distribution(RealType l_shape, RealType l_scale = 1)
      : m_shape(l_shape), m_scale(l_scale)
   {
      RealType result;
      detail::check_weibull("boost::math::weibull_distribution<%1%>::weibull_distribution", l_scale, l_shape, &result, Policy());
   }

   RealType shape()const
   {
      return m_shape;
   }

   RealType scale()const
   {
      return m_scale;
   }
private:
   //
   // Data members:
   //
   RealType m_shape;     // distribution shape
   RealType m_scale;     // distribution scale
};

typedef weibull_distribution<double> weibull;

template <class RealType, class Policy>
inline const std::pair<RealType, RealType> range(const weibull_distribution<RealType, Policy>& /*dist*/)
{ // Range of permissible values for random variable x.
   using boost::math::tools::max_value;
   return std::pair<RealType, RealType>(static_cast<RealType>(0), max_value<RealType>());
}

template <class RealType, class Policy>
inline const std::pair<RealType, RealType> support(const weibull_distribution<RealType, Policy>& /*dist*/)
{ // Range of supported values for random variable x.
   // This is range where cdf rises from 0 to 1, and outside it, the pdf is zero.
   using boost::math::tools::max_value;
   using boost::math::tools::min_value;
   return std::pair<RealType, RealType>(min_value<RealType>(),  max_value<RealType>());
   // A discontinuity at x == 0, so only support down to min_value.
}

template <class RealType, class Policy>
inline RealType pdf(const weibull_distribution<RealType, Policy>& dist, const RealType& x)
{
   BOOST_MATH_STD_USING  // for ADL of std functions

   static const char* function = "boost::math::pdf(const weibull_distribution<%1%>, %1%)";

   RealType shape = dist.shape();
   RealType scale = dist.scale();

   RealType result = 0;
   if(false == detail::check_weibull(function, scale, shape, &result, Policy()))
      return result;
   if(false == detail::check_weibull_x(function, x, &result, Policy()))
      return result;

   if(x == 0)
   {
      if(shape == 1)
      {
         return 1 / scale;
      }
      if(shape > 1)
      {
         return 0;
      }
      return policies::raise_overflow_error<RealType>(function, 0, Policy());
   }
   result = exp(-pow(x / scale, shape));
   result *= pow(x / scale, shape - 1) * shape / scale;

   return result;
}

template <class RealType, class Policy>
inline RealType cdf(const weibull_distribution<RealType, Policy>& dist, const RealType& x)
{
   BOOST_MATH_STD_USING  // for ADL of std functions

   static const char* function = "boost::math::cdf(const weibull_distribution<%1%>, %1%)";

   RealType shape = dist.shape();
   RealType scale = dist.scale();

   RealType result = 0;
   if(false == detail::check_weibull(function, scale, shape, &result, Policy()))
      return result;
   if(false == detail::check_weibull_x(function, x, &result, Policy()))
      return result;

   result = -boost::math::expm1(-pow(x / scale, shape), Policy());

   return result;
}

template <class RealType, class Policy>
inline RealType quantile(const weibull_distribution<RealType, Policy>& dist, const RealType& p)
{
   BOOST_MATH_STD_USING  // for ADL of std functions

   static const char* function = "boost::math::quantile(const weibull_distribution<%1%>, %1%)";

   RealType shape = dist.shape();
   RealType scale = dist.scale();

   RealType result = 0;
   if(false == detail::check_weibull(function, scale, shape, &result, Policy()))
      return result;
   if(false == detail::check_probability(function, p, &result, Policy()))
      return result;

   if(p == 1)
      return policies::raise_overflow_error<RealType>(function, 0, Policy());

   result = scale * pow(-boost::math::log1p(-p, Policy()), 1 / shape);

   return result;
}

template <class RealType, class Policy>
inline RealType cdf(const complemented2_type<weibull_distribution<RealType, Policy>, RealType>& c)
{
   BOOST_MATH_STD_USING  // for ADL of std functions

   static const char* function = "boost::math::cdf(const weibull_distribution<%1%>, %1%)";

   RealType shape = c.dist.shape();
   RealType scale = c.dist.scale();

   RealType result = 0;
   if(false == detail::check_weibull(function, scale, shape, &result, Policy()))
      return result;
   if(false == detail::check_weibull_x(function, c.param, &result, Policy()))
      return result;

   result = exp(-pow(c.param / scale, shape));

   return result;
}

template <class RealType, class Policy>
inline RealType quantile(const complemented2_type<weibull_distribution<RealType, Policy>, RealType>& c)
{
   BOOST_MATH_STD_USING  // for ADL of std functions

   static const char* function = "boost::math::quantile(const weibull_distribution<%1%>, %1%)";

   RealType shape = c.dist.shape();
   RealType scale = c.dist.scale();
   RealType q = c.param;

   RealType result = 0;
   if(false == detail::check_weibull(function, scale, shape, &result, Policy()))
      return result;
   if(false == detail::check_probability(function, q, &result, Policy()))
      return result;

   if(q == 0)
      return policies::raise_overflow_error<RealType>(function, 0, Policy());

   result = scale * pow(-log(q), 1 / shape);

   return result;
}

template <class RealType, class Policy>
inline RealType mean(const weibull_distribution<RealType, Policy>& dist)
{
   BOOST_MATH_STD_USING  // for ADL of std functions

   static const char* function = "boost::math::mean(const weibull_distribution<%1%>)";

   RealType shape = dist.shape();
   RealType scale = dist.scale();

   RealType result = 0;
   if(false == detail::check_weibull(function, scale, shape, &result, Policy()))
      return result;

   result = scale * boost::math::tgamma(1 + 1 / shape, Policy());
   return result;
}

template <class RealType, class Policy>
inline RealType variance(const weibull_distribution<RealType, Policy>& dist)
{
   RealType shape = dist.shape();
   RealType scale = dist.scale();

   static const char* function = "boost::math::variance(const weibull_distribution<%1%>)";

   RealType result = 0;
   if(false == detail::check_weibull(function, scale, shape, &result, Policy()))
   {
      return result;
   }
   result = boost::math::tgamma(1 + 1 / shape, Policy());
   result *= -result;
   result += boost::math::tgamma(1 + 2 / shape, Policy());
   result *= scale * scale;
   return result;
}

template <class RealType, class Policy>
inline RealType mode(const weibull_distribution<RealType, Policy>& dist)
{
   BOOST_MATH_STD_USING  // for ADL of std function pow.

   static const char* function = "boost::math::mode(const weibull_distribution<%1%>)";

   RealType shape = dist.shape();
   RealType scale = dist.scale();

   RealType result = 0;
   if(false == detail::check_weibull(function, scale, shape, &result, Policy()))
   {
      return result;
   }
   if(shape <= 1)
      return 0;
   result = scale * pow((shape - 1) / shape, 1 / shape);
   return result;
}

template <class RealType, class Policy>
inline RealType median(const weibull_distribution<RealType, Policy>& dist)
{
   BOOST_MATH_STD_USING  // for ADL of std function pow.

   static const char* function = "boost::math::median(const weibull_distribution<%1%>)";

   RealType shape = dist.shape(); // Wikipedia k
   RealType scale = dist.scale(); // Wikipedia lambda

   RealType result = 0;
   if(false == detail::check_weibull(function, scale, shape, &result, Policy()))
   {
      return result;
   }
   using boost::math::constants::ln_two;
   result = scale * pow(ln_two<RealType>(), 1 / shape);
   return result;
}

template <class RealType, class Policy>
inline RealType skewness(const weibull_distribution<RealType, Policy>& dist)
{
   BOOST_MATH_STD_USING  // for ADL of std functions

   static const char* function = "boost::math::skewness(const weibull_distribution<%1%>)";

   RealType shape = dist.shape();
   RealType scale = dist.scale();

   RealType result = 0;
   if(false == detail::check_weibull(function, scale, shape, &result, Policy()))
   {
      return result;
   }
   RealType g1, g2, g3, d;

   g1 = boost::math::tgamma(1 + 1 / shape, Policy());
   g2 = boost::math::tgamma(1 + 2 / shape, Policy());
   g3 = boost::math::tgamma(1 + 3 / shape, Policy());
   d = pow(g2 - g1 * g1, RealType(1.5));

   result = (2 * g1 * g1 * g1 - 3 * g1 * g2 + g3) / d;
   return result;
}

template <class RealType, class Policy>
inline RealType kurtosis_excess(const weibull_distribution<RealType, Policy>& dist)
{
   BOOST_MATH_STD_USING  // for ADL of std functions

   static const char* function = "boost::math::kurtosis_excess(const weibull_distribution<%1%>)";

   RealType shape = dist.shape();
   RealType scale = dist.scale();

   RealType result = 0;
   if(false == detail::check_weibull(function, scale, shape, &result, Policy()))
      return result;

   RealType g1, g2, g3, g4, d, g1_2, g1_4;

   g1 = boost::math::tgamma(1 + 1 / shape, Policy());
   g2 = boost::math::tgamma(1 + 2 / shape, Policy());
   g3 = boost::math::tgamma(1 + 3 / shape, Policy());
   g4 = boost::math::tgamma(1 + 4 / shape, Policy());
   g1_2 = g1 * g1;
   g1_4 = g1_2 * g1_2;
   d = g2 - g1_2;
   d *= d;

   result = -6 * g1_4 + 12 * g1_2 * g2 - 3 * g2 * g2 - 4 * g1 * g3 + g4;
   result /= d;
   return result;
}

template <class RealType, class Policy>
inline RealType kurtosis(const weibull_distribution<RealType, Policy>& dist)
{
   return kurtosis_excess(dist) + 3;
}

template <class RealType, class Policy>
inline RealType entropy(const weibull_distribution<RealType, Policy>& dist)
{
   using std::log;
   RealType k = dist.shape();
   RealType lambda = dist.scale();
   return constants::euler<RealType>()*(1-1/k) + log(lambda/k) + 1;
}

} // namespace math
} // namespace boost

// This include must be at the end, *after* the accessors
// for this distribution have been defined, in order to
// keep compilers that support two-phase lookup happy.
#include <boost/math/distributions/detail/derived_accessors.hpp>

#endif // BOOST_STATS_WEIBULL_HPP


