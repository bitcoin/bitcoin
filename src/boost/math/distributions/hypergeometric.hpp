// Copyright 2008 Gautam Sewani
// Copyright 2008 John Maddock
// Copyright 2021 Paul A. Bristow
//
// Use, modification and distribution are subject to the
// Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MATH_DISTRIBUTIONS_HYPERGEOMETRIC_HPP
#define BOOST_MATH_DISTRIBUTIONS_HYPERGEOMETRIC_HPP

#include <boost/math/distributions/detail/common_error_handling.hpp>
#include <boost/math/distributions/complement.hpp>
#include <boost/math/distributions/detail/hypergeometric_pdf.hpp>
#include <boost/math/distributions/detail/hypergeometric_cdf.hpp>
#include <boost/math/distributions/detail/hypergeometric_quantile.hpp>
#include <boost/math/special_functions/fpclassify.hpp>

namespace boost { namespace math {

   template <class RealType = double, class Policy = policies::policy<> >
   class hypergeometric_distribution
   {
   public:
      typedef RealType value_type;
      typedef Policy policy_type;

      hypergeometric_distribution(unsigned r, unsigned n, unsigned N) // Constructor. r=defective/failures/success, n=trials/draws, N=total population.
         : m_n(n), m_N(N), m_r(r)
      {
         static const char* function = "boost::math::hypergeometric_distribution<%1%>::hypergeometric_distribution";
         RealType ret;
         check_params(function, &ret);
      }
      // Accessor functions.
      unsigned total()const
      {
         return m_N;
      }

      unsigned defective()const // successes/failures/events
      {
         return m_r;
      }

      unsigned sample_count()const
      {
         return m_n;
      }

      bool check_params(const char* function, RealType* result)const
      {
         if(m_r > m_N)
         {
            *result = boost::math::policies::raise_domain_error<RealType>(
               function, "Parameter r out of range: must be <= N but got %1%", static_cast<RealType>(m_r), Policy());
            return false;
         }
         if(m_n > m_N)
         {
            *result = boost::math::policies::raise_domain_error<RealType>(
               function, "Parameter n out of range: must be <= N but got %1%", static_cast<RealType>(m_n), Policy());
            return false;
         }
         return true;
      }
      bool check_x(unsigned x, const char* function, RealType* result)const
      {
         if(x < static_cast<unsigned>((std::max)(0, (int)(m_n + m_r) - (int)(m_N))))
         {
            *result = boost::math::policies::raise_domain_error<RealType>(
               function, "Random variable out of range: must be > 0 and > m + r - N but got %1%", static_cast<RealType>(x), Policy());
            return false;
         }
         if(x > (std::min)(m_r, m_n))
         {
            *result = boost::math::policies::raise_domain_error<RealType>(
               function, "Random variable out of range: must be less than both n and r but got %1%", static_cast<RealType>(x), Policy());
            return false;
         }
         return true;
      }

   private:
      // Data members:
      unsigned m_n;  // number of items picked or drawn.
      unsigned m_N; // number of "total" items.
      unsigned m_r; // number of "defective/successes/failures/events items.

   }; // class hypergeometric_distribution

   typedef hypergeometric_distribution<double> hypergeometric;

   template <class RealType, class Policy>
   inline const std::pair<unsigned, unsigned> range(const hypergeometric_distribution<RealType, Policy>& dist)
   { // Range of permissible values for random variable x.
#ifdef _MSC_VER
#  pragma warning(push)
#  pragma warning(disable:4267)
#endif
      unsigned r = dist.defective();
      unsigned n = dist.sample_count();
      unsigned N = dist.total();
      unsigned l = static_cast<unsigned>((std::max)(0, (int)(n + r) - (int)(N)));
      unsigned u = (std::min)(r, n);
      return std::pair<unsigned, unsigned>(l, u);
#ifdef _MSC_VER
#  pragma warning(pop)
#endif
   }

   template <class RealType, class Policy>
   inline const std::pair<unsigned, unsigned> support(const hypergeometric_distribution<RealType, Policy>& d)
   { 
      return range(d);
   }

   template <class RealType, class Policy>
   inline RealType pdf(const hypergeometric_distribution<RealType, Policy>& dist, const unsigned& x)
   {
      static const char* function = "boost::math::pdf(const hypergeometric_distribution<%1%>&, const %1%&)";
      RealType result = 0;
      if(!dist.check_params(function, &result))
         return result;
      if(!dist.check_x(x, function, &result))
         return result;

      return boost::math::detail::hypergeometric_pdf<RealType>(
         x, dist.defective(), dist.sample_count(), dist.total(), Policy());
   }

