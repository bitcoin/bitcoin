//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#include "input.hpp"

#include "extra/fuzzer/fuzzer_gc_guard.hpp"

#include <immer/heap/gc_heap.hpp>
#include <immer/refcount/no_refcount_policy.hpp>
#include <immer/set.hpp>

#include <immer/algorithm.hpp>

#include <catch.hpp>

#define IMMER_FUZZED_TRACE_ENABLE 0

#if IMMER_FUZZED_TRACE_ENABLE
#include <fmt/printf.h>
#define IMMER_FUZZED_TRACE(...) fmt::print(std::cerr, __VA_ARGS__)
#else
#define IMMER_FUZZED_TRACE(...)
#endif

using gc_memory = immer::memory_policy<immer::heap_policy<immer::gc_heap>,
                                       immer::no_refcount_policy,
                                       immer::default_lock_policy,
                                       immer::gc_transience_policy,
                                       false>;

struct colliding_hash_t
{
    std::size_t operator()(std::size_t x) const { return x & ~15; }
};

namespace {

int run_input(const std::uint8_t* data, std::size_t size)
{
    auto guard = fuzzer_gc_guard{};

    constexpr auto var_count = 4;

    using set_t =
        immer::set<size_t, colliding_hash_t, std::equal_to<>, gc_memory>;

    auto vars = std::array<set_t, var_count>{};

#if IMMER_FUZZED_TRACE_ENABLE
    IMMER_FUZZED_TRACE("/// new test run\n");
    IMMER_FUZZED_TRACE("using set_t = immer::set<size_t, colliding_hash_t, "
                       "std::equal_to<>, gc_memory>;\n");
    for (auto i = 0u; i < var_count; ++i)
        IMMER_FUZZED_TRACE("auto v{} = set_t{{}};\n", i);
#endif

    auto is_valid_var = [&](auto idx) { return idx >= 0 && idx < var_count; };

    return fuzzer_input{data, size}.run([&](auto& in) {
        enum ops
        {
            op_insert,
            op_erase,
            op_insert_move,
            op_erase_move,
            op_iterate
        };
        auto src = read<char>(in, is_valid_var);
        auto dst = read<char>(in, is_valid_var);
        switch (read<char>(in)) {
        case op_insert: {
            auto value = read<size_t>(in);
            IMMER_FUZZED_TRACE("v{} = v{}.insert({}ul);\n", +dst, +src, +value);
            vars[dst] = vars[src].insert(value);
            break;
        }
        case op_erase: {
            auto value = read<size_t>(in);
            IMMER_FUZZED_TRACE("v{} = v{}.erase({}ul);\n", +dst, +src, +value);
            vars[dst] = vars[src].erase(value);
            break;
        }
        case op_insert_move: {
            auto value = read<size_t>(in);
            IMMER_FUZZED_TRACE(
                "v{} = std::move(v{}).insert({}ul);\n", +dst, +src, +value);
            vars[dst] = std::move(vars[src]).insert(value);
            break;
        }
        case op_erase_move: {
            auto value = read<size_t>(in);
            IMMER_FUZZED_TRACE(
                "v{} = std::move(v{}).erase({}ul);\n", +dst, +src, +value);
            vars[dst] = std::move(vars[src]).erase(value);
            break;
        }
        case op_iterate: {
            auto srcv = vars[src];
            IMMER_FUZZED_TRACE("{{ auto v = v{}; for (const auto& x : v) v{} = "
                               "std::move(v{}).insert(x); }}\n",
                               +src,
                               +dst,
                               +dst);
            for (auto&& v : srcv) {
                vars[dst] = vars[dst].insert(v);
            }
            break;
        }
        default:
            break;
        };
        return true;
    });
}

} // namespace

TEST_CASE("https://bugs.chromium.org/p/oss-fuzz/issues/detail?id=24374")
{
    SECTION("fuzzer")
    {
        auto input = load_input(
            "clusterfuzz-testcase-minimized-set-gc-5193673156067328");
        CHECK(run_input(input.data(), input.size()) == 0);
    }
}
