//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#include "fuzzer_input.hpp"

#include <immer/box.hpp>
#include <immer/heap/gc_heap.hpp>
#include <immer/refcount/no_refcount_policy.hpp>
#include <immer/set.hpp>

#include <immer/algorithm.hpp>

#include <array>

using st_memory = immer::memory_policy<immer::heap_policy<immer::cpp_heap>,
                                       immer::unsafe_refcount_policy,
                                       immer::no_lock_policy,
                                       immer::no_transience_policy,
                                       false>;

struct colliding_hash_t
{
    std::size_t operator()(const std::string& x) const
    {
        return std::hash<std::string>{}(x) & ~((std::size_t{1} << 48) - 1);
    }
};

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data,
                                      std::size_t size)
{
    constexpr auto var_count = 4;

    using set_t = immer::
        set<std::string, colliding_hash_t, std::equal_to<>, st_memory, 3>;

    auto vars = std::array<set_t, var_count>{};

    auto is_valid_var = [&](auto idx) { return idx >= 0 && idx < var_count; };

    return fuzzer_input{data, size}.run([&](auto& in) {
        enum ops
        {
            op_insert,
            op_erase,
            op_insert_move,
            op_erase_move,
            op_iterate,
            op_diff
        };
        auto src = read<char>(in, is_valid_var);
        auto dst = read<char>(in, is_valid_var);
        assert(vars[src].impl().check_champ());
        switch (read<char>(in)) {
        case op_insert: {
            auto value = std::to_string(read<size_t>(in));
            vars[dst]  = vars[src].insert(value);
            break;
        }
        case op_erase: {
            auto value = std::to_string(read<size_t>(in));
            vars[dst]  = vars[src].erase(value);
            break;
        }
        case op_insert_move: {
            auto value = std::to_string(read<size_t>(in));
            vars[dst]  = std::move(vars[src]).insert(value);
            break;
        }
        case op_erase_move: {
            auto value = std::to_string(read<size_t>(in));
            vars[dst]  = std::move(vars[src]).erase(value);
            break;
        }
        case op_iterate: {
            auto srcv = vars[src];
            for (const auto& v : srcv) {
                vars[dst] = vars[dst].insert(v);
            }
            break;
        }
        case op_diff: {
            auto&& a = vars[src];
            auto&& b = vars[dst];
            diff(
                a,
                b,
                [&](auto&& x) {
                    assert(!a.count(x));
                    assert(b.count(x));
                },
                [&](auto&& x) {
                    assert(a.count(x));
                    assert(!b.count(x));
                },
                [&](auto&& x, auto&& y) { assert(false); });
        }
        default:
            break;
        };
        return true;
    });
}
