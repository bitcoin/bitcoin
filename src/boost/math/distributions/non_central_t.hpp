// boost\math\distributions\non_central_t.hpp

// Copyright John Maddock 2008.

// Use, modification and distribution are subject to the
// Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MATH_SPECIAL_NON_CENTRAL_T_HPP
#define BOOST_MATH_SPECIAL_NON_CENTRAL_T_HPP

#include <boost/math/distributions/fwd.hpp>
#include <boost/math/distributions/non_central_beta.hpp> // for nc beta
#include <boost/math/distributions/normal.hpp> // for normal CDF and quantile
#include <boost/math/distributions/students_t.hpp>
#include <boost/math/distributions/detail/generic_quantile.hpp> // quantile

namespace boost
{
   namespace math
   {

      template <class RealType, class Policy>
      class non_central_t_distribution;

      namespace detail{

         template <class T, class Policy>
         T non_central_t2_p(T v, T delta, T x, T y, const Policy& pol, T init_val)
         {
            BOOST_MATH_STD_USING
            //
            // Variables come first:
            //
            std::uintmax_t max_iter = policies::get_max_series_iterations<Policy>();
            T errtol = policies::get_epsilon<T, Policy>();
            T d2 = delta * delta / 2;
            //
            // k is the starting point for iteration, and is the
            // maximum of the poisson weighting term, we don't
            // ever allow k == 0 as this can lead to catastrophic
            // cancellation errors later (test case is v = 1621286869049072.3
            // delta = 0.16212868690490723, x = 0.86987415482475994).
            //
            int k = itrunc(d2);
            T pois;
            if(k == 0) k = 1;
            // Starting Poisson weight:
            pois = gamma_p_derivative(T(k+1), d2, pol) 
               * tgamma_delta_ratio(T(k + 1), T(0.5f))
               * delta / constants::root_two<T>();
            if(pois == 0)
               return init_val;
            T xterm, beta;
            // Recurrence & starting beta terms:
            beta = x < y
               ? detail::ibeta_imp(T(k + 1), T(v / 2), x, pol, false, true, &xterm)
               : detail::ibeta_imp(T(v / 2), T(k + 1), y, pol, true, true, &xterm);
            xterm *= y / (v / 2 + k);
            T poisf(pois), betaf(beta), xtermf(xterm);
            T sum = init_val;
            if((xterm == 0) && (beta == 0))
               return init_val;

            //
            // Backwards recursion first, this is the stable
            // direction for recursion:
            //
            std::uintmax_t count = 0;
            T last_term = 0;
            for(int i = k; i >= 0; --i)
            {
               T term = beta * pois;
               sum += term;
               // Don't terminate on first term in case we "fixed" k above:
               if((fabs(last_term) > fabs(term)) && fabs(term/sum) < errtol)
                  break;
               last_term = term;
               pois *= (i + 0.5f) / d2;
               beta += xterm;
               xterm *= (i) / (x * (v / 2 + i - 1));
               ++count;
            }
            last_term = 0;
            for(int i = k + 1; ; ++i)
            {
               poisf *= d2 / (i + 0.5f);
               xtermf *= (x * (v / 2 + i - 1)) / (i);
               betaf -= xtermf;
               T term = poisf * betaf;
               sum += term;
               if((fabs(last_term) >= fabs(term)) && (fabs(term/sum) < errtol))
                  break;
               last_term = term;
               ++count;
               if(count > max_iter)
               {
                  return policies::raise_evaluation_error(
                     "cdf(non_central_t_distribution<%1%>, %1%)", 
                     "Series did not converge, closest value was %1%", sum, pol);
               }
            }
            return sum;
         }

