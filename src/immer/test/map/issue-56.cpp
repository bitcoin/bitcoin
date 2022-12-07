//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

// Thanks @dgel for reporting this issue
// https://github.com/arximboldi/immer/issues/56

#include <immer/flex_vector.hpp>
#include <immer/map.hpp>
#include <immer/vector.hpp>

#include <catch.hpp>

TEST_CASE("const map")
{
    const auto x = immer::map<std::string, int>{}.set("A", 1);
    auto it      = x.begin();
    CHECK(it->first == "A");
    CHECK(it->second == 1);
}

TEST_CASE("const vector")
{
    const auto x =
        immer::vector<std::pair<std::string, int>>{}.push_back({"A", 1});
    auto it = x.begin();
    CHECK(it->first == "A");
    CHECK(it->second == 1);
}

TEST_CASE("const flex vector")
{
    const auto x =
        immer::flex_vector<std::pair<std::string, int>>{}.push_back({"A", 1});
    auto it = x.begin();
    CHECK(it->first == "A");
    CHECK(it->second == 1);
}
