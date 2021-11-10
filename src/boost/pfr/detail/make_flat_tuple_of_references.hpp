// Copyright (c) 2016-2021 Antony Polukhin
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PFR_DETAIL_MAKE_FLAT_TUPLE_OF_REFERENCES_HPP
#define BOOST_PFR_DETAIL_MAKE_FLAT_TUPLE_OF_REFERENCES_HPP
#pragma once

#include <boost/pfr/detail/config.hpp>

#include <utility>      // metaprogramming stuff
#include <boost/pfr/detail/sequence_tuple.hpp>
#include <boost/pfr/detail/rvalue_t.hpp>
#include <boost/pfr/detail/make_integer_sequence.hpp>


namespace boost { namespace pfr { namespace detail {

template <std::size_t Index>
using size_t_ = std::integral_constant<std::size_t, Index >;

// Helper: Make a "getter" object corresponding to built-in tuple::get
// For user-defined structures, the getter should be "offset_based_getter"
struct sequence_tuple_getter {
  template <std::size_t idx, typename TupleOfReferences>
  decltype(auto) get(TupleOfReferences&& t, size_t_<idx>) const noexcept {
    return sequence_tuple::get<idx>(std::forward<TupleOfReferences>(t));
  }
};


template <class TupleOrUserType, class Getter, std::size_t Begin, std::size_t Size>
constexpr auto make_flat_tuple_of_references(TupleOrUserType&, const Getter&, size_t_<Begin>, size_t_<Size>) noexcept;

template <class TupleOrUserType, class Getter, std::size_t Begin>
constexpr sequence_tuple::tuple<> make_flat_tuple_of_references(TupleOrUserType&, const Getter&, size_t_<Begin>, size_t_<0>) noexcept;

template <class TupleOrUserType, class Getter, std::size_t Begin>
constexpr auto make_flat_tuple_of_references(TupleOrUserType&, const Getter&, size_t_<Begin>, size_t_<1>) noexcept;

template <class... T>
constexpr auto tie_as_tuple_with_references(T&... args) noexcept {
    return sequence_tuple::tuple<T&...>{ args... };
}

template <class... T>
constexpr decltype(auto) tie_as_tuple_with_references(detail::sequence_tuple::tuple<T...>& t) noexcept {
    return detail::make_flat_tuple_of_references(t, sequence_tuple_getter{}, size_t_<0>{}, size_t_<sequence_tuple::tuple<T...>::size_v>{});
}

template <class... T>
constexpr decltype(auto) tie_as_tuple_with_references(const detail::sequence_tuple::tuple<T...>& t) noexcept {
    return detail::make_flat_tuple_of_references(t, sequence_tuple_getter{}, size_t_<0>{}, size_t_<sequence_tuple::tuple<T...>::size_v>{});
}

template <class Tuple1, std::size_t... I1, class Tuple2, std::size_t... I2>
constexpr auto my_tuple_cat_impl(const Tuple1& t1, std::index_sequence<I1...>, const Tuple2& t2, std::index_sequence<I2...>) noexcept {
    return detail::tie_as_tuple_with_references(
        sequence_tuple::get<I1>(t1)...,
        sequence_tuple::get<I2>(t2)...
    );
}

template <class Tuple1, class Tuple2>
constexpr auto my_tuple_cat(const Tuple1& t1, const Tuple2& t2) noexcept {
    return detail::my_tuple_cat_impl(
        t1, detail::make_index_sequence< Tuple1::size_v >{},
        t2, detail::make_index_sequence< Tuple2::size_v >{}
    );
}

template <class TupleOrUserType, class Getter, std::size_t Begin, std::size_t Size>
constexpr auto make_flat_tuple_of_references(TupleOrUserType& t, const Getter& g, size_t_<Begin>, size_t_<Size>) noexcept {
    constexpr std::size_t next_size = Size / 2;
    return detail::my_tuple_cat(
        detail::make_flat_tuple_of_references(t, g, size_t_<Begin>{}, size_t_<next_size>{}),
        detail::make_flat_tuple_of_references(t, g, size_t_<Begin + Size / 2>{}, size_t_<Size - next_size>{})
    );
}

template <class TupleOrUserType, class Getter, std::size_t Begin>
constexpr sequence_tuple::tuple<> make_flat_tuple_of_references(TupleOrUserType&, const Getter&, size_t_<Begin>, size_t_<0>) noexcept {
    return {};
}

template <class TupleOrUserType, class Getter, std::size_t Begin>
constexpr auto make_flat_tuple_of_references(TupleOrUserType& t, const Getter& g, size_t_<Begin>, size_t_<1>) noexcept {
    return detail::tie_as_tuple_with_references(
        g.get(t, size_t_<Begin>{})
    );
}

}}} // namespace boost::pfr::detail

#endif // BOOST_PFR_DETAIL_MAKE_FLAT_TUPLE_OF_REFERENCES_HPP
