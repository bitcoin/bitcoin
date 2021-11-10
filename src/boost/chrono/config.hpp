//  boost/chrono/config.hpp  -------------------------------------------------//

//  Copyright Beman Dawes 2003, 2006, 2008
//  Copyright 2009-2011 Vicente J. Botet Escriba
//  Copyright (c) Microsoft Corporation 2014

//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/chrono for documentation.

#ifndef BOOST_CHRONO_CONFIG_HPP
#define BOOST_CHRONO_CONFIG_HPP

#include <boost/config.hpp>
#include <boost/predef.h>

#if !defined BOOST_CHRONO_VERSION
#define BOOST_CHRONO_VERSION 1
#else
#if BOOST_CHRONO_VERSION!=1  && BOOST_CHRONO_VERSION!=2
#error "BOOST_CHRONO_VERSION must be 1 or 2"
#endif
#endif

#if defined(BOOST_CHRONO_SOURCE) && !defined(BOOST_USE_WINDOWS_H)
#define BOOST_USE_WINDOWS_H
#endif

#if ! defined BOOST_CHRONO_PROVIDES_DATE_IO_FOR_SYSTEM_CLOCK_TIME_POINT \
    && ! defined BOOST_CHRONO_DONT_PROVIDE_DATE_IO_FOR_SYSTEM_CLOCK_TIME_POINT

# define BOOST_CHRONO_PROVIDES_DATE_IO_FOR_SYSTEM_CLOCK_TIME_POINT

#endif

//  BOOST_CHRONO_POSIX_API, BOOST_CHRONO_MAC_API, or BOOST_CHRONO_WINDOWS_API
//  can be defined by the user to specify which API should be used

#if defined(BOOST_CHRONO_WINDOWS_API)
# warning Boost.Chrono will use the Windows API
#elif defined(BOOST_CHRONO_MAC_API)
# warning Boost.Chrono will use the Mac API
#elif defined(BOOST_CHRONO_POSIX_API)
# warning Boost.Chrono will use the POSIX API
#endif

# if defined( BOOST_CHRONO_WINDOWS_API ) && defined( BOOST_CHRONO_POSIX_API )
#   error both BOOST_CHRONO_WINDOWS_API and BOOST_CHRONO_POSIX_API are defined
# elif defined( BOOST_CHRONO_WINDOWS_API ) && defined( BOOST_CHRONO_MAC_API )
#   error both BOOST_CHRONO_WINDOWS_API and BOOST_CHRONO_MAC_API are defined
# elif defined( BOOST_CHRONO_MAC_API ) && defined( BOOST_CHRONO_POSIX_API )
#   error both BOOST_CHRONO_MAC_API and BOOST_CHRONO_POSIX_API are defined
# elif !defined( BOOST_CHRONO_WINDOWS_API ) && !defined( BOOST_CHRONO_MAC_API ) && !defined( BOOST_CHRONO_POSIX_API )
#   if (defined(_WIN32) || defined(__WIN32__) || defined(WIN32))
#     define BOOST_CHRONO_WINDOWS_API
#   elif defined(macintosh) || defined(__APPLE__) || defined(__APPLE_CC__)
#     define BOOST_CHRONO_MAC_API
#   else
#     define BOOST_CHRONO_POSIX_API
#   endif
# endif

# if defined( BOOST_CHRONO_WINDOWS_API )
#   ifndef UNDER_CE
#     define BOOST_CHRONO_HAS_PROCESS_CLOCKS
#   endif
#   define BOOST_CHRONO_HAS_CLOCK_STEADY
#   if BOOST_PLAT_WINDOWS_DESKTOP
#     define BOOST_CHRONO_HAS_THREAD_CLOCK
#   endif
#   define BOOST_CHRONO_THREAD_CLOCK_IS_STEADY true
# endif

# if defined( BOOST_CHRONO_MAC_API )
#   define BOOST_CHRONO_HAS_PROCESS_CLOCKS
#   define BOOST_CHRONO_HAS_CLOCK_STEADY
#   define BOOST_CHRONO_HAS_THREAD_CLOCK
#   define BOOST_CHRONO_THREAD_CLOCK_IS_STEADY true
# endif

# if defined( BOOST_CHRONO_POSIX_API )
#   define BOOST_CHRONO_HAS_PROCESS_CLOCKS
#   include <time.h>  //to check for CLOCK_REALTIME and CLOCK_MONOTONIC and _POSIX_THREAD_CPUTIME
#   if defined(CLOCK_MONOTONIC)
#      define BOOST_CHRONO_HAS_CLOCK_STEADY
#   endif
#   if defined(_POSIX_THREAD_CPUTIME) && !defined(BOOST_DISABLE_THREADS)
#     define BOOST_CHRONO_HAS_THREAD_CLOCK
#     define BOOST_CHRONO_THREAD_CLOCK_IS_STEADY true
#   endif
#   if defined(CLOCK_THREAD_CPUTIME_ID) && !defined(BOOST_DISABLE_THREADS)
#     define BOOST_CHRONO_HAS_THREAD_CLOCK
#     define BOOST_CHRONO_THREAD_CLOCK_IS_STEADY true
#   endif
#   if defined(sun) || defined(__sun)
#     undef BOOST_CHRONO_HAS_THREAD_CLOCK
#     undef BOOST_CHRONO_THREAD_CLOCK_IS_STEADY
#   endif
#   if (defined(__HP_aCC) || defined(__GNUC__)) && defined(__hpux)
#     undef BOOST_CHRONO_HAS_THREAD_CLOCK
#     undef BOOST_CHRONO_THREAD_CLOCK_IS_STEADY
#   endif
#   if defined(__VXWORKS__)
#     undef BOOST_CHRONO_HAS_PROCESS_CLOCKS
#   endif
# endif

