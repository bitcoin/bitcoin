//  Copyright (c) 2006 Xiaogang Zhang
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MATH_BESSEL_JY_HPP
#define BOOST_MATH_BESSEL_JY_HPP

#ifdef _MSC_VER
#pragma once
#endif

#include <boost/math/tools/config.hpp>
#include <boost/math/special_functions/gamma.hpp>
#include <boost/math/special_functions/sign.hpp>
#include <boost/math/special_functions/hypot.hpp>
#include <boost/math/special_functions/sin_pi.hpp>
#include <boost/math/special_functions/cos_pi.hpp>
#include <boost/math/special_functions/detail/bessel_jy_asym.hpp>
#include <boost/math/special_functions/detail/bessel_jy_series.hpp>
#include <boost/math/constants/constants.hpp>
#include <boost/math/policies/error_handling.hpp>
#include <complex>

// Bessel functions of the first and second kind of fractional order

namespace boost { namespace math {

   namespace detail {

      //
      // Simultaneous calculation of A&S 9.2.9 and 9.2.10
      // for use in A&S 9.2.5 and 9.2.6.
      // This series is quick to evaluate, but divergent unless
      // x is very large, in fact it's pretty hard to figure out
      // with any degree of precision when this series actually 
      // *will* converge!!  Consequently, we may just have to
      // try it and see...
      //
      template <class T, class Policy>
      bool hankel_PQ(T v, T x, T* p, T* q, const Policy& )
      {
         BOOST_MATH_STD_USING
            T tolerance = 2 * policies::get_epsilon<T, Policy>();
         *p = 1;
         *q = 0;
         T k = 1;
         T z8 = 8 * x;
         T sq = 1;
         T mu = 4 * v * v;
         T term = 1;
         bool ok = true;
         do
         {
            term *= (mu - sq * sq) / (k * z8);
            *q += term;
            k += 1;
            sq += 2;
            T mult = (sq * sq - mu) / (k * z8);
            ok = fabs(mult) < 0.5f;
            term *= mult;
            *p += term;
            k += 1;
            sq += 2;
         }
         while((fabs(term) > tolerance * *p) && ok);
         return ok;
      }

      // Calculate Y(v, x) and Y(v+1, x) by Temme's method, see
      // Temme, Journal of Computational Physics, vol 21, 343 (1976)
      template <typename T, typename Policy>
      int temme_jy(T v, T x, T* Y, T* Y1, const Policy& pol)
      {
         T g, h, p, q, f, coef, sum, sum1, tolerance;
         T a, d, e, sigma;
         unsigned long k;

         BOOST_MATH_STD_USING
            using namespace boost::math::tools;
         using namespace boost::math::constants;

         BOOST_MATH_ASSERT(fabs(v) <= 0.5f);  // precondition for using this routine

         T gp = boost::math::tgamma1pm1(v, pol);
         T gm = boost::math::tgamma1pm1(-v, pol);
         T spv = boost::math::sin_pi(v, pol);
         T spv2 = boost::math::sin_pi(v/2, pol);
         T xp = pow(x/2, v);

         a = log(x / 2);
         sigma = -a * v;
         d = abs(sigma) < tools::epsilon<T>() ?
            T(1) : sinh(sigma) / sigma;
         e = abs(v) < tools::epsilon<T>() ? T(v*pi<T>()*pi<T>() / 2)
            : T(2 * spv2 * spv2 / v);

         T g1 = (v == 0) ? T(-euler<T>()) : T((gp - gm) / ((1 + gp) * (1 + gm) * 2 * v));
         T g2 = (2 + gp + gm) / ((1 + gp) * (1 + gm) * 2);
         T vspv = (fabs(v) < tools::epsilon<T>()) ? T(1/constants::pi<T>()) : T(v / spv);
         f = (g1 * cosh(sigma) - g2 * a * d) * 2 * vspv;

         p = vspv / (xp * (1 + gm));
         q = vspv * xp / (1 + gp);

         g = f + e * q;
         h = p;
         coef = 1;
         sum = coef * g;
         sum1 = coef * h;

         T v2 = v * v;
         T coef_mult = -x * x / 4;

         // series summation
         tolerance = policies::get_epsilon<T, Policy>();
         for (k = 1; k < policies::get_max_series_iterations<Policy>(); k++)
         {
            f = (k * f + p + q) / (k*k - v2);
            p /= k - v;
            q /= k + v;
            g = f + e * q;
            h = p - k * g;
            coef *= coef_mult / k;
            sum += coef * g;
            sum1 += coef * h;
            if (abs(coef * g) < abs(sum) * tolerance) 
            { 
               break; 
            }
         }
         policies::check_series_iterations<T>("boost::math::bessel_jy<%1%>(%1%,%1%) in temme_jy", k, pol);
         *Y = -sum;
         *Y1 = -2 * sum1 / x;

         return 0;
      }

