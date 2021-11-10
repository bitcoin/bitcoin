// boost\math\distributions\geometric.hpp

// Copyright John Maddock 2010.
// Copyright Paul A. Bristow 2010.

// Use, modification and distribution are subject to the
// Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

// geometric distribution is a discrete probability distribution.
// It expresses the probability distribution of the number (k) of
// events, occurrences, failures or arrivals before the first success.
// supported on the set {0, 1, 2, 3...}

// Note that the set includes zero (unlike some definitions that start at one).

// The random variate k is the number of events, occurrences or arrivals.
// k argument may be integral, signed, or unsigned, or floating point.
// If necessary, it has already been promoted from an integral type.

// Note that the geometric distribution
// (like others including the binomial, geometric & Bernoulli)
// is strictly defined as a discrete function:
// only integral values of k are envisaged.
// However because the method of calculation uses a continuous gamma function,
// it is convenient to treat it as if a continuous function,
// and permit non-integral values of k.
// To enforce the strict mathematical model, users should use floor or ceil functions
// on k outside this function to ensure that k is integral.

// See http://en.wikipedia.org/wiki/geometric_distribution
// http://documents.wolfram.com/v5/Add-onsLinks/StandardPackages/Statistics/DiscreteDistributions.html
// http://mathworld.wolfram.com/GeometricDistribution.html

#ifndef BOOST_MATH_SPECIAL_GEOMETRIC_HPP
#define BOOST_MATH_SPECIAL_GEOMETRIC_HPP

#include <boost/math/distributions/fwd.hpp>
#include <boost/math/special_functions/beta.hpp> // for ibeta(a, b, x) == Ix(a, b).
#include <boost/math/distributions/complement.hpp> // complement.
#include <boost/math/distributions/detail/common_error_handling.hpp> // error checks domain_error & logic_error.
#include <boost/math/special_functions/fpclassify.hpp> // isnan.
#include <boost/math/tools/roots.hpp> // for root finding.
#include <boost/math/distributions/detail/inv_discrete_quantile.hpp>

#include <limits> // using std::numeric_limits;
#include <utility>

#if defined (BOOST_MSVC)
#  pragma warning(push)
// This believed not now necessary, so commented out.
//#  pragma warning(disable: 4702) // unreachable code.
// in domain_error_imp in error_handling.
#endif

namespace boost
{
  namespace math
  {
    namespace geometric_detail
    {
      // Common error checking routines for geometric distribution function:
      template <class RealType, class Policy>
      inline bool check_success_fraction(const char* function, const RealType& p, RealType* result, const Policy& pol)
      {
        if( !(boost::math::isfinite)(p) || (p < 0) || (p > 1) )
        {
          *result = policies::raise_domain_error<RealType>(
            function,
            "Success fraction argument is %1%, but must be >= 0 and <= 1 !", p, pol);
          return false;
        }
        return true;
      }

      template <class RealType, class Policy>
      inline bool check_dist(const char* function, const RealType& p, RealType* result, const Policy& pol)
      {
        return check_success_fraction(function, p, result, pol);
      }

      template <class RealType, class Policy>
      inline bool check_dist_and_k(const char* function,  const RealType& p, RealType k, RealType* result, const Policy& pol)
      {
        if(check_dist(function, p, result, pol) == false)
        {
          return false;
        }
        if( !(boost::math::isfinite)(k) || (k < 0) )
        { // Check k failures.
          *result = policies::raise_domain_error<RealType>(
            function,
            "Number of failures argument is %1%, but must be >= 0 !", k, pol);
          return false;
        }
        return true;
      } // Check_dist_and_k

      template <class RealType, class Policy>
      inline bool check_dist_and_prob(const char* function, RealType p, RealType prob, RealType* result, const Policy& pol)
      {
        if((check_dist(function, p, result, pol) && detail::check_probability(function, prob, result, pol)) == false)
        {
          return false;
        }
        return true;
      } // check_dist_and_prob
    } //  namespace geometric_detail

