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

#include <catch.hpp>

#ifndef VECTOR_T
#error "define the vector template to use in VECTOR_T"
#endif

#ifndef VECTOR_TRANSIENT_T
#error "define the vector template to use in VECTOR_TRANSIENT_T"
#endif

template <typename V = VECTOR_T<unsigned>>
auto make_test_vector(unsigned min, unsigned max)
{
    auto v = V{};
    for (auto i = min; i < max; ++i)
        v = v.push_back({i});
    return v;
}

TEST_CASE("from vector and to vector")
{
    constexpr auto n = 100u;

    auto v = make_test_vector(0, n).transient();
    CHECK_VECTOR_EQUALS(v, boost::irange(0u, n));

    auto p = v.persistent();
    CHECK_VECTOR_EQUALS(p, boost::irange(0u, n));
}

TEST_CASE("protect persistence")
{
    auto v = VECTOR_T<unsigned>{}.transient();
    v.push_back(12);
    auto p = v.persistent();
    v.set(0, 42);
    CHECK(p[0] == 12);
    CHECK(v[0] == 42);
}

TEST_CASE("push back move")
{
    using vector_t = VECTOR_T<unsigned>;

    auto v = vector_t{};

    auto check_move = [&](vector_t&& x) -> vector_t&& {
        if (vector_t::memory_policy::use_transient_rvalues)
            CHECK(&x == &v);
        else
            CHECK(&x != &v);
        return std::move(x);
    };

    v                = check_move(std::move(v).push_back(0));
    v                = check_move(std::move(v).push_back(1));
    v                = check_move(std::move(v).push_back(2));
    auto addr_before = &v[0];
    v                = check_move(std::move(v).push_back(3));
    auto addr_after  = &v[0];

    if (vector_t::memory_policy::use_transient_rvalues)
        CHECK(addr_before == addr_after);
    else
        CHECK(addr_before != addr_after);

    CHECK_VECTOR_EQUALS(v, boost::irange(0u, 4u));
}

TEST_CASE("set move")
{
    using vector_t = VECTOR_T<unsigned>;

    auto v = vector_t{};

    auto check_move = [&](vector_t&& x) -> vector_t&& {
        if (vector_t::memory_policy::use_transient_rvalues)
            CHECK(&x == &v);
        else
            CHECK(&x != &v);
        return std::move(x);
    };

    v = v.push_back(0);

    auto addr_before = &v[0];
    v                = check_move(std::move(v).set(0, 1));
    auto addr_after  = &v[0];

    if (vector_t::memory_policy::use_transient_rvalues)
        CHECK(addr_before == addr_after);
    else
        CHECK(addr_before != addr_after);

    CHECK_VECTOR_EQUALS(v, boost::irange(1u, 2u));
}

TEST_CASE("update move")
{
    using vector_t = VECTOR_T<unsigned>;

    auto v = vector_t{};

    auto check_move = [&](vector_t&& x) -> vector_t&& {
        if (vector_t::memory_policy::use_transient_rvalues)
            CHECK(&x == &v);
        else
            CHECK(&x != &v);
        return std::move(x);
    };

    v = v.push_back(0);

    auto addr_before = &v[0];
    v = check_move(std::move(v).update(0, [](auto x) { return x + 1; }));
    auto addr_after = &v[0];

    if (vector_t::memory_policy::use_transient_rvalues)
        CHECK(addr_before == addr_after);
    else
        CHECK(addr_before != addr_after);

    CHECK_VECTOR_EQUALS(v, boost::irange(1u, 2u));
}

TEST_CASE("take move")
{
    using vector_t = VECTOR_T<unsigned>;

    auto v = vector_t{};

    auto check_move = [&](vector_t&& x) -> vector_t&& {
        if (vector_t::memory_policy::use_transient_rvalues)
            CHECK(&x == &v);
        else
            CHECK(&x != &v);
        return std::move(x);
    };

    v = v.push_back(0).push_back(1);

    auto addr_before = &v[0];
    v                = check_move(std::move(v).take(1));
    auto addr_after  = &v[0];

    if (vector_t::memory_policy::use_transient_rvalues)
        CHECK(addr_before == addr_after);
    else
        CHECK(addr_before != addr_after);

    CHECK_VECTOR_EQUALS(v, boost::irange(0u, 1u));
}