      // Evaluate continued fraction fv = J_(v+1) / J_v, see
      // Abramowitz and Stegun, Handbook of Mathematical Functions, 1972, 9.1.73
      template <typename T, typename Policy>
      int CF1_jy(T v, T x, T* fv, int* sign, const Policy& pol)
      {
         T C, D, f, a, b, delta, tiny, tolerance;
         unsigned long k;
         int s = 1;

         BOOST_MATH_STD_USING

            // |x| <= |v|, CF1_jy converges rapidly
            // |x| > |v|, CF1_jy needs O(|x|) iterations to converge

            // modified Lentz's method, see
            // Lentz, Applied Optics, vol 15, 668 (1976)
            tolerance = 2 * policies::get_epsilon<T, Policy>();
         tiny = sqrt(tools::min_value<T>());
         C = f = tiny;                           // b0 = 0, replace with tiny
         D = 0;
         for (k = 1; k < policies::get_max_series_iterations<Policy>() * 100; k++)
         {
            a = -1;
            b = 2 * (v + k) / x;
            C = b + a / C;
            D = b + a * D;
            if (C == 0) { C = tiny; }
            if (D == 0) { D = tiny; }
            D = 1 / D;
            delta = C * D;
            f *= delta;
            if (D < 0) { s = -s; }
            if (abs(delta - 1) < tolerance) 
            { break; }
         }
         policies::check_series_iterations<T>("boost::math::bessel_jy<%1%>(%1%,%1%) in CF1_jy", k / 100, pol);
         *fv = -f;
         *sign = s;                              // sign of denominator

         return 0;
      }
      //
      // This algorithm was originally written by Xiaogang Zhang
      // using std::complex to perform the complex arithmetic.
      // However, that turns out to 10x or more slower than using
      // all real-valued arithmetic, so it's been rewritten using
      // real values only.
      //
      template <typename T, typename Policy>
      int CF2_jy(T v, T x, T* p, T* q, const Policy& pol)
      {
         BOOST_MATH_STD_USING

            T Cr, Ci, Dr, Di, fr, fi, a, br, bi, delta_r, delta_i, temp;
         T tiny;
         unsigned long k;

         // |x| >= |v|, CF2_jy converges rapidly
         // |x| -> 0, CF2_jy fails to converge
         BOOST_MATH_ASSERT(fabs(x) > 1);

         // modified Lentz's method, complex numbers involved, see
         // Lentz, Applied Optics, vol 15, 668 (1976)
         T tolerance = 2 * policies::get_epsilon<T, Policy>();
         tiny = sqrt(tools::min_value<T>());
         Cr = fr = -0.5f / x;
         Ci = fi = 1;
         //Dr = Di = 0;
         T v2 = v * v;
         a = (0.25f - v2) / x; // Note complex this one time only!
         br = 2 * x;
         bi = 2;
         temp = Cr * Cr + 1;
         Ci = bi + a * Cr / temp;
         Cr = br + a / temp;
         Dr = br;
         Di = bi;
         if (fabs(Cr) + fabs(Ci) < tiny) { Cr = tiny; }
         if (fabs(Dr) + fabs(Di) < tiny) { Dr = tiny; }
         temp = Dr * Dr + Di * Di;
         Dr = Dr / temp;
         Di = -Di / temp;
         delta_r = Cr * Dr - Ci * Di;
         delta_i = Ci * Dr + Cr * Di;
         temp = fr;
         fr = temp * delta_r - fi * delta_i;
         fi = temp * delta_i + fi * delta_r;
         for (k = 2; k < policies::get_max_series_iterations<Policy>(); k++)
         {
            a = k - 0.5f;
            a *= a;
            a -= v2;
            bi += 2;
            temp = Cr * Cr + Ci * Ci;
            Cr = br + a * Cr / temp;
            Ci = bi - a * Ci / temp;
            Dr = br + a * Dr;
            Di = bi + a * Di;
            if (fabs(Cr) + fabs(Ci) < tiny) { Cr = tiny; }
            if (fabs(Dr) + fabs(Di) < tiny) { Dr = tiny; }
            temp = Dr * Dr + Di * Di;
            Dr = Dr / temp;
            Di = -Di / temp;
            delta_r = Cr * Dr - Ci * Di;
            delta_i = Ci * Dr + Cr * Di;
            temp = fr;
            fr = temp * delta_r - fi * delta_i;
            fi = temp * delta_i + fi * delta_r;
            if (fabs(delta_r - 1) + fabs(delta_i) < tolerance)
               break;
         }
         policies::check_series_iterations<T>("boost::math::bessel_jy<%1%>(%1%,%1%) in CF2_jy", k, pol);
         *p = fr;
         *q = fi;

         return 0;
      }

