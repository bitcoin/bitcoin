#ifndef BOOST_MP11_DETAIL_MP_IS_LIST_HPP_INCLUDED
#define BOOST_MP11_DETAIL_MP_IS_LIST_HPP_INCLUDED

// Copyright 2015-2019 Peter Dimov.
//
// Distributed under the Boost Software License, Version 1.0.
//
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt

#include <boost/mp11/integral.hpp>

namespace boost
{
namespace mp11
{

// mp_is_list<L>
namespace detail
{

template<class L> struct mp_is_list_impl
{
    using type = mp_false;
};

template<template<class...> class L, class... T> struct mp_is_list_impl<L<T...>>
{
    using type = mp_true;
};

} // namespace detail

template<class L> using mp_is_list = typename detail::mp_is_list_impl<L>::type;

} // namespace mp11
} // namespace boost

#endif // #ifndef BOOST_MP11_DETAIL_MP_IS_LIST_HPP_INCLUDED
