//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#pragma once

//
// These are some experiments to get insights about memory usage with various
// data-structures and configurations.
//
// The idea is to run this inside valgrind's massif tool and see what comes
// out.  The following is the libraries that we do check.
//

// these are for "exp" tests
#include <boost/container/flat_set.hpp>
#include <hash_trie.hpp> // Phil Nash
#include <immer/set.hpp>
#include <immer/set_transient.hpp>
#include <set>
#include <unordered_set>

// these are for "lin" tests, which are map based actually
#include <boost/container/flat_map.hpp>
#include <hash_trie.hpp> // Phil Nash
#include <immer/map.hpp>
#include <immer/map_transient.hpp>
#include <map>
#include <unordered_map>

#include <boost/core/demangle.hpp>
#include <iostream>
#include <random>
#include <vector>

#include <valgrind/valgrind.h>

struct generate_string_short
{
    static constexpr auto char_set =
        "_-0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    static constexpr auto max_length = 15;
    static constexpr auto min_length = 4;

    auto operator()() const
    {
        auto engine = std::default_random_engine{42};
        auto dist   = std::uniform_int_distribution<unsigned>{};
        auto gen    = std::bind(dist, engine);

        return [=]() mutable {
            auto len = gen() % (max_length - min_length) + min_length;
            auto str = std::string(len, ' ');
            std::generate_n(str.begin(), len, [&] {
                return char_set[gen() % sizeof(char_set)];
            });
            return str;
        };
    }
};

struct generate_string_long
{
    static constexpr auto char_set =
        "_-0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    static constexpr auto max_length = 256;
    static constexpr auto min_length = 32;

    auto operator()() const
    {
        auto engine = std::default_random_engine{42};
        auto dist   = std::uniform_int_distribution<unsigned>{};
        auto gen    = std::bind(dist, engine);

        return [=]() mutable {
            auto len = gen() % (max_length - min_length) + min_length;
            auto str = std::string(len, ' ');
            std::generate_n(str.begin(), len, [&] {
                return char_set[gen() % sizeof(char_set)];
            });
            return str;
        };
    }
};

struct generate_unsigned
{
    auto operator()() const
    {
        auto engine = std::default_random_engine{42};
        auto dist   = std::uniform_int_distribution<unsigned>{};
        auto gen    = std::bind(dist, engine);
        return gen;
    }
};

namespace basic_params {
constexpr auto N = 1 << 20;

void take_snapshot(std::size_t i)
{
    std::cerr << "  snapshot " << i << " / " << N << std::endl;
    // This is not doing what we thing it does.. it is better to control the
    // snapshot generation with something like this:
    //
    //   --max-snapshots=1000 --detailed-freq=1
    //
    // VALGRIND_MONITOR_COMMAND("detailed_snapshot");
    // VALGRIND_MONITOR_COMMAND("all_snapshots");
}
} // namespace basic_params

template <typename Generator, typename Set>
auto benchmark_memory_basic_std()
{
    using namespace basic_params;

    std::cerr << "running... " << boost::core::demangle(typeid(Set).name())
              << std::endl;

    auto rs = std::vector<Set>{};
    auto g  = Generator{}();
    auto v  = Set{};

    take_snapshot(0);
    for (auto i = 0u; i < N; ++i) {
        v.insert(g());
    }
    take_snapshot(N);

    volatile auto dont_optimize_ = v.size();
    return dont_optimize_;
}

template <typename Generator, typename Set>
auto benchmark_memory_basic()
{
    using namespace basic_params;

    std::cerr << "running... " << boost::core::demangle(typeid(Set).name())
              << std::endl;

    auto g = Generator{}();
    auto v = Set{};

    take_snapshot(0);
    for (auto i = 0u; i < N; ++i) {
        v = std::move(v).insert(g());
    }
    take_snapshot(N);

    volatile auto dont_optimize_ = v.size();
    return dont_optimize_;
}

namespace exp_params {
constexpr auto N = 1 << 20;
constexpr auto E = 2;

void take_snapshot(std::size_t i)
{
    std::cerr << "  snapshot " << i << " / " << N << std::endl;
    // This is not doing what we thing it does.. it is better to control the
    // snapshot generation with something like this:
    //
    //   --max-snapshots=1000 --detailed-freq=1
    //
    // VALGRIND_MONITOR_COMMAND("detailed_snapshot");
    // VALGRIND_MONITOR_COMMAND("all_snapshots");
}
} // namespace exp_params

