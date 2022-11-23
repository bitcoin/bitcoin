//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#include "benchmark/vector/drop.hpp"

#include <immer/flex_vector.hpp>
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
NONIUS_BENCHMARK("librrb", benchmark_drop_librrb(make_librrb_vector))
NONIUS_BENCHMARK("librrb/F", benchmark_drop_librrb(make_librrb_vector_f))
NONIUS_BENCHMARK("l/librrb", benchmark_drop_lin_librrb(make_librrb_vector))
NONIUS_BENCHMARK("l/librrb/F", benchmark_drop_lin_librrb(make_librrb_vector_f))
NONIUS_BENCHMARK("t/librrb", benchmark_drop_mut_librrb(make_librrb_vector))
NONIUS_BENCHMARK("t/librrb/F", benchmark_drop_mut_librrb(make_librrb_vector_f))
#endif

NONIUS_BENCHMARK("flex/4B", benchmark_drop<immer::flex_vector<unsigned,def_memory,4>>())
NONIUS_BENCHMARK("flex/5B", benchmark_drop<immer::flex_vector<unsigned,def_memory,5>>())
NONIUS_BENCHMARK("flex/6B", benchmark_drop<immer::flex_vector<unsigned,def_memory,6>>())
NONIUS_BENCHMARK("flex/GC", benchmark_drop<immer::flex_vector<unsigned,gc_memory,5>>())
NONIUS_BENCHMARK("flex/NO", benchmark_drop<immer::flex_vector<unsigned,basic_memory,5>>())
NONIUS_BENCHMARK("flex/UN", benchmark_drop<immer::flex_vector<unsigned,unsafe_memory,5>>())
NONIUS_BENCHMARK("flex_s/GC", benchmark_drop<immer::flex_vector<std::size_t,gc_memory,5>>())

NONIUS_BENCHMARK("flex/F/5B", benchmark_drop<immer::flex_vector<unsigned,def_memory,5>, push_front_fn>())
NONIUS_BENCHMARK("flex/F/GC", benchmark_drop<immer::flex_vector<unsigned,gc_memory,5>, push_front_fn>())
NONIUS_BENCHMARK("flex/F/GCF", benchmark_drop<immer::flex_vector<unsigned,gcf_memory,5>, push_front_fn>())
NONIUS_BENCHMARK("flex_s/F/GC", benchmark_drop<immer::flex_vector<std::size_t,gc_memory,5>, push_front_fn>())

NONIUS_BENCHMARK("l/flex/5B", benchmark_drop_lin<immer::flex_vector<unsigned,def_memory,5>>())
NONIUS_BENCHMARK("l/flex/GC", benchmark_drop_lin<immer::flex_vector<unsigned,gc_memory,5>>())
NONIUS_BENCHMARK("l/flex/NO", benchmark_drop_lin<immer::flex_vector<unsigned,basic_memory,5>>())
NONIUS_BENCHMARK("l/flex/UN", benchmark_drop_lin<immer::flex_vector<unsigned,unsafe_memory,5>>())
NONIUS_BENCHMARK("l/flex/F/5B", benchmark_drop_lin<immer::flex_vector<unsigned,def_memory,5>, push_front_fn>())

NONIUS_BENCHMARK("m/flex/5B", benchmark_drop_move<immer::flex_vector<unsigned,def_memory,5>>())
NONIUS_BENCHMARK("m/flex/GC", benchmark_drop_move<immer::flex_vector<unsigned,gc_memory,5>>())
NONIUS_BENCHMARK("m/flex/NO", benchmark_drop_move<immer::flex_vector<unsigned,basic_memory,5>>())
NONIUS_BENCHMARK("m/flex/UN", benchmark_drop_move<immer::flex_vector<unsigned,unsafe_memory,5>>())
NONIUS_BENCHMARK("m/flex/F/5B", benchmark_drop_move<immer::flex_vector<unsigned,def_memory,5>, push_front_fn>())

NONIUS_BENCHMARK("t/flex/5B", benchmark_drop_mut<immer::flex_vector<unsigned,def_memory,5>>())
NONIUS_BENCHMARK("t/flex/GC", benchmark_drop_mut<immer::flex_vector<unsigned,gc_memory,5>>())
NONIUS_BENCHMARK("t/flex/NO", benchmark_drop_mut<immer::flex_vector<unsigned,basic_memory,5>>())
NONIUS_BENCHMARK("t/flex/UN", benchmark_drop_mut<immer::flex_vector<unsigned,unsafe_memory,5>>())
NONIUS_BENCHMARK("t/flex/F/5B", benchmark_drop_mut<immer::flex_vector<unsigned,def_memory,5>, push_front_fn>())
