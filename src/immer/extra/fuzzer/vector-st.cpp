//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#include "fuzzer_input.hpp"

#include <immer/vector.hpp>

#include <array>

using st_memory = immer::memory_policy<immer::heap_policy<immer::cpp_heap>,
                                       immer::unsafe_refcount_policy,
                                       immer::no_lock_policy,
                                       immer::no_transience_policy,
                                       false>;

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data,
                                      std::size_t size)
{
    constexpr auto var_count = 4;
    constexpr auto bits      = 2;

    using vector_t = immer::vector<int, st_memory, bits, bits>;
    using size_t   = std::uint8_t;

    auto vars = std::array<vector_t, var_count>{};

    auto is_valid_var   = [&](auto idx) { return idx >= 0 && idx < var_count; };
    auto is_valid_index = [](auto& v) {
        return [&](auto idx) { return idx >= 0 && idx < v.size(); };
    };
    auto is_valid_size = [](auto& v) {
        return [&](auto idx) { return idx >= 0 && idx <= v.size(); };
    };

    return fuzzer_input{data, size}.run([&](auto& in) {
        enum ops
        {
            op_push_back,
            op_update,
            op_take,
            op_push_back_move,
            op_update_move,
            op_take_move,
        };
        auto src = read<char>(in, is_valid_var);
        auto dst = read<char>(in, is_valid_var);
        switch (read<char>(in)) {
        case op_push_back: {
            vars[dst] = vars[src].push_back(42);
            break;
        }
        case op_update: {
            auto idx  = read<size_t>(in, is_valid_index(vars[src]));
            vars[dst] = vars[src].update(idx, [](auto x) { return x + 1; });
            break;
        }
        case op_take: {
            auto idx  = read<size_t>(in, is_valid_size(vars[src]));
            vars[dst] = vars[src].take(idx);
            break;
        }
        case op_push_back_move: {
            vars[dst] = std::move(vars[src]).push_back(12);
            break;
        }
        case op_update_move: {
            auto idx = read<size_t>(in, is_valid_index(vars[src]));
            vars[dst] =
                std::move(vars[src]).update(idx, [](auto x) { return x + 1; });
            break;
        }
        case op_take_move: {
            auto idx  = read<size_t>(in, is_valid_size(vars[src]));
            vars[dst] = std::move(vars[src]).take(idx);
            break;
        }
        default:
            break;
        };
        return true;
    });
}