      static const int need_j = 1;
      static const int need_y = 2;

      // Compute J(v, x) and Y(v, x) simultaneously by Steed's method, see
      // Barnett et al, Computer Physics Communications, vol 8, 377 (1974)
      template <typename T, typename Policy>
      int bessel_jy(T v, T x, T* J, T* Y, int kind, const Policy& pol)
      {
         BOOST_MATH_ASSERT(x >= 0);

         T u, Jv, Ju, Yv, Yv1, Yu, Yu1(0), fv, fu;
         T W, p, q, gamma, current, prev, next;
         bool reflect = false;
         unsigned n, k;
         int s;
         int org_kind = kind;
         T cp = 0;
         T sp = 0;

         static const char* function = "boost::math::bessel_jy<%1%>(%1%,%1%)";

         BOOST_MATH_STD_USING
            using namespace boost::math::tools;
         using namespace boost::math::constants;

         if (v < 0)
         {
            reflect = true;
            v = -v;                             // v is non-negative from here
         }
         if (v > static_cast<T>((std::numeric_limits<int>::max)()))
         {
            *J = *Y = policies::raise_evaluation_error<T>(function, "Order of Bessel function is too large to evaluate: got %1%", v, pol);
            return 1;
         }
         n = iround(v, pol);
         u = v - n;                              // -1/2 <= u < 1/2

         if(reflect)
         {
            T z = (u + n % 2);
            cp = boost::math::cos_pi(z, pol);
            sp = boost::math::sin_pi(z, pol);
            if(u != 0)
               kind = need_j|need_y;               // need both for reflection formula
         }

         if(x == 0)
         {
            if(v == 0)
               *J = 1;
            else if((u == 0) || !reflect)
               *J = 0;
            else if(kind & need_j)
               *J = policies::raise_domain_error<T>(function, "Value of Bessel J_v(x) is complex-infinity at %1%", x, pol); // complex infinity
            else
               *J = std::numeric_limits<T>::quiet_NaN();  // any value will do, not using J.

            if((kind & need_y) == 0)
               *Y = std::numeric_limits<T>::quiet_NaN();  // any value will do, not using Y.
            else if(v == 0)
               *Y = -policies::raise_overflow_error<T>(function, 0, pol);
            else
               *Y = policies::raise_domain_error<T>(function, "Value of Bessel Y_v(x) is complex-infinity at %1%", x, pol); // complex infinity
            return 1;
         }

         // x is positive until reflection
         W = T(2) / (x * pi<T>());               // Wronskian
         T Yv_scale = 1;
         if(((kind & need_y) == 0) && ((x < 1) || (v > x * x / 4) || (x < 5)))
         {
            //
            // This series will actually converge rapidly for all small
            // x - say up to x < 20 - but the first few terms are large
            // and divergent which leads to large errors :-(
            //
            Jv = bessel_j_small_z_series(v, x, pol);
            Yv = std::numeric_limits<T>::quiet_NaN();
         }
         else if((x < 1) && (u != 0) && (log(policies::get_epsilon<T, Policy>() / 2) > v * log((x/2) * (x/2) / v)))
         {
            // Evaluate using series representations.
            // This is particularly important for x << v as in this
            // area temme_jy may be slow to converge, if it converges at all.
            // Requires x is not an integer.
            if(kind&need_j)
               Jv = bessel_j_small_z_series(v, x, pol);
            else
               Jv = std::numeric_limits<T>::quiet_NaN();
            if((org_kind&need_y && (!reflect || (cp != 0))) 
               || (org_kind & need_j && (reflect && (sp != 0))))
            {
               // Only calculate if we need it, and if the reflection formula will actually use it:
               Yv = bessel_y_small_z_series(v, x, &Yv_scale, pol);
            }
            else
               Yv = std::numeric_limits<T>::quiet_NaN();
         }
         else if((u == 0) && (x < policies::get_epsilon<T, Policy>()))
         {
            // Truncated series evaluation for small x and v an integer,
            // much quicker in this area than temme_jy below.
            if(kind&need_j)
               Jv = bessel_j_small_z_series(v, x, pol);
            else
               Jv = std::numeric_limits<T>::quiet_NaN();
            if((org_kind&need_y && (!reflect || (cp != 0))) 
               || (org_kind & need_j && (reflect && (sp != 0))))
            {
               // Only calculate if we need it, and if the reflection formula will actually use it:
               Yv = bessel_yn_small_z(n, x, &Yv_scale, pol);
            }
            else
               Yv = std::numeric_limits<T>::quiet_NaN();
         }
         else if(asymptotic_bessel_large_x_limit(v, x))
         {
            if(kind&need_y)
            {
               Yv = asymptotic_bessel_y_large_x_2(v, x, pol);
            }
            else
               Yv = std::numeric_limits<T>::quiet_NaN(); // any value will do, we're not using it.
            if(kind&need_j)
            {
               Jv = asymptotic_bessel_j_large_x_2(v, x, pol);
            }
            else
               Jv = std::numeric_limits<T>::quiet_NaN(); // any value will do, we're not using it.
         }
         else if((x > 8) && hankel_PQ(v, x, &p, &q, pol))
         {
            //
            // Hankel approximation: note that this method works best when x 
            // is large, but in that case we end up calculating sines and cosines
            // of large values, with horrendous resulting accuracy.  It is fast though
            // when it works....
            //
            // Normally we calculate sin/cos(chi) where:
            //
            // chi = x - fmod(T(v / 2 + 0.25f), T(2)) * boost::math::constants::pi<T>();
            //
            // But this introduces large errors, so use sin/cos addition formulae to
            // improve accuracy:
            //
            T mod_v = fmod(T(v / 2 + 0.25f), T(2));
            T sx = sin(x);
            T cx = cos(x);
            T sv = boost::math::sin_pi(mod_v, pol);
            T cv = boost::math::cos_pi(mod_v, pol);

            T sc = sx * cv - sv * cx; // == sin(chi);
            T cc = cx * cv + sx * sv; // == cos(chi);
            T chi = boost::math::constants::root_two<T>() / (boost::math::constants::root_pi<T>() * sqrt(x)); //sqrt(2 / (boost::math::constants::pi<T>() * x));
            Yv = chi * (p * sc + q * cc);
            Jv = chi * (p * cc - q * sc);
         }
         else if (x <= 2)                           // x in (0, 2]
         {
            if(temme_jy(u, x, &Yu, &Yu1, pol))             // Temme series
            {
               // domain error:
               *J = *Y = Yu;
               return 1;
            }
            prev = Yu;
            current = Yu1;
            T scale = 1;
            policies::check_series_iterations<T>(function, n, pol);
            for (k = 1; k <= n; k++)            // forward recurrence for Y
            {
               T fact = 2 * (u + k) / x;
               if((tools::max_value<T>() - fabs(prev)) / fact < fabs(current))
               {
                  scale /= current;
                  prev /= current;
                  current = 1;
               }
               next = fact * current - prev;
               prev = current;
               current = next;
            }
            Yv = prev;
            Yv1 = current;
            if(kind&need_j)
            {
               CF1_jy(v, x, &fv, &s, pol);                 // continued fraction CF1_jy
               Jv = scale * W / (Yv * fv - Yv1);           // Wronskian relation
            }
            else
               Jv = std::numeric_limits<T>::quiet_NaN(); // any value will do, we're not using it.
            Yv_scale = scale;
         }
         else                                    // x in (2, \infty)
         {
            // Get Y(u, x):

            T ratio;
            CF1_jy(v, x, &fv, &s, pol);
            // tiny initial value to prevent overflow
            T init = sqrt(tools::min_value<T>());
            BOOST_MATH_INSTRUMENT_VARIABLE(init);
            prev = fv * s * init;
            current = s * init;
            if(v < max_factorial<T>::value)
            {
               policies::check_series_iterations<T>(function, n, pol);
               for (k = n; k > 0; k--)             // backward recurrence for J
               {
                  next = 2 * (u + k) * current / x - prev;
                  prev = current;
                  current = next;
               }
               ratio = (s * init) / current;     // scaling ratio
               // can also call CF1_jy() to get fu, not much difference in precision
               fu = prev / current;
            }
            else
            {
               //
               // When v is large we may get overflow in this calculation
               // leading to NaN's and other nasty surprises:
               //
               policies::check_series_iterations<T>(function, n, pol);
               bool over = false;
               for (k = n; k > 0; k--)             // backward recurrence for J
               {
                  T t = 2 * (u + k) / x;
                  if((t > 1) && (tools::max_value<T>() / t < current))
                  {
                     over = true;
                     break;
                  }
                  next = t * current - prev;
                  prev = current;
                  current = next;
               }
               if(!over)
               {
                  ratio = (s * init) / current;     // scaling ratio
                  // can also call CF1_jy() to get fu, not much difference in precision
                  fu = prev / current;
               }
               else
               {
                  ratio = 0;
                  fu = 1;
               }
            }
            CF2_jy(u, x, &p, &q, pol);                  // continued fraction CF2_jy
            T t = u / x - fu;                   // t = J'/J
            gamma = (p - t) / q;
            //
            // We can't allow gamma to cancel out to zero completely as it messes up
            // the subsequent logic.  So pretend that one bit didn't cancel out
            // and set to a suitably small value.  The only test case we've been able to
            // find for this, is when v = 8.5 and x = 4*PI.
            //
            if(gamma == 0)
            {
               gamma = u * tools::epsilon<T>() / x;
            }
            BOOST_MATH_INSTRUMENT_VARIABLE(current);
            BOOST_MATH_INSTRUMENT_VARIABLE(W);
            BOOST_MATH_INSTRUMENT_VARIABLE(q);
            BOOST_MATH_INSTRUMENT_VARIABLE(gamma);
            BOOST_MATH_INSTRUMENT_VARIABLE(p);
            BOOST_MATH_INSTRUMENT_VARIABLE(t);
            Ju = sign(current) * sqrt(W / (q + gamma * (p - t)));
            BOOST_MATH_INSTRUMENT_VARIABLE(Ju);

            Jv = Ju * ratio;                    // normalization

            Yu = gamma * Ju;
            Yu1 = Yu * (u/x - p - q/gamma);

            if(kind&need_y)
            {
               // compute Y:
               prev = Yu;
               current = Yu1;
               policies::check_series_iterations<T>(function, n, pol);
               for (k = 1; k <= n; k++)            // forward recurrence for Y
               {
                  T fact = 2 * (u + k) / x;
                  if((tools::max_value<T>() - fabs(prev)) / fact < fabs(current))
                  {
                     prev /= current;
                     Yv_scale /= current;
                     current = 1;
                  }
                  next = fact * current - prev;
                  prev = current;
                  current = next;
               }
               Yv = prev;
            }
            else
               Yv = std::numeric_limits<T>::quiet_NaN(); // any value will do, we're not using it.
         }

         if (reflect)
         {
            if((sp != 0) && (tools::max_value<T>() * fabs(Yv_scale) < fabs(sp * Yv)))
               *J = org_kind & need_j ? T(-sign(sp) * sign(Yv) * (Yv_scale != 0 ? sign(Yv_scale) : 1) * policies::raise_overflow_error<T>(function, 0, pol)) : T(0);
            else
               *J = cp * Jv - (sp == 0 ? T(0) : T((sp * Yv) / Yv_scale));     // reflection formula
            if((cp != 0) && (tools::max_value<T>() * fabs(Yv_scale) < fabs(cp * Yv)))
               *Y = org_kind & need_y ? T(-sign(cp) * sign(Yv) * (Yv_scale != 0 ? sign(Yv_scale) : 1) * policies::raise_overflow_error<T>(function, 0, pol)) : T(0);
            else
               *Y = (sp != 0 ? sp * Jv : T(0)) + (cp == 0 ? T(0) : T((cp * Yv) / Yv_scale));
         }
         else
         {
            *J = Jv;
            if(tools::max_value<T>() * fabs(Yv_scale) < fabs(Yv))
               *Y = org_kind & need_y ? T(sign(Yv) * sign(Yv_scale) * policies::raise_overflow_error<T>(function, 0, pol)) : T(0);
            else
               *Y = Yv / Yv_scale;
         }

         return 0;
      }

   } // namespace detail

}} // namespaces

#endif // BOOST_MATH_BESSEL_JY_HPP
