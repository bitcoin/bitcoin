// boost\math\distributions\non_central_f.hpp

// Copyright John Maddock 2008.

// Use, modification and distribution are subject to the
// Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MATH_SPECIAL_NON_CENTRAL_F_HPP
#define BOOST_MATH_SPECIAL_NON_CENTRAL_F_HPP

#include <boost/math/distributions/non_central_beta.hpp>
#include <boost/math/distributions/detail/generic_mode.hpp>
#include <boost/math/special_functions/pow.hpp>

namespace boost
{
   namespace math
   {
      template <class RealType = double, class Policy = policies::policy<> >
      class non_central_f_distribution
      {
      public:
         typedef RealType value_type;
         typedef Policy policy_type;

         non_central_f_distribution(RealType v1_, RealType v2_, RealType lambda) : v1(v1_), v2(v2_), ncp(lambda)
         {
            const char* function = "boost::math::non_central_f_distribution<%1%>::non_central_f_distribution(%1%,%1%)";
            RealType r;
            detail::check_df(
               function,
               v1, &r, Policy());
            detail::check_df(
               function,
               v2, &r, Policy());
            detail::check_non_centrality(
               function,
               lambda,
               &r,
               Policy());
         } // non_central_f_distribution constructor.

         RealType degrees_of_freedom1()const
         {
            return v1;
         }
         RealType degrees_of_freedom2()const
         {
            return v2;
         }
         RealType non_centrality() const
         { // Private data getter function.
            return ncp;
         }
      private:
         // Data member, initialized by constructor.
         RealType v1;   // alpha.
         RealType v2;   // beta.
         RealType ncp; // non-centrality parameter
      }; // template <class RealType, class Policy> class non_central_f_distribution

      typedef non_central_f_distribution<double> non_central_f; // Reserved name of type double.

      // Non-member functions to give properties of the distribution.

      template <class RealType, class Policy>
      inline const std::pair<RealType, RealType> range(const non_central_f_distribution<RealType, Policy>& /* dist */)
      { // Range of permissible values for random variable k.
         using boost::math::tools::max_value;
         return std::pair<RealType, RealType>(static_cast<RealType>(0), max_value<RealType>());
      }

      template <class RealType, class Policy>
      inline const std::pair<RealType, RealType> support(const non_central_f_distribution<RealType, Policy>& /* dist */)
      { // Range of supported values for random variable k.
         // This is range where cdf rises from 0 to 1, and outside it, the pdf is zero.
         using boost::math::tools::max_value;
         return std::pair<RealType, RealType>(static_cast<RealType>(0), max_value<RealType>());
      }

      template <class RealType, class Policy>
      inline RealType mean(const non_central_f_distribution<RealType, Policy>& dist)
      {
         const char* function = "mean(non_central_f_distribution<%1%> const&)";
         RealType v1 = dist.degrees_of_freedom1();
         RealType v2 = dist.degrees_of_freedom2();
         RealType l = dist.non_centrality();
         RealType r;
         if(!detail::check_df(
            function,
            v1, &r, Policy())
               ||
            !detail::check_df(
               function,
               v2, &r, Policy())
               ||
            !detail::check_non_centrality(
               function,
               l,
               &r,
               Policy()))
               return r;
         if(v2 <= 2)
            return policies::raise_domain_error(
               function,
               "Second degrees of freedom parameter was %1%, but must be > 2 !",
               v2, Policy());
         return v2 * (v1 + l) / (v1 * (v2 - 2));
      } // mean

      template <class RealType, class Policy>
      inline RealType mode(const non_central_f_distribution<RealType, Policy>& dist)
      { // mode.
         static const char* function = "mode(non_central_chi_squared_distribution<%1%> const&)";

         RealType n = dist.degrees_of_freedom1();
         RealType m = dist.degrees_of_freedom2();
         RealType l = dist.non_centrality();
         RealType r;
         if(!detail::check_df(
            function,
            n, &r, Policy())
               ||
            !detail::check_df(
               function,
               m, &r, Policy())
               ||
            !detail::check_non_centrality(
               function,
               l,
               &r,
               Policy()))
               return r;
         RealType guess = m > 2 ? RealType(m * (n + l) / (n * (m - 2))) : RealType(1);
         return detail::generic_find_mode(
            dist,
            guess,
            function);
      }

      template <class RealType, class Policy>
      inline RealType variance(const non_central_f_distribution<RealType, Policy>& dist)
      { // variance.
         const char* function = "variance(non_central_f_distribution<%1%> const&)";
         RealType n = dist.degrees_of_freedom1();
         RealType m = dist.degrees_of_freedom2();
         RealType l = dist.non_centrality();
         RealType r;
         if(!detail::check_df(
            function,
            n, &r, Policy())
               ||
            !detail::check_df(
               function,
               m, &r, Policy())
               ||
            !detail::check_non_centrality(
               function,
               l,
               &r,
               Policy()))
               return r;
         if(m <= 4)
            return policies::raise_domain_error(
               function,
               "Second degrees of freedom parameter was %1%, but must be > 4 !",
               m, Policy());
         RealType result = 2 * m * m * ((n + l) * (n + l)
            + (m - 2) * (n + 2 * l));
         result /= (m - 4) * (m - 2) * (m - 2) * n * n;
         return result;
      }

