// boost\math\distributions\non_central_beta.hpp

// Copyright John Maddock 2008.

// Use, modification and distribution are subject to the
// Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MATH_SPECIAL_NON_CENTRAL_BETA_HPP
#define BOOST_MATH_SPECIAL_NON_CENTRAL_BETA_HPP

#include <boost/math/distributions/fwd.hpp>
#include <boost/math/special_functions/beta.hpp> // for incomplete gamma. gamma_q
#include <boost/math/distributions/complement.hpp> // complements
#include <boost/math/distributions/beta.hpp> // central distribution
#include <boost/math/distributions/detail/generic_mode.hpp>
#include <boost/math/distributions/detail/common_error_handling.hpp> // error checks
#include <boost/math/special_functions/fpclassify.hpp> // isnan.
#include <boost/math/tools/roots.hpp> // for root finding.
#include <boost/math/tools/series.hpp>

namespace boost
{
   namespace math
   {

      template <class RealType, class Policy>
      class non_central_beta_distribution;

      namespace detail{

         template <class T, class Policy>
         T non_central_beta_p(T a, T b, T lam, T x, T y, const Policy& pol, T init_val = 0)
         {
            BOOST_MATH_STD_USING
               using namespace boost::math;
            //
            // Variables come first:
            //
            std::uintmax_t max_iter = policies::get_max_series_iterations<Policy>();
            T errtol = boost::math::policies::get_epsilon<T, Policy>();
            T l2 = lam / 2;
            //
            // k is the starting point for iteration, and is the
            // maximum of the poisson weighting term,
            // note that unlike other similar code, we do not set
            // k to zero, when l2 is small, as forward iteration
            // is unstable:
            //
            int k = itrunc(l2);
            if(k == 0)
               k = 1;
               // Starting Poisson weight:
            T pois = gamma_p_derivative(T(k+1), l2, pol);
            if(pois == 0)
               return init_val;
            // recurance term:
            T xterm;
            // Starting beta term:
            T beta = x < y
               ? detail::ibeta_imp(T(a + k), b, x, pol, false, true, &xterm)
               : detail::ibeta_imp(b, T(a + k), y, pol, true, true, &xterm);

            xterm *= y / (a + b + k - 1);
            T poisf(pois), betaf(beta), xtermf(xterm);
            T sum = init_val;

            if((beta == 0) && (xterm == 0))
               return init_val;

            //
            // Backwards recursion first, this is the stable
            // direction for recursion:
            //
            T last_term = 0;
            std::uintmax_t count = k;
            for(int i = k; i >= 0; --i)
            {
               T term = beta * pois;
               sum += term;
               if(((fabs(term/sum) < errtol) && (last_term >= term)) || (term == 0))
               {
                  count = k - i;
                  break;
               }
               pois *= i / l2;
               beta += xterm;
               xterm *= (a + i - 1) / (x * (a + b + i - 2));
               last_term = term;
            }
            for(int i = k + 1; ; ++i)
            {
               poisf *= l2 / i;
               xtermf *= (x * (a + b + i - 2)) / (a + i - 1);
               betaf -= xtermf;

               T term = poisf * betaf;
               sum += term;
               if((fabs(term/sum) < errtol) || (term == 0))
               {
                  break;
               }
               if(static_cast<std::uintmax_t>(count + i - k) > max_iter)
               {
                  return policies::raise_evaluation_error(
                     "cdf(non_central_beta_distribution<%1%>, %1%)",
                     "Series did not converge, closest value was %1%", sum, pol);
               }
            }
            return sum;
         }

