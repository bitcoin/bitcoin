#ifndef BOOST_DESCRIBE_DATA_MEMBERS_HPP_INCLUDED
#define BOOST_DESCRIBE_DATA_MEMBERS_HPP_INCLUDED

// Copyright 2020 Peter Dimov
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include <boost/describe/modifiers.hpp>
#include <boost/describe/bases.hpp>

#if defined(BOOST_DESCRIBE_CXX11)

#include <boost/mp11/algorithm.hpp>
#include <boost/mp11/utility.hpp>
#include <boost/mp11/integral.hpp>
#include <boost/mp11/list.hpp>
#include <boost/mp11/bind.hpp>

namespace boost
{
namespace describe
{
namespace detail
{

// _describe_members<T>

template<class T> using _describe_public_members = decltype( boost_public_member_descriptor_fn( static_cast<T*>(0) ) );
template<class T> using _describe_protected_members = decltype( boost_protected_member_descriptor_fn( static_cast<T*>(0) ) );
template<class T> using _describe_private_members = decltype( boost_private_member_descriptor_fn( static_cast<T*>(0) ) );

template<class T> using _describe_members = mp11::mp_append<_describe_public_members<T>, _describe_protected_members<T>, _describe_private_members<T>>;

// describe_inherited_members<T>

// T: type
// V: list of virtual bases visited so far
template<class T, class V> struct describe_inherited_members_impl;
template<class T, class V> using describe_inherited_members = typename describe_inherited_members_impl<T, V>::type;

// L: list of base class descriptors
// T: derived type
// V: list of virtual bases visited so far
template<class L, class T, class V> struct describe_inherited_members2_impl;
template<class L, class T, class V> using describe_inherited_members2 = typename describe_inherited_members2_impl<L, T, V>::type;

template<class T, class V> struct describe_inherited_members_impl
{
    using R1 = describe_inherited_members2<describe_bases<T, mod_any_access>, T, V>;
    using R2 = _describe_members<T>;

    using type = mp11::mp_append<R1, R2>;
};

template<template<class...> class L, class T, class V> struct describe_inherited_members2_impl<L<>, T, V>
{
    using type = L<>;
};

constexpr bool cx_streq( char const * s1, char const * s2 )
{
    return s1[0] == s2[0] && ( s1[0] == 0 || cx_streq( s1 + 1, s2 + 1 ) );
}

template<class D1, class D2> using name_matches = mp11::mp_bool< cx_streq( D1::name, D2::name ) >;

template<class D, class L> using name_is_hidden = mp11::mp_any_of_q<L, mp11::mp_bind_front<name_matches, D>>;

constexpr unsigned cx_max( unsigned m1, unsigned m2 )
{
    return m1 > m2? m1: m2;
}

template<class T, unsigned Bm> struct update_modifiers
{
    template<class D> struct fn
    {
        using L = _describe_members<T>;
        static constexpr unsigned hidden = name_is_hidden<D, L>::value? mod_hidden: 0;

        static constexpr unsigned mods = D::modifiers;
        static constexpr unsigned access = cx_max( mods & mod_any_access, Bm & mod_any_access );

        static constexpr decltype(D::pointer) pointer = D::pointer;
        static constexpr decltype(D::name) name = D::name;
        static constexpr unsigned modifiers = ( mods & ~mod_any_access ) | access | mod_inherited | hidden;
    };
};

template<class T, unsigned Bm> template<class D> constexpr decltype(D::pointer) update_modifiers<T, Bm>::fn<D>::pointer;
template<class T, unsigned Bm> template<class D> constexpr decltype(D::name) update_modifiers<T, Bm>::fn<D>::name;
template<class T, unsigned Bm> template<class D> constexpr unsigned update_modifiers<T, Bm>::fn<D>::modifiers;

template<class D> struct gather_virtual_bases_impl;
template<class D> using gather_virtual_bases = typename gather_virtual_bases_impl<D>::type;

template<class D> struct gather_virtual_bases_impl
{
    using B = typename D::type;
    static constexpr unsigned M = D::modifiers;

    using R1 = mp11::mp_transform<gather_virtual_bases, describe_bases<B, mod_any_access>>;
    using R2 = mp11::mp_apply<mp11::mp_append, R1>;

    using type = mp11::mp_if_c<(M & mod_virtual) != 0, mp11::mp_push_front<R2, B>, R2>;
};

template<template<class...> class L, class D1, class... D, class T, class V> struct describe_inherited_members2_impl<L<D1, D...>, T, V>
{
    using B = typename D1::type;
    static constexpr unsigned M = D1::modifiers;

    using R1 = mp11::mp_if_c<(M & mod_virtual) && mp11::mp_contains<V, B>::value, L<>, describe_inherited_members<B, V>>;

    using R2 = mp11::mp_transform_q<update_modifiers<T, M>, R1>;

    using V2 = mp11::mp_append<V, gather_virtual_bases<D1>>;
    using R3 = describe_inherited_members2<L<D...>, T, V2>;

    using type = mp11::mp_append<R2, R3>;
};

// describe_members<T, M>

template<class T, unsigned M> using describe_members = mp11::mp_eval_if_c<(M & mod_inherited) == 0, _describe_members<T>, describe_inherited_members, T, mp11::mp_list<>>;

// member_filter

template<unsigned M> struct member_filter
{
    template<class T> using fn = mp11::mp_bool<
        (M & mod_any_access & T::modifiers) != 0 &&
        ( (M & mod_any_member) != 0 || (M & mod_static) == (T::modifiers & mod_static) ) &&
        ( (M & mod_any_member) != 0 || (M & mod_function) == (T::modifiers & mod_function) ) &&
        (M & mod_hidden) >= (T::modifiers & mod_hidden)
    >;
};

} // namespace detail

template<class T, unsigned M> using describe_members = mp11::mp_copy_if_q<detail::describe_members<T, M>, detail::member_filter<M>>;

} // namespace describe
} // namespace boost

#endif // !defined(BOOST_DESCRIBE_CXX11)

#endif // #ifndef BOOST_DESCRIBE_DATA_MEMBERS_HPP_INCLUDED
