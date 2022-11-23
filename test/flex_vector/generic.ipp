//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#include "test/dada.hpp"
#include "test/transient_tester.hpp"
#include "test/util.hpp"

#include <immer/algorithm.hpp>

#include <boost/range/adaptors.hpp>
#include <boost/range/irange.hpp>
#include <catch.hpp>

#include <algorithm>
#include <array>
#include <numeric>
#include <vector>

#ifndef FLEX_VECTOR_T
#error "define the vector template to use in FLEX_VECTOR_T"
#endif

#ifndef VECTOR_T
#error "define the vector template to use in VECTOR_T"
#endif

template <typename V = FLEX_VECTOR_T<unsigned>>
auto make_test_flex_vector(unsigned min, unsigned max)
{
    auto v = V{};
    for (auto i = min; i < max; ++i)
        v = v.push_back({i});
    return v;
}

template <typename V = FLEX_VECTOR_T<unsigned>>
auto make_test_flex_vector_front(unsigned min, unsigned max)
{
    auto v = V{};
    for (auto i = max; i > min;)
        v = v.push_front({--i});
    return v;
}

template <std::size_t N>
auto make_many_test_flex_vector()
{
    using vektor_t = FLEX_VECTOR_T<unsigned>;
    auto many      = std::array<vektor_t, N>{};
    std::generate_n(many.begin(), N, [v = vektor_t{}, i = 0u]() mutable {
        auto r = v;
        v      = v.push_back(i++);
        return r;
    });
    return many;
}

template <std::size_t N>
auto make_many_test_flex_vector_front()
{
    using vektor_t = FLEX_VECTOR_T<unsigned>;
    auto many      = std::array<vektor_t, N>{};
    std::generate_n(many.begin(), N, [i = 0u]() mutable {
        return make_test_flex_vector_front(0, i++);
    });
    return many;
}

template <std::size_t N>
auto make_many_test_flex_vector_front_remainder()
{
    using vektor_t = FLEX_VECTOR_T<unsigned>;
    auto many      = std::array<vektor_t, N>{};
    std::generate_n(many.begin(), N, [v = vektor_t{}, i = N - 1]() mutable {
        auto r = v;
        v      = v.push_front(--i);
        return r;
    });
    return many;
}

TEST_CASE("set relaxed")
{
    auto v = make_test_flex_vector_front(0, 666u);
    for (decltype(v.size()) i = 0; i < v.size(); ++i) {
        v = v.set(i, i + 1);
        CHECK(v[i] == i + 1);
    }
}

TEST_CASE("push_front")
{
    const auto n = 666u;
    auto v       = FLEX_VECTOR_T<unsigned>{};

    for (auto i = 0u; i < n; ++i) {
        v = v.push_front(i);
        CHECK(v.size() == i + 1);
        for (decltype(v.size()) j = 0; j < v.size(); ++j)
            CHECK(v[v.size() - j - 1] == j);
    }
}

TEST_CASE("concat")
{
#if IMMER_SLOW_TESTS
    constexpr auto n = 666u;
#else
    constexpr auto n = 101u;
#endif

    auto all_lhs = make_many_test_flex_vector<n>();
    auto all_rhs = make_many_test_flex_vector_front_remainder<n>();

    SECTION("regular plus regular")
    {
        for (auto i : test_irange(0u, n)) {
            auto c = all_lhs[i] + all_lhs[i];
            CHECK_VECTOR_EQUALS(
                c, boost::join(boost::irange(0u, i), boost::irange(0u, i)));
        }
    }

    SECTION("regular plus relaxed")
    {
        for (auto i : test_irange(0u, n)) {
            auto c = all_lhs[i] + all_rhs[n - i - 1];
            CHECK_VECTOR_EQUALS(c, boost::irange(0u, n - 1));
        }
    }
}

auto make_flex_vector_concat(std::size_t min, std::size_t max)
{
    using vektor_t = FLEX_VECTOR_T<unsigned>;

    if (max == min)
        return vektor_t{};
    else if (max == min + 1)
        return vektor_t{}.push_back(min);
    else {
        auto mid = min + (max - min) / 2;
        return make_flex_vector_concat(min, mid) +
               make_flex_vector_concat(mid, max);
    }
}

TEST_CASE("concat recursive")
{
    const auto n = 666u;
    auto v       = make_flex_vector_concat(0, n);
    CHECK_VECTOR_EQUALS(v, boost::irange(0u, n));
}

