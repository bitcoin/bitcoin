
//          Copyright Oliver Kowalke 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_CONTEXT_DETAIL_CONFIG_H
#define BOOST_CONTEXT_DETAIL_CONFIG_H

// required for SD-6 compile-time integer sequences
#include <utility>

#include <boost/config.hpp>
#include <boost/detail/workaround.hpp>

#ifdef BOOST_CONTEXT_DECL
# undef BOOST_CONTEXT_DECL
#endif

#if (defined(BOOST_ALL_DYN_LINK) || defined(BOOST_CONTEXT_DYN_LINK) ) && ! defined(BOOST_CONTEXT_STATIC_LINK)
# if defined(BOOST_CONTEXT_SOURCE)
#  define BOOST_CONTEXT_DECL BOOST_SYMBOL_EXPORT
#  define BOOST_CONTEXT_BUILD_DLL
# else
#  define BOOST_CONTEXT_DECL BOOST_SYMBOL_IMPORT
# endif
#endif

#if ! defined(BOOST_CONTEXT_DECL)
# define BOOST_CONTEXT_DECL
#endif

#if ! defined(BOOST_USE_UCONTEXT) && defined(__CYGWIN__)
# define BOOST_USE_UCONTEXT
#endif

#if ! defined(BOOST_CONTEXT_SOURCE) && ! defined(BOOST_ALL_NO_LIB) && ! defined(BOOST_CONTEXT_NO_LIB)
# define BOOST_LIB_NAME boost_context
# if defined(BOOST_ALL_DYN_LINK) || defined(BOOST_CONTEXT_DYN_LINK)
#  define BOOST_DYN_LINK
# endif
# include <boost/config/auto_link.hpp>
#endif

#undef BOOST_CONTEXT_CALLDECL
#if (defined(i386) || defined(__i386__) || defined(__i386) \
     || defined(__i486__) || defined(__i586__) || defined(__i686__) \
     || defined(__X86__) || defined(_X86_) || defined(__THW_INTEL__) \
     || defined(__I86__) || defined(__INTEL__) || defined(__IA32__) \
     || defined(_M_IX86) || defined(_I86_)) && defined(BOOST_WINDOWS)
# define BOOST_CONTEXT_CALLDECL __cdecl
#else
# define BOOST_CONTEXT_CALLDECL
#endif

#if defined(BOOST_USE_SEGMENTED_STACKS)
# if ! ( (defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6) ) ) || \
         (defined(__clang__) && (__clang_major__ > 2 || ( __clang_major__ == 2 && __clang_minor__ > 3) ) ) )
#  error "compiler does not support segmented_stack stacks"
# endif
# define BOOST_CONTEXT_SEGMENTS 10
#endif


#define BOOST_CONTEXT_NO_CXX14_INTEGER_SEQUENCE
// use rd6 macros for std::integer_sequence
#if defined(__cpp_lib_integer_sequence) && __cpp_lib_integer_sequence >= 201304
# undef BOOST_CONTEXT_NO_CXX14_INTEGER_SEQUENCE
#endif
// workaroud: MSVC 14 does not provide macros to test for compile-time integer sequence
#if _MSC_VER > 1800 // _MSC_VER == 1800 -> MS Visual Studio 2013
# undef BOOST_CONTEXT_NO_INDEX_SEQUENCE
#endif
// workaround: Xcode clang feature detection
#if ! defined(__cpp_lib_integer_sequence) && __cpp_lib_integer_sequence >= 201304
# if _LIBCPP_STD_VER > 11
#  undef BOOST_CONTEXT_NO_CXX14_INTEGER_SEQUENCE
# endif
#endif

// workaroud: MSVC 14 does support constexpr
#if _MSC_VER > 1800 // _MSC_VER == 1800 -> MS Visual Studio 2013
# undef BOOST_NO_CXX11_CONSTEXPR
#endif

#undef BOOST_CONTEXT_NO_CXX11
#if defined(BOOST_NO_CXX11_AUTO_DECLARATIONS) || \
    defined(BOOST_NO_CXX11_CONSTEXPR) || \
    defined(BOOST_NO_CXX11_DEFAULTED_FUNCTIONS) || \
    defined(BOOST_NO_CXX11_FINAL) || \
    defined(BOOST_NO_CXX11_HDR_TUPLE) || \
    defined(BOOST_NO_CXX11_NOEXCEPT) || \
    defined(BOOST_NO_CXX11_NULLPTR) || \
    defined(BOOST_NO_CXX11_RVALUE_REFERENCES) || \
    defined(BOOST_NO_CXX11_TEMPLATE_ALIASES) || \
    defined(BOOST_NO_CXX11_UNIFIED_INITIALISATION_SYNTAX) || \
    defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES) || \
    defined(BOOST_NO_HDR_ATOMIC) || \
    defined(BOOST_NO_HDR_TUPLE) 
# define BOOST_CONTEXT_NO_CXX11
#endif

#if ! defined(BOOST_EXECUTION_CONTEXT)
# if defined(BOOST_USE_SEGMENTED_STACKS)
#  define BOOST_EXECUTION_CONTEXT 1
# else
#  define BOOST_EXECUTION_CONTEXT 2
# endif
#endif

#if ! defined(BOOST_NO_CXX11_CONSTEXPR)
// modern architectures have cachelines with 64byte length
// ARM Cortex-A15 32/64byte, Cortex-A9 16/32/64bytes
// MIPS 74K: 32byte, 4KEc: 16byte
// ist should be safe to use 64byte for all
static constexpr std::size_t cache_alignment{ 64 };
static constexpr std::size_t cacheline_length{ 64 };
// lookahead size for prefetching
static constexpr std::size_t prefetch_stride{ 4 * cacheline_length };
#endif

#if defined(__GLIBCPP__) || defined(__GLIBCXX__)
// GNU libstdc++ 3
#  define BOOST_CONTEXT_HAS_CXXABI_H
#endif

#if defined( BOOST_CONTEXT_HAS_CXXABI_H )
# include <cxxabi.h>
#endif

#if defined(__OpenBSD__)
// stacks need mmap(2) with MAP_STACK
# define BOOST_CONTEXT_USE_MAP_STACK
#endif

#endif // BOOST_CONTEXT_DETAIL_CONFIG_H
