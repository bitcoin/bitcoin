//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#include <immer/set.hpp>

template <typename T,
          typename Hash = std::hash<T>,
          typename Eq   = std::equal_to<T>>
using test_set_t = immer::set<T, Hash, Eq, immer::default_memory_policy, 6u>;

#define SET_T test_set_t
#include "generic.ipp"
