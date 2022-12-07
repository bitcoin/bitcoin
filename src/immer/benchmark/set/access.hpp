//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#pragma once

#include "benchmark/config.hpp"

#include <immer/set.hpp>
#include <hash_trie.hpp> // Phil Nash
#include <boost/container/flat_set.hpp>
#include <set>
#include <unordered_set>

namespace {

template <typename T=unsigned>
auto make_generator_ranged(std::size_t runs)
{
    assert(runs > 0);
    auto engine = std::default_random_engine{13};
    auto dist = std::uniform_int_distribution<T>{0, (T)runs-1};
    auto r = std::vector<T>(runs);
    std::generate_n(r.begin(), runs, std::bind(dist, engine));
    return r;
}

template <typename Generator, typename Set>
auto benchmark_access_std()
{
    return [] (nonius::chronometer meter)
    {
        auto n  = meter.param<N>();
        auto g1 = Generator{}(n);
        auto g2 = make_generator_ranged(n);

        auto v = Set{};
        for (auto i = 0u; i < n; ++i)
            v.insert(g1[i]);

        measure(meter, [&] {
            auto c = 0u;
            for (auto i = 0u; i < n; ++i)
                c += v.count(g1[g2[i]]);
            volatile auto r = c;
            return r;
        });
    };
}

template <typename Generator, typename Set>
auto benchmark_access_hamt()
{
    return [] (nonius::chronometer meter)
    {
        auto n = meter.param<N>();
        auto g1 = Generator{}(n);
        auto g2 = make_generator_ranged(n);

        auto v = Set{};
        for (auto i = 0u; i < n; ++i)
            v.insert(g1[i]);

        measure(meter, [&] {
            auto c = 0u;
            for (auto i = 0u; i < n; ++i) {
                auto& x = g1[g2[i]];
                auto leaf = v.find(x).leaf();
                c += !!(leaf && leaf->find(x));
            }
            volatile auto r = c;
            return r;
        });
    };
}


template <typename Generator, typename Set>
auto benchmark_access()
{
    return [] (nonius::chronometer meter)
    {
        auto n = meter.param<N>();
        auto g1 = Generator{}(n);
        auto g2 = make_generator_ranged(n);

        auto v = Set{};
        for (auto i = 0u; i < n; ++i)
            v = v.insert(g1[i]);

        measure(meter, [&] {
            auto c = 0u;
            for (auto i = 0u; i < n; ++i)
                c += v.count(g1[g2[i]]);
            volatile auto r = c;
            return r;
        });
    };
}

template <typename Generator, typename Set>
auto benchmark_bad_access_std()
{
    return [] (nonius::chronometer meter)
    {
        auto n = meter.param<N>();
        auto g1 = Generator{}(n*2);

        auto v = Set{};
        for (auto i = 0u; i < n; ++i)
            v.insert(g1[i]);

        measure(meter, [&] {
            auto c = 0u;
            for (auto i = 0u; i < n; ++i)
                c += v.count(g1[n+i]);
            volatile auto r = c;
            return r;
        });
    };
}

template <typename Generator, typename Set>
auto benchmark_bad_access_hamt()
{
    return [] (nonius::chronometer meter)
    {
        auto n = meter.param<N>();
        auto g1 = Generator{}(n*2);

        auto v = Set{};
        for (auto i = 0u; i < n; ++i)
            v.insert(g1[i]);

        measure(meter, [&] {
            auto c = 0u;
            for (auto i = 0u; i < n; ++i) {
                auto& x = g1[n+i];
                auto leaf = v.find(x).leaf();
                c += !!(leaf && leaf->find(x));
            }
            volatile auto r = c;
            return r;
        });
    };
}


template <typename Generator, typename Set>
auto benchmark_bad_access()
{
    return [] (nonius::chronometer meter)
    {
        auto n = meter.param<N>();
        auto g1 = Generator{}(n*2);

        auto v = Set{};
        for (auto i = 0u; i < n; ++i)
            v = v.insert(g1[i]);

        measure(meter, [&] {
            auto c = 0u;
            for (auto i = 0u; i < n; ++i)
                c += v.count(g1[n+i]);
            volatile auto r = c;
            return r;
        });
    };
}

} // namespace
