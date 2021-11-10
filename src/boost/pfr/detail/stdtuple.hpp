// Copyright (c) 2016-2021 Antony Polukhin
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PFR_DETAIL_STDTUPLE_HPP
#define BOOST_PFR_DETAIL_STDTUPLE_HPP
#pragma once

#include <boost/pfr/detail/config.hpp>

#include <utility>      // metaprogramming stuff
#include <tuple>

#include <boost/pfr/detail/sequence_tuple.hpp>

namespace boost { namespace pfr { namespace detail {

template <class T, std::size_t... I>
constexpr auto make_stdtuple_from_tietuple(const T& t, std::index_sequence<I...>) noexcept {
    return std::make_tuple(
        boost::pfr::detail::sequence_tuple::get<I>(t)...
    );
}

template <class T, std::size_t... I>
constexpr auto make_stdtiedtuple_from_tietuple(const T& t, std::index_sequence<I...>) noexcept {
    return std::tie(
        boost::pfr::detail::sequence_tuple::get<I>(t)...
    );
}

template <class T, std::size_t... I>
constexpr auto make_conststdtiedtuple_from_tietuple(const T& t, std::index_sequence<I...>) noexcept {
    return std::tuple<
        std::add_lvalue_reference_t<std::add_const_t<
            std::remove_reference_t<decltype(boost::pfr::detail::sequence_tuple::get<I>(t))>
        >>...
    >(
        boost::pfr::detail::sequence_tuple::get<I>(t)...
    );
}

}}} // namespace boost::pfr::detail

#endif // BOOST_PFR_DETAIL_STDTUPLE_HPP