template <typename Generator, typename Set>
auto benchmark_memory_exp_std()
{
    using namespace exp_params;

    std::cerr << "running... " << boost::core::demangle(typeid(Set).name())
              << std::endl;

    auto rs = std::vector<Set>{};
    auto g  = Generator{}();
    auto v  = Set{};

    take_snapshot(0);
    for (auto i = 0u, n = 1u; i < N; ++i) {
        if (i == n) {
            rs.push_back(v);
            n *= E;
            take_snapshot(i);
        }
        v.insert(g());
    }
    take_snapshot(N);

    volatile auto dont_optimize_ = rs.data();
    return dont_optimize_;
}

template <typename Generator, typename Set>
auto benchmark_memory_exp()
{
    using namespace exp_params;

    std::cerr << "running... " << boost::core::demangle(typeid(Set).name())
              << std::endl;

    auto rs = std::vector<Set>{};
    auto g  = Generator{}();
    auto v  = Set{};

    take_snapshot(0);
    for (auto i = 0u, n = 1u; i < N; ++i) {
        if (i == n) {
            rs.push_back(v);
            n *= E;
            take_snapshot(i);
        }
        v = std::move(v).insert(g());
    }
    take_snapshot(N);

    volatile auto dont_optimize_ = rs.data();
    return dont_optimize_;
}

namespace lin_params {
constexpr auto N = 1 << 14;
constexpr auto M = 1 << 7;
constexpr auto S = 1;

void take_snapshot(std::size_t i)
{
    std::cerr << "  snapshot " << i << " / " << M << std::endl;
    // This is not doing what we thing it does.. it is better to control the
    // snapshot generation with something like this:
    //
    //   --max-snapshots=1000 --detailed-freq=1
    //
    // VALGRIND_MONITOR_COMMAND("detailed_snapshot");
    // VALGRIND_MONITOR_COMMAND("all_snapshots");
}
} // namespace lin_params

template <typename Generator, typename Map>
auto benchmark_memory_lin_std()
{
    using namespace lin_params;

    std::cerr << "running... " << boost::core::demangle(typeid(Map).name())
              << std::endl;

    auto rs = std::vector<Map>{};
    auto ks = std::vector<typename Map::key_type>{};
    auto g  = Generator{}();
    auto v  = Map{};
    auto ug = generate_unsigned{}();

    take_snapshot(0);
    for (auto i = 0u; i < N; ++i) {
        auto k = g();
        v.insert({k, 0u});
        ks.push_back(std::move(k));
    }
    take_snapshot(N);

    take_snapshot(0);
    for (auto i = 0u; i < M; ++i) {
        for (auto j = 0u; j < S; ++j) {
            auto&& k = ks[ug() % ks.size()];
            ++v.at(k);
        }
        rs.push_back(v);
        take_snapshot(i);
    }
    take_snapshot(M);

    volatile auto dont_optimize_ = rs.data();
    return dont_optimize_;
}

template <typename Generator, typename Map>
auto benchmark_memory_lin()
{
    using namespace lin_params;

    std::cerr << "running... " << boost::core::demangle(typeid(Map).name())
              << std::endl;

    auto rs = std::vector<Map>{};
    auto ks = std::vector<typename Map::key_type>{};
    auto g  = Generator{}();
    auto v  = Map{};
    auto ug = generate_unsigned{}();

    take_snapshot(0);
    for (auto i = 0u; i < N; ++i) {
        auto k = g();
        v      = std::move(v).insert({k, 0u});
        ks.push_back(std::move(k));
    }
    take_snapshot(N);

    take_snapshot(0);
    for (auto i = 0u; i < M; ++i) {
        for (auto j = 0u; j < S; ++j) {
            auto&& k = ks[ug() % ks.size()];
            v        = std::move(v).update(k, [](auto x) { return ++x; });
        }
        rs.push_back(v);
        take_snapshot(i);
    }
    take_snapshot(M);

    volatile auto dont_optimize_ = rs.data();
    return dont_optimize_;
}

