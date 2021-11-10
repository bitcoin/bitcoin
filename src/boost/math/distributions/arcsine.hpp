// boost/math/distributions/arcsine.hpp

// Copyright John Maddock 2014.
// Copyright Paul A. Bristow 2014.

// Use, modification and distribution are subject to the
// Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

// http://en.wikipedia.org/wiki/arcsine_distribution

// The arcsine Distribution is a continuous probability distribution.
// http://en.wikipedia.org/wiki/Arcsine_distribution
// http://www.wolframalpha.com/input/?i=ArcSinDistribution

// Standard arcsine distribution is a special case of beta distribution with both a & b = one half,
// and 0 <= x <= 1.

// It is generalized to include any bounded support a <= x <= b from 0 <= x <= 1
// by Wolfram and Wikipedia,
// but using location and scale parameters by
// Virtual Laboratories in Probability and Statistics http://www.math.uah.edu/stat/index.html
// http://www.math.uah.edu/stat/special/Arcsine.html
// The end-point version is simpler and more obvious, so we implement that.
// TODO Perhaps provide location and scale functions?


#ifndef BOOST_MATH_DIST_ARCSINE_HPP
#define BOOST_MATH_DIST_ARCSINE_HPP

#include <boost/math/distributions/fwd.hpp>
#include <boost/math/distributions/complement.hpp> // complements.
#include <boost/math/distributions/detail/common_error_handling.hpp> // error checks.
#include <boost/math/constants/constants.hpp>

#include <boost/math/special_functions/fpclassify.hpp> // isnan.

#if defined (BOOST_MSVC)
#  pragma warning(push)
#  pragma warning(disable: 4702) // Unreachable code,
// in domain_error_imp in error_handling.
#endif

#include <utility>
#include <exception>  // For std::domain_error.

namespace boost
{
  namespace math
  {
    namespace arcsine_detail
    {
      // Common error checking routines for arcsine distribution functions:
      // Duplicating for x_min and x_max provides specific error messages.
      template <class RealType, class Policy>
      inline bool check_x_min(const char* function, const RealType& x, RealType* result, const Policy& pol)
      {
        if (!(boost::math::isfinite)(x))
        {
          *result = policies::raise_domain_error<RealType>(
            function,
            "x_min argument is %1%, but must be finite !", x, pol);
          return false;
        }
        return true;
      } // bool check_x_min

      template <class RealType, class Policy>
      inline bool check_x_max(const char* function, const RealType& x, RealType* result, const Policy& pol)
      {
        if (!(boost::math::isfinite)(x))
        {
          *result = policies::raise_domain_error<RealType>(
            function,
            "x_max argument is %1%, but must be finite !", x, pol);
          return false;
        }
        return true;
      } // bool check_x_max


      template <class RealType, class Policy>
      inline bool check_x_minmax(const char* function, const RealType& x_min, const RealType& x_max, RealType* result, const Policy& pol)
      { // Check x_min < x_max
        if (x_min >= x_max)
        {
          std::string msg = "x_max argument is %1%, but must be > x_min";
          *result = policies::raise_domain_error<RealType>(
            function,
            msg.c_str(), x_max, pol);
            // "x_max argument is %1%, but must be > x_min !", x_max, pol);
            // "x_max argument is %1%, but must be > x_min %2!", x_max, x_min, pol); would be better. 
            // But would require replication of all helpers functions in /policies/error_handling.hpp for two values,
            // as well as two value versions of raise_error, raise_domain_error and do_format
          return false;
        }
        return true;
      } // bool check_x_minmax

      template <class RealType, class Policy>
      inline bool check_prob(const char* function, const RealType& p, RealType* result, const Policy& pol)
      {
        if ((p < 0) || (p > 1) || !(boost::math::isfinite)(p))
        {
          *result = policies::raise_domain_error<RealType>(
            function,
            "Probability argument is %1%, but must be >= 0 and <= 1 !", p, pol);
          return false;
        }
        return true;
      } // bool check_prob

      template <class RealType, class Policy>
      inline bool check_x(const char* function, const RealType& x_min, const RealType& x_max, const RealType& x, RealType* result, const Policy& pol)
      { // Check x finite and x_min < x < x_max.
        if (!(boost::math::isfinite)(x))
        {
          *result = policies::raise_domain_error<RealType>(
            function,
            "x argument is %1%, but must be finite !", x, pol);
          return false;
        }
        if ((x < x_min) || (x > x_max))
        {
          // std::cout << x_min << ' ' << x << x_max << std::endl;
          *result = policies::raise_domain_error<RealType>(
            function,
            "x argument is %1%, but must be x_min < x < x_max !", x, pol);
          // For example:
          // Error in function boost::math::pdf(arcsine_distribution<double> const&, double) : x argument is -1.01, but must be x_min < x < x_max !
          // TODO Perhaps show values of x_min and x_max?
          return false;
        }
        return true;
      } // bool check_x

