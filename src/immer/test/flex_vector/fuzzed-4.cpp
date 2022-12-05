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
#include <immer/flex_vector_transient.hpp>
#include <immer/heap/gc_heap.hpp>
#include <immer/refcount/no_refcount_policy.hpp>
#include <iostream>

#define IMMER_FUZZED_TRACE_ENABLE 0

#if IMMER_FUZZED_TRACE_ENABLE
#define IMMER_FUZZED_TRACE(...) std::cout << __VA_ARGS__ << std::endl;
#else
#define IMMER_FUZZED_TRACE(...)
#endif

using gc_memory = immer::memory_policy<immer::heap_policy<immer::gc_heap>,
                                       immer::no_refcount_policy,
                                       immer::default_lock_policy,
                                       immer::gc_transience_policy,
                                       false>;

namespace {

template <std::size_t VarCount = 4, unsigned Bits = 2>
int run_input(const std::uint8_t* data, std::size_t size)
{
    using vector_t    = immer::flex_vector<int, gc_memory, Bits, Bits>;
    using transient_t = typename vector_t::transient_type;
    using size_t      = std::uint8_t;

    auto vs = std::array<vector_t, VarCount>{};
    auto ts = std::array<transient_t, VarCount>{};

#if IMMER_FUZZED_TRACE_ENABLE
    std::cout << "/// new test run" << std::endl;
    for (auto i = 0; i < VarCount; ++i)
        std::cout << "auto v" << i << " = vector_t{};" << std::endl;
    for (auto i = 0; i < VarCount; ++i)
        std::cout << "auto t" << i << " = transient_t{};" << std::endl;
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
        auto max        = std::numeric_limits<size_type>::max() >> (Bits * 4);
        return v1.size() < max && v2.size() < max;
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
        auto dst = read<std::uint8_t>(in, is_valid_var);
        switch (read<char>(in)) {
        case op_transient: {
            auto src = read<std::uint8_t>(in, is_valid_var);
            IMMER_FUZZED_TRACE("t" << +dst << " = v" << +src
                                   << ".transient();");
            ts[dst] = vs[src].transient();
            break;
        }
        case op_persistent: {
            auto src = read<std::uint8_t>(in, is_valid_var);
            IMMER_FUZZED_TRACE("v" << +dst << " = t" << +src
                                   << ".persistent();");
            vs[dst] = ts[src].persistent();
            break;
        }
        case op_push_back: {
            auto src = read<std::uint8_t>(in, is_valid_var);
            IMMER_FUZZED_TRACE("v" << +dst << " = v" << +src
                                   << ".push_back(42);");
            vs[dst] = vs[src].push_back(42);
            break;
        }
        case op_update: {
            auto src = read<std::uint8_t>(in, is_valid_var);
            auto idx = read<size_t>(in, is_valid_index(vs[src]));
            IMMER_FUZZED_TRACE("v" << +dst << " = v" << +src << ".update("
                                   << +idx
                                   << ", [] (auto x) { return x + 1; });");
            vs[dst] = vs[src].update(idx, [](auto x) { return x + 1; });
            break;
        }
        case op_take: {
            auto src = read<std::uint8_t>(in, is_valid_var);
            auto idx = read<size_t>(in, is_valid_size(vs[src]));
            IMMER_FUZZED_TRACE("v" << +dst << " = v" << +src << ".take(" << +idx
                                   << ");");
            vs[dst] = vs[src].take(idx);
            break;
        }
        case op_drop: {
            auto src = read<std::uint8_t>(in, is_valid_var);
            auto idx = read<size_t>(in, is_valid_size(vs[src]));
            IMMER_FUZZED_TRACE("v" << +dst << " = v" << +src << ".take(" << +idx
                                   << ");");
            vs[dst] = vs[src].drop(idx);
            break;
        }
        case op_concat: {
            auto src  = read<std::uint8_t>(in, is_valid_var);
            auto src2 = read<std::uint8_t>(in, is_valid_var);
            if (can_concat(vs[src], vs[src2])) {
                IMMER_FUZZED_TRACE("v" << +dst << " = v" << +src << " + v"
                                       << +src2 << ";");
                vs[dst] = vs[src] + vs[src2];
            }
            break;
        }
        case op_push_back_mut: {
            IMMER_FUZZED_TRACE("t" << +dst << ".push_back(13);");
            ts[dst].push_back(13);
            break;
        }
        case op_update_mut: {
            auto idx = read<size_t>(in, is_valid_index(ts[dst]));
            IMMER_FUZZED_TRACE("t" << +dst << ".update(" << +idx
                                   << ", [] (auto x) { return x + 1; });");
            ts[dst].update(idx, [](auto x) { return x + 1; });
            break;
        }
        case op_take_mut: {
            auto idx = read<size_t>(in, is_valid_size(ts[dst]));
            IMMER_FUZZED_TRACE("t" << +dst << ").take(" << +idx << ");");
            ts[dst].take(idx);
            break;
        }
        case op_prepend_mut: {
            auto src = read<std::uint8_t>(in, is_valid_var_neq(dst));
            if (can_concat(ts[dst], ts[src])) {
                IMMER_FUZZED_TRACE("t" << +dst << ".prepend(t" << +src << ");");
                ts[dst].prepend(ts[src]);
            }
            break;
        }
        case op_prepend_mut_move: {
            auto src = read<std::uint8_t>(in, is_valid_var_neq(dst));
            if (can_concat(ts[dst], ts[src])) {
                IMMER_FUZZED_TRACE("t" << +dst << ".prepend(std::move(t" << +src
                                       << "));"
                                       << " t" << +src << " = {};");
                ts[dst].prepend(std::move(ts[src]));
                ts[src] = {};
            }
            break;
        }
        case op_append_mut: {
            auto src = read<std::uint8_t>(in, is_valid_var_neq(dst));
            if (can_concat(ts[dst], ts[src])) {
                IMMER_FUZZED_TRACE("t" << +dst << ".append(t" << +src << ");");
                ts[dst].append(ts[src]);
            }
            break;
        }
        case op_append_mut_move: {
            auto src = read<std::uint8_t>(in, is_valid_var_neq(dst));
            if (can_concat(ts[dst], ts[src])) {
                IMMER_FUZZED_TRACE("t" << +dst << ".append(std::move(t" << +src
                                       << "));"
                                       << " t" << +src << " = {};");
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

} // namespace

TEST_CASE("bug: concatenating transients")
{
    // When concatenating two transients vectors the nodes from the
    // argument become aliased in the result.  As such, we need to
    // reset the identitiy of the argument.
    SECTION("simplified")
    {
        using vector_t = immer::flex_vector<int, gc_memory, 2, 2>;
        auto t0        = vector_t{}.transient();
        t0.push_back(42);
        t0.push_back(42);
        t0.push_back(42);
        t0.push_back(42);
        t0.push_back(42);
        t0.push_back(42);
        auto t1 = t0;
        t1.append(t0);
        t1.append(t0);
        t0.append(t1);
        t1.append(t0);
    }

#if __GNUC__ != 9 && __GNUC__ != 8
    SECTION("")
    {
        constexpr std::uint8_t input[] = {
            0x2, 0x2, 0x2, 0x2, 0x29, 0x32, 0x0, 0x0,  0x2,  0x2,  0x2,
            0x2, 0x2, 0x2, 0x2, 0x2,  0x2,  0x2, 0x2,  0x2,  0x6,  0x2,
            0x2, 0x2, 0x2, 0x2, 0x6,  0x2,  0x2, 0x2,  0x0,  0x38, 0x2,
            0x0, 0x0, 0x2, 0x2, 0xd,  0x0,  0x0, 0x3b, 0xff, 0x3a, 0x2,
            0xd, 0xd, 0x2, 0x0, 0x0,  0x10, 0xe, 0x0,  0xd,  0x0,  0x0,
            0x2, 0x2, 0xd, 0x0, 0x1,  0x5,
        };
        CHECK(run_input(input, sizeof(input)) == 0);
    }
#endif
}

TEST_CASE("bug: concatenating moved transients")
{
    // A moved from concatenated transient is totally smashed, we can
    // not do anything with it but reasign...
    SECTION("simplified")
    {
        using vector_t    = immer::flex_vector<int, gc_memory, 2, 2>;
        using transient_t = typename vector_t::transient_type;
        auto v0           = vector_t{};
        auto t0           = transient_t{};
        auto t2           = transient_t{};
        v0                = v0.push_back(42);
        t2                = v0.transient();
        v0                = v0 + v0;
        v0                = v0 + v0;
        v0                = v0 + v0;
        v0                = v0 + v0;
        t0                = v0.transient();
        t0                = v0.transient();
        t0.append(std::move(t2));
        t2 = {};
        t2.append(std::move(t0));
    }

#if __GNUC__ != 9 && __GNUC__ != 8
    SECTION("")
    {
        constexpr std::uint8_t input[] = {
            0x0,  0x2, 0x0,  0x2,  0x0,  0x0,  0x0,  0x6,  0x0,  0x0,  0x0,
            0x6,  0x0, 0x0,  0x0,  0x9d, 0x0,  0x6,  0x0,  0x0,  0x0,  0x6,
            0x0,  0x0, 0x0,  0x9d, 0x28, 0x0,  0x0,  0x0,  0x0,  0x0,  0xf7,
            0xc5, 0x0, 0xa,  0xa,  0x0,  0xfa, 0xe7, 0xff, 0xe7, 0xff, 0x0,
            0xe,  0x2, 0x9,  0x0,  0x28, 0x2,  0xe,  0x0,  0x0,  0x2,  0xd,
            0x0,  0x0, 0x28, 0x0,  0xd,  0x2,  0x5,  0x0,  0x2,
        };
        CHECK(run_input(input, sizeof(input)) == 0);
    }
#endif

    SECTION("simplified")
    {
        using vector_t    = immer::flex_vector<int, gc_memory, 2, 2>;
        using transient_t = typename vector_t::transient_type;
        auto v0           = vector_t{};
        auto t0           = transient_t{};
        auto t2           = transient_t{};
        v0                = v0.push_back(42);
        v0                = v0.push_back(42);
        v0                = v0 + v0;
        t0                = v0.transient();
        t2.prepend(std::move(t0));
        t0 = {};
        t0 = v0.transient();
        t0.push_back(13);
        t2.append(std::move(t0));
        t0 = {};
        t0 = v0.transient();
        t0.push_back(13);
        t2.prepend(std::move(t0));
        t0 = {};
    }

#if __GNUC__ != 9 && __GNUC__ != 8
    SECTION("")
    {
        return;
        constexpr std::uint8_t input[] = {
            0x0,  0x2, 0x0,  0x0,  0x2, 0xb7, 0x1, 0x36, 0x40, 0x0,  0x0,
            0x0,  0x0, 0xb6, 0x0,  0x2, 0x0,  0x0, 0x6,  0xe,  0x0,  0x0,
            0xfe, 0x0, 0x0,  0xff, 0x0, 0x2,  0xc, 0xff, 0xfc, 0x29, 0x0,
            0x0,  0x0, 0x0,  0x0,  0x7, 0x2,  0xe, 0xff, 0xfc, 0x29, 0x0,
            0x0,  0x0, 0x0,  0x0,  0x7, 0x3,  0x0, 0x0,  0x2,  0xc,  0x2,
            0xc,  0x0, 0xd,  0x0,  0x0, 0x0,  0x0, 0x25, 0x6,
        };
        CHECK(run_input(input, sizeof(input)) == 0);
    }
#endif
}

TEST_CASE("bug: aegsdas")
{
    SECTION("simplified")
    {
        using vector_t    = immer::flex_vector<int, gc_memory, 2, 2>;
        using transient_t = typename vector_t::transient_type;
        auto v2           = vector_t{};
        auto t0           = transient_t{};
        auto t1           = transient_t{};
        v2                = v2.push_back(42);
        v2                = v2.push_back(42);
        v2                = v2.push_back(42);
        v2                = v2.push_back(42);
        v2                = v2.push_back(42);
        t0                = v2.transient();
        t1.prepend(t0);
        t1.prepend(t0);
        t1.prepend(t0);
        t0.prepend(std::move(t1));
        t1 = {};
    }

#if __GNUC__ != 9 && __GNUC__ != 8
    SECTION("")
    {
        constexpr std::uint8_t input[] = {
            0xff, 0xff, 0x2, 0x2,  0x2, 0x2,  0x2,  0x2,  0x2,  0x2,  0x2,
            0x82, 0x2,  0x2, 0x2,  0x2, 0x2,  0x2,  0x0,  0x0,  0x3d, 0x0,
            0x0,  0x0,  0x2, 0x84, 0x0, 0x3b, 0x1,  0xb,  0x0,  0xa,  0x1,
            0xb,  0x0,  0x0, 0x3,  0x2, 0x0,  0x3b, 0x1,  0xb,  0x1,  0x0,
            0x0,  0xc,  0xb, 0x1,  0x8, 0xff, 0xff, 0xfc, 0xfd, 0x0,  0x3b,
            0x3,  0x2,  0x0, 0x3b, 0x1, 0x9,  0x1,  0x3b,
        };
        CHECK(run_input(input, sizeof(input)) == 0);
    }
#endif
}
