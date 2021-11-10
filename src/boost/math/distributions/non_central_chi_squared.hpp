// boost\math\distributions\non_central_chi_squared.hpp

// Copyright John Maddock 2008.

// Use, modification and distribution are subject to the
// Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MATH_SPECIAL_NON_CENTRAL_CHI_SQUARE_HPP
#define BOOST_MATH_SPECIAL_NON_CENTRAL_CHI_SQUARE_HPP

#include <boost/math/distributions/fwd.hpp>
#include <boost/math/special_functions/gamma.hpp> // for incomplete gamma. gamma_q
#include <boost/math/special_functions/bessel.hpp> // for cyl_bessel_i
#include <boost/math/special_functions/round.hpp> // for iround
#include <boost/math/distributions/complement.hpp> // complements
#include <boost/math/distributions/chi_squared.hpp> // central distribution
#include <boost/math/distributions/detail/common_error_handling.hpp> // error checks
#include <boost/math/special_functions/fpclassify.hpp> // isnan.
#include <boost/math/tools/roots.hpp> // for root finding.
#include <boost/math/distributions/detail/generic_mode.hpp>
#include <boost/math/distributions/detail/generic_quantile.hpp>

namespace boost
{
   namespace math
   {

      template <class RealType, class Policy>
      class non_central_chi_squared_distribution;

      namespace detail{

         template <class T, class Policy>
         T non_central_chi_square_q(T x, T f, T theta, const Policy& pol, T init_sum = 0)
         {
            //
            // Computes the complement of the Non-Central Chi-Square
            // Distribution CDF by summing a weighted sum of complements
            // of the central-distributions.  The weighting factor is
            // a Poisson Distribution.
            //
            // This is an application of the technique described in:
            //
            // Computing discrete mixtures of continuous
            // distributions: noncentral chisquare, noncentral t
            // and the distribution of the square of the sample
            // multiple correlation coefficient.
            // D. Benton, K. Krishnamoorthy.
            // Computational Statistics & Data Analysis 43 (2003) 249 - 267
            //
            BOOST_MATH_STD_USING

            // Special case:
            if(x == 0)
               return 1;

            //
            // Initialize the variables we'll be using:
            //
            T lambda = theta / 2;
            T del = f / 2;
            T y = x / 2;
            std::uintmax_t max_iter = policies::get_max_series_iterations<Policy>();
            T errtol = boost::math::policies::get_epsilon<T, Policy>();
            T sum = init_sum;
            //
            // k is the starting location for iteration, we'll
            // move both forwards and backwards from this point.
            // k is chosen as the peek of the Poisson weights, which
            // will occur *before* the largest term.
            //
            int k = iround(lambda, pol);
            // Forwards and backwards Poisson weights:
            T poisf = boost::math::gamma_p_derivative(static_cast<T>(1 + k), lambda, pol);
            T poisb = poisf * k / lambda;
            // Initial forwards central chi squared term:
            T gamf = boost::math::gamma_q(del + k, y, pol);
            // Forwards and backwards recursion terms on the central chi squared:
            T xtermf = boost::math::gamma_p_derivative(del + 1 + k, y, pol);
            T xtermb = xtermf * (del + k) / y;
            // Initial backwards central chi squared term:
            T gamb = gamf - xtermb;

            //
            // Forwards iteration first, this is the
            // stable direction for the gamma function
            // recurrences:
            //
            int i;
            for(i = k; static_cast<std::uintmax_t>(i-k) < max_iter; ++i)
            {
               T term = poisf * gamf;
               sum += term;
               poisf *= lambda / (i + 1);
               gamf += xtermf;
               xtermf *= y / (del + i + 1);
               if(((sum == 0) || (fabs(term / sum) < errtol)) && (term >= poisf * gamf))
                  break;
            }
            //Error check:
            if(static_cast<std::uintmax_t>(i-k) >= max_iter)
               return policies::raise_evaluation_error(
                  "cdf(non_central_chi_squared_distribution<%1%>, %1%)",
                  "Series did not converge, closest value was %1%", sum, pol);
            //
            // Now backwards iteration: the gamma
            // function recurrences are unstable in this
            // direction, we rely on the terms diminishing in size
            // faster than we introduce cancellation errors.
            // For this reason it's very important that we start
            // *before* the largest term so that backwards iteration
            // is strictly converging.
            //
            for(i = k - 1; i >= 0; --i)
            {
               T term = poisb * gamb;
               sum += term;
               poisb *= i / lambda;
               xtermb *= (del + i) / y;
               gamb -= xtermb;
               if((sum == 0) || (fabs(term / sum) < errtol))
                  break;
            }

            return sum;
         }

