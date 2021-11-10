#ifndef BOOST_QVM_MATH_HPP_INCLUDED
#define BOOST_QVM_MATH_HPP_INCLUDED

/// Copyright (c) 2008-2021 Emil Dotchevski and Reverge Studios, Inc.

/// Distributed under the Boost Software License, Version 1.0. (See accompanying
/// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <math.h>
#include <boost/qvm/inline.hpp>

namespace boost { namespace qvm {

template <class T> T acos( T );
template <class T> T asin( T );
template <class T> T atan( T );
template <class T> T atan2( T, T );
template <class T> T cos( T );
template <class T> T sin( T );
template <class T> T tan( T );
template <class T> T cosh( T );
template <class T> T sinh( T );
template <class T> T tanh( T );
template <class T> T exp( T );
template <class T> T log( T );
template <class T> T log10( T );
template <class T> T mod( T , T );
template <class T> T pow( T, T );
template <class T> T sqrt( T );
template <class T> T ceil( T );
template <class T> T abs( T );
template <class T> T floor( T );
template <class T> T mod( T, T );
template <class T> T ldexp( T, int );
template <class T> T sign( T );

template <> BOOST_QVM_INLINE_TRIVIAL float acos<float>( float x ) { return ::acosf(x); }
template <> BOOST_QVM_INLINE_TRIVIAL float asin<float>( float x ) { return ::asinf(x); }
template <> BOOST_QVM_INLINE_TRIVIAL float atan<float>( float x ) { return ::atanf(x); }
template <> BOOST_QVM_INLINE_TRIVIAL float atan2<float>( float x, float y ) { return ::atan2f(x,y); }
template <> BOOST_QVM_INLINE_TRIVIAL float cos<float>( float x ) { return ::cosf(x); }
template <> BOOST_QVM_INLINE_TRIVIAL float sin<float>( float x ) { return ::sinf(x); }
template <> BOOST_QVM_INLINE_TRIVIAL float tan<float>( float x ) { return ::tanf(x); }
template <> BOOST_QVM_INLINE_TRIVIAL float cosh<float>( float x ) { return ::coshf(x); }
template <> BOOST_QVM_INLINE_TRIVIAL float sinh<float>( float x ) { return ::sinhf(x); }
template <> BOOST_QVM_INLINE_TRIVIAL float tanh<float>( float x ) { return ::tanhf(x); }
template <> BOOST_QVM_INLINE_TRIVIAL float exp<float>( float x ) { return ::expf(x); }
template <> BOOST_QVM_INLINE_TRIVIAL float log<float>( float x ) { return ::logf(x); }
template <> BOOST_QVM_INLINE_TRIVIAL float log10<float>( float x ) { return ::log10f(x); }
template <> BOOST_QVM_INLINE_TRIVIAL float mod<float>( float x, float y ) { return ::fmodf(x,y); }
template <> BOOST_QVM_INLINE_TRIVIAL float pow<float>( float x, float y ) { return ::powf(x,y); }
template <> BOOST_QVM_INLINE_TRIVIAL float sqrt<float>( float x ) { return ::sqrtf(x); }
template <> BOOST_QVM_INLINE_TRIVIAL float ceil<float>( float x ) { return ::ceilf(x); }
template <> BOOST_QVM_INLINE_TRIVIAL float abs<float>( float x ) { return ::fabsf(x); }
template <> BOOST_QVM_INLINE_TRIVIAL float floor<float>( float x ) { return ::floorf(x); }
template <> BOOST_QVM_INLINE_TRIVIAL float ldexp<float>( float x, int y ) { return ::ldexpf(x,y); }
template <> BOOST_QVM_INLINE_TRIVIAL float sign<float>( float x ) { return x<0 ? -1.f : +1.f; }

template <> BOOST_QVM_INLINE_TRIVIAL double acos<double>( double x ) { return ::acos(x); }
template <> BOOST_QVM_INLINE_TRIVIAL double asin<double>( double x ) { return ::asin(x); }
template <> BOOST_QVM_INLINE_TRIVIAL double atan<double>( double x ) { return ::atan(x); }
template <> BOOST_QVM_INLINE_TRIVIAL double atan2<double>( double x, double y ) { return ::atan2(x,y); }
template <> BOOST_QVM_INLINE_TRIVIAL double cos<double>( double x ) { return ::cos(x); }
template <> BOOST_QVM_INLINE_TRIVIAL double sin<double>( double x ) { return ::sin(x); }
template <> BOOST_QVM_INLINE_TRIVIAL double tan<double>( double x ) { return ::tan(x); }
template <> BOOST_QVM_INLINE_TRIVIAL double cosh<double>( double x ) { return ::cosh(x); }
template <> BOOST_QVM_INLINE_TRIVIAL double sinh<double>( double x ) { return ::sinh(x); }
template <> BOOST_QVM_INLINE_TRIVIAL double tanh<double>( double x ) { return ::tanh(x); }
template <> BOOST_QVM_INLINE_TRIVIAL double exp<double>( double x ) { return ::exp(x); }
template <> BOOST_QVM_INLINE_TRIVIAL double log<double>( double x ) { return ::log(x); }
template <> BOOST_QVM_INLINE_TRIVIAL double log10<double>( double x ) { return ::log10(x); }
template <> BOOST_QVM_INLINE_TRIVIAL double mod<double>( double x, double y ) { return ::fmod(x,y); }
template <> BOOST_QVM_INLINE_TRIVIAL double pow<double>( double x, double y ) { return ::pow(x,y); }
template <> BOOST_QVM_INLINE_TRIVIAL double sqrt<double>( double x ) { return ::sqrt(x); }
template <> BOOST_QVM_INLINE_TRIVIAL double ceil<double>( double x ) { return ::ceil(x); }
template <> BOOST_QVM_INLINE_TRIVIAL double abs<double>( double x ) { return ::fabs(x); }
template <> BOOST_QVM_INLINE_TRIVIAL double floor<double>( double x ) { return ::floor(x); }
template <> BOOST_QVM_INLINE_TRIVIAL double ldexp<double>( double x, int y ) { return ::ldexp(x,y); }
template <> BOOST_QVM_INLINE_TRIVIAL double sign<double>( double x ) { return x<0 ? -1.0 : +1.0; }

template <> BOOST_QVM_INLINE_TRIVIAL long double acos<long double>( long double x ) { return ::acosl(x); }
template <> BOOST_QVM_INLINE_TRIVIAL long double asin<long double>( long double x ) { return ::asinl(x); }
template <> BOOST_QVM_INLINE_TRIVIAL long double atan<long double>( long double x ) { return ::atanl(x); }
template <> BOOST_QVM_INLINE_TRIVIAL long double atan2<long double>( long double x, long double y ) { return ::atan2l(x,y); }
template <> BOOST_QVM_INLINE_TRIVIAL long double cos<long double>( long double x ) { return ::cosl(x); }
template <> BOOST_QVM_INLINE_TRIVIAL long double sin<long double>( long double x ) { return ::sinl(x); }
template <> BOOST_QVM_INLINE_TRIVIAL long double tan<long double>( long double x ) { return ::tanl(x); }
template <> BOOST_QVM_INLINE_TRIVIAL long double cosh<long double>( long double x ) { return ::coshl(x); }
template <> BOOST_QVM_INLINE_TRIVIAL long double sinh<long double>( long double x ) { return ::sinhl(x); }
template <> BOOST_QVM_INLINE_TRIVIAL long double tanh<long double>( long double x ) { return ::tanhl(x); }
template <> BOOST_QVM_INLINE_TRIVIAL long double exp<long double>( long double x ) { return ::expl(x); }
template <> BOOST_QVM_INLINE_TRIVIAL long double log<long double>( long double x ) { return ::logl(x); }
template <> BOOST_QVM_INLINE_TRIVIAL long double log10<long double>( long double x ) { return ::log10l(x); }
template <> BOOST_QVM_INLINE_TRIVIAL long double mod<long double>( long double x, long double y ) { return ::fmodl(x,y); }
template <> BOOST_QVM_INLINE_TRIVIAL long double pow<long double>( long double x, long double y ) { return ::powl(x,y); }
template <> BOOST_QVM_INLINE_TRIVIAL long double sqrt<long double>( long double x ) { return ::sqrtl(x); }
template <> BOOST_QVM_INLINE_TRIVIAL long double ceil<long double>( long double x ) { return ::ceill(x); }
template <> BOOST_QVM_INLINE_TRIVIAL long double abs<long double>( long double x ) { return ::fabsl(x); }
template <> BOOST_QVM_INLINE_TRIVIAL long double floor<long double>( long double x ) { return ::floorl(x); }
template <> BOOST_QVM_INLINE_TRIVIAL long double ldexp<long double>( long double x, int y ) { return ::ldexpl(x,y); }
template <> BOOST_QVM_INLINE_TRIVIAL long double sign<long double>( long double x ) { return x<0 ? -1.0 : +1.0; }

} }

#endif
