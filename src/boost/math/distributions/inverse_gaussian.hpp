//  Copyright John Maddock 2010.
//  Copyright Paul A. Bristow 2010.

//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_STATS_INVERSE_GAUSSIAN_HPP
#define BOOST_STATS_INVERSE_GAUSSIAN_HPP

#ifdef _MSC_VER
#pragma warning(disable: 4512) // assignment operator could not be generated
#endif

// http://en.wikipedia.org/wiki/Normal-inverse_Gaussian_distribution
// http://mathworld.wolfram.com/InverseGaussianDistribution.html

// The normal-inverse Gaussian distribution
// also called the Wald distribution (some sources limit this to when mean = 1).

// It is the continuous probability distribution
// that is defined as the normal variance-mean mixture where the mixing density is the 
// inverse Gaussian distribution. The tails of the distribution decrease more slowly
// than the normal distribution. It is therefore suitable to model phenomena
// where numerically large values are more probable than is the case for the normal distribution.

// The Inverse Gaussian distribution was first studied in relationship to Brownian motion.
// In 1956 M.C.K. Tweedie used the name 'Inverse Gaussian' because there is an inverse 
// relationship between the time to cover a unit distance and distance covered in unit time.

// Examples are returns from financial assets and turbulent wind speeds. 
// The normal-inverse Gaussian distributions form
// a subclass of the generalised hyperbolic distributions.

// See also

// http://en.wikipedia.org/wiki/Normal_distribution
// http://www.itl.nist.gov/div898/handbook/eda/section3/eda3661.htm
// Also:
// Weisstein, Eric W. "Normal Distribution."
// From MathWorld--A Wolfram Web Resource.
// http://mathworld.wolfram.com/NormalDistribution.html

// http://www.jstatsoft.org/v26/i04/paper General class of inverse Gaussian distributions.
// ig package - withdrawn but at http://cran.r-project.org/src/contrib/Archive/ig/

// http://www.stat.ucl.ac.be/ISdidactique/Rhelp/library/SuppDists/html/inverse_gaussian.html
// R package for dinverse_gaussian, ...

// http://www.statsci.org/s/inverse_gaussian.s  and http://www.statsci.org/s/inverse_gaussian.html

//#include <boost/math/distributions/fwd.hpp>
#include <boost/math/special_functions/erf.hpp> // for erf/erfc.
#include <boost/math/distributions/complement.hpp>
#include <boost/math/distributions/detail/common_error_handling.hpp>
#include <boost/math/distributions/normal.hpp>
#include <boost/math/distributions/gamma.hpp> // for gamma function
// using boost::math::gamma_p;

#include <boost/math/tools/tuple.hpp>
//using std::tr1::tuple;
//using std::tr1::make_tuple;
#include <boost/math/tools/roots.hpp>
//using boost::math::tools::newton_raphson_iterate;

#include <utility>

namespace boost{ namespace math{

template <class RealType = double, class Policy = policies::policy<> >
class inverse_gaussian_distribution
{
public:
   typedef RealType value_type;
   typedef Policy policy_type;

   inverse_gaussian_distribution(RealType l_mean = 1, RealType l_scale = 1)
      : m_mean(l_mean), m_scale(l_scale)
   { // Default is a 1,1 inverse_gaussian distribution.
     static const char* function = "boost::math::inverse_gaussian_distribution<%1%>::inverse_gaussian_distribution";

     RealType result;
     detail::check_scale(function, l_scale, &result, Policy());
     detail::check_location(function, l_mean, &result, Policy());
     detail::check_x_gt0(function, l_mean, &result, Policy());
   }

   RealType mean()const
   { // alias for location.
      return m_mean; // aka mu
   }

   // Synonyms, provided to allow generic use of find_location and find_scale.
   RealType location()const
   { // location, aka mu.
      return m_mean;
   }
   RealType scale()const
   { // scale, aka lambda.
      return m_scale;
   }

