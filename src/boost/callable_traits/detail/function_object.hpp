/*

@Copyright Barrett Adair 2015-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)

*/

#ifndef BOOST_CLBL_TRTS_DETAIL_FUNCTION_OBJECT_HPP
#define BOOST_CLBL_TRTS_DETAIL_FUNCTION_OBJECT_HPP

#include <boost/callable_traits/detail/pmf.hpp>
#include <boost/callable_traits/detail/default_callable_traits.hpp>
#include <boost/callable_traits/detail/forward_declarations.hpp>
#include <boost/callable_traits/detail/utility.hpp>

namespace boost { namespace callable_traits { namespace detail {

template<typename T, typename Base>
struct function_object : Base {

    using type = T;
    using error_t = error_type<T>;
    using function_type = typename Base::function_object_signature;
    using arg_types = typename Base::non_invoke_arg_types;
    using non_invoke_arg_types = arg_types;

    static constexpr const bool value = std::is_class<
        typename std::remove_reference<T>::type>::value;

    using traits = function_object;
    using class_type = error_t;
    using invoke_type = error_t;
    using remove_varargs = error_t;
    using add_varargs = error_t;
    using is_noexcept = typename Base::is_noexcept;
    using add_noexcept = error_t;
    using remove_noexcept = error_t;
    using is_transaction_safe = typename Base::is_transaction_safe;
    using add_transaction_safe = error_t;
    using remove_transaction_safe = error_t;
    using clear_args = error_t;

    template<template<class...> class Container>
    using expand_args = typename function<function_type>::template
        expand_args<Container>;

    template<template<class...> class Container, typename... RightArgs>
    using expand_args_left = typename function<function_type>::template
        expand_args_left<Container, RightArgs...>;

    template<template<class...> class Container, typename... LeftArgs>
    using expand_args_right = typename function<function_type>::template
        expand_args_right<Container, LeftArgs...>;

    template<typename C, typename U = T>
    using apply_member_pointer =
        typename std::remove_reference<U>::type C::*;

    template<typename>
    using apply_return = error_t;

    template<typename...>
    using push_front = error_t;
    
    template<typename...>
    using push_back = error_t;
    
    template<std::size_t ElementCount>
    using pop_args_front = error_t;

    template<std::size_t ElementCount>
    using pop_args_back = error_t;
    
    template<std::size_t Index, typename... NewArgs>
    using insert_args = error_t;
    
    template<std::size_t Index, std::size_t Count>
    using remove_args = error_t;

    template<std::size_t Index, typename... NewArgs>
    using replace_args = error_t;

    template<std::size_t Count>
    using pop_front = error_t;

    template<std::size_t Count>
    using pop_back = error_t;

    using remove_member_reference = error_t;
    using add_member_lvalue_reference = error_t;
    using add_member_rvalue_reference = error_t;
    using add_member_const = error_t;
    using add_member_volatile = error_t;
    using add_member_cv = error_t;
    using remove_member_const = error_t;
    using remove_member_volatile = error_t;
    using remove_member_cv = error_t;
};

template<typename T, typename U, typename Base>
struct function_object <T U::*, Base>
    : default_callable_traits<> {};

}}} // namespace boost::callable_traits::detail

#endif // #ifndef BOOST_CLBL_TRTS_DETAIL_FUNCTION_OBJECT_HPP
