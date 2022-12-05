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

template <typename Vektor,
          typename PushFn=push_back_fn>
auto benchmark_take()
{
    return [] (nonius::chronometer meter)
    {
        auto n = meter.param<N>();

        auto v = Vektor{};
        for (auto i = 0u; i < n; ++i)
            v = PushFn{}(std::move(v), i);

        measure(meter, [&] {
            for (auto i = 0u; i < n; ++i)
                (void) v.take(i);
        });
    };
}

template <typename Vektor,
          typename PushFn=push_back_fn>
auto benchmark_take_lin()
{
    return [] (nonius::chronometer meter)
    {
        auto n = meter.param<N>();

        auto v = Vektor{};
        for (auto i = 0u; i < n; ++i)
            v = PushFn{}(std::move(v), i);

        measure(meter, [&] {
            auto r = v;
            for (auto i = n; i > 0; --i)
                r = r.take(i);
            return r;
        });
    };
}

template <typename Vektor,
          typename PushFn=push_back_fn>
auto benchmark_take_move()
{
    return [] (nonius::chronometer meter)
    {
        auto n = meter.param<N>();

        auto v = Vektor{};
        for (auto i = 0u; i < n; ++i)
            v = PushFn{}(std::move(v), i);

        measure(meter, [&] {
            auto r = v;
            for (auto i = n; i > 0; --i)
                r = std::move(r).take(i);
            return r;
        });
    };
}

template <typename Vektor,
          typename PushFn=push_back_fn>
auto benchmark_take_mut()
{
    return [] (nonius::chronometer meter)
    {
        auto n = meter.param<N>();

        auto vv = Vektor{};
        for (auto i = 0u; i < n; ++i)
            vv = PushFn{}(std::move(vv), i);

        measure(meter, [&] {
            auto v = vv.transient();
            for (auto i = n; i > 0; --i)
                (void) v.take(i);
        });
    };
}

template <typename Fn>
auto benchmark_take_librrb(Fn make)
{
    return [=] (nonius::chronometer meter)
    {
        auto n = meter.param<N>();
        auto v = make(n);
        measure(meter, [&] {
                for (auto i = 0u; i < n; ++i)
                    rrb_slice(v, 0, i);
            });
    };
}

template <typename Fn>
auto benchmark_take_lin_librrb(Fn make)
{
    return [=] (nonius::chronometer meter)
    {
        auto n = meter.param<N>();
        auto v = make(n);
        measure(
            meter, [&] {
                auto r = v;
                for (auto i = n; i > 0; --i)
                    r = rrb_slice(r, 0, i);
                return r;
            });
    };
}

template <typename Fn>
auto benchmark_take_mut_librrb(Fn make)
{
    return [=] (nonius::chronometer meter)
    {
        auto n = meter.param<N>();
        auto v = make(n);
        measure(
            meter, [&] {
                auto r = rrb_to_transient(v);
                for (auto i = n; i > 0; --i)
                    r = transient_rrb_slice(r, 0, i);
                return r;
            });
    };
}

} // anonymous namespace