      template <class RealType, class Policy>
      inline bool check_dist(const char* function, const RealType& x_min, const RealType& x_max, RealType* result, const Policy& pol)
      { // Check both x_min and x_max finite, and x_min  < x_max.
        return check_x_min(function, x_min, result, pol)
            && check_x_max(function, x_max, result, pol)
            && check_x_minmax(function, x_min, x_max, result, pol);
      } // bool check_dist

      template <class RealType, class Policy>
      inline bool check_dist_and_x(const char* function, const RealType& x_min, const RealType& x_max, RealType x, RealType* result, const Policy& pol)
      {
        return check_dist(function, x_min, x_max, result, pol)
          && arcsine_detail::check_x(function, x_min, x_max, x, result, pol);
      } // bool check_dist_and_x

      template <class RealType, class Policy>
      inline bool check_dist_and_prob(const char* function, const RealType& x_min, const RealType& x_max, RealType p, RealType* result, const Policy& pol)
      {
        return check_dist(function, x_min, x_max, result, pol)
          && check_prob(function, p, result, pol);
      } // bool check_dist_and_prob

    } // namespace arcsine_detail

    template <class RealType = double, class Policy = policies::policy<> >
    class arcsine_distribution
    {
    public:
      typedef RealType value_type;
      typedef Policy policy_type;

      arcsine_distribution(RealType x_min = 0, RealType x_max = 1) : m_x_min(x_min), m_x_max(x_max)
      { // Default beta (alpha = beta = 0.5) is standard arcsine with x_min = 0, x_max = 1.
        // Generalized to allow x_min and x_max to be specified.
        RealType result;
        arcsine_detail::check_dist(
          "boost::math::arcsine_distribution<%1%>::arcsine_distribution",
          m_x_min,
          m_x_max,
          &result, Policy());
      } // arcsine_distribution constructor.
      // Accessor functions:
      RealType x_min() const
      {
        return m_x_min;
      }
      RealType x_max() const
      {
        return m_x_max;
      }

    private:
      RealType m_x_min; // Two x min and x max parameters of the arcsine distribution.
      RealType m_x_max;
    }; // template <class RealType, class Policy> class arcsine_distribution

    // Convenient typedef to construct double version.
    typedef arcsine_distribution<double> arcsine;


    template <class RealType, class Policy>
    inline const std::pair<RealType, RealType> range(const arcsine_distribution<RealType, Policy>&  dist)
    { // Range of permissible values for random variable x.
      using boost::math::tools::max_value;
      return std::pair<RealType, RealType>(static_cast<RealType>(dist.x_min()), static_cast<RealType>(dist.x_max()));
    }

    template <class RealType, class Policy>
    inline const std::pair<RealType, RealType> support(const arcsine_distribution<RealType, Policy>&  dist)
    { // Range of supported values for random variable x.
      // This is range where cdf rises from 0 to 1, and outside it, the pdf is zero.
      return std::pair<RealType, RealType>(static_cast<RealType>(dist.x_min()), static_cast<RealType>(dist.x_max()));
    }

    template <class RealType, class Policy>
    inline RealType mean(const arcsine_distribution<RealType, Policy>& dist)
    { // Mean of arcsine distribution .
      RealType result;
      RealType x_min = dist.x_min();
      RealType x_max = dist.x_max();

      if (false == arcsine_detail::check_dist(
        "boost::math::mean(arcsine_distribution<%1%> const&, %1% )",
        x_min,
        x_max,
        &result, Policy())
        )
      {
        return result;
      }
      return  (x_min + x_max) / 2;
    } // mean

    template <class RealType, class Policy>
    inline RealType variance(const arcsine_distribution<RealType, Policy>& dist)
    { // Variance of standard arcsine distribution = (1-0)/8 = 0.125.
      RealType result;
      RealType x_min = dist.x_min();
      RealType x_max = dist.x_max();
      if (false == arcsine_detail::check_dist(
        "boost::math::variance(arcsine_distribution<%1%> const&, %1% )",
        x_min,
        x_max,
        &result, Policy())
        )
      {
        return result;
      }
      return  (x_max - x_min) * (x_max - x_min) / 8;
    } // variance

