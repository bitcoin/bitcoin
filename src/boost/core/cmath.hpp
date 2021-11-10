#ifndef BOOST_CORE_CMATH_HPP_INCLUDED
#define BOOST_CORE_CMATH_HPP_INCLUDED

// MS compatible compilers support #pragma once

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
# pragma once
#endif

// boost/core/cmath.hpp
//
// Floating point classification and sign manipulation functions
// Extracted from https://github.com/boostorg/lexical_cast/pull/37
//
// Copyright 2020 Peter Dimov
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include <cmath>
#if defined(_MSC_VER) && _MSC_VER < 1800
# include <float.h>
#endif

namespace boost
{
namespace core
{
#if defined(_MSC_VER) && _MSC_VER < 1800

template<class T> T copysign( T x, T y )
{
    return static_cast<T>( _copysign( static_cast<double>( x ), static_cast<double>( y ) ) );
}

template<class T> bool isnan( T x )
{
    return _isnan( static_cast<double>( x ) ) != 0;
}

template<class T> bool isfinite( T x )
{
    return _finite( static_cast<double>( x ) ) != 0;
}

template<class T> bool isinf( T x )
{
    return ( _fpclass( static_cast<double>( x ) ) & ( _FPCLASS_PINF | _FPCLASS_NINF ) ) != 0;
}

inline bool isnormal( float x )
{
    // no _fpclassf in 32 bit mode
    unsigned y = reinterpret_cast< unsigned const& >( x );
    unsigned exp = ( y >> 23 ) & 0xFF;
    return exp != 0 && exp != 0xFF;
}

inline bool isnormal( double x )
{
    return ( _fpclass( x ) & ( _FPCLASS_PN | _FPCLASS_NN ) ) != 0;
}

inline bool isnormal( long double x )
{
    return boost::core::isnormal( static_cast<double>( x ) );
}

template<class T> bool signbit( T x )
{
    return _copysign( 1.0, static_cast<double>( x ) ) < 0.0;
}

int const fp_zero = 0;
int const fp_subnormal = 1;
int const fp_normal = 2;
int const fp_infinite = 3;
int const fp_nan = 4;

inline int fpclassify( float x )
{
    switch( _fpclass( x ) )
    {
    case _FPCLASS_SNAN:
    case _FPCLASS_QNAN:

        return fp_nan;

    case _FPCLASS_NINF:
    case _FPCLASS_PINF:

        return fp_infinite;

    case _FPCLASS_NZ:
    case _FPCLASS_PZ:

        return fp_zero;

    default:

        return boost::core::isnormal( x )? fp_normal: fp_subnormal;
    }
}

inline int fpclassify( double x )
{
    switch( _fpclass( x ) )
    {
    case _FPCLASS_SNAN:
    case _FPCLASS_QNAN:

        return fp_nan;

    case _FPCLASS_NINF:
    case _FPCLASS_PINF:

        return fp_infinite;

    case _FPCLASS_NZ:
    case _FPCLASS_PZ:

        return fp_zero;

    case _FPCLASS_ND:
    case _FPCLASS_PD:

        return fp_subnormal;

    default:

        return fp_normal;
    }
}

inline int fpclassify( long double x )
{
    return boost::core::fpclassify( static_cast<double>( x ) );
}

#else

using std::isfinite;
using std::isnan;
using std::isinf;
using std::isnormal;
using std::fpclassify;

int const fp_zero = FP_ZERO;
int const fp_subnormal = FP_SUBNORMAL;
int const fp_normal = FP_NORMAL;
int const fp_infinite = FP_INFINITE;
int const fp_nan = FP_NAN;

using std::signbit;

// std::copysign doesn't exist in libstdc++ under -std=c++03

#if !defined(__GNUC__)

template<class T> T copysign( T x, T y )
{
    return std::copysign( x, y );
}

#else

namespace detail
{

// ::copysignl is unreliable, use the built-ins

inline float copysign_impl( float x, float y )
{
    return __builtin_copysignf( x, y );
}

inline double copysign_impl( double x, double y )
{
    return __builtin_copysign( x, y );
}

inline long double copysign_impl( long double x, long double y )
{
    return __builtin_copysignl( x, y );
}

} // namespace detail

template<class T> T copysign( T x, T y )
{
    return boost::core::detail::copysign_impl( x, y );
}

#endif // !defined(__GNUC__)
#endif // #if defined(_MSC_VER) && _MSC_VER < 1800

} // namespace core
} // namespace boost

#endif  // #ifndef BOOST_CORE_CMATH_HPP_INCLUDED
