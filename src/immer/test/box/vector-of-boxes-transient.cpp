//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#include <immer/box.hpp>
#include <immer/vector.hpp>
#include <immer/vector_transient.hpp>

#include <catch.hpp>

TEST_CASE("issue-33")
{
    using Element = immer::box<std::string>;
    auto vect     = immer::vector<Element>{};

    // this one works fine
    for (auto i = 0; i < 100; ++i) {
        vect = vect.push_back(Element("x"));
    }

    // this one doesn't compile
    auto t = vect.transient();

    for (auto i = 0; i < 100; ++i) {
        t.push_back(Element("x"));
    }

    vect = t.persistent();

    CHECK(*vect[0] == "x");
    CHECK(*vect[99] == "x");
}
