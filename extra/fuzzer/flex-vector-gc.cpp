//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#include "fuzzer_input.hpp"

#include "extra/fuzzer/fuzzer_gc_guard.hpp"

#include <immer/flex_vector.hpp>
#include <immer/flex_vector_transient.hpp>
#include <immer/heap/gc_heap.hpp>
#include <immer/refcount/no_refcount_policy.hpp>

#include <array>

using gc_memory = immer::memory_policy<immer::heap_policy<immer::gc_heap>,
                                       immer::no_refcount_policy,
                                       immer::default_lock_policy,
                                       immer::gc_transience_policy,
                                       false>;

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data,
                                      std::size_t size)
{
    auto guard = fuzzer_gc_guard{};

    constexpr auto var_count = 4;
    constexpr auto bits      = 2;

    using vector_t    = immer::flex_vector<int, gc_memory, bits, bits>;
    using transient_t = typename vector_t::transient_type;
    using size_t      = std::uint8_t;

    auto vs = std::array<vector_t, var_count>{};
    auto ts = std::array<transient_t, var_count>{};

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

    return fuzzer_input{data, size}.run([&](auto& in) {
        enum ops
        {
            op_transient,
            op_persistent,
            op_push_back,
            op_update,
            op_take,
            op_drop,
            op_concat,
            op_push_back_mut,
            op_update_mut,
            op_take_mut,
            op_drop_mut,
            op_prepend_mut,
            op_prepend_mut_move,
            op_append_mut,
            op_append_mut_move,
        };
        auto dst = read<char>(in, is_valid_var);
        switch (read<char>(in)) {
        case op_transient: {
            auto src = read<char>(in, is_valid_var);
            ts[dst]  = vs[src].transient();
            break;
        }
        case op_persistent: {
            auto src = read<char>(in, is_valid_var);
            vs[dst]  = ts[src].persistent();
            break;
        }
        case op_push_back: {
            auto src = read<char>(in, is_valid_var);
            vs[dst]  = vs[src].push_back(42);
            break;
        }
        case op_update: {
            auto src = read<char>(in, is_valid_var);
            auto idx = read<size_t>(in, is_valid_index(vs[src]));
            vs[dst]  = vs[src].update(idx, [](auto x) { return x + 1; });
            break;
        }
        case op_take: {
            auto src = read<char>(in, is_valid_var);
            auto idx = read<size_t>(in, is_valid_size(vs[src]));
            vs[dst]  = vs[src].take(idx);
            break;
        }
        case op_drop: {
            auto src = read<char>(in, is_valid_var);
            auto idx = read<size_t>(in, is_valid_size(vs[src]));
            vs[dst]  = vs[src].drop(idx);
            break;
        }
        case op_concat: {
            auto src  = read<char>(in, is_valid_var);
            auto src2 = read<char>(in, is_valid_var);
            if (can_concat(vs[src], vs[src2]))
                vs[dst] = vs[src] + vs[src2];
            break;
        }
        case op_push_back_mut: {
            ts[dst].push_back(13);
            break;
        }
        case op_update_mut: {
            auto idx = read<size_t>(in, is_valid_index(ts[dst]));
            ts[dst].update(idx, [](auto x) { return x + 1; });
            break;
        }
        case op_take_mut: {
            auto idx = read<size_t>(in, is_valid_size(ts[dst]));
            ts[dst].take(idx);
            break;
        }
        case op_prepend_mut: {
            auto src = read<char>(in, is_valid_var_neq(dst));
            if (can_concat(ts[dst], ts[src]))
                ts[dst].prepend(ts[src]);
            break;
        }
        case op_prepend_mut_move: {
            auto src = read<char>(in, is_valid_var_neq(dst));
            if (can_concat(ts[dst], ts[src])) {
                ts[dst].prepend(std::move(ts[src]));
                ts[src] = {};
            }
            break;
        }
        case op_append_mut: {
            auto src = read<char>(in, is_valid_var_neq(dst));
            if (can_concat(ts[dst], ts[src]))
                ts[dst].append(ts[src]);
            break;
        }
        case op_append_mut_move: {
            auto src = read<char>(in, is_valid_var_neq(dst));
            if (can_concat(ts[dst], ts[src])) {
                ts[dst].append(std::move(ts[src]));
                ts[src] = {};
            }
            break;
        }
        default:
            break;
        };
        return true;
    });
}
