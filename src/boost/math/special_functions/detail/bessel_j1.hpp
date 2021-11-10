//  Copyright (c) 2006 Xiaogang Zhang
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MATH_BESSEL_J1_HPP
#define BOOST_MATH_BESSEL_J1_HPP

#ifdef _MSC_VER
#pragma once
#endif

#include <boost/math/constants/constants.hpp>
#include <boost/math/tools/rational.hpp>
#include <boost/math/tools/big_constant.hpp>
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

// Bessel function of the first kind of order one
// x <= 8, minimax rational approximations on root-bracketing intervals
// x > 8, Hankel asymptotic expansion in Hart, Computer Approximations, 1968

namespace boost { namespace math{  namespace detail{

template <typename T>
T bessel_j1(T x);

template <class T>
struct bessel_j1_initializer
{
   struct init
   {
      init()
      {
         do_init();
      }
      static void do_init()
      {
         bessel_j1(T(1));
      }
      void force_instantiate()const{}
   };
   static const init initializer;
   static void force_instantiate()
   {
      initializer.force_instantiate();
   }
};

template <class T>
const typename bessel_j1_initializer<T>::init bessel_j1_initializer<T>::initializer;

template <typename T>
T bessel_j1(T x)
{
    bessel_j1_initializer<T>::force_instantiate();

    static const T P1[] = {
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, -1.4258509801366645672e+11)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 6.6781041261492395835e+09)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, -1.1548696764841276794e+08)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 9.8062904098958257677e+05)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, -4.4615792982775076130e+03)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 1.0650724020080236441e+01)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, -1.0767857011487300348e-02))
    };
    static const T Q1[] = {
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 4.1868604460820175290e+12)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 4.2091902282580133541e+10)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 2.0228375140097033958e+08)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 5.9117614494174794095e+05)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 1.0742272239517380498e+03)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 1.0)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 0.0))
    };
    static const T P2[] = {
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, -1.7527881995806511112e+16)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 1.6608531731299018674e+15)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, -3.6658018905416665164e+13)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 3.5580665670910619166e+11)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, -1.8113931269860667829e+09)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 5.0793266148011179143e+06)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, -7.5023342220781607561e+03)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 4.6179191852758252278e+00))
    };
    static const T Q2[] = {
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 1.7253905888447681194e+18)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 1.7128800897135812012e+16)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 8.4899346165481429307e+13)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 2.7622777286244082666e+11)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 6.4872502899596389593e+08)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 1.1267125065029138050e+06)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 1.3886978985861357615e+03)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 1.0))
    };
    static const T PC[] = {
        static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, -4.4357578167941278571e+06)),
        static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, -9.9422465050776411957e+06)),
        static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, -6.6033732483649391093e+06)),
        static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, -1.5235293511811373833e+06)),
        static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, -1.0982405543459346727e+05)),
        static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, -1.6116166443246101165e+03)),
        static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 0.0))
    };
    static const T QC[] = {
        static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, -4.4357578167941278568e+06)),
        static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, -9.9341243899345856590e+06)),
        static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, -6.5853394797230870728e+06)),
        static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, -1.5118095066341608816e+06)),
        static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, -1.0726385991103820119e+05)),
        static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, -1.4550094401904961825e+03)),
        static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 1.0))
    };
    static const T PS[] = {
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 3.3220913409857223519e+04)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 8.5145160675335701966e+04)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 6.6178836581270835179e+04)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 1.8494262873223866797e+04)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 1.7063754290207680021e+03)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 3.5265133846636032186e+01)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 0.0))
    };
    static const T QS[] = {
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 7.0871281941028743574e+05)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 1.8194580422439972989e+06)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 1.4194606696037208929e+06)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 4.0029443582266975117e+05)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 3.7890229745772202641e+04)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 8.6383677696049909675e+02)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 1.0))
    };
    static const T x1  =  static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 3.8317059702075123156e+00)),
                   x2  =  static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 7.0155866698156187535e+00)),
                   x11 =  static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 9.810e+02)),
                   x12 =  static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, -3.2527979248768438556e-04)),
                   x21 =  static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 1.7960e+03)),
                   x22 =  static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, -3.8330184381246462950e-05));

    T value, factor, r, rc, rs, w;

    BOOST_MATH_STD_USING
    using namespace boost::math::tools;
    using namespace boost::math::constants;

    w = abs(x);
    if (x == 0)
    {
        return static_cast<T>(0);
    }
    if (w <= 4)                       // w in (0, 4]
    {
        T y = x * x;
        BOOST_MATH_ASSERT(sizeof(P1) == sizeof(Q1));
        r = evaluate_rational(P1, Q1, y);
        factor = w * (w + x1) * ((w - x11/256) - x12);
        value = factor * r;
    }
    else if (w <= 8)                  // w in (4, 8]
    {
        T y = x * x;
        BOOST_MATH_ASSERT(sizeof(P2) == sizeof(Q2));
        r = evaluate_rational(P2, Q2, y);
        factor = w * (w + x2) * ((w - x21/256) - x22);
        value = factor * r;
    }
    else                                // w in (8, \infty)
    {
        T y = 8 / w;
        T y2 = y * y;
        BOOST_MATH_ASSERT(sizeof(PC) == sizeof(QC));
        BOOST_MATH_ASSERT(sizeof(PS) == sizeof(QS));
        rc = evaluate_rational(PC, QC, y2);
        rs = evaluate_rational(PS, QS, y2);
        factor = 1 / (sqrt(w) * constants::root_pi<T>());
        //
        // What follows is really just:
        //
        // T z = w - 0.75f * pi<T>();
        // value = factor * (rc * cos(z) - y * rs * sin(z));
        //
        // but using the sin/cos addition rules plus constants
        // for the values of sin/cos of 3PI/4 which then cancel
        // out with corresponding terms in "factor".
        //
        T sx = sin(x);
        T cx = cos(x);
        value = factor * (rc * (sx - cx) + y * rs * (sx + cx));
    }

    if (x < 0)
    {
        value *= -1;                 // odd function
    }
    return value;
}

}}} // namespaces

#endif // BOOST_MATH_BESSEL_J1_HPP

