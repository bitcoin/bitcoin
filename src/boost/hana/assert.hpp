/*!
@file
Defines macros to perform different kinds of assertions.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_ASSERT_HPP
#define BOOST_HANA_ASSERT_HPP

#include <boost/hana/concept/constant.hpp>
#include <boost/hana/config.hpp>
#include <boost/hana/detail/preprocessor.hpp>
#include <boost/hana/if.hpp>
#include <boost/hana/value.hpp>

#include <cstdio>
#include <cstdlib>


#if defined(BOOST_HANA_DOXYGEN_INVOKED)
    //! @ingroup group-assertions
    //! Expands to a runtime assertion.
    //!
    //! Given a condition known at runtime, this macro expands to a runtime
    //! assertion similar to the `assert` macro. The provided condition must
    //! be explicitly convertible to a `bool`, and it must not be a model of
    //! the `Constant` concept. If the condition is a `Constant`, a static
    //! assertion will be triggered, asking you to use the
    //! `BOOST_HANA_CONSTANT_ASSERT` macro instead.
    //!
    //! @note
    //! This macro may only be used at function scope.
#   define BOOST_HANA_RUNTIME_ASSERT(condition) unspecified

    //! @ingroup group-assertions
    //! Equivalent to `BOOST_HANA_RUNTIME_ASSERT`, but allows providing a
    //! custom failure message.
    //!
    //! @warning
    //! Conditions that contain multiple comma-separated elements should be
    //! parenthesized.
#   define BOOST_HANA_RUNTIME_ASSERT_MSG(condition, message) unspecified

    //! @ingroup group-assertions
    //! Compile-time assertion for `Constant`s.
    //!
    //! Given a condition known at compile-time in the form of a `Constant`,
    //! this macro expands to a compile-time assertion similar to a `static_assert`.
    //! The provided condition must be a model of the `Constant` concept, in
    //! which case its value is retrieved using `hana::value` and then converted
    //! to a `bool`. If the condition is not a `Constant`, a static assertion
    //! will be triggered, asking you to use the `BOOST_HANA_RUNTIME_ASSERT`
    //! macro instead.
    //!
    //! This macro may be used at global/namespace scope and function scope
    //! only; it may not be used at class scope. Note that the condition may
    //! never be evaluated at runtime. Hence, any side effect may not take
    //! place (but you shouldn't rely on side effects inside assertions anyway).
#   define BOOST_HANA_CONSTANT_ASSERT(condition) unspecified

    //! @ingroup group-assertions
    //! Equivalent to `BOOST_HANA_CONSTANT_ASSERT`, but allows providing a
    //! custom failure message.
    //!
    //! @warning
    //! Conditions that contain multiple comma-separated elements should be
    //! parenthesized.
#   define BOOST_HANA_CONSTANT_ASSERT_MSG(condition, message) unspecified

    //! @ingroup group-assertions
    //! Expands to the strongest form of assertion possible for the given
    //! condition.
    //!
    //! Given a condition, `BOOST_HANA_ASSERT` expands either to a compile-time
    //! or to a runtime assertion, depending on whether the value of the
    //! condition is known at compile-time or at runtime. Compile-time
    //! assertions are always preferred over runtime assertions. If the
    //! condition is a model of the `Constant` concept, its value (retrievable
    //! with `hana::value`) is assumed to be explicitly convertible to `bool`,
    //! and a compile-time assertion is performed on it. Otherwise, the
    //! condition itself is assumed to be explicitly convertible to `bool`,
    //! and a runtime assertion is performed on it.
    //!
    //! If the assertion can be carried out at compile-time, the condition
    //! is not guaranteed to be evaluated at runtime at all (but it may).
    //! Hence, in general, you shouldn't rely on side effects that take place
    //! inside an assertion.
    //!
    //! @note
    //! This macro may only be used at function scope.
#   define BOOST_HANA_ASSERT(condition) unspecified

    //! @ingroup group-assertions
    //! Equivalent to `BOOST_HANA_ASSERT`, but allows providing a custom
    //! failure message.
    //!
    //! @warning
    //! Conditions that contain multiple comma-separated elements should be
    //! parenthesized.
#   define BOOST_HANA_ASSERT_MSG(condition, message) unspecified

    //! @ingroup group-assertions
    //! Expands to a static assertion or a runtime assertion, depending on
    //! whether `constexpr` lambdas are supported.
    //!
    //! This macro is used to assert on a condition that would be a constant
    //! expression if constexpr lambdas were supported. Right now, constexpr
    //! lambdas are not supported, and this is always a runtime assertion.
    //! Specifically, this is equivalent to `BOOST_HANA_RUNTIME_ASSERT`.
#   define BOOST_HANA_CONSTEXPR_ASSERT(condition) unspecified

    //! @ingroup group-assertions
    //! Equivalent to `BOOST_HANA_CONSTEXPR_ASSERT`, but allows providing a
    //! custom failure message.
#   define BOOST_HANA_CONSTEXPR_ASSERT_MSG(condition, message) unspecified

#elif defined(BOOST_HANA_CONFIG_DISABLE_ASSERTIONS)

#   define BOOST_HANA_CONSTANT_ASSERT(...)                      /* nothing */
#   define BOOST_HANA_CONSTANT_ASSERT_MSG(condition, message)   /* nothing */

