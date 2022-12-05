//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#include "benchmark/vector/access.hpp"

#include <immer/algorithm.hpp>
#include <immer/array.hpp>
#include <immer/flex_vector.hpp>
#include <immer/vector.hpp>

#if IMMER_BENCHMARK_EXPERIMENTAL
#include <immer/experimental/dvektor.hpp>
#endif

#if IMMER_BENCHMARK_STEADY
#define QUARK_ASSERT_ON 0
#include <steady/steady_vector.h>
#endif

#include <vector>
#include <list>
#include <numeric>

NONIUS_BENCHMARK("std::vector", benchmark_access_iter_std<std::vector<unsigned>>())
NONIUS_BENCHMARK("std::vector/idx", benchmark_access_idx_std<std::vector<unsigned>>())
NONIUS_BENCHMARK("std::vector/random", benchmark_access_random_std<std::vector<unsigned>>())
NONIUS_BENCHMARK("std::list", benchmark_access_iter_std<std::list<unsigned>>())

#if IMMER_BENCHMARK_LIBRRB
NONIUS_BENCHMARK("librrb", benchmark_access_librrb(make_librrb_vector))
NONIUS_BENCHMARK("librrb/F", benchmark_access_librrb(make_librrb_vector_f))
NONIUS_BENCHMARK("librrb/random", benchmark_access_random_librrb(make_librrb_vector))
NONIUS_BENCHMARK("librrb/F/random", benchmark_access_random_librrb(make_librrb_vector_f))
#endif

NONIUS_BENCHMARK("flex/5B",     benchmark_access_iter<immer::flex_vector<unsigned,def_memory,5>>())
NONIUS_BENCHMARK("flex/F/5B",   benchmark_access_iter<immer::flex_vector<unsigned,def_memory,5>,push_front_fn>())
NONIUS_BENCHMARK("vector/4B",   benchmark_access_iter<immer::vector<unsigned,def_memory,4>>())
NONIUS_BENCHMARK("vector/5B",   benchmark_access_iter<immer::vector<unsigned,def_memory,5>>())
NONIUS_BENCHMARK("vector/6B",   benchmark_access_iter<immer::vector<unsigned,def_memory,6>>())

#if IMMER_BENCHMARK_EXPERIMENTAL
NONIUS_BENCHMARK("dvektor/4B",  benchmark_access_iter<immer::dvektor<unsigned,def_memory,4>>())
NONIUS_BENCHMARK("dvektor/5B",  benchmark_access_iter<immer::dvektor<unsigned,def_memory,5>>())
NONIUS_BENCHMARK("dvektor/6B",  benchmark_access_iter<immer::dvektor<unsigned,def_memory,6>>())
#endif

#if IMMER_BENCHMARK_STEADY
NONIUS_BENCHMARK("steady/idx",      benchmark_access_idx<steady::vector<unsigned>>())
#endif

NONIUS_BENCHMARK("flex/5B/idx",     benchmark_access_idx<immer::flex_vector<unsigned,def_memory,5>>())
NONIUS_BENCHMARK("flex/F/5B/idx",   benchmark_access_idx<immer::flex_vector<unsigned,def_memory,5>,push_front_fn>())
NONIUS_BENCHMARK("vector/4B/idx",   benchmark_access_idx<immer::vector<unsigned,def_memory,4>>())
NONIUS_BENCHMARK("vector/5B/idx",   benchmark_access_idx<immer::vector<unsigned,def_memory,5>>())
NONIUS_BENCHMARK("vector/6B/idx",   benchmark_access_idx<immer::vector<unsigned,def_memory,6>>())
#if IMMER_BENCHMARK_EXPERIMENTAL
NONIUS_BENCHMARK("dvektor/4B/idx",  benchmark_access_idx<immer::dvektor<unsigned,def_memory,4>>())
NONIUS_BENCHMARK("dvektor/5B/idx",  benchmark_access_idx<immer::dvektor<unsigned,def_memory,5>>())
NONIUS_BENCHMARK("dvektor/6B/idx",  benchmark_access_idx<immer::dvektor<unsigned,def_memory,6>>())
#endif

NONIUS_BENCHMARK("flex/5B/reduce_i", benchmark_access_reduce_range<immer::flex_vector<unsigned,def_memory,5>>())
NONIUS_BENCHMARK("vector/5B/reduce_i", benchmark_access_reduce_range<immer::vector<unsigned,def_memory,5>>())

NONIUS_BENCHMARK("flex/5B/reduce",   benchmark_access_reduce<immer::flex_vector<unsigned,def_memory,5>>())
NONIUS_BENCHMARK("flex/F/5B/reduce", benchmark_access_reduce<immer::flex_vector<unsigned,def_memory,5>,push_front_fn>())
NONIUS_BENCHMARK("vector/4B/reduce", benchmark_access_reduce<immer::vector<unsigned,def_memory,4>>())
NONIUS_BENCHMARK("vector/5B/reduce", benchmark_access_reduce<immer::vector<unsigned,def_memory,5>>())
NONIUS_BENCHMARK("vector/6B/reduce", benchmark_access_reduce<immer::vector<unsigned,def_memory,6>>())

#if IMMER_BENCHMARK_STEADY
NONIUS_BENCHMARK("steady/random",      benchmark_access_random<steady::vector<unsigned>>())
#endif

NONIUS_BENCHMARK("flex/5B/random",     benchmark_access_random<immer::flex_vector<unsigned,def_memory,5>>())
NONIUS_BENCHMARK("flex/F/5B/random",   benchmark_access_random<immer::flex_vector<unsigned,def_memory,5>, push_front_fn>())
NONIUS_BENCHMARK("vector/4B/random",   benchmark_access_random<immer::vector<unsigned,def_memory,4>>())
NONIUS_BENCHMARK("vector/5B/random",   benchmark_access_random<immer::vector<unsigned,def_memory,5>>())
NONIUS_BENCHMARK("vector/6B/random",   benchmark_access_random<immer::vector<unsigned,def_memory,6>>())
#if IMMER_BENCHMARK_EXPERIMENTAL
NONIUS_BENCHMARK("dvektor/4B/random",  benchmark_access_random<immer::dvektor<unsigned,def_memory,4>>())
NONIUS_BENCHMARK("dvektor/5B/random",  benchmark_access_random<immer::dvektor<unsigned,def_memory,5>>())
NONIUS_BENCHMARK("dvektor/6B/random",  benchmark_access_random<immer::dvektor<unsigned,def_memory,6>>())
#endif

#if IMMER_BENCHMARK_BOOST_COROUTINE
NONIUS_BENCHMARK("vector/5B/coro", benchmark_access_coro<immer::vector<unsigned,def_memory,5>>())
#endif
