//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

// Thanks Guiguiprim for reporting this issue
// https://github.com/arximboldi/immer/issues/46

#include <immer/flex_vector.hpp>
#include <immer/vector.hpp>
#include <immer/vector_transient.hpp>

#include <catch.hpp>

TEST_CASE("operator==() may return bad result")
{
    using bool_vec = immer::flex_vector<bool>;

    immer::vector<bool_vec> v0;
    auto tv = v0.transient();
    tv.push_back(bool_vec(9, false));
    tv.push_back(bool_vec(10, false));
    tv.push_back(bool_vec(8, false));
    tv.push_back(bool_vec(6, false));
    tv.push_back(bool_vec(9, false));
    tv.push_back(bool_vec(7, false));
    tv.push_back(bool_vec(8, false));
    tv.push_back(bool_vec(9, false));
    tv.push_back(bool_vec(10, false));
    v0 = tv.persistent();

    auto v1 = v0.update(1, [](bool_vec vec) { return vec.set(8, true); });

    CHECK(v0[1] != v1[1]);
    CHECK(v0 != v1);
}