    template <class RealType = double, class Policy = policies::policy<> >
    class geometric_distribution
    {
    public:
      typedef RealType value_type;
      typedef Policy policy_type;

      geometric_distribution(RealType p) : m_p(p)
      { // Constructor stores success_fraction p.
        RealType result;
        geometric_detail::check_dist(
          "geometric_distribution<%1%>::geometric_distribution",
          m_p, // Check success_fraction 0 <= p <= 1.
          &result, Policy());
      } // geometric_distribution constructor.

      // Private data getter class member functions.
      RealType success_fraction() const
      { // Probability of success as fraction in range 0 to 1.
        return m_p;
      }
      RealType successes() const
      { // Total number of successes r = 1 (for compatibility with negative binomial?).
        return 1;
      }

      // Parameter estimation.
      // (These are copies of negative_binomial distribution with successes = 1).
      static RealType find_lower_bound_on_p(
        RealType trials,
        RealType alpha) // alpha 0.05 equivalent to 95% for one-sided test.
      {
        static const char* function = "boost::math::geometric<%1%>::find_lower_bound_on_p";
        RealType result = 0;  // of error checks.
        RealType successes = 1;
        RealType failures = trials - successes;
        if(false == detail::check_probability(function, alpha, &result, Policy())
          && geometric_detail::check_dist_and_k(
          function, RealType(0), failures, &result, Policy()))
        {
          return result;
        }
        // Use complement ibeta_inv function for lower bound.
        // This is adapted from the corresponding binomial formula
        // here: http://www.itl.nist.gov/div898/handbook/prc/section2/prc241.htm
        // This is a Clopper-Pearson interval, and may be overly conservative,
        // see also "A Simple Improved Inferential Method for Some
        // Discrete Distributions" Yong CAI and K. KRISHNAMOORTHY
        // http://www.ucs.louisiana.edu/~kxk4695/Discrete_new.pdf
        //
        return ibeta_inv(successes, failures + 1, alpha, static_cast<RealType*>(0), Policy());
      } // find_lower_bound_on_p

      static RealType find_upper_bound_on_p(
        RealType trials,
        RealType alpha) // alpha 0.05 equivalent to 95% for one-sided test.
      {
        static const char* function = "boost::math::geometric<%1%>::find_upper_bound_on_p";
        RealType result = 0;  // of error checks.
        RealType successes = 1;
        RealType failures = trials - successes;
        if(false == geometric_detail::check_dist_and_k(
          function, RealType(0), failures, &result, Policy())
          && detail::check_probability(function, alpha, &result, Policy()))
        {
          return result;
        }
        if(failures == 0)
        {
           return 1;
        }// Use complement ibetac_inv function for upper bound.
        // Note adjusted failures value: *not* failures+1 as usual.
        // This is adapted from the corresponding binomial formula
        // here: http://www.itl.nist.gov/div898/handbook/prc/section2/prc241.htm
        // This is a Clopper-Pearson interval, and may be overly conservative,
        // see also "A Simple Improved Inferential Method for Some
        // Discrete Distributions" Yong CAI and K. Krishnamoorthy
        // http://www.ucs.louisiana.edu/~kxk4695/Discrete_new.pdf
        //
        return ibetac_inv(successes, failures, alpha, static_cast<RealType*>(0), Policy());
      } // find_upper_bound_on_p

      // Estimate number of trials :
      // "How many trials do I need to be P% sure of seeing k or fewer failures?"

      static RealType find_minimum_number_of_trials(
        RealType k,     // number of failures (k >= 0).
        RealType p,     // success fraction 0 <= p <= 1.
        RealType alpha) // risk level threshold 0 <= alpha <= 1.
      {
        static const char* function = "boost::math::geometric<%1%>::find_minimum_number_of_trials";
        // Error checks:
        RealType result = 0;
        if(false == geometric_detail::check_dist_and_k(
          function, p, k, &result, Policy())
          && detail::check_probability(function, alpha, &result, Policy()))
        {
          return result;
        }
        result = ibeta_inva(k + 1, p, alpha, Policy());  // returns n - k
        return result + k;
      } // RealType find_number_of_failures