#   define BOOST_HANA_RUNTIME_ASSERT(...)                       /* nothing */
#   define BOOST_HANA_RUNTIME_ASSERT_MSG(condition, message)    /* nothing */

#   define BOOST_HANA_ASSERT(...)                               /* nothing */
#   define BOOST_HANA_ASSERT_MSG(condition, message)            /* nothing */

#   define BOOST_HANA_CONSTEXPR_ASSERT(...)                     /* nothing */
#   define BOOST_HANA_CONSTEXPR_ASSERT_MSG(condition, message)  /* nothing */

#else

//////////////////////////////////////////////////////////////////////////////
// BOOST_HANA_RUNTIME_ASSERT and BOOST_HANA_RUNTIME_ASSERT_MSG
//////////////////////////////////////////////////////////////////////////////
#   define BOOST_HANA_RUNTIME_ASSERT_MSG(condition, message)                \
        BOOST_HANA_RUNTIME_CHECK_MSG(condition, message)                    \
/**/

#   define BOOST_HANA_RUNTIME_ASSERT(...)                                   \
        BOOST_HANA_RUNTIME_CHECK(__VA_ARGS__)                               \
/**/

//////////////////////////////////////////////////////////////////////////////
// BOOST_HANA_CONSTANT_ASSERT and BOOST_HANA_CONSTANT_ASSERT_MSG
//////////////////////////////////////////////////////////////////////////////
#   define BOOST_HANA_CONSTANT_ASSERT_MSG(condition, message)               \
    BOOST_HANA_CONSTANT_CHECK_MSG(condition, message)                       \
/**/

#   define BOOST_HANA_CONSTANT_ASSERT(...)                                  \
        BOOST_HANA_CONSTANT_CHECK(__VA_ARGS__)                              \
/**/

//////////////////////////////////////////////////////////////////////////////
// BOOST_HANA_ASSERT and BOOST_HANA_ASSERT_MSG
//////////////////////////////////////////////////////////////////////////////
#   define BOOST_HANA_ASSERT_MSG(condition, message)                        \
        BOOST_HANA_CHECK_MSG(condition, message)                            \
/**/

#   define BOOST_HANA_ASSERT(...)                                           \
        BOOST_HANA_CHECK(__VA_ARGS__)                                       \
/**/

//////////////////////////////////////////////////////////////////////////////
// BOOST_HANA_CONSTEXPR_ASSERT and BOOST_HANA_CONSTEXPR_ASSERT_MSG
//////////////////////////////////////////////////////////////////////////////
#   define BOOST_HANA_CONSTEXPR_ASSERT_MSG(condition, message)              \
        BOOST_HANA_CONSTEXPR_CHECK_MSG(condition, message)                  \
