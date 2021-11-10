//  Copyright John Maddock 2007.
//  Copyright Paul A. Bristow 2007
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MATH_SF_DETAIL_INV_T_HPP
#define BOOST_MATH_SF_DETAIL_INV_T_HPP

#ifdef _MSC_VER
#pragma once
#endif

#include <boost/math/special_functions/cbrt.hpp>
#include <boost/math/special_functions/round.hpp>
#include <boost/math/special_functions/trunc.hpp>

namespace boost{ namespace math{ namespace detail{

//
// The main method used is due to Hill:
//
// G. W. Hill, Algorithm 396, Student's t-Quantiles,
// Communications of the ACM, 13(10): 619-620, Oct., 1970.
//
template <class T, class Policy>
T inverse_students_t_hill(T ndf, T u, const Policy& pol)
{
   BOOST_MATH_STD_USING
   BOOST_MATH_ASSERT(u <= 0.5);

   T a, b, c, d, q, x, y;

   if (ndf > 1e20f)
      return -boost::math::erfc_inv(2 * u, pol) * constants::root_two<T>();

   a = 1 / (ndf - 0.5f);
   b = 48 / (a * a);
   c = ((20700 * a / b - 98) * a - 16) * a + 96.36f;
   d = ((94.5f / (b + c) - 3) / b + 1) * sqrt(a * constants::pi<T>() / 2) * ndf;
   y = pow(d * 2 * u, 2 / ndf);

   if (y > (0.05f + a))
   {
      //
      // Asymptotic inverse expansion about normal:
      //
      x = -boost::math::erfc_inv(2 * u, pol) * constants::root_two<T>();
      y = x * x;

      if (ndf < 5)
         c += 0.3f * (ndf - 4.5f) * (x + 0.6f);
      c += (((0.05f * d * x - 5) * x - 7) * x - 2) * x + b;
      y = (((((0.4f * y + 6.3f) * y + 36) * y + 94.5f) / c - y - 3) / b + 1) * x;
      y = boost::math::expm1(a * y * y, pol);
   }
   else
   {
      y = static_cast<T>(((1 / (((ndf + 6) / (ndf * y) - 0.089f * d - 0.822f)
              * (ndf + 2) * 3) + 0.5 / (ndf + 4)) * y - 1)
              * (ndf + 1) / (ndf + 2) + 1 / y);
   }
   q = sqrt(ndf * y);

   return -q;
}
//
// Tail and body series are due to Shaw:
//
// www.mth.kcl.ac.uk/~shaww/web_page/papers/Tdistribution06.pdf
//
// Shaw, W.T., 2006, "Sampling Student's T distribution - use of
// the inverse cumulative distribution function."
// Journal of Computational Finance, Vol 9 Issue 4, pp 37-73, Summer 2006
//
template <class T, class Policy>
T inverse_students_t_tail_series(T df, T v, const Policy& pol)
{
   BOOST_MATH_STD_USING
   // Tail series expansion, see section 6 of Shaw's paper.
   // w is calculated using Eq 60:
   T w = boost::math::tgamma_delta_ratio(df / 2, constants::half<T>(), pol)
      * sqrt(df * constants::pi<T>()) * v;
   // define some variables:
   T np2 = df + 2;
   T np4 = df + 4;
   T np6 = df + 6;
   //
   // Calculate the coefficients d(k), these depend only on the
   // number of degrees of freedom df, so at least in theory
   // we could tabulate these for fixed df, see p15 of Shaw:
   //
   T d[7] = { 1, };
   d[1] = -(df + 1) / (2 * np2);
   np2 *= (df + 2);
   d[2] = -df * (df + 1) * (df + 3) / (8 * np2 * np4);
   np2 *= df + 2;
   d[3] = -df * (df + 1) * (df + 5) * (((3 * df) + 7) * df -2) / (48 * np2 * np4 * np6);
   np2 *= (df + 2);
   np4 *= (df + 4);
   d[4] = -df * (df + 1) * (df + 7) *
      ( (((((15 * df) + 154) * df + 465) * df + 286) * df - 336) * df + 64 )
      / (384 * np2 * np4 * np6 * (df + 8));
   np2 *= (df + 2);
   d[5] = -df * (df + 1) * (df + 3) * (df + 9)
            * (((((((35 * df + 452) * df + 1573) * df + 600) * df - 2020) * df) + 928) * df -128)
            / (1280 * np2 * np4 * np6 * (df + 8) * (df + 10));
   np2 *= (df + 2);
   np4 *= (df + 4);
   np6 *= (df + 6);
   d[6] = -df * (df + 1) * (df + 11)
            * ((((((((((((945 * df) + 31506) * df + 425858) * df + 2980236) * df + 11266745) * df + 20675018) * df + 7747124) * df - 22574632) * df - 8565600) * df + 18108416) * df - 7099392) * df + 884736)
            / (46080 * np2 * np4 * np6 * (df + 8) * (df + 10) * (df +12));
   //
   // Now bring everything together to provide the result,
   // this is Eq 62 of Shaw:
   //
   T rn = sqrt(df);
   T div = pow(rn * w, 1 / df);
   T power = div * div;
   T result = tools::evaluate_polynomial<7, T, T>(d, power);
   result *= rn;
   result /= div;
   return -result;
}

template <class T, class Policy>
T inverse_students_t_body_series(T df, T u, const Policy& pol)
{
   BOOST_MATH_STD_USING
   //
   // Body series for small N:
   //
   // Start with Eq 56 of Shaw:
   //
   T v = boost::math::tgamma_delta_ratio(df / 2, constants::half<T>(), pol)
      * sqrt(df * constants::pi<T>()) * (u - constants::half<T>());
   //
   // Workspace for the polynomial coefficients:
   //
   T c[11] = { 0, 1, };
   //
   // Figure out what the coefficients are, note these depend
   // only on the degrees of freedom (Eq 57 of Shaw):
   //
   T in = 1 / df;
   c[2] = static_cast<T>(0.16666666666666666667 + 0.16666666666666666667 * in);
   c[3] = static_cast<T>((0.0083333333333333333333 * in
      + 0.066666666666666666667) * in 
      + 0.058333333333333333333);
   c[4] = static_cast<T>(((0.00019841269841269841270 * in
      + 0.0017857142857142857143) * in 
      + 0.026785714285714285714) * in 
      + 0.025198412698412698413);
   c[5] = static_cast<T>((((2.7557319223985890653e-6 * in
      + 0.00037477954144620811287) * in 
      - 0.0011078042328042328042) * in 
      + 0.010559964726631393298) * in 
      + 0.012039792768959435626);
   c[6] = static_cast<T>(((((2.5052108385441718775e-8 * in
      - 0.000062705427288760622094) * in 
      + 0.00059458674042007375341) * in 
      - 0.0016095979637646304313) * in 
      + 0.0061039211560044893378) * in 
      + 0.0038370059724226390893);
   c[7] = static_cast<T>((((((1.6059043836821614599e-10 * in
      + 0.000015401265401265401265) * in 
      - 0.00016376804137220803887) * in
      + 0.00069084207973096861986) * in 
      - 0.0012579159844784844785) * in 
      + 0.0010898206731540064873) * in 
      + 0.0032177478835464946576);
   c[8] = static_cast<T>(((((((7.6471637318198164759e-13 * in
      - 3.9851014346715404916e-6) * in
      + 0.000049255746366361445727) * in
      - 0.00024947258047043099953) * in 
      + 0.00064513046951456342991) * in
      - 0.00076245135440323932387) * in
      + 0.000033530976880017885309) * in 
      + 0.0017438262298340009980);
   c[9] = static_cast<T>((((((((2.8114572543455207632e-15 * in
      + 1.0914179173496789432e-6) * in
      - 0.000015303004486655377567) * in
      + 0.000090867107935219902229) * in
      - 0.00029133414466938067350) * in
      + 0.00051406605788341121363) * in
      - 0.00036307660358786885787) * in
      - 0.00031101086326318780412) * in 
      + 0.00096472747321388644237);
   c[10] = static_cast<T>(((((((((8.2206352466243297170e-18 * in
      - 3.1239569599829868045e-7) * in
      + 4.8903045291975346210e-6) * in
      - 0.000033202652391372058698) * in
      + 0.00012645437628698076975) * in
      - 0.00028690924218514613987) * in
      + 0.00035764655430568632777) * in
      - 0.00010230378073700412687) * in
      - 0.00036942667800009661203) * in
      + 0.00054229262813129686486);
   //
   // The result is then a polynomial in v (see Eq 56 of Shaw):
   //
   return tools::evaluate_odd_polynomial<11, T, T>(c, v);
}

template <class T, class Policy>
T inverse_students_t(T df, T u, T v, const Policy& pol, bool* pexact = 0)
{
   //
   // df = number of degrees of freedom.
   // u = probability.
   // v = 1 - u.
   // l = lanczos type to use.
   //
   BOOST_MATH_STD_USING
   bool invert = false;
   T result = 0;
   if(pexact)
      *pexact = false;
   if(u > v)
   {
      // function is symmetric, invert it:
      std::swap(u, v);
      invert = true;
   }
   if((floor(df) == df) && (df < 20))
   {
      //
      // we have integer degrees of freedom, try for the special
      // cases first:
      //
      T tolerance = ldexp(1.0f, (2 * policies::digits<T, Policy>()) / 3);

      switch(itrunc(df, Policy()))
      {
      case 1:
         {
            //
            // df = 1 is the same as the Cauchy distribution, see
            // Shaw Eq 35:
            //
            if(u == 0.5)
               result = 0;
            else
               result = -cos(constants::pi<T>() * u) / sin(constants::pi<T>() * u);
            if(pexact)
               *pexact = true;
            break;
         }
      case 2:
         {
            //
            // df = 2 has an exact result, see Shaw Eq 36:
            //
            result =(2 * u - 1) / sqrt(2 * u * v);
            if(pexact)
               *pexact = true;
            break;
         }
      case 4:
         {
            //
            // df = 4 has an exact result, see Shaw Eq 38 & 39:
            //
            T alpha = 4 * u * v;
            T root_alpha = sqrt(alpha);
            T r = 4 * cos(acos(root_alpha) / 3) / root_alpha;
            T x = sqrt(r - 4);
            result = u - 0.5f < 0 ? (T)-x : x;
            if(pexact)
               *pexact = true;
            break;
         }
      case 6:
         {
            //
            // We get numeric overflow in this area:
            //
            if(u < 1e-150)
               return (invert ? -1 : 1) * inverse_students_t_hill(df, u, pol);
            //
            // Newton-Raphson iteration of a polynomial case,
            // choice of seed value is taken from Shaw's online
            // supplement:
            //
            T a = 4 * (u - u * u);//1 - 4 * (u - 0.5f) * (u - 0.5f);
            T b = boost::math::cbrt(a, pol);
            static const T c = static_cast<T>(0.85498797333834849467655443627193);
            T p = 6 * (1 + c * (1 / b - 1));
            T p0;
            do{
               T p2 = p * p;
               T p4 = p2 * p2;
               T p5 = p * p4;
               p0 = p;
               // next term is given by Eq 41:
               p = 2 * (8 * a * p5 - 270 * p2 + 2187) / (5 * (4 * a * p4 - 216 * p - 243));
            }while(fabs((p - p0) / p) > tolerance);
            //
            // Use Eq 45 to extract the result:
            //
            p = sqrt(p - df);
            result = (u - 0.5f) < 0 ? (T)-p : p;
            break;
         }
#if 0
         //
         // These are Shaw's "exact" but iterative solutions
         // for even df, the numerical accuracy of these is
         // rather less than Hill's method, so these are disabled
         // for now, which is a shame because they are reasonably
         // quick to evaluate...
         //
      case 8:
         {
            //
            // Newton-Raphson iteration of a polynomial case,
            // choice of seed value is taken from Shaw's online
            // supplement:
            //
            static const T c8 = 0.85994765706259820318168359251872L;
            T a = 4 * (u - u * u); //1 - 4 * (u - 0.5f) * (u - 0.5f);
            T b = pow(a, T(1) / 4);
            T p = 8 * (1 + c8 * (1 / b - 1));
            T p0 = p;
            do{
               T p5 = p * p;
               p5 *= p5 * p;
               p0 = p;
               // Next term is given by Eq 42:
               p = 2 * (3 * p + (640 * (160 + p * (24 + p * (p + 4)))) / (-5120 + p * (-2048 - 960 * p + a * p5))) / 7;
            }while(fabs((p - p0) / p) > tolerance);
            //
            // Use Eq 45 to extract the result:
            //
            p = sqrt(p - df);
            result = (u - 0.5f) < 0 ? -p : p;
            break;
         }
      case 10:
         {
            //
            // Newton-Raphson iteration of a polynomial case,
            // choice of seed value is taken from Shaw's online
            // supplement:
            //
            static const T c10 = 0.86781292867813396759105692122285L;
            T a = 4 * (u - u * u); //1 - 4 * (u - 0.5f) * (u - 0.5f);
            T b = pow(a, T(1) / 5);
            T p = 10 * (1 + c10 * (1 / b - 1));
            T p0;
            do{
               T p6 = p * p;
               p6 *= p6 * p6;
               p0 = p;
               // Next term given by Eq 43:
               p = (8 * p) / 9 + (218750 * (21875 + 4 * p * (625 + p * (75 + 2 * p * (5 + p))))) /
                  (9 * (-68359375 + 8 * p * (-2343750 + p * (-546875 - 175000 * p + 8 * a * p6))));
            }while(fabs((p - p0) / p) > tolerance);
            //
            // Use Eq 45 to extract the result:
            //
            p = sqrt(p - df);
            result = (u - 0.5f) < 0 ? -p : p;
            break;
         }
#endif
      default:
         goto calculate_real;
      }
   }
   else
   {
calculate_real:
      if(df > 0x10000000)
      {
         result = -boost::math::erfc_inv(2 * u, pol) * constants::root_two<T>();
         if((pexact) && (df >= 1e20))
            *pexact = true;
      }
      else if(df < 3)
      {
         //
         // Use a roughly linear scheme to choose between Shaw's
         // tail series and body series:
         //
         T crossover = 0.2742f - df * 0.0242143f;
         if(u > crossover)
         {
            result = boost::math::detail::inverse_students_t_body_series(df, u, pol);
         }
         else
         {
            result = boost::math::detail::inverse_students_t_tail_series(df, u, pol);
         }
      }
      else
      {
         //
         // Use Hill's method except in the extreme tails
         // where we use Shaw's tail series.
         // The crossover point is roughly exponential in -df:
         //
         T crossover = ldexp(1.0f, iround(T(df / -0.654f), typename policies::normalise<Policy, policies::rounding_error<policies::ignore_error> >::type()));
         if(u > crossover)
         {
            result = boost::math::detail::inverse_students_t_hill(df, u, pol);
         }
         else
         {
            result = boost::math::detail::inverse_students_t_tail_series(df, u, pol);
         }
      }
   }
   return invert ? (T)-result : result;
}

template <class T, class Policy>
inline T find_ibeta_inv_from_t_dist(T a, T p, T /*q*/, T* py, const Policy& pol)
{
   T u = p / 2;
   T v = 1 - u;
   T df = a * 2;
   T t = boost::math::detail::inverse_students_t(df, u, v, pol);
   *py = t * t / (df + t * t);
   return df / (df + t * t);
}

template <class T, class Policy>
inline T fast_students_t_quantile_imp(T df, T p, const Policy& pol, const std::false_type*)
{
   BOOST_MATH_STD_USING
   //
   // Need to use inverse incomplete beta to get
   // required precision so not so fast:
   //
   T probability = (p > 0.5) ? 1 - p : p;
   T t, x, y(0);
   x = ibeta_inv(df / 2, T(0.5), 2 * probability, &y, pol);
   if(df * y > tools::max_value<T>() * x)
      t = policies::raise_overflow_error<T>("boost::math::students_t_quantile<%1%>(%1%,%1%)", 0, pol);
   else
      t = sqrt(df * y / x);
   //
   // Figure out sign based on the size of p:
   //
   if(p < 0.5)
      t = -t;
   return t;
}

template <class T, class Policy>
T fast_students_t_quantile_imp(T df, T p, const Policy& pol, const std::true_type*)
{
   BOOST_MATH_STD_USING
   bool invert = false;
   if((df < 2) && (floor(df) != df))
      return boost::math::detail::fast_students_t_quantile_imp(df, p, pol, static_cast<std::false_type*>(0));
   if(p > 0.5)
   {
      p = 1 - p;
      invert = true;
   }
   //
   // Get an estimate of the result:
   //
   bool exact;
   T t = inverse_students_t(df, p, T(1-p), pol, &exact);
   if((t == 0) || exact)
      return invert ? -t : t; // can't do better!
   //
   // Change variables to inverse incomplete beta:
   //
   T t2 = t * t;
   T xb = df / (df + t2);
   T y = t2 / (df + t2);
   T a = df / 2;
   //
   // t can be so large that x underflows,
   // just return our estimate in that case:
   //
   if(xb == 0)
      return t;
   //
   // Get incomplete beta and it's derivative:
   //
   T f1;
   T f0 = xb < y ? ibeta_imp(a, constants::half<T>(), xb, pol, false, true, &f1)
      : ibeta_imp(constants::half<T>(), a, y, pol, true, true, &f1);

   // Get cdf from incomplete beta result:
   T p0 = f0 / 2  - p;
   // Get pdf from derivative:
   T p1 = f1 * sqrt(y * xb * xb * xb / df);
   //
   // Second derivative divided by p1:
   //
   // yacas gives:
   //
   // In> PrettyForm(Simplify(D(t) (1 + t^2/v) ^ (-(v+1)/2)))
   //
   //  |                        | v + 1     |     |
   //  |                       -| ----- + 1 |     |
   //  |                        |   2       |     |
   // -|             |  2     |                   |
   //  |             | t      |                   |
   //  |             | -- + 1 |                   |
   //  | ( v + 1 ) * | v      |               * t |
   // ---------------------------------------------
   //                       v
   //
   // Which after some manipulation is:
   //
   // -p1 * t * (df + 1) / (t^2 + df)
   //
   T p2 = t * (df + 1) / (t * t + df);
   // Halley step:
   t = fabs(t);
   t += p0 / (p1 + p0 * p2 / 2);
   return !invert ? -t : t;
}

template <class T, class Policy>
inline T fast_students_t_quantile(T df, T p, const Policy& pol)
{
   typedef typename policies::evaluation<T, Policy>::type value_type;
   typedef typename policies::normalise<
      Policy, 
      policies::promote_float<false>, 
      policies::promote_double<false>, 
      policies::discrete_quantile<>,
      policies::assert_undefined<> >::type forwarding_policy;

   typedef std::integral_constant<bool,
      (std::numeric_limits<T>::digits <= 53)
       &&
      (std::numeric_limits<T>::is_specialized)
       &&
      (std::numeric_limits<T>::radix == 2)
   > tag_type;
   return policies::checked_narrowing_cast<T, forwarding_policy>(fast_students_t_quantile_imp(static_cast<value_type>(df), static_cast<value_type>(p), pol, static_cast<tag_type*>(0)), "boost::math::students_t_quantile<%1%>(%1%,%1%,%1%)");
}

}}} // namespaces

#endif // BOOST_MATH_SF_DETAIL_INV_T_HPP



