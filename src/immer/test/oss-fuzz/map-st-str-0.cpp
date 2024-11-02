//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#include "input.hpp"

#include <immer/heap/gc_heap.hpp>
#include <immer/map.hpp>
#include <immer/refcount/no_refcount_policy.hpp>

#include <immer/algorithm.hpp>

#include <array>

#include <catch2/catch.hpp>

#define IMMER_FUZZED_TRACE_ENABLE 1

#if IMMER_FUZZED_TRACE_ENABLE
#include <fmt/ostream.h>
#define IMMER_FUZZED_TRACE(...) fmt::print(std::cerr, __VA_ARGS__)
#else
#define IMMER_FUZZED_TRACE(...)
#endif

namespace {

using st_memory = immer::memory_policy<immer::heap_policy<immer::cpp_heap>,
                                       immer::unsafe_refcount_policy,
                                       immer::no_lock_policy,
                                       immer::no_transience_policy,
                                       false>;

struct colliding_hash_t
{
    std::size_t operator()(const std::string& x) const
    {
        return std::hash<std::string>{}(x) & ~15;
    }
};

int run_input(const std::uint8_t* data, std::size_t size)
{
    constexpr auto var_count = 4;

    using map_t = immer::map<std::string,
                             std::string,
                             colliding_hash_t,
                             std::equal_to<>,
                             st_memory>;

    auto vars = std::array<map_t, var_count>{};

#if IMMER_FUZZED_TRACE_ENABLE
    IMMER_FUZZED_TRACE("/// new test run\n");
    IMMER_FUZZED_TRACE("using map_t = immer::map<std::string, std::string,"
                       "colliding_hash_t, std::equal_to<>, st_memory>;\n");
    for (auto i = 0u; i < var_count; ++i)
        IMMER_FUZZED_TRACE("auto v{} = map_t{{}};\n", i);
#endif

    auto is_valid_var = [&](auto idx) { return idx >= 0 && idx < var_count; };

    return fuzzer_input{data, size}.run([&](auto& in) {
        enum ops
        {
            op_set,
            op_erase,
            op_set_move,
            op_erase_move,
            op_iterate,
            op_find,
            op_update,
            op_update_move,
            op_diff
        };
        auto src = read<char>(in, is_valid_var);
        auto dst = read<char>(in, is_valid_var);
        IMMER_FUZZED_TRACE("CHECK(v{}.impl().check_champ());\n", +src);
        CHECK(vars[src].impl().check_champ());
        switch (read<char>(in)) {
        case op_set: {
            auto value = std::to_string(read<size_t>(in));
            IMMER_FUZZED_TRACE(
                "v{} = v{}.set(\"{}\", \"foo\");\n", +dst, +src, value);
            vars[dst] = vars[src].set(value, "foo");
            break;
        }
        case op_erase: {
            auto value = std::to_string(read<size_t>(in));
            IMMER_FUZZED_TRACE("v{} = v{}.erase(\"{}\");\n", +dst, +src, value);
            vars[dst] = vars[src].erase(value);
            break;
        }
        case op_set_move: {
            auto value = std::to_string(read<size_t>(in));
            IMMER_FUZZED_TRACE("v{} = std::move(v{}).set(\"{}\", \"foo\");\n",
                               +dst,
                               +src,
                               value);
            vars[dst] = std::move(vars[src]).set(value, "foo");
            break;
        }
        case op_erase_move: {
            auto value = std::to_string(read<size_t>(in));
            IMMER_FUZZED_TRACE(
                "v{} = std::move(v{}).erase(\"{}\");\n", +dst, +src, value);
            vars[dst] = std::move(vars[src]).erase(value);
            break;
        }
        case op_iterate: {
            IMMER_FUZZED_TRACE("!!!TODO;\n");
            auto srcv = vars[src];
            for (const auto& v : srcv) {
                vars[dst] = vars[dst].set(v.first, v.second);
            }
            break;
        }
        case op_find: {
            IMMER_FUZZED_TRACE("!!!TODO;\n");
            auto value = std::to_string(read<size_t>(in));
            auto res   = vars[src].find(value);
            if (res != nullptr) {
                vars[dst] = vars[dst].set(*res, "foo");
            }
            break;
        }
        case op_update: {
            auto key = std::to_string(read<size_t>(in));
            IMMER_FUZZED_TRACE("v{} = v{}.update(\"{}\", [] (auto x) {{ "
                               "return x + \"bar\"; }});\n",
                               +dst,
                               +src,
                               key);
            vars[dst] = vars[src].update(
                key, [](std::string x) { return std::move(x) + "bar"; });
            break;
        }
        case op_update_move: {
            auto key = std::to_string(read<size_t>(in));
            IMMER_FUZZED_TRACE(
                "v{} = std::move(v{}).update(\"{}\", [] (auto x) {{ "
                "return x + \"bar\"; }});\n",
                +dst,
                +src,
                key);
            vars[dst] = std::move(vars[src]).update(
                key, [](std::string x) { return std::move(x) + "baz"; });
            break;
        }
        case op_diff: {
            IMMER_FUZZED_TRACE("!!!TODO;\n");
            auto&& a = vars[src];
            auto&& b = vars[dst];
            diff(
                a,
                b,
                [&](auto&& x) {
                    assert(!a.count(x.first));
                    assert(b.count(x.first));
                },
                [&](auto&& x) {
                    assert(a.count(x.first));
                    assert(!b.count(x.first));
                },
                [&](auto&& x, auto&& y) {
                    assert(x.first == y.first);
                    assert(x.second != y.second);
                });
        }
        default:
            break;
        };
        return true;
    });
}

} // namespace

TEST_CASE("local test runs")
{
    SECTION("fuzzer")
    {
        auto input =
            load_input("crash-dc9dad6beae69a6bb8ffd6d203b95032f445ec9b");
        CHECK(run_input(input.data(), input.size()) == 0);
        (void) run_input;
    }

    SECTION("simplify")
    {
        using map_t = immer::map<std::string,
                                 std::string,
                                 colliding_hash_t,
                                 std::equal_to<>,
                                 st_memory>;
        auto v0     = map_t{};
        auto v3     = map_t{};

        v0 = std::move(v0)
                 .set("256", "foo")
                 .set("217020539959771201", "foo")
                 .set("91201394110889985", "foo")
                 .set("217020518514230019", "foo");
        v3 = v0;
        v0 = v0.set("0", "foo");
        CHECK(v0.impl().check_champ());
        CHECK(v3.impl().check_champ());

        v3 = std::move(v3).erase("217020518514230019"); // here
        CHECK(v3.impl().check_champ());
        CHECK(v0.impl().check_champ());
    }
}
