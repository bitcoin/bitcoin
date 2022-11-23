//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#include "benchmark/vector/assoc.hpp"

#include <immer/array.hpp>
#include <immer/flex_vector.hpp>
#include <immer/flex_vector_transient.hpp>
#include <immer/vector.hpp>
#include <immer/vector_transient.hpp>

#if IMMER_BENCHMARK_EXPERIMENTAL
#include <immer/experimental/dvektor.hpp>
#endif

#include <immer/heap/gc_heap.hpp>
#include <immer/refcount/no_refcount_policy.hpp>
#include <immer/refcount/unsafe_refcount_policy.hpp>

#include <vector>
#include <list>

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

NONIUS_BENCHMARK("std::vector", benchmark_assoc_std<std::vector<unsigned>>())
NONIUS_BENCHMARK("std::vector/random", benchmark_assoc_random_std<std::vector<unsigned>>())

#if IMMER_BENCHMARK_LIBRRB
NONIUS_BENCHMARK("librrb", benchmark_assoc_librrb(make_librrb_vector))
NONIUS_BENCHMARK("librrb/F", benchmark_assoc_librrb(make_librrb_vector_f))
NONIUS_BENCHMARK("librrb/random", benchmark_assoc_random_librrb(make_librrb_vector))
NONIUS_BENCHMARK("t/librrb", benchmark_assoc_mut_librrb(make_librrb_vector))
NONIUS_BENCHMARK("t/librrb/F", benchmark_assoc_mut_librrb(make_librrb_vector_f))
NONIUS_BENCHMARK("t/librrb/random", benchmark_assoc_mut_random_librrb(make_librrb_vector))
#endif

#if IMMER_BENCHMARK_STEADY
NONIUS_BENCHMARK("steady",     benchmark_assoc<steady::vector<unsigned>, push_back_fn, store_fn>())
#endif

NONIUS_BENCHMARK("flex/5B",    benchmark_assoc<immer::flex_vector<unsigned,def_memory,5>>())
NONIUS_BENCHMARK("flex/F/5B",  benchmark_assoc<immer::flex_vector<unsigned,def_memory,5>,push_front_fn>())

NONIUS_BENCHMARK("flex/GC",    benchmark_assoc<immer::flex_vector<unsigned,gc_memory,5>>())
NONIUS_BENCHMARK("flex/F/GC",  benchmark_assoc<immer::flex_vector<unsigned,gc_memory,5>,push_front_fn>())
NONIUS_BENCHMARK("flex/F/GCF", benchmark_assoc<immer::flex_vector<unsigned,gcf_memory,5>,push_front_fn>())

NONIUS_BENCHMARK("flex_s/GC",  benchmark_assoc<immer::flex_vector<std::size_t,gc_memory,5>>())
NONIUS_BENCHMARK("flex_s/F/GC",benchmark_assoc<immer::flex_vector<std::size_t,gc_memory,5>,push_front_fn>())
NONIUS_BENCHMARK("flex_s/F/GCF",benchmark_assoc<immer::flex_vector<std::size_t,gcf_memory,5>,push_front_fn>())

NONIUS_BENCHMARK("vector/4B",  benchmark_assoc<immer::vector<unsigned,def_memory,4>>())
NONIUS_BENCHMARK("vector/5B",  benchmark_assoc<immer::vector<unsigned,def_memory,5>>())
NONIUS_BENCHMARK("vector/6B",  benchmark_assoc<immer::vector<unsigned,def_memory,6>>())

NONIUS_BENCHMARK("vector/GC",  benchmark_assoc<immer::vector<unsigned,gc_memory,5>>())
NONIUS_BENCHMARK("vector/NO",  benchmark_assoc<immer::vector<unsigned,basic_memory,5>>())
NONIUS_BENCHMARK("vector/UN",  benchmark_assoc<immer::vector<unsigned,unsafe_memory,5>>())

#if IMMER_BENCHMARK_EXPERIMENTAL
NONIUS_BENCHMARK("dvektor/4B", benchmark_assoc<immer::dvektor<unsigned,def_memory,4>>())
NONIUS_BENCHMARK("dvektor/5B", benchmark_assoc<immer::dvektor<unsigned,def_memory,5>>())
NONIUS_BENCHMARK("dvektor/6B", benchmark_assoc<immer::dvektor<unsigned,def_memory,6>>())

