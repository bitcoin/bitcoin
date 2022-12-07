//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#include "benchmark/vector/access.hpp"
#include <immer/flex_vector.hpp>

#ifndef MEMORY_T
#error "define the MEMORY_T"
#endif

NONIUS_BENCHMARK("flex/3", benchmark_access_idx<immer::flex_vector<std::size_t,MEMORY_T,3>>())
NONIUS_BENCHMARK("flex/4", benchmark_access_idx<immer::flex_vector<std::size_t,MEMORY_T,4>>())
NONIUS_BENCHMARK("flex/5", benchmark_access_idx<immer::flex_vector<std::size_t,MEMORY_T,5>>())
NONIUS_BENCHMARK("flex/6", benchmark_access_idx<immer::flex_vector<std::size_t,MEMORY_T,6>>())
NONIUS_BENCHMARK("flex/7", benchmark_access_idx<immer::flex_vector<std::size_t,MEMORY_T,7>>())
