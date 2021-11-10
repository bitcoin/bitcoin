// boost\math\special_functions\negative_binomial.hpp

// Copyright Paul A. Bristow 2007.
// Copyright John Maddock 2007.

// Use, modification and distribution are subject to the
// Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

// http://en.wikipedia.org/wiki/negative_binomial_distribution
// http://mathworld.wolfram.com/NegativeBinomialDistribution.html
// http://documents.wolfram.com/teachersedition/Teacher/Statistics/DiscreteDistributions.html

// The negative binomial distribution NegativeBinomialDistribution[n, p]
// is the distribution of the number (k) of failures that occur in a sequence of trials before
// r successes have occurred, where the probability of success in each trial is p.

// In a sequence of Bernoulli trials or events
// (independent, yes or no, succeed or fail) with success_fraction probability p,
// negative_binomial is the probability that k or fewer failures
// precede the r th trial's success.
// random variable k is the number of failures (NOT the probability).

// Negative_binomial distribution is a discrete probability distribution.
// But note that the negative binomial distribution
// (like others including the binomial, Poisson & Bernoulli)
// is strictly defined as a discrete function: only integral values of k are envisaged.
// However because of the method of calculation using a continuous gamma function,
// it is convenient to treat it as if a continuous function,
// and permit non-integral values of k.

// However, by default the policy is to use discrete_quantile_policy.

// To enforce the strict mathematical model, users should use conversion
// on k outside this function to ensure that k is integral.

// MATHCAD cumulative negative binomial pnbinom(k, n, p)

// Implementation note: much greater speed, and perhaps greater accuracy,
// might be achieved for extreme values by using a normal approximation.
// This is NOT been tested or implemented.

