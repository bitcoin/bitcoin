//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#include <cassert>
#include <immer/vector.hpp>

// include:move-bad/start
immer::vector<int> do_stuff(const immer::vector<int> v)
{
    return std::move(v).push_back(42);
}
// include:move-bad/end

// include:move-good/start
immer::vector<int> do_stuff_better(immer::vector<int> v)
{
    return std::move(v).push_back(42);
}
// include:move-good/end

int main()
{
    auto v  = immer::vector<int>{};
    auto v1 = do_stuff(v);
    auto v2 = do_stuff_better(v);
    assert(v1.size() == 1);
    assert(v2.size() == 1);
    assert(v1[0] == 42);
    assert(v2[0] == 42);
}