         template <class T, class Policy>
         T non_central_chi_square_p_ding(T x, T f, T theta, const Policy& pol, T init_sum = 0)
         {
            //
            // This is an implementation of:
            //
            // Algorithm AS 275:
            // Computing the Non-Central #2 Distribution Function
            // Cherng G. Ding
            // Applied Statistics, Vol. 41, No. 2. (1992), pp. 478-482.
            //
            // This uses a stable forward iteration to sum the
            // CDF, unfortunately this can not be used for large
            // values of the non-centrality parameter because:
            // * The first term may underflow to zero.
            // * We may need an extra-ordinary number of terms
            //   before we reach the first *significant* term.
            //
            BOOST_MATH_STD_USING
            // Special case:
            if(x == 0)
               return 0;
            T tk = boost::math::gamma_p_derivative(f/2 + 1, x/2, pol);
            T lambda = theta / 2;
            T vk = exp(-lambda);
            T uk = vk;
            T sum = init_sum + tk * vk;
            if(sum == 0)
               return sum;

            std::uintmax_t max_iter = policies::get_max_series_iterations<Policy>();
            T errtol = boost::math::policies::get_epsilon<T, Policy>();

            int i;
            T lterm(0), term(0);
            for(i = 1; static_cast<std::uintmax_t>(i) < max_iter; ++i)
            {
               tk = tk * x / (f + 2 * i);
               uk = uk * lambda / i;
               vk = vk + uk;
               lterm = term;
               term = vk * tk;
               sum += term;
               if((fabs(term / sum) < errtol) && (term <= lterm))
                  break;
            }
            //Error check:
            if(static_cast<std::uintmax_t>(i) >= max_iter)
               return policies::raise_evaluation_error(
                  "cdf(non_central_chi_squared_distribution<%1%>, %1%)",
                  "Series did not converge, closest value was %1%", sum, pol);
            return sum;
         }


         template <class T, class Policy>
         T non_central_chi_square_p(T y, T n, T lambda, const Policy& pol, T init_sum)
         {
            //
            // This is taken more or less directly from:
            //
            // Computing discrete mixtures of continuous
            // distributions: noncentral chisquare, noncentral t
            // and the distribution of the square of the sample
            // multiple correlation coefficient.
            // D. Benton, K. Krishnamoorthy.
            // Computational Statistics & Data Analysis 43 (2003) 249 - 267
            //
            // We're summing a Poisson weighting term multiplied by
            // a central chi squared distribution.
            //
            BOOST_MATH_STD_USING
            // Special case:
            if(y == 0)
               return 0;
            std::uintmax_t max_iter = policies::get_max_series_iterations<Policy>();
            T errtol = boost::math::policies::get_epsilon<T, Policy>();
            T errorf(0), errorb(0);

            T x = y / 2;
            T del = lambda / 2;
            //
            // Starting location for the iteration, we'll iterate
            // both forwards and backwards from this point.  The
            // location chosen is the maximum of the Poisson weight
            // function, which ocurrs *after* the largest term in the
            // sum.
            //
            int k = iround(del, pol);
            T a = n / 2 + k;
            // Central chi squared term for forward iteration:
            T gamkf = boost::math::gamma_p(a, x, pol);

            if(lambda == 0)
               return gamkf;
            // Central chi squared term for backward iteration:
            T gamkb = gamkf;
            // Forwards Poisson weight:
            T poiskf = gamma_p_derivative(static_cast<T>(k+1), del, pol);
            // Backwards Poisson weight:
            T poiskb = poiskf;
            // Forwards gamma function recursion term:
            T xtermf = boost::math::gamma_p_derivative(a, x, pol);
            // Backwards gamma function recursion term:
            T xtermb = xtermf * x / a;
            T sum = init_sum + poiskf * gamkf;
            if(sum == 0)
               return sum;
            int i = 1;
            //
            // Backwards recursion first, this is the stable
            // direction for gamma function recurrences:
            //
            while(i <= k)
            {
               xtermb *= (a - i + 1) / x;
               gamkb += xtermb;
               poiskb = poiskb * (k - i + 1) / del;
               errorf = errorb;
               errorb = gamkb * poiskb;
               sum += errorb;
               if((fabs(errorb / sum) < errtol) && (errorb <= errorf))
                  break;
               ++i;
            }
            i = 1;
            //
            // Now forwards recursion, the gamma function
            // recurrence relation is unstable in this direction,
            // so we rely on the magnitude of successive terms
            // decreasing faster than we introduce cancellation error.
            // For this reason it's vital that k is chosen to be *after*
            // the largest term, so that successive forward iterations
            // are strictly (and rapidly) converging.
            //
            do
            {
               xtermf = xtermf * x / (a + i - 1);
               gamkf = gamkf - xtermf;
               poiskf = poiskf * del / (k + i);
               errorf = poiskf * gamkf;
               sum += errorf;
               ++i;
            }while((fabs(errorf / sum) > errtol) && (static_cast<std::uintmax_t>(i) < max_iter));

            //Error check:
            if(static_cast<std::uintmax_t>(i) >= max_iter)
               return policies::raise_evaluation_error(
                  "cdf(non_central_chi_squared_distribution<%1%>, %1%)",
                  "Series did not converge, closest value was %1%", sum, pol);

            return sum;
         }

