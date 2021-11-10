//  Copyright (c) 2006 Xiaogang Zhang
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MATH_BESSEL_Y1_HPP
#define BOOST_MATH_BESSEL_Y1_HPP

#ifdef _MSC_VER
#pragma once
#pragma warning(push)
#pragma warning(disable:4702) // Unreachable code (release mode only warning)
#endif

#include <boost/math/special_functions/detail/bessel_j1.hpp>
#include <boost/math/constants/constants.hpp>
#include <boost/math/tools/rational.hpp>
#include <boost/math/tools/big_constant.hpp>
#include <boost/math/policies/error_handling.hpp>
#include <boost/math/tools/assert.hpp>

#if defined(__GNUC__) && defined(BOOST_MATH_USE_FLOAT128)
//
// This is the only way we can avoid
// warning: non-standard suffix on floating constant [-Wpedantic]
// when building with -Wall -pedantic.  Neither __extension__
// nor #pragma diagnostic ignored work :(
//
#pragma GCC system_header
#endif

// Bessel function of the second kind of order one
// x <= 8, minimax rational approximations on root-bracketing intervals
// x > 8, Hankel asymptotic expansion in Hart, Computer Approximations, 1968

namespace boost { namespace math { namespace detail{

template <typename T, typename Policy>
T bessel_y1(T x, const Policy&);

template <class T, class Policy>
struct bessel_y1_initializer
{
   struct init
   {
      init()
      {
         do_init();
      }
      static void do_init()
      {
         bessel_y1(T(1), Policy());
      }
      void force_instantiate()const{}
   };
   static const init initializer;
   static void force_instantiate()
   {
      initializer.force_instantiate();
   }
};

template <class T, class Policy>
const typename bessel_y1_initializer<T, Policy>::init bessel_y1_initializer<T, Policy>::initializer;

template <typename T, typename Policy>
T bessel_y1(T x, const Policy& pol)
{
    bessel_y1_initializer<T, Policy>::force_instantiate();

    static const T P1[] = {
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 4.0535726612579544093e+13)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 5.4708611716525426053e+12)),
        static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, -3.7595974497819597599e+11)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 7.2144548214502560419e+09)),
        static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, -5.9157479997408395984e+07)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 2.2157953222280260820e+05)),
        static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, -3.1714424660046133456e+02)),
    };
    static const T Q1[] = {
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 3.0737873921079286084e+14)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 4.1272286200406461981e+12)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 2.7800352738690585613e+10)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 1.2250435122182963220e+08)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 3.8136470753052572164e+05)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 8.2079908168393867438e+02)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 1.0)),
    };
    static const T P2[] = {
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 1.1514276357909013326e+19)),
        static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, -5.6808094574724204577e+18)),
        static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, -2.3638408497043134724e+16)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 4.0686275289804744814e+15)),
        static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, -5.9530713129741981618e+13)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 3.7453673962438488783e+11)),
        static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, -1.1957961912070617006e+09)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 1.9153806858264202986e+06)),
        static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, -1.2337180442012953128e+03)),
    };
    static const T Q2[] = {
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 5.3321844313316185697e+20)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 5.6968198822857178911e+18)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 3.0837179548112881950e+16)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 1.1187010065856971027e+14)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 3.0221766852960403645e+11)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 6.3550318087088919566e+08)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 1.0453748201934079734e+06)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 1.2855164849321609336e+03)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 1.0)),
    };
    static const T PC[] = {
        static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, -4.4357578167941278571e+06)),
        static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, -9.9422465050776411957e+06)),
        static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, -6.6033732483649391093e+06)),
        static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, -1.5235293511811373833e+06)),
        static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, -1.0982405543459346727e+05)),
        static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, -1.6116166443246101165e+03)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 0.0)),
    };
    static const T QC[] = {
        static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, -4.4357578167941278568e+06)),
        static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, -9.9341243899345856590e+06)),
        static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, -6.5853394797230870728e+06)),
        static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, -1.5118095066341608816e+06)),
        static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, -1.0726385991103820119e+05)),
        static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, -1.4550094401904961825e+03)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 1.0)),
    };
    static const T PS[] = {
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 3.3220913409857223519e+04)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 8.5145160675335701966e+04)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 6.6178836581270835179e+04)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 1.8494262873223866797e+04)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 1.7063754290207680021e+03)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 3.5265133846636032186e+01)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 0.0)),
    };
    static const T QS[] = {
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 7.0871281941028743574e+05)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 1.8194580422439972989e+06)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 1.4194606696037208929e+06)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 4.0029443582266975117e+05)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 3.7890229745772202641e+04)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 8.6383677696049909675e+02)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 1.0)),
    };
    static const T x1  =  static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 2.1971413260310170351e+00)),
                   x2  =  static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 5.4296810407941351328e+00)),
                   x11 =  static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 5.620e+02)),
                   x12 =  static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 1.8288260310170351490e-03)),
                   x21 =  static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 1.3900e+03)),
                   x22 = static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, -6.4592058648672279948e-06))
    ;
    T value, factor, r, rc, rs;

    BOOST_MATH_STD_USING
    using namespace boost::math::tools;
    using namespace boost::math::constants;

    if (x <= 0)
    {
       return policies::raise_domain_error<T>("boost::math::bessel_y1<%1%>(%1%,%1%)",
            "Got x == %1%, but x must be > 0, complex result not supported.", x, pol);
    }
    if (x <= 4)                       // x in (0, 4]
    {
        T y = x * x;
        T z = 2 * log(x/x1) * bessel_j1(x) / pi<T>();
        r = evaluate_rational(P1, Q1, y);
        factor = (x + x1) * ((x - x11/256) - x12) / x;
        value = z + factor * r;
    }
    else if (x <= 8)                  // x in (4, 8]
    {
        T y = x * x;
        T z = 2 * log(x/x2) * bessel_j1(x) / pi<T>();
        r = evaluate_rational(P2, Q2, y);
        factor = (x + x2) * ((x - x21/256) - x22) / x;
        value = z + factor * r;
    }
    else                                // x in (8, \infty)
    {
        T y = 8 / x;
        T y2 = y * y;
        rc = evaluate_rational(PC, QC, y2);
        rs = evaluate_rational(PS, QS, y2);
        factor = 1 / (sqrt(x) * root_pi<T>());
        //
        // This code is really just:
        //
        // T z = x - 0.75f * pi<T>();
        // value = factor * (rc * sin(z) + y * rs * cos(z));
        //
        // But using the sin/cos addition rules, plus constants for sin/cos of 3PI/4
        // which then cancel out with corresponding terms in "factor".
        //
        T sx = sin(x);
        T cx = cos(x);
        value = factor * (y * rs * (sx - cx) - rc * (sx + cx));
    }

    return value;
}

}}} // namespaces

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif // BOOST_MATH_BESSEL_Y1_HPP

