//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#include <immer/box.hpp>

#include <cassert>
#include <string>

int main()
{
    {
        // include:update/start
        auto v1 = immer::box<std::string>{"hello"};
        auto v2 = v1.update([&](auto l) { return l + ", world!"; });

        assert((v1 == immer::box<std::string>{"hello"}));
        assert((v2 == immer::box<std::string>{"hello, world!"}));
        // include:update/end
    }
}
