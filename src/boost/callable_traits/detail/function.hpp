/*

@Copyright Barrett Adair 2015-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)

*/

#ifndef BOOST_CLBL_TRTS_DETAIL_FUNCTION_HPP
#define BOOST_CLBL_TRTS_DETAIL_FUNCTION_HPP

#include <boost/callable_traits/detail/config.hpp>
#include <boost/callable_traits/detail/qualifier_flags.hpp>
#include <boost/callable_traits/detail/forward_declarations.hpp>
#include <boost/callable_traits/detail/set_function_qualifiers.hpp>
#include <boost/callable_traits/detail/default_callable_traits.hpp>

namespace boost { namespace callable_traits { namespace detail {

template<typename T>
struct function : default_callable_traits<T> {};

#undef BOOST_CLBL_TRTS_INCLUDE_QUALIFIERS
#define BOOST_CLBL_TRTS_INCLUDE_QUALIFIERS
#include <boost/callable_traits/detail/unguarded/function.hpp>
#undef BOOST_CLBL_TRTS_INCLUDE_QUALIFIERS

#ifndef BOOST_CLBL_TRTS_DISABLE_ABOMINABLE_FUNCTIONS

#define BOOST_CLBL_TRTS_INCLUDE_QUALIFIERS const
#include <boost/callable_traits/detail/unguarded/function.hpp>
#undef BOOST_CLBL_TRTS_INCLUDE_QUALIFIERS

#define BOOST_CLBL_TRTS_INCLUDE_QUALIFIERS volatile
#include <boost/callable_traits/detail/unguarded/function.hpp>
#undef BOOST_CLBL_TRTS_INCLUDE_QUALIFIERS

#define BOOST_CLBL_TRTS_INCLUDE_QUALIFIERS const volatile
#include <boost/callable_traits/detail/unguarded/function.hpp>
#undef BOOST_CLBL_TRTS_INCLUDE_QUALIFIERS

#ifndef BOOST_CLBL_TRTS_DISABLE_REFERENCE_QUALIFIERS

#define BOOST_CLBL_TRTS_INCLUDE_QUALIFIERS &
#include <boost/callable_traits/detail/unguarded/function.hpp>
#undef BOOST_CLBL_TRTS_INCLUDE_QUALIFIERS

#define BOOST_CLBL_TRTS_INCLUDE_QUALIFIERS &&
#include <boost/callable_traits/detail/unguarded/function.hpp>
#undef BOOST_CLBL_TRTS_INCLUDE_QUALIFIERS

#define BOOST_CLBL_TRTS_INCLUDE_QUALIFIERS const &
#include <boost/callable_traits/detail/unguarded/function.hpp>
#undef BOOST_CLBL_TRTS_INCLUDE_QUALIFIERS

#define BOOST_CLBL_TRTS_INCLUDE_QUALIFIERS const &&
#include <boost/callable_traits/detail/unguarded/function.hpp>
#undef BOOST_CLBL_TRTS_INCLUDE_QUALIFIERS

#define BOOST_CLBL_TRTS_INCLUDE_QUALIFIERS volatile &
#include <boost/callable_traits/detail/unguarded/function.hpp>
#undef BOOST_CLBL_TRTS_INCLUDE_QUALIFIERS

#define BOOST_CLBL_TRTS_INCLUDE_QUALIFIERS volatile &&
#include <boost/callable_traits/detail/unguarded/function.hpp>
#undef BOOST_CLBL_TRTS_INCLUDE_QUALIFIERS

#define BOOST_CLBL_TRTS_INCLUDE_QUALIFIERS const volatile &
#include <boost/callable_traits/detail/unguarded/function.hpp>
#undef BOOST_CLBL_TRTS_INCLUDE_QUALIFIERS

#define BOOST_CLBL_TRTS_INCLUDE_QUALIFIERS const volatile &&
#include <boost/callable_traits/detail/unguarded/function.hpp>
#undef BOOST_CLBL_TRTS_INCLUDE_QUALIFIERS

#endif // #ifndef BOOST_CLBL_TRTS_DISABLE_REFERENCE_QUALIFIERS
#endif // #ifndef BOOST_CLBL_TRTS_DISABLE_ABOMINABLE_FUNCTIONS

// function pointers

#define BOOST_CLBL_TRTS_CC_TAG dummy
#define BOOST_CLBL_TRTS_VARARGS_CC BOOST_CLBL_TRTS_DEFAULT_VARARGS_CC
#define BOOST_CLBL_TRTS_CC
#define BOOST_CLBL_TRTS_ST
#include <boost/callable_traits/detail/unguarded/function_ptr.hpp>
#include <boost/callable_traits/detail/unguarded/function_ptr_varargs.hpp>
#undef BOOST_CLBL_TRTS_ST
#undef BOOST_CLBL_TRTS_CC
#undef BOOST_CLBL_TRTS_CC_TAG
#undef BOOST_CLBL_TRTS_VARARGS_CC

/* ?
#ifdef BOOST_CLBL_TRTS_ENABLE_CDECL
#define BOOST_CLBL_TRTS_CC_TAG cdecl_tag
#define BOOST_CLBL_TRTS_VARARGS_CC __cdecl
#define BOOST_CLBL_TRTS_CC __cdecl
#define BOOST_CLBL_TRTS_ST
#include <boost/callable_traits/detail/unguarded/function_ptr.hpp>
#undef BOOST_CLBL_TRTS_ST
#undef BOOST_CLBL_TRTS_CC
#undef BOOST_CLBL_TRTS_CC_TAG
#undef BOOST_CLBL_TRTS_VARARGS_CC
#endif*/

#ifdef BOOST_CLBL_TRTS_ENABLE_STDCALL
#define BOOST_CLBL_TRTS_CC_TAG stdcall_tag
#define BOOST_CLBL_TRTS_VARARGS_CC BOOST_CLBL_TRTS_DEFAULT_VARARGS_CC
#define BOOST_CLBL_TRTS_CC __stdcall
#define BOOST_CLBL_TRTS_ST
#include <boost/callable_traits/detail/unguarded/function_ptr.hpp>
#undef BOOST_CLBL_TRTS_ST
#undef BOOST_CLBL_TRTS_CC
#undef BOOST_CLBL_TRTS_CC_TAG
#undef BOOST_CLBL_TRTS_VARARGS_CC
#endif

#ifdef BOOST_CLBL_TRTS_ENABLE_FASTCALL
#define BOOST_CLBL_TRTS_CC_TAG fastcall_tag
#define BOOST_CLBL_TRTS_VARARGS_CC BOOST_CLBL_TRTS_DEFAULT_VARARGS_CC
#define BOOST_CLBL_TRTS_CC __fastcall
#define BOOST_CLBL_TRTS_ST
#include <boost/callable_traits/detail/unguarded/function_ptr.hpp>
#undef BOOST_CLBL_TRTS_CC
#undef BOOST_CLBL_TRTS_ST
#undef BOOST_CLBL_TRTS_CC_TAG
#undef BOOST_CLBL_TRTS_VARARGS_CC
#endif

#ifdef BOOST_CLBL_TRTS_ENABLE_PASCAL
#define BOOST_CLBL_TRTS_CC_TAG pascal_tag
#define BOOST_CLBL_TRTS_VARARGS_CC BOOST_CLBL_TRTS_DEFAULT_VARARGS_CC
#define BOOST_CLBL_TRTS_CC
#define BOOST_CLBL_TRTS_ST pascal
#include <boost/callable_traits/detail/unguarded/function_ptr.hpp>
#undef BOOST_CLBL_TRTS_CC
#undef BOOST_CLBL_TRTS_ST
#undef BOOST_CLBL_TRTS_CC_TAG
#undef BOOST_CLBL_TRTS_VARARGS_CC
#endif

template<typename T>
struct function<T&> : std::conditional<function<T>::value,
    function<T>, default_callable_traits<T&>>::type {

    static constexpr const bool value = !std::is_pointer<T>::value;

    using traits = function;
    using base = function<T>;
    using type = T&;
    using remove_varargs = typename base::remove_varargs&;
    using add_varargs = typename base::add_varargs&;

    using remove_member_reference = reference_error;
    using add_member_lvalue_reference = reference_error;
    using add_member_rvalue_reference = reference_error;
    using add_member_const = reference_error;
    using add_member_volatile = reference_error;
    using add_member_cv = reference_error;
    using remove_member_const = reference_error;
    using remove_member_volatile = reference_error;
    using remove_member_cv = reference_error;

    template<typename NewReturn>
    using apply_return = typename base::template apply_return<NewReturn>&;
    
    using clear_args = typename base::clear_args&;
    
    template<typename... NewArgs>
    using push_front = typename base::template push_front<NewArgs...>&;

    template<typename... NewArgs>
    using push_back = typename base::template push_back<NewArgs...>&;

    template<std::size_t Count>
    using pop_back = typename base::template pop_back<Count>&;

    template<std::size_t Count>
    using pop_front = typename base::template pop_front<Count>&;

    template<std::size_t Index, typename... NewArgs>
    using insert_args = typename base::template insert_args<Index, NewArgs...>&;

    template<std::size_t Index, std::size_t Count>
    using remove_args = typename base::template remove_args<Index, Count>&;

    template<std::size_t Index, typename... NewArgs>
    using replace_args = typename base::template replace_args<Index, NewArgs...>&;
};

}}} // namespace boost::callable_traits::detail

#endif // #ifndef BOOST_CLBL_TRTS_DETAIL_FUNCTION_HPP
