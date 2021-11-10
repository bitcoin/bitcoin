// boost\math\distributions\poisson.hpp

// Copyright John Maddock 2006.
// Copyright Paul A. Bristow 2007.

// Use, modification and distribution are subject to the
// Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

// Poisson distribution is a discrete probability distribution.
// It expresses the probability of a number (k) of
// events, occurrences, failures or arrivals occurring in a fixed time,
// assuming these events occur with a known average or mean rate (lambda)
// and are independent of the time since the last event.
// The distribution was discovered by Simeon-Denis Poisson (1781-1840).

// Parameter lambda is the mean number of events in the given time interval.
// The random variate k is the number of events, occurrences or arrivals.
// k argument may be integral, signed, or unsigned, or floating point.
// If necessary, it has already been promoted from an integral type.

// Note that the Poisson distribution
// (like others including the binomial, negative binomial & Bernoulli)
// is strictly defined as a discrete function:
// only integral values of k are envisaged.
// However because the method of calculation uses a continuous gamma function,
// it is convenient to treat it as if a continuous function,
// and permit non-integral values of k.
// To enforce the strict mathematical model, users should use floor or ceil functions
// on k outside this function to ensure that k is integral.

// See http://en.wikipedia.org/wiki/Poisson_distribution
// http://documents.wolfram.com/v5/Add-onsLinks/StandardPackages/Statistics/DiscreteDistributions.html

#ifndef BOOST_MATH_SPECIAL_POISSON_HPP
#define BOOST_MATH_SPECIAL_POISSON_HPP

#include <boost/math/distributions/fwd.hpp>
#include <boost/math/special_functions/gamma.hpp> // for incomplete gamma. gamma_q
#include <boost/math/special_functions/trunc.hpp> // for incomplete gamma. gamma_q
#include <boost/math/distributions/complement.hpp> // complements
#include <boost/math/distributions/detail/common_error_handling.hpp> // error checks
#include <boost/math/special_functions/fpclassify.hpp> // isnan.
#include <boost/math/special_functions/factorials.hpp> // factorials.
#include <boost/math/tools/roots.hpp> // for root finding.
#include <boost/math/distributions/detail/inv_discrete_quantile.hpp>

#include <utility>

namespace boost
{
  namespace math
  {
    namespace poisson_detail
    {
      // Common error checking routines for Poisson distribution functions.
      // These are convoluted, & apparently redundant, to try to ensure that
      // checks are always performed, even if exceptions are not enabled.

      template <class RealType, class Policy>
      inline bool check_mean(const char* function, const RealType& mean, RealType* result, const Policy& pol)
      {
        if(!(boost::math::isfinite)(mean) || (mean < 0))
        {
          *result = policies::raise_domain_error<RealType>(
            function,
            "Mean argument is %1%, but must be >= 0 !", mean, pol);
          return false;
        }
        return true;
      } // bool check_mean

      template <class RealType, class Policy>
      inline bool check_mean_NZ(const char* function, const RealType& mean, RealType* result, const Policy& pol)
      { // mean == 0 is considered an error.
        if( !(boost::math::isfinite)(mean) || (mean <= 0))
        {
          *result = policies::raise_domain_error<RealType>(
            function,
            "Mean argument is %1%, but must be > 0 !", mean, pol);
          return false;
        }
        return true;
      } // bool check_mean_NZ

      template <class RealType, class Policy>
      inline bool check_dist(const char* function, const RealType& mean, RealType* result, const Policy& pol)
      { // Only one check, so this is redundant really but should be optimized away.
        return check_mean_NZ(function, mean, result, pol);
      } // bool check_dist

      template <class RealType, class Policy>
      inline bool check_k(const char* function, const RealType& k, RealType* result, const Policy& pol)
      {
        if((k < 0) || !(boost::math::isfinite)(k))
        {
          *result = policies::raise_domain_error<RealType>(
            function,
            "Number of events k argument is %1%, but must be >= 0 !", k, pol);
          return false;
        }
        return true;
      } // bool check_k

      template <class RealType, class Policy>
      inline bool check_dist_and_k(const char* function, RealType mean, RealType k, RealType* result, const Policy& pol)
      {
        if((check_dist(function, mean, result, pol) == false) ||
          (check_k(function, k, result, pol) == false))
        {
          return false;
        }
        return true;
      } // bool check_dist_and_k

      template <class RealType, class Policy>
      inline bool check_prob(const char* function, const RealType& p, RealType* result, const Policy& pol)
      { // Check 0 <= p <= 1
        if(!(boost::math::isfinite)(p) || (p < 0) || (p > 1))
        {
          *result = policies::raise_domain_error<RealType>(
            function,
            "Probability argument is %1%, but must be >= 0 and <= 1 !", p, pol);
          return false;
        }
        return true;
      } // bool check_prob

