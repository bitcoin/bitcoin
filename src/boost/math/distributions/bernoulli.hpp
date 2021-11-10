// boost\math\distributions\bernoulli.hpp

// Copyright John Maddock 2006.
// Copyright Paul A. Bristow 2007.

// Use, modification and distribution are subject to the
// Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

// http://en.wikipedia.org/wiki/bernoulli_distribution
// http://mathworld.wolfram.com/BernoulliDistribution.html

// bernoulli distribution is the discrete probability distribution of
// the number (k) of successes, in a single Bernoulli trials.
// It is a version of the binomial distribution when n = 1.

// But note that the bernoulli distribution
// (like others including the poisson, binomial & negative binomial)
// is strictly defined as a discrete function: only integral values of k are envisaged.
// However because of the method of calculation using a continuous gamma function,
// it is convenient to treat it as if a continuous function,
// and permit non-integral values of k.
// To enforce the strict mathematical model, users should use floor or ceil functions
// on k outside this function to ensure that k is integral.

#ifndef BOOST_MATH_SPECIAL_BERNOULLI_HPP
#define BOOST_MATH_SPECIAL_BERNOULLI_HPP

#include <boost/math/distributions/fwd.hpp>
#include <boost/math/tools/config.hpp>
#include <boost/math/distributions/complement.hpp> // complements
#include <boost/math/distributions/detail/common_error_handling.hpp> // error checks
#include <boost/math/special_functions/fpclassify.hpp> // isnan.

#include <utility>

namespace boost
{
  namespace math
  {
    namespace bernoulli_detail
    {
      // Common error checking routines for bernoulli distribution functions:
      template <class RealType, class Policy>
      inline bool check_success_fraction(const char* function, const RealType& p, RealType* result, const Policy& /* pol */)
      {
        if(!(boost::math::isfinite)(p) || (p < 0) || (p > 1))
        {
          *result = policies::raise_domain_error<RealType>(
            function,
            "Success fraction argument is %1%, but must be >= 0 and <= 1 !", p, Policy());
          return false;
        }
        return true;
      }
      template <class RealType, class Policy>
      inline bool check_dist(const char* function, const RealType& p, RealType* result, const Policy& /* pol */, const std::true_type&)
      {
        return check_success_fraction(function, p, result, Policy());
      }
      template <class RealType, class Policy>
      inline bool check_dist(const char* , const RealType& , RealType* , const Policy& /* pol */, const std::false_type&)
      {
         return true;
      }
      template <class RealType, class Policy>
      inline bool check_dist(const char* function, const RealType& p, RealType* result, const Policy& /* pol */)
      {
         return check_dist(function, p, result, Policy(), typename policies::constructor_error_check<Policy>::type());
      }

      template <class RealType, class Policy>
      inline bool check_dist_and_k(const char* function, const RealType& p, RealType k, RealType* result, const Policy& pol)
      {
        if(check_dist(function, p, result, Policy(), typename policies::method_error_check<Policy>::type()) == false)
        {
          return false;
        }
        if(!(boost::math::isfinite)(k) || !((k == 0) || (k == 1)))
        {
          *result = policies::raise_domain_error<RealType>(
            function,
            "Number of successes argument is %1%, but must be 0 or 1 !", k, pol);
          return false;
        }
       return true;
      }
      template <class RealType, class Policy>
      inline bool check_dist_and_prob(const char* function, RealType p, RealType prob, RealType* result, const Policy& /* pol */)
      {
        if((check_dist(function, p, result, Policy(), typename policies::method_error_check<Policy>::type()) && detail::check_probability(function, prob, result, Policy())) == false)
        {
          return false;
        }
        return true;
      }
    } // namespace bernoulli_detail


    template <class RealType = double, class Policy = policies::policy<> >
    class bernoulli_distribution
    {
    public:
      typedef RealType value_type;
      typedef Policy policy_type;