      static RealType find_maximum_number_of_trials(
        RealType k,     // number of failures (k >= 0).
        RealType p,     // success fraction 0 <= p <= 1.
        RealType alpha) // risk level threshold 0 <= alpha <= 1.
      {
        static const char* function = "boost::math::geometric<%1%>::find_maximum_number_of_trials";
        // Error checks:
        RealType result = 0;
        if(false == geometric_detail::check_dist_and_k(
          function, p, k, &result, Policy())
          &&  detail::check_probability(function, alpha, &result, Policy()))
        { 
          return result;
        }
        result = ibetac_inva(k + 1, p, alpha, Policy());  // returns n - k
        return result + k;
      } // RealType find_number_of_trials complemented

    private:
      //RealType m_r; // successes fixed at unity.
      RealType m_p; // success_fraction
    }; // template <class RealType, class Policy> class geometric_distribution

    typedef geometric_distribution<double> geometric; // Reserved name of type double.

    template <class RealType, class Policy>
    inline const std::pair<RealType, RealType> range(const geometric_distribution<RealType, Policy>& /* dist */)
    { // Range of permissible values for random variable k.
       using boost::math::tools::max_value;
       return std::pair<RealType, RealType>(static_cast<RealType>(0), max_value<RealType>()); // max_integer?
    }

    template <class RealType, class Policy>
    inline const std::pair<RealType, RealType> support(const geometric_distribution<RealType, Policy>& /* dist */)
    { // Range of supported values for random variable k.
       // This is range where cdf rises from 0 to 1, and outside it, the pdf is zero.
       using boost::math::tools::max_value;
       return std::pair<RealType, RealType>(static_cast<RealType>(0),  max_value<RealType>()); // max_integer?
    }

    template <class RealType, class Policy>
    inline RealType mean(const geometric_distribution<RealType, Policy>& dist)
    { // Mean of geometric distribution = (1-p)/p.
      return (1 - dist.success_fraction() ) / dist.success_fraction();
    } // mean

    // median implemented via quantile(half) in derived accessors.

    template <class RealType, class Policy>
    inline RealType mode(const geometric_distribution<RealType, Policy>&)
    { // Mode of geometric distribution = zero.
      BOOST_MATH_STD_USING // ADL of std functions.
      return 0;
    } // mode
    
    template <class RealType, class Policy>
    inline RealType variance(const geometric_distribution<RealType, Policy>& dist)
    { // Variance of Binomial distribution = (1-p) / p^2.
      return  (1 - dist.success_fraction())
        / (dist.success_fraction() * dist.success_fraction());
    } // variance

    template <class RealType, class Policy>
    inline RealType skewness(const geometric_distribution<RealType, Policy>& dist)
    { // skewness of geometric distribution = 2-p / (sqrt(r(1-p))
      BOOST_MATH_STD_USING // ADL of std functions.
      RealType p = dist.success_fraction();
      return (2 - p) / sqrt(1 - p);
    } // skewness

    template <class RealType, class Policy>
    inline RealType kurtosis(const geometric_distribution<RealType, Policy>& dist)
    { // kurtosis of geometric distribution
      // http://en.wikipedia.org/wiki/geometric is kurtosis_excess so add 3
      RealType p = dist.success_fraction();
      return 3 + (p*p - 6*p + 6) / (1 - p);
    } // kurtosis

     template <class RealType, class Policy>
    inline RealType kurtosis_excess(const geometric_distribution<RealType, Policy>& dist)
    { // kurtosis excess of geometric distribution
      // http://mathworld.wolfram.com/Kurtosis.html table of kurtosis_excess
      RealType p = dist.success_fraction();
      return (p*p - 6*p + 6) / (1 - p);
    } // kurtosis_excess

