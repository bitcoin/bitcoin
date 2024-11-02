//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#include "test/util.hpp"

#include <catch2/catch.hpp>

#ifndef SETUP_T
#error "define the table types via SETUP_T macro"
#endif

#include <string>

struct Item
{
    std::string id{};
    int value{};

    bool operator==(const Item& other) const
    {
        return value == other.value && id == other.id;
    }
};

IMMER_RANGES_CHECK(std::ranges::forward_range<SETUP_T::table<Item>>);
IMMER_RANGES_CHECK(std::ranges::forward_range<SETUP_T::table_transient<Item>>);

TEST_CASE("instantiate")
{
    auto t = SETUP_T::table_transient<Item>{};
    auto m = SETUP_T::table<Item>{};
    CHECK(t.persistent() == m);
    CHECK(t.persistent() == m.transient().persistent());
}

TEST_CASE("access")
{
    auto m = SETUP_T::table<Item>{Item{"foo", 12}, Item{"bar", 42}};
    auto t = m.transient();
    CHECK(t.size() == 2);
    CHECK(t.count("foo") == 1);
    CHECK(t["foo"].value == 12);
    CHECK(t.at("foo").value == 12);
    CHECK(t.find("foo") == m.find("foo"));
    CHECK(std::accumulate(t.begin(), t.end(), 0, [](auto acc, auto&& x) {
              return acc + x.value;
          }) == 54);
}

TEST_CASE("insert")
{
    auto t = SETUP_T::table_transient<Item>{};

    t.insert(Item{"foo", 42});
    CHECK(t["foo"].value == 42);
    CHECK(t.size() == 1);

    t.insert(Item{"bar", 13});
    CHECK(t["bar"].value == 13);
    CHECK(t.size() == 2);

    t.insert(Item{"foo", 6});
    CHECK(t["foo"].value == 6);
    CHECK(t.size() == 2);

    t.update("foo", [](auto item) {
        item.value += 1;
        return item;
    });
    CHECK(t["foo"].value == 7);
    CHECK(t.size() == 2);

    t.update("lol", [](auto item) {
        item.value += 1;
        return item;
    });
    CHECK(t["lol"].value == 1);
    CHECK(t.size() == 3);

    t.update_if_exists("foo", [](auto item) {
        item.value += 1;
        return item;
    });
    CHECK(t["foo"].value == 8);
    CHECK(t.size() == 3);
}

TEST_CASE("erase")
{
    auto t = SETUP_T::table<Item>{{"foo", 12}, {"bar", 42}}.transient();

    t.erase("foo");
    CHECK(t.find("foo") == nullptr);
    CHECK(t.count("foo") == 0);
    CHECK(t.find("bar") != nullptr);
    CHECK(t.count("bar") == 1);
    CHECK(t.size() == 1);
}
