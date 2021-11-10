// Copyright (C) 2001-2003
// William E. Kempf
// Copyright (C) 2011-2013 Vicente J. Botet Escriba
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_THREAD_CONFIG_WEK01032003_HPP
#define BOOST_THREAD_CONFIG_WEK01032003_HPP

#include <boost/config.hpp>
#include <boost/detail/workaround.hpp>
#include <boost/thread/detail/platform.hpp>
#include <boost/thread/detail/thread_safety.hpp>

//#define BOOST_THREAD_DONT_PROVIDE_INTERRUPTIONS
// ATTRIBUTE_MAY_ALIAS

//#if defined(__GNUC__) && !defined(__INTEL_COMPILER)
#if !defined(BOOST_NO_MAY_ALIAS)

  // GCC since 3.3 and some other compilers have may_alias attribute that helps
  // to alleviate optimizer issues with regard to violation of the strict aliasing rules.

  #define BOOST_THREAD_DETAIL_USE_ATTRIBUTE_MAY_ALIAS
#endif
#if defined(BOOST_MAY_ALIAS)
#define BOOST_THREAD_ATTRIBUTE_MAY_ALIAS BOOST_MAY_ALIAS
#else
#define BOOST_THREAD_ATTRIBUTE_MAY_ALIAS
#endif

#if defined(BOOST_THREAD_CHRONO_WINDOWS_API)
# warning Boost.Thread will use the Windows API for time
#elif defined(BOOST_THREAD_CHRONO_MAC_API)
# warning Boost.Thread will use the Mac API  for time
#elif defined(BOOST_THREAD_CHRONO_POSIX_API)
# warning Boost.Thread will use the POSIX API  for time
#endif

# if defined( BOOST_THREAD_CHRONO_WINDOWS_API ) && defined( BOOST_THREAD_CHRONO_POSIX_API )
#   error both BOOST_THREAD_CHRONO_WINDOWS_API and BOOST_THREAD_CHRONO_POSIX_API are defined
# elif defined( BOOST_THREAD_CHRONO_WINDOWS_API ) && defined( BOOST_THREAD_CHRONO_MAC_API )
#   error both BOOST_THREAD_CHRONO_WINDOWS_API and BOOST_THREAD_CHRONO_MAC_API are defined
# elif defined( BOOST_THREAD_CHRONO_MAC_API ) && defined( BOOST_THREAD_CHRONO_POSIX_API )
#   error both BOOST_THREAD_CHRONO_MAC_API and BOOST_THREAD_CHRONO_POSIX_API are defined
# elif !defined( BOOST_THREAD_CHRONO_WINDOWS_API ) && !defined( BOOST_THREAD_CHRONO_MAC_API ) && !defined( BOOST_THREAD_CHRONO_POSIX_API )
#   if defined(BOOST_THREAD_PLATFORM_WIN32)
#     define BOOST_THREAD_CHRONO_WINDOWS_API
#   elif defined(macintosh) || defined(__APPLE__) || defined(__APPLE_CC__)
#     define BOOST_THREAD_CHRONO_MAC_API
#   else
#     define BOOST_THREAD_CHRONO_POSIX_API
#   endif
# endif

#if !defined(BOOST_THREAD_POLL_INTERVAL_MILLISECONDS)
#define BOOST_THREAD_POLL_INTERVAL_MILLISECONDS 100
#endif

#if defined BOOST_THREAD_THROW_IF_PRECONDITION_NOT_SATISFIED
#define BOOST_THREAD_ASSERT_PRECONDITION(EXPR, EX) \
        if (EXPR) {} else boost::throw_exception(EX)
#define BOOST_THREAD_VERIFY_PRECONDITION(EXPR, EX) \
        if (EXPR) {} else boost::throw_exception(EX)
#define BOOST_THREAD_THROW_ELSE_RETURN(EX, RET) \
        boost::throw_exception(EX)
#else
#define BOOST_THREAD_ASSERT_PRECONDITION(EXPR, EX)
#define BOOST_THREAD_VERIFY_PRECONDITION(EXPR, EX) \
        (void)(EXPR)