NONIUS_BENCHMARK("dvektor/GC", benchmark_assoc<immer::dvektor<unsigned,gc_memory,5>>())
NONIUS_BENCHMARK("dvektor/NO", benchmark_assoc<immer::dvektor<unsigned,basic_memory,5>>())
NONIUS_BENCHMARK("dvektor/UN", benchmark_assoc<immer::dvektor<unsigned,unsafe_memory,5>>())
#endif

NONIUS_BENCHMARK("array",      benchmark_assoc<immer::array<unsigned>>())

#if IMMER_BENCHMARK_STEADY
NONIUS_BENCHMARK("steady/random",      benchmark_assoc_random<steady::vector<unsigned>, push_back_fn, store_fn>())
#endif

NONIUS_BENCHMARK("flex/5B/random",     benchmark_assoc_random<immer::flex_vector<unsigned,def_memory,5>>())
NONIUS_BENCHMARK("vector/4B/random",   benchmark_assoc_random<immer::vector<unsigned,def_memory,4>>())
NONIUS_BENCHMARK("vector/5B/random",   benchmark_assoc_random<immer::vector<unsigned,def_memory,5>>())
NONIUS_BENCHMARK("vector/6B/random",   benchmark_assoc_random<immer::vector<unsigned,def_memory,6>>())
#if IMMER_BENCHMARK_EXPERIMENTAL
NONIUS_BENCHMARK("dvektor/4B/random",  benchmark_assoc_random<immer::dvektor<unsigned,def_memory,4>>())
NONIUS_BENCHMARK("dvektor/5B/random",  benchmark_assoc_random<immer::dvektor<unsigned,def_memory,5>>())
NONIUS_BENCHMARK("dvektor/6B/random",  benchmark_assoc_random<immer::dvektor<unsigned,def_memory,6>>())
#endif
NONIUS_BENCHMARK("array/random",       benchmark_assoc_random<immer::array<unsigned>>())

NONIUS_BENCHMARK("t/vector/5B",  benchmark_assoc_mut<immer::vector<unsigned,def_memory,5>>())
NONIUS_BENCHMARK("t/vector/GC",  benchmark_assoc_mut<immer::vector<unsigned,gc_memory,5>>())
NONIUS_BENCHMARK("t/vector/NO",  benchmark_assoc_mut<immer::vector<unsigned,basic_memory,5>>())
NONIUS_BENCHMARK("t/vector/UN",  benchmark_assoc_mut<immer::vector<unsigned,unsafe_memory,5>>())
NONIUS_BENCHMARK("t/flex/F/5B",  benchmark_assoc_mut<immer::flex_vector<unsigned,def_memory,5>,push_front_fn>())

NONIUS_BENCHMARK("m/vector/5B",  benchmark_assoc_move<immer::vector<unsigned,def_memory,5>>())
NONIUS_BENCHMARK("m/vector/GC",  benchmark_assoc_move<immer::vector<unsigned,gc_memory,5>>())
NONIUS_BENCHMARK("m/vector/NO",  benchmark_assoc_move<immer::vector<unsigned,basic_memory,5>>())
NONIUS_BENCHMARK("m/vector/UN",  benchmark_assoc_move<immer::vector<unsigned,unsafe_memory,5>>())
NONIUS_BENCHMARK("m/flex/F/5B",  benchmark_assoc_move<immer::flex_vector<unsigned,def_memory,5>,push_front_fn>())

NONIUS_BENCHMARK("t/vector/5B/random",  benchmark_assoc_mut_random<immer::vector<unsigned,def_memory,5>>())
NONIUS_BENCHMARK("t/vector/GC/random",  benchmark_assoc_mut_random<immer::vector<unsigned,gc_memory,5>>())
NONIUS_BENCHMARK("t/vector/NO/random",  benchmark_assoc_mut_random<immer::vector<unsigned,basic_memory,5>>())
NONIUS_BENCHMARK("t/vector/UN/random",  benchmark_assoc_mut_random<immer::vector<unsigned,unsafe_memory,5>>())
NONIUS_BENCHMARK("t/flex/F/5B/random",  benchmark_assoc_mut_random<immer::flex_vector<unsigned,def_memory,5>,push_front_fn>())
