//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#include "input.hpp"

#include <immer/box.hpp>
#include <immer/flex_vector.hpp>
#include <immer/flex_vector_transient.hpp>

#include <catch2/catch.hpp>

#define IMMER_FUZZED_TRACE_ENABLE 1

#if IMMER_FUZZED_TRACE_ENABLE
#include <fmt/ostream.h>
#define IMMER_FUZZED_TRACE(...) fmt::print(std::cerr, __VA_ARGS__)
#else
#define IMMER_FUZZED_TRACE(...)
#endif

using bo_memory = immer::memory_policy<immer::heap_policy<immer::cpp_heap>,
                                       immer::unsafe_refcount_policy,
                                       immer::no_lock_policy,
                                       immer::no_transience_policy,
                                       true>;

namespace {

int run_input(const std::uint8_t* data, std::size_t size)
{
    constexpr auto var_count = 8;
    constexpr auto bits      = 3;

    using vector_t = immer::flex_vector<int, bo_memory, bits, bits>;
    using size_t   = std::uint8_t;

    auto vars = std::array<vector_t, var_count>{};

#if IMMER_FUZZED_TRACE_ENABLE
    IMMER_FUZZED_TRACE("/// new test run\n");
    IMMER_FUZZED_TRACE(
        "using vector_t = immer::flex_vector<int, bo_memory, {}, {}>;\n",
        bits,
        bits);
    for (auto i = 0u; i < var_count; ++i)
        IMMER_FUZZED_TRACE("auto v{} = vector_t{{}};\n", i);
#endif

    auto is_valid_var = [&](auto idx) { return idx >= 0 && idx < var_count; };
    auto is_valid_var_neq = [](auto other) {
        return [=](auto idx) {
            return idx >= 0 && idx < var_count && idx != other;
        };
    };
    auto is_valid_index = [](auto& v) {
        return [&](auto idx) { return idx >= 0 && idx < v.size(); };
    };
    auto is_valid_size = [](auto& v) {
        return [&](auto idx) { return idx >= 0 && idx <= v.size(); };
    };
    auto can_concat = [](auto&& v1, auto&& v2) {
        return v1.size() + v2.size() < vector_t::max_size();
    };
    auto can_compare = [](auto&& v) {
        // avoid comparing vectors that are too big, and hence, slow to compare
        return v.size() < (1 << 15);
    };
    return fuzzer_input{data, size}.run([&](auto& in) {
        enum ops
        {
            op_push_back,
            op_update,
            op_take,
            op_drop,
            op_concat,
            op_push_back_move,
            op_update_move,
            op_take_move,
            op_drop_move,
            op_concat_move_l,
            op_concat_move_r,
            op_concat_move_lr,
            op_insert,
            op_erase,
            op_compare,
        };
        auto src = read<char>(in, is_valid_var);
        auto dst = read<char>(in, is_valid_var);
        switch (read<char>(in)) {
        case op_push_back: {
            IMMER_FUZZED_TRACE("v{} = v{}.push_back(42);\n", +dst, +src);
            vars[dst] = vars[src].push_back(42);
            break;
        }
        case op_update: {
            auto idx = read<size_t>(in, is_valid_index(vars[src]));
            IMMER_FUZZED_TRACE(
                "v{} = v{}.update({}, [] (auto x) {{ return x + 1; }});\n",
                +dst,
                +src,
                idx);
            vars[dst] = vars[src].update(idx, [](auto x) { return x + 1; });
            break;
        }
        case op_take: {
            auto idx = read<size_t>(in, is_valid_size(vars[src]));
            IMMER_FUZZED_TRACE("v{} = v{}.take({});\n", +dst, +src, idx);
            vars[dst] = vars[src].take(idx);
            break;
        }
        case op_drop: {
            auto idx = read<size_t>(in, is_valid_size(vars[src]));
            IMMER_FUZZED_TRACE("v{} = v{}.drop({});\n", +dst, +src, idx);
            vars[dst] = vars[src].drop(idx);
            break;
        }
        case op_concat: {
            auto src2 = read<char>(in, is_valid_var);
            if (can_concat(vars[src], vars[src2])) {
                IMMER_FUZZED_TRACE("v{} = v{} + v{};\n", +dst, +src, +src2);
                vars[dst] = vars[src] + vars[src2];
            }
            break;
        }
        case op_push_back_move: {
            IMMER_FUZZED_TRACE(
                "v{} = std::move(v{}).push_back(21);\n", +dst, +src);
            vars[dst] = std::move(vars[src]).push_back(21);
            break;
        }
        case op_update_move: {
            auto idx = read<size_t>(in, is_valid_index(vars[src]));
            IMMER_FUZZED_TRACE("v{} = std::move(v{}).update({}, [] (auto x) {{ "
                               "return x + 1; }});\n",
                               +dst,
                               +src,
                               idx);
            vars[dst] =
                std::move(vars[src]).update(idx, [](auto x) { return x + 1; });
            break;
        }
        case op_take_move: {
            auto idx = read<size_t>(in, is_valid_size(vars[src]));
            IMMER_FUZZED_TRACE(
                "v{} = std::move(v{}).take({});\n", +dst, +src, idx);
            vars[dst] = std::move(vars[src]).take(idx);
            break;
        }
        case op_drop_move: {
            auto idx = read<size_t>(in, is_valid_size(vars[src]));
            IMMER_FUZZED_TRACE(
                "v{} = std::move(v{}).drop({});\n", +dst, +src, idx);
            vars[dst] = std::move(vars[src]).drop(idx);
            break;
        }
        case op_concat_move_l: {
            auto src2 = read<char>(in, is_valid_var_neq(src));
            if (can_concat(vars[src], vars[src2])) {
                IMMER_FUZZED_TRACE(
                    "v{} = std::move(v{}) + v{};\n", +dst, +src, +src2);
                vars[dst] = std::move(vars[src]) + vars[src2];
            }
            break;
        }
        case op_concat_move_r: {
            auto src2 = read<char>(in, is_valid_var_neq(src));
            if (can_concat(vars[src], vars[src2])) {
                IMMER_FUZZED_TRACE(
                    "v{} = v{} + std::move(v{});\n", +dst, +src, +src2);
                vars[dst] = vars[src] + std::move(vars[src2]);
            }
            break;
        }
        case op_concat_move_lr: {
            auto src2 = read<char>(in, is_valid_var_neq(src));
            if (can_concat(vars[src], vars[src2])) {
                IMMER_FUZZED_TRACE("v{} = std::move(v{}) + std::move(v{});\n",
                                   +dst,
                                   +src,
                                   +src2);
                vars[dst] = std::move(vars[src]) + std::move(vars[src2]);
            }
            break;
        }
        case op_compare: {
            using std::swap;
            if (can_compare(vars[src])) {
                IMMER_FUZZED_TRACE("if (v{} == v{}) swap(v{}, v{});\n",
                                   +src,
                                   +dst,
                                   +src,
                                   +dst);
                if (vars[src] == vars[dst])
                    swap(vars[src], vars[dst]);
            }
            break;
        }
        case op_erase: {
            auto idx = read<size_t>(in, is_valid_index(vars[src]));
            IMMER_FUZZED_TRACE("v{} = v{}.erase({});\n", +dst, +src, idx);
            vars[dst] = vars[src].erase(idx);
            break;
        }
        case op_insert: {
            auto idx = read<size_t>(in, is_valid_size(vars[src]));
            IMMER_FUZZED_TRACE("v{} = v{}.insert({}, immer::box<int>{{42}});\n",
                               +dst,
                               +src,
                               idx);
            vars[dst] = vars[src].insert(idx, immer::box<int>{42});
            break;
        }
        default:
            break;
        };
        return true;
    });
}

} // namespace

TEST_CASE("https://bugs.chromium.org/p/oss-fuzz/issues/detail?id=24366")
{
    SECTION("trace, simplified")
    {
        using vector_t = immer::flex_vector<int, bo_memory, 3, 3>;
        auto v0        = vector_t{{42, 42, 42, 42}};
        auto v1        = v0 + v0;
        v0             = v0 + v1;
        v0             = v0 + v1;
        v0             = v0 + v1;
        v0             = v0 + v1;
        v0             = v0 + v1;
        v0             = v0 + v1;
        v0             = v0 + v1;
        v0             = v0 + v1;
        v0             = v0 + v1;
        v0             = std::move(v0).drop(71);
    }

    SECTION("fuzzer")
    {
        auto input = load_input(
            "clusterfuzz-testcase-minimized-flex-vector-bo-6038320384311296");
        CHECK(run_input(input.data(), input.size()) == 0);
    }
}
