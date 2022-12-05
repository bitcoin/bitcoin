//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#include <immer/flex_vector.hpp>

#include <cassert>

int main()
{
    {
        // include:push-back/start
        auto v1 = immer::flex_vector<int>{1};
        auto v2 = v1.push_back(8);

        assert((v1 == immer::flex_vector<int>{1}));
        assert((v2 == immer::flex_vector<int>{1, 8}));
        // include:push-back/end
    }
    {
        // include:push-front/start
        auto v1 = immer::flex_vector<int>{1};
        auto v2 = v1.push_front(8);

        assert((v1 == immer::flex_vector<int>{1}));
        assert((v2 == immer::flex_vector<int>{8, 1}));
        // include:push-front/end
    }

    {
        // include:set/start
        auto v1 = immer::flex_vector<int>{1, 2, 3};
        auto v2 = v1.set(0, 5);

        assert((v1 == immer::flex_vector<int>{1, 2, 3}));
        assert((v2 == immer::flex_vector<int>{5, 2, 3}));
        // include:set/end
    }

    {
        // include:update/start
        auto v1 = immer::flex_vector<int>{1, 2, 3, 4};
        auto v2 = v1.update(2, [&](auto l) { return ++l; });

        assert((v1 == immer::flex_vector<int>{1, 2, 3, 4}));
        assert((v2 == immer::flex_vector<int>{1, 2, 4, 4}));
        // include:update/end
    }

    {
        // include:take/start
        auto v1 = immer::flex_vector<int>{1, 2, 3, 4, 5, 6};
        auto v2 = v1.take(3);

        assert((v1 == immer::flex_vector<int>{1, 2, 3, 4, 5, 6}));
        assert((v2 == immer::flex_vector<int>{1, 2, 3}));
        // include:take/end
    }

    {
        // include:drop/start
        auto v1 = immer::flex_vector<int>{1, 2, 3, 4, 5, 6};
        auto v2 = v1.drop(3);

        assert((v1 == immer::flex_vector<int>{1, 2, 3, 4, 5, 6}));
        assert((v2 == immer::flex_vector<int>{4, 5, 6}));
        // include:drop/end
    }

    {
        // include:insert/start
        auto v1 = immer::flex_vector<int>{1, 2, 3};
        auto v2 = v1.insert(0, 0);

        assert((v1 == immer::flex_vector<int>{1, 2, 3}));
        assert((v2 == immer::flex_vector<int>{0, 1, 2, 3}));
        // include:insert/end
    }

    {
        // include:erase/start
        auto v1 = immer::flex_vector<int>{1, 2, 3, 4, 5};
        auto v2 = v1.erase(2);

        assert((v1 == immer::flex_vector<int>{1, 2, 3, 4, 5}));
        assert((v2 == immer::flex_vector<int>{1, 2, 4, 5}));
        // include:erase/end
    }

    {
        // include:concat/start
        auto v1 = immer::flex_vector<int>{1, 2, 3};
        auto v2 = v1 + v1;

        assert((v1 == immer::flex_vector<int>{1, 2, 3}));
        assert((v2 == immer::flex_vector<int>{1, 2, 3, 1, 2, 3}));
        // include:concat/end
    }
}
