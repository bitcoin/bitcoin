//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#include "insert.hpp"

#ifndef GENERATOR_T
#error "you must define a GENERATOR_T"
#endif

using generator__ = GENERATOR_T;
using t__         = typename decltype(generator__{}(0))::value_type;

// clang-format off
NONIUS_BENCHMARK("std::set", benchmark_insert_mut_std<generator__, std::set<t__>>())
NONIUS_BENCHMARK("std::unordered_set", benchmark_insert_mut_std<generator__, std::unordered_set<t__>>())
NONIUS_BENCHMARK("boost::flat_set", benchmark_insert_mut_std<generator__, boost::container::flat_set<t__>>())
NONIUS_BENCHMARK("hamt::hash_trie", benchmark_insert_mut_std<generator__, hamt::hash_trie<t__>>())

NONIUS_BENCHMARK("immer::set/5B", benchmark_insert<generator__, immer::set<t__, std::hash<t__>,std::equal_to<t__>,def_memory,5>>())
NONIUS_BENCHMARK("immer::set/4B", benchmark_insert<generator__, immer::set<t__, std::hash<t__>,std::equal_to<t__>,def_memory,4>>())
#ifndef DISABLE_GC_BENCHMARKS
NONIUS_BENCHMARK("immer::set/GC", benchmark_insert<generator__, immer::set<t__, std::hash<t__>,std::equal_to<t__>,gc_memory,5>>())
#endif
NONIUS_BENCHMARK("immer::set/UN", benchmark_insert<generator__, immer::set<t__, std::hash<t__>,std::equal_to<t__>,unsafe_memory,5>>())

NONIUS_BENCHMARK("immer::set/move/5B", benchmark_insert_move<generator__, immer::set<t__, std::hash<t__>,std::equal_to<t__>,def_memory,5>>())
NONIUS_BENCHMARK("immer::set/move/4B", benchmark_insert_move<generator__, immer::set<t__, std::hash<t__>,std::equal_to<t__>,def_memory,4>>())
NONIUS_BENCHMARK("immer::set/move/UN", benchmark_insert_move<generator__, immer::set<t__, std::hash<t__>,std::equal_to<t__>,unsafe_memory,5>>())

NONIUS_BENCHMARK("immer::set/tran/5B", benchmark_insert_mut_std<generator__, immer::set_transient<t__, std::hash<t__>,std::equal_to<t__>,def_memory,5>>())
NONIUS_BENCHMARK("immer::set/tran/4B", benchmark_insert_mut_std<generator__, immer::set_transient<t__, std::hash<t__>,std::equal_to<t__>,def_memory,4>>())
#ifndef DISABLE_GC_BENCHMARKS
NONIUS_BENCHMARK("immer::set/tran/GC", benchmark_insert_mut_std<generator__, immer::set_transient<t__, std::hash<t__>,std::equal_to<t__>,gc_memory,5>>())
#endif
NONIUS_BENCHMARK("immer::set/tran/UN", benchmark_insert_mut_std<generator__, immer::set_transient<t__, std::hash<t__>,std::equal_to<t__>,unsafe_memory,5>>())

// clang-format on