#if IMMER_BENCHMARK_MEMORY_STRING_SHORT
using generator__ = generate_string_short;
using t__         = std::string;
#elif IMMER_BENCHMARK_MEMORY_STRING_LONG
using generator__ = generate_string_long;
using t__         = std::string;
#elif IMMER_BENCHMARK_MEMORY_UNSIGNED
using generator__ = generate_unsigned;
using t__         = unsigned;
#else
#error "choose some type!"
#endif

int main_basic()
{
    benchmark_memory_basic_std<generator__, std::set<t__>>();
    benchmark_memory_basic_std<generator__, std::unordered_set<t__>>();

    // too slow, why?
    // benchmark_memory_basic_std<generator__,
    // boost::container::flat_set<t__>>();

    // very bad... just ignore...
    // benchmark_memory_basic_std<generator__, hamt::hash_trie<t__>>();

    using def_memory = immer::default_memory_policy;

    benchmark_memory_basic<
        generator__,
        immer::set<t__, std::hash<t__>, std::equal_to<t__>, def_memory, 2>>();
    benchmark_memory_basic<
        generator__,
        immer::set<t__, std::hash<t__>, std::equal_to<t__>, def_memory, 3>>();
    benchmark_memory_basic<
        generator__,
        immer::set<t__, std::hash<t__>, std::equal_to<t__>, def_memory, 4>>();
    benchmark_memory_basic<
        generator__,
        immer::set<t__, std::hash<t__>, std::equal_to<t__>, def_memory, 5>>();
    benchmark_memory_basic<
        generator__,
        immer::set<t__, std::hash<t__>, std::equal_to<t__>, def_memory, 6>>();

    return 0;
}

int main_exp()
{
    benchmark_memory_exp_std<generator__, std::set<t__>>();
    benchmark_memory_exp_std<generator__, std::unordered_set<t__>>();

    // too slow, why?
    // benchmark_memory_exp_std<generator__, boost::container::flat_set<t__>>();

    // very bad... just ignore...
    // benchmark_memory_exp_std<generator__, hamt::hash_trie<t__>>();

    using def_memory = immer::default_memory_policy;

    benchmark_memory_exp<
        generator__,
        immer::set<t__, std::hash<t__>, std::equal_to<t__>, def_memory, 2>>();
    benchmark_memory_exp<
        generator__,
        immer::set<t__, std::hash<t__>, std::equal_to<t__>, def_memory, 3>>();
    benchmark_memory_exp<
        generator__,
        immer::set<t__, std::hash<t__>, std::equal_to<t__>, def_memory, 4>>();
    benchmark_memory_exp<
        generator__,
        immer::set<t__, std::hash<t__>, std::equal_to<t__>, def_memory, 5>>();
    benchmark_memory_exp<
        generator__,
        immer::set<t__, std::hash<t__>, std::equal_to<t__>, def_memory, 6>>();

    return 0;
}

int main_lin()
{
    benchmark_memory_lin_std<generator__, std::map<t__, unsigned>>();
    benchmark_memory_lin_std<generator__, std::unordered_map<t__, unsigned>>();

    // too slow, why?
    // benchmark_memory_lin_std<generator__, boost::container::flat_map<t__>>();

    // very bad... just ignore...
    // benchmark_memory_lin_std<generator__, hamt::hash_trie<t__>>();

    using def_memory = immer::default_memory_policy;

    benchmark_memory_lin<generator__,
                         immer::map<t__,
                                    unsigned,
                                    std::hash<t__>,
                                    std::equal_to<t__>,
                                    def_memory,
                                    2>>();
    benchmark_memory_lin<generator__,
                         immer::map<t__,
                                    unsigned,
                                    std::hash<t__>,
                                    std::equal_to<t__>,
                                    def_memory,
                                    3>>();
    benchmark_memory_lin<generator__,
                         immer::map<t__,
                                    unsigned,
                                    std::hash<t__>,
                                    std::equal_to<t__>,
                                    def_memory,
                                    4>>();
    benchmark_memory_lin<generator__,
                         immer::map<t__,
                                    unsigned,
                                    std::hash<t__>,
                                    std::equal_to<t__>,
                                    def_memory,
                                    5>>();
    benchmark_memory_lin<generator__,
                         immer::map<t__,
                                    unsigned,
                                    std::hash<t__>,
                                    std::equal_to<t__>,
                                    def_memory,
                                    6>>();

    return 0;
}
