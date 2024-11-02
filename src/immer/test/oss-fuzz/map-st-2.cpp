//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#include "input.hpp"

#include "extra/fuzzer/fuzzer_gc_guard.hpp"

#include <immer/algorithm.hpp>
#include <immer/heap/gc_heap.hpp>
#include <immer/map.hpp>

#include <catch2/catch.hpp>

#define IMMER_FUZZED_TRACE_ENABLE 0

#if IMMER_FUZZED_TRACE_ENABLE
#include <fmt/ostream.h>
#define IMMER_FUZZED_TRACE(...) fmt::print(std::cerr, __VA_ARGS__)
#else
#define IMMER_FUZZED_TRACE(...)
#endif

using st_memory = immer::memory_policy<immer::heap_policy<immer::cpp_heap>,
                                       immer::unsafe_refcount_policy,
                                       immer::no_lock_policy,
                                       immer::no_transience_policy,
                                       false>;

struct colliding_hash_t
{
    std::size_t operator()(std::size_t x) const { return x & ~15; }
};

namespace {

int run_input(const std::uint8_t* data, std::size_t size)
{
    constexpr auto var_count = 4;

    using map_t = immer::
        map<std::size_t, int, colliding_hash_t, std::equal_to<>, st_memory>;

    auto vars = std::array<map_t, var_count>{};

#if IMMER_FUZZED_TRACE_ENABLE
    IMMER_FUZZED_TRACE("/// new test run\n");
    IMMER_FUZZED_TRACE("using map_t = immer::map<std::size_t, int, "
                       "colliding_hash_t, std::equal_to<>,"
                       "immer::default_memory_policy, {}>;\n",
                       immer::default_bits);
    for (auto i = 0u; i < var_count; ++i)
        IMMER_FUZZED_TRACE("auto v{} = map_t{{}};\n", i);
#endif

    auto is_valid_var = [&](auto idx) { return idx >= 0 && idx < var_count; };

    return fuzzer_input{data, size}.run([&](auto& in) {
        enum ops
        {
            op_set,
            op_erase,
            op_set_move,
            op_erase_move,
            op_iterate,
            op_find,
            op_update,
            op_update_move,
            op_diff
        };
        auto src = read<char>(in, is_valid_var);
        auto dst = read<char>(in, is_valid_var);
        switch (read<char>(in)) {
        case op_set: {
            auto value = read<size_t>(in);
            IMMER_FUZZED_TRACE("v{} = v{}.set({}, 42);\n", +dst, +src, +value);
            vars[dst] = vars[src].set(value, 42);
            break;
        }
        case op_erase: {
            auto value = read<size_t>(in);
            IMMER_FUZZED_TRACE("v{} = v{}.erase({});\n", +dst, +src, +value);
            vars[dst] = vars[src].erase(value);
            break;
        }
        case op_set_move: {
            auto value = read<size_t>(in);
            IMMER_FUZZED_TRACE(
                "v{} = std::move(v{}).set({}, 42);\n", +dst, +src, +value);
            vars[dst] = std::move(vars[src]).set(value, 42);
            break;
        }
        case op_erase_move: {
            auto value = read<size_t>(in);
            IMMER_FUZZED_TRACE(
                "v{} = std::move(v{}).erase({});\n", +dst, +src, +value);
            vars[dst] = std::move(vars[src]).erase(value);
            break;
        }
        case op_iterate: {
            auto srcv = vars[src];
            IMMER_FUZZED_TRACE("{auto srcv = {}; for (const auto& v : srcv) "
                               "v{} = v{}.set(v.first, v.second); }\n",
                               +dst,
                               +src);
            for (const auto& v : srcv) {
                vars[dst] = vars[dst].set(v.first, v.second);
            }
            break;
        }
        case op_find: {
            auto value = read<size_t>(in);
            auto res   = vars[src].find(value);
            IMMER_FUZZED_TRACE("if (auto res = v{}.find({}); res) v{} = "
                               "v{}.set(*res, 42);\n",
                               +src,
                               +value,
                               +dst,
                               +dst);
            if (res != nullptr) {
                vars[dst] = vars[dst].set(*res, 42);
            }
            break;
        }
        case op_update: {
            auto key = read<size_t>(in);
            IMMER_FUZZED_TRACE("v{} = v{}.update(key, [](int x) { "
                               "return x + 1; });\n",
                               +dst,
                               +src,
                               +key);
            vars[dst] = vars[src].update(key, [](int x) { return x + 1; });
            break;
        }
        case op_update_move: {
            auto key = read<size_t>(in);
            IMMER_FUZZED_TRACE("v{} = std::move(v{}).update(key, [](int x) { "
                               "return x + 1; });\n",
                               +dst,
                               +src,
                               +key);
            vars[dst] =
                std::move(vars[src]).update(key, [](int x) { return x + 1; });
            break;
        }
        case op_diff: {
            auto&& a = vars[src];
            auto&& b = vars[dst];
            diff(
                a,
                b,
                [&](auto&& x) {
                    assert(!a.count(x.first));
                    assert(b.count(x.first));
                },
                [&](auto&& x) {
                    assert(a.count(x.first));
                    assert(!b.count(x.first));
                },
                [&](auto&& x, auto&& y) {
                    assert(x.first == y.first);
                    assert(x.second != y.second);
                });
        }
        default:
            break;
        };
        return true;
    });
}

} // namespace

TEST_CASE("https://bugs.chromium.org/p/oss-fuzz/issues/detail?id=48398")
{
    SECTION("fuzzer")
    {
        auto input = load_input(
            "clusterfuzz-testcase-minimized-map-st-6242663155761152");
        CHECK(run_input(input.data(), input.size()) == 0);
    }

    SECTION("translated")
    {
        using map_t = immer::map<std::size_t,
                                 int,
                                 colliding_hash_t,
                                 std::equal_to<>,
                                 immer::default_memory_policy,
                                 5>;
        auto v0     = map_t{};
        auto v1     = map_t{};
        v0          = v0.set(0, 42);
        v1          = v0;
        v0          = std::move(v1).erase(0);
    }
}