#ifndef BOOST_MATH_SPECIAL_NEGATIVE_BINOMIAL_HPP
#define BOOST_MATH_SPECIAL_NEGATIVE_BINOMIAL_HPP

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
    namespace negative_binomial_detail
    {
      // Common error checking routines for negative binomial distribution functions:
      template <class RealType, class Policy>
      inline bool check_successes(const char* function, const RealType& r, RealType* result, const Policy& pol)
      {
        if( !(boost::math::isfinite)(r) || (r <= 0) )
        {
          *result = policies::raise_domain_error<RealType>(
            function,
            "Number of successes argument is %1%, but must be > 0 !", r, pol);
          return false;
        }
        return true;
      }
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
      inline bool check_dist(const char* function, const RealType& r, const RealType& p, RealType* result, const Policy& pol)
      {
        return check_success_fraction(function, p, result, pol)
          && check_successes(function, r, result, pol);
      }
      template <class RealType, class Policy>
      inline bool check_dist_and_k(const char* function, const RealType& r, const RealType& p, RealType k, RealType* result, const Policy& pol)
      {
        if(check_dist(function, r, p, result, pol) == false)
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
      inline bool check_dist_and_prob(const char* function, const RealType& r, RealType p, RealType prob, RealType* result, const Policy& pol)
      {
        if((check_dist(function, r, p, result, pol) && detail::check_probability(function, prob, result, pol)) == false)
        {
          return false;
        }
        return true;
      } // check_dist_and_prob
    } //  namespace negative_binomial_detail

    template <class RealType = double, class Policy = policies::policy<> >
    class negative_binomial_distribution
    {
    public:
      typedef RealType value_type;
      typedef Policy policy_type;

      negative_binomial_distribution(RealType r, RealType p) : m_r(r), m_p(p)
      { // Constructor.
        RealType result;
        negative_binomial_detail::check_dist(
          "negative_binomial_distribution<%1%>::negative_binomial_distribution",
          m_r, // Check successes r > 0.
          m_p, // Check success_fraction 0 <= p <= 1.
          &result, Policy());
      } // negative_binomial_distribution constructor.

      // Private data getter class member functions.
      RealType success_fraction() const
      { // Probability of success as fraction in range 0 to 1.
        return m_p;
      }
      RealType successes() const
      { // Total number of successes r.
        return m_r;
      }

      static RealType find_lower_bound_on_p(
        RealType trials,
        RealType successes,
        RealType alpha) // alpha 0.05 equivalent to 95% for one-sided test.
      {
        static const char* function = "boost::math::negative_binomial<%1%>::find_lower_bound_on_p";
        RealType result = 0;  // of error checks.
        RealType failures = trials - successes;
        if(false == detail::check_probability(function, alpha, &result, Policy())
          && negative_binomial_detail::check_dist_and_k(
          function, successes, RealType(0), failures, &result, Policy()))
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
        RealType successes,
        RealType alpha) // alpha 0.05 equivalent to 95% for one-sided test.
      {
        static const char* function = "boost::math::negative_binomial<%1%>::find_upper_bound_on_p";
        RealType result = 0;  // of error checks.
        RealType failures = trials - successes;
        if(false == negative_binomial_detail::check_dist_and_k(
          function, successes, RealType(0), failures, &result, Policy())
          && detail::check_probability(function, alpha, &result, Policy()))
        {
          return result;
        }
        if(failures == 0)
           return 1;
        // Use complement ibetac_inv function for upper bound.
        // Note adjusted failures value: *not* failures+1 as usual.
        // This is adapted from the corresponding binomial formula
        // here: http://www.itl.nist.gov/div898/handbook/prc/section2/prc241.htm
        // This is a Clopper-Pearson interval, and may be overly conservative,
        // see also "A Simple Improved Inferential Method for Some
        // Discrete Distributions" Yong CAI and K. KRISHNAMOORTHY
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
        static const char* function = "boost::math::negative_binomial<%1%>::find_minimum_number_of_trials";
        // Error checks:
        RealType result = 0;
        if(false == negative_binomial_detail::check_dist_and_k(
          function, RealType(1), p, k, &result, Policy())
          && detail::check_probability(function, alpha, &result, Policy()))
        { return result; }

        result = ibeta_inva(k + 1, p, alpha, Policy());  // returns n - k
        return result + k;
      } // RealType find_number_of_failures

      static RealType find_maximum_number_of_trials(
        RealType k,     // number of failures (k >= 0).
        RealType p,     // success fraction 0 <= p <= 1.
        RealType alpha) // risk level threshold 0 <= alpha <= 1.
      {
        static const char* function = "boost::math::negative_binomial<%1%>::find_maximum_number_of_trials";
        // Error checks:
        RealType result = 0;
        if(false == negative_binomial_detail::check_dist_and_k(
          function, RealType(1), p, k, &result, Policy())
          &&  detail::check_probability(function, alpha, &result, Policy()))
        { return result; }

        result = ibetac_inva(k + 1, p, alpha, Policy());  // returns n - k
        return result + k;
      } // RealType find_number_of_trials complemented

    private:
      RealType m_r; // successes.
      RealType m_p; // success_fraction
    }; // template <class RealType, class Policy> class negative_binomial_distribution

    typedef negative_binomial_distribution<double> negative_binomial; // Reserved name of type double.

    template <class RealType, class Policy>
    inline const std::pair<RealType, RealType> range(const negative_binomial_distribution<RealType, Policy>& /* dist */)
    { // Range of permissible values for random variable k.
       using boost::math::tools::max_value;
       return std::pair<RealType, RealType>(static_cast<RealType>(0), max_value<RealType>()); // max_integer?
    }

    template <class RealType, class Policy>
    inline const std::pair<RealType, RealType> support(const negative_binomial_distribution<RealType, Policy>& /* dist */)
    { // Range of supported values for random variable k.
       // This is range where cdf rises from 0 to 1, and outside it, the pdf is zero.
       using boost::math::tools::max_value;
       return std::pair<RealType, RealType>(static_cast<RealType>(0),  max_value<RealType>()); // max_integer?
    }

    template <class RealType, class Policy>
    inline RealType mean(const negative_binomial_distribution<RealType, Policy>& dist)
    { // Mean of Negative Binomial distribution = r(1-p)/p.
      return dist.successes() * (1 - dist.success_fraction() ) / dist.success_fraction();
    } // mean

    //template <class RealType, class Policy>
    //inline RealType median(const negative_binomial_distribution<RealType, Policy>& dist)
    //{ // Median of negative_binomial_distribution is not defined.
    //  return policies::raise_domain_error<RealType>(BOOST_CURRENT_FUNCTION, "Median is not implemented, result is %1%!", std::numeric_limits<RealType>::quiet_NaN());
    //} // median
    // Now implemented via quantile(half) in derived accessors.

    template <class RealType, class Policy>
    inline RealType mode(const negative_binomial_distribution<RealType, Policy>& dist)
    { // Mode of Negative Binomial distribution = floor[(r-1) * (1 - p)/p]
      BOOST_MATH_STD_USING // ADL of std functions.
      return floor((dist.successes() -1) * (1 - dist.success_fraction()) / dist.success_fraction());
    } // mode

    template <class RealType, class Policy>
    inline RealType skewness(const negative_binomial_distribution<RealType, Policy>& dist)
    { // skewness of Negative Binomial distribution = 2-p / (sqrt(r(1-p))
      BOOST_MATH_STD_USING // ADL of std functions.
      RealType p = dist.success_fraction();
      RealType r = dist.successes();

      return (2 - p) /
        sqrt(r * (1 - p));
    } // skewness

    template <class RealType, class Policy>
    inline RealType kurtosis(const negative_binomial_distribution<RealType, Policy>& dist)
    { // kurtosis of Negative Binomial distribution
      // http://en.wikipedia.org/wiki/Negative_binomial is kurtosis_excess so add 3
      RealType p = dist.success_fraction();
      RealType r = dist.successes();
      return 3 + (6 / r) + ((p * p) / (r * (1 - p)));
    } // kurtosis

     template <class RealType, class Policy>
    inline RealType kurtosis_excess(const negative_binomial_distribution<RealType, Policy>& dist)
    { // kurtosis excess of Negative Binomial distribution
      // http://mathworld.wolfram.com/Kurtosis.html table of kurtosis_excess
      RealType p = dist.success_fraction();
      RealType r = dist.successes();
      return (6 - p * (6-p)) / (r * (1-p));
    } // kurtosis_excess

    template <class RealType, class Policy>
    inline RealType variance(const negative_binomial_distribution<RealType, Policy>& dist)
    { // Variance of Binomial distribution = r (1-p) / p^2.
      return  dist.successes() * (1 - dist.success_fraction())
        / (dist.success_fraction() * dist.success_fraction());
    } // variance

    // RealType standard_deviation(const negative_binomial_distribution<RealType, Policy>& dist)
    // standard_deviation provided by derived accessors.
    // RealType hazard(const negative_binomial_distribution<RealType, Policy>& dist)
    // hazard of Negative Binomial distribution provided by derived accessors.
    // RealType chf(const negative_binomial_distribution<RealType, Policy>& dist)
    // chf of Negative Binomial distribution provided by derived accessors.

    template <class RealType, class Policy>
    inline RealType pdf(const negative_binomial_distribution<RealType, Policy>& dist, const RealType& k)
    { // Probability Density/Mass Function.
      BOOST_FPU_EXCEPTION_GUARD

      static const char* function = "boost::math::pdf(const negative_binomial_distribution<%1%>&, %1%)";

      RealType r = dist.successes();
      RealType p = dist.success_fraction();
      RealType result = 0;
      if(false == negative_binomial_detail::check_dist_and_k(
        function,
        r,
        dist.success_fraction(),
        k,
        &result, Policy()))
      {
        return result;
      }

      result = (p/(r + k)) * ibeta_derivative(r, static_cast<RealType>(k+1), p, Policy());
      // Equivalent to:
      // return exp(lgamma(r + k) - lgamma(r) - lgamma(k+1)) * pow(p, r) * pow((1-p), k);
      return result;
    } // negative_binomial_pdf

    template <class RealType, class Policy>
    inline RealType cdf(const negative_binomial_distribution<RealType, Policy>& dist, const RealType& k)
    { // Cumulative Distribution Function of Negative Binomial.
      static const char* function = "boost::math::cdf(const negative_binomial_distribution<%1%>&, %1%)";
      using boost::math::ibeta; // Regularized incomplete beta function.
      // k argument may be integral, signed, or unsigned, or floating point.
      // If necessary, it has already been promoted from an integral type.
      RealType p = dist.success_fraction();
      RealType r = dist.successes();
      // Error check:
      RealType result = 0;
      if(false == negative_binomial_detail::check_dist_and_k(
        function,
        r,
        dist.success_fraction(),
        k,
        &result, Policy()))
      {
        return result;
      }

      RealType probability = ibeta(r, static_cast<RealType>(k+1), p, Policy());
      // Ip(r, k+1) = ibeta(r, k+1, p)
      return probability;
    } // cdf Cumulative Distribution Function Negative Binomial.

      template <class RealType, class Policy>
      inline RealType cdf(const complemented2_type<negative_binomial_distribution<RealType, Policy>, RealType>& c)
      { // Complemented Cumulative Distribution Function Negative Binomial.

      static const char* function = "boost::math::cdf(const negative_binomial_distribution<%1%>&, %1%)";
      using boost::math::ibetac; // Regularized incomplete beta function complement.
      // k argument may be integral, signed, or unsigned, or floating point.
      // If necessary, it has already been promoted from an integral type.
      RealType const& k = c.param;
      negative_binomial_distribution<RealType, Policy> const& dist = c.dist;
      RealType p = dist.success_fraction();
      RealType r = dist.successes();
      // Error check:
      RealType result = 0;
      if(false == negative_binomial_detail::check_dist_and_k(
        function,
        r,
        p,
        k,
        &result, Policy()))
      {
        return result;
      }
      // Calculate cdf negative binomial using the incomplete beta function.
      // Use of ibeta here prevents cancellation errors in calculating
      // 1-p if p is very small, perhaps smaller than machine epsilon.
      // Ip(k+1, r) = ibetac(r, k+1, p)
      // constrain_probability here?
     RealType probability = ibetac(r, static_cast<RealType>(k+1), p, Policy());
      // Numerical errors might cause probability to be slightly outside the range < 0 or > 1.
      // This might cause trouble downstream, so warn, possibly throw exception, but constrain to the limits.
      return probability;
    } // cdf Cumulative Distribution Function Negative Binomial.

    template <class RealType, class Policy>
    inline RealType quantile(const negative_binomial_distribution<RealType, Policy>& dist, const RealType& P)
    { // Quantile, percentile/100 or Percent Point Negative Binomial function.
      // Return the number of expected failures k for a given probability p.

      // Inverse cumulative Distribution Function or Quantile (percentile / 100) of negative_binomial Probability.
      // MAthCAD pnbinom return smallest k such that negative_binomial(k, n, p) >= probability.
      // k argument may be integral, signed, or unsigned, or floating point.
      // BUT Cephes/CodeCogs says: finds argument p (0 to 1) such that cdf(k, n, p) = y
      static const char* function = "boost::math::quantile(const negative_binomial_distribution<%1%>&, %1%)";
      BOOST_MATH_STD_USING // ADL of std functions.

      RealType p = dist.success_fraction();
      RealType r = dist.successes();
      // Check dist and P.
      RealType result = 0;
      if(false == negative_binomial_detail::check_dist_and_prob
        (function, r, p, P, &result, Policy()))
      {
        return result;
      }

      // Special cases.
      if (P == 1)
      {  // Would need +infinity failures for total confidence.
        result = policies::raise_overflow_error<RealType>(
            function,
            "Probability argument is 1, which implies infinite failures !", Policy());
        return result;
       // usually means return +std::numeric_limits<RealType>::infinity();
       // unless #define BOOST_MATH_THROW_ON_OVERFLOW_ERROR
      }
      if (P == 0)
      { // No failures are expected if P = 0.
        return 0; // Total trials will be just dist.successes.
      }
      if (P <= pow(dist.success_fraction(), dist.successes()))
      { // p <= pdf(dist, 0) == cdf(dist, 0)
        return 0;
      }
      if(p == 0)
      {  // Would need +infinity failures for total confidence.
         result = policies::raise_overflow_error<RealType>(
            function,
            "Success fraction is 0, which implies infinite failures !", Policy());
         return result;
         // usually means return +std::numeric_limits<RealType>::infinity();
         // unless #define BOOST_MATH_THROW_ON_OVERFLOW_ERROR
      }
      /*
      // Calculate quantile of negative_binomial using the inverse incomplete beta function.
      using boost::math::ibeta_invb;
      return ibeta_invb(r, p, P, Policy()) - 1; //
      */
      RealType guess = 0;
      RealType factor = 5;
      if(r * r * r * P * p > 0.005)
         guess = detail::inverse_negative_binomial_cornish_fisher(r, p, RealType(1-p), P, RealType(1-P), Policy());

      if(guess < 10)
      {
         //
         // Cornish-Fisher Negative binomial approximation not accurate in this area:
         //
         guess = (std::min)(RealType(r * 2), RealType(10));
      }
      else
         factor = (1-P < sqrt(tools::epsilon<RealType>())) ? 2 : (guess < 20 ? 1.2f : 1.1f);
      BOOST_MATH_INSTRUMENT_CODE("guess = " << guess);
      //
      // Max iterations permitted:
      //
      std::uintmax_t max_iter = policies::get_max_root_iterations<Policy>();
      typedef typename Policy::discrete_quantile_type discrete_type;
      return detail::inverse_discrete_quantile(
         dist,
         P,
         false,
         guess,
         factor,
         RealType(1),
         discrete_type(),
         max_iter);
    } // RealType quantile(const negative_binomial_distribution dist, p)

    template <class RealType, class Policy>
    inline RealType quantile(const complemented2_type<negative_binomial_distribution<RealType, Policy>, RealType>& c)
    {  // Quantile or Percent Point Binomial function.
       // Return the number of expected failures k for a given
       // complement of the probability Q = 1 - P.
       static const char* function = "boost::math::quantile(const negative_binomial_distribution<%1%>&, %1%)";
       BOOST_MATH_STD_USING

       // Error checks:
       RealType Q = c.param;
       const negative_binomial_distribution<RealType, Policy>& dist = c.dist;
       RealType p = dist.success_fraction();
       RealType r = dist.successes();
       RealType result = 0;
       if(false == negative_binomial_detail::check_dist_and_prob(
          function,
          r,
          p,
          Q,
          &result, Policy()))
       {
          return result;
       }

       // Special cases:
       //
       if(Q == 1)
       {  // There may actually be no answer to this question,
          // since the probability of zero failures may be non-zero,
          return 0; // but zero is the best we can do:
       }
       if(Q == 0)
       {  // Probability 1 - Q  == 1 so infinite failures to achieve certainty.
          // Would need +infinity failures for total confidence.
          result = policies::raise_overflow_error<RealType>(
             function,
             "Probability argument complement is 0, which implies infinite failures !", Policy());
          return result;
          // usually means return +std::numeric_limits<RealType>::infinity();
          // unless #define BOOST_MATH_THROW_ON_OVERFLOW_ERROR
       }
       if (-Q <= boost::math::powm1(dist.success_fraction(), dist.successes(), Policy()))
       {  // q <= cdf(complement(dist, 0)) == pdf(dist, 0)
          return 0; //
       }
       if(p == 0)
       {  // Success fraction is 0 so infinite failures to achieve certainty.
          // Would need +infinity failures for total confidence.
          result = policies::raise_overflow_error<RealType>(
             function,
             "Success fraction is 0, which implies infinite failures !", Policy());
          return result;
          // usually means return +std::numeric_limits<RealType>::infinity();
          // unless #define BOOST_MATH_THROW_ON_OVERFLOW_ERROR
       }
       //return ibetac_invb(r, p, Q, Policy()) -1;
       RealType guess = 0;
       RealType factor = 5;
       if(r * r * r * (1-Q) * p > 0.005)
          guess = detail::inverse_negative_binomial_cornish_fisher(r, p, RealType(1-p), RealType(1-Q), Q, Policy());

       if(guess < 10)
       {
          //
          // Cornish-Fisher Negative binomial approximation not accurate in this area:
          //
          guess = (std::min)(RealType(r * 2), RealType(10));
       }
       else
          factor = (Q < sqrt(tools::epsilon<RealType>())) ? 2 : (guess < 20 ? 1.2f : 1.1f);
       BOOST_MATH_INSTRUMENT_CODE("guess = " << guess);
       //
       // Max iterations permitted:
       //
       std::uintmax_t max_iter = policies::get_max_root_iterations<Policy>();
       typedef typename Policy::discrete_quantile_type discrete_type;
       return detail::inverse_discrete_quantile(
          dist,
          Q,
          true,
          guess,
          factor,
          RealType(1),
          discrete_type(),
          max_iter);
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

#endif // BOOST_MATH_SPECIAL_NEGATIVE_BINOMIAL_HPP
