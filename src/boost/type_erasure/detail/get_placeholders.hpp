// Boost.TypeErasure library
//
// Copyright 2011 Steven Watanabe
//
// Distributed under the Boost Software License Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// $Id$

#if !defined(BOOST_PP_IS_ITERATING)

#ifndef BOOST_TYPE_ERASURE_DETAIL_GET_PLACEHOLDERS_HPP_INCLUDED
#define BOOST_TYPE_ERASURE_DETAIL_GET_PLACEHOLDERS_HPP_INCLUDED

#include <boost/mpl/eval_if.hpp>
#include <boost/mpl/identity.hpp>
#include <boost/mpl/insert.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/iteration/iterate.hpp>
#include <boost/preprocessor/repetition/enum.hpp>
#include <boost/preprocessor/repetition/enum_params.hpp>
#include <boost/preprocessor/repetition/enum_trailing_params.hpp>
#include <boost/type_erasure/detail/meta.hpp>
#include <boost/type_erasure/config.hpp>
#include <boost/type_erasure/is_placeholder.hpp>

namespace boost {
namespace type_erasure {
namespace detail {

#ifdef BOOST_TYPE_ERASURE_USE_MP11

template<class T>
struct get_placeholders_in_argument_impl
{
    template<class Out>
    using apply = ::boost::type_erasure::detail::eval_if<
        ::boost::type_erasure::is_placeholder<T>::value,
        ::boost::mp11::mp_set_push_back,
        ::boost::type_erasure::detail::first,
        Out, T
    >;
};

template<class T>
struct get_placeholders_in_argument_impl<T&>
{
    template<class Out>
    using apply = typename ::boost::type_erasure::detail::get_placeholders_in_argument_impl<T>::template apply<Out>;
};

template<class T>
struct get_placeholders_in_argument_impl<T&&>
{
    template<class Out>
    using apply = typename ::boost::type_erasure::detail::get_placeholders_in_argument_impl<T>::template apply<Out>;
};

template<class T>
struct get_placeholders_in_argument_impl<const T>
{
    template<class Out>
    using apply = typename ::boost::type_erasure::detail::get_placeholders_in_argument_impl<T>::template apply<Out>;
};

template<class Out, class T>
using get_placeholders_f = typename get_placeholders_in_argument_impl<T>::template apply<Out>;

template<class Out, class... T>
using get_placeholders_impl =
    ::boost::mp11::mp_fold<
        ::boost::mp11::mp_list<T...>,
        Out,
        ::boost::type_erasure::detail::get_placeholders_f
    >;

template<class R, class... T>
struct get_placeholders_in_argument_impl<R(T...)>
{
    template<class Out>
    using apply = ::boost::type_erasure::detail::get_placeholders_impl<Out, R, T...>;
};

template<class R, class C, class... T>
struct get_placeholders_in_argument_impl<R (C::*)(T...)>
{
    template<class Out>
    using apply = ::boost::type_erasure::detail::get_placeholders_impl<Out, R, C, T...>;
};

template<class R, class C, class... T>
struct get_placeholders_in_argument_impl<R (C::*)(T...) const>
{
    template<class Out>
    using apply = ::boost::type_erasure::detail::get_placeholders_impl<Out, R, C, T...>;
};

template<class T, class Out>
struct get_placeholders;

template<template<class...> class F, class... T, class Out>
struct get_placeholders<F<T...>, Out>
{
    using type = ::boost::type_erasure::detail::get_placeholders_impl<Out, T...>;
};

template<class T, class Out>
using get_placeholders_t = typename ::boost::type_erasure::detail::get_placeholders<T, Out>::type;

#else

template<class T, class Out>
struct get_placeholders_in_argument
{
    typedef typename ::boost::mpl::eval_if<
        ::boost::type_erasure::is_placeholder<T>,
        ::boost::mpl::insert<Out, T>,
        ::boost::mpl::identity<Out>
    >::type type;
};

template<class T, class Out>
struct get_placeholders;

template<class T, class Out>
struct get_placeholders_in_argument<T&, Out>
{
    typedef typename ::boost::type_erasure::detail::get_placeholders_in_argument<
        T,
        Out
    >::type type;
};

template<class T, class Out>
struct get_placeholders_in_argument<const T, Out>
{
    typedef typename ::boost::type_erasure::detail::get_placeholders_in_argument<
        T,
        Out
    >::type type;
};

#if !defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)

template<class Out, class... T>
struct get_placeholders_impl;

template<class Out, class T0, class... T>
struct get_placeholders_impl<Out, T0, T...>
{
    typedef typename ::boost::type_erasure::detail::get_placeholders_in_argument<
        T0,
        typename get_placeholders_impl<Out, T...>::type
    >::type type;
};

template<class Out>
struct get_placeholders_impl<Out>
{
    typedef Out type;
};

template<template<class...> class T, class... U, class Out>
struct get_placeholders<T<U...>, Out>
{
    typedef typename get_placeholders_impl<Out, U...>::type type;
};

template<class R, class... T, class Out>
struct get_placeholders_in_argument<R(T...), Out>
{
    typedef typename get_placeholders_impl<Out, R, T...>::type type;
};

template<class R, class C, class... T, class Out>
struct get_placeholders_in_argument<R (C::*)(T...), Out>
{
    typedef typename get_placeholders_impl<Out, R, C, T...>::type type;
};

template<class R, class C, class... T, class Out>
struct get_placeholders_in_argument<R (C::*)(T...) const, Out>
{
    typedef typename get_placeholders_impl<Out, R, C, T...>::type type;
};

#else

#define BOOST_PP_FILENAME_1 <boost/type_erasure/detail/get_placeholders.hpp>
#define BOOST_PP_ITERATION_LIMITS (0, BOOST_TYPE_ERASURE_MAX_ARITY)
#include BOOST_PP_ITERATE()

#endif

#endif

}
}
}

#endif

#else

#define N BOOST_PP_ITERATION()
#define BOOST_TYPE_ERASURE_GET_PLACEHOLDER(z, n, data)                              \
    typedef typename ::boost::type_erasure::detail::get_placeholders_in_argument<   \
        BOOST_PP_CAT(data, n), BOOST_PP_CAT(type, n)>::type                         \
        BOOST_PP_CAT(type, BOOST_PP_INC(n));

#if N != 0

template<template<BOOST_PP_ENUM_PARAMS(N, class T)> class T,
    BOOST_PP_ENUM_PARAMS(N, class T), class Out>
struct get_placeholders<T<BOOST_PP_ENUM_PARAMS(N, T)>, Out>
{
    typedef Out type0;
    BOOST_PP_REPEAT(N, BOOST_TYPE_ERASURE_GET_PLACEHOLDER, T)
    typedef BOOST_PP_CAT(type, N) type;
};

#endif

template<class R
    BOOST_PP_ENUM_TRAILING_PARAMS(N, class T), class Out>
struct get_placeholders_in_argument<R(BOOST_PP_ENUM_PARAMS(N, T)), Out>
{
    typedef typename ::boost::type_erasure::detail::get_placeholders_in_argument<
        R,
        Out
    >::type type0;
    BOOST_PP_REPEAT(N, BOOST_TYPE_ERASURE_GET_PLACEHOLDER, T)
    typedef BOOST_PP_CAT(type, N) type;
};

#undef BOOST_TYPE_ERASURE_GET_PLACEHOLDER
#undef N

#endif