         template <class T, class Policy>
         T non_central_beta_q(T a, T b, T lam, T x, T y, const Policy& pol, T init_val = 0)
         {
            BOOST_MATH_STD_USING
               using namespace boost::math;
            //
            // Variables come first:
            //
            std::uintmax_t max_iter = policies::get_max_series_iterations<Policy>();
            T errtol = boost::math::policies::get_epsilon<T, Policy>();
            T l2 = lam / 2;
            //
            // k is the starting point for iteration, and is the
            // maximum of the poisson weighting term:
            //
            int k = itrunc(l2);
            T pois;
            if(k <= 30)
            {
               //
               // Might as well start at 0 since we'll likely have this number of terms anyway:
               //
               if(a + b > 1)
                  k = 0;
               else if(k == 0)
                  k = 1;
            }
            if(k == 0)
            {
               // Starting Poisson weight:
               pois = exp(-l2);
            }
            else
            {
               // Starting Poisson weight:
               pois = gamma_p_derivative(T(k+1), l2, pol);
            }
            if(pois == 0)
               return init_val;
            // recurance term:
            T xterm;
            // Starting beta term:
            T beta = x < y
               ? detail::ibeta_imp(T(a + k), b, x, pol, true, true, &xterm)
               : detail::ibeta_imp(b, T(a + k), y, pol, false, true, &xterm);

            xterm *= y / (a + b + k - 1);
            T poisf(pois), betaf(beta), xtermf(xterm);
            T sum = init_val;
            if((beta == 0) && (xterm == 0))
               return init_val;
            //
            // Forwards recursion first, this is the stable
            // direction for recursion, and the location
            // of the bulk of the sum:
            //
            T last_term = 0;
            std::uintmax_t count = 0;
            for(int i = k + 1; ; ++i)
            {
               poisf *= l2 / i;
               xtermf *= (x * (a + b + i - 2)) / (a + i - 1);
               betaf += xtermf;

               T term = poisf * betaf;
               sum += term;
               if((fabs(term/sum) < errtol) && (last_term >= term))
               {
                  count = i - k;
                  break;
               }
               if(static_cast<std::uintmax_t>(i - k) > max_iter)
               {
                  return policies::raise_evaluation_error(
                     "cdf(non_central_beta_distribution<%1%>, %1%)",
                     "Series did not converge, closest value was %1%", sum, pol);
               }
               last_term = term;
            }
            for(int i = k; i >= 0; --i)
            {
               T term = beta * pois;
               sum += term;
               if(fabs(term/sum) < errtol)
               {
                  break;
               }
               if(static_cast<std::uintmax_t>(count + k - i) > max_iter)
               {
                  return policies::raise_evaluation_error(
                     "cdf(non_central_beta_distribution<%1%>, %1%)",
                     "Series did not converge, closest value was %1%", sum, pol);
               }
               pois *= i / l2;
               beta -= xterm;
               xterm *= (a + i - 1) / (x * (a + b + i - 2));
            }
            return sum;
         }

         template <class RealType, class Policy>
         inline RealType non_central_beta_cdf(RealType x, RealType y, RealType a, RealType b, RealType l, bool invert, const Policy&)
         {
            typedef typename policies::evaluation<RealType, Policy>::type value_type;
            typedef typename policies::normalise<
               Policy,
               policies::promote_float<false>,
               policies::promote_double<false>,
               policies::discrete_quantile<>,
               policies::assert_undefined<> >::type forwarding_policy;

            BOOST_MATH_STD_USING

            if(x == 0)
               return invert ? 1.0f : 0.0f;
            if(y == 0)
               return invert ? 0.0f : 1.0f;
            value_type result;
            value_type c = a + b + l / 2;
            value_type cross = 1 - (b / c) * (1 + l / (2 * c * c));
            if(l == 0)
               result = cdf(boost::math::beta_distribution<RealType, Policy>(a, b), x);
            else if(x > cross)
            {
               // Complement is the smaller of the two:
               result = detail::non_central_beta_q(
                  static_cast<value_type>(a),
                  static_cast<value_type>(b),
                  static_cast<value_type>(l),
                  static_cast<value_type>(x),
                  static_cast<value_type>(y),
                  forwarding_policy(),
                  static_cast<value_type>(invert ? 0 : -1));
               invert = !invert;
            }
            else
            {
               result = detail::non_central_beta_p(
                  static_cast<value_type>(a),
                  static_cast<value_type>(b),
                  static_cast<value_type>(l),
                  static_cast<value_type>(x),
                  static_cast<value_type>(y),
                  forwarding_policy(),
                  static_cast<value_type>(invert ? -1 : 0));
            }
            if(invert)
               result = -result;
            return policies::checked_narrowing_cast<RealType, forwarding_policy>(
               result,
               "boost::math::non_central_beta_cdf<%1%>(%1%, %1%, %1%)");
         }

