//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#include "benchmark/vector/take.hpp"

#include <immer/vector.hpp>
#include <immer/flex_vector.hpp>
#include <immer/vector_transient.hpp>
#include <immer/flex_vector_transient.hpp>
#include <immer/heap/gc_heap.hpp>
#include <immer/refcount/no_refcount_policy.hpp>
#include <immer/refcount/unsafe_refcount_policy.hpp>

#if IMMER_BENCHMARK_LIBRRB
extern "C" {
#define restrict __restrict__
#include <rrb.h>
#undef restrict
}
#endif

#if IMMER_BENCHMARK_LIBRRB
NONIUS_BENCHMARK("librrb", benchmark_take_librrb(make_librrb_vector))
NONIUS_BENCHMARK("librrb/F", benchmark_take_librrb(make_librrb_vector_f))
NONIUS_BENCHMARK("l/librrb", benchmark_take_lin_librrb(make_librrb_vector))
NONIUS_BENCHMARK("l/librrb/F", benchmark_take_lin_librrb(make_librrb_vector_f))
NONIUS_BENCHMARK("t/librrb", benchmark_take_mut_librrb(make_librrb_vector))
NONIUS_BENCHMARK("t/librrb/F", benchmark_take_mut_librrb(make_librrb_vector_f))
#endif

NONIUS_BENCHMARK("vector/4B", benchmark_take<immer::vector<unsigned,def_memory,4>>())
NONIUS_BENCHMARK("vector/5B", benchmark_take<immer::vector<unsigned,def_memory,5>>())
NONIUS_BENCHMARK("vector/6B", benchmark_take<immer::vector<unsigned,def_memory,6>>())
NONIUS_BENCHMARK("vector/GC", benchmark_take<immer::vector<unsigned,gc_memory,5>>())
NONIUS_BENCHMARK("vector/NO", benchmark_take<immer::vector<unsigned,basic_memory,5>>())
NONIUS_BENCHMARK("vector/UN", benchmark_take<immer::vector<unsigned,unsafe_memory,5>>())
NONIUS_BENCHMARK("vector_s/GC", benchmark_take<immer::vector<std::size_t,gc_memory,5>>())

NONIUS_BENCHMARK("flex/F/5B", benchmark_take<immer::flex_vector<unsigned,def_memory,5>, push_front_fn>())
NONIUS_BENCHMARK("flex/F/GC", benchmark_take<immer::flex_vector<unsigned,gc_memory,5>, push_front_fn>())
NONIUS_BENCHMARK("flex/F/GCF", benchmark_take<immer::flex_vector<unsigned,gcf_memory,5>, push_front_fn>())
NONIUS_BENCHMARK("flex_s/F/GC", benchmark_take<immer::flex_vector<std::size_t,gc_memory,5>, push_front_fn>())

NONIUS_BENCHMARK("l/vector/5B", benchmark_take_lin<immer::vector<unsigned,def_memory,5>>())
NONIUS_BENCHMARK("l/vector/GC", benchmark_take_lin<immer::vector<unsigned,gc_memory,5>>())
NONIUS_BENCHMARK("l/vector/NO", benchmark_take_lin<immer::vector<unsigned,basic_memory,5>>())
NONIUS_BENCHMARK("l/vector/UN", benchmark_take_lin<immer::vector<unsigned,unsafe_memory,5>>())
NONIUS_BENCHMARK("l/flex/F/5B", benchmark_take_lin<immer::flex_vector<unsigned,def_memory,5>, push_front_fn>())

NONIUS_BENCHMARK("m/vector/5B", benchmark_take_move<immer::vector<unsigned,def_memory,5>>())
NONIUS_BENCHMARK("m/vector/GC", benchmark_take_move<immer::vector<unsigned,gc_memory,5>>())
NONIUS_BENCHMARK("m/vector/NO", benchmark_take_move<immer::vector<unsigned,basic_memory,5>>())
NONIUS_BENCHMARK("m/vector/UN", benchmark_take_move<immer::vector<unsigned,unsafe_memory,5>>())
NONIUS_BENCHMARK("m/flex/F/5B", benchmark_take_move<immer::flex_vector<unsigned,def_memory,5>, push_front_fn>())

NONIUS_BENCHMARK("t/vector/5B", benchmark_take_mut<immer::vector<unsigned,def_memory,5>>())
NONIUS_BENCHMARK("t/vector/GC", benchmark_take_mut<immer::vector<unsigned,gc_memory,5>>())
NONIUS_BENCHMARK("t/vector/NO", benchmark_take_mut<immer::vector<unsigned,basic_memory,5>>())
NONIUS_BENCHMARK("t/vector/UN", benchmark_take_mut<immer::vector<unsigned,unsafe_memory,5>>())
NONIUS_BENCHMARK("t/flex/F/5B", benchmark_take_mut<immer::flex_vector<unsigned,def_memory,5>, push_front_fn>())
