// Kolmogorov-Smirnov 1st order asymptotic distribution
// Copyright Evan Miller 2020
//
// Use, modification and distribution are subject to the
// Boost Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// The Kolmogorov-Smirnov test in statistics compares two empirical distributions,
// or an empirical distribution against any theoretical distribution. It makes
// use of a specific distribution which doesn't have a formal name, but which
// is often called the Kolmogorv-Smirnov distribution for lack of anything
// better. This file implements the limiting form of this distribution, first
// identified by Andrey Kolmogorov in
//
// Kolmogorov, A. (1933) "Sulla Determinazione Empirica di una Legge di
// Distribuzione." Giornale dell' Istituto Italiano degli Attuari
//
// This limiting form of the CDF is a first-order Taylor expansion that is
// easily implemented by the fourth Jacobi Theta function (setting z=0). The
// PDF is then implemented here as a derivative of the Theta function. Note
// that this derivative is with respect to x, which enters into \tau, and not
// with respect to the z argument, which is always zero, and so the derivative
// identities in DLMF 20.4 do not apply here.
//
// A higher order order expansion is possible, and was first outlined by
//
// Pelz W, Good IJ (1976). "Approximating the Lower Tail-Areas of the
// Kolmogorov-Smirnov One-sample Statistic." Journal of the Royal Statistical
// Society B.
//
// The terms in this expansion get fairly complicated, and as far as I know the
// Pelz-Good expansion is not used in any statistics software. Someone could
// consider updating this implementation to use the Pelz-Good expansion in the
// future, but the math gets considerably hairier with each additional term.
//
// A formula for an exact version of the Kolmogorov-Smirnov test is laid out in
// Equation 2.4.4 of
//
// Durbin J (1973). "Distribution Theory for Tests Based on the Sample
// Distribution Func- tion." In SIAM CBMS-NSF Regional Conference Series in
// Applied Mathematics. SIAM, Philadelphia, PA.
//
// which is available in book form from Amazon and others. This exact version
// involves taking powers of large matrices. To do that right you need to
// compute eigenvalues and eigenvectors, which are beyond the scope of Boost.
// (Some recent work indicates the exact form can also be computed via FFT, see
// https://cran.r-project.org/web/packages/KSgeneral/KSgeneral.pdf).
//
// Even if the CDF of the exact distribution could be computed using Boost
// libraries (which would be cumbersome), the PDF would present another
// difficulty. Therefore I am limiting this implementation to the asymptotic
// form, even though the exact form has trivial values for certain specific
// values of x and n. For more on trivial values see
//
// Ruben H, Gambino J (1982). "The Exact Distribution of Kolmogorov's Statistic
// Dn for n <= 10." Annals of the Institute of Statistical Mathematics.
// 
// For a good bibliography and overview of the various algorithms, including
// both exact and asymptotic forms, see
// https://www.jstatsoft.org/article/view/v039i11
//
// As for this implementation: the distribution is parameterized by n (number
// of observations) in the spirit of chi-squared's degrees of freedom. It then
// takes a single argument x. In terms of the Kolmogorov-Smirnov statistical
// test, x represents the distribution of D_n, where D_n is the maximum
// difference between the CDFs being compared, that is,
//
//   D_n = sup|F_n(x) - G(x)|
//
// In the exact distribution, x is confined to the support [0, 1], but in this
// limiting approximation, we allow x to exceed unity (similar to how a normal
// approximation always spills over any boundaries).
//
// As mentioned previously, the CDF is implemented using the \tau
// parameterization of the fourth Jacobi Theta function as
//
// CDF=theta_4(0|2*x*x*n/pi)
//
// The PDF is a hand-coded derivative of that function. Actually, there are two
// (independent) derivatives, as separate code paths are used for "small x"
// (2*x*x*n < pi) and "large x", mirroring the separate code paths in the
// Jacobi Theta implementation to achieve fast convergence. Quantiles are
// computed using a Newton-Raphson iteration from an initial guess that I
// arrived at by trial and error.
//
// The mean and variance are implemented using simple closed-form expressions.
// Skewness and kurtosis use slightly more complicated closed-form expressions
// that involve the zeta function. The mode is calculated at run-time by
// maximizing the PDF. If you have an analytical solution for the mode, feel
// free to plop it in.
//
// The CDF and PDF could almost certainly be re-implemented and sped up using a
// polynomial or rational approximation, since the only meaningful argument is
// x * sqrt(n). But that is left as an exercise for the next maintainer.
//
// In the future, the Pelz-Good approximation could be added. I suggest adding
// a second parameter representing the order, e.g.
//
// kolmogorov_smirnov_dist<>(100) // N=100, order=1
// kolmogorov_smirnov_dist<>(100, 1) // N=100, order=1, i.e. Kolmogorov's formula
// kolmogorov_smirnov_dist<>(100, 4) // N=100, order=4, i.e. Pelz-Good formula
//
// The exact distribution could be added to the API with a special order
// parameter (e.g. 0 or infinity), or a separate distribution type altogether
// (e.g. kolmogorov_smirnov_exact_distribution).
//
#ifndef BOOST_MATH_DISTRIBUTIONS_KOLMOGOROV_SMIRNOV_HPP
#define BOOST_MATH_DISTRIBUTIONS_KOLMOGOROV_SMIRNOV_HPP

