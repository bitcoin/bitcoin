//  Copyright John Maddock 2006.
//  Copyright Paul A. Bristow 2006.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// TODO deal with infinity as special better - or remove.
//

#ifndef BOOST_STATS_UNIFORM_HPP
#define BOOST_STATS_UNIFORM_HPP

// http://www.itl.nist.gov/div898/handbook/eda/section3/eda3668.htm
// http://mathworld.wolfram.com/UniformDistribution.html
// http://documents.wolfram.com/calculationcenter/v2/Functions/ListsMatrices/Statistics/UniformDistribution.html
// http://en.wikipedia.org/wiki/Uniform_distribution_%28continuous%29

#include <boost/math/distributions/fwd.hpp>
#include <boost/math/distributions/detail/common_error_handling.hpp>
#include <boost/math/distributions/complement.hpp>

#include <utility>

namespace boost{ namespace math
{
  namespace detail
  {
    template <class RealType, class Policy>
    inline bool check_uniform_lower(
      const char* function,
      RealType lower,
      RealType* result, const Policy& pol)
    {
      if((boost::math::isfinite)(lower))
      { // any finite value is OK.
        return true;
      }
      else
      { // Not finite.
        *result = policies::raise_domain_error<RealType>(
          function,
          "Lower parameter is %1%, but must be finite!", lower, pol);
        return false;
      }
    } // bool check_uniform_lower(

    template <class RealType, class Policy>
    inline bool check_uniform_upper(
      const char* function,
      RealType upper,
      RealType* result, const Policy& pol)
    {
      if((boost::math::isfinite)(upper))
      { // Any finite value is OK.
        return true;
      }
      else
      { // Not finite.
        *result = policies::raise_domain_error<RealType>(
          function,
          "Upper parameter is %1%, but must be finite!", upper, pol);
        return false;
      }
    } // bool check_uniform_upper(

    template <class RealType, class Policy>
    inline bool check_uniform_x(
      const char* function,
      RealType const& x,
      RealType* result, const Policy& pol)
    {
      if((boost::math::isfinite)(x))
      { // Any finite value is OK
        return true;
      }
      else
      { // Not finite..
        *result = policies::raise_domain_error<RealType>(
          function,
          "x parameter is %1%, but must be finite!", x, pol);
        return false;
      }
    } // bool check_uniform_x

    template <class RealType, class Policy>
    inline bool check_uniform(
      const char* function,
      RealType lower,
      RealType upper,
      RealType* result, const Policy& pol)
    {
      if((check_uniform_lower(function, lower, result, pol) == false)
        || (check_uniform_upper(function, upper, result, pol) == false))
      {
        return false;
      }
      else if (lower >= upper) // If lower == upper then 1 / (upper-lower) = 1/0 = +infinity!
      { // upper and lower have been checked before, so must be lower >= upper.
        *result = policies::raise_domain_error<RealType>(
          function,
          "lower parameter is %1%, but must be less than upper!", lower, pol);
        return false;
      }
      else
      { // All OK,
        return true;
      }
    } // bool check_uniform(

  } // namespace detail

  template <class RealType = double, class Policy = policies::policy<> >
  class uniform_distribution
  {
  public:
    typedef RealType value_type;
    typedef Policy policy_type;

    uniform_distribution(RealType l_lower = 0, RealType l_upper = 1) // Constructor.
      : m_lower(l_lower), m_upper(l_upper) // Default is standard uniform distribution.
    {
      RealType result;
      detail::check_uniform("boost::math::uniform_distribution<%1%>::uniform_distribution", l_lower, l_upper, &result, Policy());
    }
    // Accessor functions.
    RealType lower()const
    {
      return m_lower;
    }

    RealType upper()const
    {
      return m_upper;
    }
  private:
    // Data members:
    RealType m_lower;  // distribution lower aka a.
    RealType m_upper;  // distribution upper aka b.
  }; // class uniform_distribution

  typedef uniform_distribution<double> uniform;

  template <class RealType, class Policy>
  inline const std::pair<RealType, RealType> range(const uniform_distribution<RealType, Policy>& /* dist */)
  { // Range of permissible values for random variable x.
     using boost::math::tools::max_value;
     return std::pair<RealType, RealType>(-max_value<RealType>(), max_value<RealType>()); // - to + 'infinity'.
     // Note RealType infinity is NOT permitted, only max_value.
  }

