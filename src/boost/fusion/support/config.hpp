/*=============================================================================
    Copyright (c) 2014 Eric Niebler
    Copyright (c) 2014,2015,2018 Kohei Takahashi

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(FUSION_SUPPORT_CONFIG_01092014_1718)
#define FUSION_SUPPORT_CONFIG_01092014_1718

#include <boost/config.hpp>
#include <boost/detail/workaround.hpp>
#include <utility>

#ifndef BOOST_FUSION_GPU_ENABLED
#define BOOST_FUSION_GPU_ENABLED BOOST_GPU_ENABLED
#endif

// Enclose with inline namespace because unqualified lookup of GCC < 4.5 is broken.
//
//      namespace detail {
//          struct foo;
//          struct X { };
//      }
//
//      template <typename T> void foo(T) { }
//
//      int main()
//      {
//            foo(detail::X());
//            // prog.cc: In function 'int main()':
//            // prog.cc:2: error: 'struct detail::foo' is not a function,
//            // prog.cc:6: error: conflict with 'template<class T> void foo(T)'
//            // prog.cc:10: error: in call to 'foo'
//      }
namespace boost { namespace fusion { namespace detail
{
    namespace barrier { }
    using namespace barrier;
}}}
#define BOOST_FUSION_BARRIER_BEGIN namespace barrier {
#define BOOST_FUSION_BARRIER_END   }


#if BOOST_WORKAROUND(BOOST_MSVC, BOOST_TESTED_AT(1900))
// All of rvalue-reference ready MSVC don't perform implicit conversion from
// fundamental type to rvalue-reference of another fundamental type [1].
//
// Following example doesn't compile
//
//   int i;
//   long &&l = i; // sigh..., std::forward<long&&>(i) also fail.
//
// however, following one will work.
//
//   int i;
//   long &&l = static_cast<long &&>(i);
//
// OK, now can we replace all usage of std::forward to static_cast? -- I say NO!
// All of rvalue-reference ready Clang doesn't compile above static_cast usage [2], sigh...
//
// References:
// 1. https://connect.microsoft.com/VisualStudio/feedback/details/1037806/implicit-conversion-doesnt-perform-for-fund
// 2. http://llvm.org/bugs/show_bug.cgi?id=19917
//
// Tentatively, we use static_cast to forward if run under MSVC.
#   define BOOST_FUSION_FWD_ELEM(type, value) static_cast<type&&>(value)
#else
#   define BOOST_FUSION_FWD_ELEM(type, value) std::forward<type>(value)
#endif


// Workaround for LWG 2408: C++17 SFINAE-friendly std::iterator_traits.
// http://cplusplus.github.io/LWG/lwg-defects.html#2408
//
// - GCC 4.5 enables the feature under C++11.
//   https://gcc.gnu.org/ml/gcc-patches/2014-11/msg01105.html
//
// - MSVC 10.0 implements iterator intrinsics; MSVC 13.0 implements LWG2408.
#if (defined(BOOST_LIBSTDCXX_VERSION) && (BOOST_LIBSTDCXX_VERSION < 40500) && \
     defined(BOOST_LIBSTDCXX11)) || \
    (defined(BOOST_MSVC) && (1600 <= BOOST_MSVC && BOOST_MSVC < 1900))
#   define BOOST_FUSION_WORKAROUND_FOR_LWG_2408
namespace std
{
    template <typename>
    struct iterator_traits;
}
#endif


// Workaround for older GCC that doesn't accept `this` in constexpr.
#if BOOST_WORKAROUND(BOOST_GCC, < 40700)
#define BOOST_FUSION_CONSTEXPR_THIS
#else
#define BOOST_FUSION_CONSTEXPR_THIS BOOST_CONSTEXPR
#endif


// Workaround for compilers not implementing N3031 (DR743 and DR950).
#if BOOST_WORKAROUND(BOOST_MSVC, BOOST_TESTED_AT(1913)) || \
    BOOST_WORKAROUND(BOOST_GCC, < 40700) || \
    defined(BOOST_CLANG) && (__clang_major__ == 3 && __clang_minor__ == 0)
# if !defined(BOOST_NO_CXX11_TEMPLATE_ALIASES)
namespace boost { namespace fusion { namespace detail
{
    template <typename T>
    using type_alias_t = T;
}}}
#   define BOOST_FUSION_DECLTYPE_N3031(parenthesized_expr) \
        boost::fusion::detail::type_alias_t<decltype parenthesized_expr>
# else
#   include <boost/mpl/identity.hpp>
#   define BOOST_FUSION_DECLTYPE_N3031(parenthesized_expr) \
        boost::mpl::identity<decltype parenthesized_expr>::type
# endif
#else
#   define BOOST_FUSION_DECLTYPE_N3031(parenthesized_expr) \
        decltype parenthesized_expr
#endif


// Workaround for GCC 4.6 that rejects defaulted function with noexcept.
#if BOOST_WORKAROUND(BOOST_GCC, / 100 == 406)
#   define BOOST_FUSION_NOEXCEPT_ON_DEFAULTED
#else
#   define BOOST_FUSION_NOEXCEPT_ON_DEFAULTED BOOST_NOEXCEPT
#endif

#endif
