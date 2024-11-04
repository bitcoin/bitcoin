//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#include "test/util.hpp"

#include <catch2/catch.hpp>

#ifndef SET_T
#error "define the set template to use in SET_T"
#endif

#ifndef SET_TRANSIENT_T
#error "define the set template to use in SET_TRANSIENT_T"
#endif

IMMER_RANGES_CHECK(std::ranges::forward_range<SET_T<std::string>>);
IMMER_RANGES_CHECK(std::ranges::forward_range<SET_TRANSIENT_T<std::string>>);

TEST_CASE("instantiate")
{
    auto t = SET_TRANSIENT_T<int>{};
    auto m = SET_T<int>{};
    CHECK(t.persistent() == m);
    CHECK(t.persistent() == m.transient().persistent());
}

TEST_CASE("access")
{
    auto m = SET_T<int>{12, 42};
    auto t = m.transient();
    CHECK(t.size() == 2);
    CHECK(t.count(42) == 1);
    CHECK(*t.find(12) == 12);
    CHECK(std::accumulate(t.begin(), t.end(), 0) == 54);
}

TEST_CASE("insert")
{
    auto t = SET_TRANSIENT_T<int>{};

    t.insert(42);
    CHECK(*t.find(42) == 42);
    CHECK(t.size() == 1);

    t.insert(13);
    CHECK(*t.find(13) == 13);
    CHECK(t.size() == 2);

    t.insert(42);
    CHECK(*t.find(42) == 42);
    CHECK(t.size() == 2);
}

TEST_CASE("erase")
{
    auto t = SET_T<int>{12, 42}.transient();

    t.erase(42);
    CHECK(t.find(42) == nullptr);
    CHECK(t.count(42) == 0);
    CHECK(t.find(12) != nullptr);
    CHECK(t.count(12) == 1);
    CHECK(t.size() == 1);
}

TEST_CASE("insert erase many")
{
    auto t = SET_T<int>{}.transient();
    auto n = 1000;
    for (auto i = 0; i < n; ++i) {
        t.insert(i);
        CHECK(t.find(i) != nullptr);
        CHECK(t.size() == static_cast<std::size_t>(i + 1));
    }
    for (auto i = 0; i < n; ++i) {
        t.erase(i);
        CHECK(t.find(i) == nullptr);
        CHECK(t.size() == static_cast<std::size_t>(n - i - 1));
    }
}

TEST_CASE("erase many from non transient")
{
    const auto n = 10000;
    auto t       = [] {
        auto t = SET_T<int>{};
        for (auto i = 0; i < n; ++i) {
            t = t.insert(i);
            CHECK(t.find(i) != nullptr);
            CHECK(t.size() == static_cast<std::size_t>(i + 1));
        }
        return t.transient();
    }();
    for (auto i = 0; i < n; ++i) {
        t.erase(i);
        CHECK(t.find(i) == nullptr);
        CHECK(t.size() == static_cast<std::size_t>(n - i - 1));
    }
}