  template <class RealType, class Policy>
  inline const std::pair<RealType, RealType> support(const uniform_distribution<RealType, Policy>& dist)
  { // Range of supported values for random variable x.
     // This is range where cdf rises from 0 to 1, and outside it, the pdf is zero.
     using boost::math::tools::max_value;
     return std::pair<RealType, RealType>(dist.lower(),  dist.upper());
  }

  template <class RealType, class Policy>
  inline RealType pdf(const uniform_distribution<RealType, Policy>& dist, const RealType& x)
  {
    RealType lower = dist.lower();
    RealType upper = dist.upper();
    RealType result = 0; // of checks.
    if(false == detail::check_uniform("boost::math::pdf(const uniform_distribution<%1%>&, %1%)", lower, upper, &result, Policy()))
    {
      return result;
    }
    if(false == detail::check_uniform_x("boost::math::pdf(const uniform_distribution<%1%>&, %1%)", x, &result, Policy()))
    {
      return result;
    }

    if((x < lower) || (x > upper) )
    {
      return 0;
    }
    else
    {
      return 1 / (upper - lower);
    }
  } // RealType pdf(const uniform_distribution<RealType, Policy>& dist, const RealType& x)

  template <class RealType, class Policy>
  inline RealType cdf(const uniform_distribution<RealType, Policy>& dist, const RealType& x)
  {
    RealType lower = dist.lower();
    RealType upper = dist.upper();
    RealType result = 0; // of checks.
    if(false == detail::check_uniform("boost::math::cdf(const uniform_distribution<%1%>&, %1%)",lower, upper, &result, Policy()))
    {
      return result;
    }
    if(false == detail::check_uniform_x("boost::math::cdf(const uniform_distribution<%1%>&, %1%)", x, &result, Policy()))
    {
      return result;
    }
    if (x < lower)
    {
      return 0;
    }
    if (x > upper)
    {
      return 1;
    }
    return (x - lower) / (upper - lower); // lower <= x <= upper
  } // RealType cdf(const uniform_distribution<RealType, Policy>& dist, const RealType& x)

  template <class RealType, class Policy>
  inline RealType quantile(const uniform_distribution<RealType, Policy>& dist, const RealType& p)
  {
    RealType lower = dist.lower();
    RealType upper = dist.upper();
    RealType result = 0; // of checks
    if(false == detail::check_uniform("boost::math::quantile(const uniform_distribution<%1%>&, %1%)",lower, upper, &result, Policy()))
    {
      return result;
    }
    if(false == detail::check_probability("boost::math::quantile(const uniform_distribution<%1%>&, %1%)", p, &result, Policy()))
    {
      return result;
    }
    if(p == 0)
    {
      return lower;
    }
    if(p == 1)
    {
      return upper;
    }
    return p * (upper - lower) + lower;
  } // RealType quantile(const uniform_distribution<RealType, Policy>& dist, const RealType& p)

  template <class RealType, class Policy>
  inline RealType cdf(const complemented2_type<uniform_distribution<RealType, Policy>, RealType>& c)
  {
    RealType lower = c.dist.lower();
    RealType upper = c.dist.upper();
    RealType x = c.param;
    RealType result = 0; // of checks.
    if(false == detail::check_uniform("boost::math::cdf(const uniform_distribution<%1%>&, %1%)", lower, upper, &result, Policy()))
    {
      return result;
    }
    if(false == detail::check_uniform_x("boost::math::cdf(const uniform_distribution<%1%>&, %1%)", x, &result, Policy()))
    {
      return result;
    }
    if (x < lower)
    {
      return 1;
    }
    if (x > upper)
    {
      return 0;
    }
    return (upper - x) / (upper - lower);
  } // RealType cdf(const complemented2_type<uniform_distribution<RealType, Policy>, RealType>& c)

