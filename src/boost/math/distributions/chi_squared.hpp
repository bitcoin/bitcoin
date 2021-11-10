// Copyright John Maddock 2006, 2007.
// Copyright Paul A. Bristow 2008, 2010.

// Use, modification and distribution are subject to the
// Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MATH_DISTRIBUTIONS_CHI_SQUARED_HPP
#define BOOST_MATH_DISTRIBUTIONS_CHI_SQUARED_HPP

#include <boost/math/distributions/fwd.hpp>
#include <boost/math/special_functions/gamma.hpp> // for incomplete beta.
#include <boost/math/distributions/complement.hpp> // complements
#include <boost/math/distributions/detail/common_error_handling.hpp> // error checks
#include <boost/math/special_functions/fpclassify.hpp>

#include <utility>

namespace boost{ namespace math{

template <class RealType = double, class Policy = policies::policy<> >
class chi_squared_distribution
{
public:
   typedef RealType value_type;
   typedef Policy policy_type;

   chi_squared_distribution(RealType i) : m_df(i)
   {
      RealType result;
      detail::check_df(
         "boost::math::chi_squared_distribution<%1%>::chi_squared_distribution", m_df, &result, Policy());
   } // chi_squared_distribution

   RealType degrees_of_freedom()const
   {
      return m_df;
   }