   template <class RealType, class Policy, class U>
   inline RealType pdf(const hypergeometric_distribution<RealType, Policy>& dist, const U& x)
   {
      BOOST_MATH_STD_USING
      static const char* function = "boost::math::pdf(const hypergeometric_distribution<%1%>&, const %1%&)";
      RealType r = static_cast<RealType>(x);
      unsigned u = itrunc(r, typename policies::normalise<Policy, policies::rounding_error<policies::ignore_error> >::type());
      if(u != r)
      {
         return boost::math::policies::raise_domain_error<RealType>(
            function, "Random variable out of range: must be an integer but got %1%", r, Policy());
      }
      return pdf(dist, u);
   }

   template <class RealType, class Policy>
   inline RealType cdf(const hypergeometric_distribution<RealType, Policy>& dist, const unsigned& x)
   {
      static const char* function = "boost::math::cdf(const hypergeometric_distribution<%1%>&, const %1%&)";
      RealType result = 0;
      if(!dist.check_params(function, &result))
         return result;
      if(!dist.check_x(x, function, &result))
         return result;

      return boost::math::detail::hypergeometric_cdf<RealType>(
         x, dist.defective(), dist.sample_count(), dist.total(), false, Policy());
   }

   template <class RealType, class Policy, class U>
   inline RealType cdf(const hypergeometric_distribution<RealType, Policy>& dist, const U& x)
   {
      BOOST_MATH_STD_USING
      static const char* function = "boost::math::cdf(const hypergeometric_distribution<%1%>&, const %1%&)";
      RealType r = static_cast<RealType>(x);
      unsigned u = itrunc(r, typename policies::normalise<Policy, policies::rounding_error<policies::ignore_error> >::type());
      if(u != r)
      {
         return boost::math::policies::raise_domain_error<RealType>(
            function, "Random variable out of range: must be an integer but got %1%", r, Policy());
      }
      return cdf(dist, u);
   }

   template <class RealType, class Policy>
   inline RealType cdf(const complemented2_type<hypergeometric_distribution<RealType, Policy>, unsigned>& c)
   {
      static const char* function = "boost::math::cdf(const hypergeometric_distribution<%1%>&, const %1%&)";
      RealType result = 0;
      if(!c.dist.check_params(function, &result))
         return result;
      if(!c.dist.check_x(c.param, function, &result))
         return result;

      return boost::math::detail::hypergeometric_cdf<RealType>(
         c.param, c.dist.defective(), c.dist.sample_count(), c.dist.total(), true, Policy());
   }

   template <class RealType, class Policy, class U>
   inline RealType cdf(const complemented2_type<hypergeometric_distribution<RealType, Policy>, U>& c)
   {
      BOOST_MATH_STD_USING
      static const char* function = "boost::math::cdf(const hypergeometric_distribution<%1%>&, const %1%&)";
      RealType r = static_cast<RealType>(c.param);
      unsigned u = itrunc(r, typename policies::normalise<Policy, policies::rounding_error<policies::ignore_error> >::type());
      if(u != r)
      {
         return boost::math::policies::raise_domain_error<RealType>(
            function, "Random variable out of range: must be an integer but got %1%", r, Policy());
      }
      return cdf(complement(c.dist, u));
   }

   template <class RealType, class Policy>
   inline RealType quantile(const hypergeometric_distribution<RealType, Policy>& dist, const RealType& p)
   {
      BOOST_MATH_STD_USING // for ADL of std functions

         // Checking function argument
         RealType result = 0;
      const char* function = "boost::math::quantile(const hypergeometric_distribution<%1%>&, %1%)";
      if (false == dist.check_params(function, &result)) return result;
      if(false == detail::check_probability(function, p, &result, Policy())) return result;

      return static_cast<RealType>(detail::hypergeometric_quantile(p, RealType(1 - p), dist.defective(), dist.sample_count(), dist.total(), Policy()));
   } // quantile

   template <class RealType, class Policy>
   inline RealType quantile(const complemented2_type<hypergeometric_distribution<RealType, Policy>, RealType>& c)
   {
      BOOST_MATH_STD_USING // for ADL of std functions

      // Checking function argument
      RealType result = 0;
      const char* function = "quantile(const complemented2_type<hypergeometric_distribution<%1%>, %1%>&)";
      if (false == c.dist.check_params(function, &result)) return result;
      if(false == detail::check_probability(function, c.param, &result, Policy())) return result;

      return static_cast<RealType>(detail::hypergeometric_quantile(RealType(1 - c.param), c.param, c.dist.defective(), c.dist.sample_count(), c.dist.total(), Policy()));
   } // quantile