      bernoulli_distribution(RealType p = 0.5) : m_p(p)
      { // Default probability = half suits 'fair' coin tossing
        // where probability of heads == probability of tails.
        RealType result; // of checks.
        bernoulli_detail::check_dist(
           "boost::math::bernoulli_distribution<%1%>::bernoulli_distribution",
          m_p,
          &result, Policy());
      } // bernoulli_distribution constructor.

      RealType success_fraction() const
      { // Probability.
        return m_p;
      }

    private:
      RealType m_p; // success_fraction
    }; // template <class RealType> class bernoulli_distribution

    typedef bernoulli_distribution<double> bernoulli;

    template <class RealType, class Policy>
    inline const std::pair<RealType, RealType> range(const bernoulli_distribution<RealType, Policy>& /* dist */)
    { // Range of permissible values for random variable k = {0, 1}.
      using boost::math::tools::max_value;
      return std::pair<RealType, RealType>(static_cast<RealType>(0), static_cast<RealType>(1));
    }

    template <class RealType, class Policy>
    inline const std::pair<RealType, RealType> support(const bernoulli_distribution<RealType, Policy>& /* dist */)
    { // Range of supported values for random variable k = {0, 1}.
      // This is range where cdf rises from 0 to 1, and outside it, the pdf is zero.
      return std::pair<RealType, RealType>(static_cast<RealType>(0), static_cast<RealType>(1));
    }

    template <class RealType, class Policy>
    inline RealType mean(const bernoulli_distribution<RealType, Policy>& dist)
    { // Mean of bernoulli distribution = p (n = 1).
      return dist.success_fraction();
    } // mean

    // Rely on dereived_accessors quantile(half)
    //template <class RealType>
    //inline RealType median(const bernoulli_distribution<RealType, Policy>& dist)
    //{ // Median of bernoulli distribution is not defined.
    //  return tools::domain_error<RealType>(BOOST_CURRENT_FUNCTION, "Median is not implemented, result is %1%!", std::numeric_limits<RealType>::quiet_NaN());
    //} // median

    template <class RealType, class Policy>
    inline RealType variance(const bernoulli_distribution<RealType, Policy>& dist)
    { // Variance of bernoulli distribution =p * q.
      return  dist.success_fraction() * (1 - dist.success_fraction());
    } // variance

    template <class RealType, class Policy>
    RealType pdf(const bernoulli_distribution<RealType, Policy>& dist, const RealType& k)
    { // Probability Density/Mass Function.
      BOOST_FPU_EXCEPTION_GUARD
      // Error check:
      RealType result = 0; // of checks.
      if(false == bernoulli_detail::check_dist_and_k(
        "boost::math::pdf(bernoulli_distribution<%1%>, %1%)",
        dist.success_fraction(), // 0 to 1
        k, // 0 or 1
        &result, Policy()))
      {
        return result;
      }
      // Assume k is integral.
      if (k == 0)
      {
        return 1 - dist.success_fraction(); // 1 - p
      }
      else  // k == 1
      {
        return dist.success_fraction(); // p
      }
    } // pdf

    template <class RealType, class Policy>
    inline RealType cdf(const bernoulli_distribution<RealType, Policy>& dist, const RealType& k)
    { // Cumulative Distribution Function Bernoulli.
      RealType p = dist.success_fraction();
      // Error check:
      RealType result = 0;
      if(false == bernoulli_detail::check_dist_and_k(
        "boost::math::cdf(bernoulli_distribution<%1%>, %1%)",
        p,
        k,
        &result, Policy()))
      {
        return result;
      }
      if (k == 0)
      {
        return 1 - p;
      }
      else
      { // k == 1
        return 1;
      }
    } // bernoulli cdf

