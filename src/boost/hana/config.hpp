/*!
@file
Defines configuration macros used throughout the library.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_CONFIG_HPP
#define BOOST_HANA_CONFIG_HPP

#include <boost/hana/version.hpp>


//////////////////////////////////////////////////////////////////////////////
// Detect the compiler
//////////////////////////////////////////////////////////////////////////////

#if defined(_MSC_VER) && !defined(__clang__) // MSVC
    // This must be checked first, because otherwise it produces a fatal
    // error due to unrecognized #warning directives used below.

#   if _MSC_VER < 1915
#       pragma message("Warning: the native Microsoft compiler is not supported due to lack of proper C++14 support.")
#   else
        // 1. Active issues
        // Multiple copy/move ctors
#       define BOOST_HANA_WORKAROUND_MSVC_MULTIPLECTOR_106654

        // 2. Issues fixed in the development branch of MSVC
        // Forward declaration of class template member function returning decltype(auto)
#       define BOOST_HANA_WORKAROUND_MSVC_DECLTYPEAUTO_RETURNTYPE_662735

        // 3. Issues fixed conditionally
        // Requires __declspec(empty_bases)
        // Empty base optimization
#       define BOOST_HANA_WORKAROUND_MSVC_EMPTYBASE

        // Requires /experimental:preprocessor
        // Variadic macro expansion
#       if !defined(_MSVC_TRADITIONAL) || _MSVC_TRADITIONAL
#           define BOOST_HANA_WORKAROUND_MSVC_PREPROCESSOR_616033
#       endif
#   endif

#elif defined(__clang__) && defined(_MSC_VER) // Clang-cl (Clang for Windows)

#   define BOOST_HANA_CONFIG_CLANG BOOST_HANA_CONFIG_VERSION(               \
                    __clang_major__, __clang_minor__, __clang_patchlevel__)

#elif defined(__clang__) && defined(__apple_build_version__) // Apple's Clang

#   if __apple_build_version__ >= 6020049
#       define BOOST_HANA_CONFIG_CLANG BOOST_HANA_CONFIG_VERSION(3, 6, 0)
#   endif

#elif defined(__clang__) // genuine Clang

#   define BOOST_HANA_CONFIG_CLANG BOOST_HANA_CONFIG_VERSION(               \
                __clang_major__, __clang_minor__, __clang_patchlevel__)

#elif defined(__GNUC__) // GCC

#   define BOOST_HANA_CONFIG_GCC BOOST_HANA_CONFIG_VERSION(                 \
                            __GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__)

#endif

//////////////////////////////////////////////////////////////////////////////
// Check the compiler for general C++14 capabilities
//////////////////////////////////////////////////////////////////////////////
#if (__cplusplus < 201400)
#   if defined(_MSC_VER)
#       if _MSC_VER < 1915
#           pragma message("Warning: Your compiler doesn't provide C++14 or higher capabilities. Try adding the compiler flag '-std=c++14' or '-std=c++1y'.")
#       endif
#   else
#       warning "Your compiler doesn't provide C++14 or higher capabilities. Try adding the compiler flag '-std=c++14' or '-std=c++1y'."
#   endif
#endif

//////////////////////////////////////////////////////////////////////////////
// Caveats and other compiler-dependent options
//////////////////////////////////////////////////////////////////////////////

// `BOOST_HANA_CONFIG_HAS_CONSTEXPR_LAMBDA` enables some constructs requiring
// `constexpr` lambdas, which are in the language starting with C++17.
//
// Always disabled for now because Clang only has partial support for them
// (captureless lambdas only).
#if defined(__cplusplus) && __cplusplus > 201402L
#   define BOOST_HANA_CONSTEXPR_STATELESS_LAMBDA constexpr
// #   define BOOST_HANA_CONFIG_HAS_CONSTEXPR_LAMBDA
#else
#   define BOOST_HANA_CONSTEXPR_STATELESS_LAMBDA /* nothing */
#endif

