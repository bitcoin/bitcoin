//  Copyright (c) 2006 Xiaogang Zhang
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MATH_BESSEL_IK_HPP
#define BOOST_MATH_BESSEL_IK_HPP

#ifdef _MSC_VER
#pragma once
#endif

#include <cmath>
#include <cstdint>
#include <boost/math/special_functions/round.hpp>
#include <boost/math/special_functions/gamma.hpp>
#include <boost/math/special_functions/sin_pi.hpp>
#include <boost/math/constants/constants.hpp>
#include <boost/math/policies/error_handling.hpp>
#include <boost/math/tools/config.hpp>

// Modified Bessel functions of the first and second kind of fractional order

namespace boost { namespace math {

namespace detail {

template <class T, class Policy>
struct cyl_bessel_i_small_z
{
   typedef T result_type;

   cyl_bessel_i_small_z(T v_, T z_) : k(0), v(v_), mult(z_*z_/4) 
   {
      BOOST_MATH_STD_USING
      term = 1;
   }

   T operator()()
   {
      T result = term;
      ++k;
      term *= mult / k;
      term /= k + v;
      return result;
   }
private:
   unsigned k;
   T v;
   T term;
   T mult;
};

template <class T, class Policy>
inline T bessel_i_small_z_series(T v, T x, const Policy& pol)
{
   BOOST_MATH_STD_USING
   T prefix;
   if(v < max_factorial<T>::value)
   {
      prefix = pow(x / 2, v) / boost::math::tgamma(v + 1, pol);
   }
   else
   {
      prefix = v * log(x / 2) - boost::math::lgamma(v + 1, pol);
      prefix = exp(prefix);
   }
   if(prefix == 0)
      return prefix;

   cyl_bessel_i_small_z<T, Policy> s(v, x);
   std::uintmax_t max_iter = policies::get_max_series_iterations<Policy>();
   
   T result = boost::math::tools::sum_series(s, boost::math::policies::get_epsilon<T, Policy>(), max_iter);

   policies::check_series_iterations<T>("boost::math::bessel_j_small_z_series<%1%>(%1%,%1%)", max_iter, pol);
   return prefix * result;
}

// Calculate K(v, x) and K(v+1, x) by method analogous to
// Temme, Journal of Computational Physics, vol 21, 343 (1976)
template <typename T, typename Policy>
int temme_ik(T v, T x, T* K, T* K1, const Policy& pol)
{
    T f, h, p, q, coef, sum, sum1, tolerance;
    T a, b, c, d, sigma, gamma1, gamma2;
    unsigned long k;

    BOOST_MATH_STD_USING
    using namespace boost::math::tools;
    using namespace boost::math::constants;


    // |x| <= 2, Temme series converge rapidly
    // |x| > 2, the larger the |x|, the slower the convergence
    BOOST_MATH_ASSERT(abs(x) <= 2);
    BOOST_MATH_ASSERT(abs(v) <= 0.5f);

    T gp = boost::math::tgamma1pm1(v, pol);
    T gm = boost::math::tgamma1pm1(-v, pol);

    a = log(x / 2);
    b = exp(v * a);
    sigma = -a * v;
    c = abs(v) < tools::epsilon<T>() ?
       T(1) : T(boost::math::sin_pi(v, pol) / (v * pi<T>()));
    d = abs(sigma) < tools::epsilon<T>() ?
        T(1) : T(sinh(sigma) / sigma);
    gamma1 = abs(v) < tools::epsilon<T>() ?
        T(-euler<T>()) : T((0.5f / v) * (gp - gm) * c);
    gamma2 = (2 + gp + gm) * c / 2;

    // initial values
    p = (gp + 1) / (2 * b);
    q = (1 + gm) * b / 2;
    f = (cosh(sigma) * gamma1 + d * (-a) * gamma2) / c;
    h = p;
    coef = 1;
    sum = coef * f;
    sum1 = coef * h;

    BOOST_MATH_INSTRUMENT_VARIABLE(p);
    BOOST_MATH_INSTRUMENT_VARIABLE(q);
    BOOST_MATH_INSTRUMENT_VARIABLE(f);
    BOOST_MATH_INSTRUMENT_VARIABLE(sigma);
    BOOST_MATH_INSTRUMENT_CODE(sinh(sigma));
    BOOST_MATH_INSTRUMENT_VARIABLE(gamma1);
    BOOST_MATH_INSTRUMENT_VARIABLE(gamma2);
    BOOST_MATH_INSTRUMENT_VARIABLE(c);
    BOOST_MATH_INSTRUMENT_VARIABLE(d);
    BOOST_MATH_INSTRUMENT_VARIABLE(a);

    // series summation
    tolerance = tools::epsilon<T>();
    for (k = 1; k < policies::get_max_series_iterations<Policy>(); k++)
    {
        f = (k * f + p + q) / (k*k - v*v);
        p /= k - v;
        q /= k + v;
        h = p - k * f;
        coef *= x * x / (4 * k);
        sum += coef * f;
        sum1 += coef * h;
        if (abs(coef * f) < abs(sum) * tolerance) 
        { 
           break; 
        }
    }
    policies::check_series_iterations<T>("boost::math::bessel_ik<%1%>(%1%,%1%) in temme_ik", k, pol);

    *K = sum;
    *K1 = 2 * sum1 / x;

    return 0;
}

// Evaluate continued fraction fv = I_(v+1) / I_v, derived from
// Abramowitz and Stegun, Handbook of Mathematical Functions, 1972, 9.1.73
template <typename T, typename Policy>
int CF1_ik(T v, T x, T* fv, const Policy& pol)
{
    T C, D, f, a, b, delta, tiny, tolerance;
    unsigned long k;

    BOOST_MATH_STD_USING

    // |x| <= |v|, CF1_ik converges rapidly
    // |x| > |v|, CF1_ik needs O(|x|) iterations to converge

    // modified Lentz's method, see
    // Lentz, Applied Optics, vol 15, 668 (1976)
    tolerance = 2 * tools::epsilon<T>();
    BOOST_MATH_INSTRUMENT_VARIABLE(tolerance);
    tiny = sqrt(tools::min_value<T>());
    BOOST_MATH_INSTRUMENT_VARIABLE(tiny);
    C = f = tiny;                           // b0 = 0, replace with tiny
    D = 0;
    for (k = 1; k < policies::get_max_series_iterations<Policy>(); k++)
    {
        a = 1;
        b = 2 * (v + k) / x;
        C = b + a / C;
        D = b + a * D;
        if (C == 0) { C = tiny; }
        if (D == 0) { D = tiny; }
        D = 1 / D;
        delta = C * D;
        f *= delta;
        BOOST_MATH_INSTRUMENT_VARIABLE(delta-1);
        if (abs(delta - 1) <= tolerance) 
        { 
           break; 
        }
    }
    BOOST_MATH_INSTRUMENT_VARIABLE(k);
    policies::check_series_iterations<T>("boost::math::bessel_ik<%1%>(%1%,%1%) in CF1_ik", k, pol);

    *fv = f;

    return 0;
}

// Calculate K(v, x) and K(v+1, x) by evaluating continued fraction
// z1 / z0 = U(v+1.5, 2v+1, 2x) / U(v+0.5, 2v+1, 2x), see
// Thompson and Barnett, Computer Physics Communications, vol 47, 245 (1987)
template <typename T, typename Policy>
int CF2_ik(T v, T x, T* Kv, T* Kv1, const Policy& pol)
{
    BOOST_MATH_STD_USING
    using namespace boost::math::constants;

    T S, C, Q, D, f, a, b, q, delta, tolerance, current, prev;
    unsigned long k;

    // |x| >= |v|, CF2_ik converges rapidly
    // |x| -> 0, CF2_ik fails to converge

    BOOST_MATH_ASSERT(abs(x) > 1);

    // Steed's algorithm, see Thompson and Barnett,
    // Journal of Computational Physics, vol 64, 490 (1986)
    tolerance = tools::epsilon<T>();
    a = v * v - 0.25f;
    b = 2 * (x + 1);                              // b1
    D = 1 / b;                                    // D1 = 1 / b1
    f = delta = D;                                // f1 = delta1 = D1, coincidence
    prev = 0;                                     // q0
    current = 1;                                  // q1
    Q = C = -a;                                   // Q1 = C1 because q1 = 1
    S = 1 + Q * delta;                            // S1
    BOOST_MATH_INSTRUMENT_VARIABLE(tolerance);
    BOOST_MATH_INSTRUMENT_VARIABLE(a);
    BOOST_MATH_INSTRUMENT_VARIABLE(b);
    BOOST_MATH_INSTRUMENT_VARIABLE(D);
    BOOST_MATH_INSTRUMENT_VARIABLE(f);

    for (k = 2; k < policies::get_max_series_iterations<Policy>(); k++)     // starting from 2
    {
        // continued fraction f = z1 / z0
        a -= 2 * (k - 1);
        b += 2;
        D = 1 / (b + a * D);
        delta *= b * D - 1;
        f += delta;

        // series summation S = 1 + \sum_{n=1}^{\infty} C_n * z_n / z_0
        q = (prev - (b - 2) * current) / a;
        prev = current;
        current = q;                        // forward recurrence for q
        C *= -a / k;
        Q += C * q;
        S += Q * delta;
        //
        // Under some circumstances q can grow very small and C very
        // large, leading to under/overflow.  This is particularly an
        // issue for types which have many digits precision but a narrow
        // exponent range.  A typical example being a "double double" type.
        // To avoid this situation we can normalise q (and related prev/current)
        // and C.  All other variables remain unchanged in value.  A typical
        // test case occurs when x is close to 2, for example cyl_bessel_k(9.125, 2.125).
        //
        if(q < tools::epsilon<T>())
        {
           C *= q;
           prev /= q;
           current /= q;
           q = 1;
        }

        // S converges slower than f
        BOOST_MATH_INSTRUMENT_VARIABLE(Q * delta);
        BOOST_MATH_INSTRUMENT_VARIABLE(abs(S) * tolerance);
        BOOST_MATH_INSTRUMENT_VARIABLE(S);
        if (abs(Q * delta) < abs(S) * tolerance) 
        { 
           break; 
        }
    }
    policies::check_series_iterations<T>("boost::math::bessel_ik<%1%>(%1%,%1%) in CF2_ik", k, pol);

    if(x >= tools::log_max_value<T>())
       *Kv = exp(0.5f * log(pi<T>() / (2 * x)) - x - log(S));
    else
      *Kv = sqrt(pi<T>() / (2 * x)) * exp(-x) / S;
    *Kv1 = *Kv * (0.5f + v + x + (v * v - 0.25f) * f) / x;
    BOOST_MATH_INSTRUMENT_VARIABLE(*Kv);
    BOOST_MATH_INSTRUMENT_VARIABLE(*Kv1);

    return 0;
}

enum{
   need_i = 1,
   need_k = 2
};

// Compute I(v, x) and K(v, x) simultaneously by Temme's method, see
// Temme, Journal of Computational Physics, vol 19, 324 (1975)
template <typename T, typename Policy>
int bessel_ik(T v, T x, T* I, T* K, int kind, const Policy& pol)
{
    // Kv1 = K_(v+1), fv = I_(v+1) / I_v
    // Ku1 = K_(u+1), fu = I_(u+1) / I_u
    T u, Iv, Kv, Kv1, Ku, Ku1, fv;
    T W, current, prev, next;
    bool reflect = false;
    unsigned n, k;
    int org_kind = kind;
    BOOST_MATH_INSTRUMENT_VARIABLE(v);
    BOOST_MATH_INSTRUMENT_VARIABLE(x);
    BOOST_MATH_INSTRUMENT_VARIABLE(kind);

    BOOST_MATH_STD_USING
    using namespace boost::math::tools;
    using namespace boost::math::constants;

    static const char* function = "boost::math::bessel_ik<%1%>(%1%,%1%)";

    if (v < 0)
    {
        reflect = true;
        v = -v;                             // v is non-negative from here
        kind |= need_k;
    }
    n = iround(v, pol);
    u = v - n;                              // -1/2 <= u < 1/2
    BOOST_MATH_INSTRUMENT_VARIABLE(n);
    BOOST_MATH_INSTRUMENT_VARIABLE(u);

    if (x < 0)
    {
       *I = *K = policies::raise_domain_error<T>(function,
            "Got x = %1% but real argument x must be non-negative, complex number result not supported.", x, pol);
        return 1;
    }
    if (x == 0)
    {
       Iv = (v == 0) ? static_cast<T>(1) : static_cast<T>(0);
       if(kind & need_k)
       {
         Kv = policies::raise_overflow_error<T>(function, 0, pol);
       }
       else
       {
          Kv = std::numeric_limits<T>::quiet_NaN(); // any value will do
       }

       if(reflect && (kind & need_i))
       {
           T z = (u + n % 2);
           Iv = boost::math::sin_pi(z, pol) == 0 ? 
               Iv : 
               policies::raise_overflow_error<T>(function, 0, pol);   // reflection formula
       }

       *I = Iv;
       *K = Kv;
       return 0;
    }

    // x is positive until reflection
    W = 1 / x;                                 // Wronskian
    if (x <= 2)                                // x in (0, 2]
    {
        temme_ik(u, x, &Ku, &Ku1, pol);             // Temme series
    }
    else                                       // x in (2, \infty)
    {
        CF2_ik(u, x, &Ku, &Ku1, pol);               // continued fraction CF2_ik
    }
    BOOST_MATH_INSTRUMENT_VARIABLE(Ku);
    BOOST_MATH_INSTRUMENT_VARIABLE(Ku1);
    prev = Ku;
    current = Ku1;
    T scale = 1;
    T scale_sign = 1;
    for (k = 1; k <= n; k++)                   // forward recurrence for K
    {
        T fact = 2 * (u + k) / x;
        if((tools::max_value<T>() - fabs(prev)) / fact < fabs(current))
        {
           prev /= current;
           scale /= current;
           scale_sign *= boost::math::sign(current);
           current = 1;
        }
        next = fact * current + prev;
        prev = current;
        current = next;
    }
    Kv = prev;
    Kv1 = current;
    BOOST_MATH_INSTRUMENT_VARIABLE(Kv);
    BOOST_MATH_INSTRUMENT_VARIABLE(Kv1);
    if(kind & need_i)
    {
       T lim = (4 * v * v + 10) / (8 * x);
       lim *= lim;
       lim *= lim;
       lim /= 24;
       if((lim < tools::epsilon<T>() * 10) && (x > 100))
       {
          // x is huge compared to v, CF1 may be very slow
          // to converge so use asymptotic expansion for large
          // x case instead.  Note that the asymptotic expansion
          // isn't very accurate - so it's deliberately very hard 
          // to get here - probably we're going to overflow:
          Iv = asymptotic_bessel_i_large_x(v, x, pol);
       }
       else if((v > 0) && (x / v < 0.25))
       {
          Iv = bessel_i_small_z_series(v, x, pol);
       }
       else
       {
          CF1_ik(v, x, &fv, pol);                         // continued fraction CF1_ik
          Iv = scale * W / (Kv * fv + Kv1);                  // Wronskian relation
       }
    }
    else
       Iv = std::numeric_limits<T>::quiet_NaN(); // any value will do

    if (reflect)
    {
        T z = (u + n % 2);
        T fact = (2 / pi<T>()) * (boost::math::sin_pi(z, pol) * Kv);
        if(fact == 0)
           *I = Iv;
        else if(tools::max_value<T>() * scale < fact)
           *I = (org_kind & need_i) ? T(sign(fact) * scale_sign * policies::raise_overflow_error<T>(function, 0, pol)) : T(0);
        else
         *I = Iv + fact / scale;   // reflection formula
    }
    else
    {
        *I = Iv;
    }
    if(tools::max_value<T>() * scale < Kv)
       *K = (org_kind & need_k) ? T(sign(Kv) * scale_sign * policies::raise_overflow_error<T>(function, 0, pol)) : T(0);
    else
      *K = Kv / scale;
    BOOST_MATH_INSTRUMENT_VARIABLE(*I);
    BOOST_MATH_INSTRUMENT_VARIABLE(*K);
    return 0;
}

}}} // namespaces

#endif // BOOST_MATH_BESSEL_IK_HPP

