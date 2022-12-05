//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#include "benchmark/vector/access.hpp"
#include <immer/flex_vector.hpp>
#include <chunkedseq/chunkedseq.hpp>

NONIUS_BENCHMARK("idx owrs", benchmark_access_idx<immer::flex_vector<unsigned,def_memory>>())
NONIUS_BENCHMARK("idx librrb", benchmark_access_librrb(make_librrb_vector))
NONIUS_BENCHMARK("idx relaxed owrs", benchmark_access_idx<immer::flex_vector<unsigned,def_memory>,push_front_fn>())
NONIUS_BENCHMARK("idx relaxed librrb", benchmark_access_librrb(make_librrb_vector_f))
NONIUS_BENCHMARK("idx std::vector", benchmark_access_idx_std<std::vector<unsigned>>())
NONIUS_BENCHMARK("idx chunkedseq32", benchmark_access_idx_std<pasl::data::chunkedseq::bootstrapped::deque<unsigned, 32>>())
NONIUS_BENCHMARK("idx chunkedseq", benchmark_access_idx_std<pasl::data::chunkedseq::bootstrapped::deque<unsigned>>())

NONIUS_BENCHMARK("iter orws", benchmark_access_iter<immer::flex_vector<unsigned,def_memory>>())
NONIUS_BENCHMARK("iter relaxed orws", benchmark_access_iter<immer::flex_vector<unsigned,def_memory>,push_front_fn>())
NONIUS_BENCHMARK("iter chunkedseq32", benchmark_access_iter_std<pasl::data::chunkedseq::bootstrapped::deque<unsigned, 32>>())
NONIUS_BENCHMARK("iter chunkedseq", benchmark_access_iter_std<pasl::data::chunkedseq::bootstrapped::deque<unsigned>>())

NONIUS_BENCHMARK("reduce owrs", benchmark_access_reduce<immer::flex_vector<unsigned,def_memory>>())
NONIUS_BENCHMARK("reduce relaxed owrs", benchmark_access_reduce<immer::flex_vector<unsigned,def_memory>,push_front_fn>())
NONIUS_BENCHMARK("reduce chunkedseq32", benchmark_access_reduce_chunkedseq<pasl::data::chunkedseq::bootstrapped::deque<unsigned, 32>>())
NONIUS_BENCHMARK("reduce chunkedseq", benchmark_access_reduce_chunkedseq<pasl::data::chunkedseq::bootstrapped::deque<unsigned>>())
NONIUS_BENCHMARK("reduce std::vector", benchmark_access_iter_std<std::vector<unsigned>>())
NONIUS_BENCHMARK("reduce std::list", benchmark_access_iter_std<std::list<unsigned>>())
