// Boost.Geometry

// Copyright (c) 2020, Oracle and/or its affiliates.

// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Licensed under the Boost Software License version 1.0.
// http://www.boost.org/users/license.html

#ifndef BOOST_GEOMETRY_UTIL_SEQUENCE_HPP
#define BOOST_GEOMETRY_UTIL_SEQUENCE_HPP


#include <type_traits>


namespace boost { namespace geometry
{

namespace util
{


// An alternative would be to use std:tuple and std::pair
//   but it would add dependency.


template <typename ...Ts>
struct type_sequence {};


// true if T is a sequence
template <typename T>
struct is_sequence : std::false_type {};

template <typename ...Ts>
struct is_sequence<type_sequence<Ts...>> : std::true_type {};

template <typename T, T ...Is>
struct is_sequence<std::integer_sequence<T, Is...>> : std::true_type {};


// number of elements in a sequence
template <typename Sequence>
struct sequence_size {};

template <typename ...Ts>
struct sequence_size<type_sequence<Ts...>>
    : std::integral_constant<std::size_t, sizeof...(Ts)>
{};

template <typename T, T ...Is>
struct sequence_size<std::integer_sequence<T, Is...>>
    : std::integral_constant<std::size_t, sizeof...(Is)>
{};


// element of a sequence
template <std::size_t I, typename Sequence>
struct sequence_element {};

template <std::size_t I, typename T, typename ...Ts>
struct sequence_element<I, type_sequence<T, Ts...>>
{
    typedef typename sequence_element<I - 1, type_sequence<Ts...>>::type type;
};

template <typename T, typename ...Ts>
struct sequence_element<0, type_sequence<T, Ts...>>
{
    typedef T type;
};

template <std::size_t I, typename T, T J, T ...Js>
struct sequence_element<I, std::integer_sequence<T, J, Js...>>
    : std::integral_constant
        <
            T,
            sequence_element<I - 1, std::integer_sequence<T, Js...>>::value
        >
{};

template <typename T, T J, T ...Js>
struct sequence_element<0, std::integer_sequence<T, J, Js...>>
    : std::integral_constant<T, J>
{};


template <typename Sequence>
struct sequence_front
    : sequence_element<0, Sequence>
{
    static_assert(sequence_size<Sequence>::value > 0, "Sequence can not be empty.");
};


template <typename Sequence>
struct sequence_back
    : sequence_element<sequence_size<Sequence>::value - 1, Sequence>
{
    static_assert(sequence_size<Sequence>::value > 0, "Sequence can not be empty.");
};


template <typename Sequence>
struct sequence_empty
    : std::integral_constant
        <
            bool,
            sequence_size<Sequence>::value == 0
        >
{};


template
<
    typename Sequence,
    template <typename> class UnaryPred
>
struct sequence_find_if {};

template
<
    typename T, typename ...Ts,
    template <typename> class UnaryPred
>
struct sequence_find_if<type_sequence<T, Ts...>, UnaryPred>
    : std::conditional
        <
            UnaryPred<T>::value,
            T,
            // TODO: prevent instantiation for the rest of the sequence if value is true
            typename sequence_find_if<type_sequence<Ts...>, UnaryPred>::type
        >
{};

template <template <typename> class UnaryPred>
struct sequence_find_if<type_sequence<>, UnaryPred>
{
    // TODO: This is technically incorrect because void can be stored in a type_sequence
    typedef void type;
};


// merge<type_sequence<A, B>, type_sequence<C, D>>::type is
//   type_sequence<A, B, C, D>
// merge<integer_sequence<A, B>, integer_sequence<C, D>>::type is
//   integer_sequence<A, B, C, D>
template <typename ...Sequences>
struct merge;

template <typename S>
struct merge<S>
{
    typedef S type;
};

template <typename ...T1s, typename ...T2s>
struct merge<type_sequence<T1s...>, type_sequence<T2s...>>
{
    typedef type_sequence<T1s..., T2s...> type;
};

template <typename T, T ...I1s, T ...I2s>
struct merge<std::integer_sequence<T, I1s...>, std::integer_sequence<T, I2s...>>
{
    typedef std::integer_sequence<T, I1s..., I2s...> type;
};

template <typename S1, typename S2, typename ...Sequences>
struct merge<S1, S2, Sequences...>
{
    typedef typename merge
        <
            typename merge<S1, S2>::type,
            typename merge<Sequences...>::type
        >::type type;
};


// combine<type_sequence<A, B>, type_sequence<C, D>>::type is
//   type_sequence<type_sequence<A, C>, type_sequence<A, D>,
//                 type_sequence<B, C>, type_sequence<B, D>>
template <typename Sequence1, typename Sequence2>
struct combine;

template <typename ...T1s, typename ...T2s>
struct combine<type_sequence<T1s...>, type_sequence<T2s...>>
{
    template <typename U1, typename ...U2s>
    using type_sequence_t = type_sequence<type_sequence<U1, U2s>...>;

    typedef typename merge
        <
            type_sequence_t<T1s, T2s...>...
        >::type type;
};

// combine<integer_sequence<T, 1, 2>, integer_sequence<T, 3, 4>>::type is
//   type_sequence<integer_sequence<T, 1, 3>, integer_sequence<T, 1, 4>,
//                 integer_sequence<T, 2, 3>, integer_sequence<T, 2, 4>>
template <typename T, T ...I1s, T ...I2s>
struct combine<std::integer_sequence<T, I1s...>, std::integer_sequence<T, I2s...>>
{
    template <T J1, T ...J2s>
    using type_sequence_t = type_sequence<std::integer_sequence<T, J1, J2s>...>;

    typedef typename merge
        <
            type_sequence_t<I1s, I2s...>...
        >::type type;
};


// Selects least element from a parameter pack based on
// LessPred<T1, T2>::value comparison, similar to std::min_element
template
<
    template <typename, typename> class LessPred,
    typename ...Ts
>
struct select_pack_element;

template
<
    template <typename, typename> class LessPred,
    typename T
>
struct select_pack_element<LessPred, T>
{
    typedef T type;
};

template
<
    template <typename, typename> class LessPred,
    typename T1, typename T2
>
struct select_pack_element<LessPred, T1, T2>
{
    typedef std::conditional_t<LessPred<T1, T2>::value, T1, T2> type;
};

template
<
    template <typename, typename> class LessPred,
    typename T1, typename T2, typename ...Ts
>
struct select_pack_element<LessPred, T1, T2, Ts...>
{
    typedef typename select_pack_element
        <
            LessPred,
            typename select_pack_element<LessPred, T1, T2>::type,
            typename select_pack_element<LessPred, Ts...>::type
        >::type type;
};


// Selects least element from a sequence based on
// LessPred<T1, T2>::value comparison, similar to std::min_element
template
<
    typename Sequence,
    template <typename, typename> class LessPred    
>
struct select_element;

template
<
    typename ...Ts,
    template <typename, typename> class LessPred
>
struct select_element<type_sequence<Ts...>, LessPred>
{
    typedef typename select_pack_element<LessPred, Ts...>::type type;
};


// Selects least pair sequence of elements from a parameter pack based on
// LessPred<xxx_sequence<T11, T12>, xxx_sequence<T21, T22>>::value comparison
template
<
    typename Sequence1,
    typename Sequence2,
    template <typename, typename> class LessPred
>
struct select_combination_element
{
    typedef typename select_element
        <
            typename combine<Sequence1, Sequence2>::type,
            LessPred
        >::type type;
};


} // namespace util


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_UTIL_SEQUENCE_HPP
