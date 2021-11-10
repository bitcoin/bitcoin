//
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/json
//

#ifndef BOOST_JSON_DETAIL_CONFIG_HPP
#define BOOST_JSON_DETAIL_CONFIG_HPP

#ifndef BOOST_JSON_STANDALONE
# include <boost/config.hpp>
# include <boost/assert.hpp>
# include <boost/throw_exception.hpp>
#else
# include <cassert>
#endif
#include <cstdint>
#include <type_traits>
#include <utility>

// detect 32/64 bit
#if UINTPTR_MAX == UINT64_MAX
# define BOOST_JSON_ARCH 64
#elif UINTPTR_MAX == UINT32_MAX
# define BOOST_JSON_ARCH 32
#else
# error Unknown or unsupported architecture, please open an issue
#endif

// VFALCO Copied from Boost.Config
//        This is a derivative work.
#ifndef BOOST_JSON_NODISCARD
# ifdef __has_cpp_attribute
// clang-6 accepts [[nodiscard]] with -std=c++14, but warns about it -pedantic
#  if __has_cpp_attribute(nodiscard) && !(defined(__clang__) && (__cplusplus < 201703L))
#   define BOOST_JSON_NODISCARD [[nodiscard]]
#  else
#   define BOOST_JSON_NODISCARD
#  endif
# else
#  define BOOST_JSON_NODISCARD
# endif
#endif

#ifndef BOOST_JSON_REQUIRE_CONST_INIT
# define BOOST_JSON_REQUIRE_CONST_INIT
# if __cpp_constinit >= 201907L
#  undef BOOST_JSON_REQUIRE_CONST_INIT
#  define BOOST_JSON_REQUIRE_CONST_INIT constinit
# elif defined(__clang__) && defined(__has_cpp_attribute)
#  if __has_cpp_attribute(clang::require_constant_initialization)
#   undef BOOST_JSON_REQUIRE_CONST_INIT
#   define BOOST_JSON_REQUIRE_CONST_INIT [[clang::require_constant_initialization]]
#  endif
# endif
#endif

#ifndef BOOST_JSON_NO_DESTROY
# if defined(__clang__) && defined(__has_cpp_attribute)
#  if __has_cpp_attribute(clang::no_destroy)
#   define BOOST_JSON_NO_DESTROY [[clang::no_destroy]]
#  endif
# endif
#endif

// BOOST_NORETURN ---------------------------------------------//
// Macro to use before a function declaration/definition to designate
// the function as not returning normally (i.e. with a return statement
// or by leaving the function scope, if the function return type is void).
#if !defined(BOOST_NORETURN)
#  if defined(_MSC_VER)
#    define BOOST_NORETURN __declspec(noreturn)
#  elif defined(__GNUC__)
#    define BOOST_NORETURN __attribute__ ((__noreturn__))
#  elif defined(__has_attribute) && defined(__SUNPRO_CC) && (__SUNPRO_CC > 0x5130)
#    if __has_attribute(noreturn)
#      define BOOST_NORETURN [[noreturn]]
#    endif
#  elif defined(__has_cpp_attribute)
#    if __has_cpp_attribute(noreturn)
#      define BOOST_NORETURN [[noreturn]]
#    endif
#  endif
#endif

#ifndef BOOST_ASSERT
#define BOOST_ASSERT assert
#endif

#ifndef BOOST_STATIC_ASSERT
#define BOOST_STATIC_ASSERT( ... ) static_assert(__VA_ARGS__, #__VA_ARGS__)
#endif

#ifndef BOOST_FALLTHROUGH
#define BOOST_FALLTHROUGH [[fallthrough]]
#endif

#ifndef BOOST_FORCEINLINE
# ifdef _MSC_VER
#  define BOOST_FORCEINLINE __forceinline
# elif defined(__GNUC__) || defined(__clang__)
#  define BOOST_FORCEINLINE inline __attribute__((always_inline))
# else
#  define BOOST_FORCEINLINE inline
# endif
#endif

