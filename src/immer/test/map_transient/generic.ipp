//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#include "test/util.hpp"

#include <catch2/catch.hpp>

#ifndef MAP_T
#error "define the map template to use in MAP_T"
#endif

#ifndef MAP_TRANSIENT_T
#error "define the map template to use in MAP_TRANSIENT_T"
#endif

IMMER_RANGES_CHECK(std::ranges::forward_range<MAP_T<std::string, std::string>>);
IMMER_RANGES_CHECK(
    std::ranges::forward_range<MAP_TRANSIENT_T<std::string, std::string>>);

TEST_CASE("instantiate")
{
    auto t = MAP_TRANSIENT_T<std::string, int>{};
    auto m = MAP_T<std::string, int>{};
    CHECK(t.persistent() == m);
    CHECK(t.persistent() == m.transient().persistent());
}

TEST_CASE("access")
{
    auto m = MAP_T<std::string, int>{{"foo", 12}, {"bar", 42}};
    auto t = m.transient();
    CHECK(t.size() == 2);
    CHECK(t.count("foo") == 1);
    CHECK(t["foo"] == 12);
    CHECK(t.at("foo") == 12);
    CHECK(t.find("foo") == m.find("foo"));
    CHECK(std::accumulate(t.begin(), t.end(), 0, [](auto acc, auto&& x) {
              return acc + x.second;
          }) == 54);
}

TEST_CASE("insert")
{
    auto t = MAP_TRANSIENT_T<std::string, int>{};

    t.insert({"foo", 42});
    CHECK(t["foo"] == 42);
    CHECK(t.size() == 1);

    t.insert({"bar", 13});
    CHECK(t["bar"] == 13);
    CHECK(t.size() == 2);

    t.insert({"foo", 6});
    CHECK(t["foo"] == 6);
    CHECK(t.size() == 2);
}

TEST_CASE("set")
{
    auto t = MAP_TRANSIENT_T<std::string, int>{};

    t.set("foo", 42);
    CHECK(t["foo"] == 42);
    CHECK(t.size() == 1);

    t.set("bar", 13);
    CHECK(t["bar"] == 13);
    CHECK(t.size() == 2);

    t.set("foo", 6);
    CHECK(t["foo"] == 6);
    CHECK(t.size() == 2);
}

TEST_CASE("update")
{
    auto t = MAP_TRANSIENT_T<std::string, int>{};

    t.update("foo", [](auto x) { return x + 42; });
    CHECK(t["foo"] == 42);
    CHECK(t.size() == 1);

    t.update("bar", [](auto x) { return x + 13; });
    CHECK(t["bar"] == 13);
    CHECK(t.size() == 2);

    t.update("foo", [](auto x) { return x + 6; });
    CHECK(t["foo"] == 48);
    CHECK(t.size() == 2);

    t.update_if_exists("foo", [](auto x) { return x + 42; });
    CHECK(t["foo"] == 90);
    CHECK(t.size() == 2);

    t.update_if_exists("manolo", [](auto x) { return x + 42; });
    CHECK(t["manolo"] == 0);
    CHECK(t.size() == 2);
}

TEST_CASE("erase")
{
    auto t = MAP_T<std::string, int>{{"foo", 12}, {"bar", 42}}.transient();

    t.erase("foo");
    CHECK(t.find("foo") == nullptr);
    CHECK(t.count("foo") == 0);
    CHECK(t.find("bar") != nullptr);
    CHECK(t.count("bar") == 1);
    CHECK(t.size() == 1);
}
