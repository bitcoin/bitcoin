//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#include <immer/refcount/no_refcount_policy.hpp>
#include <immer/refcount/refcount_policy.hpp>
#include <immer/refcount/unsafe_refcount_policy.hpp>

#include <catch2/catch.hpp>

TEST_CASE("no refcount has no data")
{
    static_assert(std::is_empty<immer::no_refcount_policy>{}, "");
}

template <typename RefcountPolicy>
void test_refcount()
{
    using refcount = RefcountPolicy;

    SECTION("starts at one")
    {
        refcount elem{};
        CHECK(elem.dec());
    }

    SECTION("disowned starts at zero")
    {
        refcount elem{immer::disowned{}};
        elem.inc();
        CHECK(elem.dec());
    }

    SECTION("inc dec")
    {
        refcount elem{};
        elem.inc();
        CHECK(!elem.dec());
        CHECK(elem.dec());
    }
}

TEST_CASE("basic refcount") { test_refcount<immer::refcount_policy>(); }

TEST_CASE("thread unsafe refcount")
{
    test_refcount<immer::unsafe_refcount_policy>();
}
