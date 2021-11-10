//  Copyright John Maddock 2007.
//  Copyright Paul A. Bristow 2007, 2009
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_STATS_PARETO_HPP
#define BOOST_STATS_PARETO_HPP

// http://en.wikipedia.org/wiki/Pareto_distribution
// http://www.itl.nist.gov/div898/handbook/eda/section3/eda3661.htm
// Also:
// Weisstein, Eric W. "Pareto Distribution."
// From MathWorld--A Wolfram Web Resource.
// http://mathworld.wolfram.com/ParetoDistribution.html
// Handbook of Statistical Distributions with Applications, K Krishnamoorthy, ISBN 1-58488-635-8, Chapter 23, pp 257 - 267.
// Caution KK's a and b are the reverse of Mathworld!

#include <boost/math/distributions/fwd.hpp>
#include <boost/math/distributions/complement.hpp>
#include <boost/math/distributions/detail/common_error_handling.hpp>
#include <boost/math/special_functions/powm1.hpp>

#include <utility> // for BOOST_CURRENT_VALUE?

namespace boost
{
  namespace math
  {
    namespace detail
    { // Parameter checking.
      template <class RealType, class Policy>
      inline bool check_pareto_scale(
        const char* function,
        RealType scale,
        RealType* result, const Policy& pol)
      {
        if((boost::math::isfinite)(scale))
        { // any > 0 finite value is OK.
          if (scale > 0)
          {
            return true;
          }
          else
          {
            *result = policies::raise_domain_error<RealType>(
              function,
              "Scale parameter is %1%, but must be > 0!", scale, pol);
            return false;
          }
        }
        else
        { // Not finite.
          *result = policies::raise_domain_error<RealType>(
            function,
            "Scale parameter is %1%, but must be finite!", scale, pol);
          return false;
        }
      } // bool check_pareto_scale

      template <class RealType, class Policy>
      inline bool check_pareto_shape(
        const char* function,
        RealType shape,
        RealType* result, const Policy& pol)
      {
        if((boost::math::isfinite)(shape))
        { // Any finite value > 0 is OK.
          if (shape > 0)
          {
            return true;
          }
          else
          {
            *result = policies::raise_domain_error<RealType>(
              function,
              "Shape parameter is %1%, but must be > 0!", shape, pol);
            return false;
          }
        }
        else
        { // Not finite.
          *result = policies::raise_domain_error<RealType>(
            function,
            "Shape parameter is %1%, but must be finite!", shape, pol);
          return false;
        }
      } // bool check_pareto_shape(

      template <class RealType, class Policy>
      inline bool check_pareto_x(
        const char* function,
        RealType const& x,
        RealType* result, const Policy& pol)
      {
        if((boost::math::isfinite)(x))
        { //
          if (x > 0)
          {
            return true;
          }
          else
          {
            *result = policies::raise_domain_error<RealType>(
              function,
              "x parameter is %1%, but must be > 0 !", x, pol);
            return false;
          }
        }
        else
        { // Not finite..
          *result = policies::raise_domain_error<RealType>(
            function,
            "x parameter is %1%, but must be finite!", x, pol);
          return false;
        }
      } // bool check_pareto_x

      template <class RealType, class Policy>
      inline bool check_pareto( // distribution parameters.
        const char* function,
        RealType scale,
        RealType shape,
        RealType* result, const Policy& pol)
      {
        return check_pareto_scale(function, scale, result, pol)
           && check_pareto_shape(function, shape, result, pol);
      } // bool check_pareto(

    } // namespace detail

    template <class RealType = double, class Policy = policies::policy<> >
    class pareto_distribution
    {
    public:
      typedef RealType value_type;
      typedef Policy policy_type;

      pareto_distribution(RealType l_scale = 1, RealType l_shape = 1)
        : m_scale(l_scale), m_shape(l_shape)
      { // Constructor.
        RealType result = 0;
        detail::check_pareto("boost::math::pareto_distribution<%1%>::pareto_distribution", l_scale, l_shape, &result, Policy());
      }

      RealType scale()const
      { // AKA Xm and Wolfram b and beta
        return m_scale;
      }

      RealType shape()const
      { // AKA k and Wolfram a and alpha
        return m_shape;
      }
    private:
      // Data members:
      RealType m_scale;  // distribution scale (xm) or beta
      RealType m_shape;  // distribution shape (k) or alpha
    };

    typedef pareto_distribution<double> pareto; // Convenience to allow pareto(2., 3.);

    template <class RealType, class Policy>
    inline const std::pair<RealType, RealType> range(const pareto_distribution<RealType, Policy>& /*dist*/)
    { // Range of permissible values for random variable x.
      using boost::math::tools::max_value;
      return std::pair<RealType, RealType>(static_cast<RealType>(0), max_value<RealType>()); // scale zero to + infinity.
    } // range