#if defined(BOOST_CHRONO_THREAD_DISABLED) && defined(BOOST_CHRONO_HAS_THREAD_CLOCK)
#undef BOOST_CHRONO_HAS_THREAD_CLOCK
#undef BOOST_CHRONO_THREAD_CLOCK_IS_STEADY
#endif

// unicode support  ------------------------------//

#if defined(BOOST_NO_CXX11_UNICODE_LITERALS) || defined(BOOST_NO_CXX11_CHAR16_T) || defined(BOOST_NO_CXX11_CHAR32_T)
//~ #define BOOST_CHRONO_HAS_UNICODE_SUPPORT
#else
#define BOOST_CHRONO_HAS_UNICODE_SUPPORT 1
#endif

#ifndef BOOST_CHRONO_LIB_CONSTEXPR
#if defined( BOOST_NO_CXX11_NUMERIC_LIMITS )
#define BOOST_CHRONO_LIB_CONSTEXPR
#elif defined(_LIBCPP_VERSION) &&  !defined(_LIBCPP_CONSTEXPR)
  #define BOOST_CHRONO_LIB_CONSTEXPR
#else
  #define BOOST_CHRONO_LIB_CONSTEXPR BOOST_CONSTEXPR
#endif
#endif

#if defined( BOOST_NO_CXX11_NUMERIC_LIMITS )
#  define BOOST_CHRONO_LIB_NOEXCEPT_OR_THROW throw()
#else
#ifdef BOOST_NO_CXX11_NOEXCEPT
#  define BOOST_CHRONO_LIB_NOEXCEPT_OR_THROW throw()
#else
#  define BOOST_CHRONO_LIB_NOEXCEPT_OR_THROW noexcept
#endif
#endif

#if defined BOOST_CHRONO_PROVIDE_HYBRID_ERROR_HANDLING \
 && defined BOOST_CHRONO_DONT_PROVIDE_HYBRID_ERROR_HANDLING
#error "BOOST_CHRONO_PROVIDE_HYBRID_ERROR_HANDLING && BOOST_CHRONO_PROVIDE_HYBRID_ERROR_HANDLING defined"
#endif

#if defined BOOST_CHRONO_PROVIDES_DEPRECATED_IO_SINCE_V2_0_0 \
 && defined BOOST_CHRONO_DONT_PROVIDES_DEPRECATED_IO_SINCE_V2_0_0
#error "BOOST_CHRONO_PROVIDES_DEPRECATED_IO_SINCE_V2_0_0 && BOOST_CHRONO_DONT_PROVIDES_DEPRECATED_IO_SINCE_V2_0_0 defined"
#endif

#if ! defined BOOST_CHRONO_PROVIDE_HYBRID_ERROR_HANDLING \
 && ! defined BOOST_CHRONO_DONT_PROVIDE_HYBRID_ERROR_HANDLING
#define BOOST_CHRONO_PROVIDE_HYBRID_ERROR_HANDLING
#endif

#if (BOOST_CHRONO_VERSION == 2)
#if ! defined BOOST_CHRONO_PROVIDES_DEPRECATED_IO_SINCE_V2_0_0 \
 && ! defined BOOST_CHRONO_DONT_PROVIDES_DEPRECATED_IO_SINCE_V2_0_0
#define BOOST_CHRONO_DONT_PROVIDES_DEPRECATED_IO_SINCE_V2_0_0
#endif
#endif

#ifdef BOOST_CHRONO_HEADER_ONLY
#define BOOST_CHRONO_INLINE inline
#define BOOST_CHRONO_STATIC inline
#define BOOST_CHRONO_DECL

#else
#define BOOST_CHRONO_INLINE
#define BOOST_CHRONO_STATIC static

//  enable dynamic linking on Windows  ---------------------------------------//

// we need to import/export our code only if the user has specifically
// asked for it by defining either BOOST_ALL_DYN_LINK if they want all boost
// libraries to be dynamically linked, or BOOST_CHRONO_DYN_LINK
// if they want just this one to be dynamically liked:
#if defined(BOOST_ALL_DYN_LINK) || defined(BOOST_CHRONO_DYN_LINK)
// export if this is our own source, otherwise import:
#ifdef BOOST_CHRONO_SOURCE
# define BOOST_CHRONO_DECL BOOST_SYMBOL_EXPORT
#else
# define BOOST_CHRONO_DECL BOOST_SYMBOL_IMPORT
#endif  // BOOST_CHRONO_SOURCE
#endif  // DYN_LINK
//
// if BOOST_CHRONO_DECL isn't defined yet define it now:
#ifndef BOOST_CHRONO_DECL
#define BOOST_CHRONO_DECL
#endif



//  enable automatic library variant selection  ------------------------------//

#if !defined(BOOST_CHRONO_SOURCE) && !defined(BOOST_ALL_NO_LIB) && !defined(BOOST_CHRONO_NO_LIB)
//
// Set the name of our library; this will get undef'ed by auto_link.hpp
// once it's done with it:
//
#define BOOST_LIB_NAME boost_chrono
//
// If we're importing code from a dll, then tell auto_link.hpp about it:
//
#if defined(BOOST_ALL_DYN_LINK) || defined(BOOST_CHRONO_DYN_LINK)
#  define BOOST_DYN_LINK
#endif
//
// And include the header that does the work:
//
#include <boost/config/auto_link.hpp>
#endif  // auto-linking disabled
#endif // BOOST_CHRONO_HEADER_ONLY
#endif // BOOST_CHRONO_CONFIG_HPP