    // RealType standard_deviation(const geometric_distribution<RealType, Policy>& dist)
    // standard_deviation provided by derived accessors.
    // RealType hazard(const geometric_distribution<RealType, Policy>& dist)
    // hazard of geometric distribution provided by derived accessors.
    // RealType chf(const geometric_distribution<RealType, Policy>& dist)
    // chf of geometric distribution provided by derived accessors.

    template <class RealType, class Policy>
    inline RealType pdf(const geometric_distribution<RealType, Policy>& dist, const RealType& k)
    { // Probability Density/Mass Function.
      BOOST_FPU_EXCEPTION_GUARD
      BOOST_MATH_STD_USING  // For ADL of math functions.
      static const char* function = "boost::math::pdf(const geometric_distribution<%1%>&, %1%)";

      RealType p = dist.success_fraction();
      RealType result = 0;
      if(false == geometric_detail::check_dist_and_k(
        function,
        p,
        k,
        &result, Policy()))
      {
        return result;
      }
      if (k == 0)
      {
        return p; // success_fraction
      }
      RealType q = 1 - p;  // Inaccurate for small p?
      // So try to avoid inaccuracy for large or small p.
      // but has little effect > last significant bit.
      //cout << "p *  pow(q, k) " << result << endl; // seems best whatever p
      //cout << "exp(p * k * log1p(-p)) " << p * exp(k * log1p(-p)) << endl;
      //if (p < 0.5)
      //{
      //  result = p *  pow(q, k);
      //}
      //else
      //{
      //  result = p * exp(k * log1p(-p));
      //}
      result = p * pow(q, k);
      return result;
    } // geometric_pdf

    template <class RealType, class Policy>
    inline RealType cdf(const geometric_distribution<RealType, Policy>& dist, const RealType& k)
    { // Cumulative Distribution Function of geometric.
      static const char* function = "boost::math::cdf(const geometric_distribution<%1%>&, %1%)";

      // k argument may be integral, signed, or unsigned, or floating point.
      // If necessary, it has already been promoted from an integral type.
      RealType p = dist.success_fraction();
      // Error check:
      RealType result = 0;
      if(false == geometric_detail::check_dist_and_k(
        function,
        p,
        k,
        &result, Policy()))
      {
        return result;
      }
      if(k == 0)
      {
        return p; // success_fraction
      }
      //RealType q = 1 - p;  // Bad for small p
      //RealType probability = 1 - std::pow(q, k+1);

      RealType z = boost::math::log1p(-p, Policy()) * (k + 1);
      RealType probability = -boost::math::expm1(z, Policy());

      return probability;
    } // cdf Cumulative Distribution Function geometric.

      template <class RealType, class Policy>
      inline RealType cdf(const complemented2_type<geometric_distribution<RealType, Policy>, RealType>& c)
      { // Complemented Cumulative Distribution Function geometric.
      BOOST_MATH_STD_USING
      static const char* function = "boost::math::cdf(const geometric_distribution<%1%>&, %1%)";
      // k argument may be integral, signed, or unsigned, or floating point.
      // If necessary, it has already been promoted from an integral type.
      RealType const& k = c.param;
      geometric_distribution<RealType, Policy> const& dist = c.dist;
      RealType p = dist.success_fraction();
      // Error check:
      RealType result = 0;
      if(false == geometric_detail::check_dist_and_k(
        function,
        p,
        k,
        &result, Policy()))
      {
        return result;
      }
      RealType z = boost::math::log1p(-p, Policy()) * (k+1);
      RealType probability = exp(z);
      return probability;
    } // cdf Complemented Cumulative Distribution Function geometric.

