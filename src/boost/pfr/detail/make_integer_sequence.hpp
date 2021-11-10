// Copyright (c) 2018 Sergei Fedorov
// Copyright (c) 2019-2021 Antony Polukhin
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PFR_DETAIL_MAKE_INTEGER_SEQUENCE_HPP
#define BOOST_PFR_DETAIL_MAKE_INTEGER_SEQUENCE_HPP
#pragma once

#include <boost/pfr/detail/config.hpp>

#include <type_traits>
#include <utility>
#include <cstddef>

namespace boost { namespace pfr { namespace detail {

#if BOOST_PFR_USE_STD_MAKE_INTEGRAL_SEQUENCE == 0

#ifdef __has_builtin
#   if __has_builtin(__make_integer_seq)
#       define BOOST_PFR_USE_MAKE_INTEGER_SEQ_BUILTIN
#   endif
#endif

#ifdef BOOST_PFR_USE_MAKE_INTEGER_SEQ_BUILTIN

using std::integer_sequence;

// Clang unable to use namespace qualified std::integer_sequence in __make_integer_seq.
template <typename T, T N>
using make_integer_sequence = __make_integer_seq<integer_sequence, T, N>;

#undef BOOST_PFR_USE_MAKE_INTEGER_SEQ_BUILTIN

#else

template <typename T, typename U>
struct join_sequences;

template <typename T, T... A, T... B>
struct join_sequences<std::integer_sequence<T, A...>, std::integer_sequence<T, B...>> {
    using type = std::integer_sequence<T, A..., B...>;
};

template <typename T, T Min, T Max>
struct build_sequence_impl {
    static_assert(Min < Max, "Start of range must be less than its end");
    static constexpr T size = Max - Min;
    using type = typename join_sequences<
            typename build_sequence_impl<T, Min, Min + size / 2>::type,
            typename build_sequence_impl<T, Min + size / 2 + 1, Max>::type
        >::type;
};

template <typename T, T V>
struct build_sequence_impl<T, V, V> {
    using type = std::integer_sequence<T, V>;
};

template <typename T, std::size_t N>
struct make_integer_sequence_impl : build_sequence_impl<T, 0, N - 1> {};

template <typename T>
struct make_integer_sequence_impl<T, 0> {
    using type = std::integer_sequence<T>;
};

template <typename T, T N>
using make_integer_sequence = typename make_integer_sequence_impl<T, N>::type;

#endif // !defined BOOST_PFR_USE_MAKE_INTEGER_SEQ_BUILTIN
#else // BOOST_PFR_USE_STD_MAKE_INTEGRAL_SEQUENCE == 1

template <typename T, T N>
using make_integer_sequence = std::make_integer_sequence<T, N>;

#endif // BOOST_PFR_USE_STD_MAKE_INTEGRAL_SEQUENCE == 1

template <std::size_t N>
using make_index_sequence = make_integer_sequence<std::size_t, N>;

template <typename... T>
using index_sequence_for = make_index_sequence<sizeof...(T)>;

}}} // namespace boost::pfr::detail

#endif

