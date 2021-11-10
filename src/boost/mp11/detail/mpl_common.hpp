#ifndef BOOST_MP11_DETAIL_MPL_COMMON_HPP_INCLUDED
#define BOOST_MP11_DETAIL_MPL_COMMON_HPP_INCLUDED

// Copyright 2017, 2019 Peter Dimov.
//
// Distributed under the Boost Software License, Version 1.0.
//
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt

#include <boost/mp11/list.hpp>
#include <boost/mp11/algorithm.hpp>

namespace boost
{
namespace mpl
{

struct forward_iterator_tag;

namespace aux
{

struct mp11_tag {};

template<class L> struct mp11_iterator
{
    using category = forward_iterator_tag;

    using type = mp11::mp_first<L>;
    using next = mp11_iterator<mp11::mp_rest<L>>;
};

} // namespace aux

// at

template< typename Tag > struct at_impl;

template<> struct at_impl<aux::mp11_tag>
{
    template<class L, class I> struct apply
    {
        using type = mp11::mp_at<L, I>;
    };
};

// back

template< typename Tag > struct back_impl;

template<> struct back_impl<aux::mp11_tag>
{
    template<class L> struct apply
    {
        using N = mp11::mp_size<L>;
        using type = mp11::mp_at_c<L, N::value - 1>;
    };
};

// begin

template< typename Tag > struct begin_impl;

template<> struct begin_impl<aux::mp11_tag>
{
    template<class L> struct apply
    {
        using type = aux::mp11_iterator<L>;
    };
};

// clear

template< typename Tag > struct clear_impl;

template<> struct clear_impl<aux::mp11_tag>
{
    template<class L> struct apply
    {
        using type = mp11::mp_clear<L>;
    };
};

// end

template< typename Tag > struct end_impl;

template<> struct end_impl<aux::mp11_tag>
{
    template<class L> struct apply
    {
        using type = aux::mp11_iterator<mp11::mp_clear<L>>;
    };
};

// front

template< typename Tag > struct front_impl;

template<> struct front_impl<aux::mp11_tag>
{
    template<class L> struct apply
    {
        using type = mp11::mp_front<L>;
    };
};

// pop_front

template< typename Tag > struct pop_front_impl;

template<> struct pop_front_impl<aux::mp11_tag>
{
    template<class L> struct apply
    {
        using type = mp11::mp_pop_front<L>;
    };
};

// push_back

template< typename Tag > struct push_back_impl;

template<> struct push_back_impl<aux::mp11_tag>
{
    template<class L, class T> struct apply
    {
        using type = mp11::mp_push_back<L, T>;
    };
};

// push_front

template< typename Tag > struct push_front_impl;

template<> struct push_front_impl<aux::mp11_tag>
{
    template<class L, class T> struct apply
    {
        using type = mp11::mp_push_front<L, T>;
    };
};

// size

template< typename Tag > struct size_impl;

template<> struct size_impl<aux::mp11_tag>
{
    template<class L> struct apply
    {
        using type = mp11::mp_size<L>;
    };
};

} // namespace mpl
} // namespace boost

#endif // #ifndef BOOST_MP11_DETAIL_MPL_COMMON_HPP_INCLUDED