      // RealType standard_deviation(const non_central_f_distribution<RealType, Policy>& dist)
      // standard_deviation provided by derived accessors.

      template <class RealType, class Policy>
      inline RealType skewness(const non_central_f_distribution<RealType, Policy>& dist)
      { // skewness = sqrt(l).
         const char* function = "skewness(non_central_f_distribution<%1%> const&)";
         BOOST_MATH_STD_USING
         RealType n = dist.degrees_of_freedom1();
         RealType m = dist.degrees_of_freedom2();
         RealType l = dist.non_centrality();
         RealType r;
         if(!detail::check_df(
            function,
            n, &r, Policy())
               ||
            !detail::check_df(
               function,
               m, &r, Policy())
               ||
            !detail::check_non_centrality(
               function,
               l,
               &r,
               Policy()))
               return r;
         if(m <= 6)
            return policies::raise_domain_error(
               function,
               "Second degrees of freedom parameter was %1%, but must be > 6 !",
               m, Policy());
         RealType result = 2 * constants::root_two<RealType>();
         result *= sqrt(m - 4);
         result *= (n * (m + n - 2) *(m + 2 * n - 2)
            + 3 * (m + n - 2) * (m + 2 * n - 2) * l
            + 6 * (m + n - 2) * l * l + 2 * l * l * l);
         result /= (m - 6) * pow(n * (m + n - 2) + 2 * (m + n - 2) * l + l * l, RealType(1.5f));
         return result;
      }

      template <class RealType, class Policy>
      inline RealType kurtosis_excess(const non_central_f_distribution<RealType, Policy>& dist)
      {
         const char* function = "kurtosis_excess(non_central_f_distribution<%1%> const&)";
         BOOST_MATH_STD_USING
         RealType n = dist.degrees_of_freedom1();
         RealType m = dist.degrees_of_freedom2();
         RealType l = dist.non_centrality();
         RealType r;
         if(!detail::check_df(
            function,
            n, &r, Policy())
               ||
            !detail::check_df(
               function,
               m, &r, Policy())
               ||
            !detail::check_non_centrality(
               function,
               l,
               &r,
               Policy()))
               return r;
         if(m <= 8)
            return policies::raise_domain_error(
               function,
               "Second degrees of freedom parameter was %1%, but must be > 8 !",
               m, Policy());
         RealType l2 = l * l;
         RealType l3 = l2 * l;
         RealType l4 = l2 * l2;
         RealType result = (3 * (m - 4) * (n * (m + n - 2)
            * (4 * (m - 2) * (m - 2)
            + (m - 2) * (m + 10) * n
            + (10 + m) * n * n)
            + 4 * (m + n - 2) * (4 * (m - 2) * (m - 2)
            + (m - 2) * (10 + m) * n
            + (10 + m) * n * n) * l + 2 * (10 + m)
            * (m + n - 2) * (2 * m + 3 * n - 4) * l2
            + 4 * (10 + m) * (-2 + m + n) * l3
            + (10 + m) * l4))
            /
            ((-8 + m) * (-6 + m) * boost::math::pow<2>(n * (-2 + m + n)
            + 2 * (-2 + m + n) * l + l2));
            return result;
      } // kurtosis_excess

      template <class RealType, class Policy>
      inline RealType kurtosis(const non_central_f_distribution<RealType, Policy>& dist)
      {
         return kurtosis_excess(dist) + 3;
      }

      template <class RealType, class Policy>
      inline RealType pdf(const non_central_f_distribution<RealType, Policy>& dist, const RealType& x)
      { // Probability Density/Mass Function.
         typedef typename policies::evaluation<RealType, Policy>::type value_type;
         typedef typename policies::normalise<
            Policy,
            policies::promote_float<false>,
            policies::promote_double<false>,
            policies::discrete_quantile<>,
            policies::assert_undefined<> >::type forwarding_policy;

         value_type alpha = dist.degrees_of_freedom1() / 2;
         value_type beta = dist.degrees_of_freedom2() / 2;
         value_type y = x * alpha / beta;
         value_type r = pdf(boost::math::non_central_beta_distribution<value_type, forwarding_policy>(alpha, beta, dist.non_centrality()), y / (1 + y));
         return policies::checked_narrowing_cast<RealType, forwarding_policy>(
            r * (dist.degrees_of_freedom1() / dist.degrees_of_freedom2()) / ((1 + y) * (1 + y)),
            "pdf(non_central_f_distribution<%1%>, %1%)");
      } // pdf