         template <class T, class Policy>
         struct nc_beta_quantile_functor
         {
            nc_beta_quantile_functor(const non_central_beta_distribution<T,Policy>& d, T t, bool c)
               : dist(d), target(t), comp(c) {}

            T operator()(const T& x)
            {
               return comp ?
                  T(target - cdf(complement(dist, x)))
                  : T(cdf(dist, x) - target);
            }

         private:
            non_central_beta_distribution<T,Policy> dist;
            T target;
            bool comp;
         };

         //
         // This is more or less a copy of bracket_and_solve_root, but
         // modified to search only the interval [0,1] using similar
         // heuristics.
         //
         template <class F, class T, class Tol, class Policy>
         std::pair<T, T> bracket_and_solve_root_01(F f, const T& guess, T factor, bool rising, Tol tol, std::uintmax_t& max_iter, const Policy& pol)
         {
            BOOST_MATH_STD_USING
               static const char* function = "boost::math::tools::bracket_and_solve_root_01<%1%>";
            //
            // Set up initial brackets:
            //
            T a = guess;
            T b = a;
            T fa = f(a);
            T fb = fa;
            //
            // Set up invocation count:
            //
            std::uintmax_t count = max_iter - 1;

            if((fa < 0) == (guess < 0 ? !rising : rising))
            {
               //
               // Zero is to the right of b, so walk upwards
               // until we find it:
               //
               while((boost::math::sign)(fb) == (boost::math::sign)(fa))
               {
                  if(count == 0)
                  {
                     b = policies::raise_evaluation_error(function, "Unable to bracket root, last nearest value was %1%", b, pol);
                     return std::make_pair(a, b);
                  }
                  //
                  // Heuristic: every 20 iterations we double the growth factor in case the
                  // initial guess was *really* bad !
                  //
                  if((max_iter - count) % 20 == 0)
                     factor *= 2;
                  //
                  // Now go ahead and move are guess by "factor",
                  // we do this by reducing 1-guess by factor:
                  //
                  a = b;
                  fa = fb;
                  b = 1 - ((1 - b) / factor);
                  fb = f(b);
                  --count;
                  BOOST_MATH_INSTRUMENT_CODE("a = " << a << " b = " << b << " fa = " << fa << " fb = " << fb << " count = " << count);
               }
            }
            else
            {
               //
               // Zero is to the left of a, so walk downwards
               // until we find it:
               //
               while((boost::math::sign)(fb) == (boost::math::sign)(fa))
               {
                  if(fabs(a) < tools::min_value<T>())
                  {
                     // Escape route just in case the answer is zero!
                     max_iter -= count;
                     max_iter += 1;
                     return a > 0 ? std::make_pair(T(0), T(a)) : std::make_pair(T(a), T(0));
                  }
                  if(count == 0)
                  {
                     a = policies::raise_evaluation_error(function, "Unable to bracket root, last nearest value was %1%", a, pol);
                     return std::make_pair(a, b);
                  }
                  //
                  // Heuristic: every 20 iterations we double the growth factor in case the
                  // initial guess was *really* bad !
                  //
                  if((max_iter - count) % 20 == 0)
                     factor *= 2;
                  //
                  // Now go ahead and move are guess by "factor":
                  //
                  b = a;
                  fb = fa;
                  a /= factor;
                  fa = f(a);
                  --count;
                  BOOST_MATH_INSTRUMENT_CODE("a = " << a << " b = " << b << " fa = " << fa << " fb = " << fb << " count = " << count);
               }
            }
            max_iter -= count;
            max_iter += 1;
            std::pair<T, T> r = toms748_solve(
               f,
               (a < 0 ? b : a),
               (a < 0 ? a : b),
               (a < 0 ? fb : fa),
               (a < 0 ? fa : fb),
               tol,
               count,
               pol);
            max_iter += count;
            BOOST_MATH_INSTRUMENT_CODE("max_iter = " << max_iter << " count = " << count);
            return r;
         }

