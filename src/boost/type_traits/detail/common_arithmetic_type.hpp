#ifndef BOOST_TYPE_TRAITS_DETAIL_COMMON_ARITHMETIC_TYPE_HPP_INCLUDED
#define BOOST_TYPE_TRAITS_DETAIL_COMMON_ARITHMETIC_TYPE_HPP_INCLUDED

//
//  Copyright 2015 Peter Dimov
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//

#include <boost/config.hpp>

namespace boost
{

namespace type_traits_detail
{

template<int I> struct arithmetic_type;

// Types bool, char, char16_t, char32_t, wchar_t,
// and the signed and unsigned integer types are
// collectively called integral types

template<> struct arithmetic_type<1>
{
    typedef bool type;
    typedef char (&result_type) [1];
};

template<> struct arithmetic_type<2>
{
    typedef char type;
    typedef char (&result_type) [2];
};

#ifndef BOOST_NO_INTRINSIC_WCHAR_T

template<> struct arithmetic_type<3>
{
    typedef wchar_t type;
    typedef char (&result_type) [3];
};

#endif

// There are five standard signed integer types:
// "signed char", "short int", "int", "long int", and "long long int".

template<> struct arithmetic_type<4>
{
    typedef signed char type;
    typedef char (&result_type) [4];
};

template<> struct arithmetic_type<5>
{
    typedef short type;
    typedef char (&result_type) [5];
};

template<> struct arithmetic_type<6>
{
    typedef int type;
    typedef char (&result_type) [6];
};

template<> struct arithmetic_type<7>
{
    typedef long type;
    typedef char (&result_type) [7];
};

template<> struct arithmetic_type<8>
{
    typedef boost::long_long_type type;
    typedef char (&result_type) [8];
};

// For each of the standard signed integer types, there exists a corresponding
// (but different) standard unsigned integer type: "unsigned char", "unsigned short int",
// "unsigned int", "unsigned long int", and "unsigned long long int"

template<> struct arithmetic_type<9>
{
    typedef unsigned char type;
    typedef char (&result_type) [9];
};

template<> struct arithmetic_type<10>
{
    typedef unsigned short type;
    typedef char (&result_type) [10];
};

template<> struct arithmetic_type<11>
{
    typedef unsigned int type;
    typedef char (&result_type) [11];
};

template<> struct arithmetic_type<12>
{
    typedef unsigned long type;
    typedef char (&result_type) [12];
};

template<> struct arithmetic_type<13>
{
    typedef boost::ulong_long_type type;
    typedef char (&result_type) [13];
};

// There are three floating point types: float, double, and long double.

template<> struct arithmetic_type<14>
{
    typedef float type;
    typedef char (&result_type) [14];
};

template<> struct arithmetic_type<15>
{
    typedef double type;
    typedef char (&result_type) [15];
};

template<> struct arithmetic_type<16>
{
    typedef long double type;
    typedef char (&result_type) [16];
};

#if !defined( BOOST_NO_CXX11_CHAR16_T )

template<> struct arithmetic_type<17>
{
    typedef char16_t type;
    typedef char (&result_type) [17];
};

#endif

#if !defined( BOOST_NO_CXX11_CHAR32_T )

template<> struct arithmetic_type<18>
{
    typedef char32_t type;
    typedef char (&result_type) [18];
};

#endif

#if defined( BOOST_HAS_INT128 )

template<> struct arithmetic_type<19>
{
    typedef boost::int128_type type;
    typedef char (&result_type) [19];
};

template<> struct arithmetic_type<20>
{
    typedef boost::uint128_type type;
    typedef char (&result_type) [20];
};

#endif

template<class T, class U> class common_arithmetic_type
{
private:

    static arithmetic_type<1>::result_type select( arithmetic_type<1>::type );
    static arithmetic_type<2>::result_type select( arithmetic_type<2>::type );
#ifndef BOOST_NO_INTRINSIC_WCHAR_T
    static arithmetic_type<3>::result_type select( arithmetic_type<3>::type );
#endif
    static arithmetic_type<4>::result_type select( arithmetic_type<4>::type );
    static arithmetic_type<5>::result_type select( arithmetic_type<5>::type );
    static arithmetic_type<6>::result_type select( arithmetic_type<6>::type );
    static arithmetic_type<7>::result_type select( arithmetic_type<7>::type );
    static arithmetic_type<8>::result_type select( arithmetic_type<8>::type );
    static arithmetic_type<9>::result_type select( arithmetic_type<9>::type );
    static arithmetic_type<10>::result_type select( arithmetic_type<10>::type );
    static arithmetic_type<11>::result_type select( arithmetic_type<11>::type );
    static arithmetic_type<12>::result_type select( arithmetic_type<12>::type );
    static arithmetic_type<13>::result_type select( arithmetic_type<13>::type );
    static arithmetic_type<14>::result_type select( arithmetic_type<14>::type );
    static arithmetic_type<15>::result_type select( arithmetic_type<15>::type );
    static arithmetic_type<16>::result_type select( arithmetic_type<16>::type );

#if !defined( BOOST_NO_CXX11_CHAR16_T )
    static arithmetic_type<17>::result_type select( arithmetic_type<17>::type );
#endif

#if !defined( BOOST_NO_CXX11_CHAR32_T )
    static arithmetic_type<18>::result_type select( arithmetic_type<18>::type );
#endif

#if defined( BOOST_HAS_INT128 )
    static arithmetic_type<19>::result_type select( arithmetic_type<19>::type );
    static arithmetic_type<20>::result_type select( arithmetic_type<20>::type );
#endif

    static bool cond();

    BOOST_STATIC_CONSTANT(int, selector = sizeof(select(cond() ? T() : U())));

public:

    typedef typename arithmetic_type<selector>::type type;
};

} // namespace type_traits_detail

} // namespace boost

#endif // #ifndef BOOST_TYPE_TRAITS_DETAIL_COMMON_ARITHMETIC_TYPE_HPP_INCLUDED
