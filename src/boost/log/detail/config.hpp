/*
 *          Copyright Andrey Semashev 2007 - 2015.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   config.hpp
 * \author Andrey Semashev
 * \date   08.03.2007
 *
 * \brief  This header is the Boost.Log library implementation, see the library documentation
 *         at http://www.boost.org/doc/libs/release/libs/log/doc/html/index.html. In this file
 *         internal configuration macros are defined.
 */

#ifndef BOOST_LOG_DETAIL_CONFIG_HPP_INCLUDED_
#define BOOST_LOG_DETAIL_CONFIG_HPP_INCLUDED_

#include <boost/predef/os.h>

// Try including WinAPI config as soon as possible so that any other headers don't include Windows SDK headers
#if defined(BOOST_OS_WINDOWS_AVAILABLE)
#include <boost/winapi/config.hpp>
#endif

#include <limits.h> // To bring in libc macros
#include <boost/config.hpp>

// The library requires dynamic_cast in a few places
#if defined(BOOST_NO_RTTI)
#   error Boost.Log: RTTI is required by the library
#endif

#if defined(_MSC_VER) && _MSC_VER >= 1600
#   define BOOST_LOG_HAS_PRAGMA_DETECT_MISMATCH
#endif

#if defined(BOOST_LOG_HAS_PRAGMA_DETECT_MISMATCH)
#include <boost/preprocessor/stringize.hpp>
#endif

#if !defined(BOOST_WINDOWS)
#   ifndef BOOST_LOG_WITHOUT_DEBUG_OUTPUT
#       define BOOST_LOG_WITHOUT_DEBUG_OUTPUT
#   endif
#   ifndef BOOST_LOG_WITHOUT_EVENT_LOG
#       define BOOST_LOG_WITHOUT_EVENT_LOG
#   endif
#endif

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

#if defined(BOOST_MSVC)
    // For some reason MSVC 9.0 fails to link the library if static integral constants are defined in cpp
#   define BOOST_LOG_BROKEN_STATIC_CONSTANTS_LINKAGE
#   if _MSC_VER <= 1310
        // MSVC 7.1 sometimes fails to match out-of-class template function definitions with
        // their declarations if the return type or arguments of the functions involve typename keyword
        // and depend on the template parameters.
#       define BOOST_LOG_BROKEN_TEMPLATE_DEFINITION_MATCHING
#   endif
#   if _MSC_VER <= 1400
        // Older MSVC versions reject friend declarations for class template specializations
#       define BOOST_LOG_BROKEN_FRIEND_TEMPLATE_SPECIALIZATIONS
#   endif
#   if _MSC_VER <= 1600
        // MSVC up to 10.0 attempts to invoke copy constructor when initializing a const reference from rvalue returned from a function.
        // This fails when the returned value cannot be copied (only moved):
        //
        // class base {};
        // class derived : public base { BOOST_MOVABLE_BUT_NOT_COPYABLE(derived) };
        // derived foo();
        // base const& var = foo(); // attempts to call copy constructor of derived
#       define BOOST_LOG_BROKEN_REFERENCE_FROM_RVALUE_INIT
#   endif
#   if !defined(_STLPORT_VERSION)
        // MSVC 9.0 mandates packaging of STL classes, which apparently affects alignment and
        // makes alignment_of< T >::value no longer be a power of 2 for types that derive from STL classes.
        // This breaks type_with_alignment and everything that relies on it.
        // This doesn't happen with non-native STLs, such as STLPort. Strangely, this doesn't show with
        // STL classes themselves or most of the user-defined derived classes.
        // Not sure if that happens with other MSVC versions.
        // See: http://svn.boost.org/trac/boost/ticket/1946
#       define BOOST_LOG_BROKEN_STL_ALIGNMENT
#   endif
#endif

#if defined(BOOST_INTEL) || defined(__SUNPRO_CC)
    // Intel compiler and Sun Studio 12.3 have problems with friend declarations for nested class templates
