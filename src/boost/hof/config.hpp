/*=============================================================================
    Copyright (c) 2016 Paul Fultz II
    config.hpp
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/

#ifndef BOOST_HOF_GUARD_CONFIG_HPP
#define BOOST_HOF_GUARD_CONFIG_HPP

// Unpack has extra checks to ensure that the function will be invoked with
// the sequence. This extra check can help improve error reporting but it can
// slow down compilation. This is enabled by default.
#ifndef BOOST_HOF_CHECK_UNPACK_SEQUENCE
#define BOOST_HOF_CHECK_UNPACK_SEQUENCE 1
#endif

// Check for std version
#if __cplusplus >= 201606
#define BOOST_HOF_HAS_STD_17 1
#else
#define BOOST_HOF_HAS_STD_17 0
#endif

#if __cplusplus >= 201402
#define BOOST_HOF_HAS_STD_14 1
#else
#define BOOST_HOF_HAS_STD_14 0
#endif

#if __cplusplus >= 201103
#define BOOST_HOF_HAS_STD_11 1
#else
#define BOOST_HOF_HAS_STD_11 0
#endif


// This determines if it safe to use inheritance for EBO. On every platform
// except clang, compilers have problems with ambigous base conversion. So
// this configures the library to use a different technique to achieve empty
// optimization.
#ifndef BOOST_HOF_HAS_EBO
#ifdef __clang__
#define BOOST_HOF_HAS_EBO 1
#else
#define BOOST_HOF_HAS_EBO 0
#endif
#endif

// This configures the library whether expression sfinae can be used to detect
// callability of a function.
#ifndef BOOST_HOF_NO_EXPRESSION_SFINAE
#ifdef _MSC_VER
#define BOOST_HOF_NO_EXPRESSION_SFINAE 1
#else
#define BOOST_HOF_NO_EXPRESSION_SFINAE 0
#endif
#endif

// This configures the library to use manual type deduction in a few places
// where it problematic on a few platforms.
#ifndef BOOST_HOF_HAS_MANUAL_DEDUCTION
#if (defined(__GNUC__) && !defined (__clang__) && __GNUC__ == 4 && __GNUC_MINOR__ < 8)
#define BOOST_HOF_HAS_MANUAL_DEDUCTION 1
#else
#define BOOST_HOF_HAS_MANUAL_DEDUCTION 0
#endif
#endif

// Whether the compiler has relaxed constexpr.
#ifndef BOOST_HOF_HAS_RELAXED_CONSTEXPR
#ifdef __cpp_constexpr
#if __cpp_constexpr >= 201304
#define BOOST_HOF_HAS_RELAXED_CONSTEXPR 1
#else
#define BOOST_HOF_HAS_RELAXED_CONSTEXPR 0
#endif
#else
#define BOOST_HOF_HAS_RELAXED_CONSTEXPR BOOST_HOF_HAS_STD_14
#endif
#endif

// Whether the compiler supports generic lambdas
#ifndef BOOST_HOF_HAS_GENERIC_LAMBDA
#if defined(__cpp_generic_lambdas) || defined(_MSC_VER)
#define BOOST_HOF_HAS_GENERIC_LAMBDA 1
#else
#define BOOST_HOF_HAS_GENERIC_LAMBDA BOOST_HOF_HAS_STD_14
#endif
#endif

// Whether the compiler supports constexpr lambdas
#ifndef BOOST_HOF_HAS_CONSTEXPR_LAMBDA
#if defined(__cpp_constexpr) && __cpp_constexpr >= 201603
#define BOOST_HOF_HAS_CONSTEXPR_LAMBDA 1
#else
#define BOOST_HOF_HAS_CONSTEXPR_LAMBDA BOOST_HOF_HAS_STD_17
#endif
#endif

// Whether the compiler supports inline variables
#ifndef BOOST_HOF_HAS_INLINE_VARIABLES
#if defined(__cpp_inline_variables)
#define BOOST_HOF_HAS_INLINE_VARIABLES 1
#else
#define BOOST_HOF_HAS_INLINE_VARIABLES BOOST_HOF_HAS_STD_17
#endif
#endif

// Whether inline variables defined with lambdas have external linkage.
// Currently, no compiler supports this yet.
#ifndef BOOST_HOF_HAS_INLINE_LAMBDAS
#define BOOST_HOF_HAS_INLINE_LAMBDAS 0
#endif

// Whether the compiler supports variable templates
#ifndef BOOST_HOF_HAS_VARIABLE_TEMPLATES
#if defined(__clang__) && __clang_major__ == 3 && __clang_minor__ < 5
#define BOOST_HOF_HAS_VARIABLE_TEMPLATES 0
#elif defined(__cpp_variable_templates)
#define BOOST_HOF_HAS_VARIABLE_TEMPLATES 1
#else
#define BOOST_HOF_HAS_VARIABLE_TEMPLATES BOOST_HOF_HAS_STD_14
#endif
#endif

// Whether a constexpr function can use a void return type
#ifndef BOOST_HOF_NO_CONSTEXPR_VOID
#if BOOST_HOF_HAS_RELAXED_CONSTEXPR
#define BOOST_HOF_NO_CONSTEXPR_VOID 0
#else
#define BOOST_HOF_NO_CONSTEXPR_VOID 1
#endif
#endif

// Whether to use template aliases
#ifndef BOOST_HOF_HAS_TEMPLATE_ALIAS
#if defined(__GNUC__) && !defined (__clang__) && __GNUC__ == 4 && __GNUC_MINOR__ < 8
#define BOOST_HOF_HAS_TEMPLATE_ALIAS 0
#else
#define BOOST_HOF_HAS_TEMPLATE_ALIAS 1
#endif
#endif

// Whether evaluations of function in brace initialization is ordered from
// left-to-right.
#ifndef BOOST_HOF_NO_ORDERED_BRACE_INIT
#if (defined(__GNUC__) && !defined (__clang__) && __GNUC__ == 4 && __GNUC_MINOR__ < 9) || defined(_MSC_VER)
#define BOOST_HOF_NO_ORDERED_BRACE_INIT 1
#else
#define BOOST_HOF_NO_ORDERED_BRACE_INIT 0
#endif 
#endif

// Whether the compiler has trouble mangling some expressions used in
// decltype.
#ifndef BOOST_HOF_HAS_MANGLE_OVERLOAD
#if defined(__GNUC__) && !defined (__clang__) && __GNUC__ == 4 && __GNUC_MINOR__ < 7
#define BOOST_HOF_HAS_MANGLE_OVERLOAD 0
#else
#define BOOST_HOF_HAS_MANGLE_OVERLOAD 1
#endif
#endif

// Whether an incomplete 'this' pointer can be used in a trailing decltype.
#ifndef BOOST_HOF_HAS_COMPLETE_DECLTYPE
#if !BOOST_HOF_HAS_MANGLE_OVERLOAD || (defined(__GNUC__) && !defined (__clang__))
#define BOOST_HOF_HAS_COMPLETE_DECLTYPE 0
#else
#define BOOST_HOF_HAS_COMPLETE_DECLTYPE 1
#endif
#endif

// Whether function will deduce noexcept from an expression
#ifndef BOOST_HOF_HAS_NOEXCEPT_DEDUCTION
#if defined(__GNUC__) && !defined (__clang__) && ((__GNUC__ == 4 && __GNUC_MINOR__ < 8) || (__GNUC__ == 7 && __GNUC_MINOR__ == 1))
#define BOOST_HOF_HAS_NOEXCEPT_DEDUCTION 0
#else
#define BOOST_HOF_HAS_NOEXCEPT_DEDUCTION 1
#endif
#endif

// Some type expansion failures on gcc 4.6
#ifndef BOOST_HOF_NO_TYPE_PACK_EXPANSION_IN_TEMPLATE
#if defined(__GNUC__) && !defined (__clang__) && __GNUC__ == 4 && __GNUC_MINOR__ < 7
#define BOOST_HOF_NO_TYPE_PACK_EXPANSION_IN_TEMPLATE 1
#else
#define BOOST_HOF_NO_TYPE_PACK_EXPANSION_IN_TEMPLATE 0
#endif
#endif

// Whether to use std::default_constructible, it is a little buggy on gcc 4.6.
#ifndef BOOST_HOF_NO_STD_DEFAULT_CONSTRUCTIBLE
#if defined(__GNUC__) && !defined (__clang__) && __GNUC__ == 4 && __GNUC_MINOR__ < 7
#define BOOST_HOF_NO_STD_DEFAULT_CONSTRUCTIBLE 1
#else
#define BOOST_HOF_NO_STD_DEFAULT_CONSTRUCTIBLE 0
#endif
#endif

#endif
