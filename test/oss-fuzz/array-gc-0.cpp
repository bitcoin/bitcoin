//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#include "input.hpp"

#include "extra/fuzzer/fuzzer_gc_guard.hpp"

#include <immer/array.hpp>
#include <immer/array_transient.hpp>
#include <immer/heap/gc_heap.hpp>
#include <immer/refcount/no_refcount_policy.hpp>

#include <catch2/catch.hpp>

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

    using array_t     = immer::array<int, gc_memory>;
    using transient_t = typename array_t::transient_type;
    using size_t      = std::uint8_t;

    auto vs = std::array<array_t, var_count>{};
    auto ts = std::array<transient_t, var_count>{};

    auto is_valid_var   = [&](auto idx) { return idx >= 0 && idx < var_count; };
    auto is_valid_index = [](auto& v) {
        return [&](auto idx) { return idx >= 0 && idx < v.size(); };
    };
    auto is_valid_size = [](auto& v) {
        return [&](auto idx) { return idx >= 0 && idx <= v.size(); };
    };
    // limit doing immutable pushes on vectors that are too big already to
    // prevent timeouts
    auto too_big = [](auto&& v) { return v.size() > (std::size_t{1} << 10); };
    return fuzzer_input{data, size}.run([&](auto& in) {
        enum ops
        {
            op_transient,
            op_persistent,
            op_push_back,
            op_update,
            op_take,
            op_push_back_mut,
            op_update_mut,
            op_take_mut,
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
            if (!too_big(vs[src]))
                vs[dst] = vs[src].push_back(42);
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
        case op_push_back_mut: {
            if (!too_big(vs[dst]))
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
        default:
            break;
        };
        return true;
    });
}

} // namespace

TEST_CASE("https://bugs.chromium.org/p/oss-fuzz/issues/detail?id=24244")
{
    SECTION("fuzzer")
    {
        auto input = load_input(
            "clusterfuzz-testcase-minimized-array-gc-5983642523009024");
        CHECK(run_input(input.data(), input.size()) == 0);
    }
}
