//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#pragma once

#include <libguile.h>

namespace scm {
namespace detail {

#define SCM_DECLTYPE_RETURN(...)                \
    decltype(__VA_ARGS__)                       \
    { return __VA_ARGS__; }                     \
    /**/

template <typename... Ts>
constexpr bool is_valid_v = true;

template <typename... Ts>
using is_valid_t = void;

template <typename... Ts>
void check_call_once()
{
    static bool called = false;
    if (called) scm_misc_error (nullptr, "Double defined binding. \
This may be caused because there are multiple C++ binding groups in the same \
translation unit.  You may solve this by using different type tags for each \
binding group.", SCM_EOL);
    called = true;
}

struct move_sequence
{
    move_sequence() = default;
    move_sequence(const move_sequence&) = delete;
    move_sequence(move_sequence&& other)
    { other.moved_from_ = true; };

    bool moved_from_ = false;
};

} // namespace detail
} // namespace scm
