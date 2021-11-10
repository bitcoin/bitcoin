//  ratio_fwd.hpp  ---------------------------------------------------------------//

//  Copyright 2008 Howard Hinnant
//  Copyright 2008 Beman Dawes
//  Copyright 2009 Vicente J. Botet Escriba

//  Distributed under the Boost Software License, Version 1.0.
//  See http://www.boost.org/LICENSE_1_0.txt

/*

This code was derived by Beman Dawes from Howard Hinnant's time2_demo prototype.
Many thanks to Howard for making his code available under the Boost license.
The original code was modified to conform to Boost conventions and to section
20.4 Compile-time rational arithmetic [ratio], of the C++ committee working
paper N2798.
See http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2008/n2798.pdf.

time2_demo contained this comment:

    Much thanks to Andrei Alexandrescu,
                   Walter Brown,
                   Peter Dimov,
                   Jeff Garland,
                   Terry Golubiewski,
                   Daniel Krugler,
                   Anthony Williams.
*/

// The way overflow is managed for ratio_less is taken from llvm/libcxx/include/ratio

#ifndef BOOST_RATIO_RATIO_FWD_HPP
#define BOOST_RATIO_RATIO_FWD_HPP

#include <boost/ratio/config.hpp>

#if defined(__GNUC__) && (__GNUC__ >= 4)
#pragma GCC system_header
#endif

namespace boost
{

//----------------------------------------------------------------------------//
//                                                                            //
//              20.6 Compile-time rational arithmetic [ratio]                 //
//                                                                            //
//----------------------------------------------------------------------------//

// ratio
template <boost::intmax_t N, boost::intmax_t D = 1> class ratio;

// ratio arithmetic
template <class R1, class R2> struct ratio_add;
template <class R1, class R2> struct ratio_subtract;
template <class R1, class R2> struct ratio_multiply;
template <class R1, class R2> struct ratio_divide;
#ifdef BOOST_RATIO_EXTENSIONS
template <class R1, class R2> struct ratio_gcd;
template <class R1, class R2> struct ratio_lcm;
template <class R> struct ratio_negate;
template <class R> struct ratio_abs;
template <class R> struct ratio_sign;
template <class R, int P> struct ratio_power;
#endif

// ratio comparison
template <class R1, class R2> struct ratio_equal;
template <class R1, class R2> struct ratio_not_equal;
template <class R1, class R2> struct ratio_less;
template <class R1, class R2> struct ratio_less_equal;
template <class R1, class R2> struct ratio_greater;
template <class R1, class R2> struct ratio_greater_equal;

// convenience SI typedefs
typedef ratio<BOOST_RATIO_INTMAX_C(1), BOOST_RATIO_INTMAX_C(1000000000000000000)> atto;
typedef ratio<BOOST_RATIO_INTMAX_C(1),    BOOST_RATIO_INTMAX_C(1000000000000000)> femto;
typedef ratio<BOOST_RATIO_INTMAX_C(1),       BOOST_RATIO_INTMAX_C(1000000000000)> pico;
typedef ratio<BOOST_RATIO_INTMAX_C(1),          BOOST_RATIO_INTMAX_C(1000000000)> nano;
typedef ratio<BOOST_RATIO_INTMAX_C(1),             BOOST_RATIO_INTMAX_C(1000000)> micro;
typedef ratio<BOOST_RATIO_INTMAX_C(1),                BOOST_RATIO_INTMAX_C(1000)> milli;
typedef ratio<BOOST_RATIO_INTMAX_C(1),                 BOOST_RATIO_INTMAX_C(100)> centi;
typedef ratio<BOOST_RATIO_INTMAX_C(1),                  BOOST_RATIO_INTMAX_C(10)> deci;
typedef ratio<                 BOOST_RATIO_INTMAX_C(10), BOOST_RATIO_INTMAX_C(1)> deca;
typedef ratio<                BOOST_RATIO_INTMAX_C(100), BOOST_RATIO_INTMAX_C(1)> hecto;
typedef ratio<               BOOST_RATIO_INTMAX_C(1000), BOOST_RATIO_INTMAX_C(1)> kilo;
typedef ratio<            BOOST_RATIO_INTMAX_C(1000000), BOOST_RATIO_INTMAX_C(1)> mega;
typedef ratio<         BOOST_RATIO_INTMAX_C(1000000000), BOOST_RATIO_INTMAX_C(1)> giga;
typedef ratio<      BOOST_RATIO_INTMAX_C(1000000000000), BOOST_RATIO_INTMAX_C(1)> tera;
typedef ratio<   BOOST_RATIO_INTMAX_C(1000000000000000), BOOST_RATIO_INTMAX_C(1)> peta;
typedef ratio<BOOST_RATIO_INTMAX_C(1000000000000000000), BOOST_RATIO_INTMAX_C(1)> exa;

#ifdef BOOST_RATIO_EXTENSIONS

#define BOOST_RATIO_1024 BOOST_RATIO_INTMAX_C(1024)

// convenience IEC typedefs
typedef ratio<                                                                                     BOOST_RATIO_1024> kibi;
typedef ratio<                                                                    BOOST_RATIO_1024*BOOST_RATIO_1024> mebi;
typedef ratio<                                                   BOOST_RATIO_1024*BOOST_RATIO_1024*BOOST_RATIO_1024> gibi;
typedef ratio<                                  BOOST_RATIO_1024*BOOST_RATIO_1024*BOOST_RATIO_1024*BOOST_RATIO_1024> tebi;
typedef ratio<                 BOOST_RATIO_1024*BOOST_RATIO_1024*BOOST_RATIO_1024*BOOST_RATIO_1024*BOOST_RATIO_1024> pebi;
typedef ratio<BOOST_RATIO_1024*BOOST_RATIO_1024*BOOST_RATIO_1024*BOOST_RATIO_1024*BOOST_RATIO_1024*BOOST_RATIO_1024> exbi;

#endif
}  // namespace boost


#endif  // BOOST_RATIO_HPP