#   define BOOST_LOG_NO_MEMBER_TEMPLATE_FRIENDS
#endif

#if defined(BOOST_MSVC) && BOOST_MSVC <= 1600
    // MSVC cannot interpret constant expressions in certain contexts, such as non-type template parameters
#   define BOOST_LOG_BROKEN_CONSTANT_EXPRESSIONS
#endif

#if defined(BOOST_NO_CXX11_HDR_CODECVT)
    // The compiler does not support std::codecvt<char16_t> and std::codecvt<char32_t> specializations.
    // The BOOST_NO_CXX11_HDR_CODECVT means there's no usable <codecvt>, which is slightly different from this macro.
    // But in order for <codecvt> to be implemented the std::codecvt specializations have to be implemented as well.
#   define BOOST_LOG_NO_CXX11_CODECVT_FACETS
#endif

#if defined(__CYGWIN__)
    // Boost.ASIO is broken on Cygwin
#   define BOOST_LOG_NO_ASIO
#endif

#if defined(__VXWORKS__)
#   define BOOST_LOG_NO_GETPGRP
#   define BOOST_LOG_NO_GETSID
    // for _WRS_CONFIG_USER_MANAGEMENT used below
#   include <vsbConfig.h>
#endif

#if (!defined(__CRYSTAX__) && defined(__ANDROID__) && (__ANDROID_API__+0) < 21) \
     || (defined(__VXWORKS__) && !defined(_WRS_CONFIG_USER_MANAGEMENT))
// Until Android API version 21 Google NDK does not provide getpwuid_r
#    define BOOST_LOG_NO_GETPWUID_R
#endif

#if !defined(BOOST_LOG_USE_NATIVE_SYSLOG) && defined(BOOST_LOG_NO_ASIO)
#   ifndef BOOST_LOG_WITHOUT_SYSLOG
#       define BOOST_LOG_WITHOUT_SYSLOG
#   endif
#endif

#if defined(__GNUC__) && (__GNUC__ == 4 && __GNUC_MINOR__ <= 2)
    // GCC 4.1 and 4.2 have buggy anonymous namespaces support, which interferes with symbol linkage
#   define BOOST_LOG_ANONYMOUS_NAMESPACE namespace anonymous {} using namespace anonymous; namespace anonymous
#else
#   define BOOST_LOG_ANONYMOUS_NAMESPACE namespace
#endif

#if defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES) || (defined(__GNUC__) && (__GNUC__ == 4 && __GNUC_MINOR__ <= 6))
// GCC up to 4.6 (inclusively) did not support expanding template argument packs into non-variadic template arguments
#define BOOST_LOG_NO_CXX11_ARG_PACKS_TO_NON_VARIADIC_ARGS_EXPANSION
#endif

#if defined(BOOST_NO_CXX11_CONSTEXPR) || (defined(BOOST_GCC) && ((BOOST_GCC+0) / 100) <= 406)
// GCC 4.6 does not support in-class brace initializers for static constexpr array members
#define BOOST_LOG_NO_CXX11_CONSTEXPR_DATA_MEMBER_BRACE_INITIALIZERS
#endif

#if defined(BOOST_NO_CXX11_DEFAULTED_FUNCTIONS) || (defined(BOOST_GCC) && ((BOOST_GCC+0) / 100) <= 406)
// GCC 4.6 cannot handle a defaulted function with noexcept specifier
#define BOOST_LOG_NO_CXX11_DEFAULTED_NOEXCEPT_FUNCTIONS
#endif

#if defined(BOOST_NO_CXX11_DEFAULTED_FUNCTIONS) || (defined(BOOST_CLANG) && (((__clang_major__+0) == 3) && ((__clang_minor__+0) <= 1)))
// Clang 3.1 cannot handle a defaulted constexpr constructor in some cases (presumably, if the class contains a member with a constexpr constructor)
#define BOOST_LOG_NO_CXX11_DEFAULTED_CONSTEXPR_CONSTRUCTORS
#endif

