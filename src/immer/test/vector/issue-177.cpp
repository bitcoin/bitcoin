//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

// Thanks to @jeaye for reporting this issue
// https://github.com/arximboldi/immer/issues/177

#include <immer/box.hpp>
#include <immer/vector.hpp>
#include <immer/vector_transient.hpp>

#include <catch2/catch.hpp>

struct object;

template <typename T>
struct is_valid_type
{
    static bool constexpr value{false};
};
template <>
struct is_valid_type<object>
{
    static bool constexpr value{true};
};

struct object
{
    using vector_type = immer::vector<immer::box<object>>;

    template <typename T>
    object(T&&)
    {
        static_assert(is_valid_type<T>::value, "invalid type");
    }
};

TEST_CASE("strange bad constructor called")
{
    object::vector_type::transient_type t;
    // If this next line is commented out, the code compiles.
    (void) t.persistent();
}
