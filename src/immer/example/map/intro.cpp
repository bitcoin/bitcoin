//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#include <string>
// include:intro/start
#include <immer/map.hpp>
int main()
{
    const auto v0 = immer::map<std::string, int>{};
    const auto v1 = v0.set("hello", 42);
    assert(v0["hello"] == 0);
    assert(v1["hello"] == 42);

    const auto v2 = v1.erase("hello");
    assert(*v1.find("hello") == 42);
    assert(!v2.find("hello"));
}
// include:intro/end