    template <class RealType, class Policy>
    inline const std::pair<RealType, RealType> support(const pareto_distribution<RealType, Policy>& dist)
    { // Range of supported values for random variable x.
      // This is range where cdf rises from 0 to 1, and outside it, the pdf is zero.
      using boost::math::tools::max_value;
      return std::pair<RealType, RealType>(dist.scale(), max_value<RealType>() ); // scale to + infinity.
    } // support

    template <class RealType, class Policy>
    inline RealType pdf(const pareto_distribution<RealType, Policy>& dist, const RealType& x)
    {
      BOOST_MATH_STD_USING  // for ADL of std function pow.
      static const char* function = "boost::math::pdf(const pareto_distribution<%1%>&, %1%)";
      RealType scale = dist.scale();
      RealType shape = dist.shape();
      RealType result = 0;
      if(false == (detail::check_pareto_x(function, x, &result, Policy())
         && detail::check_pareto(function, scale, shape, &result, Policy())))
         return result;
      if (x < scale)
      { // regardless of shape, pdf is zero (or should be disallow x < scale and throw an exception?).
        return 0;
      }
      result = shape * pow(scale, shape) / pow(x, shape+1);
      return result;
    } // pdf

    template <class RealType, class Policy>
    inline RealType cdf(const pareto_distribution<RealType, Policy>& dist, const RealType& x)
    {
      BOOST_MATH_STD_USING  // for ADL of std function pow.
      static const char* function = "boost::math::cdf(const pareto_distribution<%1%>&, %1%)";
      RealType scale = dist.scale();
      RealType shape = dist.shape();
      RealType result = 0;

      if(false == (detail::check_pareto_x(function, x, &result, Policy())
         && detail::check_pareto(function, scale, shape, &result, Policy())))
         return result;

      if (x <= scale)
      { // regardless of shape, cdf is zero.
        return 0;
      }

      // result = RealType(1) - pow((scale / x), shape);
      result = -boost::math::powm1(scale/x, shape, Policy()); // should be more accurate.
      return result;
    } // cdf

    template <class RealType, class Policy>
    inline RealType quantile(const pareto_distribution<RealType, Policy>& dist, const RealType& p)
    {
      BOOST_MATH_STD_USING  // for ADL of std function pow.
      static const char* function = "boost::math::quantile(const pareto_distribution<%1%>&, %1%)";
      RealType result = 0;
      RealType scale = dist.scale();
      RealType shape = dist.shape();
      if(false == (detail::check_probability(function, p, &result, Policy())
           && detail::check_pareto(function, scale, shape, &result, Policy())))
      {
        return result;
      }
      if (p == 0)
      {
        return scale; // x must be scale (or less).
      }
      if (p == 1)
      {
        return policies::raise_overflow_error<RealType>(function, 0, Policy()); // x = + infinity.
      }
      result = scale /
        (pow((1 - p), 1 / shape));
      // K. Krishnamoorthy,  ISBN 1-58488-635-8 eq 23.1.3
      return result;
    } // quantile

    template <class RealType, class Policy>
    inline RealType cdf(const complemented2_type<pareto_distribution<RealType, Policy>, RealType>& c)
    {
       BOOST_MATH_STD_USING  // for ADL of std function pow.
       static const char* function = "boost::math::cdf(const pareto_distribution<%1%>&, %1%)";
       RealType result = 0;
       RealType x = c.param;
       RealType scale = c.dist.scale();
       RealType shape = c.dist.shape();
       if(false == (detail::check_pareto_x(function, x, &result, Policy())
           && detail::check_pareto(function, scale, shape, &result, Policy())))
         return result;

       if (x <= scale)
       { // regardless of shape, cdf is zero, and complement is unity.
         return 1;
       }
       result = pow((scale/x), shape);

       return result;
    } // cdf complement

    template <class RealType, class Policy>
    inline RealType quantile(const complemented2_type<pareto_distribution<RealType, Policy>, RealType>& c)
    {
      BOOST_MATH_STD_USING  // for ADL of std function pow.
      static const char* function = "boost::math::quantile(const pareto_distribution<%1%>&, %1%)";
      RealType result = 0;
      RealType q = c.param;
      RealType scale = c.dist.scale();
      RealType shape = c.dist.shape();
      if(false == (detail::check_probability(function, q, &result, Policy())
           && detail::check_pareto(function, scale, shape, &result, Policy())))
      {
        return result;
      }
      if (q == 1)
      {
        return scale; // x must be scale (or less).
      }
      if (q == 0)
      {
         return policies::raise_overflow_error<RealType>(function, 0, Policy()); // x = + infinity.
      }
      result = scale / (pow(q, 1 / shape));
      // K. Krishnamoorthy,  ISBN 1-58488-635-8 eq 23.1.3
      return result;
    } // quantile complement

