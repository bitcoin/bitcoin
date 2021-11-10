#ifndef BOOST_SYSTEM_DETAIL_CONFIG_HPP_INCLUDED
#define BOOST_SYSTEM_DETAIL_CONFIG_HPP_INCLUDED

// Copyright 2018 Peter Dimov
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/system for documentation.

#include <boost/config.hpp>
#include <boost/config/workaround.hpp>

// BOOST_SYSTEM_HAS_SYSTEM_ERROR

#if !defined(BOOST_NO_CXX11_HDR_SYSTEM_ERROR) && !defined(BOOST_NO_CXX11_HDR_ATOMIC)
# define BOOST_SYSTEM_HAS_SYSTEM_ERROR
#endif

// BOOST_SYSTEM_NOEXCEPT
// Retained for backward compatibility

#define BOOST_SYSTEM_NOEXCEPT BOOST_NOEXCEPT

// BOOST_SYSTEM_HAS_CONSTEXPR

#if !defined(BOOST_NO_CXX14_CONSTEXPR)
# define BOOST_SYSTEM_HAS_CONSTEXPR
#endif

#if BOOST_WORKAROUND(BOOST_GCC, < 60000)
# undef BOOST_SYSTEM_HAS_CONSTEXPR
#endif

#if defined(BOOST_SYSTEM_HAS_CONSTEXPR)
# define BOOST_SYSTEM_CONSTEXPR constexpr
#else
# define BOOST_SYSTEM_CONSTEXPR
#endif

// BOOST_SYSTEM_DEPRECATED

#if defined(__clang__)
# define BOOST_SYSTEM_DEPRECATED(msg) __attribute__((deprecated(msg)))
#elif defined(__GNUC__)
# if __GNUC__ * 100 + __GNUC_MINOR__ >= 405
#  define BOOST_SYSTEM_DEPRECATED(msg) __attribute__((deprecated(msg)))
# else
#  define BOOST_SYSTEM_DEPRECATED(msg) __attribute__((deprecated))
# endif
#elif defined(_MSC_VER)
#  define BOOST_SYSTEM_DEPRECATED(msg) __declspec(deprecated(msg))
#elif defined(__sun)
#  define BOOST_SYSTEM_DEPRECATED(msg) __attribute__((deprecated(msg)))
#else
# define BOOST_SYSTEM_DEPRECATED(msg)
#endif

// BOOST_SYSTEM_CLANG_6

#if defined(__clang__) && (__clang_major__ < 7 || (defined(__APPLE__) && __clang_major__ < 11))
# define BOOST_SYSTEM_CLANG_6
#endif

#endif // BOOST_SYSTEM_DETAIL_CONFIG_HPP_INCLUDED