#include <boost/math/distributions/fwd.hpp>
#include <boost/math/distributions/complement.hpp>
#include <boost/math/distributions/detail/common_error_handling.hpp>
#include <boost/math/special_functions/jacobi_theta.hpp>
#include <boost/math/tools/tuple.hpp>
#include <boost/math/tools/roots.hpp> // Newton-Raphson
#include <boost/math/tools/minima.hpp> // For the mode

namespace boost { namespace math {

namespace detail {
template <class RealType>
inline RealType kolmogorov_smirnov_quantile_guess(RealType p) {
    // Choose a starting point for the Newton-Raphson iteration
    if (p > 0.9)
        return RealType(1.8) - 5 * (1 - p);
    if (p < 0.3)
        return p + RealType(0.45);
    return p + RealType(0.3);
}

// d/dk (theta2(0, 1/(2*k*k/M_PI))/sqrt(2*k*k*M_PI))
template <class RealType, class Policy>
RealType kolmogorov_smirnov_pdf_small_x(RealType x, RealType n, const Policy&) {
    BOOST_MATH_STD_USING
    RealType value = RealType(0), delta = RealType(0), last_delta = RealType(0);
    RealType eps = policies::get_epsilon<RealType, Policy>();
    int i = 0;
    RealType pi2 = constants::pi_sqr<RealType>();
    RealType x2n = x*x*n;
    if (x2n*x2n == 0.0) {
        return static_cast<RealType>(0);
    }
    while (1) {
        delta = exp(-RealType(i+0.5)*RealType(i+0.5)*pi2/(2*x2n)) * (RealType(i+0.5)*RealType(i+0.5)*pi2 - x2n);

        if (delta == 0.0)
            break;

        if (last_delta != 0.0 && fabs(delta/last_delta) < eps)
            break;

        value += delta + delta;
        last_delta = delta;
        i++;
    }

    return value * sqrt(n) * constants::root_half_pi<RealType>() / (x2n*x2n);
}

// d/dx (theta4(0, 2*x*x*n/M_PI))
template <class RealType, class Policy>
inline RealType kolmogorov_smirnov_pdf_large_x(RealType x, RealType n, const Policy&) {
    BOOST_MATH_STD_USING
    RealType value = RealType(0), delta = RealType(0), last_delta = RealType(0);
    RealType eps = policies::get_epsilon<RealType, Policy>();
    int i = 1;
    while (1) {
        delta = 8*x*i*i*exp(-2*i*i*x*x*n);

        if (delta == 0.0)
            break;

        if (last_delta != 0.0 && fabs(delta / last_delta) < eps)
            break;

        if (i%2 == 0)
            delta = -delta;

        value += delta;
        last_delta = delta;
        i++;
    }

    return value * n;
}

}; // detail

template <class RealType = double, class Policy = policies::policy<> >
    class kolmogorov_smirnov_distribution
{
    public:
        typedef RealType value_type;
        typedef Policy policy_type;

        // Constructor
    kolmogorov_smirnov_distribution( RealType n ) : n_obs_(n)
    {
        RealType result;
        detail::check_df(
                "boost::math::kolmogorov_smirnov_distribution<%1%>::kolmogorov_smirnov_distribution", n_obs_, &result, Policy());
    }

    RealType number_of_observations()const
    {
        return n_obs_;
    }

    private:

    RealType n_obs_; // positive integer
};

typedef kolmogorov_smirnov_distribution<double> kolmogorov_k; // Convenience typedef for double version.

namespace detail {
template <class RealType, class Policy>
struct kolmogorov_smirnov_quantile_functor
{
  kolmogorov_smirnov_quantile_functor(const boost::math::kolmogorov_smirnov_distribution<RealType, Policy> dist, RealType const& p)
    : distribution(dist), prob(p)
  {
  }

  boost::math::tuple<RealType, RealType> operator()(RealType const& x)
  {
    RealType fx = cdf(distribution, x) - prob;  // Difference cdf - value - to minimize.
    RealType dx = pdf(distribution, x); // pdf is 1st derivative.
    // return both function evaluation difference f(x) and 1st derivative f'(x).
    return boost::math::make_tuple(fx, dx);
  }
private:
  const boost::math::kolmogorov_smirnov_distribution<RealType, Policy> distribution;
  RealType prob;
};

template <class RealType, class Policy>
struct kolmogorov_smirnov_complementary_quantile_functor
{
  kolmogorov_smirnov_complementary_quantile_functor(const boost::math::kolmogorov_smirnov_distribution<RealType, Policy> dist, RealType const& p)
    : distribution(dist), prob(p)
  {
  }