         template <class RealType, class Policy>
         RealType nc_beta_quantile(const non_central_beta_distribution<RealType, Policy>& dist, const RealType& p, bool comp)
         {
            static const char* function = "quantile(non_central_beta_distribution<%1%>, %1%)";
            typedef typename policies::evaluation<RealType, Policy>::type value_type;
            typedef typename policies::normalise<
               Policy,
               policies::promote_float<false>,
               policies::promote_double<false>,
               policies::discrete_quantile<>,
               policies::assert_undefined<> >::type forwarding_policy;

            value_type a = dist.alpha();
            value_type b = dist.beta();
            value_type l = dist.non_centrality();
            value_type r;
            if(!beta_detail::check_alpha(
               function,
               a, &r, Policy())
               ||
            !beta_detail::check_beta(
               function,
               b, &r, Policy())
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
            // Special cases first:
            //
            if(p == 0)
               return comp
               ? 1.0f
               : 0.0f;
            if(p == 1)
               return !comp
               ? 1.0f
               : 0.0f;

            value_type c = a + b + l / 2;
            value_type mean = 1 - (b / c) * (1 + l / (2 * c * c));
            /*
            //
            // Calculate a normal approximation to the quantile,
            // uses mean and variance approximations from:
            // Algorithm AS 310:
            // Computing the Non-Central Beta Distribution Function
            // R. Chattamvelli; R. Shanmugam
            // Applied Statistics, Vol. 46, No. 1. (1997), pp. 146-156.
            //
            // Unfortunately, when this is wrong it tends to be *very*
            // wrong, so it's disabled for now, even though it often
            // gets the initial guess quite close.  Probably we could
            // do much better by factoring in the skewness if only
            // we could calculate it....
            //
            value_type delta = l / 2;
            value_type delta2 = delta * delta;
            value_type delta3 = delta * delta2;
            value_type delta4 = delta2 * delta2;
            value_type G = c * (c + 1) + delta;
            value_type alpha = a + b;
            value_type alpha2 = alpha * alpha;
            value_type eta = (2 * alpha + 1) * (2 * alpha + 1) + 1;
            value_type H = 3 * alpha2 + 5 * alpha + 2;
            value_type F = alpha2 * (alpha + 1) + H * delta
               + (2 * alpha + 4) * delta2 + delta3;
            value_type P = (3 * alpha + 1) * (9 * alpha + 17)
               + 2 * alpha * (3 * alpha + 2) * (3 * alpha + 4) + 15;
            value_type Q = 54 * alpha2 + 162 * alpha + 130;
            value_type R = 6 * (6 * alpha + 11);
            value_type D = delta
               * (H * H + 2 * P * delta + Q * delta2 + R * delta3 + 9 * delta4);
            value_type variance = (b / G)
               * (1 + delta * (l * l + 3 * l + eta) / (G * G))
               - (b * b / F) * (1 + D / (F * F));
            value_type sd = sqrt(variance);

            value_type guess = comp
               ? quantile(complement(normal_distribution<RealType, Policy>(static_cast<RealType>(mean), static_cast<RealType>(sd)), p))
               : quantile(normal_distribution<RealType, Policy>(static_cast<RealType>(mean), static_cast<RealType>(sd)), p);

            if(guess >= 1)
               guess = mean;
            if(guess <= tools::min_value<value_type>())
               guess = mean;
            */
            value_type guess = mean;
            detail::nc_beta_quantile_functor<value_type, Policy>
               f(non_central_beta_distribution<value_type, Policy>(a, b, l), p, comp);
            tools::eps_tolerance<value_type> tol(policies::digits<RealType, Policy>());
            std::uintmax_t max_iter = policies::get_max_root_iterations<Policy>();

            std::pair<value_type, value_type> ir
               = bracket_and_solve_root_01(
                  f, guess, value_type(2.5), true, tol,
                  max_iter, Policy());
            value_type result = ir.first + (ir.second - ir.first) / 2;

            if(max_iter >= policies::get_max_root_iterations<Policy>())
            {
               return policies::raise_evaluation_error<RealType>(function, "Unable to locate solution in a reasonable time:"
                  " either there is no answer to quantile of the non central beta distribution"
                  " or the answer is infinite.  Current best guess is %1%",
                  policies::checked_narrowing_cast<RealType, forwarding_policy>(
                     result,
                     function), Policy());
            }
            return policies::checked_narrowing_cast<RealType, forwarding_policy>(
               result,
               function);
         }

         template <class T, class Policy>
         T non_central_beta_pdf(T a, T b, T lam, T x, T y, const Policy& pol)
         {
            BOOST_MATH_STD_USING
            //
            // Special cases:
            //
            if((x == 0) || (y == 0))
               return 0;
            //
            // Variables come first:
            //
            std::uintmax_t max_iter = policies::get_max_series_iterations<Policy>();
            T errtol = boost::math::policies::get_epsilon<T, Policy>();
            T l2 = lam / 2;
            //
            // k is the starting point for iteration, and is the
            // maximum of the poisson weighting term:
            //
            int k = itrunc(l2);
            // Starting Poisson weight:
            T pois = gamma_p_derivative(T(k+1), l2, pol);
            // Starting beta term:
            T beta = x < y ?
               ibeta_derivative(a + k, b, x, pol)
               : ibeta_derivative(b, a + k, y, pol);
            T sum = 0;
            T poisf(pois);
            T betaf(beta);

            //
            // Stable backwards recursion first:
            //
            std::uintmax_t count = k;
            for(int i = k; i >= 0; --i)
            {
               T term = beta * pois;
               sum += term;
               if((fabs(term/sum) < errtol) || (term == 0))
               {
                  count = k - i;
                  break;
               }
               pois *= i / l2;
               beta *= (a + i - 1) / (x * (a + i + b - 1));
            }
            for(int i = k + 1; ; ++i)
            {
               poisf *= l2 / i;
               betaf *= x * (a + b + i - 1) / (a + i - 1);

               T term = poisf * betaf;
               sum += term;
               if((fabs(term/sum) < errtol) || (term == 0))
               {
                  break;
               }
               if(static_cast<std::uintmax_t>(count + i - k) > max_iter)
               {
                  return policies::raise_evaluation_error(
                     "pdf(non_central_beta_distribution<%1%>, %1%)",
                     "Series did not converge, closest value was %1%", sum, pol);
               }
            }
            return sum;
         }

         template <class RealType, class Policy>
         RealType nc_beta_pdf(const non_central_beta_distribution<RealType, Policy>& dist, const RealType& x)
         {
            BOOST_MATH_STD_USING
            static const char* function = "pdf(non_central_beta_distribution<%1%>, %1%)";
            typedef typename policies::evaluation<RealType, Policy>::type value_type;
            typedef typename policies::normalise<
               Policy,
               policies::promote_float<false>,
               policies::promote_double<false>,
               policies::discrete_quantile<>,
               policies::assert_undefined<> >::type forwarding_policy;

            value_type a = dist.alpha();
            value_type b = dist.beta();
            value_type l = dist.non_centrality();
            value_type r;
            if(!beta_detail::check_alpha(
               function,
               a, &r, Policy())
               ||
            !beta_detail::check_beta(
               function,
               b, &r, Policy())
               ||
            !detail::check_non_centrality(
               function,
               l,
               &r,
               Policy())
               ||
            !beta_detail::check_x(
               function,
               static_cast<value_type>(x),
               &r,
               Policy()))
                  return (RealType)r;

            if(l == 0)
               return pdf(boost::math::beta_distribution<RealType, Policy>(dist.alpha(), dist.beta()), x);
            return policies::checked_narrowing_cast<RealType, forwarding_policy>(
               non_central_beta_pdf(a, b, l, static_cast<value_type>(x), value_type(1 - static_cast<value_type>(x)), forwarding_policy()),
               "function");
         }

         template <class T>
         struct hypergeometric_2F2_sum
         {
            typedef T result_type;
            hypergeometric_2F2_sum(T a1_, T a2_, T b1_, T b2_, T z_) : a1(a1_), a2(a2_), b1(b1_), b2(b2_), z(z_), term(1), k(0) {}
            T operator()()
            {
               T result = term;
               term *= a1 * a2 / (b1 * b2);
               a1 += 1;
               a2 += 1;
               b1 += 1;
               b2 += 1;
               k += 1;
               term /= k;
               term *= z;
               return result;
            }
            T a1, a2, b1, b2, z, term, k;
         };

         template <class T, class Policy>
         T hypergeometric_2F2(T a1, T a2, T b1, T b2, T z, const Policy& pol)
         {
            typedef typename policies::evaluation<T, Policy>::type value_type;

            const char* function = "boost::math::detail::hypergeometric_2F2<%1%>(%1%,%1%,%1%,%1%,%1%)";

            hypergeometric_2F2_sum<value_type> s(a1, a2, b1, b2, z);
            std::uintmax_t max_iter = policies::get_max_series_iterations<Policy>();

            value_type result = boost::math::tools::sum_series(s, boost::math::policies::get_epsilon<value_type, Policy>(), max_iter);

            policies::check_series_iterations<T>(function, max_iter, pol);
            return policies::checked_narrowing_cast<T, Policy>(result, function);
         }

      } // namespace detail

