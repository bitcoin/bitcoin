//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//
#include <string>
// include:intro/start
#include <immer/table.hpp>

int main()
{
    struct Item
    {
        std::string id;
        int value;
    };

    const auto v0 = immer::table<Item>{};
    const auto v1 = v0.insert({"hello", 42});
    assert(v0["hello"].value == 0);
    assert(v1["hello"].value == 42);

    const auto v2 = v1.erase("hello");
    assert(v1.find("hello")->value == 42);
    assert(!v2.find("hello"));
}
// include:intro/end