      template <class RealType, class Policy>
      inline bool check_dist_and_prob(const char* function, RealType mean,  RealType p, RealType* result, const Policy& pol)
      {
        if((check_dist(function, mean, result, pol) == false) ||
          (check_prob(function, p, result, pol) == false))
        {
          return false;
        }
        return true;
      } // bool check_dist_and_prob

    } // namespace poisson_detail

    template <class RealType = double, class Policy = policies::policy<> >
    class poisson_distribution
    {
    public:
      typedef RealType value_type;
      typedef Policy policy_type;

      poisson_distribution(RealType l_mean = 1) : m_l(l_mean) // mean (lambda).
      { // Expected mean number of events that occur during the given interval.
        RealType r;
        poisson_detail::check_dist(
           "boost::math::poisson_distribution<%1%>::poisson_distribution",
          m_l,
          &r, Policy());
      } // poisson_distribution constructor.

      RealType mean() const
      { // Private data getter function.
        return m_l;
      }
    private:
      // Data member, initialized by constructor.
      RealType m_l; // mean number of occurrences.
    }; // template <class RealType, class Policy> class poisson_distribution

    typedef poisson_distribution<double> poisson; // Reserved name of type double.

    // Non-member functions to give properties of the distribution.

    template <class RealType, class Policy>
    inline const std::pair<RealType, RealType> range(const poisson_distribution<RealType, Policy>& /* dist */)
    { // Range of permissible values for random variable k.
       using boost::math::tools::max_value;
       return std::pair<RealType, RealType>(static_cast<RealType>(0), max_value<RealType>()); // Max integer?
    }

    template <class RealType, class Policy>
    inline const std::pair<RealType, RealType> support(const poisson_distribution<RealType, Policy>& /* dist */)
    { // Range of supported values for random variable k.
       // This is range where cdf rises from 0 to 1, and outside it, the pdf is zero.
       using boost::math::tools::max_value;
       return std::pair<RealType, RealType>(static_cast<RealType>(0),  max_value<RealType>());
    }

    template <class RealType, class Policy>
    inline RealType mean(const poisson_distribution<RealType, Policy>& dist)
    { // Mean of poisson distribution = lambda.
      return dist.mean();
    } // mean

    template <class RealType, class Policy>
    inline RealType mode(const poisson_distribution<RealType, Policy>& dist)
    { // mode.
      BOOST_MATH_STD_USING // ADL of std functions.
      return floor(dist.mean());
    }

    //template <class RealType, class Policy>
    //inline RealType median(const poisson_distribution<RealType, Policy>& dist)
    //{ // median = approximately lambda + 1/3 - 0.2/lambda
    //  RealType l = dist.mean();
    //  return dist.mean() + static_cast<RealType>(0.3333333333333333333333333333333333333333333333)
    //   - static_cast<RealType>(0.2) / l;
    //} // BUT this formula appears to be out-by-one compared to quantile(half)
    // Query posted on Wikipedia.
    // Now implemented via quantile(half) in derived accessors.

    template <class RealType, class Policy>
    inline RealType variance(const poisson_distribution<RealType, Policy>& dist)
    { // variance.
      return dist.mean();
    }

    // RealType standard_deviation(const poisson_distribution<RealType, Policy>& dist)
    // standard_deviation provided by derived accessors.

    template <class RealType, class Policy>
    inline RealType skewness(const poisson_distribution<RealType, Policy>& dist)
    { // skewness = sqrt(l).
      BOOST_MATH_STD_USING // ADL of std functions.
      return 1 / sqrt(dist.mean());
    }

    template <class RealType, class Policy>
    inline RealType kurtosis_excess(const poisson_distribution<RealType, Policy>& dist)
    { // skewness = sqrt(l).
      return 1 / dist.mean(); // kurtosis_excess 1/mean from Wiki & MathWorld eq 31.
      // http://mathworld.wolfram.com/Kurtosis.html explains that the kurtosis excess
      // is more convenient because the kurtosis excess of a normal distribution is zero
      // whereas the true kurtosis is 3.
    } // RealType kurtosis_excess

    template <class RealType, class Policy>
    inline RealType kurtosis(const poisson_distribution<RealType, Policy>& dist)
    { // kurtosis is 4th moment about the mean = u4 / sd ^ 4
      // http://en.wikipedia.org/wiki/Kurtosis
      // kurtosis can range from -2 (flat top) to +infinity (sharp peak & heavy tails).
      // http://www.itl.nist.gov/div898/handbook/eda/section3/eda35b.htm
      return 3 + 1 / dist.mean(); // NIST.
      // http://mathworld.wolfram.com/Kurtosis.html explains that the kurtosis excess
      // is more convenient because the kurtosis excess of a normal distribution is zero
      // whereas the true kurtosis is 3.
    } // RealType kurtosis