    template <class RealType, class Policy>
    inline RealType quantile(const geometric_distribution<RealType, Policy>& dist, const RealType& x)
    { // Quantile, percentile/100 or Percent Point geometric function.
      // Return the number of expected failures k for a given probability p.

      // Inverse cumulative Distribution Function or Quantile (percentile / 100) of geometric Probability.
      // k argument may be integral, signed, or unsigned, or floating point.

      static const char* function = "boost::math::quantile(const geometric_distribution<%1%>&, %1%)";
      BOOST_MATH_STD_USING // ADL of std functions.

      RealType success_fraction = dist.success_fraction();
      // Check dist and x.
      RealType result = 0;
      if(false == geometric_detail::check_dist_and_prob
        (function, success_fraction, x, &result, Policy()))
      {
        return result;
      }

      // Special cases.
      if (x == 1)
      {  // Would need +infinity failures for total confidence.
        result = policies::raise_overflow_error<RealType>(
            function,
            "Probability argument is 1, which implies infinite failures !", Policy());
        return result;
       // usually means return +std::numeric_limits<RealType>::infinity();
       // unless #define BOOST_MATH_THROW_ON_OVERFLOW_ERROR
      }
      if (x == 0)
      { // No failures are expected if P = 0.
        return 0; // Total trials will be just dist.successes.
      }
      // if (P <= pow(dist.success_fraction(), 1))
      if (x <= success_fraction)
      { // p <= pdf(dist, 0) == cdf(dist, 0)
        return 0;
      }
      if (x == 1)
      {
        return 0;
      }
   
      // log(1-x) /log(1-success_fraction) -1; but use log1p in case success_fraction is small
      result = boost::math::log1p(-x, Policy()) / boost::math::log1p(-success_fraction, Policy()) - 1;
      // Subtract a few epsilons here too?
      // to make sure it doesn't slip over, so ceil would be one too many.
      return result;
    } // RealType quantile(const geometric_distribution dist, p)

    template <class RealType, class Policy>
    inline RealType quantile(const complemented2_type<geometric_distribution<RealType, Policy>, RealType>& c)
    {  // Quantile or Percent Point Binomial function.
       // Return the number of expected failures k for a given
       // complement of the probability Q = 1 - P.
       static const char* function = "boost::math::quantile(const geometric_distribution<%1%>&, %1%)";
       BOOST_MATH_STD_USING
       // Error checks:
       RealType x = c.param;
       const geometric_distribution<RealType, Policy>& dist = c.dist;
       RealType success_fraction = dist.success_fraction();
       RealType result = 0;
       if(false == geometric_detail::check_dist_and_prob(
          function,
          success_fraction,
          x,
          &result, Policy()))
       {
          return result;
       }

       // Special cases:
       if(x == 1)
       {  // There may actually be no answer to this question,
          // since the probability of zero failures may be non-zero,
          return 0; // but zero is the best we can do:
       }
       if (-x <= boost::math::powm1(dist.success_fraction(), dist.successes(), Policy()))
       {  // q <= cdf(complement(dist, 0)) == pdf(dist, 0)
          return 0; //
       }
       if(x == 0)
       {  // Probability 1 - Q  == 1 so infinite failures to achieve certainty.
          // Would need +infinity failures for total confidence.
          result = policies::raise_overflow_error<RealType>(
             function,
             "Probability argument complement is 0, which implies infinite failures !", Policy());
          return result;
          // usually means return +std::numeric_limits<RealType>::infinity();
          // unless #define BOOST_MATH_THROW_ON_OVERFLOW_ERROR
       }
       // log(x) /log(1-success_fraction) -1; but use log1p in case success_fraction is small
       result = log(x) / boost::math::log1p(-success_fraction, Policy()) - 1;
      return result;

    } // quantile complement

 } // namespace math
} // namespace boost

// This include must be at the end, *after* the accessors
// for this distribution have been defined, in order to
// keep compilers that support two-phase lookup happy.
#include <boost/math/distributions/detail/derived_accessors.hpp>

#if defined (BOOST_MSVC)
# pragma warning(pop)
#endif

#endif // BOOST_MATH_SPECIAL_GEOMETRIC_HPP