#define BOOST_THREAD_THROW_ELSE_RETURN(EX, RET) \
        return (RET)
#endif

// This compiler doesn't support Boost.Chrono
#if defined __IBMCPP__ && (__IBMCPP__ < 1100) \
  && ! defined BOOST_THREAD_DONT_USE_CHRONO
#define BOOST_THREAD_DONT_USE_CHRONO
#if ! defined BOOST_THREAD_USES_DATETIME
#define BOOST_THREAD_USES_DATETIME
#endif
#endif

// This compiler doesn't support Boost.Move
#if BOOST_WORKAROUND(__SUNPRO_CC, < 0x5100) \
  && ! defined BOOST_THREAD_DONT_USE_MOVE
#define BOOST_THREAD_DONT_USE_MOVE
#endif

// This compiler doesn't support Boost.Container Allocators files
#if defined __SUNPRO_CC \
  && ! defined BOOST_THREAD_DONT_PROVIDE_FUTURE_CTOR_ALLOCATORS
#define BOOST_THREAD_DONT_PROVIDE_FUTURE_CTOR_ALLOCATORS
#endif

#if defined _WIN32_WCE && _WIN32_WCE==0x501 \
  && ! defined BOOST_THREAD_DONT_PROVIDE_FUTURE_CTOR_ALLOCATORS
#define BOOST_THREAD_DONT_PROVIDE_FUTURE_CTOR_ALLOCATORS
#endif


#if defined BOOST_NO_CXX11_UNIFIED_INITIALIZATION_SYNTAX || defined BOOST_NO_CXX11_HDR_INITIALIZER_LIST
#define BOOST_THREAD_NO_CXX11_HDR_INITIALIZER_LIST
#define BOOST_THREAD_NO_MAKE_LOCK_GUARD
#define BOOST_THREAD_NO_MAKE_STRICT_LOCK
#define BOOST_THREAD_NO_MAKE_NESTED_STRICT_LOCK
#endif

#if defined(BOOST_NO_CXX11_HDR_TUPLE) || defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
#define BOOST_THREAD_NO_MAKE_UNIQUE_LOCKS
#define BOOST_THREAD_NO_SYNCHRONIZE
#elif defined _MSC_VER && _MSC_VER <= 1600
// C++ features supported by VC++ 10 (aka 2010)
#define BOOST_THREAD_NO_MAKE_UNIQUE_LOCKS
#define BOOST_THREAD_NO_SYNCHRONIZE
#endif

/// BASIC_THREAD_ID
#if ! defined BOOST_THREAD_DONT_PROVIDE_BASIC_THREAD_ID \
 && ! defined BOOST_THREAD_PROVIDES_BASIC_THREAD_ID
#define BOOST_THREAD_PROVIDES_BASIC_THREAD_ID
#endif

/// RVALUE_REFERENCES_DONT_MATCH_FUNTION_PTR
//#if defined BOOST_NO_CXX11_RVALUE_REFERENCES || defined BOOST_MSVC
#define BOOST_THREAD_RVALUE_REFERENCES_DONT_MATCH_FUNCTION_PTR
//#endif

// Default version
#if !defined BOOST_THREAD_VERSION
#define BOOST_THREAD_VERSION 2
#else
#if BOOST_THREAD_VERSION!=2  && BOOST_THREAD_VERSION!=3 && BOOST_THREAD_VERSION!=4 && BOOST_THREAD_VERSION!=5
#error "BOOST_THREAD_VERSION must be 2, 3, 4 or 5"
#endif
#endif

// CHRONO
// Uses Boost.Chrono by default if not stated the opposite defining BOOST_THREAD_DONT_USE_CHRONO
#if ! defined BOOST_THREAD_DONT_USE_CHRONO \
  && ! defined BOOST_THREAD_USES_CHRONO
#define BOOST_THREAD_USES_CHRONO
#endif

#if ! defined BOOST_THREAD_DONT_USE_ATOMIC \
  && ! defined BOOST_THREAD_USES_ATOMIC