    template <class RealType, class Policy>
    RealType pdf(const poisson_distribution<RealType, Policy>& dist, const RealType& k)
    { // Probability Density/Mass Function.
      // Probability that there are EXACTLY k occurrences (or arrivals).
      BOOST_FPU_EXCEPTION_GUARD

      BOOST_MATH_STD_USING // for ADL of std functions.

      RealType mean = dist.mean();
      // Error check:
      RealType result = 0;
      if(false == poisson_detail::check_dist_and_k(
        "boost::math::pdf(const poisson_distribution<%1%>&, %1%)",
        mean,
        k,
        &result, Policy()))
      {
        return result;
      }

      // Special case of mean zero, regardless of the number of events k.
      if (mean == 0)
      { // Probability for any k is zero.
        return 0;
      }
      if (k == 0)
      { // mean ^ k = 1, and k! = 1, so can simplify.
        return exp(-mean);
      }
      return boost::math::gamma_p_derivative(k+1, mean, Policy());
    } // pdf

    template <class RealType, class Policy>
    RealType cdf(const poisson_distribution<RealType, Policy>& dist, const RealType& k)
    { // Cumulative Distribution Function Poisson.
      // The random variate k is the number of occurrences(or arrivals)
      // k argument may be integral, signed, or unsigned, or floating point.
      // If necessary, it has already been promoted from an integral type.
      // Returns the sum of the terms 0 through k of the Poisson Probability Density or Mass (pdf).

      // But note that the Poisson distribution
      // (like others including the binomial, negative binomial & Bernoulli)
      // is strictly defined as a discrete function: only integral values of k are envisaged.
      // However because of the method of calculation using a continuous gamma function,
      // it is convenient to treat it as if it is a continuous function
      // and permit non-integral values of k.
      // To enforce the strict mathematical model, users should use floor or ceil functions
      // outside this function to ensure that k is integral.

      // The terms are not summed directly (at least for larger k)
      // instead the incomplete gamma integral is employed,

      BOOST_MATH_STD_USING // for ADL of std function exp.

      RealType mean = dist.mean();
      // Error checks:
      RealType result = 0;
      if(false == poisson_detail::check_dist_and_k(
        "boost::math::cdf(const poisson_distribution<%1%>&, %1%)",
        mean,
        k,
        &result, Policy()))
      {
        return result;
      }
      // Special cases:
      if (mean == 0)
      { // Probability for any k is zero.
        return 0;
      }
      if (k == 0)
      { // return pdf(dist, static_cast<RealType>(0));
        // but mean (and k) have already been checked,
        // so this avoids unnecessary repeated checks.
       return exp(-mean);
      }
      // For small integral k could use a finite sum -
      // it's cheaper than the gamma function.
      // BUT this is now done efficiently by gamma_q function.
      // Calculate poisson cdf using the gamma_q function.
      return gamma_q(k+1, mean, Policy());
    } // binomial cdf

    template <class RealType, class Policy>
    RealType cdf(const complemented2_type<poisson_distribution<RealType, Policy>, RealType>& c)
    { // Complemented Cumulative Distribution Function Poisson
      // The random variate k is the number of events, occurrences or arrivals.
      // k argument may be integral, signed, or unsigned, or floating point.
      // If necessary, it has already been promoted from an integral type.
      // But note that the Poisson distribution
      // (like others including the binomial, negative binomial & Bernoulli)
      // is strictly defined as a discrete function: only integral values of k are envisaged.
      // However because of the method of calculation using a continuous gamma function,
      // it is convenient to treat it as is it is a continuous function
      // and permit non-integral values of k.
      // To enforce the strict mathematical model, users should use floor or ceil functions
      // outside this function to ensure that k is integral.

      // Returns the sum of the terms k+1 through inf of the Poisson Probability Density/Mass (pdf).
      // The terms are not summed directly (at least for larger k)
      // instead the incomplete gamma integral is employed,

      RealType const& k = c.param;
      poisson_distribution<RealType, Policy> const& dist = c.dist;

      RealType mean = dist.mean();

      // Error checks:
      RealType result = 0;
      if(false == poisson_detail::check_dist_and_k(
        "boost::math::cdf(const poisson_distribution<%1%>&, %1%)",
        mean,
        k,
        &result, Policy()))
      {
        return result;
      }
      // Special case of mean, regardless of the number of events k.
      if (mean == 0)
      { // Probability for any k is unity, complement of zero.
        return 1;
      }
      if (k == 0)
      { // Avoid repeated checks on k and mean in gamma_p.
         return -boost::math::expm1(-mean, Policy());
      }
      // Unlike un-complemented cdf (sum from 0 to k),
      // can't use finite sum from k+1 to infinity for small integral k,
      // anyway it is now done efficiently by gamma_p.
      return gamma_p(k + 1, mean, Policy()); // Calculate Poisson cdf using the gamma_p function.
      // CCDF = gamma_p(k+1, lambda)
    } // poisson ccdf

