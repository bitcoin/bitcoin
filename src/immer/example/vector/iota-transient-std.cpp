//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#include <immer/vector.hpp>
#include <immer/vector_transient.hpp>

#include <algorithm>
#include <iostream>

// include:myiota/start
immer::vector<int> myiota(immer::vector<int> v, int first, int last)
{
    auto t = v.transient();
    std::generate_n(
        std::back_inserter(t), last - first, [&] { return first++; });
    return t.persistent();
}
// include:myiota/end

int main()
{
    auto v = myiota({}, 0, 100);
    std::copy(v.begin(), v.end(), std::ostream_iterator<int>{std::cout, "\n"});
}