/**/

#   define BOOST_HANA_CONSTEXPR_ASSERT(...)                                 \
        BOOST_HANA_CONSTEXPR_CHECK(__VA_ARGS__)                             \
/**/

#endif

//////////////////////////////////////////////////////////////////////////////
// BOOST_HANA_RUNTIME_CHECK and BOOST_HANA_RUNTIME_CHECK_MSG
//////////////////////////////////////////////////////////////////////////////

//! @ingroup group-assertions
//! Equivalent to `BOOST_HANA_RUNTIME_ASSERT_MSG`, but not influenced by the
//! `BOOST_HANA_CONFIG_DISABLE_ASSERTIONS` config macro. For internal use only.
#   define BOOST_HANA_RUNTIME_CHECK_MSG(condition, message)                 \
    do {                                                                    \
        auto __hana_tmp = condition;                                        \
        static_assert(!::boost::hana::Constant<decltype(__hana_tmp)>::value,\
        "the expression (" # condition ") yields a Constant; "              \
        "use BOOST_HANA_CONSTANT_ASSERT instead");                          \
                                                                            \
        if (!static_cast<bool>(__hana_tmp)) {                               \
            ::std::fprintf(stderr, "Assertion failed: "                     \
                "(%s), function %s, file %s, line %i.\n",                   \
                message, __func__, __FILE__, __LINE__);                     \
            ::std::abort();                                                 \
        }                                                                   \
    } while (false);                                                        \
    static_assert(true, "force trailing semicolon")                         \
/**/

//! @ingroup group-assertions
//! Equivalent to `BOOST_HANA_RUNTIME_ASSERT`, but not influenced by the
//! `BOOST_HANA_CONFIG_DISABLE_ASSERTIONS` config macro. For internal use only.
#   define BOOST_HANA_RUNTIME_CHECK(...)                                    \
        BOOST_HANA_RUNTIME_CHECK_MSG(                                       \
            (__VA_ARGS__),                                                  \
            BOOST_HANA_PP_STRINGIZE(__VA_ARGS__)                            \
        )                                                                   \
/**/

//////////////////////////////////////////////////////////////////////////////
// BOOST_HANA_CONSTANT_CHECK and BOOST_HANA_CONSTANT_CHECK_MSG
//////////////////////////////////////////////////////////////////////////////

//! @ingroup group-assertions
//! Equivalent to `BOOST_HANA_CONSTANT_ASSERT_MSG`, but not influenced by the
//! `BOOST_HANA_CONFIG_DISABLE_ASSERTIONS` config macro. For internal use only.
#   define BOOST_HANA_CONSTANT_CHECK_MSG(condition, message)                \
        auto BOOST_HANA_PP_CONCAT(__hana_tmp_, __LINE__) = condition;       \
        static_assert(::boost::hana::Constant<                              \
            decltype(BOOST_HANA_PP_CONCAT(__hana_tmp_, __LINE__))           \
        >::value,                                                           \
        "the expression " # condition " does not yield a Constant; "        \
        "use BOOST_HANA_RUNTIME_ASSERT instead");                           \
        static_assert(::boost::hana::value<                                 \
            decltype(BOOST_HANA_PP_CONCAT(__hana_tmp_, __LINE__))           \
        >(), message);                                                      \
        static_assert(true, "force trailing semicolon")                     \
/**/

//! @ingroup group-assertions
//! Equivalent to `BOOST_HANA_CONSTANT_ASSERT`, but not influenced by the
//! `BOOST_HANA_CONFIG_DISABLE_ASSERTIONS` config macro. For internal use only.
#   define BOOST_HANA_CONSTANT_CHECK(...)                                   \
        BOOST_HANA_CONSTANT_CHECK_MSG(                                      \
            (__VA_ARGS__),                                                  \
            BOOST_HANA_PP_STRINGIZE(__VA_ARGS__)                            \
        )                                                                   \
/**/

