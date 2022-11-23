//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#include <immer/array.hpp>

#include <cassert>

int main()
{
    {
        // include:push-back/start
        auto v1 = immer::array<int>{1};
        auto v2 = v1.push_back(8);

        assert((v1 == immer::array<int>{1}));
        assert((v2 == immer::array<int>{1, 8}));
        // include:push-back/end
    }

    {
        // include:set/start
        auto v1 = immer::array<int>{1, 2, 3};
        auto v2 = v1.set(0, 5);

        assert((v1 == immer::array<int>{1, 2, 3}));
        assert((v2 == immer::array<int>{5, 2, 3}));
        // include:set/end
    }

    {
        // include:update/start
        auto v1 = immer::array<int>{1, 2, 3, 4};
        auto v2 = v1.update(2, [&](auto l) { return ++l; });

        assert((v1 == immer::array<int>{1, 2, 3, 4}));
        assert((v2 == immer::array<int>{1, 2, 4, 4}));
        // include:update/end
    }

    {
        // include:take/start
        auto v1 = immer::array<int>{1, 2, 3, 4, 5, 6};
        auto v2 = v1.take(3);

        assert((v1 == immer::array<int>{1, 2, 3, 4, 5, 6}));
        assert((v2 == immer::array<int>{1, 2, 3}));
        // include:take/end
    }
}