      template <class RealType = double, class Policy = policies::policy<> >
      class non_central_beta_distribution
      {
      public:
         typedef RealType value_type;
         typedef Policy policy_type;

         non_central_beta_distribution(RealType a_, RealType b_, RealType lambda) : a(a_), b(b_), ncp(lambda)
         {
            const char* function = "boost::math::non_central_beta_distribution<%1%>::non_central_beta_distribution(%1%,%1%)";
            RealType r;
            beta_detail::check_alpha(
               function,
               a, &r, Policy());
            beta_detail::check_beta(
               function,
               b, &r, Policy());
            detail::check_non_centrality(
               function,
               lambda,
               &r,
               Policy());
         } // non_central_beta_distribution constructor.

         RealType alpha() const
         { // Private data getter function.
            return a;
         }
         RealType beta() const
         { // Private data getter function.
            return b;
         }
         RealType non_centrality() const
         { // Private data getter function.
            return ncp;
         }
      private:
         // Data member, initialized by constructor.
         RealType a;   // alpha.
         RealType b;   // beta.
         RealType ncp; // non-centrality parameter
      }; // template <class RealType, class Policy> class non_central_beta_distribution

      typedef non_central_beta_distribution<double> non_central_beta; // Reserved name of type double.

      // Non-member functions to give properties of the distribution.

