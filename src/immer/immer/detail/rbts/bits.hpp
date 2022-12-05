//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#pragma once

#include <cstddef>
#include <cstdint>

namespace immer {
namespace detail {
namespace rbts {

using bits_t  = std::uint32_t;
using shift_t = std::uint32_t;
using count_t = std::uint32_t;
using size_t  = std::size_t;

template <bits_t B, typename T = count_t>
constexpr T branches = T{1} << B;

template <bits_t B, typename T = size_t>
constexpr T mask = branches<B, T> - 1;

template <bits_t B, bits_t BL>
constexpr shift_t endshift = shift_t{BL} - shift_t{B};

} // namespace rbts
} // namespace detail
} // namespace immer
