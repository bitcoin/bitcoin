//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#include <immer/vector.hpp>
#include <iostream>

// include:myiota/start
immer::vector<int> myiota(immer::vector<int> v, int first, int last)
{
    for (auto i = first; i < last; ++i)
        v = v.push_back(i);
    return v;
}
// include:myiota/end

int main()
{
    auto v = myiota({}, 0, 100);
    std::copy(v.begin(), v.end(), std::ostream_iterator<int>{std::cout, "\n"});
}
