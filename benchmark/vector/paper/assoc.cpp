//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#include "benchmark/vector/assoc.hpp"
#include <immer/flex_vector.hpp>
#include <immer/flex_vector_transient.hpp>
#include <chunkedseq/chunkedseq.hpp>

NONIUS_BENCHMARK("ours/basic",   benchmark_assoc<immer::flex_vector<unsigned,basic_memory>>())
NONIUS_BENCHMARK("ours/safe",    benchmark_assoc<immer::flex_vector<unsigned,def_memory>>())
NONIUS_BENCHMARK("ours/unsafe",  benchmark_assoc<immer::flex_vector<unsigned,unsafe_memory>>())
NONIUS_BENCHMARK("ours/gc",      benchmark_assoc<immer::flex_vector<unsigned,gc_memory>>())
NONIUS_BENCHMARK("librrb",       benchmark_assoc_librrb(make_librrb_vector))

NONIUS_BENCHMARK("relaxed ours/basic",   benchmark_assoc<immer::flex_vector<unsigned,basic_memory>,push_front_fn>())
NONIUS_BENCHMARK("relaxed ours/safe",    benchmark_assoc<immer::flex_vector<unsigned,def_memory>,push_front_fn>())
NONIUS_BENCHMARK("relaxed ours/unsafe",  benchmark_assoc<immer::flex_vector<unsigned,unsafe_memory>,push_front_fn>())
NONIUS_BENCHMARK("relaxed ours/gc",      benchmark_assoc<immer::flex_vector<unsigned,gc_memory>,push_front_fn>())
NONIUS_BENCHMARK("relaxed librrb",       benchmark_assoc_librrb(make_librrb_vector_f))

NONIUS_BENCHMARK("transient ours/basic",   benchmark_assoc_mut<immer::flex_vector<unsigned,basic_memory>>())
NONIUS_BENCHMARK("transient ours/safe",    benchmark_assoc_mut<immer::flex_vector<unsigned,def_memory>>())
NONIUS_BENCHMARK("transient ours/unsafe",  benchmark_assoc_mut<immer::flex_vector<unsigned,unsafe_memory>>())
NONIUS_BENCHMARK("transient ours/gc",      benchmark_assoc_mut<immer::flex_vector<unsigned,gc_memory>>())
NONIUS_BENCHMARK("transient librrb",       benchmark_assoc_mut_librrb(make_librrb_vector))

NONIUS_BENCHMARK("transient relaxed ours/basic",   benchmark_assoc_mut<immer::flex_vector<unsigned,basic_memory>,push_back_fn>())
NONIUS_BENCHMARK("transient relaxed ours/safe",    benchmark_assoc_mut<immer::flex_vector<unsigned,def_memory>,push_back_fn>())
NONIUS_BENCHMARK("transient relaxed ours/unsafe",  benchmark_assoc_mut<immer::flex_vector<unsigned,unsafe_memory>,push_back_fn>())
NONIUS_BENCHMARK("transient relaxed ours/gc",      benchmark_assoc_mut<immer::flex_vector<unsigned,gc_memory>,push_back_fn>())
NONIUS_BENCHMARK("transient relaxed librrb",       benchmark_assoc_mut_librrb(make_librrb_vector_f))

NONIUS_BENCHMARK("transient std::vector",  benchmark_assoc_std<std::vector<unsigned>>())
NONIUS_BENCHMARK("transient chunkedseq32", benchmark_assoc_std<pasl::data::chunkedseq::bootstrapped::deque<unsigned, 32>>())
NONIUS_BENCHMARK("transient chunkedseq",   benchmark_assoc_std<pasl::data::chunkedseq::bootstrapped::deque<unsigned>>())
