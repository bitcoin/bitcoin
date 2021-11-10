/*
Copyright Barrett Adair 2016-2017

Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http ://boost.org/LICENSE_1_0.txt)

*/

#ifndef BOOST_CLBL_TRTS_DETAIL_DEFAULT_BOOST_CLBL_TRTS_HPP
#define BOOST_CLBL_TRTS_DETAIL_DEFAULT_BOOST_CLBL_TRTS_HPP

namespace boost { namespace callable_traits { namespace detail {
          
template<typename T = void>
struct default_callable_traits {

    // value is used by all traits classes to participate 
    // in the <callable_traits/detail/traits.hpp> disjunction.
    static constexpr bool value = false;
    
    // used facilitate the disjunction in
    // <callable_traits/detail/traits.hpp>
    using traits = default_callable_traits;
    
    using error_t = error_type<T>;

    // represents the type under consideration
    using type = error_t;
    
    // std::true_type for callables with C-style variadics
    using has_varargs = std::false_type;
    
    using return_type = error_t;
    
    // arg_types is a std::tuple of argument types for
    // callables that are not overloaded/templated function objects.
    // arg_types IS defined in terms of INVOKE, which means
    // a PMF's arg_types tuple will use a reference to its
    // parent class as the first argument, with qualifiers added to
    // match the PMF's own qualifiers.
    using arg_types = error_t;
    
    // arg_types without the decltype(*this) parameter for member functions
    using non_invoke_arg_types = error_t;

    // An "approximation" of a callable type, in the form
    // of a plain function type. Defined in terms of INVOKE.
    // An identity alias for qualified/unqualified plain function
    // types.
    using function_type = error_t;
    
    // Used to smoothen the edges between PMFs and function objects
    using function_object_signature = error_t;

    // An identity alias for qualified/unqualified plain function
    // types. Equivalent to remove_member_pointer for PMFs. Same
    // as function_type for other callable types.
    using qualified_function_type = error_t;
    
    // Removes C-style variadics from a signature, if present.
    // Aliases error_t for function objects and PMDs.
    using remove_varargs = error_t;
    
    // Adds C-style variadics to a signature. Aliases
    // error_t for function objects and PMDs.
    using add_varargs = error_t;
    
    // std::true_type when the signature includes noexcept, when
    // the feature is available
    using is_noexcept = std::false_type;

    // adds noexcept to a signature if the feature is available
    using add_noexcept = error_t;

    // removes noexcept from a signature if present
    using remove_noexcept = error_t;

    // std::true_type when the signature includes transaction_safe, when
    // the feature is available
    using is_transaction_safe = std::false_type;

    // adds transaction_safe to a signature if the feature is available
    using add_transaction_safe = error_t;

    // removes transaction_safe from a signature if present
    using remove_transaction_safe = error_t;

    // The class of a PMD or PMF. error_t for other types
    using class_type = error_t;
    
    // The qualified reference type of class_type. error_t
    // for non-member-pointers.
    using invoke_type = error_t;
    
    // Removes reference qualifiers from a signature.
    using remove_reference = error_t;
    
    // Adds an lvalue qualifier to a signature, in arbitrary
    // accordance with C++11 reference collapsing rules.
    using add_member_lvalue_reference = error_t;
    
    // Adds an rvalue qualifier to a signature, in arbitrary
    // accordance with C++11 reference collapsing rules.
    using add_member_rvalue_reference = error_t;
    
    // Adds a const qualifier to a signature.
    using add_member_const = error_t;
    
    // Adds a volatile qualifier to a signature.
    using add_member_volatile = error_t;
    
    // Adds both const and volatile qualifiers to a signature.
    using add_member_cv = error_t;
    
    // Removes a const qualifier from a signature, if present.
    using remove_member_const = error_t;
    
    // Removes a volatile qualifier from a signature, if present.
    using remove_member_volatile = error_t;
    
    // Removes both const and volatile qualifiers from a
    // signature, if any.
    using remove_member_cv = error_t;
    
    // Removes the member pointer from PMDs and PMFs. An identity
    // alias for other callable types.
    using remove_member_pointer = error_t;
    
    // Changes the parent class type for PMDs and PMFs. Turns
    // function pointers, function references, and
    // qualified/unqualified function types into PMFs. Turns
    // everything else into member data pointers.
    template<typename C,
        typename U = T,
        typename K = typename std::remove_reference<U>::type,
        typename L = typename std::conditional<
            std::is_same<void, K>::value, error_t, K>::type,
        typename Class = typename std::conditional<
            std::is_class<C>::value, C, error_t>::type>
    using apply_member_pointer = typename std::conditional<
        std::is_same<L, error_t>::value || std::is_same<Class, error_t>::value,
        error_t, L Class::*>::type;
    
    // Changes the return type of PMFs, function pointers, function
    // references, and qualified/unqualified function types. Changes
    // the data type of PMDs. error_t for function objects.
    template<typename>
    using apply_return = error_t;

    // Expands the argument types into a template
    template<template<class...> class Container>
    using expand_args = error_t;

    template<template<class...> class Container, typename... RightArgs>
    using expand_args_left = error_t;

    template<template<class...> class Container, typename... LeftArgs>
    using expand_args_right = error_t;

    using clear_args = error_t;
    
    template<typename... NewArgs>
    using push_front = error_t;

    template<typename... NewArgs>
    using push_back = error_t;
    
    template<std::size_t ElementCount>
    using pop_front = error_t;

    template<std::size_t ElementCount>
    using pop_back = error_t;
    
    template<std::size_t Index, typename... NewArgs>
    using insert_args = error_t;

    template<std::size_t Index, std::size_t Count>
    using remove_args = error_t;

    template<std::size_t Index, typename... NewArgs>
    using replace_args = error_t;

    static constexpr qualifier_flags cv_flags = cv_of<T>::value;
    static constexpr qualifier_flags ref_flags = ref_of<T>::value;
    static constexpr qualifier_flags q_flags = cv_flags | ref_flags;

    using has_member_qualifiers = std::integral_constant<bool, q_flags != default_>;
    using is_const_member = std::integral_constant<bool, 0 < (cv_flags & const_)>;
    using is_volatile_member = std::integral_constant<bool, 0 < (cv_flags & volatile_)>;
    using is_cv_member = std::integral_constant<bool, cv_flags == (const_ | volatile_)>;

#ifdef BOOST_CLBL_TRTS_DISABLE_REFERENCE_QUALIFIERS
    using is_reference_member = std::false_type;
    using is_lvalue_reference_member = std::false_type;
    using is_rvalue_reference_member = std::false_type;
#else
    using is_reference_member = std::integral_constant<bool, 0 < ref_flags>;
    using is_lvalue_reference_member = std::integral_constant<bool, ref_flags == lref_>;
    using is_rvalue_reference_member = std::integral_constant<bool, ref_flags == rref_>;
#endif //#ifdef BOOST_CLBL_TRTS_DISABLE_REFERENCE_QUALIFIERS

};

}}} // namespace boost::callable_traits::detail

#endif // BOOST_CLBL_TRTS_DETAIL_DEFAULT_BOOST_CLBL_TRTS_HPP