#define BOOST_THREAD_USES_ATOMIC
//#define BOOST_THREAD_DONT_USE_ATOMIC
#endif

#if defined BOOST_THREAD_USES_ATOMIC
// Andrey Semashev
#define BOOST_THREAD_ONCE_ATOMIC
#else
//#elif ! defined BOOST_NO_CXX11_THREAD_LOCAL && ! defined BOOST_NO_THREAD_LOCAL && ! defined BOOST_THREAD_NO_UINT32_PSEUDO_ATOMIC
// http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2007/n2444.html#Appendix
#define BOOST_THREAD_ONCE_FAST_EPOCH
#endif
#if BOOST_THREAD_VERSION==2

// PROVIDE_PROMISE_LAZY
#if ! defined BOOST_THREAD_DONT_PROVIDE_PROMISE_LAZY \
  && ! defined BOOST_THREAD_PROVIDES_PROMISE_LAZY
#define BOOST_THREAD_PROVIDES_PROMISE_LAZY
#endif

// PROVIDE_THREAD_EQ
#if ! defined BOOST_THREAD_DONT_PROVIDE_THREAD_EQ \
  && ! defined BOOST_THREAD_PROVIDES_THREAD_EQ
#define BOOST_THREAD_PROVIDES_THREAD_EQ
#endif

#endif

#if BOOST_THREAD_VERSION>=3

// ONCE_CXX11
// fixme BOOST_THREAD_PROVIDES_ONCE_CXX11 doesn't works when thread.cpp is compiled BOOST_THREAD_VERSION 3
#if ! defined BOOST_THREAD_DONT_PROVIDE_ONCE_CXX11 \
 && ! defined BOOST_THREAD_PROVIDES_ONCE_CXX11
#define BOOST_THREAD_DONT_PROVIDE_ONCE_CXX11
#endif

// THREAD_DESTRUCTOR_CALLS_TERMINATE_IF_JOINABLE
#if ! defined BOOST_THREAD_DONT_PROVIDE_THREAD_DESTRUCTOR_CALLS_TERMINATE_IF_JOINABLE \
 && ! defined BOOST_THREAD_PROVIDES_THREAD_DESTRUCTOR_CALLS_TERMINATE_IF_JOINABLE
#define BOOST_THREAD_PROVIDES_THREAD_DESTRUCTOR_CALLS_TERMINATE_IF_JOINABLE
#endif

// THREAD_MOVE_ASSIGN_CALLS_TERMINATE_IF_JOINABLE
#if ! defined BOOST_THREAD_DONT_PROVIDE_THREAD_MOVE_ASSIGN_CALLS_TERMINATE_IF_JOINABLE \
 && ! defined BOOST_THREAD_PROVIDES_THREAD_MOVE_ASSIGN_CALLS_TERMINATE_IF_JOINABLE
#define BOOST_THREAD_PROVIDES_THREAD_MOVE_ASSIGN_CALLS_TERMINATE_IF_JOINABLE
#endif

// PROVIDE_FUTURE
#if ! defined BOOST_THREAD_DONT_PROVIDE_FUTURE \
 && ! defined BOOST_THREAD_PROVIDES_FUTURE
#define BOOST_THREAD_PROVIDES_FUTURE
#endif

// FUTURE_CTOR_ALLOCATORS
#if ! defined BOOST_THREAD_DONT_PROVIDE_FUTURE_CTOR_ALLOCATORS \
 && ! defined BOOST_THREAD_PROVIDES_FUTURE_CTOR_ALLOCATORS
#define BOOST_THREAD_PROVIDES_FUTURE_CTOR_ALLOCATORS
#endif

// SHARED_MUTEX_UPWARDS_CONVERSIONS
#if ! defined BOOST_THREAD_DONT_PROVIDE_SHARED_MUTEX_UPWARDS_CONVERSIONS \
 && ! defined BOOST_THREAD_PROVIDES_SHARED_MUTEX_UPWARDS_CONVERSIONS
#define BOOST_THREAD_PROVIDES_SHARED_MUTEX_UPWARDS_CONVERSIONS
#endif

