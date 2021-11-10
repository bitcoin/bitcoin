#ifndef BOOST_MP11_TUPLE_HPP_INCLUDED
#define BOOST_MP11_TUPLE_HPP_INCLUDED

//  Copyright 2015-2020 Peter Dimov.
//
//  Distributed under the Boost Software License, Version 1.0.
//
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include <boost/mp11/integer_sequence.hpp>
#include <boost/mp11/list.hpp>
#include <boost/mp11/function.hpp>
#include <boost/mp11/detail/config.hpp>
#include <tuple>
#include <utility>
#include <type_traits>
#include <cstddef>

#if BOOST_MP11_MSVC
# pragma warning( push )
# pragma warning( disable: 4100 ) // unreferenced formal parameter 'tp'
#endif

namespace boost
{
namespace mp11
{

// tuple_apply
namespace detail
{

template<class F, class Tp, std::size_t... J> BOOST_MP11_CONSTEXPR auto tuple_apply_impl( F && f, Tp && tp, integer_sequence<std::size_t, J...> )
    -> decltype( std::forward<F>(f)( std::get<J>(std::forward<Tp>(tp))... ) )
{
    return std::forward<F>(f)( std::get<J>(std::forward<Tp>(tp))... );
}

} // namespace detail

template<class F, class Tp,
    class Seq = make_index_sequence<std::tuple_size<typename std::remove_reference<Tp>::type>::value>>
BOOST_MP11_CONSTEXPR auto tuple_apply( F && f, Tp && tp )
    -> decltype( detail::tuple_apply_impl( std::forward<F>(f), std::forward<Tp>(tp), Seq() ) )
{
    return detail::tuple_apply_impl( std::forward<F>(f), std::forward<Tp>(tp), Seq() );
}

// construct_from_tuple
namespace detail
{

template<class T, class Tp, std::size_t... J> BOOST_MP11_CONSTEXPR T construct_from_tuple_impl( Tp && tp, integer_sequence<std::size_t, J...> )
{
    return T( std::get<J>(std::forward<Tp>(tp))... );
}

} // namespace detail

template<class T, class Tp,
    class Seq = make_index_sequence<std::tuple_size<typename std::remove_reference<Tp>::type>::value>>
BOOST_MP11_CONSTEXPR T construct_from_tuple( Tp && tp )
{
    return detail::construct_from_tuple_impl<T>( std::forward<Tp>(tp), Seq() );
}

// tuple_for_each
namespace detail
{

template<class Tp, std::size_t... J, class F> BOOST_MP11_CONSTEXPR F tuple_for_each_impl( Tp && tp, integer_sequence<std::size_t, J...>, F && f )
{
    using A = int[sizeof...(J)];
    return (void)A{ ((void)f(std::get<J>(std::forward<Tp>(tp))), 0)... }, std::forward<F>(f);
}

template<class Tp, class F> BOOST_MP11_CONSTEXPR F tuple_for_each_impl( Tp && /*tp*/, integer_sequence<std::size_t>, F && f )
{
    return std::forward<F>(f);
}

} // namespace detail

template<class Tp, class F> BOOST_MP11_CONSTEXPR F tuple_for_each( Tp && tp, F && f )
{
    using seq = make_index_sequence<std::tuple_size<typename std::remove_reference<Tp>::type>::value>;
    return detail::tuple_for_each_impl( std::forward<Tp>(tp), seq(), std::forward<F>(f) );
}

// tuple_transform

namespace detail
{

// std::forward_as_tuple is not constexpr in C++11 or libstdc++ 5.x
template<class... T> BOOST_MP11_CONSTEXPR auto tp_forward_r( T&&... t ) -> std::tuple<T&&...>
{
    return std::tuple<T&&...>( std::forward<T>( t )... );
}

template<class... T> BOOST_MP11_CONSTEXPR auto tp_forward_v( T&&... t ) -> std::tuple<T...>
{
    return std::tuple<T...>( std::forward<T>( t )... );
}

template<std::size_t J, class... Tp>
BOOST_MP11_CONSTEXPR auto tp_extract( Tp&&... tp )
    -> decltype( tp_forward_r( std::get<J>( std::forward<Tp>( tp ) )... ) )
{
    return tp_forward_r( std::get<J>( std::forward<Tp>( tp ) )... );
}

#if !BOOST_MP11_WORKAROUND( BOOST_MP11_MSVC, < 1900 )

template<class F, class... Tp, std::size_t... J>
BOOST_MP11_CONSTEXPR auto tuple_transform_impl( integer_sequence<std::size_t, J...>, F const& f, Tp&&... tp )
    -> decltype( tp_forward_v( tuple_apply( f, tp_extract<J>( std::forward<Tp>(tp)... ) )... ) )
{
    return tp_forward_v( tuple_apply( f, tp_extract<J>( std::forward<Tp>(tp)... ) )... );
}

#else

template<class F, class Tp1, std::size_t... J>
BOOST_MP11_CONSTEXPR auto tuple_transform_impl( integer_sequence<std::size_t, J...>, F const& f, Tp1&& tp1 )
    -> decltype( tp_forward_v( f( std::get<J>( std::forward<Tp1>(tp1) ) )... ) )
{
    return tp_forward_v( f( std::get<J>( std::forward<Tp1>(tp1) ) )... );
}

template<class F, class Tp1, class Tp2, std::size_t... J>
BOOST_MP11_CONSTEXPR auto tuple_transform_impl( integer_sequence<std::size_t, J...>, F const& f, Tp1&& tp1, Tp2&& tp2 )
    -> decltype( tp_forward_v( f( std::get<J>( std::forward<Tp1>(tp1) ), std::get<J>( std::forward<Tp2>(tp2) ) )... ) )
{
    return tp_forward_v( f( std::get<J>( std::forward<Tp1>(tp1) ), std::get<J>( std::forward<Tp2>(tp2) ) )... );
}

template<class F, class Tp1, class Tp2, class Tp3, std::size_t... J>
BOOST_MP11_CONSTEXPR auto tuple_transform_impl( integer_sequence<std::size_t, J...>, F const& f, Tp1&& tp1, Tp2&& tp2, Tp3&& tp3 )
    -> decltype( tp_forward_v( f( std::get<J>( std::forward<Tp1>(tp1) ), std::get<J>( std::forward<Tp2>(tp2) ), std::get<J>( std::forward<Tp3>(tp3) ) )... ) )
{
    return tp_forward_v( f( std::get<J>( std::forward<Tp1>(tp1) ), std::get<J>( std::forward<Tp2>(tp2) ), std::get<J>( std::forward<Tp3>(tp3) ) )... );
}

#endif // !BOOST_MP11_WORKAROUND( BOOST_MP11_MSVC, < 1900 )

} // namespace detail

#if BOOST_MP11_WORKAROUND( BOOST_MP11_MSVC, < 1910 )

template<class F, class Tp1, class... Tp,
    class Seq = make_index_sequence<std::tuple_size<typename std::remove_reference<Tp1>::type>::value>>
BOOST_MP11_CONSTEXPR auto tuple_transform( F const& f, Tp1&& tp1, Tp&&... tp )
    -> decltype( detail::tuple_transform_impl( Seq(), f, std::forward<Tp1>(tp1), std::forward<Tp>(tp)... ) )
{
    return detail::tuple_transform_impl( Seq(), f, std::forward<Tp1>(tp1), std::forward<Tp>(tp)... );
}

#else

template<class F, class... Tp,
    class Z = mp_list<mp_size_t<std::tuple_size<typename std::remove_reference<Tp>::type>::value>...>,
    class E = mp_if<mp_apply<mp_same, Z>, mp_front<Z>>,
    class Seq = make_index_sequence<E::value>>
BOOST_MP11_CONSTEXPR auto tuple_transform( F const& f, Tp&&... tp )
    -> decltype( detail::tuple_transform_impl( Seq(), f, std::forward<Tp>(tp)... ) )
{
    return detail::tuple_transform_impl( Seq(), f, std::forward<Tp>(tp)... );
}

#endif // BOOST_MP11_WORKAROUND( BOOST_MP11_MSVC, < 1910 )

} // namespace mp11
} // namespace boost

#if BOOST_MP11_MSVC
# pragma warning( pop )
#endif

#endif // #ifndef BOOST_TUPLE_HPP_INCLUDED
