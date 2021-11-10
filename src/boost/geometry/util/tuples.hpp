// Boost.Geometry Index
//
// Copyright (c) 2011-2013 Adam Wulkiewicz, Lodz, Poland.
//
// This file was modified by Oracle on 2019-2020.
// Modifications copyright (c) 2019-2020 Oracle and/or its affiliates.
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle
//
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_UTIL_TUPLES_HPP
#define BOOST_GEOMETRY_UTIL_TUPLES_HPP

#include <tuple>
#include <type_traits>
#include <utility>

#include <boost/geometry/core/config.hpp>

#include <boost/tuple/tuple.hpp>

namespace boost { namespace geometry { namespace tuples
{

template <typename T>
struct is_tuple
    : std::integral_constant<bool, false>
{};

template <typename ...Ts>
struct is_tuple<std::tuple<Ts...>>
    : std::integral_constant<bool, true>
{};

template <typename F, typename S>
struct is_tuple<std::pair<F, S>>
    : std::integral_constant<bool, true>
{};

template <typename ...Ts>
struct is_tuple<boost::tuples::tuple<Ts...>>
    : std::integral_constant<bool, true>
{};

template <typename HT, typename TT>
struct is_tuple<boost::tuples::cons<HT, TT>>
    : std::integral_constant<bool, true>
{};


template <std::size_t I, typename Tuple>
struct element;

template <std::size_t I, typename ...Ts>
struct element<I, std::tuple<Ts...>>
    : std::tuple_element<I, std::tuple<Ts...>>
{};

template <std::size_t I, typename HT, typename TT>
struct element<I, std::pair<HT, TT>>
    : std::tuple_element<I, std::pair<HT, TT>>
{};

template <std::size_t I, typename ...Ts>
struct element<I, boost::tuples::tuple<Ts...>>
{
    typedef typename boost::tuples::element
        <
            I, boost::tuples::tuple<Ts...>
        >::type type;
};

template <std::size_t I, typename HT, typename TT>
struct element<I, boost::tuples::cons<HT, TT>>
{
    typedef typename boost::tuples::element
        <
            I, boost::tuples::cons<HT, TT>
        >::type type;
};


template <typename Tuple>
struct size;

template <typename ...Ts>
struct size<std::tuple<Ts...>>
    : std::tuple_size<std::tuple<Ts...>>
{};

template <typename HT, typename TT>
struct size<std::pair<HT, TT>>
    : std::tuple_size<std::pair<HT, TT>>
{};

template <typename ...Ts>
struct size<boost::tuples::tuple<Ts...>>
    : std::integral_constant
        <
            std::size_t,
            boost::tuples::length<boost::tuples::tuple<Ts...>>::value
        >
{};

template <typename HT, typename TT>
struct size<boost::tuples::cons<HT, TT>>
    : std::integral_constant
        <
            std::size_t,
            boost::tuples::length<boost::tuples::cons<HT, TT>>::value
        >
{};


template <std::size_t I, typename ...Ts>
constexpr inline typename std::tuple_element<I, std::tuple<Ts...>>::type&
get(std::tuple<Ts...> & t)
{
    return std::get<I>(t);
}

template <std::size_t I, typename ...Ts>
constexpr inline typename std::tuple_element<I, std::tuple<Ts...>>::type const&
get(std::tuple<Ts...> const& t)
{
    return std::get<I>(t);
}

template <std::size_t I, typename HT, typename TT>
constexpr inline typename std::tuple_element<I, std::pair<HT, TT>>::type&
get(std::pair<HT, TT> & t)
{
    return std::get<I>(t);
}

template <std::size_t I, typename HT, typename TT>
constexpr inline typename std::tuple_element<I, std::pair<HT, TT>>::type const&
get(std::pair<HT, TT> const& t)
{
    return std::get<I>(t);
}

template <std::size_t I, typename ...Ts>
inline typename boost::tuples::access_traits
    <
        typename boost::tuples::element<I, boost::tuples::tuple<Ts...>>::type
    >::non_const_type
get(boost::tuples::tuple<Ts...> & t)
{
    return boost::tuples::get<I>(t);
}

template <std::size_t I, typename ...Ts>
inline typename boost::tuples::access_traits
    <
        typename boost::tuples::element<I, boost::tuples::tuple<Ts...>>::type
    >::const_type
get(boost::tuples::tuple<Ts...> const& t)
{
    return boost::tuples::get<I>(t);
}


template <std::size_t I, typename HT, typename TT>
inline typename boost::tuples::access_traits
    <
        typename boost::tuples::element<I, boost::tuples::cons<HT, TT> >::type
    >::non_const_type
get(boost::tuples::cons<HT, TT> & tup)
{
    return boost::tuples::get<I>(tup);
}

template <std::size_t I, typename HT, typename TT>
inline typename boost::tuples::access_traits
    <
        typename boost::tuples::element<I, boost::tuples::cons<HT, TT> >::type
    >::const_type
get(boost::tuples::cons<HT, TT> const& tup)
{
    return boost::tuples::get<I>(tup);
}



// find_index_if
// Searches for the index of an element for which UnaryPredicate returns true
// If such element is not found the result is N

template
<
    typename Tuple,
    template <typename> class UnaryPred,
    std::size_t I = 0,
    std::size_t N = size<Tuple>::value
>
struct find_index_if
    : std::conditional_t
        <
            UnaryPred<typename element<I, Tuple>::type>::value,
            std::integral_constant<std::size_t, I>,
            typename find_index_if<Tuple, UnaryPred, I+1, N>::type
        >
{};

template
<
    typename Tuple,
    template <typename> class UnaryPred,
    std::size_t N
>
struct find_index_if<Tuple, UnaryPred, N, N>
    : std::integral_constant<std::size_t, N>
{};


// find_if
// Searches for an element for which UnaryPredicate returns true
// If such element is not found the result is detail::null_type

namespace detail
{

struct null_type {};

} // detail

template
<
    typename Tuple,
    template <typename> class UnaryPred,
    std::size_t I = 0,
    std::size_t N = size<Tuple>::value
>
struct find_if
    : std::conditional_t
        <
            UnaryPred<typename element<I, Tuple>::type>::value,
            element<I, Tuple>,
            find_if<Tuple, UnaryPred, I+1, N>
        >
{};

template
<
    typename Tuple,
    template <typename> class UnaryPred,
    std::size_t N
>
struct find_if<Tuple, UnaryPred, N, N>
{
    typedef detail::null_type type;
};


// is_found
// Returns true if a type T (the result of find_if) was found.

template <typename T>
struct is_found
    : std::integral_constant
        <
            bool,
            ! std::is_same<T, detail::null_type>::value
        >
{};


// is_not_found
// Returns true if a type T (the result of find_if) was not found.

template <typename T>
struct is_not_found
    : std::is_same<T, detail::null_type>
{};


// exists_if
// Returns true if search for element meeting UnaryPred can be found.

template <typename Tuple, template <typename> class UnaryPred>
struct exists_if
    : is_found<typename find_if<Tuple, UnaryPred>::type>
{};


// push_back
// A utility used to create a type/object of a Tuple containing
//   all types/objects stored in another Tuple plus additional one.

template <typename Tuple,
          typename T,
          std::size_t I = 0,
          std::size_t N = size<Tuple>::value>
struct push_back_bt
{
    typedef
    boost::tuples::cons<
        typename element<I, Tuple>::type,
        typename push_back_bt<Tuple, T, I+1, N>::type
    > type;