      template <class RealType, class Policy>
      inline const std::pair<RealType, RealType> range(const non_central_beta_distribution<RealType, Policy>& /* dist */)
      { // Range of permissible values for random variable k.
         using boost::math::tools::max_value;
         return std::pair<RealType, RealType>(static_cast<RealType>(0), static_cast<RealType>(1));
      }

      template <class RealType, class Policy>
      inline const std::pair<RealType, RealType> support(const non_central_beta_distribution<RealType, Policy>& /* dist */)
      { // Range of supported values for random variable k.
         // This is range where cdf rises from 0 to 1, and outside it, the pdf is zero.
         using boost::math::tools::max_value;
         return std::pair<RealType, RealType>(static_cast<RealType>(0), static_cast<RealType>(1));
      }

      template <class RealType, class Policy>
      inline RealType mode(const non_central_beta_distribution<RealType, Policy>& dist)
      { // mode.
         static const char* function = "mode(non_central_beta_distribution<%1%> const&)";

         RealType a = dist.alpha();
         RealType b = dist.beta();
         RealType l = dist.non_centrality();
         RealType r;
         if(!beta_detail::check_alpha(
               function,
               a, &r, Policy())
               ||
            !beta_detail::check_beta(
               function,
               b, &r, Policy())
               ||
            !detail::check_non_centrality(
               function,
               l,
               &r,
               Policy()))
                  return (RealType)r;
         RealType c = a + b + l / 2;
         RealType mean = 1 - (b / c) * (1 + l / (2 * c * c));
         return detail::generic_find_mode_01(
            dist,
            mean,
            function);
      }

