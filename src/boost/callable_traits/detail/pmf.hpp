/*

@Copyright Barrett Adair 2015-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)

*/

#ifndef BOOST_CLBL_TRTS_DETAIL_PMF_HPP
#define BOOST_CLBL_TRTS_DETAIL_PMF_HPP

#include <boost/callable_traits/detail/forward_declarations.hpp>
#include <boost/callable_traits/detail/set_function_qualifiers.hpp>
#include <boost/callable_traits/detail/qualifier_flags.hpp>
#include <boost/callable_traits/detail/default_callable_traits.hpp>
#include <boost/callable_traits/detail/utility.hpp>

namespace boost { namespace callable_traits { namespace detail {

template<qualifier_flags Applied, bool IsTransactionSafe, bool IsNoExcept,
    typename CallingConvention, typename T, typename Return,
    typename... Args>
struct set_member_function_qualifiers_t;

template<qualifier_flags Applied, bool IsTransactionSafe, bool IsNoexcept,
    typename CallingConvention, typename T, typename Return,
    typename... Args>
struct set_varargs_member_function_qualifiers_t;

template<qualifier_flags Flags, bool IsTransactionSafe, bool IsNoexcept,
    typename... Ts>
using set_member_function_qualifiers =
    typename set_member_function_qualifiers_t<Flags, IsTransactionSafe,
        IsNoexcept, Ts...>::type;

template<qualifier_flags Flags, bool IsTransactionSafe, bool IsNoexcept,
    typename... Ts>
using set_varargs_member_function_qualifiers =
    typename set_varargs_member_function_qualifiers_t<Flags,
        IsTransactionSafe, IsNoexcept, Ts...>::type;

template<typename T>
struct pmf : default_callable_traits<T> {};

#define BOOST_CLBL_TRTS_CC_TAG dummy
#define BOOST_CLBL_TRTS_VARARGS_CC BOOST_CLBL_TRTS_DEFAULT_VARARGS_CC
#define BOOST_CLBL_TRTS_CC
#include <boost/callable_traits/detail/unguarded/pmf.hpp>
#undef BOOST_CLBL_TRTS_CC
#undef BOOST_CLBL_TRTS_CC_TAG
#undef BOOST_CLBL_TRTS_VARARGS_CC

#define BOOST_CLBL_TRTS_CC_TAG dummy
#define BOOST_CLBL_TRTS_VARARGS_CC BOOST_CLBL_TRTS_DEFAULT_VARARGS_CC
#define BOOST_CLBL_TRTS_CC
#include <boost/callable_traits/detail/unguarded/pmf_varargs.hpp>
#undef BOOST_CLBL_TRTS_CC
#undef BOOST_CLBL_TRTS_CC_TAG
#undef BOOST_CLBL_TRTS_VARARGS_CC

#ifdef BOOST_CLBL_TRTS_ENABLE_CDECL
#define BOOST_CLBL_TRTS_CC_TAG cdecl_tag
#define BOOST_CLBL_TRTS_VARARGS_CC __cdecl
#define BOOST_CLBL_TRTS_CC __cdecl
#include <boost/callable_traits/detail/unguarded/pmf.hpp>
#undef BOOST_CLBL_TRTS_CC
#undef BOOST_CLBL_TRTS_CC_TAG
#undef BOOST_CLBL_TRTS_VARARGS_CC
#endif // #ifdef BOOST_CLBL_TRTS_ENABLE_CDECL

// Defining this macro enables undocumented features, likely broken.
// Too much work to maintain, but knock yourself out
#ifdef BOOST_CLBL_TRTS_ENABLE_STDCALL
#define BOOST_CLBL_TRTS_CC_TAG stdcall_tag
#define BOOST_CLBL_TRTS_VARARGS_CC BOOST_CLBL_TRTS_DEFAULT_VARARGS_CC
#define BOOST_CLBL_TRTS_CC __stdcall
#include <boost/callable_traits/detail/unguarded/pmf.hpp>
#undef BOOST_CLBL_TRTS_CC
#undef BOOST_CLBL_TRTS_CC_TAG
#undef BOOST_CLBL_TRTS_VARARGS_CC
#endif // #ifdef BOOST_CLBL_TRTS_ENABLE_STDCALL

// Defining this macro enables undocumented features, likely broken.
// Too much work to officially maintain, but knock yourself out
#ifdef BOOST_CLBL_TRTS_ENABLE_FASTCALL
#define BOOST_CLBL_TRTS_CC_TAG fastcall_tag
#define BOOST_CLBL_TRTS_VARARGS_CC BOOST_CLBL_TRTS_DEFAULT_VARARGS_CC
#define BOOST_CLBL_TRTS_CC __fastcall
#include <boost/callable_traits/detail/unguarded/pmf.hpp>
#undef BOOST_CLBL_TRTS_CC
#undef BOOST_CLBL_TRTS_CC_TAG
#undef BOOST_CLBL_TRTS_VARARGS_CC
#endif // #ifdef BOOST_CLBL_TRTS_ENABLE_FASTCALL

}}} // namespace boost::callable_traits::detail

#endif // #ifndef BOOST_CLBL_TRTS_DETAIL_PMF_HPP
