#ifndef BOOST_MP11_UTILITY_HPP_INCLUDED
#define BOOST_MP11_UTILITY_HPP_INCLUDED

// Copyright 2015-2020 Peter Dimov.
//
// Distributed under the Boost Software License, Version 1.0.
//
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt

#include <boost/mp11/integral.hpp>
#include <boost/mp11/detail/mp_list.hpp>
#include <boost/mp11/detail/mp_fold.hpp>
#include <boost/mp11/detail/config.hpp>

namespace boost
{
namespace mp11
{

// mp_identity
template<class T> struct mp_identity
{
    using type = T;
};

// mp_identity_t
template<class T> using mp_identity_t = typename mp_identity<T>::type;

// mp_inherit
template<class... T> struct mp_inherit: T... {};

// mp_if, mp_if_c
namespace detail
{

template<bool C, class T, class... E> struct mp_if_c_impl
{
};

template<class T, class... E> struct mp_if_c_impl<true, T, E...>
{
    using type = T;
};

template<class T, class E> struct mp_if_c_impl<false, T, E>
{
    using type = E;
};

} // namespace detail

template<bool C, class T, class... E> using mp_if_c = typename detail::mp_if_c_impl<C, T, E...>::type;
template<class C, class T, class... E> using mp_if = typename detail::mp_if_c_impl<static_cast<bool>(C::value), T, E...>::type;

// mp_valid

#if BOOST_MP11_WORKAROUND( BOOST_MP11_INTEL, != 0 ) // tested at 1800

// contributed by Roland Schulz in https://github.com/boostorg/mp11/issues/17

namespace detail
{

template<class...> using void_t = void;

template<class, template<class...> class F, class... T>
struct mp_valid_impl: mp_false {};

template<template<class...> class F, class... T>
struct mp_valid_impl<void_t<F<T...>>, F, T...>: mp_true {};

} // namespace detail

template<template<class...> class F, class... T> using mp_valid = typename detail::mp_valid_impl<void, F, T...>;

#else

// implementation by Bruno Dutra (by the name is_evaluable)
namespace detail
{

template<template<class...> class F, class... T> struct mp_valid_impl
{
    template<template<class...> class G, class = G<T...>> static mp_true check(int);
    template<template<class...> class> static mp_false check(...);

    using type = decltype(check<F>(0));
};

} // namespace detail

template<template<class...> class F, class... T> using mp_valid = typename detail::mp_valid_impl<F, T...>::type;

#endif

template<class Q, class... T> using mp_valid_q = mp_valid<Q::template fn, T...>;

// mp_defer
namespace detail
{

template<template<class...> class F, class... T> struct mp_defer_impl
{
    using type = F<T...>;
};

struct mp_no_type
{
};

#if BOOST_MP11_WORKAROUND( BOOST_MP11_CUDA, >= 9000000 && BOOST_MP11_CUDA < 10000000 )

template<template<class...> class F, class... T> struct mp_defer_cuda_workaround
{
    using type = mp_if<mp_valid<F, T...>, detail::mp_defer_impl<F, T...>, detail::mp_no_type>;
};

#endif

} // namespace detail

#if BOOST_MP11_WORKAROUND( BOOST_MP11_CUDA, >= 9000000 && BOOST_MP11_CUDA < 10000000 )

template<template<class...> class F, class... T> using mp_defer = typename detail::mp_defer_cuda_workaround< F, T...>::type;

#else

template<template<class...> class F, class... T> using mp_defer = mp_if<mp_valid<F, T...>, detail::mp_defer_impl<F, T...>, detail::mp_no_type>;

#endif

// mp_eval_if, mp_eval_if_c
namespace detail
{

template<bool C, class T, template<class...> class F, class... U> struct mp_eval_if_c_impl;

template<class T, template<class...> class F, class... U> struct mp_eval_if_c_impl<true, T, F, U...>
{
    using type = T;
};

template<class T, template<class...> class F, class... U> struct mp_eval_if_c_impl<false, T, F, U...>: mp_defer<F, U...>
{
};

} // namespace detail

template<bool C, class T, template<class...> class F, class... U> using mp_eval_if_c = typename detail::mp_eval_if_c_impl<C, T, F, U...>::type;
template<class C, class T, template<class...> class F, class... U> using mp_eval_if = typename detail::mp_eval_if_c_impl<static_cast<bool>(C::value), T, F, U...>::type;
template<class C, class T, class Q, class... U> using mp_eval_if_q = typename detail::mp_eval_if_c_impl<static_cast<bool>(C::value), T, Q::template fn, U...>::type;

// mp_eval_if_not
template<class C, class T, template<class...> class F, class... U> using mp_eval_if_not = mp_eval_if<mp_not<C>, T, F, U...>;
template<class C, class T, class Q, class... U> using mp_eval_if_not_q = mp_eval_if<mp_not<C>, T, Q::template fn, U...>;

// mp_eval_or
template<class T, template<class...> class F, class... U> using mp_eval_or = mp_eval_if_not<mp_valid<F, U...>, T, F, U...>;
template<class T, class Q, class... U> using mp_eval_or_q = mp_eval_or<T, Q::template fn, U...>;

// mp_cond

// so elegant; so doesn't work
// template<class C, class T, class... E> using mp_cond = mp_eval_if<C, T, mp_cond, E...>;

namespace detail
{

template<class C, class T, class... E> struct mp_cond_impl;

} // namespace detail

template<class C, class T, class... E> using mp_cond = typename detail::mp_cond_impl<C, T, E...>::type;

namespace detail
{

template<class C, class T, class... E> using mp_cond_ = mp_eval_if<C, T, mp_cond, E...>;

template<class C, class T, class... E> struct mp_cond_impl: mp_defer<mp_cond_, C, T, E...>
{
};

} // namespace detail

// mp_quote
template<template<class...> class F> struct mp_quote
{
    // the indirection through mp_defer works around the language inability
    // to expand T... into a fixed parameter list of an alias template

    template<class... T> using fn = typename mp_defer<F, T...>::type;
};

// mp_quote_trait
template<template<class...> class F> struct mp_quote_trait
{
    template<class... T> using fn = typename F<T...>::type;
};

// mp_invoke_q
#if BOOST_MP11_WORKAROUND( BOOST_MP11_MSVC, < 1900 )

namespace detail
{

template<class Q, class... T> struct mp_invoke_q_impl: mp_defer<Q::template fn, T...> {};

} // namespace detail

template<class Q, class... T> using mp_invoke_q = typename detail::mp_invoke_q_impl<Q, T...>::type;

#elif BOOST_MP11_WORKAROUND( BOOST_MP11_GCC, < 50000 )

template<class Q, class... T> using mp_invoke_q = typename mp_defer<Q::template fn, T...>::type;

#else

template<class Q, class... T> using mp_invoke_q = typename Q::template fn<T...>;

#endif

// mp_not_fn<P>
template<template<class...> class P> struct mp_not_fn
{
    template<class... T> using fn = mp_not< mp_invoke_q<mp_quote<P>, T...> >;
};

template<class Q> using mp_not_fn_q = mp_not_fn<Q::template fn>;

// mp_compose
namespace detail
{

template<class T, class Q> using mp_reverse_invoke_q = mp_invoke_q<Q, T>;

} // namespace detail

#if !BOOST_MP11_WORKAROUND( BOOST_MP11_MSVC, < 1900 )

template<template<class...> class... F> struct mp_compose
{
    template<class T> using fn = mp_fold<mp_list<mp_quote<F>...>, T, detail::mp_reverse_invoke_q>;
};

#endif

template<class... Q> struct mp_compose_q
{
    template<class T> using fn = mp_fold<mp_list<Q...>, T, detail::mp_reverse_invoke_q>;
};

} // namespace mp11
} // namespace boost

#endif // #ifndef BOOST_MP11_UTILITY_HPP_INCLUDED