      //
      // We don't have the necessary information to implement
      // these at present.  These are just disabled for now,
      // prototypes retained so we can fill in the blanks
      // later:
      //
      template <class RealType, class Policy>
      inline RealType mean(const non_central_beta_distribution<RealType, Policy>& dist)
      {
         BOOST_MATH_STD_USING
         RealType a = dist.alpha();
         RealType b = dist.beta();
         RealType d = dist.non_centrality();
         RealType apb = a + b;
         return exp(-d / 2) * a * detail::hypergeometric_2F2<RealType, Policy>(1 + a, apb, a, 1 + apb, d / 2, Policy()) / apb;
      } // mean

      template <class RealType, class Policy>
      inline RealType variance(const non_central_beta_distribution<RealType, Policy>& dist)
      { 
         //
         // Relative error of this function may be arbitrarily large... absolute
         // error will be small however... that's the best we can do for now.
         //
         BOOST_MATH_STD_USING
         RealType a = dist.alpha();
         RealType b = dist.beta();
         RealType d = dist.non_centrality();
         RealType apb = a + b;
         RealType result = detail::hypergeometric_2F2(RealType(1 + a), apb, a, RealType(1 + apb), RealType(d / 2), Policy());
         result *= result * -exp(-d) * a * a / (apb * apb);
         result += exp(-d / 2) * a * (1 + a) * detail::hypergeometric_2F2(RealType(2 + a), apb, a, RealType(2 + apb), RealType(d / 2), Policy()) / (apb * (1 + apb));
         return result;
      }

      // RealType standard_deviation(const non_central_beta_distribution<RealType, Policy>& dist)
      // standard_deviation provided by derived accessors.
      template <class RealType, class Policy>
      inline RealType skewness(const non_central_beta_distribution<RealType, Policy>& /*dist*/)
      { // skewness = sqrt(l).
         const char* function = "boost::math::non_central_beta_distribution<%1%>::skewness()";
         typedef typename Policy::assert_undefined_type assert_type;
         static_assert(assert_type::value == 0, "Assert type is undefined.");

         return policies::raise_evaluation_error<RealType>(
            function,
            "This function is not yet implemented, the only sensible result is %1%.",
            std::numeric_limits<RealType>::quiet_NaN(), Policy()); // infinity?
      }