#if defined(_MSC_VER)
#   define BOOST_LOG_NO_VTABLE __declspec(novtable)
#else
#   define BOOST_LOG_NO_VTABLE
#endif

// An MS-like compilers' extension that allows to optimize away the needless code
#if defined(_MSC_VER)
#   define BOOST_LOG_ASSUME(expr) __assume(expr)
#elif defined(__has_builtin)
// Clang 3.6 adds __builtin_assume, but enabling it causes weird compilation errors, where the compiler
// doesn't see one of attachable_sstream_buf::append overloads. It works fine with Clang 3.7 and later.
#   if __has_builtin(__builtin_assume) && (!defined(__clang__) || (__clang_major__ * 100 + __clang_minor__) >= 307)
#       define BOOST_LOG_ASSUME(expr) __builtin_assume(expr)
#   else
#       define BOOST_LOG_ASSUME(expr)
#   endif
#else
#   define BOOST_LOG_ASSUME(expr)
#endif

// The statement marking unreachable branches of code to avoid warnings
#if defined(BOOST_CLANG)
#   if __has_builtin(__builtin_unreachable)
#       define BOOST_LOG_UNREACHABLE() __builtin_unreachable()
#   endif
#elif defined(__GNUC__)
#   if (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 5))
#       define BOOST_LOG_UNREACHABLE() __builtin_unreachable()
#   endif
#elif defined(_MSC_VER)
#   define BOOST_LOG_UNREACHABLE() __assume(0)
#endif
#if !defined(BOOST_LOG_UNREACHABLE)
#   define BOOST_LOG_UNREACHABLE()
#   define BOOST_LOG_UNREACHABLE_RETURN(r) return r
#else
#   define BOOST_LOG_UNREACHABLE_RETURN(r) BOOST_LOG_UNREACHABLE()
#endif

// The macro efficiently returns a local lvalue from a function.
// It employs NRVO, if supported by compiler, or uses a move constructor otherwise.
#if defined(BOOST_HAS_NRVO)
#define BOOST_LOG_NRVO_RESULT(x) x
#else
#define BOOST_LOG_NRVO_RESULT(x) boost::move(x)
#endif

// Some compilers support a special attribute that shows that a function won't return
#if defined(__GNUC__) || (defined(__SUNPRO_CC) && __SUNPRO_CC >= 0x590)
    // GCC and Sun Studio 12 support attribute syntax
#   define BOOST_LOG_NORETURN __attribute__((noreturn))
#elif defined (_MSC_VER)
    // Microsoft-compatible compilers go here
#   define BOOST_LOG_NORETURN __declspec(noreturn)
#else
    // The rest compilers might emit bogus warnings about missing return statements
    // in functions with non-void return types when throw_exception is used.
#   define BOOST_LOG_NORETURN
#endif

// Some compilers may require marking types that may alias other types
#define BOOST_LOG_MAY_ALIAS BOOST_MAY_ALIAS

#if !defined(BOOST_LOG_BUILDING_THE_LIB)

// Detect if we're dealing with dll
#   if defined(BOOST_LOG_DYN_LINK) || defined(BOOST_ALL_DYN_LINK)
#       define BOOST_LOG_DLL
#   endif

#   if defined(BOOST_LOG_DLL)
#       define BOOST_LOG_API BOOST_SYMBOL_IMPORT
#   else
#       define BOOST_LOG_API
#   endif
//
// Automatically link to the correct build variant where possible.
//
#   if !defined(BOOST_ALL_NO_LIB)
#       if !defined(BOOST_LOG_NO_LIB)
#          define BOOST_LIB_NAME boost_log
#          if defined(BOOST_LOG_DLL)
#              define BOOST_DYN_LINK
#          endif
#          include <boost/config/auto_link.hpp>
#       endif
        // In static-library builds compilers ignore auto-link comments from Boost.Log binary to
        // other Boost libraries. We explicitly add comments here for other libraries.
        // In dynamic-library builds this is not needed.
