#ifndef DATE_TIME_COMPILER_CONFIG_HPP___
#define DATE_TIME_COMPILER_CONFIG_HPP___

/* Copyright (c) 2002-2004 CrystalClear Software, Inc.
 * Subject to the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)
 * Author: Jeff Garland, Bart Garst
 * $Date$
 */

#include <cstdlib>
#include <boost/config.hpp>
#include <boost/detail/workaround.hpp>


// This file performs some local compiler configurations

#include <boost/date_time/locale_config.hpp> //set up locale configurations

//Set up a configuration parameter for platforms that have 
//GetTimeOfDay
#if defined(BOOST_HAS_GETTIMEOFDAY) || defined(BOOST_HAS_FTIME)
#define BOOST_DATE_TIME_HAS_HIGH_PRECISION_CLOCK
#endif

// To Force no default constructors for date & ptime, un-comment following
//#define DATE_TIME_NO_DEFAULT_CONSTRUCTOR

// Include extensions to date_duration - comment out to remove this feature
#define BOOST_DATE_TIME_OPTIONAL_GREGORIAN_TYPES

#if (defined(BOOST_NO_INCLASS_MEMBER_INITIALIZATION) || BOOST_WORKAROUND( BOOST_BORLANDC,  BOOST_TESTED_AT(0x581) ) )
#define BOOST_DATE_TIME_NO_MEMBER_INIT
#endif

// include these types before we try to re-define them
#include <boost/cstdint.hpp>

//Define INT64_C for compilers that don't have it
#if (!defined(INT64_C))
#define INT64_C(value)  int64_t(value)
#endif


/* Workaround for Borland iterator error. Error was "Cannot convert 'istream *' to 'wistream *' in function istream_iterator<>::istream_iterator() */
#if defined(BOOST_BORLANDC) && defined(BOOST_BCB_WITH_RW_LIB)
#define BOOST_DATE_TIME_NO_WISTREAM_ITERATOR
#endif


// Borland v5.64 does not have the following in std namespace; v5.5.1 does
#if defined(BOOST_BORLANDC) && defined(BOOST_BCB_WITH_STLPORT)
#include <locale>
namespace std {
  using stlport::tolower;
  using stlport::ctype;
  using stlport::use_facet;
}
#endif

// workaround for errors associated with output for date classes 
// modifications and input streaming for time classes. 
// Compilers affected are:
// gcc295, msvc (neither with STLPort), any borland
// 
#if (((defined(__GNUC__) && (__GNUC__ < 3)) || \
      (defined(_MSC_VER) && (_MSC_VER < 1300)) ) && \
      !defined(_STLP_OWN_IOSTREAMS) ) || \
      BOOST_WORKAROUND( BOOST_BORLANDC, BOOST_TESTED_AT(0x581) )
#define BOOST_DATE_TIME_INCLUDE_LIMITED_HEADERS
#endif

// The macro marks up places where compiler complains for missing return statement or
// uninitialized variables after calling to boost::throw_exception.
// BOOST_UNREACHABLE_RETURN doesn't work since even compilers that support
// unreachable statements detection emit such warnings.
#if defined(_MSC_VER)
// Use special MSVC extension to markup unreachable code
#  define BOOST_DATE_TIME_UNREACHABLE_EXPRESSION(x) __assume(false)
#elif !defined(BOOST_NO_UNREACHABLE_RETURN_DETECTION)
// Call to a non-returning function should suppress the warning
#  if defined(BOOST_NO_STDC_NAMESPACE)
namespace std {
    using ::abort;
}
#  endif // defined(BOOST_NO_STDC_NAMESPACE)
#  define BOOST_DATE_TIME_UNREACHABLE_EXPRESSION(x) std::abort()
#else
// For other poor compilers the specified expression is compiled. Usually, this would be a return statement.
#  define BOOST_DATE_TIME_UNREACHABLE_EXPRESSION(x) x
#endif

/* The following handles the definition of the necessary macros
 * for dll building on Win32 platforms.
 * 
 * For code that will be placed in the date_time .dll, 
 * it must be properly prefixed with BOOST_DATE_TIME_DECL.
 * The corresponding .cpp file must have BOOST_DATE_TIME_SOURCE
 * defined before including its header. For examples see:
 * greg_month.hpp & greg_month.cpp
 * 
 */

// we need to import/export our code only if the user has specifically
// asked for it by defining either BOOST_ALL_DYN_LINK if they want all boost
// libraries to be dynamically linked, or BOOST_DATE_TIME_DYN_LINK
// if they want just this one to be dynamically liked:
#if defined(BOOST_ALL_DYN_LINK) || defined(BOOST_DATE_TIME_DYN_LINK)
    // export if this is our own source, otherwise import:
#   ifdef BOOST_DATE_TIME_SOURCE
#     define BOOST_DATE_TIME_DECL BOOST_SYMBOL_EXPORT
#   else
#     define BOOST_DATE_TIME_DECL BOOST_SYMBOL_IMPORT
#   endif  // BOOST_DATE_TIME_SOURCE
#endif  // DYN_LINK
//
// if BOOST_WHATEVER_DECL isn't defined yet define it now:
#ifndef BOOST_DATE_TIME_DECL
#  define BOOST_DATE_TIME_DECL
#endif


#if defined(BOOST_HAS_THREADS) 
#  if defined(_MSC_VER) || defined(__MWERKS__) || defined(__MINGW32__) ||  defined(__BORLANDC__)
     //no reentrant posix functions (eg: localtime_r)
#  elif (!defined(__hpux) || (defined(__hpux) && defined(_REENTRANT)))
#   define BOOST_DATE_TIME_HAS_REENTRANT_STD_FUNCTIONS
#  endif
#endif

#if defined(BOOST_NO_CXX11_NULLPTR)
#  define BOOST_DATE_TIME_NULLPTR 0
#else
#  define BOOST_DATE_TIME_NULLPTR nullptr
#endif

#endif