    template <class RealType, class Policy>
    inline RealType quantile(const poisson_distribution<RealType, Policy>& dist, const RealType& p)
    { // Quantile (or Percent Point) Poisson function.
      // Return the number of expected events k for a given probability p.
      static const char* function = "boost::math::quantile(const poisson_distribution<%1%>&, %1%)";
      RealType result = 0; // of Argument checks:
      if(false == poisson_detail::check_prob(
        function,
        p,
        &result, Policy()))
      {
        return result;
      }
      // Special case:
      if (dist.mean() == 0)
      { // if mean = 0 then p = 0, so k can be anything?
         if (false == poisson_detail::check_mean_NZ(
         function,
         dist.mean(),
         &result, Policy()))
        {
          return result;
        }
      }
      if(p == 0)
      {
         return 0; // Exact result regardless of discrete-quantile Policy
      }
      if(p == 1)
      {
         return policies::raise_overflow_error<RealType>(function, 0, Policy());
      }
      typedef typename Policy::discrete_quantile_type discrete_type;
      std::uintmax_t max_iter = policies::get_max_root_iterations<Policy>();
      RealType guess, factor = 8;
      RealType z = dist.mean();
      if(z < 1)
         guess = z;
      else
         guess = boost::math::detail::inverse_poisson_cornish_fisher(z, p, RealType(1-p), Policy());
      if(z > 5)
      {
         if(z > 1000)
            factor = 1.01f;
         else if(z > 50)
            factor = 1.1f;
         else if(guess > 10)
            factor = 1.25f;
         else
            factor = 2;
         if(guess < 1.1)
            factor = 8;
      }

      return detail::inverse_discrete_quantile(
         dist,
         p,
         false,
         guess,
         factor,
         RealType(1),
         discrete_type(),
         max_iter);
   } // quantile

    template <class RealType, class Policy>
    inline RealType quantile(const complemented2_type<poisson_distribution<RealType, Policy>, RealType>& c)
    { // Quantile (or Percent Point) of Poisson function.
      // Return the number of expected events k for a given
      // complement of the probability q.
      //
      // Error checks:
      static const char* function = "boost::math::quantile(complement(const poisson_distribution<%1%>&, %1%))";
      RealType q = c.param;
      const poisson_distribution<RealType, Policy>& dist = c.dist;
      RealType result = 0;  // of argument checks.
      if(false == poisson_detail::check_prob(
        function,
        q,
        &result, Policy()))
      {
        return result;
      }
      // Special case:
      if (dist.mean() == 0)
      { // if mean = 0 then p = 0, so k can be anything?
         if (false == poisson_detail::check_mean_NZ(
         function,
         dist.mean(),
         &result, Policy()))
        {
          return result;
        }
      }
      if(q == 0)
      {
         return policies::raise_overflow_error<RealType>(function, 0, Policy());
      }
      if(q == 1)
      {
         return 0;  // Exact result regardless of discrete-quantile Policy
      }
      typedef typename Policy::discrete_quantile_type discrete_type;
      std::uintmax_t max_iter = policies::get_max_root_iterations<Policy>();
      RealType guess, factor = 8;
      RealType z = dist.mean();
      if(z < 1)
         guess = z;
      else
         guess = boost::math::detail::inverse_poisson_cornish_fisher(z, RealType(1-q), q, Policy());
      if(z > 5)
      {
         if(z > 1000)
            factor = 1.01f;
         else if(z > 50)
            factor = 1.1f;
         else if(guess > 10)
            factor = 1.25f;
         else
            factor = 2;
         if(guess < 1.1)
            factor = 8;
      }

      return detail::inverse_discrete_quantile(
         dist,
         q,
         true,
         guess,
         factor,
         RealType(1),
         discrete_type(),
         max_iter);
   } // quantile complement.

  } // namespace math
} // namespace boost

// This include must be at the end, *after* the accessors
// for this distribution have been defined, in order to
// keep compilers that support two-phase lookup happy.
#include <boost/math/distributions/detail/derived_accessors.hpp>
#include <boost/math/distributions/detail/inv_discrete_quantile.hpp>

#endif // BOOST_MATH_SPECIAL_POISSON_HPP