TEST_CASE("insert")
{
    SECTION("normal")
    {
        const auto n = 666u;
        auto v       = make_test_flex_vector(0, n);
        v            = v.insert(42, 100);
        CHECK_VECTOR_EQUALS(v,
                            boost::join(boost::irange(0u, 42u),
                                        boost::join(boost::irange(100u, 101u),
                                                    boost::irange(42u, n))));
    }

    SECTION("move")
    {
        const auto n = 666u;
        auto v       = make_test_flex_vector(0, n);
        v            = std::move(v).insert(42, 100);
        CHECK_VECTOR_EQUALS(v,
                            boost::join(boost::irange(0u, 42u),
                                        boost::join(boost::irange(100u, 101u),
                                                    boost::irange(42u, n))));
    }

    SECTION("vec")
    {
        const auto n = 666u;
        auto v       = make_test_flex_vector(0, n);
        v            = std::move(v).insert(42, {100, 101, 102});
        CHECK_VECTOR_EQUALS(v,
                            boost::join(boost::irange(0u, 42u),
                                        boost::join(boost::irange(100u, 103u),
                                                    boost::irange(42u, n))));
    }

    SECTION("vec move")
    {
        const auto n = 666u;
        auto v       = make_test_flex_vector(0, n);
        v            = std::move(v).insert(42, {100, 101, 102});
        CHECK_VECTOR_EQUALS(v,
                            boost::join(boost::irange(0u, 42u),
                                        boost::join(boost::irange(100u, 103u),
                                                    boost::irange(42u, n))));
    }
}

TEST_CASE("erase")
{
    const auto n = 666u;
    auto v       = make_test_flex_vector(0, n);
    auto vv      = v.erase(0);
    CHECK_VECTOR_EQUALS(vv, boost::irange(1u, n));
    CHECK_VECTOR_EQUALS(v.erase(v.size() - 1), boost::irange(0u, n - 1));
    CHECK_VECTOR_EQUALS(v.erase(v.size() - 1), boost::irange(0u, n - 1));
    CHECK_VECTOR_EQUALS(
        v.erase(42),
        boost::join(boost::irange(0u, 42u), boost::irange(43u, n)));
    CHECK_VECTOR_EQUALS(v.erase(v.size() - 1, v.size()),
                        boost::irange(0u, n - 1));
    CHECK_VECTOR_EQUALS(v.erase(0, 0), boost::irange(0u, n));
    CHECK_VECTOR_EQUALS(
        v.erase(42, 50),
        boost::join(boost::irange(0u, 42u), boost::irange(50u, n)));
}

