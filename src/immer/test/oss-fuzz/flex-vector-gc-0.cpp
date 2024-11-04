//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#include "input.hpp"

#include <immer/flex_vector.hpp>
#include <immer/flex_vector_transient.hpp>
#include <immer/heap/gc_heap.hpp>
#include <immer/refcount/no_refcount_policy.hpp>

#include <catch2/catch.hpp>

#define IMMER_FUZZED_TRACE_ENABLE 0

#if IMMER_FUZZED_TRACE_ENABLE
#include <fmt/ostream.h>
#define IMMER_FUZZED_TRACE(...) fmt::print(std::cerr, __VA_ARGS__)
#else
#define IMMER_FUZZED_TRACE(...)
#endif

using gc_memory = immer::memory_policy<immer::heap_policy<immer::gc_heap>,
                                       immer::no_refcount_policy,
                                       immer::default_lock_policy,
                                       immer::gc_transience_policy,
                                       false>;

namespace {

int run_input(const std::uint8_t* data, std::size_t size)
{
    constexpr std::size_t var_count = 4;
    constexpr unsigned bits         = 2;

    using vector_t    = immer::flex_vector<int, gc_memory, bits, bits>;
    using transient_t = typename vector_t::transient_type;
    using size_t      = std::uint8_t;

    auto vs = std::array<vector_t, var_count>{};
    auto ts = std::array<transient_t, var_count>{};

#if IMMER_FUZZED_TRACE_ENABLE
    IMMER_FUZZED_TRACE("/// new test run\n");
    IMMER_FUZZED_TRACE(
        "using vector_t = immer::flex_vector<int, gc_memory, {}, {}>;\n",
        bits,
        bits);
    for (auto i = 0u; i < var_count; ++i)
        IMMER_FUZZED_TRACE("auto v{} = vector_t{{}};\n", i);
    for (auto i = 0u; i < var_count; ++i)
        IMMER_FUZZED_TRACE("auto t{} = transient_t{{}};\n", i);
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
            IMMER_FUZZED_TRACE("t{} = v{}.transient();\n", +dst, +src);
            ts[dst] = vs[src].transient();
            break;
        }
        case op_persistent: {
            auto src = read<std::uint8_t>(in, is_valid_var);
            IMMER_FUZZED_TRACE("v{} = t{}.persistent();\n", +dst, +src);
            vs[dst] = ts[src].persistent();
            break;
        }
        case op_push_back: {
            auto src = read<std::uint8_t>(in, is_valid_var);
            IMMER_FUZZED_TRACE("v{} = v{}.push_back(42);\n", +dst, +src);
            vs[dst] = vs[src].push_back(42);
            break;
        }
        case op_update: {
            auto src = read<std::uint8_t>(in, is_valid_var);
            auto idx = read<size_t>(in, is_valid_index(vs[src]));
            IMMER_FUZZED_TRACE(
                "v{} = v{}.update({}, [] (auto x) {{ return x + 1; }});\n",
                +dst,
                +src,
                idx);
            vs[dst] = vs[src].update(idx, [](auto x) { return x + 1; });
            break;
        }
        case op_take: {
            auto src = read<std::uint8_t>(in, is_valid_var);
            auto idx = read<size_t>(in, is_valid_size(vs[src]));
            IMMER_FUZZED_TRACE("v{} = v{}.take({});\n", +dst, +src, idx);
            vs[dst] = vs[src].take(idx);
            break;
        }
        case op_drop: {
            auto src = read<std::uint8_t>(in, is_valid_var);
            auto idx = read<size_t>(in, is_valid_size(vs[src]));
            IMMER_FUZZED_TRACE("v{} = v{}.drop({});\n", +dst, +src, idx);
            vs[dst] = vs[src].drop(idx);
            break;
        }
        case op_concat: {
            auto src  = read<std::uint8_t>(in, is_valid_var);
            auto src2 = read<std::uint8_t>(in, is_valid_var);
            if (can_concat(vs[src], vs[src2])) {
                IMMER_FUZZED_TRACE("v{} = v{} + v{};\n", +dst, +src, +src2);
                vs[dst] = vs[src] + vs[src2];
            }
            break;
        }
        case op_push_back_mut: {
            IMMER_FUZZED_TRACE("t{}.push_back(13);\n", +dst);
            ts[dst].push_back(13);
            break;
        }
        case op_update_mut: {
            auto idx = read<size_t>(in, is_valid_index(ts[dst]));
            IMMER_FUZZED_TRACE(
                "t{}.update({}, [] (auto x) {{ return x + 1; }});\n",
                +dst,
                idx);
            ts[dst].update(idx, [](auto x) { return x + 1; });
            break;
        }
        case op_take_mut: {
            auto idx = read<size_t>(in, is_valid_size(ts[dst]));
            IMMER_FUZZED_TRACE("t{}.take({});\n", +dst, idx);
            ts[dst].take(idx);
            break;
        }
        case op_prepend_mut: {
            auto src = read<std::uint8_t>(in, is_valid_var_neq(dst));
            if (can_concat(ts[dst], ts[src])) {
                IMMER_FUZZED_TRACE("t{}.prepend(t{});\n", +dst, +src);
                ts[dst].prepend(ts[src]);
            }
            break;
        }
        case op_prepend_mut_move: {
            auto src = read<std::uint8_t>(in, is_valid_var_neq(dst));
            if (can_concat(ts[dst], ts[src])) {
                IMMER_FUZZED_TRACE("t{}.prepend(std::move(t{}));\n"
                                   "t{} = {{}};\n",
                                   +dst,
                                   +src,
                                   +src);
                ts[dst].prepend(std::move(ts[src]));
                ts[src] = {};
            }
            break;
        }
        case op_append_mut: {
            auto src = read<std::uint8_t>(in, is_valid_var_neq(dst));
            if (can_concat(ts[dst], ts[src])) {
                IMMER_FUZZED_TRACE("t{}.append(t{});\n", +dst, +src);
                ts[dst].append(ts[src]);
            }
            break;
        }
        case op_append_mut_move: {
            auto src = read<std::uint8_t>(in, is_valid_var_neq(dst));
            if (can_concat(ts[dst], ts[src])) {
                IMMER_FUZZED_TRACE("t{}.append(std::move(t{}));\n"
                                   "t{} = {{}};\n",
                                   +dst,
                                   +src,
                                   +src);
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

TEST_CASE("https://bugs.chromium.org/p/oss-fuzz/issues/detail?id=24209")
{
    SECTION("fuzzer")
    {
        auto input = load_input(
            "clusterfuzz-testcase-minimized-flex-vector-gc-5127731734642688");
        CHECK(run_input(input.data(), input.size()) == 0);
    }
}

TEST_CASE("https://bugs.chromium.org/p/oss-fuzz/issues/detail?id=24196")
{
    SECTION("fuzzer")
    {
        auto input = load_input(
            "clusterfuzz-testcase-minimized-flex-vector-gc-5676111456108544");
        CHECK(run_input(input.data(), input.size()) == 0);
    }
}

TEST_CASE("https://bugs.chromium.org/p/oss-fuzz/issues/detail?id=24168")
{
    SECTION("fuzzer")
    {
        auto input = load_input(
            "clusterfuzz-testcase-minimized-flex-vector-gc-5428967461617664");
        CHECK(run_input(input.data(), input.size()) == 0);
    }
}

TEST_CASE("https://bugs.chromium.org/p/oss-fuzz/issues/detail?id=24148")
{
    SECTION("fuzzer")
    {
        auto input = load_input(
            "clusterfuzz-testcase-minimized-flex-vector-gc-6299398922043392");
        CHECK(run_input(input.data(), input.size()) == 0);
    }
}

TEST_CASE("https://bugs.chromium.org/p/oss-fuzz/issues/detail?id=24143")
{
    SECTION("fuzzer")
    {
        auto input = load_input(
            "clusterfuzz-testcase-minimized-flex-vector-gc-5123086366801920");
        CHECK(run_input(input.data(), input.size()) == 0);
    }
}

TEST_CASE("https://bugs.chromium.org/p/oss-fuzz/issues/detail?id=24162")
{
    SECTION("fuzzer")
    {
        auto input = load_input(
            "clusterfuzz-testcase-minimized-flex-vector-gc-5151861104181248");
        CHECK(run_input(input.data(), input.size()) == 0);
    }
}

TEST_CASE("https://bugs.chromium.org/p/oss-fuzz/issues/detail?id=24155")
{
    SECTION("fuzzer")
    {
        auto input = load_input(
            "clusterfuzz-testcase-minimized-flex-vector-gc-5194423089233920");
        CHECK(run_input(input.data(), input.size()) == 0);
    }
}

TEST_CASE("https://bugs.chromium.org/p/oss-fuzz/issues/detail?id=24136")
{
    SECTION("fuzzer")
    {
        auto input = load_input(
            "clusterfuzz-testcase-minimized-flex-vector-gc-5635385259196416");
        CHECK(run_input(input.data(), input.size()) == 0);
    }
}

TEST_CASE("https://bugs.chromium.org/p/oss-fuzz/issues/detail?id=24142")
{
    SECTION("fuzzer")
    {
        auto input = load_input(
            "clusterfuzz-testcase-minimized-flex-vector-gc-4787718039797760");
        CHECK(run_input(input.data(), input.size()) == 0);
    }
}

TEST_CASE("https://bugs.chromium.org/p/oss-fuzz/issues/detail?id=24172")
{
    SECTION("fuzzer")
    {
        auto input = load_input(
            "clusterfuzz-testcase-minimized-flex-vector-gc-6595824679911424");
        CHECK(run_input(input.data(), input.size()) == 0);
    }
}

TEST_CASE("https://bugs.chromium.org/p/oss-fuzz/issues/detail?id=24173")
{
    SECTION("fuzzer")
    {
        auto input = load_input(
            "clusterfuzz-testcase-minimized-flex-vector-gc-4872518268354560");
        CHECK(run_input(input.data(), input.size()) == 0);
    }
}

TEST_CASE("https://bugs.chromium.org/p/oss-fuzz/issues/detail?id=24213")
{
    SECTION("fuzzer")
    {
        auto input = load_input(
            "clusterfuzz-testcase-minimized-flex-vector-gc-6265466893631488");
        CHECK(run_input(input.data(), input.size()) == 0);
    }
}

TEST_CASE("https://bugs.chromium.org/p/oss-fuzz/issues/detail?id=24264")
{
    SECTION("fuzzer")
    {
        auto input = load_input(
            "clusterfuzz-testcase-minimized-flex-vector-gc-4855756386729984");
        CHECK(run_input(input.data(), input.size()) == 0);
    }
}

TEST_CASE("https://bugs.chromium.org/p/oss-fuzz/issues/detail?id=24286")
{
    SECTION("fuzzer")
    {
        auto input = load_input(
            "clusterfuzz-testcase-minimized-flex-vector-gc-6017886557306880");
        CHECK(run_input(input.data(), input.size()) == 0);
    }
}

TEST_CASE("https://bugs.chromium.org/p/oss-fuzz/issues/detail?id=24287")
{
    SECTION("fuzzer")
    {
        auto input = load_input(
            "clusterfuzz-testcase-minimized-flex-vector-gc-5660697665732608");
        CHECK(run_input(input.data(), input.size()) == 0);
    }
}

TEST_CASE("https://bugs.chromium.org/p/oss-fuzz/issues/detail?id=24371")
{
    SECTION("fuzzer")
    {
        auto input = load_input(
            "clusterfuzz-testcase-minimized-flex-vector-gc-5120685673021440");
        CHECK(run_input(input.data(), input.size()) == 0);
    }
}

TEST_CASE("https://bugs.chromium.org/p/oss-fuzz/issues/detail?id=24412")
{
    SECTION("trace, hand minimized")
    {
        /// new test run
        using vector_t    = immer::flex_vector<int, gc_memory, 2, 2>;
        using transient_t = immer::flex_vector_transient<int, gc_memory, 2, 2>;
        auto v0           = vector_t{};
        auto v1           = vector_t{};
        auto v2           = vector_t{};
        auto v3           = vector_t{};
        auto t0           = transient_t{};
        auto t1           = transient_t{};
        auto t2           = transient_t{};
        auto t3           = transient_t{};
        t0.push_back(13);
        t0.push_back(13);
        t0.push_back(13);
        t0.push_back(13);
        v3 = v2.take(2);
        v3 = v2.drop(2);
        v3 = v2.take(2);
        v3 = v3.push_back(42);
        t0 = v0.transient();
        v3 = v2.take(2);
        v3 = v2.drop(2);
        v3 = v2.take(2);
        v3 = v3.push_back(42);
        t0 = v0.transient();
        v3 = v2.take(2);
        v3 = v2.drop(2);
        v3 = v2.take(2);
        v3 = v3.push_back(42);
        t0 = v0.transient();
        v3 = v2.take(2);
        v3 = v2.drop(2);
        v3 = v2.take(2);
        v3 = v3.push_back(42);
        t0 = v0.transient();
        v3 = v2.take(2);
        v3 = v2.drop(2);
        v3 = v2.take(2);
        v3 = v0.drop(0);
        v3 = v2.take(2);
        v3 = v2.drop(2);
        v3 = v2.take(2);
        v3 = v3.push_back(42);
        t0 = v0.transient();
        v3 = v2.take(2);
        v3 = v2.drop(2);
        v3 = v2.take(2);
        v3 = v3.push_back(42);
        t0 = v0.transient();
        v3 = v2.take(2);
        v3 = v2.drop(2);
        v3 = v2.take(2);
        t2 = v1.transient();
        v1 = v1.push_back(42);
        t2 = v2.transient();
        v2 = t0.persistent();
        v2 = v1.push_back(42);
        v0 = v2.push_back(42);
        v1 = v0.push_back(42);
        t0 = v0.transient();
        v1 = v3.push_back(42);
        v3 = v0.drop(0);
        v3 = v2.drop(2);
        v3 = v2.take(2);
        v3 = v3.push_back(42);
        t0 = v0.transient();
        v3 = v2.take(2);
        v3 = v2.drop(2);
        v3 = v2.take(2);
        v3 = v3.push_back(42);
        t0 = v0.transient();
        v3 = v2.take(2);
        v3 = v2.drop(2);
        v3 = v2.take(2);
        v3 = v3.push_back(42);
        t0 = v0.transient();
        v3 = v2.take(2);
        v3 = v2.drop(2);
        v3 = v2.take(2);
        v3 = v3.push_back(42);
        t0 = v0.transient();
        v3 = v2.take(2);
        v3 = v2.drop(2);
        v3 = v2.take(2);
        v3 = v3.push_back(42);
        t0 = v0.transient();
        v3 = v2.take(2);
        v3 = v2.drop(2);
        v3 = v2.take(2);
        v3 = v3.push_back(42);
        t0 = v0.transient();
        v3 = v2.take(2);
        v3 = v2.take(2);
        v3 = v3.push_back(42);
        t0 = v0.transient();
        v3 = v2.take(2);
        v3 = v2.drop(2);
        v3 = v2.take(2);
        v3 = v3.push_back(42);
        t0 = v0.transient();
        v3 = v2.take(2);
        v3 = v2.drop(2);
        v3 = v2.take(2);
        v3 = v3.push_back(42);
        t0 = v0.transient();
        v3 = v2.take(2);
        v3 = v2.drop(2);
        v3 = v2.take(2);
        v3 = v3.push_back(42);
        t0 = v0.transient();
        v3 = v2.take(2);
        v3 = v2.drop(2);
        v3 = v2.take(2);
        v3 = v3.push_back(42);
        t0 = v0.transient();
        v3 = v2.take(2);
        v3 = v2.drop(2);
        v3 = v2.take(2);
        v3 = v3.push_back(42);
        t0 = v0.transient();
        v3 = v2.take(2);
        v3 = v2.drop(2);
        v3 = v2.take(2);
        v3 = v3.push_back(42);
        t0 = v0.transient();
        v3 = v2.take(2);
        v3 = v2.drop(2);
        v3 = v2.take(2);
        v3 = v3.push_back(42);
        t0 = v0.transient();
        v3 = v2.take(2);
        v3 = v2.drop(2);
        v3 = v2.take(2);
        v3 = v3.push_back(42);
        t0 = v0.transient();
        v3 = v2.take(2);
        v3 = v2.drop(2);
        v3 = v2.take(2);
        v3 = v3.push_back(42);
        t0 = v0.transient();
        v3 = v2.take(2);
        v3 = v2.drop(2);
        v3 = v2.take(2);
        v3 = v3.push_back(42);
        t0 = v0.transient();
        v3 = v2.take(2);
        v3 = v2.drop(2);
        v3 = v2.take(2);
        v3 = v3.push_back(42);
        t0 = v0.transient();
        v3 = v2.take(2);
        v3 = v2.drop(2);
        v3 = v2.take(2);
        v3 = v3.push_back(42);
        t0 = v0.transient();
        v3 = v2.take(2);
        v3 = v2.drop(2);
        v3 = v2.take(2);
        v3 = v3.push_back(42);
        t0 = v0.transient();
        v3 = v2.take(2);
        v3 = v2.drop(2);
        v3 = v2.take(2);
        v3 = v3.push_back(42);
        t0 = v0.transient();
        v3 = v2.take(2);
        v3 = v2.drop(2);
        v3 = v2.take(2);
        v3 = v3.push_back(42);
        t0 = v0.transient();
        v3 = v2.take(2);
        v3 = v2.drop(2);
        v3 = v2.take(2);
        v3 = v3.push_back(42);
        t0 = v0.transient();
        v3 = v0.take(0);
        v3 = v2.drop(2);
        v3 = v2.take(2);
        v3 = v3.push_back(42);
        t0 = v0.transient();
        v3 = v2.take(2);
        v3 = v2.drop(2);
        v3 = v2.take(2);
        v3 = v3.push_back(42);
        t0 = v0.transient();
        v3 = v2.take(2);
        v3 = v2.drop(2);
        v3 = v2.take(2);
        v3 = v3.push_back(42);
        t0 = v0.transient();
        v3 = v2.take(2);
        v3 = v2.drop(2);
        v3 = v2.take(3);
        v3 = v2.drop(2);
        v3 = v2.take(2);
        v3 = v3.push_back(42);
        t0 = v0.transient();
        v3 = v0.take(0);
        v0 = t3.persistent();
        v3 = v2.take(2);
        v3 = v3.push_back(42);
        t0 = v1.transient();
        t0.push_back(13);
        t0.push_back(13);
        t3 = v0.transient();
        v0 = t0.persistent();
        t0.push_back(13);
        t0.push_back(13);
        t3 = v0.transient();
        v0 = t0.persistent();
        t0 = v0.transient();
        t0.push_back(13);
        t0 = v0.transient();
        t0 = v0.transient();
        t0 = v0.transient();
        t0.push_back(13);
        t0 = v3.transient();
        t0 = v0.transient();
        t1 = v0.transient();
        t0.push_back(13);
        t0.push_back(13);
        t0.push_back(13);
        t0.push_back(13);
        t0.push_back(13);
        t3 = v0.transient();
        v0 = t0.persistent();
        t0.push_back(13);
        t0.push_back(13);
        t0.push_back(13);
        t0.push_back(13);
        t0.push_back(13);
        t3 = v0.transient();
        v0 = t0.persistent();
        t0.push_back(13);
        t0.push_back(13);
        t3 = v0.transient();
        v0 = t0.persistent();
        v0 = t0.persistent();
        t0 = v0.transient();
        t0.push_back(13);
        t0 = v0.transient();
        t0 = v0.transient();
        t0 = v0.transient();
        t0.push_back(13);
        t0 = v3.transient();
        t0 = v0.transient();
        t1 = v0.transient();
        t0.push_back(13);
        t0.push_back(13);
        t2 = v0.transient();
        t1 = v0.transient();
        t2 = v0.transient();
        v1 = v0.push_back(42);
        t0.take(1);
        t0.take(1);
        t0.prepend(t2);
        t0.take(1);
        t0.prepend(t2);
        t0.take(1);
        t0.take(1);
        t0.prepend(t2);
        t0.take(1);
        t0.prepend(t2);
        t0.push_back(13);
        t0.push_back(13);
        t0.push_back(13);
        v0 = t0.persistent();
        v0 = t0.persistent();
        t0.push_back(13);
        t0.push_back(13);
        t0.push_back(13);
        t0.push_back(13);
        t0.push_back(13);
        t3 = v0.transient();
        v0 = t0.persistent();
        t0.push_back(13);
        t0.push_back(13);
        t3 = v0.transient();
        v0 = t0.persistent();
        t0 = v0.transient();
        t0.push_back(13);
        t0 = v0.transient();
        t0 = v0.transient();
        t0 = v0.transient();
        t0.push_back(13);
        t0 = v3.transient();
        t0 = v0.transient();
        t1 = v0.transient();
        t0.push_back(13);
        t0.push_back(13);
        t0.push_back(13);
        t0.push_back(13);
        t0.push_back(13);
        t3 = v0.transient();
        v0 = t0.persistent();
        t0.push_back(13);
        t0.take(1);
        t2 = v0.transient();
        t0.take(1);
        t0.take(1);
        t0.prepend(t2);
        t0.take(1);
        t0.prepend(t2);
        t0.take(1);
        t0.push_back(13);
        t0.push_back(13);
        t0.push_back(13);
        t0.push_back(13);
        t3 = v0.transient();
        v0 = t0.persistent();
        t0.push_back(13);
        t0.take(1);
        t0.prepend(t2);
        t0 = v0.transient();
        t0.take(1);
        t0.prepend(t2);
        t0.take(1);
        t0.prepend(t2);
        t0.take(1);
        t0.take(1);
        t0.prepend(t2);
        t0.take(1);
        t0.prepend(t2);
        t0.take(1);
        t2 = v0.transient();
        t0.take(1);
        t0.take(1);
        t0.prepend(t2);
        t0.take(1);
        t0.prepend(t2);
        t0.take(1);
        t0.take(1);
        t0.prepend(t2);
        t0.take(1);
        t0.prepend(t2);
        t0.take(1);
        t2 = v0.transient();
        t0.take(1);
        t0.take(1);
        t0.prepend(t2);
        t0.take(1);
        t0.prepend(t2);
        t0.take(1);
        t0.take(1);
        t0.prepend(t2);
        t0.take(1);
        t0.prepend(t2);
        t0.take(1);
        t2 = v0.transient();
        t0.take(1);
        t0.take(1);
        t0.prepend(t2);
        t0.take(1);
        t0.prepend(t2);
        t0.take(1);
        t0.take(1);
        t0.prepend(t2);
        t0.take(1);
        t0.prepend(t2);
        t0.take(1);
        t2 = v0.transient();
        t0.take(1);
        t0.take(1);
        t0.prepend(t2);
        t0.take(1);
        t0.prepend(t2);
        t0.take(1);
        t1 = v0.transient();
        t2 = v0.transient();
        v1 = v0.push_back(42);
        t0.take(1);
        t0.take(1);
        t0.prepend(t2);
        t0.take(1);
        t0.prepend(t2);
        t0.take(1);
        t0.take(1);
        t0.prepend(t2);
        t0.take(1);
        t0.prepend(t2);
        t0.take(1);
        t2 = v0.transient();
        t0.take(1);
        t0.take(1);
        t0.prepend(t2);
        t0.take(1);
        t0.prepend(t2);
        t0.take(1);
        t0.take(1);
        t0.prepend(t2);
        t0.take(1);
        t0.prepend(t2);
        t0.take(1);
        t2 = v0.transient();
        t0.take(1);
        t0.take(1);
        t0.prepend(t2);
        t0.take(1);
        t0.prepend(t2);
        t0.take(1);
        t0.take(1);
        t0.prepend(t2);
        t0.take(1);
        t0.prepend(t2);
        t0.take(1);
        t2 = v0.transient();
        t0.take(1);
        t0.take(1);
        t0.prepend(t2);
        t0.take(1);
        t0.prepend(t2);
        t0.take(1);
        t0.take(1);
        t0.prepend(t2);
        t0.take(1);
        t0.prepend(t2);
        t0.take(1);
        t2 = v0.transient();
        t0.take(1);
        t0.take(1);
        t0.prepend(t2);
        t0.take(1);
        t0.prepend(t2);
        t0.take(1);
        t0.take(1);
        t0.prepend(t2);
        t0.take(1);
        t0.prepend(t2);
        t0.take(1);
        t2 = v0.transient();
        t0.take(1);
        t0.take(1);
        t0.prepend(t2);
        t0.take(1);
        t0.prepend(t2);
        t0.take(1);
        t0.take(1);
        t0.prepend(t2);
        t0.take(1);
        t0.prepend(t2);
        t0.take(1);
        t2 = v0.transient();
        t0.take(1);
        t0.take(1);
        t0.prepend(t2);
        t0.take(1);
        t0.prepend(t2);
        t0.take(1);
        t0.take(1);
        t0.prepend(t2);
        t0.take(1);
        t0.prepend(t2);
        t0.take(1);
        t2 = v0.transient();
        t0.take(1);
        t0.take(1);
        t0.prepend(t2);
        t0.take(1);
        t0.prepend(t2);
        t0.take(1);
        t0.take(1);
        t0.prepend(t2);
        t0.take(1);
        t0.prepend(t2);
        t0.take(1);
        t2 = v0.transient();
        t0.take(1);
        t0.take(1);
        t0.prepend(t2);
        t0.take(1);
        t0.prepend(t2);
        t0 = v0.transient();
        v2 = v2.push_back(42);
        t1 = v2.transient();
        v2 = t2.persistent();
        v2 = v2.push_back(42);
        v2 = v1.push_back(42);
        v0 = t0.persistent();
        v1 = v0.push_back(42);
        t1 = v1.transient();
        t0 = v2.transient();
        v2 = t1.persistent();
        t2 = v2.transient();
        v2 = t0.persistent();
        v2 = v2.push_back(42);
        v0 = t1.persistent();
        t2 = v1.transient();
        t2 = v2.transient();
        v2 = t0.persistent();
        v2 = v1.push_back(42);
        t1 = v0.transient();
        v2 = v1.push_back(42);
        t2 = v0.transient();
        v1 = v1.drop(2);
        v0 = t2.persistent();
        v2 = v1.push_back(42);
        v2 = v1.push_back(42);
        t2 = v1.transient();
        v2 = t0.persistent();
        v2 = v1.push_back(42);
        t1 = v1.transient();
        t0 = v2.transient();
        t2.append(t1);
        t1 = v0.transient();
        t1.prepend(t0);
        t0.append(t1);
        t1.prepend(t0);
        t1 = v2.transient();
        v2 = t2.persistent();
        t0 = v0.transient();
        v0 = t0.persistent();
        t1 = v0.transient();
        v2 = v1.push_back(42);
        t2 = v0.transient();
        v2 = v1.push_back(42);
        t3 = v2.transient();
        t1 = v2.transient();
        t2 = v1.transient();
        t2 = v1.transient();
        v2 = v1.push_back(42);
        t2 = v2.transient();
        v1 = v2.push_back(42);
        v2 = t0.persistent();
        v2 = v1.push_back(42);
        v0 = v2.push_back(42);
        v1 = v0.push_back(42);
        t1 = v0.transient();
        v0 = v1.push_back(42);
        t2 = v2.transient();
        v2 = v2.push_back(42);
        v2 = v2 + v0;
        t0.push_back(13);
        t0.push_back(13);
        t0.push_back(13);
        t0 = v1.transient();
        t0.push_back(13);
        t3 = v0.transient();
        t1 = v0.transient();
        t0 = v0.transient();
        t0.append(t1);
        t1.prepend(t0);
        t1.prepend(t0);
        t0.append(t1);
        t1.prepend(t0);
        t1.prepend(t0);
        t1.prepend(t0);
        t1 = v0.transient();
        t0.push_back(13);
        v1 = v0.push_back(42);
        v1 = v0.push_back(42);
        t1 = v0.transient();
        t1.prepend(t0);
        t0.append(t1);
        t1.prepend(t0);
        t2.prepend(std::move(t0));
        t0 = {};
        t2.prepend(std::move(t0));
        t0 = {};
        v2 = v0.push_back(42);
        t1.prepend(t0);
        t1.prepend(t0);
        t0.append(t1);
        t0 = v0.transient();
        t0 = v1.transient();
        v1 = v0.update(2, [](auto x) { return x + 1; });
        v0 = v0.take(1);
        t0.prepend(t1);
        t0 = v0.transient();
        t1.prepend(t0);
        t1.prepend(t0);
        t0.append(t1);
        t1.prepend(t0);
        t1.prepend(t0);
        v3 = v2.take(5);
        t1.prepend(t0);
        t1.prepend(t0);
        t1.prepend(t0);
        v0 = t0.persistent();
        t0.append(t1);
        t1.prepend(t0);
        v1 = v0.update(32, [](auto x) { return x + 1; });
        t2 = v0.transient();
        v0 = v0.take(63);
        t1.prepend(t0);
        t0.push_back(13);
        t1.prepend(t0);
        t0.push_back(13);
        v0 = t0.persistent();
        t0.push_back(13);
        t1.prepend(t0);
        t0.append(t3);
        t1.append(t0);
        t2.prepend(std::move(t0));
        t0 = {};
        t2.prepend(std::move(t0));
        t0 = {};
        t0 = v2.transient();
        t2.prepend(std::move(t0));
        t0 = {};
        t0.push_back(13);
        t0 = v0.transient();
        t1.append(t0);
        t3.prepend(std::move(t2));
        t2 = {};
        t1.append(t0);
        t2.prepend(std::move(t0));
        t0 = {};
        t0 = v0.transient();
        t0 = v2.transient();
        t1 = v1.transient();
        t0 = v0.transient();
        v2 = v2.push_back(42);
        v2 = v0 + v2;
        v2 = v2.push_back(42);
        v2 = v2.push_back(42);
        v2 = v2.push_back(42);
        v2 = v2.push_back(42);
        v0 = v2.push_back(42);
        v2 = v2.push_back(42);
        v2 = v3.push_back(42);
        v2 = v2.push_back(42);
        v2 = v2.push_back(42);
        v2 = v0 + v2;
        v2 = v2.push_back(42);
        v2 = v2.push_back(42);
        v2 = v0.push_back(42);
        v2 = v0.push_back(42);
        t1.prepend(t0);
        t1.prepend(t0);
        t0.append(t1);
        t0 = v0.transient();
        t0 = v0.transient();
        v0 = t0.persistent();
        v1 = v0.update(32, [](auto x) { return x + 1; });
        t2 = v0.transient();
        t1.prepend(t0);
        t0.push_back(13);
        t0 = v0.transient();
        t0 = v0.transient();
        t1.prepend(t0);
        t0.prepend(std::move(t1));
        t1 = {};
        t1.prepend(t0);
        t1.prepend(t0);
        t0.append(t1);
        t1.prepend(t0);
        t1.prepend(t0);
        v3 = v2.take(5);
        t1.prepend(t0);
        t1.prepend(t0);
        t1.prepend(t0);
        v0 = t0.persistent();
        t0.append(t1);
        t1 = v0.transient();
        t0 = v0.transient();
        v2 = v1.push_back(42);
        t2 = v1.transient();
        v0 = t0.persistent();
        v0 = v2.push_back(42);
        v1 = v0.push_back(42);
        v2 = v1.push_back(42);
        v0 = v2.push_back(42);
        t2 = v1.transient();
        v1 = v0.push_back(42);
        v1 = v0.push_back(42);
        v2 = v1.push_back(42);
        v0 = v2.push_back(42);
        v1 = v0.push_back(42);
        t1 = v1.transient();
        t0 = v2.transient();
        v1 = v0.push_back(42);
        t2 = v0.transient();
        t0 = v0.transient();
        t0 = v0.transient();
        t0 = v0.transient();
        t0 = v0.transient();
        t0 = v0.transient();
        t0 = v0.transient();
        t0 = v1.transient();
        t0.append(t1);
        t0.push_back(13);
        t0.push_back(13);
        t1.push_back(13);
        t0.append(t1);
        t0.append(t1);
        t0.append(t1);
        t0.push_back(13);
        t0.push_back(13);
        t1.push_back(13);
        t0.append(t1);
        t0.append(t1);
        t0.append(t1);
        t0 = v1.transient();
        t0.push_back(13);
        t0.push_back(13);
        t1.push_back(13);
        t0.append(t1);
        t0.append(t1);
        t0.append(t1);
        t0.push_back(13);
        t0.push_back(13);
        t1.push_back(13);
        t0.append(t1);
        t0.append(t1);
        t0.append(t1);
        t0.push_back(13);
        t0.push_back(13);
        t1.push_back(13);
        t0.append(t1);
        t0.append(t1);
        t0.append(t1);
        t0.push_back(13);
        t0.push_back(13);
        t1.push_back(13);
        t0.append(t1);
        t0.append(t1);
        t0.append(t1);
        t0.push_back(13);
        t0.push_back(13);
        t1.push_back(13);
        t0.append(t1);
        t0.append(t1);
        t0.append(t1);
        t0.push_back(13);
        t0.push_back(13);
        t1.push_back(13);
        t0.append(t1);
        t0.append(t1);
        t0.append(t1);
        t0.push_back(13);
        t0.push_back(13);
        t1.push_back(13);
        t0.append(t1);
        t0.append(t1);
        t0.append(t1);
        t0.push_back(13);
        t0.push_back(13);
        t1.push_back(13);
        t0.append(t1);
        t0.append(t1);
        t0.append(t1);
        t0.push_back(13);
        t0.push_back(13);
        t1.push_back(13);
        t1.prepend(t0);
        t0.push_back(13);
        t0.append(t1);
        t0.append(t1);
        t0.append(t1);
        t0.push_back(13);
        t0.push_back(13);
        t1.push_back(13);
        t0.append(t1);
        t0.append(t1);
        t0.append(t1);
        t0.push_back(13);
        t0.push_back(13);
        t1.push_back(13);
        t0.append(t1);
        t0.append(t1);
        t0.append(t1);
        t0.push_back(13);
        t0.push_back(13);
        t1.push_back(13);
        t0.append(t1);
        t0.append(t1);
        t0.append(t1);
        t0.push_back(13);
        t0.push_back(13);
        t1.push_back(13);
        t0.append(t1);
        t0.append(t1);
        t0.append(t1);
        t0.push_back(13);
        t0.push_back(13);
        t0.push_back(13);
        t1.push_back(13);
        t0.append(t1);
        t0.append(t1);
        t0.append(t1);
        t0.push_back(13);
        t0.push_back(13);
        t1.push_back(13);
        t0.append(t1);
        t0.append(t1);
        t0.append(t1);
        t0.push_back(13);
        t0.push_back(13);
        t1.push_back(13);
        t0.append(t1);
        t0.append(t1);
        t0.append(t1);
        t0.push_back(13);
        t0.push_back(13);
        t1.push_back(13);
        t0.append(t1);
        t0.append(t1);
        t0.append(t1);
        t0.push_back(13);
        t0.push_back(13);
        t1.push_back(13);
        t0.append(t1);
        t0.append(t1);
        t0.append(t1);
        t0.push_back(13);
        t0.push_back(13);
        t1.push_back(13);
        t0.append(t1);
        t0.append(t1);
        t0.append(t1);
        t0.push_back(13);
        t0.push_back(13);
        t1.push_back(13);
        t0.append(t1);
        t0.append(t1);
        t0.append(t1);
        t0.push_back(13);
        t0.push_back(13);
        t1.push_back(13);
        t0.append(t1);
        t0.append(t1);
        t0 = v1.transient();
        t0.push_back(13);
        t1.push_back(13);
        t0.append(t1);
        t0.push_back(13);
        t0.push_back(13);
        t1.push_back(13);
        t1.prepend(t0);
        t0.push_back(13);
        t1.prepend(t0);
        t1.push_back(13);
        t0.append(t1);
        t0.push_back(13);
        t3 = v0.transient();
        v0 = t0.persistent();
        v0 = t0.persistent();
        t0 = v0.transient();
        t0.push_back(13);
        t0.take(18);
        t1 = v0.transient();
        v2 = v2 + v2;
        t0 = v0.transient();
        t0 = v0.transient();
        t0 = v0.transient();
        t0 = v0.transient();
        t0 = v0.transient();
        t0 = v0.transient();
        t0 = v0.transient();
        t0 = v0.transient();
        t0 = v0.transient();
        t0 = v0.transient();
        t0 = v0.transient();
        t0.push_back(13);
        t1.push_back(13);
        t0.append(t1);
        t0.push_back(13);
        t1.push_back(13);
        t0.append(t1);
        t0.push_back(13);
        t3 = v0.transient();
        v0 = t0.persistent();
        v0 = t0.persistent();
        t0 = v0.transient();
        t0.push_back(13);
        t0.take(18);
        t1 = v0.transient();
        v2 = v2 + v2;
        v0 = t0.persistent();
        t0 = v0.transient();
        t0.push_back(13);
        t0.push_back(13);
        t0.take(18);
        t1 = v0.transient();
        v0 = v0.push_back(42);
        t0.append(t1);
        t0.push_back(13);
        t1.push_back(13);
        t0.append(t1);
        t0.push_back(13);
        t1.push_back(13);
        t0.append(t1);
        t0.push_back(13);
        t3 = v0.transient();
        v0 = t0.persistent();
        v0 = t0.persistent();
        t0 = v0.transient();
        t0.push_back(13);
        t0.take(18);
        t1 = v0.transient();
        v2 = v2 + v2;
        v0 = t0.persistent();
        t0 = v0.transient();
        t0.push_back(13);
        t0.push_back(13);
        t0.take(18);
        t1 = v0.transient();
        v0 = v0.push_back(42);
        t0.append(t1);
        t0.push_back(13);
        t1.push_back(13);
        t0.append(t1);
        t0.push_back(13);
        t1.append(t0);
        t1.push_back(13);
        t0.push_back(13);
        t3 = v0.transient();
        v0 = t0.persistent();
        v0 = t0.persistent();
        t0 = v0.transient();
        t0.push_back(13);
        t0.take(18);
        t1 = v0.transient();
        t0 = v0.transient();
        t2 = v1.transient();
        t0 = v0.transient();
        t0.push_back(13);
        t0.push_back(13);
        t0.take(18);
        t1 = v0.transient();
        v0 = v0.push_back(42);
        t0.append(t1);
        t0.push_back(13);
        t1.push_back(13);
        t0.append(t1);
        t0.push_back(13);
        t1.push_back(13);
        t0.append(t1);
        t0.push_back(13);
        t3 = v0.transient();
        v0 = t0.persistent();
        v0 = t0.persistent();
        t0 = v0.transient();
        t0.push_back(13);
        t0.take(18);
        t1 = v0.transient();
        v2 = v2 + v2;
        v0 = t0.persistent();
        t0 = v0.transient();
        t0.push_back(13);
        t0.push_back(13);
        t0.take(18);
        t1 = v0.transient();
        v0 = v0.push_back(42);
        t0.append(t1);
        t0.push_back(13);
        t1.push_back(13);
        t0.append(t1);
        t0.push_back(13);
        t1.push_back(13);
        t0.append(t1);
        t0.push_back(13);
        t3 = v0.transient();
        v0 = t0.persistent();
        v0 = t0.persistent();
        t0 = v0.transient();
        t0.push_back(13);
        t0.take(18);
        t1 = v0.transient();
        v2 = v2 + v2;
        v0 = t0.persistent();
        t0 = v0.transient();
        t0.push_back(13);
        t0.push_back(13);
        t0.take(18);
        t1 = v0.transient();
        v0 = v0.push_back(42);
        t0.append(t1);
        t0 = v0.transient();
        t0 = v0.transient();
        t0 = v0.transient();
        t0 = v0.transient();
        t0 = v0.transient();
        t0 = v0.transient();
        t0 = v0.transient();
        t0 = v0.transient();
        t0 = v1.transient();
        t0 = v0.transient();
        t0.push_back(13);
        t0.push_back(13);
        t0.take(18);
        t1 = v0.transient();
        v2 = v2 + v2;
        v0 = t0.persistent();
        t0 = v0.transient();
        t0.push_back(13);
        t0.push_back(13);
        t0.take(18);
        t1 = v0.transient();
        v0 = v0.push_back(42);
        t0.append(t1);
        t1 = v0.transient();
        v2 = v2.drop(0);
        t1 = v0.transient();
        t0 = v0.transient();
        t0.push_back(13);
        t0.take(18);
        t1 = v0.transient();
        v0 = v0.push_back(42);
        t0.append(t1);
        t0.push_back(13);
        t1.push_back(13);
        t0.append(t1);
        t0.push_back(13);
        t1.push_back(13);
        t0.append(t1);
        t0.append(t1);
        t0.append(t2);
        v2 = v2.push_back(42);
        v2 = v2.push_back(42);
        v2 = v2.push_back(42);
        v2 = v2.push_back(42);
        v2 = v2.push_back(42);
        v2 = v2.push_back(42);
        v2 = v2.push_back(42);
        v2 = v2.push_back(42);
        v2 = v2.push_back(42);
        v2 = v2.push_back(42);
        v2 = v2.push_back(42);
        v2 = v2.push_back(42);
        v2 = v2.push_back(42);
        v2 = v2 + v2;
        v2 = v2.push_back(42);
        v2 = v2.push_back(42);
        v2 = v2.push_back(42);
        v2 = v2.push_back(42);
        v2 = v2 + v0;
        t0 = v0.transient();
        t0 = v1.transient();
        v2 = v2.push_back(42);
        t1 = v2.transient();
        v2 = t2.persistent();
        v2 = v2.push_back(42);
        v2 = v1.push_back(42);
        v0 = t0.persistent();
        v1 = v0.push_back(42);
        t1 = v1.transient();
        t0 = v2.transient();
        v2 = t1.persistent();
        t2 = v2.transient();
        v2 = t0.persistent();
        v2 = v2.push_back(42);
        v0 = t1.persistent();
        t2 = v1.transient();
        t2 = v2.transient();
        v2 = t0.persistent();
        v2 = v1.push_back(42);
        t1 = v0.transient();
        v2 = v1.push_back(42);
        t2 = v0.transient();
        v1 = v1.drop(2);
        v0 = t2.persistent();
        v2 = v1.push_back(42);
        v2 = v1.push_back(42);
        t2 = v1.transient();
        v2 = t0.persistent();
        v2 = v1.push_back(42);
        t1 = v1.transient();
        t0 = v2.transient();
        t2.append(t1);
        t1 = v0.transient();
        t1.prepend(t0);
        t0.append(t1);
        t1.prepend(t0);
        t1 = v2.transient();
        v2 = t2.persistent();
        t0 = v0.transient();
        v0 = t0.persistent();
        t1 = v0.transient();
        v2 = v1.push_back(42);
        t2 = v0.transient();
        v2 = v1.push_back(42);
        t3 = v2.transient();
        t1 = v2.transient();
        t2 = v1.transient();
        t2 = v1.transient();
        v2 = v1.push_back(42);
        t2 = v2.transient();
        v2 = t2.persistent();
        v2 = v1.push_back(42);
        v0 = v2.push_back(42);
        t1 = v2.transient();
        v2 = t2.persistent();
        v0 = t0.persistent();
        t0 = v2.transient();
        v1 = v0.push_back(42);
        v2 = v2.push_back(42);
        v2 = v2.push_back(42);
        v2 = v2 + v0;
        t0.push_back(13);
        t0.push_back(13);
        v2 = v2.push_back(42);
        v2 = v2.push_back(42);
        v2 = v2.push_back(42);
        v2 = v2.push_back(42);
        v2 = v2 + v2;
        v2 = v2.push_back(42);
        v2 = v2.push_back(42);
        v2 = v2.push_back(42);
        v0 = v2.drop(34);
        t0 = v0.transient();
        v0 = v0.push_back(42);
        v2 = v2.push_back(42);
        v2 = v2.push_back(42);
        v2 = v2.push_back(42);
        v2 = v2.push_back(42);
        v2 = v2.push_back(42);
        v0 = v2.drop(34);
        t0 = v0.transient();
        v0 = v0.take(87);
        v2 = v2.push_back(42);
        v2 = v2.push_back(42);
        v2 = v2.push_back(42);
        v2 = v2.push_back(42);
        v2 = v2.push_back(42);
        v0 = v2.drop(34);
        t0 = v0.transient();
        t0 = v0.transient();
        v0 = v0.push_back(42);
        v2 = v2.push_back(42);
        v2 = v2.push_back(42);
        v2 = v2.push_back(42);
        v2 = v2.push_back(42);
        v2 = v2.push_back(42);
        v0 = v2.drop(34);
        t0 = v0.transient();
        v0 = v0.take(87);
        v2 = v2.push_back(42);
        v2 = v2.push_back(42);
        v2 = v2.push_back(42);
        t0 = v0.transient();
        t0 = v0.transient();
        v2 = v2.push_back(42);
        v2 = v2.push_back(42);
        v2 = v2.push_back(42);
        v2 = v2 + v2;
        v2 = v2.push_back(42);
        v2 = v2.push_back(42);
        v2 = v2.push_back(42);
        v0 = v2.drop(34);
        t0 = v0.transient();
        v0 = v0.push_back(42);
        v2 = v2.push_back(42);
        v2 = v2.push_back(42);
        v2 = v2.push_back(42);
        v2 = v2.push_back(42);
        v2 = v2.push_back(42);
        v0 = v2.drop(34);
        t0 = v0.transient();
        v0 = v0.take(87);
        v2 = v2.push_back(42);
        v2 = v2.push_back(42);
        v2 = v0.push_back(42);
        v0 = v0.take(87);
        v2 = v2.push_back(42);
        v2 = v2.push_back(42);
        v2 = v2.push_back(42);
        v2 = v2.push_back(42);
        v2 = v2.push_back(42);
        v0 = v2.drop(34);
        t0 = v0.transient();
        v0 = v0.push_back(42);
        v2 = v2.push_back(42);
        v2 = v2.push_back(42);
        v2 = v2.push_back(42);
        v2 = v2.push_back(42);
        v2 = v2.push_back(42);
        v0 = v2.drop(34);
        t0 = v0.transient();
        v0 = v0.push_back(42);
        v2 = v2.push_back(42);
        v2 = v2.push_back(42);
        v2 = v2.push_back(42);
        v2 = v2.push_back(42);
        v0 = v2.drop(34);
        t0 = v0.transient();
        v0 = v0.push_back(42);
        v2 = v2.push_back(42);
        v2 = v2.push_back(42);
        v2 = v2.push_back(42);
        v2 = v2.push_back(42);
        v2 = v2.push_back(42);
        v0 = v2.drop(34);
        t0 = v0.transient();
        v0 = v0.push_back(42);
        v2 = v2.push_back(42);
        v2 = v2.push_back(42);
        v2 = v2.push_back(42);
        v2 = v2.push_back(42);
        v2 = v2.push_back(42);
        v0 = v2.drop(34);
        t0 = v0.transient();
        v0 = v0.push_back(42);
        v2 = v2.push_back(42);
        v2 = v2.push_back(42);
        v2 = v2.push_back(42);
        v2 = v2.push_back(42);
        v2 = v2.push_back(42);
        v0 = v2.drop(34);
        t0 = v0.transient();
        v0 = v0.push_back(42);
        v2 = v2.push_back(42);
        v2 = v2.push_back(42);
        v2 = v2.push_back(42);
        v2 = v2.push_back(42);
        v2 = v2.push_back(42);
        v0 = v2.drop(34);
        t0 = v0.transient();
        v0 = v0.take(4);
        v0 = v0.push_back(42);
        v2 = v2.push_back(42);
        v2 = v2.push_back(42);
        v2 = v2.push_back(42);
        v2 = v2.push_back(42);
        v2 = v2.push_back(42);
        v0 = v2.drop(34);
        t0 = v0.transient();
        v0 = v0.take(87);
        v2 = v2.push_back(42);
        v2 = v2.push_back(42);
        v2 = v2.push_back(42);
        v2 = v2.push_back(42);
        v2 = v2.push_back(42);
        v0 = v2.drop(34);
        t0 = v0.transient();
        v0 = v0.push_back(42);
        v2 = v2.push_back(42);
        v2 = v2.push_back(42);
        v2 = v2.push_back(42);
        v2 = v2.push_back(42);
        v2 = v2.push_back(42);
        t0 = v0.transient();
        v0 = v0.push_back(42);
        v2 = v2.push_back(42);
        v2 = v2.push_back(42);
        v2 = v2.push_back(42);
        v2 = v2.push_back(42);
        v2 = v2.push_back(42);
        v0 = v2.drop(34);
        t0 = v0.transient();
        v0 = v0.push_back(42);
        v2 = v2.push_back(42);
        v2 = v2.push_back(42);
        v2 = v2.push_back(42);
        v2 = v2.push_back(42);
        v2 = v2.push_back(42);
        v0 = v2.drop(34);
        t0 = v0.transient();
        v0 = v0.push_back(42);
        v2 = v2.push_back(42);
        v2 = v2.push_back(42);
        v2 = v2.push_back(42);
        v2 = v2.push_back(42);
        v0 = v2.drop(34);
        t0 = v0.transient();
        v0 = v0.push_back(42);
        v2 = v2.push_back(42);
        v2 = v2.push_back(42);
        v2 = v2.push_back(42);
        v2 = v2.push_back(42);
        v2 = v2.push_back(42);
        v0 = v2.drop(34);
        t0 = v0.transient();
        t0 = v0.transient();
        v0 = v0.push_back(42);
        v2 = v2.push_back(42);
        v2 = v2.push_back(42);
        v2 = v0.push_back(42);
        t0 = v0.transient();
        t0 = v0.transient();
        t0 = v0.transient();
        t0 = v0.transient();
        t0 = v0.transient();
        t0 = v0.transient();
        t0 = v0.transient();
        t0 = v0.transient();
        v2 = v2.push_back(42);
        v2 = v2.push_back(42);
        v2 = v2.push_back(42);
        v2 = v2.push_back(42);
        v2 = v2.push_back(42);
        v0 = v2.drop(34);
        t0 = v0.transient();
        v0 = v0.push_back(42);
        v2 = v2.push_back(42);
        v2 = v2.push_back(42);
        v2 = v2.push_back(42);
        v2 = v2.push_back(42);
        v2 = v2.push_back(42);
        v0 = v2.drop(34);
        t0 = v0.transient();
        v0 = v0.take(87);
        v2 = v2.push_back(42);
        v2 = v2.push_back(42);
        v2 = v2.push_back(42);
        v2 = v2.push_back(42);
        v2 = v2.push_back(42);
        v0 = v2.drop(34);
        t0 = v0.transient();
        v0 = v0.push_back(42);
        v2 = v2.push_back(42);
        v2 = v2.push_back(42);
        v2 = v2.push_back(42);
        v2 = v2.push_back(42);
        v2 = v2.push_back(42);
        v0 = v2.drop(34);
        v2 = v2.push_back(42);
        v2 = v2.push_back(42);
        v0 = v2.drop(34);
        t0 = v0.transient();
        t1 = v0.transient();
        t0.push_back(13);
        t0.push_back(13);
        t3 = v0.transient();
        v0 = t0.persistent();
        t0 = v0.transient();
        t0.push_back(13);
        t0 = v0.transient();
        t0 = v0.transient();
        t0 = v0.transient();
        t0.push_back(13);
        t0 = v3.transient();
        t0 = v0.transient();
        t1 = v0.transient();
        t0.push_back(13);
        t0.push_back(13);
        t0.push_back(13);
        t0.push_back(13);
        t0.push_back(13);
        t3 = v0.transient();
        v0 = t0.persistent();
        t0.push_back(13);
        t0.push_back(13);
        t0.push_back(13);
        t0.push_back(13);
        t0.push_back(13);
        t3 = v0.transient();
        v0 = t0.persistent();
        t0.push_back(13);
        t0.push_back(13);
        t3 = v0.transient();
        v0 = t0.persistent();
        t0 = v0.transient();
        t0.push_back(13);
        t0 = v0.transient();
        t0.push_back(13);
        t0.push_back(13);
        t0.push_back(13);
        t3 = v0.transient();
        t0 = v0.transient();
        t0.push_back(13);
        t0.push_back(13);
        t0.update(1, [](auto x) { return 123; });
        t0.take(7);
        t0.append(v1.transient());
        t0.take(7);
        t0.append(t1);
    }

    SECTION("fuzzer")
    {
        auto input = load_input(
            "clusterfuzz-testcase-minimized-flex-vector-gc-5651513180160000");
        CHECK(run_input(input.data(), input.size()) == 0);
    }
}