   RealType shape()const
   { // shape, aka phi = lambda/mu.
      return m_scale / m_mean;
   }

private:
   //
   // Data members:
   //
   RealType m_mean;  // distribution mean or location, aka mu.
   RealType m_scale;    // distribution standard deviation or scale, aka lambda.
}; // class normal_distribution

typedef inverse_gaussian_distribution<double> inverse_gaussian;

template <class RealType, class Policy>
inline const std::pair<RealType, RealType> range(const inverse_gaussian_distribution<RealType, Policy>& /*dist*/)
{ // Range of permissible values for random variable x, zero to max.
   using boost::math::tools::max_value;
   return std::pair<RealType, RealType>(static_cast<RealType>(0.), max_value<RealType>()); // - to + max value.
}

template <class RealType, class Policy>
inline const std::pair<RealType, RealType> support(const inverse_gaussian_distribution<RealType, Policy>& /*dist*/)
{ // Range of supported values for random variable x, zero to max.
  // This is range where cdf rises from 0 to 1, and outside it, the pdf is zero.
   using boost::math::tools::max_value;
   return std::pair<RealType, RealType>(static_cast<RealType>(0.),  max_value<RealType>()); // - to + max value.
}

template <class RealType, class Policy>
inline RealType pdf(const inverse_gaussian_distribution<RealType, Policy>& dist, const RealType& x)
{ // Probability Density Function
   BOOST_MATH_STD_USING  // for ADL of std functions

   RealType scale = dist.scale();
   RealType mean = dist.mean();
   RealType result = 0;
   static const char* function = "boost::math::pdf(const inverse_gaussian_distribution<%1%>&, %1%)";
   if(false == detail::check_scale(function, scale, &result, Policy()))
   {
      return result;
   }
   if(false == detail::check_location(function, mean, &result, Policy()))
   {
      return result;
   }
   if(false == detail::check_x_gt0(function, mean, &result, Policy()))
   {
      return result;
   }
   if(false == detail::check_positive_x(function, x, &result, Policy()))
   {
      return result;
   }

   if (x == 0)
   {
     return 0; // Convenient, even if not defined mathematically.
   }

   result =
     sqrt(scale / (constants::two_pi<RealType>() * x * x * x))
    * exp(-scale * (x - mean) * (x - mean) / (2 * x * mean * mean));
   return result;
} // pdf

template <class RealType, class Policy>
inline RealType cdf(const inverse_gaussian_distribution<RealType, Policy>& dist, const RealType& x)
{ // Cumulative Density Function.
   BOOST_MATH_STD_USING  // for ADL of std functions.

   RealType scale = dist.scale();
   RealType mean = dist.mean();
   static const char* function = "boost::math::cdf(const inverse_gaussian_distribution<%1%>&, %1%)";
   RealType result = 0;
   if(false == detail::check_scale(function, scale, &result, Policy()))
   {
      return result;
   }
   if(false == detail::check_location(function, mean, &result, Policy()))
   {
      return result;
   }
   if (false == detail::check_x_gt0(function, mean, &result, Policy()))
   {
      return result;
   }
   if(false == detail::check_positive_x(function, x, &result, Policy()))
   {
     return result;
   }
   if (x == 0)
   {
     return 0; // Convenient, even if not defined mathematically.
   }
   // Problem with this formula for large scale > 1000 or small x, 
   //result = 0.5 * (erf(sqrt(scale / x) * ((x / mean) - 1) / constants::root_two<RealType>(), Policy()) + 1)
   //  + exp(2 * scale / mean) / 2 
   //  * (1 - erf(sqrt(scale / x) * (x / mean + 1) / constants::root_two<RealType>(), Policy()));
   // so use normal distribution version:
   // Wikipedia CDF equation http://en.wikipedia.org/wiki/Inverse_Gaussian_distribution.

   normal_distribution<RealType> n01;

   RealType n0 = sqrt(scale / x);
   n0 *= ((x / mean) -1);
   RealType n1 = cdf(n01, n0);
   RealType expfactor = exp(2 * scale / mean);
   RealType n3 = - sqrt(scale / x);
   n3 *= (x / mean) + 1;
   RealType n4 = cdf(n01, n3);
   result = n1 + expfactor * n4;
   return result;
} // cdf

template <class RealType, class Policy>
struct inverse_gaussian_quantile_functor
{ 

