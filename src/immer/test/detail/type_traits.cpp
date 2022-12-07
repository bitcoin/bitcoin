//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#include <catch.hpp>
#include <forward_list>
#include <immer/detail/type_traits.hpp>
#include <list>
#include <vector>

struct string_sentinel
{};
bool operator==(const char* i, string_sentinel);
bool operator!=(const char* i, string_sentinel);

TEST_CASE("compatible_sentinel_v")
{
    SECTION("iterator pairs")
    {
        using Iter = std::vector<int>::iterator;
        static_assert(immer::detail::compatible_sentinel_v<Iter, Iter>, "");
    }

    SECTION("pointer pairs")
    {
        using Iter = char*;
        static_assert(immer::detail::compatible_sentinel_v<Iter, Iter>, "");
    }

    SECTION("iterator/sentinel pair")
    {
        using Iter = char*;
        using Sent = string_sentinel;
        static_assert(immer::detail::compatible_sentinel_v<Iter, Sent>, "");
    }

    SECTION("incompatible pair")
    {
        using Iter1 = std::vector<int>::iterator;
        using Iter2 = std::list<double>::iterator;
        static_assert(not immer::detail::compatible_sentinel_v<Iter1, Iter2>,
                      "");
    }
}

TEST_CASE("equality comparable")
{
    SECTION("iterator pairs")
    {
        using Iter = std::vector<int>::iterator;
        static_assert(immer::detail::is_equality_comparable_v<Iter, Iter>, "");
    }

    SECTION("pointer pairs")
    {
        using Iter = char*;
        static_assert(immer::detail::is_equality_comparable_v<Iter, Iter>, "");
    }

    SECTION("iterator/sentinel pair")
    {
        using Iter = char*;
        using Sent = string_sentinel;
        static_assert(immer::detail::is_equality_comparable_v<Iter, Sent>, "");
    }

    SECTION("not equality comparable")
    {
        static_assert(
            not immer::detail::is_equality_comparable_v<std::string, double>,
            "");
    }
}

TEST_CASE("inequality comparable")
{
    SECTION("iterator pairs")
    {
        using Iter = std::vector<int>::iterator;
        static_assert(immer::detail::is_inequality_comparable_v<Iter, Iter>,
                      "");
    }

    SECTION("pointer pairs")
    {
        using Iter = char*;
        static_assert(immer::detail::is_inequality_comparable_v<Iter, Iter>,
                      "");
    }

    SECTION("iterator/sentinel pair")
    {
        using Iter = char*;
        using Sent = string_sentinel;
        static_assert(immer::detail::is_inequality_comparable_v<Iter, Sent>,
                      "");
    }

    SECTION("not inequality comparable")
    {
        static_assert(
            not immer::detail::is_inequality_comparable_v<std::string, double>,
            "");
    }
}

TEST_CASE("is dereferenceable")
{
    SECTION("iterator")
    {
        using Iter = std::vector<int>::iterator;
        static_assert(immer::detail::is_dereferenceable_v<Iter>, "");
    }

    SECTION("pointer")
    {
        using Iter = char*;
        static_assert(immer::detail::is_dereferenceable_v<Iter>, "");
    }

    SECTION("not dereferenceable")
    {
        static_assert(not immer::detail::is_dereferenceable_v<int>, "");
    }
}

TEST_CASE("is forward iterator")
{
    SECTION("random access iterator")
    {
        using Iter = std::vector<int>::iterator;
        static_assert(immer::detail::is_forward_iterator_v<Iter>, "");
    }

    SECTION("bidirectional iterator")
    {
        using Iter = std::list<int>::iterator;
        static_assert(immer::detail::is_forward_iterator_v<Iter>, "");
    }

    SECTION("forward iterator")
    {
        using Iter = std::forward_list<int>::iterator;
        static_assert(immer::detail::is_forward_iterator_v<Iter>, "");
    }

    SECTION("input iterator")
    {
        using Iter = std::istream_iterator<double>;
        static_assert(not immer::detail::is_forward_iterator_v<Iter>, "");
    }

    SECTION("output iterator")
    {
        using Iter = std::ostream_iterator<double>;
        static_assert(not immer::detail::is_forward_iterator_v<Iter>, "");
    }

    SECTION("pointer")
    {
        using Iter = char*;
        static_assert(immer::detail::is_forward_iterator_v<Iter>, "");
    }
}

TEST_CASE("is iterator")
{
    SECTION("iterator")
    {
        using Iter = std::vector<int>::iterator;
        static_assert(immer::detail::is_iterator_v<Iter>, "");
    }

    SECTION("pointer")
    {
        using Iter = char*;
        static_assert(immer::detail::is_iterator_v<Iter>, "");
    }

    SECTION("not iterator")
    {
        static_assert(not immer::detail::is_iterator_v<int>, "");
    }
}

TEST_CASE("provides preincrement")
{
    SECTION("iterator")
    {
        using Iter = std::vector<int>::iterator;
        static_assert(immer::detail::is_preincrementable_v<Iter>, "");
    }

    SECTION("pointer")
    {
        using Iter = char*;
        static_assert(immer::detail::is_preincrementable_v<Iter>, "");
    }

    SECTION("does not provide preincrement")
    {
        struct type
        {};
        static_assert(not immer::detail::is_preincrementable_v<type>, "");
    }
}

TEST_CASE("void_t")
{
    static_assert(std::is_same<void, immer::detail::void_t<int>>::value, "");
}