         template <class T, class Policy>
         T non_central_chi_square_pdf(T x, T n, T lambda, const Policy& pol)
         {
            //
            // As above but for the PDF:
            //
            BOOST_MATH_STD_USING
            std::uintmax_t max_iter = policies::get_max_series_iterations<Policy>();
            T errtol = boost::math::policies::get_epsilon<T, Policy>();
            T x2 = x / 2;
            T n2 = n / 2;
            T l2 = lambda / 2;
            T sum = 0;
            int k = itrunc(l2);
            T pois = gamma_p_derivative(static_cast<T>(k + 1), l2, pol) * gamma_p_derivative(static_cast<T>(n2 + k), x2);
            if(pois == 0)
               return 0;
            T poisb = pois;
            for(int i = k; ; ++i)
            {
               sum += pois;
               if(pois / sum < errtol)
                  break;
               if(static_cast<std::uintmax_t>(i - k) >= max_iter)
                  return policies::raise_evaluation_error(
                     "pdf(non_central_chi_squared_distribution<%1%>, %1%)",
                     "Series did not converge, closest value was %1%", sum, pol);
               pois *= l2 * x2 / ((i + 1) * (n2 + i));
            }
            for(int i = k - 1; i >= 0; --i)
            {
               poisb *= (i + 1) * (n2 + i) / (l2 * x2);
               sum += poisb;
               if(poisb / sum < errtol)
                  break;
            }
            return sum / 2;
         }

         template <class RealType, class Policy>
         inline RealType non_central_chi_squared_cdf(RealType x, RealType k, RealType l, bool invert, const Policy&)
         {
            typedef typename policies::evaluation<RealType, Policy>::type value_type;
            typedef typename policies::normalise<
               Policy,
               policies::promote_float<false>,
               policies::promote_double<false>,
               policies::discrete_quantile<>,
               policies::assert_undefined<> >::type forwarding_policy;

            BOOST_MATH_STD_USING
            value_type result;
            if(l == 0)
              return invert == false ? cdf(boost::math::chi_squared_distribution<RealType, Policy>(k), x) : cdf(complement(boost::math::chi_squared_distribution<RealType, Policy>(k), x));
            else if(x > k + l)
            {
               // Complement is the smaller of the two:
               result = detail::non_central_chi_square_q(
                  static_cast<value_type>(x),
                  static_cast<value_type>(k),
                  static_cast<value_type>(l),
                  forwarding_policy(),
                  static_cast<value_type>(invert ? 0 : -1));
               invert = !invert;
            }
            else if(l < 200)
            {
               // For small values of the non-centrality parameter
               // we can use Ding's method:
               result = detail::non_central_chi_square_p_ding(
                  static_cast<value_type>(x),
                  static_cast<value_type>(k),
                  static_cast<value_type>(l),
                  forwarding_policy(),
                  static_cast<value_type>(invert ? -1 : 0));
            }
            else
            {
               // For largers values of the non-centrality
               // parameter Ding's method will consume an
               // extra-ordinary number of terms, and worse
               // may return zero when the result is in fact
               // finite, use Krishnamoorthy's method instead:
               result = detail::non_central_chi_square_p(
                  static_cast<value_type>(x),
                  static_cast<value_type>(k),
                  static_cast<value_type>(l),
                  forwarding_policy(),
                  static_cast<value_type>(invert ? -1 : 0));
            }
            if(invert)
               result = -result;
            return policies::checked_narrowing_cast<RealType, forwarding_policy>(
               result,
               "boost::math::non_central_chi_squared_cdf<%1%>(%1%, %1%, %1%)");
         }

