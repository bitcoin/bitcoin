//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#include "benchmark/vector/concat.hpp"
#include <immer/flex_vector.hpp>
#include <immer/flex_vector_transient.hpp>
#include <chunkedseq/chunkedseq.hpp>

NONIUS_BENCHMARK("ours/basic",   benchmark_concat_incr<immer::flex_vector<unsigned,basic_memory>>())
NONIUS_BENCHMARK("ours/safe",    benchmark_concat_incr<immer::flex_vector<unsigned,def_memory>>())
NONIUS_BENCHMARK("ours/unsafe",  benchmark_concat_incr<immer::flex_vector<unsigned,unsafe_memory>>())
NONIUS_BENCHMARK("ours/gc",      benchmark_concat_incr<immer::flex_vector<unsigned,gc_memory>>())
NONIUS_BENCHMARK("librrb",       benchmark_concat_incr_librrb(make_librrb_vector))

NONIUS_BENCHMARK("transient ours/gc",      benchmark_concat_incr_mut<immer::flex_vector<unsigned,gc_memory>>())
NONIUS_BENCHMARK("transient chunkedseq32", benchmark_concat_incr_chunkedseq<pasl::data::chunkedseq::bootstrapped::deque<unsigned, 32>>())
NONIUS_BENCHMARK("transient chunkedseq",   benchmark_concat_incr_chunkedseq<pasl::data::chunkedseq::bootstrapped::deque<unsigned>>())