#       if !defined(BOOST_LOG_DLL)
#           include <boost/system/config.hpp>
#           include <boost/filesystem/config.hpp>
#           if !defined(BOOST_DATE_TIME_NO_LIB) && !defined(BOOST_DATE_TIME_SOURCE)
#               define BOOST_LIB_NAME boost_date_time
#               if defined(BOOST_ALL_DYN_LINK) || defined(BOOST_DATE_TIME_DYN_LINK)
#                   define BOOST_DYN_LINK
#               endif
#               include <boost/config/auto_link.hpp>
#           endif
            // Boost.Thread's config is included below, if needed
#       endif
#   endif  // auto-linking disabled

#else // !defined(BOOST_LOG_BUILDING_THE_LIB)

#   if defined(BOOST_LOG_DLL)
#       define BOOST_LOG_API BOOST_SYMBOL_EXPORT
#   else
#       define BOOST_LOG_API BOOST_SYMBOL_VISIBLE
#   endif

#endif // !defined(BOOST_LOG_BUILDING_THE_LIB)

// By default we provide support for both char and wchar_t
#if !defined(BOOST_LOG_WITHOUT_CHAR)
#   define BOOST_LOG_USE_CHAR
#endif
#if !defined(BOOST_LOG_WITHOUT_WCHAR_T)
#   define BOOST_LOG_USE_WCHAR_T
#endif

#if !defined(BOOST_LOG_DOXYGEN_PASS)
    // Check if multithreading is supported
#   if !defined(BOOST_LOG_NO_THREADS) && !defined(BOOST_HAS_THREADS)
#       define BOOST_LOG_NO_THREADS
#   endif // !defined(BOOST_LOG_NO_THREADS) && !defined(BOOST_HAS_THREADS)
#endif // !defined(BOOST_LOG_DOXYGEN_PASS)

#if !defined(BOOST_LOG_NO_THREADS)
    // We need this header to (i) enable auto-linking with Boost.Thread and
    // (ii) to bring in configuration macros of Boost.Thread.
#   include <boost/thread/detail/config.hpp>
#endif // !defined(BOOST_LOG_NO_THREADS)

#if !defined(BOOST_LOG_NO_THREADS)
#   define BOOST_LOG_EXPR_IF_MT(expr) expr
#else
#   undef BOOST_LOG_USE_COMPILER_TLS
#   define BOOST_LOG_EXPR_IF_MT(expr)
#endif // !defined(BOOST_LOG_NO_THREADS)

#if defined(BOOST_LOG_USE_COMPILER_TLS)
#   if defined(__GNUC__) || defined(__SUNPRO_CC)
#       define BOOST_LOG_TLS __thread
#   elif defined(BOOST_MSVC)
#       define BOOST_LOG_TLS __declspec(thread)
#   else
#       undef BOOST_LOG_USE_COMPILER_TLS
#   endif
#endif // defined(BOOST_LOG_USE_COMPILER_TLS)

#ifndef BOOST_LOG_CPU_CACHE_LINE_SIZE
//! The macro defines the CPU cache line size for the target architecture. This is mostly used for optimization.
#if defined(__s390__) || defined(__s390x__)
#define BOOST_LOG_CPU_CACHE_LINE_SIZE 256
#elif defined(powerpc) || defined(__powerpc__) || defined(__ppc__)
#define BOOST_LOG_CPU_CACHE_LINE_SIZE 128
#else
#define BOOST_LOG_CPU_CACHE_LINE_SIZE 64
#endif
#endif