         template <class T, class Policy>
         struct nccs_quantile_functor
         {
            nccs_quantile_functor(const non_central_chi_squared_distribution<T,Policy>& d, T t, bool c)
               : dist(d), target(t), comp(c) {}

            T operator()(const T& x)
            {
               return comp ?
                  target - cdf(complement(dist, x))
                  : cdf(dist, x) - target;
            }

         private:
            non_central_chi_squared_distribution<T,Policy> dist;
            T target;
            bool comp;
         };

         template <class RealType, class Policy>
         RealType nccs_quantile(const non_central_chi_squared_distribution<RealType, Policy>& dist, const RealType& p, bool comp)
         {
            BOOST_MATH_STD_USING
            static const char* function = "quantile(non_central_chi_squared_distribution<%1%>, %1%)";
            typedef typename policies::evaluation<RealType, Policy>::type value_type;
            typedef typename policies::normalise<
               Policy,
               policies::promote_float<false>,
               policies::promote_double<false>,
               policies::discrete_quantile<>,
               policies::assert_undefined<> >::type forwarding_policy;

            value_type k = dist.degrees_of_freedom();
            value_type l = dist.non_centrality();
            value_type r;
            if(!detail::check_df(
               function,
               k, &r, Policy())
               ||
            !detail::check_non_centrality(
               function,
               l,
               &r,
               Policy())
               ||
            !detail::check_probability(
               function,
               static_cast<value_type>(p),
               &r,
               Policy()))
                  return (RealType)r;
            //
            // Special cases get short-circuited first:
            //
            if(p == 0)
               return comp ? policies::raise_overflow_error<RealType>(function, 0, Policy()) : 0;
            if(p == 1)
               return comp ? 0 : policies::raise_overflow_error<RealType>(function, 0, Policy());
            //
            // This is Pearson's approximation to the quantile, see
            // Pearson, E. S. (1959) "Note on an approximation to the distribution of
            // noncentral chi squared", Biometrika 46: 364.
            // See also:
            // "A comparison of approximations to percentiles of the noncentral chi2-distribution",
            // Hardeo Sahai and Mario Miguel Ojeda, Revista de Matematica: Teoria y Aplicaciones 2003 10(1-2) : 57-76.
            // Note that the latter reference refers to an approximation of the CDF, when they really mean the quantile.
            //
            value_type b = -(l * l) / (k + 3 * l);
            value_type c = (k + 3 * l) / (k + 2 * l);
            value_type ff = (k + 2 * l) / (c * c);
            value_type guess;
            if(comp)
            {
               guess = b + c * quantile(complement(chi_squared_distribution<value_type, forwarding_policy>(ff), p));
            }
            else
            {
               guess = b + c * quantile(chi_squared_distribution<value_type, forwarding_policy>(ff), p);
            }
            //
            // Sometimes guess goes very small or negative, in that case we have
            // to do something else for the initial guess, this approximation
            // was provided in a private communication from Thomas Luu, PhD candidate,
            // University College London.  It's an asymptotic expansion for the
            // quantile which usually gets us within an order of magnitude of the
            // correct answer.
            // Fast and accurate parallel computation of quantile functions for random number generation,
            // Thomas LuuDoctorial Thesis 2016
            // http://discovery.ucl.ac.uk/1482128/
            //
            if(guess < 0.005)
            {
               value_type pp = comp ? 1 - p : p;
               //guess = pow(pow(value_type(2), (k / 2 - 1)) * exp(l / 2) * pp * k, 2 / k);
               guess = pow(pow(value_type(2), (k / 2 - 1)) * exp(l / 2) * pp * k * boost::math::tgamma(k / 2, forwarding_policy()), (2 / k));
               if(guess == 0)
                  guess = tools::min_value<value_type>();
            }
            value_type result = detail::generic_quantile(
               non_central_chi_squared_distribution<value_type, forwarding_policy>(k, l),
               p,
               guess,
               comp,
               function);

            return policies::checked_narrowing_cast<RealType, forwarding_policy>(
               result,
               function);
         }