TEST_CASE("accumulate relaxed")
{
    auto expected_n = [](auto n) { return n * (n - 1) / 2; };
    auto expected_i = [&](auto i, auto n) {
        return expected_n(n) - expected_n(i);
    };

    SECTION("sum")
    {
        const auto n = 666u;
        auto v       = make_test_flex_vector_front(0, n);

        auto sum      = immer::accumulate(v, 0u);
        auto expected = v.size() * (v.size() - 1) / 2;
        CHECK(sum == expected);
    }

    SECTION("sum complex")
    {
        const auto n = 20u;

        auto v = FLEX_VECTOR_T<unsigned>{};
        for (auto i = 0u; i < n; ++i)
            v = v.push_front(i) + v;

        auto sum      = immer::accumulate(v, 0u);
        auto expected = (1 << n) - n - 1;
        CHECK(sum == expected);
    }

    SECTION("sum range")
    {
        using namespace std;
        const auto n = 666u;
        auto v       = make_test_flex_vector_front(0, n);
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

TEST_CASE("equals")
{
    const auto n = 666u;
    auto v       = make_test_flex_vector_front(0, n);

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
    CHECK(v == v.insert(42, 12).erase(42));
    CHECK(v == v.insert(0, 12).erase(0));
}

TEST_CASE("equals bugs")
{
    {
        const auto n = 666u;
        auto v       = make_test_flex_vector(0, n);
        CHECK(v == v.insert(42, 12).erase(42));
        CHECK(v == v.insert(0, 12).erase(0));
    }
    {
        const auto n = 30u;
        auto v       = make_test_flex_vector(0, n);
        CHECK(v == v.insert(10, 12).erase(10));
        CHECK(v == v.insert(0, 12).erase(0));
    }
    {
        const auto n = 666u;
        auto v       = make_test_flex_vector(0, n);
        for (auto i : test_irange(0u, n))
            CHECK(v == v.insert(i, 42).erase(i));
    }
    {
        const auto n = 666u;
        auto v       = make_test_flex_vector_front(0, n);
        for (auto i : test_irange(0u, n))
            CHECK(v == v.insert(i, 42).erase(i));
    }
    {
        const auto n = 666u;
        auto v       = FLEX_VECTOR_T<unsigned>{};
        using size_t = decltype(v.size());
        for (auto i : test_irange(0u, n)) {
            while (v.size() < i)
                v = std::move(v).push_back(i);
            auto vv = v;
            for (auto j : test_irange(size_t{}, v.size())) {
                auto vz = vv.insert(j, 42).erase(j);
                CHECK(v == vz);
                CHECK(vv == vz);
                vv = vz;
            }
        }
    }
}

TEST_CASE("take relaxed")
{
    const auto n = 666u;
    auto v       = make_test_flex_vector_front(0, n);

    for (auto i : test_irange(0u, n)) {
        auto vv = v.take(i);
        CHECK_VECTOR_EQUALS_RANGE(vv, v.begin(), v.begin() + i);
    }
}

TEST_CASE("drop")
{
    const auto n = 666u;

    SECTION("regular")
    {
        auto v = make_test_flex_vector(0, n);

        for (auto i : test_irange(0u, n)) {
            auto vv = v.drop(i);
            CHECK_VECTOR_EQUALS_RANGE(vv, v.begin() + i, v.end());
        }
    }

    SECTION("relaxed")
    {
        auto v = make_test_flex_vector_front(0, n);

        for (auto i : test_irange(0u, n)) {
            auto vv = v.drop(i);
            CHECK_VECTOR_EQUALS_RANGE(vv, v.begin() + i, v.end());
        }
    }
}

#if IMMER_SLOW_TESTS
TEST_CASE("reconcat")
{
    constexpr auto n = 666u;
    auto v           = make_test_flex_vector_front(0, n);
    auto all_lhs     = make_many_test_flex_vector_front<n + 1>();
    auto all_rhs     = make_many_test_flex_vector_front_remainder<n + 1>();

    for (auto i = 0u; i < n; ++i) {
        auto vv = all_lhs[i] + all_rhs[n - i];
        CHECK_VECTOR_EQUALS(vv, v);
        CHECK_SLOW(vv == v);
    }
}

TEST_CASE("reconcat drop")
{
    constexpr auto n = 666u;
    auto v           = make_test_flex_vector_front(0, n);
    auto all_lhs     = make_many_test_flex_vector_front<n + 1>();

    for (auto i = 0u; i < n; ++i) {
        auto vv = all_lhs[i] + v.drop(i);
        CHECK_VECTOR_EQUALS(vv, v);
        CHECK_SLOW(vv == v);
    }
}

TEST_CASE("reconcat take")
{
    constexpr auto n = 666u;
    auto v           = make_test_flex_vector_front(0, n);
    auto all_rhs     = make_many_test_flex_vector_front_remainder<n + 1>();

    for (auto i = 0u; i < n; ++i) {
        auto vv = v.take(i) + all_rhs[n - i];
        CHECK_VECTOR_EQUALS(vv, v);
        CHECK_SLOW(vv == v);
    }
}
#endif

TEST_CASE("reconcat take drop")
{
    const auto n = 666u;
    auto v       = make_test_flex_vector_front(0, n);

    for (auto i : test_irange(0u, n)) {
        auto vv = v.take(i) + v.drop(i);
        CHECK_VECTOR_EQUALS(vv, v);
        CHECK_SLOW(vv == v);
    }
}

TEST_CASE("reconcat take drop feedback")
{
    const auto n = 666u;
    auto v       = make_test_flex_vector_front(0, n);
    auto vv      = v;
    for (auto i : test_irange(0u, n)) {
        vv = vv.take(i) + vv.drop(i);
        CHECK_VECTOR_EQUALS(vv, v);
        CHECK_SLOW(vv == v);
    }
}

TEST_CASE("iterator relaxed")
{
    const auto n = 666u;
    auto v       = make_test_flex_vector_front(0, n);

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
        CHECK(expected == *--v.end());
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

TEST_CASE("adopt regular vector contents")
{
    const auto n = 666u;
    auto v       = VECTOR_T<unsigned>{};
    for (auto i = 0u; i < n; ++i) {
        v       = v.push_back(i);
        auto fv = FLEX_VECTOR_T<unsigned>{v};
        CHECK_VECTOR_EQUALS_AUX(v, fv, [](auto&& v) { return &v; });
    }
}

TEST_CASE("exception safety relaxed")
{
    using dadaist_vector_t =
        typename dadaist_wrapper<FLEX_VECTOR_T<unsigned>>::type;
    constexpr auto n = 666u;

    SECTION("push back")
    {
        auto half = n / 2;
        auto v    = make_test_flex_vector_front<dadaist_vector_t>(0, half);
        auto d    = dadaism{};
        for (auto i = half; v.size() < static_cast<decltype(v.size())>(n);) {
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
        auto v = make_test_flex_vector_front<dadaist_vector_t>(0, n);
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
        auto v = make_test_flex_vector_front<dadaist_vector_t>(0, n);
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

    SECTION("concat")
    {
        auto v = make_test_flex_vector<dadaist_vector_t>(0, n);
        auto d = dadaism{};
        for (auto i = 0u; i < n;) {
            auto lhs = v.take(i);
            auto rhs = v.drop(i);
            auto s   = d.next();
            try {
                v = lhs + rhs;
                ++i;
            } catch (dada_error) {}
            CHECK_VECTOR_EQUALS(v, boost::irange(0u, n));
        }
        CHECK(d.happenings > 0);
        IMMER_TRACE_E(d.happenings);
    }
}
