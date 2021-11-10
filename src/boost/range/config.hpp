// Boost.Range library
//
//  Copyright Thorsten Ottosen 2003-2004. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org/libs/range/
//

#ifndef BOOST_RANGE_CONFIG_HPP
#define BOOST_RANGE_CONFIG_HPP

#include <boost/detail/workaround.hpp>

#if defined(_MSC_VER)
# pragma once
#endif

#include <boost/config.hpp>

#ifdef BOOST_RANGE_DEDUCED_TYPENAME
#error "macro already defined!"
#endif

#if BOOST_WORKAROUND(BOOST_BORLANDC, BOOST_TESTED_AT(0x564))
# define BOOST_RANGE_DEDUCED_TYPENAME typename
#else
#define BOOST_RANGE_DEDUCED_TYPENAME BOOST_DEDUCED_TYPENAME
#endif

#ifdef BOOST_RANGE_NO_ARRAY_SUPPORT
#error "macro already defined!"
#endif

#if BOOST_WORKAROUND( __MWERKS__, <= 0x3003 )
#define BOOST_RANGE_NO_ARRAY_SUPPORT 1
#endif

#ifdef BOOST_RANGE_NO_ARRAY_SUPPORT
#define BOOST_RANGE_ARRAY_REF() (boost_range_array)
#define BOOST_RANGE_NO_STATIC_ASSERT
#else
#define BOOST_RANGE_ARRAY_REF() (&boost_range_array)
#endif

#if defined(__GNUC__) && ((__GNUC__ > 4) || ((__GNUC__ == 4) && (__GNUC_MINOR__ >= 7)))
#  define BOOST_RANGE_UNUSED __attribute__((unused))
#else
#  define BOOST_RANGE_UNUSED
#endif



#endif

