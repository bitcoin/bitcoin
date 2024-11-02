//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#pragma once

#include "benchmark/config.hpp"

#include <boost/container/flat_set.hpp>
#include <hash_trie.hpp> // Phil Nash
#include <immer/set.hpp>
#include <immer/set_transient.hpp>
#include <set>
#include <unordered_set>

namespace {

template <typename Generator, typename Set>
auto benchmark_erase_mut_std()
{
    return [](nonius::chronometer meter) {
        auto n  = meter.param<N>();
        auto g  = Generator{}(n);
        auto v_ = [&] {
            auto v = Set{};
            for (auto i = 0u; i < n; ++i)
                v.insert(g[i]);
            return v;
        }();
        measure(meter, [&] {
            auto v = v_;
            for (auto i = 0u; i < n; ++i)
                v.erase(g[i]);
            return v;
        });
    };
}

template <typename Generator, typename Set>
auto benchmark_erase()
{
    return [](nonius::chronometer meter) {
        auto n  = meter.param<N>();
        auto g  = Generator{}(n);
        auto v_ = [&] {
            auto v = Set{}.transient();
            for (auto i = 0u; i < n; ++i)
                v.insert(g[i]);
            return v.persistent();
        }();
        measure(meter, [&] {
            auto v = v_;
            for (auto i = 0u; i < n; ++i)
                v = v.erase(g[i]);
            return v;
        });
    };
}

template <typename Generator, typename Set>
auto benchmark_erase_move()
{
    return [](nonius::chronometer meter) {
        auto n  = meter.param<N>();
        auto g  = Generator{}(n);
        auto v_ = [&] {
            auto v = Set{}.transient();
            for (auto i = 0u; i < n; ++i)
                v.insert(g[i]);
            return v.persistent();
        }();
        measure(meter, [&] {
            auto v = v_;
            for (auto i = 0u; i < n; ++i)
                v = std::move(v).erase(g[i]);
            return v;
        });
    };
}

} // namespace