  template <class RealType, class Policy>
  inline RealType quantile(const complemented2_type<uniform_distribution<RealType, Policy>, RealType>& c)
  {
    RealType lower = c.dist.lower();
    RealType upper = c.dist.upper();
    RealType q = c.param;
    RealType result = 0; // of checks.
    if(false == detail::check_uniform("boost::math::quantile(const uniform_distribution<%1%>&, %1%)", lower, upper, &result, Policy()))
    {
      return result;
    }
    if(false == detail::check_probability("boost::math::quantile(const uniform_distribution<%1%>&, %1%)", q, &result, Policy()))
    {
       return result;
    }
    if(q == 0)
    {
       return upper;
    }
    if(q == 1)
    {
       return lower;
    }
    return -q * (upper - lower) + upper;
  } // RealType quantile(const complemented2_type<uniform_distribution<RealType, Policy>, RealType>& c)

  template <class RealType, class Policy>
  inline RealType mean(const uniform_distribution<RealType, Policy>& dist)
  {
    RealType lower = dist.lower();
    RealType upper = dist.upper();
    RealType result = 0;  // of checks.
    if(false == detail::check_uniform("boost::math::mean(const uniform_distribution<%1%>&)", lower, upper, &result, Policy()))
    {
      return result;
    }
    return (lower + upper ) / 2;
  } // RealType mean(const uniform_distribution<RealType, Policy>& dist)

  template <class RealType, class Policy>
  inline RealType variance(const uniform_distribution<RealType, Policy>& dist)
  {
    RealType lower = dist.lower();
    RealType upper = dist.upper();
    RealType result = 0; // of checks.
    if(false == detail::check_uniform("boost::math::variance(const uniform_distribution<%1%>&)", lower, upper, &result, Policy()))
    {
      return result;
    }
    return (upper - lower) * ( upper - lower) / 12;
    // for standard uniform = 0.833333333333333333333333333333333333333333;
  } // RealType variance(const uniform_distribution<RealType, Policy>& dist)

  template <class RealType, class Policy>
  inline RealType mode(const uniform_distribution<RealType, Policy>& dist)
  {
    RealType lower = dist.lower();
    RealType upper = dist.upper();
    RealType result = 0; // of checks.
    if(false == detail::check_uniform("boost::math::mode(const uniform_distribution<%1%>&)", lower, upper, &result, Policy()))
    {
      return result;
    }
    result = lower; // Any value [lower, upper] but arbitrarily choose lower.
    return result;
  }

  template <class RealType, class Policy>
  inline RealType median(const uniform_distribution<RealType, Policy>& dist)
  {
    RealType lower = dist.lower();
    RealType upper = dist.upper();
    RealType result = 0; // of checks.
    if(false == detail::check_uniform("boost::math::median(const uniform_distribution<%1%>&)", lower, upper, &result, Policy()))
    {
      return result;
    }
    return (lower + upper) / 2; //
  }
  template <class RealType, class Policy>
  inline RealType skewness(const uniform_distribution<RealType, Policy>& dist)
  {
    RealType lower = dist.lower();
    RealType upper = dist.upper();
    RealType result = 0; // of checks.
    if(false == detail::check_uniform("boost::math::skewness(const uniform_distribution<%1%>&)",lower, upper, &result, Policy()))
    {
      return result;
    }
    return 0;
  } // RealType skewness(const uniform_distribution<RealType, Policy>& dist)

  template <class RealType, class Policy>
  inline RealType kurtosis_excess(const uniform_distribution<RealType, Policy>& dist)
  {
    RealType lower = dist.lower();
    RealType upper = dist.upper();
    RealType result = 0;  // of checks.
    if(false == detail::check_uniform("boost::math::kurtosis_execess(const uniform_distribution<%1%>&)", lower, upper, &result, Policy()))
    {
      return result;
    }
    return static_cast<RealType>(-6)/5; //  -6/5 = -1.2;
  } // RealType kurtosis_excess(const uniform_distribution<RealType, Policy>& dist)

  template <class RealType, class Policy>
  inline RealType kurtosis(const uniform_distribution<RealType, Policy>& dist)
  {
    return kurtosis_excess(dist) + 3;
  }

  template <class RealType, class Policy>
  inline RealType entropy(const uniform_distribution<RealType, Policy>& dist)
  {
    using std::log;
    return log(dist.upper() - dist.lower());
  }

} // namespace math
} // namespace boost

// This include must be at the end, *after* the accessors
// for this distribution have been defined, in order to
// keep compilers that support two-phase lookup happy.
#include <boost/math/distributions/detail/derived_accessors.hpp>

#endif // BOOST_STATS_UNIFORM_HPP