    template <class RealType, class Policy>
    inline RealType mode(const arcsine_distribution<RealType, Policy>& /* dist */)
    { //There are always [*two] values for the mode, at ['x_min] and at ['x_max], default 0 and 1,
      // so instead we raise the exception domain_error.
      return policies::raise_domain_error<RealType>(
        "boost::math::mode(arcsine_distribution<%1%>&)",
        "The arcsine distribution has two modes at x_min and x_max: "
        "so the return value is %1%.",
        std::numeric_limits<RealType>::quiet_NaN(), Policy());
    } // mode

    template <class RealType, class Policy>
    inline RealType median(const arcsine_distribution<RealType, Policy>& dist)
    { // Median of arcsine distribution (a + b) / 2 == mean.
      RealType x_min = dist.x_min();
      RealType x_max = dist.x_max();
      RealType result;
      if (false == arcsine_detail::check_dist(
        "boost::math::median(arcsine_distribution<%1%> const&, %1% )",
        x_min,
        x_max,
        &result, Policy())
        )
      {
        return result;
      }
      return  (x_min + x_max) / 2;
    }

    template <class RealType, class Policy>
    inline RealType skewness(const arcsine_distribution<RealType, Policy>& dist)
    {
      RealType result;
      RealType x_min = dist.x_min();
      RealType x_max = dist.x_max();

      if (false == arcsine_detail::check_dist(
        "boost::math::skewness(arcsine_distribution<%1%> const&, %1% )",
        x_min,
        x_max,
        &result, Policy())
        )
      {
        return result;
      }
      return 0;
    } // skewness

    template <class RealType, class Policy>
    inline RealType kurtosis_excess(const arcsine_distribution<RealType, Policy>& dist)
    {
      RealType result;
      RealType x_min = dist.x_min();
      RealType x_max = dist.x_max();

      if (false == arcsine_detail::check_dist(
        "boost::math::kurtosis_excess(arcsine_distribution<%1%> const&, %1% )",
        x_min,
        x_max,
        &result, Policy())
        )
      {
        return result;
      }
      result = -3;
      return  result / 2;
    } // kurtosis_excess

    template <class RealType, class Policy>
    inline RealType kurtosis(const arcsine_distribution<RealType, Policy>& dist)
    {
      RealType result;
      RealType x_min = dist.x_min();
      RealType x_max = dist.x_max();

      if (false == arcsine_detail::check_dist(
        "boost::math::kurtosis(arcsine_distribution<%1%> const&, %1% )",
        x_min,
        x_max,
        &result, Policy())
        )
      {
        return result;
      }

      return 3 + kurtosis_excess(dist);
    } // kurtosis

    template <class RealType, class Policy>
    inline RealType pdf(const arcsine_distribution<RealType, Policy>& dist, const RealType& xx)
    { // Probability Density/Mass Function arcsine.
      BOOST_FPU_EXCEPTION_GUARD
      BOOST_MATH_STD_USING // For ADL of std functions.

      static const char* function = "boost::math::pdf(arcsine_distribution<%1%> const&, %1%)";

      RealType lo = dist.x_min();
      RealType hi = dist.x_max();
      RealType x = xx;

      // Argument checks:
      RealType result = 0; 
      if (false == arcsine_detail::check_dist_and_x(
        function,
        lo, hi, x,
        &result, Policy()))
      {
        return result;
      }
      using boost::math::constants::pi;
      result = static_cast<RealType>(1) / (pi<RealType>() * sqrt((x - lo) * (hi - x)));
      return result;
    } // pdf

    template <class RealType, class Policy>
    inline RealType cdf(const arcsine_distribution<RealType, Policy>& dist, const RealType& x)
    { // Cumulative Distribution Function arcsine.
      BOOST_MATH_STD_USING // For ADL of std functions.

      static const char* function = "boost::math::cdf(arcsine_distribution<%1%> const&, %1%)";

      RealType x_min = dist.x_min();
      RealType x_max = dist.x_max();

      // Argument checks:
      RealType result = 0;
      if (false == arcsine_detail::check_dist_and_x(
        function,
        x_min, x_max, x,
        &result, Policy()))
      {
        return result;
      }
      // Special cases:
      if (x == x_min)
      {
        return 0;
      }
      else if (x == x_max)
      {
        return 1;
      }
      using boost::math::constants::pi;
      result = static_cast<RealType>(2) * asin(sqrt((x - x_min) / (x_max - x_min))) / pi<RealType>();
      return result;
    } // arcsine cdf