// PROVIDE_EXPLICIT_LOCK_CONVERSION
#if ! defined BOOST_THREAD_DONT_PROVIDE_EXPLICIT_LOCK_CONVERSION \
 && ! defined BOOST_THREAD_PROVIDES_EXPLICIT_LOCK_CONVERSION
#define BOOST_THREAD_PROVIDES_EXPLICIT_LOCK_CONVERSION
#endif

// GENERIC_SHARED_MUTEX_ON_WIN
#if ! defined BOOST_THREAD_DONT_PROVIDE_GENERIC_SHARED_MUTEX_ON_WIN \
 && ! defined BOOST_THREAD_PROVIDES_GENERIC_SHARED_MUTEX_ON_WIN
#define BOOST_THREAD_PROVIDES_GENERIC_SHARED_MUTEX_ON_WIN
#endif

// USE_MOVE
#if ! defined BOOST_THREAD_DONT_USE_MOVE \
 && ! defined BOOST_THREAD_USES_MOVE
#define BOOST_THREAD_USES_MOVE
#endif

#endif

// deprecated since version 4
#if BOOST_THREAD_VERSION < 4

// NESTED_LOCKS
#if ! defined BOOST_THREAD_PROVIDES_NESTED_LOCKS \
 && ! defined BOOST_THREAD_DONT_PROVIDE_NESTED_LOCKS
#define BOOST_THREAD_PROVIDES_NESTED_LOCKS
#endif

// CONDITION
#if ! defined BOOST_THREAD_PROVIDES_CONDITION \
 && ! defined BOOST_THREAD_DONT_PROVIDE_CONDITION
#define BOOST_THREAD_PROVIDES_CONDITION
#endif

// USE_DATETIME
#if ! defined BOOST_THREAD_DONT_USE_DATETIME \
 && ! defined BOOST_THREAD_USES_DATETIME
#define BOOST_THREAD_USES_DATETIME
#endif
#endif

#if BOOST_THREAD_VERSION>=4

// SIGNATURE_PACKAGED_TASK
#if ! defined BOOST_THREAD_PROVIDES_SIGNATURE_PACKAGED_TASK \
 && ! defined BOOST_THREAD_DONT_PROVIDE_SIGNATURE_PACKAGED_TASK
#define BOOST_THREAD_PROVIDES_SIGNATURE_PACKAGED_TASK
#endif

// VARIADIC_THREAD
#if ! defined BOOST_THREAD_PROVIDES_VARIADIC_THREAD \
 && ! defined BOOST_THREAD_DONT_PROVIDE_VARIADIC_THREAD

#if ! defined(BOOST_NO_SFINAE_EXPR) && \
    ! defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES) && \
    ! defined(BOOST_NO_CXX11_DECLTYPE) && \
    ! defined(BOOST_NO_CXX11_DECLTYPE_N3276) && \
    ! defined(BOOST_NO_CXX11_TRAILING_RESULT_TYPES) && \
    ! defined(BOOST_NO_CXX11_RVALUE_REFERENCES) && \
    ! defined(BOOST_NO_CXX11_HDR_TUPLE)

#define BOOST_THREAD_PROVIDES_VARIADIC_THREAD
#endif
#endif

#if ! defined BOOST_THREAD_PROVIDES_FUTURE_WHEN_ALL_WHEN_ANY \
 && ! defined BOOST_THREAD_DONT_PROVIDE_FUTURE_WHEN_ALL_WHEN_ANY

#if ! defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES) && \
    ! defined(BOOST_NO_CXX11_HDR_TUPLE)

#define BOOST_THREAD_PROVIDES_FUTURE_WHEN_ALL_WHEN_ANY
#endif
#endif

//    ! defined(BOOST_NO_SFINAE_EXPR) &&
//    ! defined(BOOST_NO_CXX11_RVALUE_REFERENCES) &&
//    ! defined(BOOST_NO_CXX11_AUTO) &&
//    ! defined(BOOST_NO_CXX11_DECLTYPE) &&
//    ! defined(BOOST_NO_CXX11_DECLTYPE_N3276) &&


