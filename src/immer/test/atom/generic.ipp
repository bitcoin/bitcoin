//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#ifndef ATOM_T
#error "define the box template to use in ATOM_T"
#endif

#include <catch.hpp>

template <typename T>
using BOX_T = typename ATOM_T<T>::box_type;

TEST_CASE("construction") { ATOM_T<int> x; }

TEST_CASE("store, load, exchange")
{
    ATOM_T<int> x;
    CHECK(x.load() == BOX_T<int>{0});
    x.store(42);
    CHECK(x.load() == BOX_T<int>{42});
    auto old = x.exchange(12);
    CHECK(old == 42);
    CHECK(x.load() == BOX_T<int>{12});
}

TEST_CASE("box conversion")
{
    ATOM_T<int> x;
    auto v1 = BOX_T<int>{42};
    x       = v1;
    auto v2 = BOX_T<int>{x};
    CHECK(v1 == v2);
}

TEST_CASE("value conversion")
{
    ATOM_T<int> x;
    x      = 42;
    auto v = int{x};
    CHECK(v == 42);
}

TEST_CASE("update")
{
    ATOM_T<int> x{42};
    x.update([](auto x) { return x + 2; });
    CHECK(x.load() == 44);
}