         template <class T, class Policy>
         T non_central_t2_q(T v, T delta, T x, T y, const Policy& pol, T init_val)
         {
            BOOST_MATH_STD_USING
            //
            // Variables come first:
            //
            std::uintmax_t max_iter = policies::get_max_series_iterations<Policy>();
            T errtol = boost::math::policies::get_epsilon<T, Policy>();
            T d2 = delta * delta / 2;
            //
            // k is the starting point for iteration, and is the
            // maximum of the poisson weighting term, we don't allow
            // k == 0 as this can cause catastrophic cancellation errors
            // (test case is v = 561908036470413.25, delta = 0.056190803647041321,
            // x = 1.6155232703966216):
            //
            int k = itrunc(d2);
            if(k == 0) k = 1;
            // Starting Poisson weight:
            T pois;
            if((k < (int)(max_factorial<T>::value)) && (d2 < tools::log_max_value<T>()) && (log(d2) * k < tools::log_max_value<T>()))
            {
               //
               // For small k we can optimise this calculation by using
               // a simpler reduced formula:
               //
               pois = exp(-d2);
               pois *= pow(d2, static_cast<T>(k));
               pois /= boost::math::tgamma(T(k + 1 + 0.5), pol);
               pois *= delta / constants::root_two<T>();
            }
            else
            {
               pois = gamma_p_derivative(T(k+1), d2, pol) 
                  * tgamma_delta_ratio(T(k + 1), T(0.5f))
                  * delta / constants::root_two<T>();
            }
            if(pois == 0)
               return init_val;
            // Recurance term:
            T xterm;
            T beta;
            // Starting beta term:
            if(k != 0)
            {
               beta = x < y 
                  ? detail::ibeta_imp(T(k + 1), T(v / 2), x, pol, true, true, &xterm) 
                  : detail::ibeta_imp(T(v / 2), T(k + 1), y, pol, false, true, &xterm);

               xterm *= y / (v / 2 + k);
            }
            else
            {
               beta = pow(y, v / 2);
               xterm = beta;
            }
            T poisf(pois), betaf(beta), xtermf(xterm);
            T sum = init_val;
            if((xterm == 0) && (beta == 0))
               return init_val;

            //
            // Fused forward and backwards recursion:
            //
            std::uintmax_t count = 0;
            T last_term = 0;
            for(int i = k + 1, j = k; ; ++i, --j)
            {
               poisf *= d2 / (i + 0.5f);
               xtermf *= (x * (v / 2 + i - 1)) / (i);
               betaf += xtermf;
               T term = poisf * betaf;

               if(j >= 0)
               {
                  term += beta * pois;
                  pois *= (j + 0.5f) / d2;
                  beta -= xterm;
                  xterm *= (j) / (x * (v / 2 + j - 1));
               }

               sum += term;
               // Don't terminate on first term in case we "fixed" the value of k above:
               if((fabs(last_term) > fabs(term)) && fabs(term/sum) < errtol)
                  break;
               last_term = term;
               if(count > max_iter)
               {
                  return policies::raise_evaluation_error(
                     "cdf(non_central_t_distribution<%1%>, %1%)", 
                     "Series did not converge, closest value was %1%", sum, pol);
               }
               ++count;
            }
            return sum;
         }

         template <class T, class Policy>
         T non_central_t_cdf(T v, T delta, T t, bool invert, const Policy& pol)
         {
            BOOST_MATH_STD_USING
            if ((boost::math::isinf)(v))
            { // Infinite degrees of freedom, so use normal distribution located at delta.
               normal_distribution<T, Policy> n(delta, 1); 
               return cdf(n, t);
            }
            //
            // Otherwise, for t < 0 we have to use the reflection formula:
            if(t < 0)
            {
               t = -t;
               delta = -delta;
               invert = !invert;
            }
            if(fabs(delta / (4 * v)) < policies::get_epsilon<T, Policy>())
            {
               // Approximate with a Student's T centred on delta,
               // the crossover point is based on eq 2.6 from
               // "A Comparison of Approximations To Percentiles of the
               // Noncentral t-Distribution".  H. Sahai and M. M. Ojeda,
               // Revista Investigacion Operacional Vol 21, No 2, 2000.
               // Original sources referenced in the above are:
               // "Some Approximations to the Percentage Points of the Noncentral
               // t-Distribution". C. van Eeden. International Statistical Review, 29, 4-31.
               // "Continuous Univariate Distributions".  N.L. Johnson, S. Kotz and
               // N. Balkrishnan. 1995. John Wiley and Sons New York.
               T result = cdf(students_t_distribution<T, Policy>(v), t - delta);
               return invert ? 1 - result : result;
            }
            //
            // x and y are the corresponding random
            // variables for the noncentral beta distribution,
            // with y = 1 - x:
            //
            T x = t * t / (v + t * t);
            T y = v / (v + t * t);
            T d2 = delta * delta;
            T a = 0.5f;
            T b = v / 2;
            T c = a + b + d2 / 2;
            //
            // Crossover point for calculating p or q is the same
            // as for the noncentral beta:
            //
            T cross = 1 - (b / c) * (1 + d2 / (2 * c * c));
            T result;
            if(x < cross)
            {
               //
               // Calculate p:
               //
               if(x != 0)
               {
                  result = non_central_beta_p(a, b, d2, x, y, pol);
                  result = non_central_t2_p(v, delta, x, y, pol, result);
                  result /= 2;
               }
               else
                  result = 0;
               result += cdf(boost::math::normal_distribution<T, Policy>(), -delta);
            }
            else
            {
               //
               // Calculate q:
               //
               invert = !invert;
               if(x != 0)
               {
                  result = non_central_beta_q(a, b, d2, x, y, pol);
                  result = non_central_t2_q(v, delta, x, y, pol, result);
                  result /= 2;
               }
               else // x == 0
                  result = cdf(complement(boost::math::normal_distribution<T, Policy>(), -delta));
            }
            if(invert)
               result = 1 - result;
            return result;
         }