// MAKE_READY_AT_THREAD_EXIT
#if ! defined BOOST_THREAD_PROVIDES_MAKE_READY_AT_THREAD_EXIT \
 && ! defined BOOST_THREAD_DONT_PROVIDE_MAKE_READY_AT_THREAD_EXIT

//#if defined BOOST_THREAD_PROVIDES_SIGNATURE_PACKAGED_TASK && defined(BOOST_THREAD_PROVIDES_VARIADIC_THREAD)
#define BOOST_THREAD_PROVIDES_MAKE_READY_AT_THREAD_EXIT
//#endif
#endif

// FUTURE_CONTINUATION
#if ! defined BOOST_THREAD_PROVIDES_FUTURE_CONTINUATION \
 && ! defined BOOST_THREAD_DONT_PROVIDE_FUTURE_CONTINUATION
#define BOOST_THREAD_PROVIDES_FUTURE_CONTINUATION
#endif

#if ! defined BOOST_THREAD_PROVIDES_FUTURE_UNWRAP \
 && ! defined BOOST_THREAD_DONT_PROVIDE_FUTURE_UNWRAP
#define BOOST_THREAD_PROVIDES_FUTURE_UNWRAP
#endif

// FUTURE_INVALID_AFTER_GET
#if ! defined BOOST_THREAD_PROVIDES_FUTURE_INVALID_AFTER_GET \
 && ! defined BOOST_THREAD_DONT_PROVIDE_FUTURE_INVALID_AFTER_GET
#define BOOST_THREAD_PROVIDES_FUTURE_INVALID_AFTER_GET
#endif

// NESTED_LOCKS
#if ! defined BOOST_THREAD_PROVIDES_NESTED_LOCKS \
 && ! defined BOOST_THREAD_DONT_PROVIDE_NESTED_LOCKS
#define BOOST_THREAD_DONT_PROVIDE_NESTED_LOCKS
#endif

// CONDITION
#if ! defined BOOST_THREAD_PROVIDES_CONDITION \
 && ! defined BOOST_THREAD_DONT_PROVIDE_CONDITION
#define BOOST_THREAD_DONT_PROVIDE_CONDITION
#endif

#endif // BOOST_THREAD_VERSION>=4


#if BOOST_THREAD_VERSION>=5
//#define BOOST_THREAD_FUTURE_BLOCKING

#if ! defined BOOST_THREAD_PROVIDES_EXECUTORS \
 && ! defined BOOST_THREAD_DONT_PROVIDE_EXECUTORS
#define BOOST_THREAD_PROVIDES_EXECUTORS
#endif

#else
//#define BOOST_THREAD_FUTURE_BLOCKING
#define BOOST_THREAD_ASYNC_FUTURE_WAITS
#endif


// INTERRUPTIONS
#if ! defined BOOST_THREAD_PROVIDES_INTERRUPTIONS \
 && ! defined BOOST_THREAD_DONT_PROVIDE_INTERRUPTIONS
#define BOOST_THREAD_PROVIDES_INTERRUPTIONS
#endif

// CORRELATIONS

// EXPLICIT_LOCK_CONVERSION.
#if defined BOOST_THREAD_PROVIDES_EXPLICIT_LOCK_CONVERSION
#define BOOST_THREAD_EXPLICIT_LOCK_CONVERSION explicit
#else
#define BOOST_THREAD_EXPLICIT_LOCK_CONVERSION
#endif

// BOOST_THREAD_PROVIDES_GENERIC_SHARED_MUTEX_ON_WIN is defined if BOOST_THREAD_PROVIDES_SHARED_MUTEX_UPWARDS_CONVERSIONS
#if defined BOOST_THREAD_PROVIDES_SHARED_MUTEX_UPWARDS_CONVERSIONS \
&& ! defined BOOST_THREAD_PROVIDES_GENERIC_SHARED_MUTEX_ON_WIN
#define BOOST_THREAD_PROVIDES_GENERIC_SHARED_MUTEX_ON_WIN
#endif