    template <class RealType, class Policy>
    inline RealType cdf(const complemented2_type<arcsine_distribution<RealType, Policy>, RealType>& c)
    { // Complemented Cumulative Distribution Function arcsine.
      BOOST_MATH_STD_USING // For ADL of std functions.
      static const char* function = "boost::math::cdf(arcsine_distribution<%1%> const&, %1%)";

      RealType x = c.param;
      arcsine_distribution<RealType, Policy> const& dist = c.dist;
      RealType x_min = dist.x_min();
      RealType x_max = dist.x_max();

      // Argument checks:
      RealType result = 0;
      if (false == arcsine_detail::check_dist_and_x(
        function,
        x_min, x_max, x,
        &result, Policy()))
      {
        return result;
      }
      if (x == x_min)
      {
        return 0;
      }
      else if (x == x_max)
      {
        return 1;
      }
      using boost::math::constants::pi;
      // Naive version x = 1 - x;
      // result = static_cast<RealType>(2) * asin(sqrt((x - x_min) / (x_max - x_min))) / pi<RealType>();
      // is less accurate, so use acos instead of asin for complement.
      result = static_cast<RealType>(2) * acos(sqrt((x - x_min) / (x_max - x_min))) / pi<RealType>();
      return result;
    } // arcsine ccdf

    template <class RealType, class Policy>
    inline RealType quantile(const arcsine_distribution<RealType, Policy>& dist, const RealType& p)
    { 
      // Quantile or Percent Point arcsine function or
      // Inverse Cumulative probability distribution function CDF.
      // Return x (0 <= x <= 1),
      // for a given probability p (0 <= p <= 1).
      // These functions take a probability as an argument
      // and return a value such that the probability that a random variable x
      // will be less than or equal to that value
      // is whatever probability you supplied as an argument.
      BOOST_MATH_STD_USING // For ADL of std functions.

      using boost::math::constants::half_pi;

      static const char* function = "boost::math::quantile(arcsine_distribution<%1%> const&, %1%)";

      RealType result = 0; // of argument checks:
      RealType x_min = dist.x_min();
      RealType x_max = dist.x_max();
      if (false == arcsine_detail::check_dist_and_prob(
        function,
        x_min, x_max, p,
        &result, Policy()))
      {
        return result;
      }
      // Special cases:
      if (p == 0)
      {
        return 0;
      }
      if (p == 1)
      {
        return 1;
      }

      RealType sin2hpip = sin(half_pi<RealType>() * p);
      RealType sin2hpip2 = sin2hpip * sin2hpip;
      result = -x_min * sin2hpip2 + x_min + x_max * sin2hpip2;

      return result;
    } // quantile

    template <class RealType, class Policy>
    inline RealType quantile(const complemented2_type<arcsine_distribution<RealType, Policy>, RealType>& c)
    { 
      // Complement Quantile or Percent Point arcsine function.
      // Return the number of expected x for a given
      // complement of the probability q.
      BOOST_MATH_STD_USING // For ADL of std functions.

      using boost::math::constants::half_pi;
      static const char* function = "boost::math::quantile(arcsine_distribution<%1%> const&, %1%)";

      // Error checks:
      RealType q = c.param;
      const arcsine_distribution<RealType, Policy>& dist = c.dist;
      RealType result = 0;
      RealType x_min = dist.x_min();
      RealType x_max = dist.x_max();
      if (false == arcsine_detail::check_dist_and_prob(
        function,
        x_min,
        x_max,
        q,
        &result, Policy()))
      {
        return result;
      }
      // Special cases:
      if (q == 1)
      {
        return 0;
      }
      if (q == 0)
      {
        return 1;
      }
      // Naive RealType p = 1 - q; result = sin(half_pi<RealType>() * p); loses accuracy, so use a cos alternative instead.
      //result = cos(half_pi<RealType>() * q); // for arcsine(0,1)
      //result = result * result;
      // For generalized arcsine:
      RealType cos2hpip = cos(half_pi<RealType>() * q);
      RealType cos2hpip2 = cos2hpip * cos2hpip;
      result = -x_min * cos2hpip2 + x_min + x_max * cos2hpip2;

      return result;
    } // Quantile Complement

  } // namespace math
} // namespace boost

// This include must be at the end, *after* the accessors
// for this distribution have been defined, in order to
// keep compilers that support two-phase lookup happy.
#include <boost/math/distributions/detail/derived_accessors.hpp>

#if defined (BOOST_MSVC)
# pragma warning(pop)
#endif

#endif // BOOST_MATH_DIST_ARCSINE_HPP