      template <class RealType, class Policy>
      RealType cdf(const non_central_f_distribution<RealType, Policy>& dist, const RealType& x)
      {
         const char* function = "cdf(const non_central_f_distribution<%1%>&, %1%)";
         RealType r;
         if(!detail::check_df(
            function,
            dist.degrees_of_freedom1(), &r, Policy())
               ||
            !detail::check_df(
               function,
               dist.degrees_of_freedom2(), &r, Policy())
               ||
            !detail::check_non_centrality(
               function,
               dist.non_centrality(),
               &r,
               Policy()))
               return r;

         if((x < 0) || !(boost::math::isfinite)(x))
         {
            return policies::raise_domain_error<RealType>(
               function, "Random Variable parameter was %1%, but must be > 0 !", x, Policy());
         }

         RealType alpha = dist.degrees_of_freedom1() / 2;
         RealType beta = dist.degrees_of_freedom2() / 2;
         RealType y = x * alpha / beta;
         RealType c = y / (1 + y);
         RealType cp = 1 / (1 + y);
         //
         // To ensure accuracy, we pass both x and 1-x to the
         // non-central beta cdf routine, this ensures accuracy
         // even when we compute x to be ~ 1:
         //
         r = detail::non_central_beta_cdf(c, cp, alpha, beta,
            dist.non_centrality(), false, Policy());
         return r;
      } // cdf

      template <class RealType, class Policy>
      RealType cdf(const complemented2_type<non_central_f_distribution<RealType, Policy>, RealType>& c)
      { // Complemented Cumulative Distribution Function
         const char* function = "cdf(complement(const non_central_f_distribution<%1%>&, %1%))";
         RealType r;
         if(!detail::check_df(
            function,
            c.dist.degrees_of_freedom1(), &r, Policy())
               ||
            !detail::check_df(
               function,
               c.dist.degrees_of_freedom2(), &r, Policy())
               ||
            !detail::check_non_centrality(
               function,
               c.dist.non_centrality(),
               &r,
               Policy()))
               return r;

         if((c.param < 0) || !(boost::math::isfinite)(c.param))
         {
            return policies::raise_domain_error<RealType>(
               function, "Random Variable parameter was %1%, but must be > 0 !", c.param, Policy());
         }

         RealType alpha = c.dist.degrees_of_freedom1() / 2;
         RealType beta = c.dist.degrees_of_freedom2() / 2;
         RealType y = c.param * alpha / beta;
         RealType x = y / (1 + y);
         RealType cx = 1 / (1 + y);
         //
         // To ensure accuracy, we pass both x and 1-x to the
         // non-central beta cdf routine, this ensures accuracy
         // even when we compute x to be ~ 1:
         //
         r = detail::non_central_beta_cdf(x, cx, alpha, beta,
            c.dist.non_centrality(), true, Policy());
         return r;
      } // ccdf

      template <class RealType, class Policy>
      inline RealType quantile(const non_central_f_distribution<RealType, Policy>& dist, const RealType& p)
      { // Quantile (or Percent Point) function.
         RealType alpha = dist.degrees_of_freedom1() / 2;
         RealType beta = dist.degrees_of_freedom2() / 2;
         RealType x = quantile(boost::math::non_central_beta_distribution<RealType, Policy>(alpha, beta, dist.non_centrality()), p);
         if(x == 1)
            return policies::raise_overflow_error<RealType>(
               "quantile(const non_central_f_distribution<%1%>&, %1%)",
               "Result of non central F quantile is too large to represent.",
               Policy());
         return (x / (1 - x)) * (dist.degrees_of_freedom2() / dist.degrees_of_freedom1());
      } // quantile

      template <class RealType, class Policy>
      inline RealType quantile(const complemented2_type<non_central_f_distribution<RealType, Policy>, RealType>& c)
      { // Quantile (or Percent Point) function.
         RealType alpha = c.dist.degrees_of_freedom1() / 2;
         RealType beta = c.dist.degrees_of_freedom2() / 2;
         RealType x = quantile(complement(boost::math::non_central_beta_distribution<RealType, Policy>(alpha, beta, c.dist.non_centrality()), c.param));
         if(x == 1)
            return policies::raise_overflow_error<RealType>(
               "quantile(complement(const non_central_f_distribution<%1%>&, %1%))",
               "Result of non central F quantile is too large to represent.",
               Policy());
         return (x / (1 - x)) * (c.dist.degrees_of_freedom2() / c.dist.degrees_of_freedom1());
      } // quantile complement.

   } // namespace math
} // namespace boost

// This include must be at the end, *after* the accessors
// for this distribution have been defined, in order to
// keep compilers that support two-phase lookup happy.
#include <boost/math/distributions/detail/derived_accessors.hpp>

#endif // BOOST_MATH_SPECIAL_NON_CENTRAL_F_HPP



