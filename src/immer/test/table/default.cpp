//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#include <immer/config.hpp>
#include <immer/memory_policy.hpp>

struct setup_t
{
    using memory_policy = immer::default_memory_policy;

    static constexpr auto bits = immer::default_bits;
};

#define SETUP_T setup_t
#include "generic.ipp"