         template <class T, class Policy>
         T non_central_t_quantile(const char* function, T v, T delta, T p, T q, const Policy&)
         {
            BOOST_MATH_STD_USING
     //       static const char* function = "quantile(non_central_t_distribution<%1%>, %1%)";
     // now passed as function
            typedef typename policies::evaluation<T, Policy>::type value_type;
            typedef typename policies::normalise<
               Policy, 
               policies::promote_float<false>, 
               policies::promote_double<false>, 
               policies::discrete_quantile<>,
               policies::assert_undefined<> >::type forwarding_policy;

               T r;
               if(!detail::check_df_gt0_to_inf(
                  function,
                  v, &r, Policy())
                  ||
               !detail::check_finite(
                  function,
                  delta,
                  &r,
                  Policy())
                  ||
               !detail::check_probability(
                  function,
                  p,
                  &r,
                  Policy()))
                     return r;


            value_type guess = 0;
            if ( ((boost::math::isinf)(v)) || (v > 1 / boost::math::tools::epsilon<T>()) )
            { // Infinite or very large degrees of freedom, so use normal distribution located at delta.
               normal_distribution<T, Policy> n(delta, 1);
               if (p < q)
               {
                 return quantile(n, p);
               }
               else
               {
                 return quantile(complement(n, q));
               }
            }
            else if(v > 3)
            { // Use normal distribution to calculate guess.
               value_type mean = (v > 1 / policies::get_epsilon<T, Policy>()) ? delta : delta * sqrt(v / 2) * tgamma_delta_ratio((v - 1) * 0.5f, T(0.5f));
               value_type var = (v > 1 / policies::get_epsilon<T, Policy>()) ? value_type(1) : (((delta * delta + 1) * v) / (v - 2) - mean * mean);
               if(p < q)
                  guess = quantile(normal_distribution<value_type, forwarding_policy>(mean, var), p);
               else
                  guess = quantile(complement(normal_distribution<value_type, forwarding_policy>(mean, var), q));
            }
            //
            // We *must* get the sign of the initial guess correct, 
            // or our root-finder will fail, so double check it now:
            //
            value_type pzero = non_central_t_cdf(
               static_cast<value_type>(v), 
               static_cast<value_type>(delta), 
               static_cast<value_type>(0), 
               !(p < q), 
               forwarding_policy());
            int s;
            if(p < q)
               s = boost::math::sign(p - pzero);
            else
               s = boost::math::sign(pzero - q);
            if(s != boost::math::sign(guess))
            {
               guess = static_cast<T>(s);
            }

            value_type result = detail::generic_quantile(
               non_central_t_distribution<value_type, forwarding_policy>(v, delta), 
               (p < q ? p : q), 
               guess, 
               (p >= q), 
               function);
            return policies::checked_narrowing_cast<T, forwarding_policy>(
               result, 
               function);
         }

         template <class T, class Policy>
         T non_central_t2_pdf(T n, T delta, T x, T y, const Policy& pol, T init_val)
         {
            BOOST_MATH_STD_USING
            //
            // Variables come first:
            //
            std::uintmax_t max_iter = policies::get_max_series_iterations<Policy>();
            T errtol = boost::math::policies::get_epsilon<T, Policy>();
            T d2 = delta * delta / 2;
            //
            // k is the starting point for iteration, and is the
            // maximum of the poisson weighting term:
            //
            int k = itrunc(d2);
            T pois, xterm;
            if(k == 0)
               k = 1;
            // Starting Poisson weight:
            pois = gamma_p_derivative(T(k+1), d2, pol) 
               * tgamma_delta_ratio(T(k + 1), T(0.5f))
               * delta / constants::root_two<T>();
            // Starting beta term:
            xterm = x < y
               ? ibeta_derivative(T(k + 1), n / 2, x, pol)
               : ibeta_derivative(n / 2, T(k + 1), y, pol);
            T poisf(pois), xtermf(xterm);
            T sum = init_val;
            if((pois == 0) || (xterm == 0))
               return init_val;

            //
            // Backwards recursion first, this is the stable
            // direction for recursion:
            //
            std::uintmax_t count = 0;
            for(int i = k; i >= 0; --i)
            {
               T term = xterm * pois;
               sum += term;
               if(((fabs(term/sum) < errtol) && (i != k)) || (term == 0))
                  break;
               pois *= (i + 0.5f) / d2;
               xterm *= (i) / (x * (n / 2 + i));
               ++count;
               if(count > max_iter)
               {
                  return policies::raise_evaluation_error(
                     "pdf(non_central_t_distribution<%1%>, %1%)", 
                     "Series did not converge, closest value was %1%", sum, pol);
               }
            }
            for(int i = k + 1; ; ++i)
            {
               poisf *= d2 / (i + 0.5f);
               xtermf *= (x * (n / 2 + i)) / (i);
               T term = poisf * xtermf;
               sum += term;
               if((fabs(term/sum) < errtol) || (term == 0))
                  break;
               ++count;
               if(count > max_iter)
               {
                  return policies::raise_evaluation_error(
                     "pdf(non_central_t_distribution<%1%>, %1%)", 
                     "Series did not converge, closest value was %1%", sum, pol);
               }
            }
            return sum;
         }

