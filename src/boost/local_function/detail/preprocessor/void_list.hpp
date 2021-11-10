
// Copyright (C) 2009-2012 Lorenzo Caminiti
// Distributed under the Boost Software License, Version 1.0
// (see accompanying file LICENSE_1_0.txt or a copy at
// http://www.boost.org/LICENSE_1_0.txt)
// Home at http://www.boost.org/libs/local_function

#ifndef BOOST_LOCAL_FUNCTION_DETAIL_PP_VOID_LIST_HPP_
#define BOOST_LOCAL_FUNCTION_DETAIL_PP_VOID_LIST_HPP_

#include <boost/local_function/detail/preprocessor/keyword/void.hpp>
#include <boost/config.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/control/iif.hpp>
#include <boost/preprocessor/comparison/equal.hpp>
#include <boost/preprocessor/tuple/to_list.hpp>
#include <boost/preprocessor/seq/size.hpp>
#include <boost/preprocessor/seq/to_tuple.hpp>

// PRIVATE //

// Argument: (token1)...
#define BOOST_LOCAL_FUNCTION_DETAIL_PP_VOID_LIST_FROM_SEQ_(unused, seq) \
    BOOST_PP_TUPLE_TO_LIST(BOOST_PP_SEQ_SIZE(seq), BOOST_PP_SEQ_TO_TUPLE(seq))

// Token: void | token1
#define BOOST_LOCAL_FUNCTION_DETAIL_PP_VOID_LIST_HANDLE_VOID_( \
        is_void_macro, token) \
    BOOST_PP_IIF(is_void_macro(token), \
        BOOST_PP_NIL \
    , \
        (token, BOOST_PP_NIL) \
    )

// Token: (a)(b)... | empty | void | token
#define BOOST_LOCAL_FUNCTION_DETAIL_PP_VOID_LIST_HANDLE_SEQ_( \
        is_void_macro, token) \
    BOOST_PP_IIF(BOOST_PP_IS_UNARY(token), /* unary paren (a)... */ \
        BOOST_LOCAL_FUNCTION_DETAIL_PP_VOID_LIST_FROM_SEQ_ \
    , \
        BOOST_LOCAL_FUNCTION_DETAIL_PP_VOID_LIST_HANDLE_VOID_ \
    )(is_void_macro, token)

#ifdef BOOST_NO_CXX11_VARIADIC_MACROS

#define BOOST_LOCAL_FUNCTION_DETAIL_PP_VOID_LIST_(is_void_macro, seq) \
    BOOST_LOCAL_FUNCTION_DETAIL_PP_VOID_LIST_HANDLE_SEQ_(is_void_macro, seq)

#else // VARIADICS

// FUTURE: Replace this with BOOST_PP_VARIADIC_SIZE when and if
// BOOST_PP_VARIAIDCS detection will match !BOOST_NO_CXX11_VARIADIC_MACROS (for now
// Boost.Preprocessor and Boost.Config disagree on detecting compiler variadic
// support while this VARIADIC_SIZE works on compilers not detected by PP).
#if BOOST_MSVC
#   define BOOST_LOCAL_FUNCTION_DETAIL_PP_VOID_LIST_VARIADIC_SIZE_(...) \
        BOOST_PP_CAT(BOOST_LOCAL_FUNCTION_DETAIL_PP_VOID_LIST_VARIADIC_SIZE_I_(__VA_ARGS__, 64, 63, 62, 61, 60, 59, 58, 57, 56, 55, 54, 53, 52, 51, 50, 49, 48, 47, 46, 45, 44, 43, 42, 41, 40, 39, 38, 37, 36, 35, 34, 33, 32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1,),)
#else // MSVC
#   define BOOST_LOCAL_FUNCTION_DETAIL_PP_VOID_LIST_VARIADIC_SIZE_(...) \
        BOOST_LOCAL_FUNCTION_DETAIL_PP_VOID_LIST_VARIADIC_SIZE_I_(__VA_ARGS__, 64, 63, 62, 61, 60, 59, 58, 57, 56, 55, 54, 53, 52, 51, 50, 49, 48, 47, 46, 45, 44, 43, 42, 41, 40, 39, 38, 37, 36, 35, 34, 33, 32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1,)