    template <class RealType, class Policy>
    inline RealType mean(const pareto_distribution<RealType, Policy>& dist)
    {
      RealType result = 0;
      static const char* function = "boost::math::mean(const pareto_distribution<%1%>&, %1%)";
      if(false == detail::check_pareto(function, dist.scale(), dist.shape(), &result, Policy()))
      {
        return result;
      }
      if (dist.shape() > RealType(1))
      {
        return dist.shape() * dist.scale() / (dist.shape() - 1);
      }
      else
      {
        using boost::math::tools::max_value;
        return max_value<RealType>(); // +infinity.
      }
    } // mean

    template <class RealType, class Policy>
    inline RealType mode(const pareto_distribution<RealType, Policy>& dist)
    {
      return dist.scale();
    } // mode

    template <class RealType, class Policy>
    inline RealType median(const pareto_distribution<RealType, Policy>& dist)
    {
      RealType result = 0;
      static const char* function = "boost::math::median(const pareto_distribution<%1%>&, %1%)";
      if(false == detail::check_pareto(function, dist.scale(), dist.shape(), &result, Policy()))
      {
        return result;
      }
      BOOST_MATH_STD_USING
      return dist.scale() * pow(RealType(2), (1/dist.shape()));
    } // median

    template <class RealType, class Policy>
    inline RealType variance(const pareto_distribution<RealType, Policy>& dist)
    {
      RealType result = 0;
      RealType scale = dist.scale();
      RealType shape = dist.shape();
      static const char* function = "boost::math::variance(const pareto_distribution<%1%>&, %1%)";
      if(false == detail::check_pareto(function, scale, shape, &result, Policy()))
      {
        return result;
      }
      if (shape > 2)
      {
        result = (scale * scale * shape) /
         ((shape - 1) *  (shape - 1) * (shape - 2));
      }
      else
      {
        result = policies::raise_domain_error<RealType>(
          function,
          "variance is undefined for shape <= 2, but got %1%.", dist.shape(), Policy());
      }
      return result;
    } // variance

    template <class RealType, class Policy>
    inline RealType skewness(const pareto_distribution<RealType, Policy>& dist)
    {
      BOOST_MATH_STD_USING
      RealType result = 0;
      RealType shape = dist.shape();
      static const char* function = "boost::math::pdf(const pareto_distribution<%1%>&, %1%)";
      if(false == detail::check_pareto(function, dist.scale(), shape, &result, Policy()))
      {
        return result;
      }
      if (shape > 3)
      {
        result = sqrt((shape - 2) / shape) *
          2 * (shape + 1) /
          (shape - 3);
      }
      else
      {
        result = policies::raise_domain_error<RealType>(
          function,
          "skewness is undefined for shape <= 3, but got %1%.", dist.shape(), Policy());
      }
      return result;
    } // skewness

    template <class RealType, class Policy>
    inline RealType kurtosis(const pareto_distribution<RealType, Policy>& dist)
    {
      RealType result = 0;
      RealType shape = dist.shape();
      static const char* function = "boost::math::pdf(const pareto_distribution<%1%>&, %1%)";
      if(false == detail::check_pareto(function, dist.scale(), shape, &result, Policy()))
      {
        return result;
      }
      if (shape > 4)
      {
        result = 3 * ((shape - 2) * (3 * shape * shape + shape + 2)) /
          (shape * (shape - 3) * (shape - 4));
      }
      else
      {
        result = policies::raise_domain_error<RealType>(
          function,
          "kurtosis_excess is undefined for shape <= 4, but got %1%.", shape, Policy());
      }
      return result;
    } // kurtosis

    template <class RealType, class Policy>
    inline RealType kurtosis_excess(const pareto_distribution<RealType, Policy>& dist)
    {
      RealType result = 0;
      RealType shape = dist.shape();
      static const char* function = "boost::math::pdf(const pareto_distribution<%1%>&, %1%)";
      if(false == detail::check_pareto(function, dist.scale(), shape, &result, Policy()))
      {
        return result;
      }
      if (shape > 4)
      {
        result = 6 * ((shape * shape * shape) + (shape * shape) - 6 * shape - 2) /
          (shape * (shape - 3) * (shape - 4));
      }
      else
      {
        result = policies::raise_domain_error<RealType>(
          function,
          "kurtosis_excess is undefined for shape <= 4, but got %1%.", dist.shape(), Policy());
      }
      return result;
    } // kurtosis_excess

    template <class RealType, class Policy>
    inline RealType entropy(const pareto_distribution<RealType, Policy>& dist)
    {
      using std::log;
      RealType xm = dist.scale();
      RealType alpha = dist.shape();
      return log(xm/alpha) + 1 + 1/alpha;
    }

    } // namespace math
  } // namespace boost

  // This include must be at the end, *after* the accessors
  // for this distribution have been defined, in order to
  // keep compilers that support two-phase lookup happy.
#include <boost/math/distributions/detail/derived_accessors.hpp>

#endif // BOOST_STATS_PARETO_HPP