//////////////////////////////////////////////////////////////////////////////
// BOOST_HANA_CHECK and BOOST_HANA_CHECK_MSG
//////////////////////////////////////////////////////////////////////////////

//! @ingroup group-assertions
//! Equivalent to `BOOST_HANA_ASSERT_MSG`, but not influenced by the
//! `BOOST_HANA_CONFIG_DISABLE_ASSERTIONS` config macro. For internal use only.
#   define BOOST_HANA_CHECK_MSG(condition, message)                         \
    do {                                                                    \
        auto __hana_tmp = condition;                                        \
        ::boost::hana::if_(::boost::hana::bool_c<                           \
            ::boost::hana::Constant<decltype(__hana_tmp)>::value>,          \
            [](auto expr) {                                                 \
                static_assert(::boost::hana::value<decltype(expr)>(),       \
                message);                                                   \
            },                                                              \
            [](auto expr) {                                                 \
                if (!static_cast<bool>(expr)) {                             \
                    ::std::fprintf(stderr, "Assertion failed: "             \
                        "(%s), function %s, file %s, line %i.\n",           \
                        message, __func__, __FILE__, __LINE__);             \
                    ::std::abort();                                         \
                }                                                           \
            }                                                               \
        )(__hana_tmp);                                                      \
    } while (false);                                                        \
    static_assert(true, "force trailing semicolon")                         \
/**/

//! @ingroup group-assertions
//! Equivalent to `BOOST_HANA__ASSERT`, but not influenced by the
//! `BOOST_HANA_CONFIG_DISABLE_ASSERTIONS` config macro. For internal use only.
#   define BOOST_HANA_CHECK(...)                                            \
        BOOST_HANA_CHECK_MSG(                                               \
            (__VA_ARGS__),                                                  \
            BOOST_HANA_PP_STRINGIZE(__VA_ARGS__)                            \
        )                                                                   \
/**/

//////////////////////////////////////////////////////////////////////////////
// BOOST_HANA_CONSTEXPR_CHECK and BOOST_HANA_CONSTEXPR_CHECK_MSG
//////////////////////////////////////////////////////////////////////////////

#if defined(BOOST_HANA_DOXYGEN_INVOKED)
    //! @ingroup group-assertions
    //! Equivalent to `BOOST_HANA_CONSTEXPR_ASSERT_MSG`, but not influenced by
    //! the `BOOST_HANA_CONFIG_DISABLE_ASSERTIONS` config macro.
    //! For internal use only.
#   define BOOST_HANA_CONSTEXPR_CHECK_MSG(condition, message) implementation-defined

    //! @ingroup group-assertions
    //! Equivalent to `BOOST_HANA_CONSTEXPR_ASSERT`, but not influenced by the
    //! `BOOST_HANA_CONFIG_DISABLE_ASSERTIONS` config macro.
    //! For internal use only.
#   define BOOST_HANA_CONSTEXPR_CHECK(...) implementation-defined

#elif defined(BOOST_HANA_CONFIG_HAS_CONSTEXPR_LAMBDA)

#   define BOOST_HANA_CONSTEXPR_CHECK_MSG(condition, message)               \
        static_assert(condition, message)                                   \
/**/

#   define BOOST_HANA_CONSTEXPR_CHECK(...)                                  \
        static_assert((__VA_ARGS__), BOOST_HANA_PP_STRINGIZE(__VA_ARGS__))  \
/**/

#else

#   define BOOST_HANA_CONSTEXPR_CHECK_MSG(condition, message)               \
        BOOST_HANA_RUNTIME_CHECK_MSG(condition, message)                    \
/**/

#   define BOOST_HANA_CONSTEXPR_CHECK(...)                                  \
        BOOST_HANA_CONSTEXPR_CHECK_MSG(                                     \
            (__VA_ARGS__),                                                  \
            BOOST_HANA_PP_STRINGIZE(__VA_ARGS__)                            \
        )                                                                   \
/**/

#endif

#endif // !BOOST_HANA_ASSERT_HPP