#endif // MSVC
#define BOOST_LOCAL_FUNCTION_DETAIL_PP_VOID_LIST_VARIADIC_SIZE_I_(e0, e1, e2, e3, e4, e5, e6, e7, e8, e9, e10, e11, e12, e13, e14, e15, e16, e17, e18, e19, e20, e21, e22, e23, e24, e25, e26, e27, e28, e29, e30, e31, e32, e33, e34, e35, e36, e37, e38, e39, e40, e41, e42, e43, e44, e45, e46, e47, e48, e49, e50, e51, e52, e53, e54, e55, e56, e57, e58, e59, e60, e61, e62, e63, size, ...) size

// Argument: token1, ...
#define BOOST_LOCAL_FUNCTION_DETAIL_PP_VOID_LIST_FROM_VARIADIC_(unused, ...) \
    BOOST_PP_TUPLE_TO_LIST( \
            BOOST_LOCAL_FUNCTION_DETAIL_PP_VOID_LIST_VARIADIC_SIZE_( \
                    __VA_ARGS__), (__VA_ARGS__))

#define BOOST_LOCAL_FUNCTION_DETAIL_PP_VOID_LIST_(is_void_macro, ...) \
    BOOST_PP_IIF(BOOST_PP_EQUAL( \
            BOOST_LOCAL_FUNCTION_DETAIL_PP_VOID_LIST_VARIADIC_SIZE_( \
                    __VA_ARGS__), 1), \
        BOOST_LOCAL_FUNCTION_DETAIL_PP_VOID_LIST_HANDLE_SEQ_ \
    , \
        BOOST_LOCAL_FUNCTION_DETAIL_PP_VOID_LIST_FROM_VARIADIC_ \
    )(is_void_macro, __VA_ARGS__)

#endif // VARIADICS

#define BOOST_LOCAL_FUNCTION_DETAIL_PP_VOID_LIST_NEVER_(tokens) \
    0 /* void check always returns false */

// PUBLIC //

// NOTE: Empty list must always be represented is void (which is also a way to
// specify no function parameter) and it can never be empty because (1)
// IS_EMPTY(&var) fails (because of the leading non alphanumeric symbol) and
// (2) some compilers (MSVC) fail to correctly pass empty macro parameters
// even if they support variadic macros. Therefore, always using void to
// represent is more portable.

#ifdef BOOST_NO_CXX11_VARIADIC_MACROS

// Expand `void | (a)(b)...` to pp-list `NIL | (a, (b, NIL))`.
#define BOOST_LOCAL_FUNCTION_DETAIL_PP_VOID_LIST(sign) \
    BOOST_LOCAL_FUNCTION_DETAIL_PP_VOID_LIST_( \
            BOOST_LOCAL_FUNCTION_DETAIL_PP_KEYWORD_IS_VOID_BACK, sign)

// Expand `(a)(b)...` to pp-list `(a, (b, NIL))`.
#define BOOST_LOCAL_FUNCTION_DETAIL_PP_NON_VOID_LIST(seq) \
    BOOST_LOCAL_FUNCTION_DETAIL_PP_VOID_LIST_( \
            BOOST_LOCAL_FUNCTION_DETAIL_PP_VOID_LIST_NEVER_, seq)

#else // VARIADICS

// Expand `void | (a)(b)... | a, b, ...` to pp-list `NIL | (a, (b, NIL))`.
#define BOOST_LOCAL_FUNCTION_DETAIL_PP_VOID_LIST(...) \
    BOOST_LOCAL_FUNCTION_DETAIL_PP_VOID_LIST_( \
            BOOST_LOCAL_FUNCTION_DETAIL_PP_KEYWORD_IS_VOID_BACK, __VA_ARGS__)

// Expand `(a)(b)... | a, b, ...` to pp-list `(a, (b, NIL))`.
#define BOOST_LOCAL_FUNCTION_DETAIL_PP_NON_VOID_LIST(...) \
    BOOST_LOCAL_FUNCTION_DETAIL_PP_VOID_LIST_( \
            BOOST_LOCAL_FUNCTION_DETAIL_PP_VOID_LIST_NEVER_, __VA_ARGS__)

#endif // VARIADICS

#endif // #include guard