  boost::math::tuple<RealType, RealType> operator()(RealType const& x)
  {
    RealType fx = cdf(complement(distribution, x)) - prob;  // Difference cdf - value - to minimize.
    RealType dx = -pdf(distribution, x); // pdf is the negative of the derivative of (1-CDF)
    // return both function evaluation difference f(x) and 1st derivative f'(x).
    return boost::math::make_tuple(fx, dx);
  }
private:
  const boost::math::kolmogorov_smirnov_distribution<RealType, Policy> distribution;
  RealType prob;
};

template <class RealType, class Policy>
struct kolmogorov_smirnov_negative_pdf_functor
{
    RealType operator()(RealType const& x) {
        if (2*x*x < constants::pi<RealType>()) {
            return -kolmogorov_smirnov_pdf_small_x(x, static_cast<RealType>(1), Policy());
        }
        return -kolmogorov_smirnov_pdf_large_x(x, static_cast<RealType>(1), Policy());
    }
};
} // namespace detail

template <class RealType, class Policy>
inline const std::pair<RealType, RealType> range(const kolmogorov_smirnov_distribution<RealType, Policy>& /*dist*/)
{ // Range of permissible values for random variable x.
   using boost::math::tools::max_value;
   return std::pair<RealType, RealType>(static_cast<RealType>(0), max_value<RealType>());
}

template <class RealType, class Policy>
inline const std::pair<RealType, RealType> support(const kolmogorov_smirnov_distribution<RealType, Policy>& /*dist*/)
{ // Range of supported values for random variable x.
   // This is range where cdf rises from 0 to 1, and outside it, the pdf is zero.
   // In the exact distribution, the upper limit would be 1.
   using boost::math::tools::max_value;
   return std::pair<RealType, RealType>(static_cast<RealType>(0), max_value<RealType>());
}

template <class RealType, class Policy>
inline RealType pdf(const kolmogorov_smirnov_distribution<RealType, Policy>& dist, const RealType& x)
{
   BOOST_FPU_EXCEPTION_GUARD
   BOOST_MATH_STD_USING  // for ADL of std functions.

   RealType n = dist.number_of_observations();
   RealType error_result;
   static const char* function = "boost::math::pdf(const kolmogorov_smirnov_distribution<%1%>&, %1%)";
   if(false == detail::check_x_not_NaN(function, x, &error_result, Policy()))
      return error_result;

   if(false == detail::check_df(function, n, &error_result, Policy()))
      return error_result;

   if (x < 0 || !(boost::math::isfinite)(x))
   {
      return policies::raise_domain_error<RealType>(
         function, "Kolmogorov-Smirnov parameter was %1%, but must be > 0 !", x, Policy());
   }

   if (2*x*x*n < constants::pi<RealType>()) {
       return detail::kolmogorov_smirnov_pdf_small_x(x, n, Policy());
   }

   return detail::kolmogorov_smirnov_pdf_large_x(x, n, Policy());
} // pdf

template <class RealType, class Policy>
inline RealType cdf(const kolmogorov_smirnov_distribution<RealType, Policy>& dist, const RealType& x)
{
    BOOST_MATH_STD_USING // for ADL of std function exp.
   static const char* function = "boost::math::cdf(const kolmogorov_smirnov_distribution<%1%>&, %1%)";
   RealType error_result;
   RealType n = dist.number_of_observations();
   if(false == detail::check_x_not_NaN(function, x, &error_result, Policy()))
      return error_result;
   if(false == detail::check_df(function, n, &error_result, Policy()))
      return error_result;
   if((x < 0) || !(boost::math::isfinite)(x)) {
      return policies::raise_domain_error<RealType>(
         function, "Random variable parameter was %1%, but must be between > 0 !", x, Policy());
   }

   if (x*x*n == 0)
       return 0;

   return jacobi_theta4tau(RealType(0), 2*x*x*n/constants::pi<RealType>(), Policy());
} // cdf

template <class RealType, class Policy>
inline RealType cdf(const complemented2_type<kolmogorov_smirnov_distribution<RealType, Policy>, RealType>& c) {
    BOOST_MATH_STD_USING // for ADL of std function exp.
    RealType x = c.param;
   static const char* function = "boost::math::cdf(const complemented2_type<const kolmogorov_smirnov_distribution<%1%>&, %1%>)";
   RealType error_result;
   kolmogorov_smirnov_distribution<RealType, Policy> const& dist = c.dist;
   RealType n = dist.number_of_observations();

   if(false == detail::check_x_not_NaN(function, x, &error_result, Policy()))
      return error_result;
   if(false == detail::check_df(function, n, &error_result, Policy()))
      return error_result;

   if((x < 0) || !(boost::math::isfinite)(x))
      return policies::raise_domain_error<RealType>(
         function, "Random variable parameter was %1%, but must be between > 0 !", x, Policy());

   if (x*x*n == 0)
       return 1;

   if (2*x*x*n > constants::pi<RealType>())
       return -jacobi_theta4m1tau(RealType(0), 2*x*x*n/constants::pi<RealType>(), Policy());

   return RealType(1) - jacobi_theta4tau(RealType(0), 2*x*x*n/constants::pi<RealType>(), Policy());
} // cdf (complemented)

template <class RealType, class Policy>
inline RealType quantile(const kolmogorov_smirnov_distribution<RealType, Policy>& dist, const RealType& p)
{
    BOOST_MATH_STD_USING
   static const char* function = "boost::math::quantile(const kolmogorov_smirnov_distribution<%1%>&, %1%)";
   // Error check:
   RealType error_result;
   RealType n = dist.number_of_observations();
   if(false == detail::check_probability(function, p, &error_result, Policy()))
      return error_result;
   if(false == detail::check_df(function, n, &error_result, Policy()))
      return error_result;

   RealType k = detail::kolmogorov_smirnov_quantile_guess(p) / sqrt(n);
   const int get_digits = policies::digits<RealType, Policy>();// get digits from policy,
   std::uintmax_t m = policies::get_max_root_iterations<Policy>(); // and max iterations.

   return tools::newton_raphson_iterate(detail::kolmogorov_smirnov_quantile_functor<RealType, Policy>(dist, p),
           k, RealType(0), boost::math::tools::max_value<RealType>(), get_digits, m);
} // quantile

template <class RealType, class Policy>
inline RealType quantile(const complemented2_type<kolmogorov_smirnov_distribution<RealType, Policy>, RealType>& c) {
    BOOST_MATH_STD_USING
   static const char* function = "boost::math::quantile(const kolmogorov_smirnov_distribution<%1%>&, %1%)";
   kolmogorov_smirnov_distribution<RealType, Policy> const& dist = c.dist;
   RealType n = dist.number_of_observations();
   // Error check:
   RealType error_result;
   RealType p = c.param;

   if(false == detail::check_probability(function, p, &error_result, Policy()))
      return error_result;
   if(false == detail::check_df(function, n, &error_result, Policy()))
      return error_result;

   RealType k = detail::kolmogorov_smirnov_quantile_guess(RealType(1-p)) / sqrt(n);

   const int get_digits = policies::digits<RealType, Policy>();// get digits from policy,
   std::uintmax_t m = policies::get_max_root_iterations<Policy>(); // and max iterations.

   return tools::newton_raphson_iterate(
           detail::kolmogorov_smirnov_complementary_quantile_functor<RealType, Policy>(dist, p),
           k, RealType(0), boost::math::tools::max_value<RealType>(), get_digits, m);
} // quantile (complemented)

template <class RealType, class Policy>
inline RealType mode(const kolmogorov_smirnov_distribution<RealType, Policy>& dist)
{
    BOOST_MATH_STD_USING
   static const char* function = "boost::math::mode(const kolmogorov_smirnov_distribution<%1%>&)";
   RealType n = dist.number_of_observations();
   RealType error_result;
   if(false == detail::check_df(function, n, &error_result, Policy()))
      return error_result;

    std::pair<RealType, RealType> r = boost::math::tools::brent_find_minima(
            detail::kolmogorov_smirnov_negative_pdf_functor<RealType, Policy>(),
            static_cast<RealType>(0), static_cast<RealType>(1), policies::digits<RealType, Policy>());
    return r.first / sqrt(n);
}

// Mean and variance come directly from
// https://www.jstatsoft.org/article/view/v008i18 Section 3
template <class RealType, class Policy>
inline RealType mean(const kolmogorov_smirnov_distribution<RealType, Policy>& dist)
{
    BOOST_MATH_STD_USING
   static const char* function = "boost::math::mean(const kolmogorov_smirnov_distribution<%1%>&)";
    RealType n = dist.number_of_observations();
    RealType error_result;
    if(false == detail::check_df(function, n, &error_result, Policy()))
        return error_result;
    return constants::root_half_pi<RealType>() * constants::ln_two<RealType>() / sqrt(n);
}

template <class RealType, class Policy>
inline RealType variance(const kolmogorov_smirnov_distribution<RealType, Policy>& dist)
{
   static const char* function = "boost::math::variance(const kolmogorov_smirnov_distribution<%1%>&)";
    RealType n = dist.number_of_observations();
    RealType error_result;
    if(false == detail::check_df(function, n, &error_result, Policy()))
        return error_result;
    return (constants::pi_sqr_div_six<RealType>()
            - constants::pi<RealType>() * constants::ln_two<RealType>() * constants::ln_two<RealType>()) / (2*n);
}

// Skewness and kurtosis come from integrating the PDF
// The alternating series pops out a Dirichlet eta function which is related to the zeta function
template <class RealType, class Policy>
inline RealType skewness(const kolmogorov_smirnov_distribution<RealType, Policy>& dist)
{
    BOOST_MATH_STD_USING
   static const char* function = "boost::math::skewness(const kolmogorov_smirnov_distribution<%1%>&)";
    RealType n = dist.number_of_observations();
    RealType error_result;
    if(false == detail::check_df(function, n, &error_result, Policy()))
        return error_result;
    RealType ex3 = RealType(0.5625) * constants::root_half_pi<RealType>() * constants::zeta_three<RealType>() / n / sqrt(n);
    RealType mean = boost::math::mean(dist);
    RealType var = boost::math::variance(dist);
    return (ex3 - 3 * mean * var - mean * mean * mean) / var / sqrt(var);
}

template <class RealType, class Policy>
inline RealType kurtosis(const kolmogorov_smirnov_distribution<RealType, Policy>& dist)
{
    BOOST_MATH_STD_USING
   static const char* function = "boost::math::kurtosis(const kolmogorov_smirnov_distribution<%1%>&)";
    RealType n = dist.number_of_observations();
    RealType error_result;
    if(false == detail::check_df(function, n, &error_result, Policy()))
        return error_result;
    RealType ex4 = 7 * constants::pi_sqr_div_six<RealType>() * constants::pi_sqr_div_six<RealType>() / 20 / n / n;
    RealType mean = boost::math::mean(dist);
    RealType var = boost::math::variance(dist);
    RealType skew = boost::math::skewness(dist);
    return (ex4 - 4 * mean * skew * var * sqrt(var) - 6 * mean * mean * var - mean * mean * mean * mean) / var / var;
}

template <class RealType, class Policy>
inline RealType kurtosis_excess(const kolmogorov_smirnov_distribution<RealType, Policy>& dist)
{
   static const char* function = "boost::math::kurtosis_excess(const kolmogorov_smirnov_distribution<%1%>&)";
    RealType n = dist.number_of_observations();
    RealType error_result;
    if(false == detail::check_df(function, n, &error_result, Policy()))
        return error_result;
    return kurtosis(dist) - 3;
}
}}
#endif