    template <class RealType, class Policy>
    inline RealType cdf(const complemented2_type<bernoulli_distribution<RealType, Policy>, RealType>& c)
    { // Complemented Cumulative Distribution Function bernoulli.
      RealType const& k = c.param;
      bernoulli_distribution<RealType, Policy> const& dist = c.dist;
      RealType p = dist.success_fraction();
      // Error checks:
      RealType result = 0;
      if(false == bernoulli_detail::check_dist_and_k(
        "boost::math::cdf(bernoulli_distribution<%1%>, %1%)",
        p,
        k,
        &result, Policy()))
      {
        return result;
      }
      if (k == 0)
      {
        return p;
      }
      else
      { // k == 1
        return 0;
      }
    } // bernoulli cdf complement

    template <class RealType, class Policy>
    inline RealType quantile(const bernoulli_distribution<RealType, Policy>& dist, const RealType& p)
    { // Quantile or Percent Point Bernoulli function.
      // Return the number of expected successes k either 0 or 1.
      // for a given probability p.

      RealType result = 0; // of error checks:
      if(false == bernoulli_detail::check_dist_and_prob(
        "boost::math::quantile(bernoulli_distribution<%1%>, %1%)",
        dist.success_fraction(),
        p,
        &result, Policy()))
      {
        return result;
      }
      if (p <= (1 - dist.success_fraction()))
      { // p <= pdf(dist, 0) == cdf(dist, 0)
        return 0;
      }
      else
      {
        return 1;
      }
    } // quantile

    template <class RealType, class Policy>
    inline RealType quantile(const complemented2_type<bernoulli_distribution<RealType, Policy>, RealType>& c)
    { // Quantile or Percent Point bernoulli function.
      // Return the number of expected successes k for a given
      // complement of the probability q.
      //
      // Error checks:
      RealType q = c.param;
      const bernoulli_distribution<RealType, Policy>& dist = c.dist;
      RealType result = 0;
      if(false == bernoulli_detail::check_dist_and_prob(
        "boost::math::quantile(bernoulli_distribution<%1%>, %1%)",
        dist.success_fraction(),
        q,
        &result, Policy()))
      {
        return result;
      }

      if (q <= 1 - dist.success_fraction())
      { // // q <= cdf(complement(dist, 0)) == pdf(dist, 0)
        return 1;
      }
      else
      {
        return 0;
      }
    } // quantile complemented.

    template <class RealType, class Policy>
    inline RealType mode(const bernoulli_distribution<RealType, Policy>& dist)
    {
      return static_cast<RealType>((dist.success_fraction() <= 0.5) ? 0 : 1); // p = 0.5 can be 0 or 1
    }

    template <class RealType, class Policy>
    inline RealType skewness(const bernoulli_distribution<RealType, Policy>& dist)
    {
      BOOST_MATH_STD_USING; // Aid ADL for sqrt.
      RealType p = dist.success_fraction();
      return (1 - 2 * p) / sqrt(p * (1 - p));
    }

    template <class RealType, class Policy>
    inline RealType kurtosis_excess(const bernoulli_distribution<RealType, Policy>& dist)
    {
      RealType p = dist.success_fraction();
      // Note Wolfram says this is kurtosis in text, but gamma2 is the kurtosis excess,
      // and Wikipedia also says this is the kurtosis excess formula.
      // return (6 * p * p - 6 * p + 1) / (p * (1 - p));
      // But Wolfram kurtosis article gives this simpler formula for kurtosis excess:
      return 1 / (1 - p) + 1/p -6;
    }

    template <class RealType, class Policy>
    inline RealType kurtosis(const bernoulli_distribution<RealType, Policy>& dist)
    {
      RealType p = dist.success_fraction();
      return 1 / (1 - p) + 1/p -6 + 3;
      // Simpler than:
      // return (6 * p * p - 6 * p + 1) / (p * (1 - p)) + 3;
    }

  } // namespace math
} // namespace boost

// This include must be at the end, *after* the accessors
// for this distribution have been defined, in order to
// keep compilers that support two-phase lookup happy.
#include <boost/math/distributions/detail/derived_accessors.hpp>

#endif // BOOST_MATH_SPECIAL_BERNOULLI_HPP



