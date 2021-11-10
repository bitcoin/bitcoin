// Boost.TypeErasure library
//
// Copyright 2018 Steven Watanabe
//
// Distributed under the Boost Software License Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// $Id$

#ifndef BOOST_TYPE_ERASURE_DETAIL_META_HPP_INCLUDED
#define BOOST_TYPE_ERASURE_DETAIL_META_HPP_INCLUDED

#include <boost/config.hpp>


#if !defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES) && \
    !defined(BOOST_NO_CXX11_TEMPLATE_ALIASES) && \
    /* MSVC 14.0 breaks down in the template alias quagmire. */ \
    !BOOST_WORKAROUND(BOOST_MSVC, <= 1900)

#define BOOST_TYPE_ERASURE_USE_MP11

#include <boost/mp11/list.hpp>
#include <boost/mp11/map.hpp>
#include <boost/mp11/set.hpp>
#include <boost/mp11/algorithm.hpp>
#include <boost/mp11/function.hpp>
#include <boost/mp11/mpl.hpp>

namespace boost {
namespace type_erasure {
namespace detail {

struct mp11_list_inserter
{
    template<class L, class T>
    using apply = ::boost::mpl::identity< ::boost::mp11::mp_push_back<L, T> >;
};

template<class T>
struct make_mp_list_impl
{
    typedef typename ::boost::mpl::fold<
        T,
        ::boost::mp11::mp_list<>,
        ::boost::type_erasure::detail::mp11_list_inserter
    >::type type;
};

template<class... T>
struct make_mp_list_impl< ::boost::mp11::mp_list<T...> >
{
    typedef ::boost::mp11::mp_list<T...> type;
};

template<class T>
using make_mp_list = typename make_mp_list_impl<T>::type;

template<bool>
struct eval_if_impl;

template<>
struct eval_if_impl<true>
{
    template<template<class...> class T, template<class...> class F, class... A>
    using apply = T<A...>;
};

template<>
struct eval_if_impl<false>
{
    template<template<class...> class T, template<class...> class F, class... A>
    using apply = F<A...>;
};

template<bool B, template<class...> class T, template<class...> class F, class... A>
using eval_if = typename ::boost::type_erasure::detail::eval_if_impl<B>::template apply<T, F, A...>;

template<class T0, class...>
using first = T0;

}
}
}

#endif

#endif