      template <class RealType, class Policy>
      inline RealType kurtosis_excess(const non_central_beta_distribution<RealType, Policy>& /*dist*/)
      {
         const char* function = "boost::math::non_central_beta_distribution<%1%>::kurtosis_excess()";
         typedef typename Policy::assert_undefined_type assert_type;
         static_assert(assert_type::value == 0, "Assert type is undefined.");

         return policies::raise_evaluation_error<RealType>(
            function,
            "This function is not yet implemented, the only sensible result is %1%.",
            std::numeric_limits<RealType>::quiet_NaN(), Policy()); // infinity?
      } // kurtosis_excess

      template <class RealType, class Policy>
      inline RealType kurtosis(const non_central_beta_distribution<RealType, Policy>& dist)
      {
         return kurtosis_excess(dist) + 3;
      }

      template <class RealType, class Policy>
      inline RealType pdf(const non_central_beta_distribution<RealType, Policy>& dist, const RealType& x)
      { // Probability Density/Mass Function.
         return detail::nc_beta_pdf(dist, x);
      } // pdf

      template <class RealType, class Policy>
      RealType cdf(const non_central_beta_distribution<RealType, Policy>& dist, const RealType& x)
      {
         const char* function = "boost::math::non_central_beta_distribution<%1%>::cdf(%1%)";
            RealType a = dist.alpha();
            RealType b = dist.beta();
            RealType l = dist.non_centrality();
            RealType r;
            if(!beta_detail::check_alpha(
               function,
               a, &r, Policy())
               ||
            !beta_detail::check_beta(
               function,
               b, &r, Policy())
               ||
            !detail::check_non_centrality(
               function,
               l,
               &r,
               Policy())
               ||
            !beta_detail::check_x(
               function,
               x,
               &r,
               Policy()))
                  return (RealType)r;

         if(l == 0)
            return cdf(beta_distribution<RealType, Policy>(a, b), x);

         return detail::non_central_beta_cdf(x, RealType(1 - x), a, b, l, false, Policy());
      } // cdf

      template <class RealType, class Policy>
      RealType cdf(const complemented2_type<non_central_beta_distribution<RealType, Policy>, RealType>& c)
      { // Complemented Cumulative Distribution Function
         const char* function = "boost::math::non_central_beta_distribution<%1%>::cdf(%1%)";
         non_central_beta_distribution<RealType, Policy> const& dist = c.dist;
            RealType a = dist.alpha();
            RealType b = dist.beta();
            RealType l = dist.non_centrality();
            RealType x = c.param;
            RealType r;
            if(!beta_detail::check_alpha(
               function,
               a, &r, Policy())
               ||
            !beta_detail::check_beta(
               function,
               b, &r, Policy())
               ||
            !detail::check_non_centrality(
               function,
               l,
               &r,
               Policy())
               ||
            !beta_detail::check_x(
               function,
               x,
               &r,
               Policy()))
                  return (RealType)r;

         if(l == 0)
            return cdf(complement(beta_distribution<RealType, Policy>(a, b), x));

         return detail::non_central_beta_cdf(x, RealType(1 - x), a, b, l, true, Policy());
      } // ccdf

      template <class RealType, class Policy>
      inline RealType quantile(const non_central_beta_distribution<RealType, Policy>& dist, const RealType& p)
      { // Quantile (or Percent Point) function.
         return detail::nc_beta_quantile(dist, p, false);
      } // quantile

      template <class RealType, class Policy>
      inline RealType quantile(const complemented2_type<non_central_beta_distribution<RealType, Policy>, RealType>& c)
      { // Quantile (or Percent Point) function.
         return detail::nc_beta_quantile(c.dist, c.param, true);
      } // quantile complement.

   } // namespace math
} // namespace boost

// This include must be at the end, *after* the accessors
// for this distribution have been defined, in order to
// keep compilers that support two-phase lookup happy.
#include <boost/math/distributions/detail/derived_accessors.hpp>

#endif // BOOST_MATH_SPECIAL_NON_CENTRAL_BETA_HPP