   // Parameter estimation:
   static RealType find_degrees_of_freedom(
      RealType difference_from_variance,
      RealType alpha,
      RealType beta,
      RealType variance,
      RealType hint = 100);

private:
   //
   // Data member:
   //
   RealType m_df; // degrees of freedom is a positive real number.
}; // class chi_squared_distribution

typedef chi_squared_distribution<double> chi_squared;

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4127)
#endif

template <class RealType, class Policy>
inline const std::pair<RealType, RealType> range(const chi_squared_distribution<RealType, Policy>& /*dist*/)
{ // Range of permissible values for random variable x.
  if (std::numeric_limits<RealType>::has_infinity)
  { 
    return std::pair<RealType, RealType>(static_cast<RealType>(0), std::numeric_limits<RealType>::infinity()); // 0 to + infinity.
  }
  else
  {
    using boost::math::tools::max_value;
    return std::pair<RealType, RealType>(static_cast<RealType>(0), max_value<RealType>()); // 0 to + max.
  }
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif

template <class RealType, class Policy>
inline const std::pair<RealType, RealType> support(const chi_squared_distribution<RealType, Policy>& /*dist*/)
{ // Range of supported values for random variable x.
   // This is range where cdf rises from 0 to 1, and outside it, the pdf is zero.
   return std::pair<RealType, RealType>(static_cast<RealType>(0), tools::max_value<RealType>()); // 0 to + infinity.
}

template <class RealType, class Policy>
RealType pdf(const chi_squared_distribution<RealType, Policy>& dist, const RealType& chi_square)
{
   BOOST_MATH_STD_USING  // for ADL of std functions
   RealType degrees_of_freedom = dist.degrees_of_freedom();
   // Error check:
   RealType error_result;

   static const char* function = "boost::math::pdf(const chi_squared_distribution<%1%>&, %1%)";

   if(false == detail::check_df(
         function, degrees_of_freedom, &error_result, Policy()))
      return error_result;

   if((chi_square < 0) || !(boost::math::isfinite)(chi_square))
   {
      return policies::raise_domain_error<RealType>(
         function, "Chi Square parameter was %1%, but must be > 0 !", chi_square, Policy());
   }

   if(chi_square == 0)
   {
      // Handle special cases:
      if(degrees_of_freedom < 2)
      {
         return policies::raise_overflow_error<RealType>(
            function, 0, Policy());
      }
      else if(degrees_of_freedom == 2)
      {
         return 0.5f;
      }
      else
      {
         return 0;
      }
   }

   return gamma_p_derivative(degrees_of_freedom / 2, chi_square / 2, Policy()) / 2;
} // pdf

template <class RealType, class Policy>
inline RealType cdf(const chi_squared_distribution<RealType, Policy>& dist, const RealType& chi_square)
{
   RealType degrees_of_freedom = dist.degrees_of_freedom();
   // Error check:
   RealType error_result;
   static const char* function = "boost::math::cdf(const chi_squared_distribution<%1%>&, %1%)";

   if(false == detail::check_df(
         function, degrees_of_freedom, &error_result, Policy()))
      return error_result;

   if((chi_square < 0) || !(boost::math::isfinite)(chi_square))
   {
      return policies::raise_domain_error<RealType>(
         function, "Chi Square parameter was %1%, but must be > 0 !", chi_square, Policy());
   }

   return boost::math::gamma_p(degrees_of_freedom / 2, chi_square / 2, Policy());
} // cdf

template <class RealType, class Policy>
inline RealType quantile(const chi_squared_distribution<RealType, Policy>& dist, const RealType& p)
{
   RealType degrees_of_freedom = dist.degrees_of_freedom();
   static const char* function = "boost::math::quantile(const chi_squared_distribution<%1%>&, %1%)";
   // Error check:
   RealType error_result;
   if(false ==
     (
       detail::check_df(function, degrees_of_freedom, &error_result, Policy())
       && detail::check_probability(function, p, &error_result, Policy()))
     )
     return error_result;

   return 2 * boost::math::gamma_p_inv(degrees_of_freedom / 2, p, Policy());
} // quantile

template <class RealType, class Policy>
inline RealType cdf(const complemented2_type<chi_squared_distribution<RealType, Policy>, RealType>& c)
{
   RealType const& degrees_of_freedom = c.dist.degrees_of_freedom();
   RealType const& chi_square = c.param;
   static const char* function = "boost::math::cdf(const chi_squared_distribution<%1%>&, %1%)";
   // Error check:
   RealType error_result;
   if(false == detail::check_df(
         function, degrees_of_freedom, &error_result, Policy()))
      return error_result;

   if((chi_square < 0) || !(boost::math::isfinite)(chi_square))
   {
      return policies::raise_domain_error<RealType>(
         function, "Chi Square parameter was %1%, but must be > 0 !", chi_square, Policy());
   }

   return boost::math::gamma_q(degrees_of_freedom / 2, chi_square / 2, Policy());
}

template <class RealType, class Policy>
inline RealType quantile(const complemented2_type<chi_squared_distribution<RealType, Policy>, RealType>& c)
{
   RealType const& degrees_of_freedom = c.dist.degrees_of_freedom();
   RealType const& q = c.param;
   static const char* function = "boost::math::quantile(const chi_squared_distribution<%1%>&, %1%)";
   // Error check:
   RealType error_result;
   if(false == (
     detail::check_df(function, degrees_of_freedom, &error_result, Policy())
     && detail::check_probability(function, q, &error_result, Policy()))
     )
    return error_result;

   return 2 * boost::math::gamma_q_inv(degrees_of_freedom / 2, q, Policy());
}

template <class RealType, class Policy>
inline RealType mean(const chi_squared_distribution<RealType, Policy>& dist)
{ // Mean of Chi-Squared distribution = v.
  return dist.degrees_of_freedom();
} // mean

template <class RealType, class Policy>
inline RealType variance(const chi_squared_distribution<RealType, Policy>& dist)
{ // Variance of Chi-Squared distribution = 2v.
  return 2 * dist.degrees_of_freedom();
} // variance

template <class RealType, class Policy>
inline RealType mode(const chi_squared_distribution<RealType, Policy>& dist)
{
   RealType df = dist.degrees_of_freedom();
   static const char* function = "boost::math::mode(const chi_squared_distribution<%1%>&)";
   // Most sources only define mode for df >= 2,
   // but for 0 <= df <= 2, the pdf maximum actually occurs at random variate = 0;
   // So one could extend the definition of mode thus:
   //if(df < 0)
   //{
   //   return policies::raise_domain_error<RealType>(
   //      function,
   //      "Chi-Squared distribution only has a mode for degrees of freedom >= 0, but got degrees of freedom = %1%.",
   //      df, Policy());
   //}
   //return (df <= 2) ? 0 : df - 2;

   if(df < 2)
      return policies::raise_domain_error<RealType>(
         function,
         "Chi-Squared distribution only has a mode for degrees of freedom >= 2, but got degrees of freedom = %1%.",
         df, Policy());
   return df - 2;
}

//template <class RealType, class Policy>
//inline RealType median(const chi_squared_distribution<RealType, Policy>& dist)
//{ // Median is given by Quantile[dist, 1/2]
//   RealType df = dist.degrees_of_freedom();
//   if(df <= 1)
//      return tools::domain_error<RealType>(
//         BOOST_CURRENT_FUNCTION,
//         "The Chi-Squared distribution only has a mode for degrees of freedom >= 2, but got degrees of freedom = %1%.",
//         df);
//   return df - RealType(2)/3;
//}
// Now implemented via quantile(half) in derived accessors.

template <class RealType, class Policy>
inline RealType skewness(const chi_squared_distribution<RealType, Policy>& dist)
{
   BOOST_MATH_STD_USING // For ADL
   RealType df = dist.degrees_of_freedom();
   return sqrt (8 / df);  // == 2 * sqrt(2 / df);
}

template <class RealType, class Policy>
inline RealType kurtosis(const chi_squared_distribution<RealType, Policy>& dist)
{
   RealType df = dist.degrees_of_freedom();
   return 3 + 12 / df;
}

template <class RealType, class Policy>
inline RealType kurtosis_excess(const chi_squared_distribution<RealType, Policy>& dist)
{
   RealType df = dist.degrees_of_freedom();
   return 12 / df;
}

//
// Parameter estimation comes last:
//
namespace detail
{

template <class RealType, class Policy>
struct df_estimator
{
   df_estimator(RealType a, RealType b, RealType variance, RealType delta)
      : alpha(a), beta(b), ratio(delta/variance)
   { // Constructor
   }

   RealType operator()(const RealType& df)
   {
      if(df <= tools::min_value<RealType>())
         return 1;
      chi_squared_distribution<RealType, Policy> cs(df);

      RealType result;
      if(ratio > 0)
      {
         RealType r = 1 + ratio;
         result = cdf(cs, quantile(complement(cs, alpha)) / r) - beta;
      }
      else
      { // ratio <= 0
         RealType r = 1 + ratio;
         result = cdf(complement(cs, quantile(cs, alpha) / r)) - beta;
      }
      return result;
   }
private:
   RealType alpha;
   RealType beta;
   RealType ratio; // Difference from variance / variance, so fractional.
};

} // namespace detail

template <class RealType, class Policy>
RealType chi_squared_distribution<RealType, Policy>::find_degrees_of_freedom(
   RealType difference_from_variance,
   RealType alpha,
   RealType beta,
   RealType variance,
   RealType hint)
{
   static const char* function = "boost::math::chi_squared_distribution<%1%>::find_degrees_of_freedom(%1%,%1%,%1%,%1%,%1%)";
   // Check for domain errors:
   RealType error_result;
   if(false ==
     detail::check_probability(function, alpha, &error_result, Policy())
     && detail::check_probability(function, beta, &error_result, Policy()))
   { // Either probability is outside 0 to 1.
      return error_result;
   }

   if(hint <= 0)
   { // No hint given, so guess df = 1.
      hint = 1;
   }

   detail::df_estimator<RealType, Policy> f(alpha, beta, variance, difference_from_variance);
   tools::eps_tolerance<RealType> tol(policies::digits<RealType, Policy>());
   std::uintmax_t max_iter = policies::get_max_root_iterations<Policy>();
   std::pair<RealType, RealType> r =
     tools::bracket_and_solve_root(f, hint, RealType(2), false, tol, max_iter, Policy());
   RealType result = r.first + (r.second - r.first) / 2;
   if(max_iter >= policies::get_max_root_iterations<Policy>())
   {
      policies::raise_evaluation_error<RealType>(function, "Unable to locate solution in a reasonable time:"
         " either there is no answer to how many degrees of freedom are required"
         " or the answer is infinite.  Current best guess is %1%", result, Policy());
   }
   return result;
}

} // namespace math
} // namespace boost

// This include must be at the end, *after* the accessors
// for this distribution have been defined, in order to
// keep compilers that support two-phase lookup happy.
#include <boost/math/distributions/detail/derived_accessors.hpp>

#endif // BOOST_MATH_DISTRIBUTIONS_CHI_SQUARED_HPP
