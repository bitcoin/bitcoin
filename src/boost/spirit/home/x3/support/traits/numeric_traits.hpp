/*=============================================================================
	Copyright (c) 2001-2011 Hartmut Kaiser

	Distributed under the Boost Software License, Version 1.0. (See accompanying
	file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(BOOST_SPIRIT_X3_NUMERIC_TRAITS_JAN_07_2011_0722AM)
#define BOOST_SPIRIT_X3_NUMERIC_TRAITS_JAN_07_2011_0722AM

#include <boost/config.hpp>
#include <boost/mpl/bool.hpp>
#include <limits>

namespace boost { namespace spirit { namespace x3 { namespace traits
{
    ///////////////////////////////////////////////////////////////////////////
    // Determine if T is a boolean type
    ///////////////////////////////////////////////////////////////////////////
    template <typename T>
    struct is_bool : mpl::false_ {};

    template <typename T>
    struct is_bool<T const> : is_bool<T> {};

    template <>
    struct is_bool<bool> : mpl::true_ {};

    ///////////////////////////////////////////////////////////////////////////
    // Determine if T is a signed integer type
    ///////////////////////////////////////////////////////////////////////////
    template <typename T>
    struct is_int : mpl::false_ {};

    template <typename T>
    struct is_int<T const> : is_int<T> {};

    template <>
    struct is_int<short> : mpl::true_ {};

    template <>
    struct is_int<int> : mpl::true_ {};

    template <>
    struct is_int<long> : mpl::true_ {};

#ifdef BOOST_HAS_LONG_LONG
    template <>
    struct is_int<boost::long_long_type> : mpl::true_ {};
#endif

    ///////////////////////////////////////////////////////////////////////////
    // Determine if T is an unsigned integer type
    ///////////////////////////////////////////////////////////////////////////
    template <typename T>
    struct is_uint : mpl::false_ {};

    template <typename T>
    struct is_uint<T const> : is_uint<T> {};

#if !defined(BOOST_NO_INTRINSIC_WCHAR_T)
    template <>
    struct is_uint<unsigned short> : mpl::true_ {};
#endif

    template <>
    struct is_uint<unsigned int> : mpl::true_ {};

    template <>
    struct is_uint<unsigned long> : mpl::true_ {};

#ifdef BOOST_HAS_LONG_LONG
    template <>
    struct is_uint<boost::ulong_long_type> : mpl::true_ {};
#endif

    ///////////////////////////////////////////////////////////////////////////
    // Determine if T is a floating point type
    ///////////////////////////////////////////////////////////////////////////
    template <typename T>
    struct is_real : mpl::false_ {};

    template <typename T>
    struct is_real<T const> : is_uint<T> {};

    template <>
    struct is_real<float> : mpl::true_ {};

    template <>
    struct is_real<double> : mpl::true_ {};

    template <>
    struct is_real<long double> : mpl::true_ {};

    ///////////////////////////////////////////////////////////////////////////
    // customization points for numeric operations
    ///////////////////////////////////////////////////////////////////////////
    template <typename T, typename Enable = void>
    struct absolute_value;

    template <typename T, typename Enable = void>
    struct is_negative;

    template <typename T, typename Enable = void>
    struct is_zero;

    template <typename T, typename Enable = void>
    struct pow10_helper;

    template <typename T, typename Enable = void>
    struct is_nan;

    template <typename T, typename Enable = void>
    struct is_infinite;

    template <typename T, typename Enable = void>
    struct check_overflow : mpl::bool_<std::numeric_limits<T>::is_bounded> {};
}}}}

#endif
