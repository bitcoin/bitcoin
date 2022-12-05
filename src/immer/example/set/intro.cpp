//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

// include:intro/start
#include <immer/set.hpp>
int main()
{
    const auto v0 = immer::set<int>{};
    const auto v1 = v0.insert(42);
    assert(v0.count(42) == 0);
    assert(v1.count(42) == 1);

    const auto v2 = v1.erase(42);
    assert(v1.count(42) == 1);
    assert(v2.count(42) == 0);
}
// include:intro/end