         template <class RealType, class Policy>
         RealType nccs_pdf(const non_central_chi_squared_distribution<RealType, Policy>& dist, const RealType& x)
         {
            BOOST_MATH_STD_USING
            static const char* function = "pdf(non_central_chi_squared_distribution<%1%>, %1%)";
            typedef typename policies::evaluation<RealType, Policy>::type value_type;
            typedef typename policies::normalise<
               Policy,
               policies::promote_float<false>,
               policies::promote_double<false>,
               policies::discrete_quantile<>,
               policies::assert_undefined<> >::type forwarding_policy;

            value_type k = dist.degrees_of_freedom();
            value_type l = dist.non_centrality();
            value_type r;
            if(!detail::check_df(
               function,
               k, &r, Policy())
               ||
            !detail::check_non_centrality(
               function,
               l,
               &r,
               Policy())
               ||
            !detail::check_positive_x(
               function,
               (value_type)x,
               &r,
               Policy()))
                  return (RealType)r;

         if(l == 0)
            return pdf(boost::math::chi_squared_distribution<RealType, forwarding_policy>(dist.degrees_of_freedom()), x);

         // Special case:
         if(x == 0)
            return 0;
         if(l > 50)
         {
            r = non_central_chi_square_pdf(static_cast<value_type>(x), k, l, forwarding_policy());
         }
         else
         {
            r = log(x / l) * (k / 4 - 0.5f) - (x + l) / 2;
            if(fabs(r) >= tools::log_max_value<RealType>() / 4)
            {
               r = non_central_chi_square_pdf(static_cast<value_type>(x), k, l, forwarding_policy());
            }
            else
            {
               r = exp(r);
               r = 0.5f * r
                  * boost::math::cyl_bessel_i(k/2 - 1, sqrt(l * x), forwarding_policy());
            }
         }
         return policies::checked_narrowing_cast<RealType, forwarding_policy>(
               r,
               function);
         }

         template <class RealType, class Policy>
         struct degrees_of_freedom_finder
         {
            degrees_of_freedom_finder(
               RealType lam_, RealType x_, RealType p_, bool c)
               : lam(lam_), x(x_), p(p_), comp(c) {}

            RealType operator()(const RealType& v)
            {
               non_central_chi_squared_distribution<RealType, Policy> d(v, lam);
               return comp ?
                  RealType(p - cdf(complement(d, x)))
                  : RealType(cdf(d, x) - p);
            }
         private:
            RealType lam;
            RealType x;
            RealType p;
            bool comp;
         };

         template <class RealType, class Policy>
         inline RealType find_degrees_of_freedom(
            RealType lam, RealType x, RealType p, RealType q, const Policy& pol)
         {
            const char* function = "non_central_chi_squared<%1%>::find_degrees_of_freedom";
            if((p == 0) || (q == 0))
            {
               //
               // Can't a thing if one of p and q is zero:
               //
               return policies::raise_evaluation_error<RealType>(function,
                  "Can't find degrees of freedom when the probability is 0 or 1, only possible answer is %1%",
                  RealType(std::numeric_limits<RealType>::quiet_NaN()), Policy());
            }
            degrees_of_freedom_finder<RealType, Policy> f(lam, x, p < q ? p : q, p < q ? false : true);
            tools::eps_tolerance<RealType> tol(policies::digits<RealType, Policy>());
            std::uintmax_t max_iter = policies::get_max_root_iterations<Policy>();
            //
            // Pick an initial guess that we know will give us a probability
            // right around 0.5.
            //
            RealType guess = x - lam;
            if(guess < 1)
               guess = 1;
            std::pair<RealType, RealType> ir = tools::bracket_and_solve_root(
               f, guess, RealType(2), false, tol, max_iter, pol);
            RealType result = ir.first + (ir.second - ir.first) / 2;
            if(max_iter >= policies::get_max_root_iterations<Policy>())
            {
               return policies::raise_evaluation_error<RealType>(function, "Unable to locate solution in a reasonable time:"
                  " or there is no answer to problem.  Current best guess is %1%", result, Policy());
            }
            return result;
         }

         template <class RealType, class Policy>
         struct non_centrality_finder
         {
            non_centrality_finder(
               RealType v_, RealType x_, RealType p_, bool c)
               : v(v_), x(x_), p(p_), comp(c) {}

