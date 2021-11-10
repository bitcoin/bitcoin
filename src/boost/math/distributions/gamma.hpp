//  Copyright John Maddock 2006.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_STATS_GAMMA_HPP
#define BOOST_STATS_GAMMA_HPP

// http://www.itl.nist.gov/div898/handbook/eda/section3/eda366b.htm
// http://mathworld.wolfram.com/GammaDistribution.html
// http://en.wikipedia.org/wiki/Gamma_distribution

#include <boost/math/distributions/fwd.hpp>
#include <boost/math/special_functions/gamma.hpp>
#include <boost/math/special_functions/digamma.hpp>
#include <boost/math/distributions/detail/common_error_handling.hpp>
#include <boost/math/distributions/complement.hpp>

#include <utility>

namespace boost{ namespace math
{
namespace detail
{

template <class RealType, class Policy>
inline bool check_gamma_shape(
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
inline bool check_gamma_x(
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
inline bool check_gamma(
      const char* function,
      RealType scale,
      RealType shape,
      RealType* result, const Policy& pol)
{
   return check_scale(function, scale, result, pol) && check_gamma_shape(function, shape, result, pol);
}

} // namespace detail

template <class RealType = double, class Policy = policies::policy<> >
class gamma_distribution
{
public:
   typedef RealType value_type;
   typedef Policy policy_type;

   gamma_distribution(RealType l_shape, RealType l_scale = 1)
      : m_shape(l_shape), m_scale(l_scale)
   {
      RealType result;
      detail::check_gamma("boost::math::gamma_distribution<%1%>::gamma_distribution", l_scale, l_shape, &result, Policy());
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

// NO typedef because of clash with name of gamma function.

template <class RealType, class Policy>
inline const std::pair<RealType, RealType> range(const gamma_distribution<RealType, Policy>& /* dist */)
{ // Range of permissible values for random variable x.
   using boost::math::tools::max_value;
   return std::pair<RealType, RealType>(static_cast<RealType>(0), max_value<RealType>());
}

template <class RealType, class Policy>
inline const std::pair<RealType, RealType> support(const gamma_distribution<RealType, Policy>& /* dist */)
{ // Range of supported values for random variable x.
   // This is range where cdf rises from 0 to 1, and outside it, the pdf is zero.
   using boost::math::tools::max_value;
   using boost::math::tools::min_value;
   return std::pair<RealType, RealType>(min_value<RealType>(),  max_value<RealType>());
}

template <class RealType, class Policy>
inline RealType pdf(const gamma_distribution<RealType, Policy>& dist, const RealType& x)
{
   BOOST_MATH_STD_USING  // for ADL of std functions

   static const char* function = "boost::math::pdf(const gamma_distribution<%1%>&, %1%)";

   RealType shape = dist.shape();
   RealType scale = dist.scale();

   RealType result = 0;
   if(false == detail::check_gamma(function, scale, shape, &result, Policy()))
      return result;
   if(false == detail::check_gamma_x(function, x, &result, Policy()))
      return result;

   if(x == 0)
   {
      return 0;
   }
   result = gamma_p_derivative(shape, x / scale, Policy()) / scale;
   return result;
} // pdf

template <class RealType, class Policy>
inline RealType cdf(const gamma_distribution<RealType, Policy>& dist, const RealType& x)
{
   BOOST_MATH_STD_USING  // for ADL of std functions

   static const char* function = "boost::math::cdf(const gamma_distribution<%1%>&, %1%)";

   RealType shape = dist.shape();
   RealType scale = dist.scale();

   RealType result = 0;
   if(false == detail::check_gamma(function, scale, shape, &result, Policy()))
      return result;
   if(false == detail::check_gamma_x(function, x, &result, Policy()))
      return result;

   result = boost::math::gamma_p(shape, x / scale, Policy());
   return result;
} // cdf

template <class RealType, class Policy>
inline RealType quantile(const gamma_distribution<RealType, Policy>& dist, const RealType& p)
{
   BOOST_MATH_STD_USING  // for ADL of std functions

   static const char* function = "boost::math::quantile(const gamma_distribution<%1%>&, %1%)";

   RealType shape = dist.shape();
   RealType scale = dist.scale();

   RealType result = 0;
   if(false == detail::check_gamma(function, scale, shape, &result, Policy()))
      return result;
   if(false == detail::check_probability(function, p, &result, Policy()))
      return result;

   if(p == 1)
      return policies::raise_overflow_error<RealType>(function, 0, Policy());

   result = gamma_p_inv(shape, p, Policy()) * scale;

   return result;
}

template <class RealType, class Policy>
inline RealType cdf(const complemented2_type<gamma_distribution<RealType, Policy>, RealType>& c)
{
   BOOST_MATH_STD_USING  // for ADL of std functions

   static const char* function = "boost::math::quantile(const gamma_distribution<%1%>&, %1%)";

   RealType shape = c.dist.shape();
   RealType scale = c.dist.scale();

   RealType result = 0;
   if(false == detail::check_gamma(function, scale, shape, &result, Policy()))
      return result;
   if(false == detail::check_gamma_x(function, c.param, &result, Policy()))
      return result;

   result = gamma_q(shape, c.param / scale, Policy());

   return result;
}

template <class RealType, class Policy>
inline RealType quantile(const complemented2_type<gamma_distribution<RealType, Policy>, RealType>& c)
{
   BOOST_MATH_STD_USING  // for ADL of std functions

   static const char* function = "boost::math::quantile(const gamma_distribution<%1%>&, %1%)";

   RealType shape = c.dist.shape();
   RealType scale = c.dist.scale();
   RealType q = c.param;

   RealType result = 0;
   if(false == detail::check_gamma(function, scale, shape, &result, Policy()))
      return result;
   if(false == detail::check_probability(function, q, &result, Policy()))
      return result;

   if(q == 0)
      return policies::raise_overflow_error<RealType>(function, 0, Policy());

   result = gamma_q_inv(shape, q, Policy()) * scale;

   return result;
}

template <class RealType, class Policy>
inline RealType mean(const gamma_distribution<RealType, Policy>& dist)
{
   BOOST_MATH_STD_USING  // for ADL of std functions

   static const char* function = "boost::math::mean(const gamma_distribution<%1%>&)";

   RealType shape = dist.shape();
   RealType scale = dist.scale();

   RealType result = 0;
   if(false == detail::check_gamma(function, scale, shape, &result, Policy()))
      return result;

   result = shape * scale;
   return result;
}

template <class RealType, class Policy>
inline RealType variance(const gamma_distribution<RealType, Policy>& dist)
{
   BOOST_MATH_STD_USING  // for ADL of std functions

   static const char* function = "boost::math::variance(const gamma_distribution<%1%>&)";

   RealType shape = dist.shape();
   RealType scale = dist.scale();

   RealType result = 0;
   if(false == detail::check_gamma(function, scale, shape, &result, Policy()))
      return result;

   result = shape * scale * scale;
   return result;
}

template <class RealType, class Policy>
inline RealType mode(const gamma_distribution<RealType, Policy>& dist)
{
   BOOST_MATH_STD_USING  // for ADL of std functions

   static const char* function = "boost::math::mode(const gamma_distribution<%1%>&)";

   RealType shape = dist.shape();
   RealType scale = dist.scale();

   RealType result = 0;
   if(false == detail::check_gamma(function, scale, shape, &result, Policy()))
      return result;

   if(shape < 1)
      return policies::raise_domain_error<RealType>(
         function,
         "The mode of the gamma distribution is only defined for values of the shape parameter >= 1, but got %1%.",
         shape, Policy());

   result = (shape - 1) * scale;
   return result;
}

//template <class RealType, class Policy>
//inline RealType median(const gamma_distribution<RealType, Policy>& dist)
//{  // Rely on default definition in derived accessors.
//}

template <class RealType, class Policy>
inline RealType skewness(const gamma_distribution<RealType, Policy>& dist)
{
   BOOST_MATH_STD_USING  // for ADL of std functions

   static const char* function = "boost::math::skewness(const gamma_distribution<%1%>&)";

   RealType shape = dist.shape();
   RealType scale = dist.scale();

   RealType result = 0;
   if(false == detail::check_gamma(function, scale, shape, &result, Policy()))
      return result;

   result = 2 / sqrt(shape);
   return result;
}

template <class RealType, class Policy>
inline RealType kurtosis_excess(const gamma_distribution<RealType, Policy>& dist)
{
   BOOST_MATH_STD_USING  // for ADL of std functions

   static const char* function = "boost::math::kurtosis_excess(const gamma_distribution<%1%>&)";

   RealType shape = dist.shape();
   RealType scale = dist.scale();

   RealType result = 0;
   if(false == detail::check_gamma(function, scale, shape, &result, Policy()))
      return result;

   result = 6 / shape;
   return result;
}

template <class RealType, class Policy>
inline RealType kurtosis(const gamma_distribution<RealType, Policy>& dist)
{
   return kurtosis_excess(dist) + 3;
}

template <class RealType, class Policy>
inline RealType entropy(const gamma_distribution<RealType, Policy>& dist)
{
   RealType k = dist.shape();
   RealType theta = dist.scale();
   using std::log;
   return k + log(theta) + lgamma(k) + (1-k)*digamma(k);
}

} // namespace math
} // namespace boost

// This include must be at the end, *after* the accessors
// for this distribution have been defined, in order to
// keep compilers that support two-phase lookup happy.
#include <boost/math/distributions/detail/derived_accessors.hpp>

#endif // BOOST_STATS_GAMMA_HPP


