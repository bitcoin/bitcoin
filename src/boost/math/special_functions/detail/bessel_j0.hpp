//  Copyright (c) 2006 Xiaogang Zhang
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MATH_BESSEL_J0_HPP
#define BOOST_MATH_BESSEL_J0_HPP

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

// Bessel function of the first kind of order zero
// x <= 8, minimax rational approximations on root-bracketing intervals
// x > 8, Hankel asymptotic expansion in Hart, Computer Approximations, 1968

namespace boost { namespace math { namespace detail{

template <typename T>
T bessel_j0(T x);

template <class T>
struct bessel_j0_initializer
{
   struct init
   {
      init()
      {
         do_init();
      }
      static void do_init()
      {
         bessel_j0(T(1));
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
const typename bessel_j0_initializer<T>::init bessel_j0_initializer<T>::initializer;

template <typename T>
T bessel_j0(T x)
{
    bessel_j0_initializer<T>::force_instantiate();
    
    static const T P1[] = {
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, -4.1298668500990866786e+11)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 2.7282507878605942706e+10)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, -6.2140700423540120665e+08)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 6.6302997904833794242e+06)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, -3.6629814655107086448e+04)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 1.0344222815443188943e+02)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, -1.2117036164593528341e-01))
    };
    static const T Q1[] = {
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 2.3883787996332290397e+12)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 2.6328198300859648632e+10)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 1.3985097372263433271e+08)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 4.5612696224219938200e+05)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 9.3614022392337710626e+02)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 1.0)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 0.0))
    };
    static const T P2[] = {
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, -1.8319397969392084011e+03)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, -1.2254078161378989535e+04)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, -7.2879702464464618998e+03)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 1.0341910641583726701e+04)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 1.1725046279757103576e+04)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 4.4176707025325087628e+03)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 7.4321196680624245801e+02)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 4.8591703355916499363e+01))
    };
    static const T Q2[] = {
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, -3.5783478026152301072e+05)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 2.4599102262586308984e+05)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, -8.4055062591169562211e+04)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 1.8680990008359188352e+04)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, -2.9458766545509337327e+03)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 3.3307310774649071172e+02)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, -2.5258076240801555057e+01)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 1.0))
    };
    static const T PC[] = {
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 2.2779090197304684302e+04)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 4.1345386639580765797e+04)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 2.1170523380864944322e+04)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 3.4806486443249270347e+03)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 1.5376201909008354296e+02)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 8.8961548424210455236e-01))
    };
    static const T QC[] = {
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 2.2779090197304684318e+04)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 4.1370412495510416640e+04)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 2.1215350561880115730e+04)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 3.5028735138235608207e+03)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 1.5711159858080893649e+02)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 1.0))
    };
    static const T PS[] = {
        static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, -8.9226600200800094098e+01)),
        static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, -1.8591953644342993800e+02)),
        static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, -1.1183429920482737611e+02)),
        static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, -2.2300261666214198472e+01)),
        static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, -1.2441026745835638459e+00)),
        static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, -8.8033303048680751817e-03))
    };
    static const T QS[] = {
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 5.7105024128512061905e+03)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 1.1951131543434613647e+04)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 7.2642780169211018836e+03)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 1.4887231232283756582e+03)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 9.0593769594993125859e+01)),
         static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 1.0))
    };
    static const T x1  =  static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 2.4048255576957727686e+00)),
                   x2  =  static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 5.5200781102863106496e+00)),
                   x11 =  static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 6.160e+02)),
                   x12 =  static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, -1.42444230422723137837e-03)),
                   x21 =  static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 1.4130e+03)),
                   x22 =  static_cast<T>(BOOST_MATH_BIG_CONSTANT(T, 64, 5.46860286310649596604e-04));

    T value, factor, r, rc, rs;

    BOOST_MATH_STD_USING
    using namespace boost::math::tools;
    using namespace boost::math::constants;

    if (x < 0)
    {
        x = -x;                         // even function
    }
    if (x == 0)
    {
        return static_cast<T>(1);
    }
    if (x <= 4)                       // x in (0, 4]
    {
        T y = x * x;
        BOOST_MATH_ASSERT(sizeof(P1) == sizeof(Q1));
        r = evaluate_rational(P1, Q1, y);
        factor = (x + x1) * ((x - x11/256) - x12);
        value = factor * r;
    }
    else if (x <= 8.0)                  // x in (4, 8]
    {
        T y = 1 - (x * x)/64;
        BOOST_MATH_ASSERT(sizeof(P2) == sizeof(Q2));
        r = evaluate_rational(P2, Q2, y);
        factor = (x + x2) * ((x - x21/256) - x22);
        value = factor * r;
    }
    else                                // x in (8, \infty)
    {
        T y = 8 / x;
        T y2 = y * y;
        BOOST_MATH_ASSERT(sizeof(PC) == sizeof(QC));
        BOOST_MATH_ASSERT(sizeof(PS) == sizeof(QS));
        rc = evaluate_rational(PC, QC, y2);
        rs = evaluate_rational(PS, QS, y2);
        factor = constants::one_div_root_pi<T>() / sqrt(x);
        //
        // What follows is really just:
        //
        // T z = x - pi/4;
        // value = factor * (rc * cos(z) - y * rs * sin(z));
        //
        // But using the addition formulae for sin and cos, plus
        // the special values for sin/cos of pi/4.
        //
        T sx = sin(x);
        T cx = cos(x);
        value = factor * (rc * (cx + sx) - y * rs * (sx - cx));
    }

    return value;
}

}}} // namespaces

#endif // BOOST_MATH_BESSEL_J0_HPP