            RealType operator()(const RealType& lam)
            {
               non_central_chi_squared_distribution<RealType, Policy> d(v, lam);
               return comp ?
                  RealType(p - cdf(complement(d, x)))
                  : RealType(cdf(d, x) - p);
            }
         private:
            RealType v;
            RealType x;
            RealType p;
            bool comp;
         };

         template <class RealType, class Policy>
         inline RealType find_non_centrality(
            RealType v, RealType x, RealType p, RealType q, const Policy& pol)
         {
            const char* function = "non_central_chi_squared<%1%>::find_non_centrality";
            if((p == 0) || (q == 0))
            {
               //
               // Can't do a thing if one of p and q is zero:
               //
               return policies::raise_evaluation_error<RealType>(function,
                  "Can't find non centrality parameter when the probability is 0 or 1, only possible answer is %1%",
                  RealType(std::numeric_limits<RealType>::quiet_NaN()), Policy());
            }
            non_centrality_finder<RealType, Policy> f(v, x, p < q ? p : q, p < q ? false : true);
            tools::eps_tolerance<RealType> tol(policies::digits<RealType, Policy>());
            std::uintmax_t max_iter = policies::get_max_root_iterations<Policy>();
            //
            // Pick an initial guess that we know will give us a probability
            // right around 0.5.
            //
            RealType guess = x - v;
            if(guess < 1)
               guess = 1;
            std::pair<RealType, RealType> ir = tools::bracket_and_solve_root(
               f, guess, RealType(2), false, tol, max_iter, pol);
            RealType result = ir.first + (ir.second - ir.first) / 2;
            if(max_iter >= policies::get_max_root_iterations<Policy>())
            {
               return policies::raise_evaluation_error<RealType>(function, "Unable to locate solution in a reasonable time:"
                  " or there is no answer to problem.  Current best guess is %1%", result, Policy());
            }
            return result;
         }

      }

      template <class RealType = double, class Policy = policies::policy<> >
      class non_central_chi_squared_distribution
      {
      public:
         typedef RealType value_type;
         typedef Policy policy_type;

         non_central_chi_squared_distribution(RealType df_, RealType lambda) : df(df_), ncp(lambda)
         {
            const char* function = "boost::math::non_central_chi_squared_distribution<%1%>::non_central_chi_squared_distribution(%1%,%1%)";
            RealType r;
            detail::check_df(
               function,
               df, &r, Policy());
            detail::check_non_centrality(
               function,
               ncp,
               &r,
               Policy());
         } // non_central_chi_squared_distribution constructor.

         RealType degrees_of_freedom() const
         { // Private data getter function.
            return df;
         }
         RealType non_centrality() const
         { // Private data getter function.
            return ncp;
         }
         static RealType find_degrees_of_freedom(RealType lam, RealType x, RealType p)
         {
            const char* function = "non_central_chi_squared<%1%>::find_degrees_of_freedom";
            typedef typename policies::evaluation<RealType, Policy>::type eval_type;
            typedef typename policies::normalise<
               Policy,
               policies::promote_float<false>,
               policies::promote_double<false>,
               policies::discrete_quantile<>,
               policies::assert_undefined<> >::type forwarding_policy;
            eval_type result = detail::find_degrees_of_freedom(
               static_cast<eval_type>(lam),
               static_cast<eval_type>(x),
               static_cast<eval_type>(p),
               static_cast<eval_type>(1-p),
               forwarding_policy());
            return policies::checked_narrowing_cast<RealType, forwarding_policy>(
               result,
               function);
         }
         template <class A, class B, class C>
         static RealType find_degrees_of_freedom(const complemented3_type<A,B,C>& c)
         {
            const char* function = "non_central_chi_squared<%1%>::find_degrees_of_freedom";
            typedef typename policies::evaluation<RealType, Policy>::type eval_type;
            typedef typename policies::normalise<
               Policy,
               policies::promote_float<false>,
               policies::promote_double<false>,
               policies::discrete_quantile<>,
               policies::assert_undefined<> >::type forwarding_policy;
            eval_type result = detail::find_degrees_of_freedom(
               static_cast<eval_type>(c.dist),
               static_cast<eval_type>(c.param1),
               static_cast<eval_type>(1-c.param2),
               static_cast<eval_type>(c.param2),
               forwarding_policy());
            return policies::checked_narrowing_cast<RealType, forwarding_policy>(
               result,
               function);
         }
         static RealType find_non_centrality(RealType v, RealType x, RealType p)
         {
            const char* function = "non_central_chi_squared<%1%>::find_non_centrality";
            typedef typename policies::evaluation<RealType, Policy>::type eval_type;
            typedef typename policies::normalise<
               Policy,
               policies::promote_float<false>,
               policies::promote_double<false>,
               policies::discrete_quantile<>,
               policies::assert_undefined<> >::type forwarding_policy;
            eval_type result = detail::find_non_centrality(
               static_cast<eval_type>(v),
               static_cast<eval_type>(x),
               static_cast<eval_type>(p),
               static_cast<eval_type>(1-p),
               forwarding_policy());
            return policies::checked_narrowing_cast<RealType, forwarding_policy>(
               result,
               function);
         }
         template <class A, class B, class C>
         static RealType find_non_centrality(const complemented3_type<A,B,C>& c)
         {
            const char* function = "non_central_chi_squared<%1%>::find_non_centrality";
            typedef typename policies::evaluation<RealType, Policy>::type eval_type;
            typedef typename policies::normalise<
               Policy,
               policies::promote_float<false>,
               policies::promote_double<false>,
               policies::discrete_quantile<>,
               policies::assert_undefined<> >::type forwarding_policy;
            eval_type result = detail::find_non_centrality(
               static_cast<eval_type>(c.dist),
               static_cast<eval_type>(c.param1),
               static_cast<eval_type>(1-c.param2),
               static_cast<eval_type>(c.param2),
               forwarding_policy());
            return policies::checked_narrowing_cast<RealType, forwarding_policy>(
               result,
               function);
         }
      private:
         // Data member, initialized by constructor.
         RealType df; // degrees of freedom.
         RealType ncp; // non-centrality parameter
      }; // template <class RealType, class Policy> class non_central_chi_squared_distribution

