//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

// Thanks Guiguiprim for reporting this issue
// https://github.com/arximboldi/immer/issues/46

#include <immer/flex_vector.hpp>
#include <immer/vector.hpp>
#include <immer/vector_transient.hpp>

#include <catch2/catch.hpp>

#if IMMER_CXX_STANDARD >= 17

#include <variant>

TEST_CASE("error when erasing an element from a "
          "immer::flex_vector<std::variant/optional/any>")
{
    using Vector = immer::flex_vector<std::variant<int, double>>;
    // using Vector = immer::flex_vector<std::optional<int>>;
    // using Vector = immer::flex_vector<std::any>;

    Vector v{1, 2, 3, 4};
    Vector v2 = v.erase(2);

    CHECK(v2.size() == 3);
}

#endif
