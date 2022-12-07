//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#include <immer/array.hpp>
#include <immer/array_transient.hpp>

#define VECTOR_T ::immer::array
#define VECTOR_TRANSIENT_T ::immer::array_transient

#include "../vector_transient/generic.ipp"

TEST_CASE("array_transient default constructor compiles")
{
    immer::array_transient<int> transient;
}

TEST_CASE("array provides mutable data")
{
    auto arr = immer::array<int>(10, 0);
    CHECK(arr.size() == 10);
    auto tr = arr.transient();
    CHECK(tr.data() == arr.data());

    auto d = tr.data_mut();
    CHECK(tr.data_mut() != arr.data());
    CHECK(tr.data() == tr.data_mut());
    CHECK(arr.data() != tr.data_mut());

    arr = tr.persistent();
    CHECK(arr.data() == d);
    CHECK(arr.data() == tr.data());

    CHECK(tr.data_mut() != arr.data());
    CHECK(tr.data() == tr.data_mut());
    CHECK(arr.data() != tr.data_mut());
}