      typedef non_central_chi_squared_distribution<double> non_central_chi_squared; // Reserved name of type double.

      // Non-member functions to give properties of the distribution.

      template <class RealType, class Policy>
      inline const std::pair<RealType, RealType> range(const non_central_chi_squared_distribution<RealType, Policy>& /* dist */)
      { // Range of permissible values for random variable k.
         using boost::math::tools::max_value;
         return std::pair<RealType, RealType>(static_cast<RealType>(0), max_value<RealType>()); // Max integer?
      }

      template <class RealType, class Policy>
      inline const std::pair<RealType, RealType> support(const non_central_chi_squared_distribution<RealType, Policy>& /* dist */)
      { // Range of supported values for random variable k.
         // This is range where cdf rises from 0 to 1, and outside it, the pdf is zero.
         using boost::math::tools::max_value;
         return std::pair<RealType, RealType>(static_cast<RealType>(0),  max_value<RealType>());
      }

      template <class RealType, class Policy>
      inline RealType mean(const non_central_chi_squared_distribution<RealType, Policy>& dist)
      { // Mean of poisson distribution = lambda.
         const char* function = "boost::math::non_central_chi_squared_distribution<%1%>::mean()";
         RealType k = dist.degrees_of_freedom();
         RealType l = dist.non_centrality();
         RealType r;
         if(!detail::check_df(
            function,
            k, &r, Policy())
            ||
         !detail::check_non_centrality(
            function,
            l,
            &r,
            Policy()))
               return r;
         return k + l;
      } // mean

      template <class RealType, class Policy>
      inline RealType mode(const non_central_chi_squared_distribution<RealType, Policy>& dist)
      { // mode.
         static const char* function = "mode(non_central_chi_squared_distribution<%1%> const&)";

         RealType k = dist.degrees_of_freedom();
         RealType l = dist.non_centrality();
         RealType r;
         if(!detail::check_df(
            function,
            k, &r, Policy())
            ||
         !detail::check_non_centrality(
            function,
            l,
            &r,
            Policy()))
               return (RealType)r;
         return detail::generic_find_mode(dist, 1 + k, function);
      }

      template <class RealType, class Policy>
      inline RealType variance(const non_central_chi_squared_distribution<RealType, Policy>& dist)
      { // variance.
         const char* function = "boost::math::non_central_chi_squared_distribution<%1%>::variance()";
         RealType k = dist.degrees_of_freedom();
         RealType l = dist.non_centrality();
         RealType r;
         if(!detail::check_df(
            function,
            k, &r, Policy())
            ||
         !detail::check_non_centrality(
            function,
            l,
            &r,
            Policy()))
               return r;
         return 2 * (2 * l + k);
      }

