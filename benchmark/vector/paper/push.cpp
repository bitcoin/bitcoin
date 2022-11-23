//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#include "benchmark/vector/push.hpp"
#include <immer/flex_vector.hpp>
#include <immer/flex_vector_transient.hpp>
#include <chunkedseq/chunkedseq.hpp>

NONIUS_BENCHMARK("ours/basic",   benchmark_push<immer::flex_vector<unsigned,basic_memory>>())
NONIUS_BENCHMARK("ours/safe",    benchmark_push<immer::flex_vector<unsigned,def_memory>>())
NONIUS_BENCHMARK("ours/unsafe",  benchmark_push<immer::flex_vector<unsigned,unsafe_memory>>())
NONIUS_BENCHMARK("ours/gc",      benchmark_push<immer::flex_vector<unsigned,gc_memory>>())
NONIUS_BENCHMARK("librrb",       benchmark_push_librrb)

NONIUS_BENCHMARK("transient ours/basic",   benchmark_push_mut<immer::flex_vector<unsigned,basic_memory>>())
NONIUS_BENCHMARK("transient ours/safe",    benchmark_push_mut<immer::flex_vector<unsigned,def_memory>>())
NONIUS_BENCHMARK("transient ours/unsafe",  benchmark_push_mut<immer::flex_vector<unsigned,unsafe_memory>>())
NONIUS_BENCHMARK("transient ours/gc",      benchmark_push_mut<immer::flex_vector<unsigned,gc_memory>>())
NONIUS_BENCHMARK("transient librrb",       benchmark_push_mut_librrb)
NONIUS_BENCHMARK("transient std::vector",  benchmark_push_mut_std<std::vector<unsigned>>())
NONIUS_BENCHMARK("transient std::list",    benchmark_push_mut_std<std::list<unsigned>>())
NONIUS_BENCHMARK("transient chunkedseq32", benchmark_push_mut_std<pasl::data::chunkedseq::bootstrapped::deque<unsigned, 32>>())
NONIUS_BENCHMARK("transient chunkedseq",   benchmark_push_mut_std<pasl::data::chunkedseq::bootstrapped::deque<unsigned>>())
