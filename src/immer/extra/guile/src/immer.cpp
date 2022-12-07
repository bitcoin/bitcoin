//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#include <immer/algorithm.hpp>
#include <immer/flex_vector.hpp>
#include <immer/flex_vector_transient.hpp>
#include <iostream>
#include <scm/scm.hpp>

namespace {

struct guile_heap
{
    static void* allocate(std::size_t size)
    {
        return scm_gc_malloc(size, "immer");
    }

    static void* allocate(std::size_t size, immer::norefs_tag)
    {
        return scm_gc_malloc_pointerless(size, "immer");
    }

    template <typename... Tags>
    static void deallocate(std::size_t size, void* obj, Tags...)
    {
        scm_gc_free(obj, size, "immer");
    }
};

using guile_memory = immer::memory_policy<immer::heap_policy<guile_heap>,
                                          immer::no_refcount_policy,
                                          immer::default_lock_policy,
                                          immer::gc_transience_policy,
                                          false>;

template <typename T>
using guile_ivector = immer::flex_vector<T, guile_memory>;

struct dummy
{
    SCM port_ = scm_current_warning_port();

    dummy(dummy&&) { scm_puts("~~ dummy move constructor\n", port_); }

    dummy() { scm_puts("~~ dummy default constructor\n", port_); }

    ~dummy() { scm_puts("~~ dummy finalized\n", port_); }

    void foo() { scm_puts("~~ dummy foo\n", port_); }

    int bar(int x)
    {
        auto res = x + 42;
        scm_puts("~~ dummy bar: ", port_);
        scm_display(scm::val{res}, port_);
        scm_newline(port_);
        return res;
    }
};

template <int I>
void func()
{
    auto port = scm_current_warning_port();
    scm_puts("~~ func", port);
    scm_display(scm_from_int(I), port);
    scm_newline(port);
}

template <typename T = scm::val>
void init_ivector(std::string type_name = "")
{
    using namespace std::string_literals;

    using self_t = guile_ivector<T>;
    using size_t = typename self_t::size_type;

    auto name = "ivector"s + (type_name.empty() ? ""s : "-" + type_name);

    scm::type<self_t>(name)
        .constructor(
            [](scm::args rest) { return self_t(rest.begin(), rest.end()); })
        .maker([](size_t n, scm::args rest) {
            return self_t(n, rest ? *rest : scm::val{});
        })
        .define("ref", &self_t::operator[])
        .define("length", &self_t::size)
        .define(
            "set",
            [](const self_t& v, size_t i, scm::val x) { return v.set(i, x); })
        .define("update",
                [](const self_t& v, size_t i, scm::val fn) {
                    return v.update(i, fn);
                })
        .define("push",
                [](const self_t& v, scm::val x) { return v.push_back(x); })
        .define("take", [](const self_t& v, size_t s) { return v.take(s); })
        .define("drop", [](const self_t& v, size_t s) { return v.drop(s); })
        .define("append",
                [](self_t v, scm::args rest) {
                    for (auto x : rest)
                        v = v + x;
                    return v;
                })
        .define("fold", [](scm::val fn, scm::val first, const self_t& v) {
            return immer::accumulate(v, first, fn);
        });
}

} // anonymous namespace

struct bar_tag_t
{};

extern "C" void init_immer()
{
    scm::type<dummy>("dummy")
        .constructor()
        .finalizer()
        .define("foo", &dummy::foo)
        .define("bar", &dummy::bar);

    scm::group().define("func1", func<1>);

    scm::group<bar_tag_t>()
        .define("func2", func<2>)
        .define("func3", &dummy::bar);

    scm::group("foo").define("func1", func<1>);

    init_ivector();
    init_ivector<std::uint8_t>("u8");
    init_ivector<std::uint16_t>("u16");
    init_ivector<std::uint32_t>("u32");
    init_ivector<std::uint64_t>("u64");
    init_ivector<std::int8_t>("s8");
    init_ivector<std::int16_t>("s16");
    init_ivector<std::int32_t>("s32");
    init_ivector<std::int64_t>("s64");
    init_ivector<float>("f32");
    init_ivector<double>("f64");
}