      // RealType standard_deviation(const non_central_chi_squared_distribution<RealType, Policy>& dist)
      // standard_deviation provided by derived accessors.

      template <class RealType, class Policy>
      inline RealType skewness(const non_central_chi_squared_distribution<RealType, Policy>& dist)
      { // skewness = sqrt(l).
         const char* function = "boost::math::non_central_chi_squared_distribution<%1%>::skewness()";
         RealType k = dist.degrees_of_freedom();
         RealType l = dist.non_centrality();
         RealType r;
         if(!detail::check_df(
            function,
            k, &r, Policy())
            ||
         !detail::check_non_centrality(
            function,
            l,
            &r,
            Policy()))
               return r;
         BOOST_MATH_STD_USING
            return pow(2 / (k + 2 * l), RealType(3)/2) * (k + 3 * l);
      }

      template <class RealType, class Policy>
      inline RealType kurtosis_excess(const non_central_chi_squared_distribution<RealType, Policy>& dist)
      {
         const char* function = "boost::math::non_central_chi_squared_distribution<%1%>::kurtosis_excess()";
         RealType k = dist.degrees_of_freedom();
         RealType l = dist.non_centrality();
         RealType r;
         if(!detail::check_df(
            function,
            k, &r, Policy())
            ||
         !detail::check_non_centrality(
            function,
            l,
            &r,
            Policy()))
               return r;
         return 12 * (k + 4 * l) / ((k + 2 * l) * (k + 2 * l));
      } // kurtosis_excess

      template <class RealType, class Policy>
      inline RealType kurtosis(const non_central_chi_squared_distribution<RealType, Policy>& dist)
      {
         return kurtosis_excess(dist) + 3;
      }

      template <class RealType, class Policy>
      inline RealType pdf(const non_central_chi_squared_distribution<RealType, Policy>& dist, const RealType& x)
      { // Probability Density/Mass Function.
         return detail::nccs_pdf(dist, x);
      } // pdf

      template <class RealType, class Policy>
      RealType cdf(const non_central_chi_squared_distribution<RealType, Policy>& dist, const RealType& x)
      {
         const char* function = "boost::math::non_central_chi_squared_distribution<%1%>::cdf(%1%)";
         RealType k = dist.degrees_of_freedom();
         RealType l = dist.non_centrality();
         RealType r;
         if(!detail::check_df(
            function,
            k, &r, Policy())
            ||
         !detail::check_non_centrality(
            function,
            l,
            &r,
            Policy())
            ||
         !detail::check_positive_x(
            function,
            x,
            &r,
            Policy()))
               return r;

         return detail::non_central_chi_squared_cdf(x, k, l, false, Policy());
      } // cdf

      template <class RealType, class Policy>
      RealType cdf(const complemented2_type<non_central_chi_squared_distribution<RealType, Policy>, RealType>& c)
      { // Complemented Cumulative Distribution Function
         const char* function = "boost::math::non_central_chi_squared_distribution<%1%>::cdf(%1%)";
         non_central_chi_squared_distribution<RealType, Policy> const& dist = c.dist;
         RealType x = c.param;
         RealType k = dist.degrees_of_freedom();
         RealType l = dist.non_centrality();
         RealType r;
         if(!detail::check_df(
            function,
            k, &r, Policy())
            ||
         !detail::check_non_centrality(
            function,
            l,
            &r,
            Policy())
            ||
         !detail::check_positive_x(
            function,
            x,
            &r,
            Policy()))
               return r;

         return detail::non_central_chi_squared_cdf(x, k, l, true, Policy());
      } // ccdf

      template <class RealType, class Policy>
      inline RealType quantile(const non_central_chi_squared_distribution<RealType, Policy>& dist, const RealType& p)
      { // Quantile (or Percent Point) function.
         return detail::nccs_quantile(dist, p, false);
      } // quantile

      template <class RealType, class Policy>
      inline RealType quantile(const complemented2_type<non_central_chi_squared_distribution<RealType, Policy>, RealType>& c)
      { // Quantile (or Percent Point) function.
         return detail::nccs_quantile(c.dist, c.param, true);
      } // quantile complement.

   } // namespace math
} // namespace boost

// This include must be at the end, *after* the accessors
// for this distribution have been defined, in order to
// keep compilers that support two-phase lookup happy.
#include <boost/math/distributions/detail/derived_accessors.hpp>

#endif // BOOST_MATH_SPECIAL_NON_CENTRAL_CHI_SQUARE_HPP



