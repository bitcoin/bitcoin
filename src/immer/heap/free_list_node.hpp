//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#pragma once

#include <immer/heap/with_data.hpp>

namespace immer {

struct free_list_node
{
    free_list_node* next;
};

template <typename Base>
struct with_free_list_node
    : with_data<free_list_node, Base>
{};

} // namespace immer