// For C++11 call_once interface the compiler MUST support constexpr.
// Otherwise once_flag would be initialized during dynamic initialization stage, which is not thread-safe.
#if defined(BOOST_THREAD_PROVIDES_ONCE_CXX11)
#if defined(BOOST_NO_CXX11_CONSTEXPR)
#undef BOOST_THREAD_PROVIDES_ONCE_CXX11
#endif
#endif

#if defined(BOOST_THREAD_PLATFORM_WIN32) && defined BOOST_THREAD_DONT_USE_DATETIME
#undef BOOST_THREAD_DONT_USE_DATETIME
#define BOOST_THREAD_USES_DATETIME
#endif

#if defined(BOOST_THREAD_PLATFORM_WIN32) && defined BOOST_THREAD_DONT_USE_CHRONO
#undef BOOST_THREAD_DONT_USE_CHRONO
#define BOOST_THREAD_USES_CHRONO
#endif

// BOOST_THREAD_PROVIDES_DEPRECATED_FEATURES_SINCE_V3_0_0 defined by default up to Boost 1.55
// BOOST_THREAD_DONT_PROVIDE_DEPRECATED_FEATURES_SINCE_V3_0_0 defined by default up to Boost 1.55
#if defined BOOST_THREAD_PROVIDES_DEPRECATED_FEATURES_SINCE_V3_0_0

#if  ! defined BOOST_THREAD_PROVIDES_THREAD_EQ
#define BOOST_THREAD_PROVIDES_THREAD_EQ
#endif

#endif


//#if ! defined BOOST_NO_CXX11_RVALUE_REFERENCES || defined BOOST_THREAD_USES_MOVE
#if ! defined BOOST_NO_CXX11_RVALUE_REFERENCES
#define BOOST_THREAD_FUTURE_USES_OPTIONAL
#endif

#if BOOST_WORKAROUND(BOOST_BORLANDC, < 0x600)
#  pragma warn -8008 // Condition always true/false
#  pragma warn -8080 // Identifier declared but never used
#  pragma warn -8057 // Parameter never used
#  pragma warn -8066 // Unreachable code
#endif

#include <boost/thread/detail/platform.hpp>

#if defined(BOOST_THREAD_PLATFORM_WIN32)
#else
  #   if defined(BOOST_HAS_PTHREAD_DELAY_NP) || defined(BOOST_HAS_NANOSLEEP)
  #     define BOOST_THREAD_SLEEP_FOR_IS_STEADY
  #   endif
#endif

#if defined(BOOST_THREAD_CHRONO_WINDOWS_API)
  #define BOOST_THREAD_HAS_MONO_CLOCK
  #define BOOST_THREAD_INTERNAL_CLOCK_IS_MONO
#elif defined(BOOST_THREAD_CHRONO_MAC_API)
  #define BOOST_THREAD_HAS_MONO_CLOCK
#elif defined(__ANDROID__)
  #define BOOST_THREAD_HAS_MONO_CLOCK
  #if defined(__ANDROID_API__) && __ANDROID_API__ >= 21
    #define BOOST_THREAD_INTERNAL_CLOCK_IS_MONO
  #endif
#else
  #include <time.h> // check for CLOCK_MONOTONIC
  #if defined(CLOCK_MONOTONIC)
    #define BOOST_THREAD_HAS_MONO_CLOCK
    #define BOOST_THREAD_INTERNAL_CLOCK_IS_MONO
  #endif
#endif

#if defined(BOOST_THREAD_PLATFORM_WIN32)
#elif ! defined BOOST_THREAD_INTERNAL_CLOCK_IS_MONO
#if defined BOOST_PTHREAD_HAS_TIMEDLOCK
#define BOOST_THREAD_USES_PTHREAD_TIMEDLOCK
#elif (defined(_POSIX_TIMEOUTS) && (_POSIX_TIMEOUTS-0)>=200112L) \
 || (defined(__ANDROID__) && defined(__ANDROID_API__) && __ANDROID_API__ >= 21)