         template <class T, class Policy>
         T non_central_t_pdf(T n, T delta, T t, const Policy& pol)
         {
            BOOST_MATH_STD_USING
            if ((boost::math::isinf)(n))
            { // Infinite degrees of freedom, so use normal distribution located at delta.
               normal_distribution<T, Policy> norm(delta, 1); 
               return pdf(norm, t);
            }
            //
            // Otherwise, for t < 0 we have to use the reflection formula:
            if(t < 0)
            {
               t = -t;
               delta = -delta;
            }
            if(t == 0)
            {
               //
               // Handle this as a special case, using the formula
               // from Weisstein, Eric W. 
               // "Noncentral Student's t-Distribution." 
               // From MathWorld--A Wolfram Web Resource. 
               // http://mathworld.wolfram.com/NoncentralStudentst-Distribution.html 
               // 
               // The formula is simplified thanks to the relation
               // 1F1(a,b,0) = 1.
               //
               return tgamma_delta_ratio(n / 2 + 0.5f, T(0.5f))
                  * sqrt(n / constants::pi<T>()) 
                  * exp(-delta * delta / 2) / 2;
            }
            if(fabs(delta / (4 * n)) < policies::get_epsilon<T, Policy>())
            {
               // Approximate with a Student's T centred on delta,
               // the crossover point is based on eq 2.6 from
               // "A Comparison of Approximations To Percentiles of the
               // Noncentral t-Distribution".  H. Sahai and M. M. Ojeda,
               // Revista Investigacion Operacional Vol 21, No 2, 2000.
               // Original sources referenced in the above are:
               // "Some Approximations to the Percentage Points of the Noncentral
               // t-Distribution". C. van Eeden. International Statistical Review, 29, 4-31.
               // "Continuous Univariate Distributions".  N.L. Johnson, S. Kotz and
               // N. Balkrishnan. 1995. John Wiley and Sons New York.
               return pdf(students_t_distribution<T, Policy>(n), t - delta);
            }
            //
            // x and y are the corresponding random
            // variables for the noncentral beta distribution,
            // with y = 1 - x:
            //
            T x = t * t / (n + t * t);
            T y = n / (n + t * t);
            T a = 0.5f;
            T b = n / 2;
            T d2 = delta * delta;
            //
            // Calculate pdf:
            //
            T dt = n * t / (n * n + 2 * n * t * t + t * t * t * t);
            T result = non_central_beta_pdf(a, b, d2, x, y, pol);
            T tol = tools::epsilon<T>() * result * 500;
            result = non_central_t2_pdf(n, delta, x, y, pol, result);
            if(result <= tol)
               result = 0;
            result *= dt;
            return result;
         }

         template <class T, class Policy>
         T mean(T v, T delta, const Policy& pol)
         {
            if ((boost::math::isinf)(v))
            {
               return delta;
            }
            BOOST_MATH_STD_USING
            if (v > 1 / boost::math::tools::epsilon<T>() )
            {
              //normal_distribution<T, Policy> n(delta, 1);
              //return boost::math::mean(n); 
              return delta;
            }
            else
            {
             return delta * sqrt(v / 2) * tgamma_delta_ratio((v - 1) * 0.5f, T(0.5f), pol);
            }
            // Other moments use mean so using normal distribution is propagated.
         }

         template <class T, class Policy>
         T variance(T v, T delta, const Policy& pol)
         {
            if ((boost::math::isinf)(v))
            {
               return 1;
            }
            if (delta == 0)
            {  // == Student's t
              return v / (v - 2);
            }
            T result = ((delta * delta + 1) * v) / (v - 2);
            T m = mean(v, delta, pol);
            result -= m * m;
            return result;
         }

         template <class T, class Policy>
         T skewness(T v, T delta, const Policy& pol)
         {
            BOOST_MATH_STD_USING
            if ((boost::math::isinf)(v))
            {
               return 0;
            }
            if(delta == 0)
            { // == Student's t
              return 0;
            }
            T mean = boost::math::detail::mean(v, delta, pol);
            T l2 = delta * delta;
            T var = ((l2 + 1) * v) / (v - 2) - mean * mean;
            T result = -2 * var;
            result += v * (l2 + 2 * v - 3) / ((v - 3) * (v - 2));
            result *= mean;
            result /= pow(var, T(1.5f));
            return result;
         }

         template <class T, class Policy>
         T kurtosis_excess(T v, T delta, const Policy& pol)
         {
            BOOST_MATH_STD_USING
            if ((boost::math::isinf)(v))
            {
               return 3;
            }
            if (delta == 0)
            { // == Student's t
              return 3;
            }
            T mean = boost::math::detail::mean(v, delta, pol);
            T l2 = delta * delta;
            T var = ((l2 + 1) * v) / (v - 2) - mean * mean;
            T result = -3 * var;
            result += v * (l2 * (v + 1) + 3 * (3 * v - 5)) / ((v - 3) * (v - 2));
            result *= -mean * mean;
            result += v * v * (l2 * l2 + 6 * l2 + 3) / ((v - 4) * (v - 2));
            result /= var * var;
            return result;
         }

#if 0
         // 
         // This code is disabled, since there can be multiple answers to the
         // question, and it's not clear how to find the "right" one.
         //
         template <class RealType, class Policy>
         struct t_degrees_of_freedom_finder
         {
            t_degrees_of_freedom_finder(
               RealType delta_, RealType x_, RealType p_, bool c)
               : delta(delta_), x(x_), p(p_), comp(c) {}

            RealType operator()(const RealType& v)
            {
               non_central_t_distribution<RealType, Policy> d(v, delta);
               return comp ?
                  p - cdf(complement(d, x))
                  : cdf(d, x) - p;
            }
         private:
            RealType delta;
            RealType x;
            RealType p;
            bool comp;
         };

