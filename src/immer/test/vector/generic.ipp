//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#include "test/dada.hpp"
#include "test/util.hpp"

#include <immer/algorithm.hpp>

#include <boost/range/adaptors.hpp>
#include <catch.hpp>

#include <algorithm>
#include <numeric>
#include <string>
#include <vector>

using namespace std::string_literals;

#ifndef VECTOR_T
#error "define the vector template to use in VECTOR_T"
#endif

template <typename V = VECTOR_T<unsigned>>
auto make_test_vector(unsigned min, unsigned max)
{
    auto v = V{};
    for (auto i = min; i < max; ++i)
        v = v.push_back({i});
    return v;
}

struct big_object
{
    std::array<std::size_t, 42> v;
};

struct string_sentinel
{};

bool operator==(const char16_t* i, string_sentinel) { return *i == '\0'; }

bool operator!=(const char16_t* i, string_sentinel) { return *i != '\0'; }

TEST_CASE("instantiation")
{
    SECTION("default")
    {
        auto v = VECTOR_T<int>{};
        CHECK(v.size() == 0u);
        CHECK(v.empty());
    }

    SECTION("initializer list")
    {
        auto v = VECTOR_T<unsigned>{0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
        CHECK_VECTOR_EQUALS(v, boost::irange(0u, 10u));
        CHECK(!v.empty());
    }

    SECTION("big object")
    {
        auto v = VECTOR_T<big_object>{{}, {}, {}, {}};
        CHECK(v.size() == 4);
    }

    SECTION("range")
    {
        auto r = std::vector<int>{{0, 1, 2, 3, 4, 5, 6, 7, 8, 9}};
        auto v = VECTOR_T<unsigned>{r.begin(), r.end()};
        CHECK_VECTOR_EQUALS(v, boost::irange(0u, 10u));
    }

    SECTION("empty range")
    {
        auto r = std::vector<int>{};
        auto v = VECTOR_T<unsigned>{r.begin(), r.end()};
        CHECK(v.size() == 0);
    }

    SECTION("iterator/sentinel")
    {
        auto r = u"012345678";
        string_sentinel s;
        auto v = VECTOR_T<unsigned>{r, s};
        CHECK_VECTOR_EQUALS(v, boost::irange(u'0', u'9'));
    }

    SECTION("fill")
    {
        auto v1 = VECTOR_T<int>(4);
        CHECK(v1.size() == 4);
        auto v2 = VECTOR_T<int>(4, 42);
        CHECK(v2.size() == 4);
        CHECK(v2[2] == 42);
    }
}

TEST_CASE("back and front")
{
    auto v = VECTOR_T<unsigned>{0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    CHECK(v.front() == 0);
    CHECK(v.back() == 9);
}

TEST_CASE("at")
{
    auto v = VECTOR_T<unsigned>{0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    CHECK(v.at(0) == 0);
    CHECK(v.at(5) == 5);
#ifndef IMMER_NO_EXCEPTIONS
    CHECK_THROWS_AS(v.at(10), std::out_of_range);
    CHECK_THROWS_AS(v.at(11), std::out_of_range);
#endif
}

TEST_CASE("push back one element")
{
    SECTION("one element")
    {
        const auto v1 = VECTOR_T<int>{};
        auto v2       = v1.push_back(42);
        CHECK(v1.size() == 0u);
        CHECK(v2.size() == 1u);
        CHECK(v2[0] == 42);
    }

    SECTION("many elements")
    {
        const auto n = 666u;
        auto v       = VECTOR_T<unsigned>{};
        for (auto i = 0u; i < n; ++i) {
            v = v.push_back(i * 42);
            CHECK(v.size() == i + 1);
            for (decltype(v.size()) j = 0; j < v.size(); ++j)
                CHECK(v[j] == j * 42);
        }
    }
}

TEST_CASE("update")
{
    const auto n = 42u;
    auto v       = make_test_vector(0, n);

    SECTION("set")
    {
        const auto u = v.set(3u, 13u);
        CHECK(u.size() == v.size());
        CHECK(u[2u] == 2u);
        CHECK(u[3u] == 13u);
        CHECK(u[4u] == 4u);
        CHECK(u[40u] == 40u);
        CHECK(v[3u] == 3u);
    }

    SECTION("set further")
    {
        auto v = make_test_vector(0, 666);

        auto u = v.set(3u, 13u);
        u      = u.set(200u, 7u);
        CHECK(u.size() == v.size());

        CHECK(u[2u] == 2u);
        CHECK(u[4u] == 4u);
        CHECK(u[40u] == 40u);
        CHECK(u[600u] == 600u);

        CHECK(u[3u] == 13u);
        CHECK(u[200u] == 7u);

        CHECK(v[3u] == 3u);
        CHECK(v[200u] == 200u);
    }

    SECTION("set further more")
    {
        auto v = make_test_vector(0, 666u);

        for (decltype(v.size()) i = 0; i < v.size(); ++i) {
            v = v.set(i, i + 1);
            CHECK(v[i] == i + 1);
        }
    }

    SECTION("update")
    {
        const auto u = v.update(10u, [](auto x) { return x + 10; });
        CHECK(u.size() == v.size());
        CHECK(u[10u] == 20u);
        CHECK(v[40u] == 40u);

        const auto w = v.update(40u, [](auto x) { return x - 10; });
        CHECK(w.size() == v.size());
        CHECK(w[40u] == 30u);
        CHECK(v[40u] == 40u);
    }
}

TEST_CASE("iterator")
{
    const auto n = 666u;
    auto v       = make_test_vector(0, n);

    SECTION("empty vector")
    {
        auto v = VECTOR_T<unsigned>{};
        CHECK(v.begin() == v.end());
    }

    SECTION("works with range loop")
    {
        auto i = 0u;
        for (const auto& x : v)
            CHECK(x == i++);
        CHECK(i == v.size());
    }

    SECTION("works with standard algorithms")
    {
        auto s = std::vector<unsigned>(n);
        std::iota(s.begin(), s.end(), 0u);
        std::equal(v.begin(), v.end(), s.begin(), s.end());
    }

    SECTION("can go back from end")
    {
        auto expected = n - 1;
        auto last     = v.end();
        CHECK(expected == *--last);
    }

    SECTION("works with reversed range adaptor")
    {
        auto r = v | boost::adaptors::reversed;
        auto i = n;
        for (const auto& x : r)
            CHECK(x == --i);
    }

    SECTION("works with strided range adaptor")
    {
        auto r = v | boost::adaptors::strided(5);
        auto i = 0u;
        for (const auto& x : r)
            CHECK(x == 5 * i++);
    }

    SECTION("works reversed")
    {
        auto i = n;
        for (auto iter = v.rbegin(), last = v.rend(); iter != last; ++iter)
            CHECK(*iter == --i);
    }

    SECTION("advance and distance")
    {
        auto i1 = v.begin();
        auto i2 = i1 + 100;
        CHECK(100u == *i2);
        CHECK(100 == i2 - i1);
        CHECK(50u == *(i2 - 50));
        CHECK(-30 == (i2 - 30) - i2);
    }
}

TEST_CASE("equals")
{
    const auto n = 666u;
    auto v       = make_test_vector(0, n);

    CHECK(v == v);
    CHECK(v == v.set(42, 42));
    CHECK(v != v.set(42, 24));
    CHECK(v == v.set(42, 24).set(42, 42));
    CHECK(v.set(42, 24) == v.set(42, 24));
    CHECK(v != v.push_back(7));
    CHECK(v.push_back(7) == v.push_back(7));
    CHECK(v.push_back(5) != v.push_back(7));
    CHECK(v != v.set(v.size() - 2, 24));
    CHECK(v == v.set(v.size() - 2, 24).set(v.size() - 2, v[v.size() - 2]));
}

TEST_CASE("all of")
{
    const auto n = 666u;
    auto v       = make_test_vector(0, n);

    SECTION("false")
    {
        auto res = immer::all_of(v, [](auto x) { return x < 100; });
        CHECK(res == false);
    }
    SECTION("true")
    {
        auto res = immer::all_of(v, [](auto x) { return x < 1000; });
        CHECK(res == true);
    }
    SECTION("bounded, true")
    {
        auto res = immer::all_of(
            v.begin() + 101, v.end() - 10, [](auto x) { return x > 100; });
        CHECK(res == true);
    }
    SECTION("bounded, false")
    {
        auto res = immer::all_of(
            v.begin() + 101, v.end() - 10, [](auto x) { return x < 600; });
        CHECK(res == false);
    }
}

TEST_CASE("accumulate")
{
    const auto n = 666u;
    auto v       = make_test_vector(0, n);

    auto expected_n = [](auto n) { return n * (n - 1) / 2; };
    auto expected_i = [&](auto i, auto n) {
        return expected_n(n) - expected_n(i);
    };

    SECTION("sum collection")
    {
        auto sum = immer::accumulate(v, 0u);
        CHECK(sum == expected_n(v.size()));
    }

    SECTION("sum range")
    {
        using namespace std;
        {
            auto sum = immer::accumulate(begin(v) + 100, begin(v) + 300, 0u);
            CHECK(sum == expected_i(100u, 300u));
        }
        {
            auto sum = immer::accumulate(begin(v) + 31, begin(v) + 300, 0u);
            CHECK(sum == expected_i(31u, 300u));
        }
        {
            auto sum = immer::accumulate(begin(v), begin(v) + 33, 0u);
            CHECK(sum == expected_i(0u, 33u));
        }
        {
            auto sum = immer::accumulate(begin(v) + 100, begin(v) + 660, 0u);
            CHECK(sum == expected_i(100u, 660u));
        }
        {
            auto sum = immer::accumulate(begin(v) + 100, begin(v) + 105, 0u);
            CHECK(sum == expected_i(100u, 105u));
        }
        {
            auto sum = immer::accumulate(begin(v) + 660, begin(v) + 664, 0u);
            CHECK(sum == expected_i(660u, 664u));
        }
    }
}

TEST_CASE("vector of strings")
{
    const auto n = 666u;

    auto v = VECTOR_T<std::string>{};
    for (auto i = 0u; i < n; ++i)
        v = v.push_back(std::to_string(i));

    for (decltype(v.size()) i = 0; i < v.size(); ++i)
        CHECK(v[i] == std::to_string(i));

    SECTION("set")
    {
        for (auto i = 0u; i < n; ++i)
            v = v.set(i, "foo " + std::to_string(i));
        for (auto i = 0u; i < n; ++i)
            CHECK(v[i] == "foo " + std::to_string(i));
    }
}

struct non_default
{
    unsigned value;
    non_default(unsigned value_)
        : value{value_}
    {}
    non_default() = delete;
    operator unsigned() const { return value; }

#if IMMER_DEBUG_PRINT
    friend std::ostream& operator<<(std::ostream& os, non_default x)
    {
        os << "ND{" << x.value << "}";
        return os;
    }
#endif
};

TEST_CASE("non default")
{
    const auto n = 666u;

    auto v = VECTOR_T<non_default>{};
    for (auto i = 0u; i < n; ++i)
        v = v.push_back({i});

    CHECK_VECTOR_EQUALS(v, boost::irange(0u, n));

    SECTION("set")
    {
        for (auto i = 0u; i < n; ++i)
            v = v.set(i, {i + 1});

        CHECK_VECTOR_EQUALS(v, boost::irange(1u, n + 1u));
    }
}

TEST_CASE("take")
{
    const auto n = 666u;

    SECTION("anywhere")
    {
        auto v = make_test_vector(0, n);

        for (auto i : test_irange(0u, n)) {
            auto vv = v.take(i);
            CHECK(vv.size() == i);
            CHECK_VECTOR_EQUALS_RANGE(vv, v.begin(), v.begin() + i);
        }
    }
}

TEST_CASE("exception safety")
{
    constexpr auto n = 666u;

    using dadaist_vector_t = typename dadaist_wrapper<VECTOR_T<unsigned>>::type;

    SECTION("push back")
    {
        auto v = dadaist_vector_t{};
        auto d = dadaism{};
        for (auto i = 0u; v.size() < static_cast<decltype(v.size())>(n);) {
            auto s = d.next();
            try {
                v = v.push_back({i});
                ++i;
            } catch (dada_error) {}
            CHECK_VECTOR_EQUALS(v, boost::irange(0u, i));
        }
        CHECK(d.happenings > 0);
        IMMER_TRACE_E(d.happenings);
    }

    SECTION("update")
    {
        auto v = make_test_vector<dadaist_vector_t>(0, n);
        auto d = dadaism{};
        for (auto i = 0u; i < n;) {
            auto s = d.next();
            try {
                v = v.update(i, [](auto x) { return dada(), x + 1; });
                ++i;
            } catch (dada_error) {}
            CHECK_VECTOR_EQUALS(
                v, boost::join(boost::irange(1u, 1u + i), boost::irange(i, n)));
        }
        CHECK(d.happenings > 0);
        IMMER_TRACE_E(d.happenings);
    }

    SECTION("take")
    {
        auto v = make_test_vector<dadaist_vector_t>(0, n);
        auto d = dadaism{};
        for (auto i = 0u; i < n;) {
            auto s = d.next();
            auto r = dadaist_vector_t{};
            try {
                r = v.take(i);
                CHECK_VECTOR_EQUALS(r, boost::irange(0u, i++));
            } catch (dada_error) {
                CHECK_VECTOR_EQUALS(r, boost::irange(0u, 0u));
            }
        }
        CHECK(d.happenings > 0);
        IMMER_TRACE_E(d.happenings);
    }
}
