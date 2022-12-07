//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#pragma once

namespace scm {
namespace detail {

struct none_t;

template <typename... Ts>
struct pack {};

template <typename Pack>
struct pack_size;

template <typename... Ts>
struct pack_size<pack<Ts...>>
{
    static constexpr auto value = sizeof...(Ts);
};

template <typename Pack>
constexpr auto pack_size_v = pack_size<Pack>::value;

template <typename Pack>
struct pack_last
{
    using type = none_t;
};

template <typename T, typename ...Ts>
struct pack_last<pack<T, Ts...>>
    : pack_last<pack<Ts...>>
{};

template <typename T>
struct pack_last<pack<T>>
{
    using type = T;
};

template <typename Pack>
using pack_last_t = typename pack_last<Pack>::type;

} // namespace detail
} // namespace scm