#ifndef BOOST_NOINLINE
# ifdef _MSC_VER
#  define BOOST_NOINLINE __declspec(noinline)
# elif defined(__GNUC__) || defined(__clang__)
#  define BOOST_NOINLINE __attribute__((noinline))
# else
#  define BOOST_NOINLINE
# endif
#endif

#ifndef BOOST_THROW_EXCEPTION
# ifndef BOOST_NO_EXCEPTIONS
#  define BOOST_THROW_EXCEPTION(x) throw(x)
# else
#  define BOOST_THROW_EXCEPTION(x) do{}while(0)
# endif
#endif

#if ! defined(BOOST_JSON_NO_SSE2) && \
    ! defined(BOOST_JSON_USE_SSE2)
# if (defined(_M_IX86) && _M_IX86_FP == 2) || \
      defined(_M_X64) || defined(__SSE2__)
#  define BOOST_JSON_USE_SSE2
# endif
#endif

#ifndef BOOST_SYMBOL_VISIBLE
#define BOOST_SYMBOL_VISIBLE
#endif

#ifdef BOOST_JSON_STANDALONE
# define BOOST_JSON_NS_BEGIN \
    namespace boost { \
    namespace json { \
    inline namespace standalone {
# define BOOST_JSON_NS_END } } }
#elif ! defined(BOOST_JSON_DOCS)
# define BOOST_JSON_NS_BEGIN \
    namespace boost { \
    namespace json {
# define BOOST_JSON_NS_END } }
#endif

#ifndef BOOST_JSON_STANDALONE
# if defined(BOOST_JSON_DOCS)
#  define BOOST_JSON_DECL
# else
#  if (defined(BOOST_JSON_DYN_LINK) || defined(BOOST_ALL_DYN_LINK)) && !defined(BOOST_JSON_STATIC_LINK)
#   if defined(BOOST_JSON_SOURCE)
#    define BOOST_JSON_DECL        BOOST_SYMBOL_EXPORT
#    define BOOST_JSON_CLASS_DECL  BOOST_SYMBOL_EXPORT
#    define BOOST_JSON_BUILD_DLL
#   else
#    define BOOST_JSON_DECL        BOOST_SYMBOL_IMPORT
#    define BOOST_JSON_CLASS_DECL  BOOST_SYMBOL_IMPORT
#   endif
#  endif // shared lib
#  ifndef  BOOST_JSON_DECL
#   define BOOST_JSON_DECL
#  endif
#  if !defined(BOOST_JSON_SOURCE) && !defined(BOOST_ALL_NO_LIB) && !defined(BOOST_JSON_NO_LIB)
#   define BOOST_LIB_NAME boost_json
#   if defined(BOOST_ALL_DYN_LINK) || defined(BOOST_JSON_DYN_LINK)
#    define BOOST_DYN_LINK
#   endif
#   include <boost/config/auto_link.hpp>
#  endif
# endif
#else
// For standalone, shared library builds, users must manually
// define the macros BOOST_JSON_DECL and BOOST_JSON_CLASS_DECL
#endif

#ifndef BOOST_JSON_DECL
#define BOOST_JSON_DECL
#endif
#ifndef BOOST_JSON_CLASS_DECL
#define BOOST_JSON_CLASS_DECL
#endif

#ifndef BOOST_JSON_LIKELY
# if defined(__GNUC__) || defined(__clang__)
#  define BOOST_JSON_LIKELY(x) __builtin_expect(!!(x), 1)
# else
#  define BOOST_JSON_LIKELY(x) x
# endif
#endif

#ifndef BOOST_JSON_UNLIKELY
# if defined(__GNUC__) || defined(__clang__)
#  define BOOST_JSON_UNLIKELY(x) __builtin_expect(!!(x), 0)
# else
#  define BOOST_JSON_UNLIKELY(x) x
# endif
#endif

#ifndef BOOST_JSON_UNREACHABLE
# define BOOST_JSON_UNREACHABLE() static_cast<void>(0)
# ifdef _MSC_VER
#  undef BOOST_JSON_UNREACHABLE
#  define BOOST_JSON_UNREACHABLE() __assume(0)
# elif defined(__has_builtin)
#  if __has_builtin(__builtin_unreachable)
#   undef BOOST_JSON_UNREACHABLE
#   define BOOST_JSON_UNREACHABLE() __builtin_unreachable()
#  endif
# endif
#endif

#ifndef BOOST_JSON_ASSUME
# define BOOST_JSON_ASSUME(x) (!!(x) ? void() : BOOST_JSON_UNREACHABLE())
# ifdef _MSC_VER
#  undef BOOST_JSON_ASSUME
#  define BOOST_JSON_ASSUME(x) __assume(!!(x))
# elif defined(__has_builtin)
#  if __has_builtin(__builtin_assume)
#   undef BOOST_JSON_ASSUME
#   define BOOST_JSON_ASSUME(x) __builtin_assume(!!(x))
#  endif
# endif
#endif

// older versions of msvc and clang don't always
// constant initialize when they are supposed to
#ifndef BOOST_JSON_WEAK_CONSTINIT
# if defined(_MSC_VER) && ! defined(__clang__) && _MSC_VER < 1920
#  define BOOST_JSON_WEAK_CONSTINIT
# elif defined(__clang__) && __clang_major__ < 4
#  define BOOST_JSON_WEAK_CONSTINIT
# endif
#endif

// These macros are private, for tests, do not change
// them or else previously built libraries won't match.
#ifndef  BOOST_JSON_MAX_STRING_SIZE
# define BOOST_JSON_NO_MAX_STRING_SIZE
# define BOOST_JSON_MAX_STRING_SIZE  0x7ffffffe
#endif
#ifndef  BOOST_JSON_MAX_STRUCTURED_SIZE
# define BOOST_JSON_NO_MAX_STRUCTURED_SIZE
# define BOOST_JSON_MAX_STRUCTURED_SIZE  0x7ffffffe
#endif
#ifndef  BOOST_JSON_STACK_BUFFER_SIZE
# define BOOST_JSON_NO_STACK_BUFFER_SIZE
# if defined(__i386__) || defined(__x86_64__) || \
     defined(_M_IX86)  || defined(_M_X64)
#  define BOOST_JSON_STACK_BUFFER_SIZE 4096
# else
// If we are not on Intel, then assume we are on
// embedded and use a smaller stack size. If this
// is not suitable, the user can define the macro
// themselves when building the library or including
// src.hpp.
#  define BOOST_JSON_STACK_BUFFER_SIZE 256
# endif
#endif

BOOST_JSON_NS_BEGIN
namespace detail {

template<class...>
struct make_void
{
    using type =void;
};

template<class... Ts>
using void_t = typename
    make_void<Ts...>::type;

template<class T>
using remove_cvref = typename
    std::remove_cv<typename
        std::remove_reference<T>::type>::type;

template<class T, class U>
T exchange(T& t, U u) noexcept
{
    T v = std::move(t);
    t = std::move(u);
    return v;
}

/*  This is a derivative work, original copyright:

    Copyright Eric Niebler 2013-present

    Use, modification and distribution is subject to the
    Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)

    Project home: https://github.com/ericniebler/range-v3
*/
template<typename T>
struct static_const
{
    static constexpr T value {};
};
template<typename T>
constexpr T static_const<T>::value;

#define BOOST_JSON_INLINE_VARIABLE(name, type) \
    namespace { constexpr auto& name = \
        ::boost::json::detail::static_const<type>::value; \
    } struct _unused_ ## name ## _semicolon_bait_

} // detail
BOOST_JSON_NS_END

#endif
