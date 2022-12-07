//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#pragma once

#include "benchmark/vector/common.hpp"

namespace {

template <typename Vektor>
auto bechmark_push_front()
{
    return [] (nonius::chronometer meter)
    {
        auto n = meter.param<N>();

        measure(meter, [&] {
            auto v = Vektor{};
            for (auto i = 0u; i < n; ++i)
                v = v.push_front(i);
            return v;
        });
    };
}

auto benchmark_push_front_librrb(nonius::chronometer meter)
{
    auto n = meter.param<N>();

    measure(meter, [&] {
        auto v = rrb_create();
        for (auto i = 0u; i < n; ++i) {
            auto f = rrb_push(rrb_create(),
                              reinterpret_cast<void*>(i));
            v = rrb_concat(f, v);
        }
        return v;
    });
}

} // anonymous namespace