#define BOOST_THREAD_USES_PTHREAD_TIMEDLOCK
#endif
#endif

// provided for backwards compatibility, since this
// macro was used for several releases by mistake.
#if defined(BOOST_THREAD_DYN_DLL) && ! defined(BOOST_THREAD_DYN_LINK)
# define BOOST_THREAD_DYN_LINK
#endif

// compatibility with the rest of Boost's auto-linking code:
#if defined(BOOST_THREAD_DYN_LINK) || defined(BOOST_ALL_DYN_LINK)
# undef  BOOST_THREAD_USE_LIB
# if !defined(BOOST_THREAD_USE_DLL)
#  define BOOST_THREAD_USE_DLL
# endif
#endif

#if defined(BOOST_THREAD_BUILD_DLL)   //Build dll
#elif defined(BOOST_THREAD_BUILD_LIB) //Build lib
#elif defined(BOOST_THREAD_USE_DLL)   //Use dll
#elif defined(BOOST_THREAD_USE_LIB)   //Use lib
#else //Use default
#   if defined(BOOST_THREAD_PLATFORM_WIN32)
#       if defined(BOOST_MSVC) || defined(BOOST_INTEL_WIN) \
      || defined(__MINGW32__) || defined(MINGW32) || defined(BOOST_MINGW32) \
      || (defined(_MSC_VER) && defined(__clang__))
      //For compilers supporting auto-tss cleanup
            //with Boost.Threads lib, use Boost.Threads lib
#           define BOOST_THREAD_USE_LIB
#       else
            //For compilers not yet supporting auto-tss cleanup
            //with Boost.Threads lib, use Boost.Threads dll
#           define BOOST_THREAD_USE_DLL
#       endif
#   else
#       define BOOST_THREAD_USE_LIB
#   endif
#endif

#if defined(BOOST_HAS_DECLSPEC)
#   if defined(BOOST_THREAD_BUILD_DLL) //Build dll
#       define BOOST_THREAD_DECL BOOST_SYMBOL_EXPORT
//#       define BOOST_THREAD_DECL __declspec(dllexport)

#   elif defined(BOOST_THREAD_USE_DLL) //Use dll
#       define BOOST_THREAD_DECL BOOST_SYMBOL_IMPORT
//#       define BOOST_THREAD_DECL __declspec(dllimport)
#   else
#       define BOOST_THREAD_DECL
#   endif
#elif (__GNUC__ == 4 && __GNUC_MINOR__ >= 1) || (__GNUC__ > 4)
#  define BOOST_THREAD_DECL BOOST_SYMBOL_VISIBLE

#else
#   define BOOST_THREAD_DECL
#endif // BOOST_HAS_DECLSPEC

//
// Automatically link to the correct build variant where possible.
//
#if !defined(BOOST_ALL_NO_LIB) && !defined(BOOST_THREAD_NO_LIB) && !defined(BOOST_THREAD_BUILD_DLL) && !defined(BOOST_THREAD_BUILD_LIB)
//
// Tell the autolink to link dynamically, this will get undef'ed by auto_link.hpp
// once it's done with it:
//
#if defined(BOOST_THREAD_USE_DLL) &  ! defined(BOOST_DYN_LINK)
#   define BOOST_DYN_LINK
#endif
//
// Set the name of our library, this will get undef'ed by auto_link.hpp
// once it's done with it:
//
#if defined(BOOST_THREAD_LIB_NAME)
#    define BOOST_LIB_NAME BOOST_THREAD_LIB_NAME
#else
#    define BOOST_LIB_NAME boost_thread
#endif
//
// If we're importing code from a dll, then tell auto_link.hpp about it:
//
// And include the header that does the work:
//
#include <boost/config/auto_link.hpp>
#endif  // auto-linking disabled

#endif // BOOST_THREAD_CONFIG_WEK1032003_HPP

// Change Log:
//   22 Jan 05 Roland Schwarz (speedsnail)
//      Usage of BOOST_HAS_DECLSPEC macro.
//      Default again is static lib usage.
//      BOOST_DYN_LINK only defined when autolink included.