  inverse_gaussian_quantile_functor(const boost::math::inverse_gaussian_distribution<RealType, Policy> dist, RealType const& p)
    : distribution(dist), prob(p)
  {
  }
  boost::math::tuple<RealType, RealType> operator()(RealType const& x)
  {
    RealType c = cdf(distribution, x);
    RealType fx = c - prob;  // Difference cdf - value - to minimize.
    RealType dx = pdf(distribution, x); // pdf is 1st derivative.
    // return both function evaluation difference f(x) and 1st derivative f'(x).
    return boost::math::make_tuple(fx, dx);
  }
  private:
  const boost::math::inverse_gaussian_distribution<RealType, Policy> distribution;
  RealType prob; 
};

template <class RealType, class Policy>
struct inverse_gaussian_quantile_complement_functor
{ 
    inverse_gaussian_quantile_complement_functor(const boost::math::inverse_gaussian_distribution<RealType, Policy> dist, RealType const& p)
    : distribution(dist), prob(p)
  {
  }
  boost::math::tuple<RealType, RealType> operator()(RealType const& x)
  {
    RealType c = cdf(complement(distribution, x));
    RealType fx = c - prob;  // Difference cdf - value - to minimize.
    RealType dx = -pdf(distribution, x); // pdf is 1st derivative.
    // return both function evaluation difference f(x) and 1st derivative f'(x).
    //return std::tr1::make_tuple(fx, dx); if available.
    return boost::math::make_tuple(fx, dx);
  }
  private:
  const boost::math::inverse_gaussian_distribution<RealType, Policy> distribution;
  RealType prob; 
};

namespace detail
{
  template <class RealType>
  inline RealType guess_ig(RealType p, RealType mu = 1, RealType lambda = 1)
  { // guess at random variate value x for inverse gaussian quantile.
      BOOST_MATH_STD_USING
      using boost::math::policies::policy;
      // Error type.
      using boost::math::policies::overflow_error;
      // Action.
      using boost::math::policies::ignore_error;

      typedef policy<
        overflow_error<ignore_error> // Ignore overflow (return infinity)
      > no_overthrow_policy;

    RealType x; // result is guess at random variate value x.
    RealType phi = lambda / mu;
    if (phi > 2.)
    { // Big phi, so starting to look like normal Gaussian distribution.
      //    x=(qnorm(p,0,1,true,false) - 0.5 * sqrt(mu/lambda)) / sqrt(lambda/mu);
      // Whitmore, G.A. and Yalovsky, M.
      // A normalising logarithmic transformation for inverse Gaussian random variables,
      // Technometrics 20-2, 207-208 (1978), but using expression from
      // V Seshadri, Inverse Gaussian distribution (1998) ISBN 0387 98618 9, page 6.
 
      normal_distribution<RealType, no_overthrow_policy> n01;
      x = mu * exp(quantile(n01, p) / sqrt(phi) - 1/(2 * phi));
     }
    else
    { // phi < 2 so much less symmetrical with long tail,
      // so use gamma distribution as an approximation.
      using boost::math::gamma_distribution;

      // Define the distribution, using gamma_nooverflow:
      typedef gamma_distribution<RealType, no_overthrow_policy> gamma_nooverflow;

      gamma_nooverflow g(static_cast<RealType>(0.5), static_cast<RealType>(1.));

      // gamma_nooverflow g(static_cast<RealType>(0.5), static_cast<RealType>(1.));
      // R qgamma(0.2, 0.5, 1)  0.0320923
      RealType qg = quantile(complement(g, p));
      //RealType qg1 = qgamma(1.- p, 0.5, 1.0, true, false);
      x = lambda / (qg * 2);
      // 
      if (x > mu/2) // x > mu /2?
      { // x too large for the gamma approximation to work well.
        //x = qgamma(p, 0.5, 1.0); // qgamma(0.270614, 0.5, 1) = 0.05983807
        RealType q = quantile(g, p);
       // x = mu * exp(q * static_cast<RealType>(0.1));  // Said to improve at high p
       // x = mu * x;  // Improves at high p?
        x = mu * exp(q / sqrt(phi) - 1/(2 * phi));
      }
    }
    return x;
  }  // guess_ig
} // namespace detail

template <class RealType, class Policy>
inline RealType quantile(const inverse_gaussian_distribution<RealType, Policy>& dist, const RealType& p)
{
   BOOST_MATH_STD_USING  // for ADL of std functions.
   // No closed form exists so guess and use Newton Raphson iteration.

   RealType mean = dist.mean();
   RealType scale = dist.scale();
   static const char* function = "boost::math::quantile(const inverse_gaussian_distribution<%1%>&, %1%)";

   RealType result = 0;
   if(false == detail::check_scale(function, scale, &result, Policy()))
      return result;
   if(false == detail::check_location(function, mean, &result, Policy()))
      return result;
   if (false == detail::check_x_gt0(function, mean, &result, Policy()))
      return result;
   if(false == detail::check_probability(function, p, &result, Policy()))
      return result;
   if (p == 0)
   {
     return 0; // Convenient, even if not defined mathematically?
   }
   if (p == 1)
   { // overflow 
      result = policies::raise_overflow_error<RealType>(function,
        "probability parameter is 1, but must be < 1!", Policy());
      return result; // std::numeric_limits<RealType>::infinity();
   }

  RealType guess = detail::guess_ig(p, dist.mean(), dist.scale());
  using boost::math::tools::max_value;

  RealType min = 0.; // Minimum possible value is bottom of range of distribution.
  RealType max = max_value<RealType>();// Maximum possible value is top of range. 
  // int digits = std::numeric_limits<RealType>::digits; // Maximum possible binary digits accuracy for type T.
  // digits used to control how accurate to try to make the result.
  // To allow user to control accuracy versus speed,
  int get_digits = policies::digits<RealType, Policy>();// get digits from policy, 
  std::uintmax_t m = policies::get_max_root_iterations<Policy>(); // and max iterations.
  using boost::math::tools::newton_raphson_iterate;
  result =
    newton_raphson_iterate(inverse_gaussian_quantile_functor<RealType, Policy>(dist, p), guess, min, max, get_digits, m);
   return result;
} // quantile

template <class RealType, class Policy>
inline RealType cdf(const complemented2_type<inverse_gaussian_distribution<RealType, Policy>, RealType>& c)
{
   BOOST_MATH_STD_USING  // for ADL of std functions.

   RealType scale = c.dist.scale();
   RealType mean = c.dist.mean();
   RealType x = c.param;
   static const char* function = "boost::math::cdf(const complement(inverse_gaussian_distribution<%1%>&), %1%)";
   // infinite arguments not supported.
   //if((boost::math::isinf)(x))
   //{
   //  if(x < 0) return 1; // cdf complement -infinity is unity.
   //  return 0; // cdf complement +infinity is zero
   //}
   // These produce MSVC 4127 warnings, so the above used instead.
   //if(std::numeric_limits<RealType>::has_infinity && x == std::numeric_limits<RealType>::infinity())
   //{ // cdf complement +infinity is zero.
   //  return 0;
   //}
   //if(std::numeric_limits<RealType>::has_infinity && x == -std::numeric_limits<RealType>::infinity())
   //{ // cdf complement -infinity is unity.
   //  return 1;
   //}
   RealType result = 0;
   if(false == detail::check_scale(function, scale, &result, Policy()))
      return result;
   if(false == detail::check_location(function, mean, &result, Policy()))
      return result;
   if (false == detail::check_x_gt0(function, mean, &result, Policy()))
      return result;
   if(false == detail::check_positive_x(function, x, &result, Policy()))
      return result;

   normal_distribution<RealType> n01;
   RealType n0 = sqrt(scale / x);
   n0 *= ((x / mean) -1);
   RealType cdf_1 = cdf(complement(n01, n0));

   RealType expfactor = exp(2 * scale / mean);
   RealType n3 = - sqrt(scale / x);
   n3 *= (x / mean) + 1;

   //RealType n5 = +sqrt(scale/x) * ((x /mean) + 1); // note now positive sign.
   RealType n6 = cdf(complement(n01, +sqrt(scale/x) * ((x /mean) + 1)));
   // RealType n4 = cdf(n01, n3); // = 
   result = cdf_1 - expfactor * n6; 
   return result;
} // cdf complement

template <class RealType, class Policy>
inline RealType quantile(const complemented2_type<inverse_gaussian_distribution<RealType, Policy>, RealType>& c)
{
   BOOST_MATH_STD_USING  // for ADL of std functions

   RealType scale = c.dist.scale();
   RealType mean = c.dist.mean();
   static const char* function = "boost::math::quantile(const complement(inverse_gaussian_distribution<%1%>&), %1%)";
   RealType result = 0;
   if(false == detail::check_scale(function, scale, &result, Policy()))
      return result;
   if(false == detail::check_location(function, mean, &result, Policy()))
      return result;
   if (false == detail::check_x_gt0(function, mean, &result, Policy()))
      return result;
   RealType q = c.param;
   if(false == detail::check_probability(function, q, &result, Policy()))
      return result;

   RealType guess = detail::guess_ig(q, mean, scale);
   // Complement.
   using boost::math::tools::max_value;

  RealType min = 0.; // Minimum possible value is bottom of range of distribution.
  RealType max = max_value<RealType>();// Maximum possible value is top of range. 
  // int digits = std::numeric_limits<RealType>::digits; // Maximum possible binary digits accuracy for type T.
  // digits used to control how accurate to try to make the result.
  int get_digits = policies::digits<RealType, Policy>();
  std::uintmax_t m = policies::get_max_root_iterations<Policy>();
  using boost::math::tools::newton_raphson_iterate;
  result =
    newton_raphson_iterate(inverse_gaussian_quantile_complement_functor<RealType, Policy>(c.dist, q), guess, min, max, get_digits, m);
   return result;
} // quantile

template <class RealType, class Policy>
inline RealType mean(const inverse_gaussian_distribution<RealType, Policy>& dist)
{ // aka mu
   return dist.mean();
}

template <class RealType, class Policy>
inline RealType scale(const inverse_gaussian_distribution<RealType, Policy>& dist)
{ // aka lambda
   return dist.scale();
}

template <class RealType, class Policy>
inline RealType shape(const inverse_gaussian_distribution<RealType, Policy>& dist)
{ // aka phi
   return dist.shape();
}

template <class RealType, class Policy>
inline RealType standard_deviation(const inverse_gaussian_distribution<RealType, Policy>& dist)
{
  BOOST_MATH_STD_USING
  RealType scale = dist.scale();
  RealType mean = dist.mean();
  RealType result = sqrt(mean * mean * mean / scale);
  return result;
}

template <class RealType, class Policy>
inline RealType mode(const inverse_gaussian_distribution<RealType, Policy>& dist)
{
  BOOST_MATH_STD_USING
  RealType scale = dist.scale();
  RealType  mean = dist.mean();
  RealType result = mean * (sqrt(1 + (9 * mean * mean)/(4 * scale * scale)) 
      - 3 * mean / (2 * scale));
  return result;
}

template <class RealType, class Policy>
inline RealType skewness(const inverse_gaussian_distribution<RealType, Policy>& dist)
{
  BOOST_MATH_STD_USING
  RealType scale = dist.scale();
  RealType  mean = dist.mean();
  RealType result = 3 * sqrt(mean/scale);
  return result;
}

template <class RealType, class Policy>
inline RealType kurtosis(const inverse_gaussian_distribution<RealType, Policy>& dist)
{
  RealType scale = dist.scale();
  RealType  mean = dist.mean();
  RealType result = 15 * mean / scale -3;
  return result;
}

template <class RealType, class Policy>
inline RealType kurtosis_excess(const inverse_gaussian_distribution<RealType, Policy>& dist)
{
  RealType scale = dist.scale();
  RealType  mean = dist.mean();
  RealType result = 15 * mean / scale;
  return result;
}

} // namespace math
} // namespace boost

// This include must be at the end, *after* the accessors
// for this distribution have been defined, in order to
// keep compilers that support two-phase lookup happy.
#include <boost/math/distributions/detail/derived_accessors.hpp>

#endif // BOOST_STATS_INVERSE_GAUSSIAN_HPP


