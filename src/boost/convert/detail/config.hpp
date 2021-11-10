// Copyright (c) 2009-2020 Vladimir Batov.
// Use, modification and distribution are subject to the Boost Software License,
// Version 1.0. See http://www.boost.org/LICENSE_1_0.txt.

#ifndef BOOST_CONVERT_FORWARD_HPP
#define BOOST_CONVERT_FORWARD_HPP

#include <boost/config.hpp>
#include <boost/version.hpp>
#include <boost/optional.hpp>
#include <type_traits>

#if defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES) || defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
#undef BOOST_CONVERT_CXX11
#else
#define BOOST_CONVERT_CXX11
#endif

// Intel 12.0 and lower have broken SFINAE
#if defined(BOOST_INTEL) && (BOOST_INTEL <= 1200)
#   define BOOST_CONVERT_IS_NOT_SUPPORTED
#endif

// No C++11 support
#if defined(BOOST_GCC_VERSION) && (BOOST_GCC_VERSION <= 40600)
#   define BOOST_CONVERT_IS_NOT_SUPPORTED
#endif

// MSVC-11 and lower have broken SFINAE
#if defined(BOOST_MSVC) && (BOOST_MSVC < 1800)
#   define BOOST_CONVERT_IS_NOT_SUPPORTED
#endif

#if defined(_MSC_VER)

//MSVC++ 7.0  _MSC_VER == 1300
//MSVC++ 7.1  _MSC_VER == 1310 (Visual Studio 2003)
//MSVC++ 8.0  _MSC_VER == 1400 (Visual Studio 2005)
//MSVC++ 9.0  _MSC_VER == 1500 (Visual Studio 2008)
//MSVC++ 10.0 _MSC_VER == 1600 (Visual Studio 2010)
//MSVC++ 11.0 _MSC_VER == 1700 (Visual Studio 2012)
//MSVC++ 12.0 _MSC_VER == 1800 (Visual Studio 2013)
//MSVC++ 14.0 _MSC_VER == 1900 (Visual Studio 2015)
//MSVC++ 15.0 _MSC_VER == 1910 (Visual Studio 2017)

#   pragma warning(disable: 4100) // unreferenced formal parameter
#   pragma warning(disable: 4146) // unary minus operator applied to unsigned type
#   pragma warning(disable: 4180) // qualifier applied to function type has no meaning
#   pragma warning(disable: 4224)
#   pragma warning(disable: 4244)
#   pragma warning(disable: 4800) // forcing value to bool
#   pragma warning(disable: 4996)

#if _MSC_VER < 1900 /* MSVC-14 defines real snprintf()... just about time! */
#   define snprintf _snprintf
#endif

#endif

#endif // BOOST_CONVERT_FORWARD_HPP
