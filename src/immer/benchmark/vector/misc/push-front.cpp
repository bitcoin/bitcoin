//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#include "benchmark/vector/push_front.hpp"

#include <immer/flex_vector.hpp>

#if IMMER_BENCHMARK_LIBRRB
extern "C" {
#define restrict __restrict__
#include <rrb.h>
#undef restrict
}
#endif

#if IMMER_BENCHMARK_LIBRRB
NONIUS_BENCHMARK("librrb", benchmark_push_front_librrb)
#endif
NONIUS_BENCHMARK("flex/4B", bechmark_push_front<immer::flex_vector<unsigned,def_memory,4>>())
NONIUS_BENCHMARK("flex/5B", bechmark_push_front<immer::flex_vector<unsigned,def_memory,5>>())
NONIUS_BENCHMARK("flex/6B", bechmark_push_front<immer::flex_vector<unsigned,def_memory,6>>())
NONIUS_BENCHMARK("flex/GC", bechmark_push_front<immer::flex_vector<unsigned,gc_memory,5>>())
NONIUS_BENCHMARK("flex_s/GC", bechmark_push_front<immer::flex_vector<std::size_t,gc_memory,5>>())
NONIUS_BENCHMARK("flex/NO", bechmark_push_front<immer::flex_vector<unsigned,basic_memory,5>>())
NONIUS_BENCHMARK("flex/UN", bechmark_push_front<immer::flex_vector<unsigned,unsafe_memory,5>>())
