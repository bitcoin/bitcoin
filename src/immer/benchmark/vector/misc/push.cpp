//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#include "benchmark/vector/push.hpp"

#include <immer/array.hpp>
#include <immer/flex_vector.hpp>
#include <immer/vector_transient.hpp>
#include <immer/vector.hpp>

#if IMMER_BENCHMARK_EXPERIMENTAL
#include <immer/experimental/dvektor.hpp>
#endif

#include <immer/heap/gc_heap.hpp>
#include <immer/refcount/no_refcount_policy.hpp>
#include <immer/refcount/unsafe_refcount_policy.hpp>

#include <vector>
#include <list>
#include <numeric>

#if IMMER_BENCHMARK_STEADY
#define QUARK_ASSERT_ON 0
#include <steady/steady_vector.h>
#endif

#if IMMER_BENCHMARK_LIBRRB
extern "C" {
#define restrict __restrict__
#include <rrb.h>
#undef restrict
}
#endif

#if IMMER_BENCHMARK_LIBRRB
NONIUS_BENCHMARK("librrb", benchmark_push_librrb)
NONIUS_BENCHMARK("t/librrb", benchmark_push_mut_librrb)
#endif

NONIUS_BENCHMARK("std::vector", benchmark_push_mut_std<std::vector<unsigned>>())
NONIUS_BENCHMARK("std::list",   benchmark_push_mut_std<std::list<unsigned>>())

NONIUS_BENCHMARK("m/vector/5B", benchmark_push_move<immer::vector<unsigned,def_memory,5>>())
NONIUS_BENCHMARK("m/vector/GC", benchmark_push_move<immer::vector<unsigned,gc_memory,5>>())
NONIUS_BENCHMARK("m/vector/NO", benchmark_push_move<immer::vector<unsigned,basic_memory,5>>())

NONIUS_BENCHMARK("t/vector/5B", benchmark_push_mut<immer::vector<unsigned,def_memory,5>>())
NONIUS_BENCHMARK("t/vector/GC", benchmark_push_mut<immer::vector<unsigned,gc_memory,5>>())
NONIUS_BENCHMARK("t/vector/NO", benchmark_push_mut<immer::vector<unsigned,basic_memory,5>>())
NONIUS_BENCHMARK("t/vector/UN", benchmark_push_mut<immer::vector<unsigned,unsafe_memory,5>>())

NONIUS_BENCHMARK("flex/5B",    benchmark_push<immer::flex_vector<unsigned,def_memory,5>>())
NONIUS_BENCHMARK("flex_s/GC",  benchmark_push<immer::flex_vector<std::size_t,gc_memory,5>>())

NONIUS_BENCHMARK("vector/4B",  benchmark_push<immer::vector<unsigned,def_memory,4>>())
NONIUS_BENCHMARK("vector/5B",  benchmark_push<immer::vector<unsigned,def_memory,5>>())
NONIUS_BENCHMARK("vector/6B",  benchmark_push<immer::vector<unsigned,def_memory,6>>())

NONIUS_BENCHMARK("vector/GC",  benchmark_push<immer::vector<unsigned,gc_memory,5>>())
NONIUS_BENCHMARK("vector/NO",  benchmark_push<immer::vector<unsigned,basic_memory,5>>())
NONIUS_BENCHMARK("vector/UN",  benchmark_push<immer::vector<unsigned,unsafe_memory,5>>())

#if IMMER_BENCHMARK_EXPERIMENTAL
NONIUS_BENCHMARK("dvektor/4B", benchmark_push<immer::dvektor<unsigned,def_memory,4>>())
NONIUS_BENCHMARK("dvektor/5B", benchmark_push<immer::dvektor<unsigned,def_memory,5>>())
NONIUS_BENCHMARK("dvektor/6B", benchmark_push<immer::dvektor<unsigned,def_memory,6>>())

NONIUS_BENCHMARK("dvektor/GC", benchmark_push<immer::dvektor<unsigned,gc_memory,5>>())
NONIUS_BENCHMARK("dvektor/NO", benchmark_push<immer::dvektor<unsigned,basic_memory,5>>())
NONIUS_BENCHMARK("dvektor/UN", benchmark_push<immer::dvektor<unsigned,unsafe_memory,5>>())
#endif

NONIUS_BENCHMARK("array",      benchmark_push<immer::array<unsigned>>())

#if IMMER_BENCHMARK_STEADY
NONIUS_BENCHMARK("steady",     benchmark_push<steady::vector<unsigned>>())
#endif