    static type apply(Tuple const& tup, T const& t)
    {
        return
        type(
            geometry::tuples::get<I>(tup),
            push_back_bt<Tuple, T, I+1, N>::apply(tup, t)
        );
    }
};

template <typename Tuple, typename T, std::size_t N>
struct push_back_bt<Tuple, T, N, N>
{
    typedef boost::tuples::cons<T, boost::tuples::null_type> type;

    static type apply(Tuple const&, T const& t)
    {
        return type(t, boost::tuples::null_type());
    }
};

template <typename Tuple, typename T>
struct push_back
    : push_back_bt<Tuple, T>
{};

template <typename F, typename S, typename T>
struct push_back<std::pair<F, S>, T>
{
    typedef std::tuple<F, S, T> type;

    static type apply(std::pair<F, S> const& p, T const& t)
    {
        return type(p.first, p.second, t);
    }

    static type apply(std::pair<F, S> && p, T const& t)
    {
        return type(std::move(p.first), std::move(p.second), t);
    }

    static type apply(std::pair<F, S> && p, T && t)
    {
        return type(std::move(p.first), std::move(p.second), std::move(t));
    }
};

template <typename Is, typename Tuple, typename T>
struct push_back_st;

template <std::size_t ...Is, typename ...Ts, typename T>
struct push_back_st<std::index_sequence<Is...>, std::tuple<Ts...>, T>
{
    typedef std::tuple<Ts..., T> type;

    static type apply(std::tuple<Ts...> const& tup, T const& t)
    {
        return type(std::get<Is>(tup)..., t);
    }

    static type apply(std::tuple<Ts...> && tup, T const& t)
    {
        return type(std::move(std::get<Is>(tup))..., t);
    }

    static type apply(std::tuple<Ts...> && tup, T && t)
    {
        return type(std::move(std::get<Is>(tup))..., std::move(t));
    }
};

template <typename ...Ts, typename T>
struct push_back<std::tuple<Ts...>, T>
    : push_back_st
        <
            std::make_index_sequence<sizeof...(Ts)>,
            std::tuple<Ts...>,
            T
        >
{};


}}} // namespace boost::geometry::tuples

#endif // BOOST_GEOMETRY_UTIL_TUPLES_HPP
