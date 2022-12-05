//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#include <immer/detail/ref_count_base.hpp>

#include <nonius.h++>
#include <boost/intrusive_ptr.hpp>

#include <atomic>
#include <cstdlib>
#include <iterator>
#include <memory>
#include <utility>
#include <array>
#include <vector>

NONIUS_PARAM(N, std::size_t{1000})

constexpr auto benchmark_size = 32u;

struct object_t : immer::detail::ref_count_base<object_t>
{};

auto make_data()
{
    auto objs = std::array<std::unique_ptr<object_t>, benchmark_size>();
    std::generate(objs.begin(), objs.end(), [] {
        return std::make_unique<object_t>();
    });
    auto refs = std::array<object_t*, benchmark_size>();
    std::transform(objs.begin(), objs.end(), refs.begin(), [](auto& obj) {
        return obj.get();
    });
    return make_pair(std::move(objs),
                     std::move(refs));
}

NONIUS_BENCHMARK("intrusive_ptr", [] (nonius::chronometer meter)
{
    auto arr     = std::array<boost::intrusive_ptr<object_t>, benchmark_size>{};
    auto storage = std::vector<
        nonius::storage_for<
            std::array<boost::intrusive_ptr<object_t>, benchmark_size>>> (
                meter.runs());
    std::generate(arr.begin(), arr.end(), [] {
        return new object_t{};
    });
    meter.measure([&] (int i) {
        storage[i].construct(arr);
    });
})

NONIUS_BENCHMARK("generic", [] (nonius::chronometer meter)
{
    auto data = make_data();
    auto& refs = data.second;
    object_t* r[benchmark_size];

    meter.measure([&] {
        std::transform(refs.begin(), refs.end(), r, [] (auto& p) {
            if (p) p->ref_count.fetch_add(1, std::memory_order_relaxed);
            return p;
        });
        return r;
    });
})

NONIUS_BENCHMARK("manual", [] (nonius::chronometer meter)
{
    auto data = make_data();
    auto& refs = data.second;
    object_t* r[benchmark_size];

    meter.measure([&] {
        for (auto& p : refs)
            if (p) p->ref_count.fetch_add(1, std::memory_order_relaxed);
        std::copy(refs.begin(), refs.end(), r);
        return r;
    });
})

NONIUS_BENCHMARK("manual - unroll", [] (nonius::chronometer meter)
{
    auto data = make_data();
    auto& refs = data.second;
    object_t* r[benchmark_size];

    meter.measure([&] {
        auto e = refs.end();
        for (auto p = refs.begin(); p != e;) {
            (*p++)->ref_count.fetch_add(1, std::memory_order_relaxed);
            (*p++)->ref_count.fetch_add(1, std::memory_order_relaxed);
            (*p++)->ref_count.fetch_add(1, std::memory_order_relaxed);
            (*p++)->ref_count.fetch_add(1, std::memory_order_relaxed);
        }
        std::copy(refs.begin(), refs.end(), r);
        return r;
    });
})

NONIUS_BENCHMARK("manual - nocheck", [] (nonius::chronometer meter)
{
    auto data = make_data();
    auto& refs = data.second;
    object_t* r[benchmark_size];

    meter.measure([&] {
        for (auto& p : refs)
            p->ref_count.fetch_add(1, std::memory_order_relaxed);
        std::copy(refs.begin(), refs.end(), r);
        return r;
    });
})

NONIUS_BENCHMARK("manual - constant", [] (nonius::chronometer meter)
{
    auto data = make_data();
    auto& refs = data.second;
    object_t* r[benchmark_size];

    meter.measure([&] {
        for (auto i = 0u; i < benchmark_size; ++i)
            refs[i]->ref_count.fetch_add(1, std::memory_order_relaxed);
        std::copy(refs.begin(), refs.end(), r);
        return r;
    });
})

NONIUS_BENCHMARK("manual - memcopy", [] (nonius::chronometer meter)
{
    auto data = make_data();
    auto& refs = data.second;
    object_t* r[benchmark_size];

    meter.measure([&] {
        for (auto& p : refs)
            if (p) p->ref_count.fetch_add(1, std::memory_order_relaxed);
        std::memcpy(r, &refs[0], sizeof(object_t*) * benchmark_size);
        return r;
    });
})