// `BOOST_HANA_CONSTEXPR_LAMBDA` expands to `constexpr` if constexpr lambdas
// are supported and to nothing otherwise.
#if defined(BOOST_HANA_CONFIG_HAS_CONSTEXPR_LAMBDA)
#   define BOOST_HANA_CONSTEXPR_LAMBDA constexpr
#else
#   define BOOST_HANA_CONSTEXPR_LAMBDA /* nothing */
#endif

// `BOOST_HANA_INLINE_VARIABLE` expands to `inline` when C++17 inline variables
// are supported, and to nothing otherwise. This allows marking global variables
// defined in a header as `inline` to avoid potential ODR violations.
#if defined(__cplusplus) && __cplusplus > 201402L
#   define BOOST_HANA_INLINE_VARIABLE inline
#else
#   define BOOST_HANA_INLINE_VARIABLE /* nothing */
#endif

//////////////////////////////////////////////////////////////////////////////
// Namespace macros
//////////////////////////////////////////////////////////////////////////////
#define BOOST_HANA_NAMESPACE_BEGIN namespace boost { namespace hana {

#define BOOST_HANA_NAMESPACE_END }}

//////////////////////////////////////////////////////////////////////////////
// Library features and options that can be tweaked by users
//////////////////////////////////////////////////////////////////////////////

#if defined(BOOST_HANA_DOXYGEN_INVOKED) || \
    (defined(NDEBUG) && !defined(BOOST_HANA_CONFIG_DISABLE_ASSERTIONS))
    //! @ingroup group-config
    //! Disables the `BOOST_HANA_*_ASSERT` macro & friends.
    //!
    //! When this macro is defined, the `BOOST_HANA_*_ASSERT` macro & friends
    //! are disabled, i.e. they expand to nothing.
    //!
    //! This macro is defined automatically when `NDEBUG` is defined. It can
    //! also be defined by users before including this header or defined on
    //! the command line.
#   define BOOST_HANA_CONFIG_DISABLE_ASSERTIONS
#endif

#if defined(BOOST_HANA_DOXYGEN_INVOKED)
    //! @ingroup group-config
    //! Disables concept checks in interface methods.
    //!
    //! When this macro is not defined (the default), tag-dispatched methods
    //! will make sure the arguments they are passed are models of the proper
    //! concept(s). This can be very helpful in catching programming errors,
    //! but it is also slightly less compile-time efficient. You should
    //! probably always leave the checks enabled (and hence never define this
    //! macro), except perhaps in translation units that are compiled very
    //! often but whose code using Hana is modified very rarely.
#   define BOOST_HANA_CONFIG_DISABLE_CONCEPT_CHECKS
#endif

#if defined(BOOST_HANA_DOXYGEN_INVOKED)
    //! @ingroup group-config
    //! Enables usage of the "string literal operator template" GNU extension.
    //!
    //! That operator is not part of the language yet, but it is supported by
    //! both Clang and GCC. This operator allows Hana to provide the nice `_s`
    //! user-defined literal for creating compile-time strings.
    //!
    //! When this macro is not defined, the GNU extension will be not used
    //! by Hana. Because this is a non-standard extension, the macro is not
    //! defined by default.
#   define BOOST_HANA_CONFIG_ENABLE_STRING_UDL
#endif

#if defined(BOOST_HANA_DOXYGEN_INVOKED)
    //! @ingroup group-config
    //! Enables additional assertions and sanity checks to be done by Hana.
    //!
    //! When this macro is defined (it is __not defined__ by default),
    //! additional sanity checks may be done by Hana. These checks may
    //! be costly to perform, either in terms of compilation time or in
    //! terms of execution time. These checks may help debugging an
    //! application during its initial development, but they should not
    //! be enabled as part of the normal configuration.
#   define BOOST_HANA_CONFIG_ENABLE_DEBUG_MODE
#endif

#endif // !BOOST_HANA_CONFIG_HPP
