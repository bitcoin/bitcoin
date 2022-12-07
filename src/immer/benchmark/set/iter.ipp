//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#include "iter.hpp"

#ifndef GENERATOR_T
#error "you must define a GENERATOR_T"
#endif

using generator__ = GENERATOR_T;
using t__ = typename decltype(generator__{}(0))::value_type;

NONIUS_BENCHMARK("iter/std::set", benchmark_access_std_iter<generator__, std::set<t__>>())
NONIUS_BENCHMARK("iter/std::unordered_set", benchmark_access_std_iter<generator__, std::unordered_set<t__>>())
NONIUS_BENCHMARK("iter/boost::flat_set", benchmark_access_std_iter<generator__, boost::container::flat_set<t__>>())
NONIUS_BENCHMARK("iter/hamt::hash_trie", benchmark_access_std_iter<generator__, hamt::hash_trie<t__>>())
NONIUS_BENCHMARK("iter/immer::set/5B", benchmark_access_iter<generator__, immer::set<t__, std::hash<t__>,std::equal_to<t__>,def_memory,5>>())
NONIUS_BENCHMARK("iter/immer::set/4B", benchmark_access_iter<generator__, immer::set<t__, std::hash<t__>,std::equal_to<t__>,def_memory,4>>())
NONIUS_BENCHMARK("reduce/immer::set/5B", benchmark_access_reduce<generator__, immer::set<t__, std::hash<t__>,std::equal_to<t__>,def_memory,5>>())
NONIUS_BENCHMARK("reduce/immer::set/4B", benchmark_access_reduce<generator__, immer::set<t__, std::hash<t__>,std::equal_to<t__>,def_memory,4>>())