TEST_CASE("exception safety")
{
    constexpr auto n = 667u;

    using dadaist_vector_t = typename dadaist_wrapper<VECTOR_T<unsigned>>::type;

    SECTION("push back")
    {
        auto t = as_transient_tester(dadaist_vector_t{});
        auto d = dadaism{};
        for (auto li = 0u, i = 0u; i < n;) {
            auto s = d.next();
            try {
                if (t.transient)
                    t.vt.push_back({i});
                else
                    t.vp = t.vp.push_back({i});
                ++i;
                if (t.step())
                    li = i;
            } catch (dada_error) {}
            if (t.transient) {
                CHECK_VECTOR_EQUALS(t.vt, boost::irange(0u, i));
                CHECK_VECTOR_EQUALS(t.vp, boost::irange(0u, li));
            } else {
                CHECK_VECTOR_EQUALS(t.vp, boost::irange(0u, i));
                CHECK_VECTOR_EQUALS(t.vt, boost::irange(0u, li));
            }
        }
        CHECK(d.happenings > 0);
        CHECK(t.d.happenings > 0);
        IMMER_TRACE_E(d.happenings);
        IMMER_TRACE_E(t.d.happenings);
    }

    SECTION("update")
    {
        using boost::irange;
        using boost::join;

        auto t = as_transient_tester(make_test_vector<dadaist_vector_t>(0, n));
        auto d = dadaism{};
        for (auto li = 0u, i = 0u; i < n;) {
            auto s = d.next();
            try {
                if (t.transient)
                    t.vt.update(i, [](auto x) { return dada(), x + 1; });
                else
                    t.vp = t.vp.update(i, [](auto x) { return dada(), x + 1; });
                ++i;
                if (t.step())
                    li = i;
            } catch (dada_error) {}
            if (t.transient) {
                CHECK_VECTOR_EQUALS(t.vt,
                                    join(irange(1u, 1u + i), irange(i, n)));
                CHECK_VECTOR_EQUALS(t.vp,
                                    join(irange(1u, 1u + li), irange(li, n)));
            } else {
                CHECK_VECTOR_EQUALS(t.vp,
                                    join(irange(1u, 1u + i), irange(i, n)));
                CHECK_VECTOR_EQUALS(t.vt,
                                    join(irange(1u, 1u + li), irange(li, n)));
            }
        }
        CHECK(d.happenings > 0);
        CHECK(t.d.happenings > 0);
    }

    SECTION("take")
    {
        auto t = as_transient_tester(make_test_vector<dadaist_vector_t>(0, n));
        auto d = dadaism{};
        auto deltas = magic_rotator();
        auto delta  = 0u;
        for (auto i = n, li = i;;) {
            auto s = d.next();
            auto r = dadaist_vector_t{};
            try {
                if (t.transient)
                    t.vt.take(i);
                else
                    t.vp = t.vp.take(i);
                if (t.step())
                    li = i;
                delta = deltas.next();
                if (i < delta)
                    break;
                i -= delta;
            } catch (dada_error) {}
            if (t.transient) {
                CHECK_VECTOR_EQUALS(t.vt, boost::irange(0u, i + delta));
                CHECK_VECTOR_EQUALS(t.vp, boost::irange(0u, li));
            } else {
                CHECK_VECTOR_EQUALS(t.vp, boost::irange(0u, i + delta));
                CHECK_VECTOR_EQUALS(t.vt, boost::irange(0u, li));
            }
        }
        CHECK(d.happenings > 0);
        CHECK(t.d.happenings > 0);
    }
}
