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

#include <catch.hpp>

using gc_memory = immer::memory_policy<immer::heap_policy<immer::gc_heap>,
                                       immer::no_refcount_policy,
                                       immer::default_lock_policy,
                                       immer::gc_transience_policy,
                                       false>;

namespace {

int run_input(const std::uint8_t* data, std::size_t size)
{
    auto guard = fuzzer_gc_guard{};

    constexpr auto var_count = 4;

    using set_t =
        immer::set<int, std::hash<char>, std::equal_to<char>, gc_memory>;

    auto vars = std::array<set_t, var_count>{};

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
            vars[dst]  = vars[src].insert(value);
            break;
        }
        case op_erase: {
            auto value = read<size_t>(in);
            vars[dst]  = vars[src].erase(value);
            break;
        }
        case op_insert_move: {
            auto value = read<size_t>(in);
            vars[dst]  = std::move(vars[src]).insert(value);
            break;
        }
        case op_erase_move: {
            auto value = read<size_t>(in);
            vars[dst]  = vars[src].erase(value);
            break;
        }
        case op_iterate: {
            auto srcv = vars[src];
            for (const auto& v : srcv) {
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

TEST_CASE("https://bugs.chromium.org/p/oss-fuzz/issues/detail?id=24170")
{
    SECTION("fuzzer")
    {
        auto input = load_input(
            "clusterfuzz-testcase-minimized-set-gc-5709797958352896");
        CHECK(run_input(input.data(), input.size()) == 0);
    }
}
