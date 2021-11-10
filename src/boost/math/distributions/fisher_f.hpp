// Copyright John Maddock 2006.

// Use, modification and distribution are subject to the
// Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MATH_DISTRIBUTIONS_FISHER_F_HPP
#define BOOST_MATH_DISTRIBUTIONS_FISHER_F_HPP

#include <boost/math/distributions/fwd.hpp>
#include <boost/math/special_functions/beta.hpp> // for incomplete beta.
#include <boost/math/distributions/complement.hpp> // complements
#include <boost/math/distributions/detail/common_error_handling.hpp> // error checks
#include <boost/math/special_functions/fpclassify.hpp>

#include <utility>

namespace boost{ namespace math{

template <class RealType = double, class Policy = policies::policy<> >
class fisher_f_distribution
{
public:
   typedef RealType value_type;
   typedef Policy policy_type;

   fisher_f_distribution(const RealType& i, const RealType& j) : m_df1(i), m_df2(j)
   {
      static const char* function = "fisher_f_distribution<%1%>::fisher_f_distribution";
      RealType result;
      detail::check_df(
         function, m_df1, &result, Policy());
      detail::check_df(
         function, m_df2, &result, Policy());
   } // fisher_f_distribution

   RealType degrees_of_freedom1()const
   {
      return m_df1;
   }
   RealType degrees_of_freedom2()const
   {
      return m_df2;
   }

private:
   //
   // Data members:
   //
   RealType m_df1;  // degrees of freedom are a real number.
   RealType m_df2;  // degrees of freedom are a real number.
};

typedef fisher_f_distribution<double> fisher_f;

template <class RealType, class Policy>
inline const std::pair<RealType, RealType> range(const fisher_f_distribution<RealType, Policy>& /*dist*/)
{ // Range of permissible values for random variable x.
   using boost::math::tools::max_value;
   return std::pair<RealType, RealType>(static_cast<RealType>(0), max_value<RealType>());
}

template <class RealType, class Policy>
inline const std::pair<RealType, RealType> support(const fisher_f_distribution<RealType, Policy>& /*dist*/)
{ // Range of supported values for random variable x.
   // This is range where cdf rises from 0 to 1, and outside it, the pdf is zero.
   using boost::math::tools::max_value;
   return std::pair<RealType, RealType>(static_cast<RealType>(0),  max_value<RealType>());
}

template <class RealType, class Policy>
RealType pdf(const fisher_f_distribution<RealType, Policy>& dist, const RealType& x)
{
   BOOST_MATH_STD_USING  // for ADL of std functions
   RealType df1 = dist.degrees_of_freedom1();
   RealType df2 = dist.degrees_of_freedom2();
   // Error check:
   RealType error_result = 0;
   static const char* function = "boost::math::pdf(fisher_f_distribution<%1%> const&, %1%)";
   if(false == (detail::check_df(
         function, df1, &error_result, Policy())
         && detail::check_df(
         function, df2, &error_result, Policy())))
      return error_result;

   if((x < 0) || !(boost::math::isfinite)(x))
   {
      return policies::raise_domain_error<RealType>(
         function, "Random variable parameter was %1%, but must be > 0 !", x, Policy());
   }

   if(x == 0)
   {
      // special cases:
      if(df1 < 2)
         return policies::raise_overflow_error<RealType>(
            function, 0, Policy());
      else if(df1 == 2)
         return 1;
      else
         return 0;
   }

   //
   // You reach this formula by direct differentiation of the
   // cdf expressed in terms of the incomplete beta.
   //
   // There are two versions so we don't pass a value of z
   // that is very close to 1 to ibeta_derivative: for some values
   // of df1 and df2, all the change takes place in this area.
   //
   RealType v1x = df1 * x;
   RealType result;
   if(v1x > df2)
   {
      result = (df2 * df1) / ((df2 + v1x) * (df2 + v1x));
      result *= ibeta_derivative(df2 / 2, df1 / 2, df2 / (df2 + v1x), Policy());
   }
   else
   {
      result = df2 + df1 * x;
      result = (result * df1 - x * df1 * df1) / (result * result);
      result *= ibeta_derivative(df1 / 2, df2 / 2, v1x / (df2 + v1x), Policy());
   }
   return result;
} // pdf

template <class RealType, class Policy>
inline RealType cdf(const fisher_f_distribution<RealType, Policy>& dist, const RealType& x)
{
   static const char* function = "boost::math::cdf(fisher_f_distribution<%1%> const&, %1%)";
   RealType df1 = dist.degrees_of_freedom1();
   RealType df2 = dist.degrees_of_freedom2();
   // Error check:
   RealType error_result = 0;
   if(false == detail::check_df(
         function, df1, &error_result, Policy())
         && detail::check_df(
         function, df2, &error_result, Policy()))
      return error_result;

   if((x < 0) || !(boost::math::isfinite)(x))
   {
      return policies::raise_domain_error<RealType>(
         function, "Random Variable parameter was %1%, but must be > 0 !", x, Policy());
   }

   RealType v1x = df1 * x;
   //
   // There are two equivalent formulas used here, the aim is
   // to prevent the final argument to the incomplete beta
   // from being too close to 1: for some values of df1 and df2
   // the rate of change can be arbitrarily large in this area,
   // whilst the value we're passing will have lost information
   // content as a result of being 0.999999something.  Better
   // to switch things around so we're passing 1-z instead.
   //
   return v1x > df2
      ? boost::math::ibetac(df2 / 2, df1 / 2, df2 / (df2 + v1x), Policy())
      : boost::math::ibeta(df1 / 2, df2 / 2, v1x / (df2 + v1x), Policy());
} // cdf

template <class RealType, class Policy>
inline RealType quantile(const fisher_f_distribution<RealType, Policy>& dist, const RealType& p)
{
   static const char* function = "boost::math::quantile(fisher_f_distribution<%1%> const&, %1%)";
   RealType df1 = dist.degrees_of_freedom1();
   RealType df2 = dist.degrees_of_freedom2();
   // Error check:
   RealType error_result = 0;
   if(false == (detail::check_df(
            function, df1, &error_result, Policy())
         && detail::check_df(
            function, df2, &error_result, Policy())
         && detail::check_probability(
            function, p, &error_result, Policy())))
      return error_result;

   // With optimizations turned on, gcc wrongly warns about y being used
   // uninitialized unless we initialize it to something:
   RealType x, y(0);

   x = boost::math::ibeta_inv(df1 / 2, df2 / 2, p, &y, Policy());

   return df2 * x / (df1 * y);
} // quantile

template <class RealType, class Policy>
inline RealType cdf(const complemented2_type<fisher_f_distribution<RealType, Policy>, RealType>& c)
{
   static const char* function = "boost::math::cdf(fisher_f_distribution<%1%> const&, %1%)";
   RealType df1 = c.dist.degrees_of_freedom1();
   RealType df2 = c.dist.degrees_of_freedom2();
   RealType x = c.param;
   // Error check:
   RealType error_result = 0;
   if(false == detail::check_df(
         function, df1, &error_result, Policy())
         && detail::check_df(
         function, df2, &error_result, Policy()))
      return error_result;

   if((x < 0) || !(boost::math::isfinite)(x))
   {
      return policies::raise_domain_error<RealType>(
         function, "Random Variable parameter was %1%, but must be > 0 !", x, Policy());
   }

   RealType v1x = df1 * x;
   //
   // There are two equivalent formulas used here, the aim is
   // to prevent the final argument to the incomplete beta
   // from being too close to 1: for some values of df1 and df2
   // the rate of change can be arbitrarily large in this area,
   // whilst the value we're passing will have lost information
   // content as a result of being 0.999999something.  Better
   // to switch things around so we're passing 1-z instead.
   //
   return v1x > df2
      ? boost::math::ibeta(df2 / 2, df1 / 2, df2 / (df2 + v1x), Policy())
      : boost::math::ibetac(df1 / 2, df2 / 2, v1x / (df2 + v1x), Policy());
}

template <class RealType, class Policy>
inline RealType quantile(const complemented2_type<fisher_f_distribution<RealType, Policy>, RealType>& c)
{
   static const char* function = "boost::math::quantile(fisher_f_distribution<%1%> const&, %1%)";
   RealType df1 = c.dist.degrees_of_freedom1();
   RealType df2 = c.dist.degrees_of_freedom2();
   RealType p = c.param;
   // Error check:
   RealType error_result = 0;
   if(false == (detail::check_df(
            function, df1, &error_result, Policy())
         && detail::check_df(
            function, df2, &error_result, Policy())
         && detail::check_probability(
            function, p, &error_result, Policy())))
      return error_result;

   RealType x, y;

   x = boost::math::ibetac_inv(df1 / 2, df2 / 2, p, &y, Policy());

   return df2 * x / (df1 * y);
}

template <class RealType, class Policy>
inline RealType mean(const fisher_f_distribution<RealType, Policy>& dist)
{ // Mean of F distribution = v.
   static const char* function = "boost::math::mean(fisher_f_distribution<%1%> const&)";
   RealType df1 = dist.degrees_of_freedom1();
   RealType df2 = dist.degrees_of_freedom2();
   // Error check:
   RealType error_result = 0;
   if(false == detail::check_df(
            function, df1, &error_result, Policy())
         && detail::check_df(
            function, df2, &error_result, Policy()))
      return error_result;
   if(df2 <= 2)
   {
      return policies::raise_domain_error<RealType>(
         function, "Second degree of freedom was %1% but must be > 2 in order for the distribution to have a mean.", df2, Policy());
   }
   return df2 / (df2 - 2);
} // mean

template <class RealType, class Policy>
inline RealType variance(const fisher_f_distribution<RealType, Policy>& dist)
{ // Variance of F distribution.
   static const char* function = "boost::math::variance(fisher_f_distribution<%1%> const&)";
   RealType df1 = dist.degrees_of_freedom1();
   RealType df2 = dist.degrees_of_freedom2();
   // Error check:
   RealType error_result = 0;
   if(false == detail::check_df(
            function, df1, &error_result, Policy())
         && detail::check_df(
            function, df2, &error_result, Policy()))
      return error_result;
   if(df2 <= 4)
   {
      return policies::raise_domain_error<RealType>(
         function, "Second degree of freedom was %1% but must be > 4 in order for the distribution to have a valid variance.", df2, Policy());
   }
   return 2 * df2 * df2 * (df1 + df2 - 2) / (df1 * (df2 - 2) * (df2 - 2) * (df2 - 4));
} // variance

template <class RealType, class Policy>
inline RealType mode(const fisher_f_distribution<RealType, Policy>& dist)
{
   static const char* function = "boost::math::mode(fisher_f_distribution<%1%> const&)";
   RealType df1 = dist.degrees_of_freedom1();
   RealType df2 = dist.degrees_of_freedom2();
   // Error check:
   RealType error_result = 0;
   if(false == detail::check_df(
            function, df1, &error_result, Policy())
         && detail::check_df(
            function, df2, &error_result, Policy()))
      return error_result;
   if(df2 <= 2)
   {
      return policies::raise_domain_error<RealType>(
         function, "Second degree of freedom was %1% but must be > 2 in order for the distribution to have a mode.", df2, Policy());
   }
   return df2 * (df1 - 2) / (df1 * (df2 + 2));
}

//template <class RealType, class Policy>
//inline RealType median(const fisher_f_distribution<RealType, Policy>& dist)
//{ // Median of Fisher F distribution is not defined.
//  return tools::domain_error<RealType>(BOOST_CURRENT_FUNCTION, "Median is not implemented, result is %1%!", std::numeric_limits<RealType>::quiet_NaN());
//  } // median

// Now implemented via quantile(half) in derived accessors.

template <class RealType, class Policy>
inline RealType skewness(const fisher_f_distribution<RealType, Policy>& dist)
{
   static const char* function = "boost::math::skewness(fisher_f_distribution<%1%> const&)";
   BOOST_MATH_STD_USING // ADL of std names
   // See http://mathworld.wolfram.com/F-Distribution.html
   RealType df1 = dist.degrees_of_freedom1();
   RealType df2 = dist.degrees_of_freedom2();
   // Error check:
   RealType error_result = 0;
   if(false == detail::check_df(
            function, df1, &error_result, Policy())
         && detail::check_df(
            function, df2, &error_result, Policy()))
      return error_result;
   if(df2 <= 6)
   {
      return policies::raise_domain_error<RealType>(
         function, "Second degree of freedom was %1% but must be > 6 in order for the distribution to have a skewness.", df2, Policy());
   }
   return 2 * (df2 + 2 * df1 - 2) * sqrt((2 * df2 - 8) / (df1 * (df2 + df1 - 2))) / (df2 - 6);
}

template <class RealType, class Policy>
RealType kurtosis_excess(const fisher_f_distribution<RealType, Policy>& dist);

template <class RealType, class Policy>
inline RealType kurtosis(const fisher_f_distribution<RealType, Policy>& dist)
{
   return 3 + kurtosis_excess(dist);
}

template <class RealType, class Policy>
inline RealType kurtosis_excess(const fisher_f_distribution<RealType, Policy>& dist)
{
   static const char* function = "boost::math::kurtosis_excess(fisher_f_distribution<%1%> const&)";
   // See http://mathworld.wolfram.com/F-Distribution.html
   RealType df1 = dist.degrees_of_freedom1();
   RealType df2 = dist.degrees_of_freedom2();
   // Error check:
   RealType error_result = 0;
   if(false == detail::check_df(
            function, df1, &error_result, Policy())
         && detail::check_df(
            function, df2, &error_result, Policy()))
      return error_result;
   if(df2 <= 8)
   {
      return policies::raise_domain_error<RealType>(
         function, "Second degree of freedom was %1% but must be > 8 in order for the distribution to have a kurtosis.", df2, Policy());
   }
   RealType df2_2 = df2 * df2;
   RealType df1_2 = df1 * df1;
   RealType n = -16 + 20 * df2 - 8 * df2_2 + df2_2 * df2 + 44 * df1 - 32 * df2 * df1 + 5 * df2_2 * df1 - 22 * df1_2 + 5 * df2 * df1_2;
   n *= 12;
   RealType d = df1 * (df2 - 6) * (df2 - 8) * (df1 + df2 - 2);
   return n / d;
}

} // namespace math
} // namespace boost

// This include must be at the end, *after* the accessors
// for this distribution have been defined, in order to
// keep compilers that support two-phase lookup happy.
#include <boost/math/distributions/detail/derived_accessors.hpp>

#endif // BOOST_MATH_DISTRIBUTIONS_FISHER_F_HPP