namespace boost {

// Setup namespace name
#if !defined(BOOST_LOG_DOXYGEN_PASS)
#   if defined(BOOST_LOG_DLL)
#       if defined(BOOST_LOG_NO_THREADS)
#           define BOOST_LOG_VERSION_NAMESPACE v2_st
#       else
#           if defined(BOOST_THREAD_PLATFORM_PTHREAD)
#               define BOOST_LOG_VERSION_NAMESPACE v2_mt_posix
#           elif defined(BOOST_THREAD_PLATFORM_WIN32)
#               if BOOST_USE_WINAPI_VERSION >= BOOST_WINAPI_VERSION_WIN6
#                   define BOOST_LOG_VERSION_NAMESPACE v2_mt_nt6
#               else
#                   define BOOST_LOG_VERSION_NAMESPACE v2_mt_nt5
#               endif
#           else
#               define BOOST_LOG_VERSION_NAMESPACE v2_mt
#           endif
#       endif // defined(BOOST_LOG_NO_THREADS)
#   else
#       if defined(BOOST_LOG_NO_THREADS)
#           define BOOST_LOG_VERSION_NAMESPACE v2s_st
#       else
#           if defined(BOOST_THREAD_PLATFORM_PTHREAD)
#               define BOOST_LOG_VERSION_NAMESPACE v2s_mt_posix
#           elif defined(BOOST_THREAD_PLATFORM_WIN32)
#               if BOOST_USE_WINAPI_VERSION >= BOOST_WINAPI_VERSION_WIN6
#                   define BOOST_LOG_VERSION_NAMESPACE v2s_mt_nt6
#               else
#                   define BOOST_LOG_VERSION_NAMESPACE v2s_mt_nt5
#               endif
#           else
#               define BOOST_LOG_VERSION_NAMESPACE v2s_mt
#           endif
#       endif // defined(BOOST_LOG_NO_THREADS)
#   endif // defined(BOOST_LOG_DLL)


namespace log {

#   if !defined(BOOST_NO_CXX11_INLINE_NAMESPACES)

inline namespace BOOST_LOG_VERSION_NAMESPACE {}

#       define BOOST_LOG_OPEN_NAMESPACE namespace log { inline namespace BOOST_LOG_VERSION_NAMESPACE {
#       define BOOST_LOG_CLOSE_NAMESPACE }}

#   elif defined(BOOST_GCC) && (BOOST_GCC+0) >= 40400

// GCC 7 deprecated strong using directives but allows inline namespaces in C++03 mode since GCC 4.4.
__extension__ inline namespace BOOST_LOG_VERSION_NAMESPACE {}

#       define BOOST_LOG_OPEN_NAMESPACE namespace log { __extension__ inline namespace BOOST_LOG_VERSION_NAMESPACE {
#       define BOOST_LOG_CLOSE_NAMESPACE }}

#   else

namespace BOOST_LOG_VERSION_NAMESPACE {}

using namespace BOOST_LOG_VERSION_NAMESPACE
#       if defined(__GNUC__) && (__GNUC__ >= 4 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4)) && !defined(__clang__)
__attribute__((__strong__))
#       endif
;

#       define BOOST_LOG_OPEN_NAMESPACE namespace log { namespace BOOST_LOG_VERSION_NAMESPACE {
#       define BOOST_LOG_CLOSE_NAMESPACE }}
#   endif

} // namespace log

#else // !defined(BOOST_LOG_DOXYGEN_PASS)

namespace log {}
#   define BOOST_LOG_OPEN_NAMESPACE namespace log {
#   define BOOST_LOG_CLOSE_NAMESPACE }

#endif // !defined(BOOST_LOG_DOXYGEN_PASS)

#if defined(BOOST_LOG_HAS_PRAGMA_DETECT_MISMATCH)
#pragma detect_mismatch("boost_log_abi", BOOST_PP_STRINGIZE(BOOST_LOG_VERSION_NAMESPACE))
#endif

} // namespace boost

#endif // BOOST_LOG_DETAIL_CONFIG_HPP_INCLUDED_
