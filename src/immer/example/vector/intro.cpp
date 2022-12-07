//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

// include:intro/start
#include <immer/vector.hpp>
int main()
{
    const auto v0 = immer::vector<int>{};
    const auto v1 = v0.push_back(13);
    assert((v0 == immer::vector<int>{}));
    assert((v1 == immer::vector<int>{13}));

    const auto v2 = v1.set(0, 42);
    assert(v1[0] == 13);
    assert(v2[0] == 42);
}
// include:intro/end
