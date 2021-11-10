#ifndef BOOST_MP11_DETAIL_MP_COUNT_HPP_INCLUDED
#define BOOST_MP11_DETAIL_MP_COUNT_HPP_INCLUDED

//  Copyright 2015, 2016 Peter Dimov.
//
//  Distributed under the Boost Software License, Version 1.0.
//
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include <boost/mp11/integral.hpp>
#include <boost/mp11/detail/mp_plus.hpp>
#include <boost/mp11/detail/config.hpp>

namespace boost
{
namespace mp11
{

// mp_count<L, V>
namespace detail
{

#if !defined( BOOST_MP11_NO_CONSTEXPR )

constexpr std::size_t cx_plus()
{
    return 0;
}

template<class T1, class... T> constexpr std::size_t cx_plus(T1 t1, T... t)
{
    return static_cast<std::size_t>(t1) + cx_plus(t...);
}

template<class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class... T>
constexpr std::size_t cx_plus(T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6, T7 t7, T8 t8, T9 t9, T10 t10, T... t)
{
    return static_cast<std::size_t>(t1 + t2 + t3 + t4 + t5 + t6 + t7 + t8 + t9 + t10) + cx_plus(t...);
}

#endif

template<class L, class V> struct mp_count_impl;

#if defined( BOOST_MP11_HAS_CXX14_CONSTEXPR )

template<class V, class... T> constexpr std::size_t cx_count()
{
    constexpr bool a[] = { false, std::is_same<T, V>::value... };

    std::size_t r = 0;

    for( std::size_t i = 1; i < sizeof...(T) + 1; ++i )
    {
        r += a[ i ];
    }

    return r;
}

template<template<class...> class L, class... T, class V> struct mp_count_impl<L<T...>, V>
{
    using type = mp_size_t<cx_count<V, T...>()>;
};

#elif !defined( BOOST_MP11_NO_CONSTEXPR )

template<template<class...> class L, class... T, class V> struct mp_count_impl<L<T...>, V>
{
    using type = mp_size_t<cx_plus(std::is_same<T, V>::value...)>;
};

#else

template<template<class...> class L, class... T, class V> struct mp_count_impl<L<T...>, V>
{
    using type = mp_size_t<mp_plus<std::is_same<T, V>...>::value>;
};

#endif

} // namespace detail

template<class L, class V> using mp_count = typename detail::mp_count_impl<L, V>::type;

// mp_count_if<L, P>
namespace detail
{

template<class L, template<class...> class P> struct mp_count_if_impl;

#if defined( BOOST_MP11_HAS_CXX14_CONSTEXPR ) && !BOOST_MP11_WORKAROUND( BOOST_MP11_MSVC, < 1930 )

template<template<class...> class P, class... T> constexpr std::size_t cx_count_if()
{
    constexpr bool a[] = { false, static_cast<bool>( P<T>::value )... };

    std::size_t r = 0;

    for( std::size_t i = 1; i < sizeof...(T) + 1; ++i )
    {
        r += a[ i ];
    }

    return r;
}

template<template<class...> class L, class... T, template<class...> class P> struct mp_count_if_impl<L<T...>, P>
{
    using type = mp_size_t<cx_count_if<P, T...>()>;
};

#elif !defined( BOOST_MP11_NO_CONSTEXPR )

template<template<class...> class L, class... T, template<class...> class P> struct mp_count_if_impl<L<T...>, P>
{
    using type = mp_size_t<cx_plus(mp_to_bool<P<T>>::value...)>;
};

#else

template<template<class...> class L, class... T, template<class...> class P> struct mp_count_if_impl<L<T...>, P>
{
#if BOOST_MP11_WORKAROUND( BOOST_MP11_MSVC, < 1920 )

    template<class T> struct _f { using type = mp_to_bool<P<T>>; };
    using type = mp_size_t<mp_plus<typename _f<T>::type...>::value>;

#else

    using type = mp_size_t<mp_plus<mp_to_bool<P<T>>...>::value>;

#endif
};

#endif

} // namespace detail

template<class L, template<class...> class P> using mp_count_if = typename detail::mp_count_if_impl<L, P>::type;
template<class L, class Q> using mp_count_if_q = mp_count_if<L, Q::template fn>;

} // namespace mp11
} // namespace boost

#endif // #ifndef BOOST_MP11_DETAIL_MP_COUNT_HPP_INCLUDED