         template <class RealType, class Policy>
         inline RealType find_t_degrees_of_freedom(
            RealType delta, RealType x, RealType p, RealType q, const Policy& pol)
         {
            const char* function = "non_central_t<%1%>::find_degrees_of_freedom";
            if((p == 0) || (q == 0))
            {
               //
               // Can't a thing if one of p and q is zero:
               //
               return policies::raise_evaluation_error<RealType>(function, 
                  "Can't find degrees of freedom when the probability is 0 or 1, only possible answer is %1%", 
                  RealType(std::numeric_limits<RealType>::quiet_NaN()), Policy());
            }
            t_degrees_of_freedom_finder<RealType, Policy> f(delta, x, p < q ? p : q, p < q ? false : true);
            tools::eps_tolerance<RealType> tol(policies::digits<RealType, Policy>());
            std::uintmax_t max_iter = policies::get_max_root_iterations<Policy>();
            //
            // Pick an initial guess:
            //
            RealType guess = 200;
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
         struct t_non_centrality_finder
         {
            t_non_centrality_finder(
               RealType v_, RealType x_, RealType p_, bool c)
               : v(v_), x(x_), p(p_), comp(c) {}

            RealType operator()(const RealType& delta)
            {
               non_central_t_distribution<RealType, Policy> d(v, delta);
               return comp ?
                  p - cdf(complement(d, x))
                  : cdf(d, x) - p;
            }
         private:
            RealType v;
            RealType x;
            RealType p;
            bool comp;
         };

         template <class RealType, class Policy>
         inline RealType find_t_non_centrality(
            RealType v, RealType x, RealType p, RealType q, const Policy& pol)
         {
            const char* function = "non_central_t<%1%>::find_t_non_centrality";
            if((p == 0) || (q == 0))
            {
               //
               // Can't do a thing if one of p and q is zero:
               //
               return policies::raise_evaluation_error<RealType>(function, 
                  "Can't find non-centrality parameter when the probability is 0 or 1, only possible answer is %1%", 
                  RealType(std::numeric_limits<RealType>::quiet_NaN()), Policy());
            }
            t_non_centrality_finder<RealType, Policy> f(v, x, p < q ? p : q, p < q ? false : true);
            tools::eps_tolerance<RealType> tol(policies::digits<RealType, Policy>());
            std::uintmax_t max_iter = policies::get_max_root_iterations<Policy>();
            //
            // Pick an initial guess that we know is the right side of
            // zero:
            //
            RealType guess;
            if(f(0) < 0)
               guess = 1;
            else
               guess = -1;
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
#endif
      } // namespace detail ======================================================================

      template <class RealType = double, class Policy = policies::policy<> >
      class non_central_t_distribution
      {
      public:
         typedef RealType value_type;
         typedef Policy policy_type;

         non_central_t_distribution(RealType v_, RealType lambda) : v(v_), ncp(lambda)
         { 
            const char* function = "boost::math::non_central_t_distribution<%1%>::non_central_t_distribution(%1%,%1%)";
            RealType r;
            detail::check_df_gt0_to_inf(
               function,
               v, &r, Policy());
            detail::check_finite(
               function,
               lambda,
               &r,
               Policy());
         } // non_central_t_distribution constructor.

         RealType degrees_of_freedom() const
         { // Private data getter function.
            return v;
         }
         RealType non_centrality() const
         { // Private data getter function.
            return ncp;
         }
#if 0
         // 
         // This code is disabled, since there can be multiple answers to the
         // question, and it's not clear how to find the "right" one.
         //
         static RealType find_degrees_of_freedom(RealType delta, RealType x, RealType p)
         {
            const char* function = "non_central_t<%1%>::find_degrees_of_freedom";
            typedef typename policies::evaluation<RealType, Policy>::type value_type;
            typedef typename policies::normalise<
               Policy, 
               policies::promote_float<false>, 
               policies::promote_double<false>, 
               policies::discrete_quantile<>,
               policies::assert_undefined<> >::type forwarding_policy;
            value_type result = detail::find_t_degrees_of_freedom(
               static_cast<value_type>(delta), 
               static_cast<value_type>(x), 
               static_cast<value_type>(p), 
               static_cast<value_type>(1-p), 
               forwarding_policy());
            return policies::checked_narrowing_cast<RealType, forwarding_policy>(
               result, 
               function);
         }
         template <class A, class B, class C>
         static RealType find_degrees_of_freedom(const complemented3_type<A,B,C>& c)
         {
            const char* function = "non_central_t<%1%>::find_degrees_of_freedom";
            typedef typename policies::evaluation<RealType, Policy>::type value_type;
            typedef typename policies::normalise<
               Policy, 
               policies::promote_float<false>, 
               policies::promote_double<false>, 
               policies::discrete_quantile<>,
               policies::assert_undefined<> >::type forwarding_policy;
            value_type result = detail::find_t_degrees_of_freedom(
               static_cast<value_type>(c.dist), 
               static_cast<value_type>(c.param1), 
               static_cast<value_type>(1-c.param2), 
               static_cast<value_type>(c.param2), 
               forwarding_policy());
            return policies::checked_narrowing_cast<RealType, forwarding_policy>(
               result, 
               function);
         }
         static RealType find_non_centrality(RealType v, RealType x, RealType p)
         {
            const char* function = "non_central_t<%1%>::find_t_non_centrality";
            typedef typename policies::evaluation<RealType, Policy>::type value_type;
            typedef typename policies::normalise<
               Policy, 
               policies::promote_float<false>, 
               policies::promote_double<false>, 
               policies::discrete_quantile<>,
               policies::assert_undefined<> >::type forwarding_policy;
            value_type result = detail::find_t_non_centrality(
               static_cast<value_type>(v), 
               static_cast<value_type>(x), 
               static_cast<value_type>(p), 
               static_cast<value_type>(1-p), 
               forwarding_policy());
            return policies::checked_narrowing_cast<RealType, forwarding_policy>(
               result, 
               function);
         }
         template <class A, class B, class C>
         static RealType find_non_centrality(const complemented3_type<A,B,C>& c)
         {
            const char* function = "non_central_t<%1%>::find_t_non_centrality";
            typedef typename policies::evaluation<RealType, Policy>::type value_type;
            typedef typename policies::normalise<
               Policy, 
               policies::promote_float<false>, 
               policies::promote_double<false>, 
               policies::discrete_quantile<>,
               policies::assert_undefined<> >::type forwarding_policy;
            value_type result = detail::find_t_non_centrality(
               static_cast<value_type>(c.dist), 
               static_cast<value_type>(c.param1), 
               static_cast<value_type>(1-c.param2), 
               static_cast<value_type>(c.param2), 
               forwarding_policy());
            return policies::checked_narrowing_cast<RealType, forwarding_policy>(
               result, 
               function);
         }
#endif
      private:
         // Data member, initialized by constructor.
         RealType v;   // degrees of freedom
         RealType ncp; // non-centrality parameter
      }; // template <class RealType, class Policy> class non_central_t_distribution

      typedef non_central_t_distribution<double> non_central_t; // Reserved name of type double.

      // Non-member functions to give properties of the distribution.

      template <class RealType, class Policy>
      inline const std::pair<RealType, RealType> range(const non_central_t_distribution<RealType, Policy>& /* dist */)
      { // Range of permissible values for random variable k.
         using boost::math::tools::max_value;
         return std::pair<RealType, RealType>(-max_value<RealType>(), max_value<RealType>());
      }

      template <class RealType, class Policy>
      inline const std::pair<RealType, RealType> support(const non_central_t_distribution<RealType, Policy>& /* dist */)
      { // Range of supported values for random variable k.
         // This is range where cdf rises from 0 to 1, and outside it, the pdf is zero.
         using boost::math::tools::max_value;
         return std::pair<RealType, RealType>(-max_value<RealType>(), max_value<RealType>());
      }

      template <class RealType, class Policy>
      inline RealType mode(const non_central_t_distribution<RealType, Policy>& dist)
      { // mode.
         static const char* function = "mode(non_central_t_distribution<%1%> const&)";
         RealType v = dist.degrees_of_freedom();
         RealType l = dist.non_centrality();
         RealType r;
         if(!detail::check_df_gt0_to_inf(
            function,
            v, &r, Policy())
            ||
         !detail::check_finite(
            function,
            l,
            &r,
            Policy()))
               return (RealType)r;

         BOOST_MATH_STD_USING

         RealType m = v < 3 ? 0 : detail::mean(v, l, Policy());
         RealType var = v < 4 ? 1 : detail::variance(v, l, Policy());

         return detail::generic_find_mode(
            dist, 
            m,
            function,
            sqrt(var));
      }

      template <class RealType, class Policy>
      inline RealType mean(const non_central_t_distribution<RealType, Policy>& dist)
      { 
         BOOST_MATH_STD_USING
         const char* function = "mean(const non_central_t_distribution<%1%>&)";
         typedef typename policies::evaluation<RealType, Policy>::type value_type;
         typedef typename policies::normalise<
            Policy, 
            policies::promote_float<false>, 
            policies::promote_double<false>, 
            policies::discrete_quantile<>,
            policies::assert_undefined<> >::type forwarding_policy;
         RealType v = dist.degrees_of_freedom();
         RealType l = dist.non_centrality();
         RealType r;
         if(!detail::check_df_gt0_to_inf(
            function,
            v, &r, Policy())
            ||
         !detail::check_finite(
            function,
            l,
            &r,
            Policy()))
               return (RealType)r;
         if(v <= 1)
            return policies::raise_domain_error<RealType>(
               function, 
               "The non-central t distribution has no defined mean for degrees of freedom <= 1: got v=%1%.", v, Policy());
         // return l * sqrt(v / 2) * tgamma_delta_ratio((v - 1) * 0.5f, RealType(0.5f));
         return policies::checked_narrowing_cast<RealType, forwarding_policy>(
            detail::mean(static_cast<value_type>(v), static_cast<value_type>(l), forwarding_policy()), function);

      } // mean

      template <class RealType, class Policy>
      inline RealType variance(const non_central_t_distribution<RealType, Policy>& dist)
      { // variance.
         const char* function = "variance(const non_central_t_distribution<%1%>&)";
         typedef typename policies::evaluation<RealType, Policy>::type value_type;
         typedef typename policies::normalise<
            Policy, 
            policies::promote_float<false>, 
            policies::promote_double<false>, 
            policies::discrete_quantile<>,
            policies::assert_undefined<> >::type forwarding_policy;
         BOOST_MATH_STD_USING
         RealType v = dist.degrees_of_freedom();
         RealType l = dist.non_centrality();
         RealType r;
         if(!detail::check_df_gt0_to_inf(
            function,
            v, &r, Policy())
            ||
         !detail::check_finite(
            function,
            l,
            &r,
            Policy()))
               return (RealType)r;
         if(v <= 2)
            return policies::raise_domain_error<RealType>(
               function, 
               "The non-central t distribution has no defined variance for degrees of freedom <= 2: got v=%1%.", v, Policy());
         return policies::checked_narrowing_cast<RealType, forwarding_policy>(
            detail::variance(static_cast<value_type>(v), static_cast<value_type>(l), forwarding_policy()), function);
      }

      // RealType standard_deviation(const non_central_t_distribution<RealType, Policy>& dist)
      // standard_deviation provided by derived accessors.

      template <class RealType, class Policy>
      inline RealType skewness(const non_central_t_distribution<RealType, Policy>& dist)
      { // skewness = sqrt(l).
         const char* function = "skewness(const non_central_t_distribution<%1%>&)";
         typedef typename policies::evaluation<RealType, Policy>::type value_type;
         typedef typename policies::normalise<
            Policy, 
            policies::promote_float<false>, 
            policies::promote_double<false>, 
            policies::discrete_quantile<>,
            policies::assert_undefined<> >::type forwarding_policy;
         RealType v = dist.degrees_of_freedom();
         RealType l = dist.non_centrality();
         RealType r;
         if(!detail::check_df_gt0_to_inf(
            function,
            v, &r, Policy())
            ||
         !detail::check_finite(
            function,
            l,
            &r,
            Policy()))
               return (RealType)r;
         if(v <= 3)
            return policies::raise_domain_error<RealType>(
               function, 
               "The non-central t distribution has no defined skewness for degrees of freedom <= 3: got v=%1%.", v, Policy());;
         return policies::checked_narrowing_cast<RealType, forwarding_policy>(
            detail::skewness(static_cast<value_type>(v), static_cast<value_type>(l), forwarding_policy()), function);
      }

      template <class RealType, class Policy>
      inline RealType kurtosis_excess(const non_central_t_distribution<RealType, Policy>& dist)
      { 
         const char* function = "kurtosis_excess(const non_central_t_distribution<%1%>&)";
         typedef typename policies::evaluation<RealType, Policy>::type value_type;
         typedef typename policies::normalise<
            Policy, 
            policies::promote_float<false>, 
            policies::promote_double<false>, 
            policies::discrete_quantile<>,
            policies::assert_undefined<> >::type forwarding_policy;
         RealType v = dist.degrees_of_freedom();
         RealType l = dist.non_centrality();
         RealType r;
         if(!detail::check_df_gt0_to_inf(
            function,
            v, &r, Policy())
            ||
         !detail::check_finite(
            function,
            l,
            &r,
            Policy()))
               return (RealType)r;
         if(v <= 4)
            return policies::raise_domain_error<RealType>(
               function, 
               "The non-central t distribution has no defined kurtosis for degrees of freedom <= 4: got v=%1%.", v, Policy());;
         return policies::checked_narrowing_cast<RealType, forwarding_policy>(
            detail::kurtosis_excess(static_cast<value_type>(v), static_cast<value_type>(l), forwarding_policy()), function);
      } // kurtosis_excess

      template <class RealType, class Policy>
      inline RealType kurtosis(const non_central_t_distribution<RealType, Policy>& dist)
      {
         return kurtosis_excess(dist) + 3;
      }

      template <class RealType, class Policy>
      inline RealType pdf(const non_central_t_distribution<RealType, Policy>& dist, const RealType& t)
      { // Probability Density/Mass Function.
         const char* function = "pdf(non_central_t_distribution<%1%>, %1%)";
         typedef typename policies::evaluation<RealType, Policy>::type value_type;
         typedef typename policies::normalise<
            Policy, 
            policies::promote_float<false>, 
            policies::promote_double<false>, 
            policies::discrete_quantile<>,
            policies::assert_undefined<> >::type forwarding_policy;

         RealType v = dist.degrees_of_freedom();
         RealType l = dist.non_centrality();
         RealType r;
         if(!detail::check_df_gt0_to_inf(
            function,
            v, &r, Policy())
            ||
         !detail::check_finite(
            function,
            l,
            &r,
            Policy())
            ||
         !detail::check_x(
            function,
            t,
            &r,
            Policy()))
               return (RealType)r;
         return policies::checked_narrowing_cast<RealType, forwarding_policy>(
            detail::non_central_t_pdf(static_cast<value_type>(v), 
               static_cast<value_type>(l), 
               static_cast<value_type>(t), 
               Policy()),
            function);
      } // pdf

      template <class RealType, class Policy>
      RealType cdf(const non_central_t_distribution<RealType, Policy>& dist, const RealType& x)
      { 
         const char* function = "boost::math::cdf(non_central_t_distribution<%1%>&, %1%)";
//   was const char* function = "boost::math::non_central_t_distribution<%1%>::cdf(%1%)";
         typedef typename policies::evaluation<RealType, Policy>::type value_type;
         typedef typename policies::normalise<
            Policy, 
            policies::promote_float<false>, 
            policies::promote_double<false>, 
            policies::discrete_quantile<>,
            policies::assert_undefined<> >::type forwarding_policy;

         RealType v = dist.degrees_of_freedom();
         RealType l = dist.non_centrality();
         RealType r;
         if(!detail::check_df_gt0_to_inf(
            function,
            v, &r, Policy())
            ||
         !detail::check_finite(
            function,
            l,
            &r,
            Policy())
            ||
         !detail::check_x(
            function,
            x,
            &r,
            Policy()))
               return (RealType)r;
          if ((boost::math::isinf)(v))
          { // Infinite degrees of freedom, so use normal distribution located at delta.
             normal_distribution<RealType, Policy> n(l, 1); 
             cdf(n, x);
              //return cdf(normal_distribution<RealType, Policy>(l, 1), x);
          }

         if(l == 0)
         { // NO non-centrality, so use Student's t instead.
            return cdf(students_t_distribution<RealType, Policy>(v), x);
         }
         return policies::checked_narrowing_cast<RealType, forwarding_policy>(
            detail::non_central_t_cdf(
               static_cast<value_type>(v), 
               static_cast<value_type>(l), 
               static_cast<value_type>(x), 
               false, Policy()),
            function);
      } // cdf

      template <class RealType, class Policy>
      RealType cdf(const complemented2_type<non_central_t_distribution<RealType, Policy>, RealType>& c)
      { // Complemented Cumulative Distribution Function
  // was       const char* function = "boost::math::non_central_t_distribution<%1%>::cdf(%1%)";
         const char* function = "boost::math::cdf(const complement(non_central_t_distribution<%1%>&), %1%)";
         typedef typename policies::evaluation<RealType, Policy>::type value_type;
         typedef typename policies::normalise<
            Policy, 
            policies::promote_float<false>, 
            policies::promote_double<false>, 
            policies::discrete_quantile<>,
            policies::assert_undefined<> >::type forwarding_policy;

         non_central_t_distribution<RealType, Policy> const& dist = c.dist;
         RealType x = c.param;
         RealType v = dist.degrees_of_freedom();
         RealType l = dist.non_centrality(); // aka delta
         RealType r;
         if(!detail::check_df_gt0_to_inf(
            function,
            v, &r, Policy())
            ||
         !detail::check_finite(
            function,
            l,
            &r,
            Policy())
            ||
         !detail::check_x(
            function,
            x,
            &r,
            Policy()))
               return (RealType)r;

         if ((boost::math::isinf)(v))
         { // Infinite degrees of freedom, so use normal distribution located at delta.
             normal_distribution<RealType, Policy> n(l, 1); 
             return cdf(complement(n, x));
         }
         if(l == 0)
         { // zero non-centrality so use Student's t distribution.
            return cdf(complement(students_t_distribution<RealType, Policy>(v), x));
         }
         return policies::checked_narrowing_cast<RealType, forwarding_policy>(
            detail::non_central_t_cdf(
               static_cast<value_type>(v), 
               static_cast<value_type>(l), 
               static_cast<value_type>(x), 
               true, Policy()),
            function);
      } // ccdf

      template <class RealType, class Policy>
      inline RealType quantile(const non_central_t_distribution<RealType, Policy>& dist, const RealType& p)
      { // Quantile (or Percent Point) function.
         static const char* function = "quantile(const non_central_t_distribution<%1%>, %1%)";
         RealType v = dist.degrees_of_freedom();
         RealType l = dist.non_centrality();
         return detail::non_central_t_quantile(function, v, l, p, RealType(1-p), Policy());
      } // quantile

      template <class RealType, class Policy>
      inline RealType quantile(const complemented2_type<non_central_t_distribution<RealType, Policy>, RealType>& c)
      { // Quantile (or Percent Point) function.
         static const char* function = "quantile(const complement(non_central_t_distribution<%1%>, %1%))";
         non_central_t_distribution<RealType, Policy> const& dist = c.dist;
         RealType q = c.param;
         RealType v = dist.degrees_of_freedom();
         RealType l = dist.non_centrality();
         return detail::non_central_t_quantile(function, v, l, RealType(1-q), q, Policy());
      } // quantile complement.

   } // namespace math
} // namespace boost

// This include must be at the end, *after* the accessors
// for this distribution have been defined, in order to
// keep compilers that support two-phase lookup happy.
#include <boost/math/distributions/detail/derived_accessors.hpp>

#endif // BOOST_MATH_SPECIAL_NON_CENTRAL_T_HPP

