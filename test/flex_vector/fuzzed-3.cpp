//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#include "extra/fuzzer/fuzzer_input.hpp"
#include <array>
#include <catch.hpp>
#include <immer/flex_vector.hpp>
#include <iostream>

#define IMMER_FUZZED_TRACE_ENABLE 0

#if IMMER_FUZZED_TRACE_ENABLE
#define IMMER_FUZZED_TRACE(...) std::cout << __VA_ARGS__ << std::endl;
#else
#define IMMER_FUZZED_TRACE(...)
#endif

namespace {

template <std::size_t VarCount = 2, unsigned Bits = 2>
int run_input(const std::uint8_t* data, std::size_t size)
{
    using vector_t =
        immer::flex_vector<int, immer::default_memory_policy, Bits, Bits>;
    using size_t = std::uint8_t;

    auto vars = std::array<vector_t, VarCount>{};

#if IMMER_FUZZED_TRACE_ENABLE
    std::cout << "/// new test run" << std::endl;
    for (auto i = 0u; i < VarCount; ++i)
        std::cout << "auto var" << i << " = vector_t{};" << std::endl;
#endif

    auto is_valid_var = [&](auto idx) { return idx >= 0 && idx < VarCount; };
    auto is_valid_var_neq = [](auto other) {
        return [=](auto idx) {
            return idx >= 0 && idx < VarCount && idx != other;
        };
    };
    auto is_valid_index = [](auto& v) {
        return [&](auto idx) { return idx >= 0 && idx < v.size(); };
    };
    auto is_valid_size = [](auto& v) {
        return [&](auto idx) { return idx >= 0 && idx <= v.size(); };
    };
    auto can_concat = [](auto&& v1, auto&& v2) {
        using size_type = decltype(v1.size());
        return v2.size() < (std::numeric_limits<size_type>::max() - v1.size());
    };
    auto can_insert = [](auto&& v1) {
        using size_type = decltype(v1.size());
        return v1.size() < std::numeric_limits<size_type>::max();
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
        };
        auto src = read<std::uint8_t>(in, is_valid_var);
        auto dst = read<std::uint8_t>(in, is_valid_var);
        switch (read<char>(in)) {
        case op_push_back:
            if (can_insert(vars[src])) {
                IMMER_FUZZED_TRACE("var" << +dst << " = var" << +src
                                         << ".push_back(42);");
                vars[dst] = vars[src].push_back(42);
            }
            break;
        case op_update: {
            auto idx = read<size_t>(in, is_valid_index(vars[src]));
            IMMER_FUZZED_TRACE("var" << +dst << " = var" << +src << ".update("
                                     << +idx
                                     << ", [] (auto x) { return x + 1; });");
            vars[dst] = vars[src].update(idx, [](auto x) { return x + 1; });
            break;
        }
        case op_take: {
            auto idx = read<size_t>(in, is_valid_size(vars[src]));
            IMMER_FUZZED_TRACE("var" << +dst << " = var" << +src << ".take("
                                     << +idx << ");");
            vars[dst] = vars[src].take(idx);
            break;
        }
        case op_drop: {
            auto idx = read<size_t>(in, is_valid_size(vars[src]));
            IMMER_FUZZED_TRACE("var" << +dst << " = var" << +src << ".take("
                                     << +idx << ");");
            vars[dst] = vars[src].drop(idx);
            break;
        }
        case op_concat: {
            auto src2 = read<std::uint8_t>(in, is_valid_var);
            if (can_concat(vars[src], vars[src2])) {
                IMMER_FUZZED_TRACE("var" << +dst << " = var" << +src << " + var"
                                         << +src2 << ";");
                vars[dst] = vars[src] + vars[src2];
            }
            break;
        }
        case op_push_back_move: {
            if (can_insert(vars[src])) {
                IMMER_FUZZED_TRACE("var" << +dst << " = std::move(var" << +src
                                         << ").push_back(21);");
                vars[dst] = std::move(vars[src]).push_back(21);
            }
            break;
        }
        case op_update_move: {
            auto idx = read<size_t>(in, is_valid_index(vars[src]));
            IMMER_FUZZED_TRACE("var" << +dst << " = std::move(var" << +src
                                     << ").update(" << +idx
                                     << ", [] (auto x) { return x + 1; });");
            vars[dst] =
                std::move(vars[src]).update(idx, [](auto x) { return x + 1; });
            break;
        }
        case op_take_move: {
            auto idx = read<size_t>(in, is_valid_size(vars[src]));
            IMMER_FUZZED_TRACE("var" << +dst << " = std::move(var" << +src
                                     << ").take(" << +idx << ");");
            vars[dst] = std::move(vars[src]).take(idx);
            break;
        }
        case op_drop_move: {
            auto idx = read<size_t>(in, is_valid_size(vars[src]));
            IMMER_FUZZED_TRACE("var" << +dst << " = std::move(var" << +src
                                     << ").drop(" << +idx << ");");
            vars[dst] = std::move(vars[src]).drop(idx);
            break;
        }
        case op_concat_move_l: {
            auto src2 = read<std::uint8_t>(in, is_valid_var_neq(src));
            if (can_concat(vars[src], vars[src2])) {
                IMMER_FUZZED_TRACE("var" << +dst << " = std::move(var" << +src
                                         << ") + var" << +src2 << ";");
                vars[dst] = std::move(vars[src]) + vars[src2];
            }
            break;
        }
        case op_concat_move_r: {
            auto src2 = read<std::uint8_t>(in, is_valid_var_neq(src));
            if (can_concat(vars[src], vars[src2])) {
                IMMER_FUZZED_TRACE("var" << +dst << " = var" << +src
                                         << " + std::move(var" << +src2
                                         << ");");
                vars[dst] = vars[src] + std::move(vars[src2]);
            }
            break;
        }
        case op_concat_move_lr: {
            auto src2 = read<std::uint8_t>(in, is_valid_var_neq(src));
            if (can_concat(vars[src], vars[src2])) {
                IMMER_FUZZED_TRACE("var" << +dst << " = std::move(var" << +src
                                         << ") + std::move(var" << +src2
                                         << ");");
                vars[dst] = std::move(vars[src]) + std::move(vars[src2]);
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

TEST_CASE("bug: concat with moving the right side")
{
    // This was a stupid bug on the concatenation of small vectors
    // when moving one of the sides.
    SECTION("simplified")
    {
        using vector_t =
            immer::flex_vector<int, immer::default_memory_policy, 2, 2>;
        auto var0 = vector_t{};
        auto var1 = vector_t{};
        var0      = var0.push_back(42);
        var0      = var0.push_back(42);
        var0      = var0.push_back(42);
        var1      = var0.push_back(42);
        var0      = var0 + var0;
        var0      = var0.push_back(42);
        var0      = var0 + var1;
        var0      = var0 + var0;
        var0      = var0 + std::move(var1);
    }

#if __GNUC__ != 9 && __GNUC__ != 8
    SECTION("vm")
    {
        constexpr std::uint8_t input[] = {
            0x0,  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x0,
            0x0,  0x0,  0x0,  0x1,  0x0,  0x40, 0x28, 0x0,  0x4,  0x3f,
            0x20, 0x0,  0x0,  0x4,  0x3f, 0x8,  0x0,  0x0,  0x0,  0x0,
            0x1,  0x0,  0x0,  0x0,  0x4,  0x3f, 0x0,  0x0,  0x40, 0x0,
            0x0,  0x0,  0x0,  0x4,  0x3f, 0x1,  0x0,  0x0,  0x4,  0x3f,
            0x0,  0x0,  0xff, 0x0,  0xa,  0x1,  0x0,  0xff, 0x0,  0x0,
        };
        CHECK(run_input(input, sizeof(input)) == 0);
    }
#endif
}
