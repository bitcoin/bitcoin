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
auto benchmark_push_mut_std()
{
    return [] (nonius::chronometer meter)
    {
        auto n = meter.param<N>();
        if (n > get_limit<Vektor>{})
            nonius::skip();

        measure(meter, [&] {
            auto v = Vektor{};
            for (auto i = 0u; i < n; ++i)
                v.push_back(i);
            return v;
        });
    };
}

template <typename Vektor>
auto benchmark_push_mut()
{
    return [] (nonius::chronometer meter)
    {
        auto n = meter.param<N>();
        if (n > get_limit<Vektor>{})
            nonius::skip();

        measure(meter, [&] {
            auto v = Vektor{}.transient();
            for (auto i = 0u; i < n; ++i)
                v.push_back(i);
            return v;
        });
    };
}

template <typename Vektor>
auto benchmark_push_move()
{
    return [] (nonius::chronometer meter)
    {
        auto n = meter.param<N>();
        if (n > get_limit<Vektor>{})
            nonius::skip();

        measure(meter, [&] {
            auto v = Vektor{};
            for (auto i = 0u; i < n; ++i)
                v = std::move(v).push_back(i);
            return v;
        });
    };
}

template <typename Vektor>
auto benchmark_push()
{
    return [] (nonius::chronometer meter)
    {
        auto n = meter.param<N>();
        if (n > get_limit<Vektor>{})
            nonius::skip();

        measure(meter, [&] {
            auto v = Vektor{};
            for (auto i = 0u; i < n; ++i)
                v = v.push_back(i);
            return v;
        });
    };
}

auto benchmark_push_librrb(nonius::chronometer meter)
{
    auto n = meter.param<N>();

    measure(meter, [&] {
        auto v = rrb_create();
        for (auto i = 0u; i < n; ++i)
            v = rrb_push(v, reinterpret_cast<void*>(i));
        return v;
    });
}

auto benchmark_push_mut_librrb(nonius::chronometer meter)
{
    auto n = meter.param<N>();

    measure(meter, [&] {
        auto v = rrb_to_transient(rrb_create());
        for (auto i = 0u; i < n; ++i)
            v = transient_rrb_push(v, reinterpret_cast<void*>(i));
        return v;
    });
}

} // anonymous namespace