   // https://www.wolframalpha.com/input/?i=kurtosis+hypergeometric+distribution 

   template <class RealType, class Policy>
   inline RealType mean(const hypergeometric_distribution<RealType, Policy>& dist)
   {
      return static_cast<RealType>(dist.defective() * dist.sample_count()) / dist.total();
   } // RealType mean(const hypergeometric_distribution<RealType, Policy>& dist)

   template <class RealType, class Policy>
   inline RealType variance(const hypergeometric_distribution<RealType, Policy>& dist)
   {
      RealType r = static_cast<RealType>(dist.defective());
      RealType n = static_cast<RealType>(dist.sample_count());
      RealType N = static_cast<RealType>(dist.total());
      return n * r  * (N - r) * (N - n) / (N * N * (N - 1));
   } // RealType variance(const hypergeometric_distribution<RealType, Policy>& dist)

   template <class RealType, class Policy>
   inline RealType mode(const hypergeometric_distribution<RealType, Policy>& dist)
   {
      BOOST_MATH_STD_USING
      RealType r = static_cast<RealType>(dist.defective());
      RealType n = static_cast<RealType>(dist.sample_count());
      RealType N = static_cast<RealType>(dist.total());
      return floor((r + 1) * (n + 1) / (N + 2));
   }

   template <class RealType, class Policy>
   inline RealType skewness(const hypergeometric_distribution<RealType, Policy>& dist)
   {
      BOOST_MATH_STD_USING
      RealType r = static_cast<RealType>(dist.defective());
      RealType n = static_cast<RealType>(dist.sample_count());
      RealType N = static_cast<RealType>(dist.total());
      return (N - 2 * r) * sqrt(N - 1) * (N - 2 * n) / (sqrt(n * r * (N - r) * (N - n)) * (N - 2));
   } // RealType skewness(const hypergeometric_distribution<RealType, Policy>& dist)

   template <class RealType, class Policy>
   inline RealType kurtosis_excess(const hypergeometric_distribution<RealType, Policy>& dist)
   {
      // https://www.wolframalpha.com/input/?i=kurtosis+hypergeometric+distribution shown as plain text:
      //  mean | (m n)/N
      //  standard deviation | sqrt((m n(N - m) (N - n))/(N - 1))/N
      //  variance | (m n(1 - m/N) (N - n))/((N - 1) N)
      //  skewness | (sqrt(N - 1) (N - 2 m) (N - 2 n))/((N - 2) sqrt(m n(N - m) (N - n)))
      //  kurtosis | ((N - 1) N^2 ((3 m(N - m) (n^2 (-N) + (n - 2) N^2 + 6 n(N - n)))/N^2 - 6 n(N - n) + N(N + 1)))/(m n(N - 3) (N - 2) (N - m) (N - n))
     // Kurtosis[HypergeometricDistribution[n, m, N]]
      RealType m = static_cast<RealType>(dist.defective()); // Failures or success events. (Also symbols K or M are used).
      RealType n = static_cast<RealType>(dist.sample_count()); // draws or trials.
      RealType n2 = n * n; // n^2
      RealType N = static_cast<RealType>(dist.total()); // Total population from which n draws or trials are made. 
      RealType N2 = N * N; // N^2
      // result = ((N - 1) N^2 ((3 m(N - m) (n^2 (-N) + (n - 2) N^2 + 6 n(N - n)))/N^2 - 6 n(N - n) + N(N + 1)))/(m n(N - 3) (N - 2) (N - m) (N - n));
      RealType result = ((N-1)*N2*((3*m*(N-m)*(n2*(-N)+(n-2)*N2+6*n*(N-n)))/N2-6*n*(N-n)+N*(N+1)))/(m*n*(N-3)*(N-2)*(N-m)*(N-n));
      // Agrees with kurtosis hypergeometric distribution(50,200,500) kurtosis = 2.96917 
      // N[kurtosis[hypergeometricdistribution(50,200,500)], 55]  2.969174035736058474901169623721804275002985337280263464
      return result;
   } // RealType kurtosis_excess(const hypergeometric_distribution<RealType, Policy>& dist)

   template <class RealType, class Policy>
   inline RealType kurtosis(const hypergeometric_distribution<RealType, Policy>& dist)
   {
      return kurtosis_excess(dist) + 3;
   } // RealType kurtosis_excess(const hypergeometric_distribution<RealType, Policy>& dist)
}} // namespaces

// This include must be at the end, *after* the accessors
// for this distribution have been defined, in order to
// keep compilers that support two-phase lookup happy.
#include <boost/math/distributions/detail/derived_accessors.hpp>

#endif // include guard
