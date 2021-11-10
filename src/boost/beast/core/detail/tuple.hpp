//
// Copyright (c) 2016-2019 Damian Jarek (damian dot jarek93 at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

#ifndef BOOST_BEAST_DETAIL_TUPLE_HPP
#define BOOST_BEAST_DETAIL_TUPLE_HPP

#include <boost/mp11/integer_sequence.hpp>
#include <boost/mp11/algorithm.hpp>
#include <boost/type_traits/remove_cv.hpp>
#include <boost/type_traits/copy_cv.hpp>
#include <cstdlib>
#include <utility>

namespace boost {
namespace beast {
namespace detail {

template<std::size_t I, class T>
struct tuple_element_impl
{
    T t;

    tuple_element_impl(T const& t_)
        : t(t_)
    {
    }

    tuple_element_impl(T&& t_)
        : t(std::move(t_))
    {
    }
};

template<std::size_t I, class T>
struct tuple_element_impl<I, T&>
{
    T& t;

    tuple_element_impl(T& t_)
        : t(t_)
    {
    }
};

template<class... Ts>
struct tuple_impl;

template<class... Ts, std::size_t... Is>
struct tuple_impl<
    boost::mp11::index_sequence<Is...>, Ts...>
  : tuple_element_impl<Is, Ts>...
{
    template<class... Us>
    explicit tuple_impl(Us&&... us)
        : tuple_element_impl<Is, Ts>(
            std::forward<Us>(us))...
    {
    }
};

template<class... Ts>
struct tuple : tuple_impl<
    boost::mp11::index_sequence_for<Ts...>, Ts...>
{
    template<class... Us>
    explicit tuple(Us&&... us)
      : tuple_impl<
            boost::mp11::index_sequence_for<Ts...>, Ts...>{
          std::forward<Us>(us)...}
    {
    }
};

template<std::size_t I, class T>
T&
get(tuple_element_impl<I, T>& te)
{
    return te.t;
}

template<std::size_t I, class T>
T const&
get(tuple_element_impl<I, T> const& te)
{
    return te.t;
}

template<std::size_t I, class T>
T&&
get(tuple_element_impl<I, T>&& te)
{
    return std::move(te.t);
}

template<std::size_t I, class T>
T&
get(tuple_element_impl<I, T&>&& te)
{
    return te.t;
}

template <std::size_t I, class T>
using tuple_element = typename boost::copy_cv<
    mp11::mp_at_c<typename remove_cv<T>::type, I>, T>::type;

} // detail
} // beast
} // boost

#endif
