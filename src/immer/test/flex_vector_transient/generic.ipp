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

#ifndef FLEX_VECTOR_TRANSIENT_T
#error "define the vector template to use in FLEX_VECTOR_TRANSIENT_T"
#endif

#ifndef VECTOR_T
#error "define the vector template to use in VECTOR_T"
#endif

template <typename V = VECTOR_T<unsigned>>
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

TEST_CASE("from flex_vector and to flex_vector")
{
    constexpr auto n = 100u;

    auto v = make_test_flex_vector(0, n).transient();
    CHECK_VECTOR_EQUALS(v, boost::irange(0u, n));

    auto p = v.persistent();
    CHECK_VECTOR_EQUALS(p, boost::irange(0u, n));
}

TEST_CASE("adopt regular vector contents")
{
    const auto n = 666u;
    auto v       = VECTOR_T<unsigned>{};
    for (auto i = 0u; i < n; ++i) {
        v       = v.push_back(i);
        auto fv = FLEX_VECTOR_TRANSIENT_T<unsigned>{v.transient()};
        CHECK_VECTOR_EQUALS_AUX(v, fv, [](auto&& v) { return &v; });
    }
}

TEST_CASE("drop move")
{
    using vector_t = FLEX_VECTOR_T<unsigned>;

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
    v                = check_move(std::move(v).drop(1));
    auto addr_after  = &v[0];

    if (vector_t::bits_leaf > 0 &&
        vector_t::memory_policy::use_transient_rvalues)
        CHECK(addr_before == addr_after);
    else
        CHECK(addr_before != addr_after);

    CHECK_VECTOR_EQUALS(v, boost::irange(1u, 2u));
}

TEST_CASE("exception safety relaxed")
{
    using dadaist_vector_t =
        typename dadaist_wrapper<FLEX_VECTOR_T<unsigned>>::type;
    constexpr auto n = 667u;

    SECTION("push back")
    {
        auto half = n / 2;
        auto t    = as_transient_tester(
            make_test_flex_vector_front<dadaist_vector_t>(0, half));
        auto d = dadaism{};
        for (auto li = half, i = half; i < n;) {
            auto s = d.next();
            try {
                if (t.transient)
                    t.vt.push_back({i});
                else
                    t.vp = t.vp.push_back({i});
                ++i;
            } catch (dada_error) {}
            if (t.step())
                li = i;
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

        auto t = as_transient_tester(
            make_test_flex_vector_front<dadaist_vector_t>(0, n));
        auto d = dadaism{};
        for (auto li = 0u, i = 0u; i < n;) {
            auto s = d.next();
            try {
                if (t.transient) {
                    t.vt.update(i, [](auto x) { return dada(), x + 1; });
                } else {
                    t.vp = t.vp.update(i, [](auto x) { return dada(), x + 1; });
                }
                ++i;
            } catch (dada_error) {}
            if (t.step())
                li = i;
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
        auto t = as_transient_tester(
            make_test_flex_vector_front<dadaist_vector_t>(0, n));
        auto d      = dadaism{};
        auto deltas = magic_rotator();
        auto delta  = deltas.next();
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

    SECTION("drop")
    {
        auto t =
            as_transient_tester(make_test_flex_vector<dadaist_vector_t>(0, n));
        auto d      = dadaism{};
        auto deltas = magic_rotator();
        auto delta  = deltas.next();
        for (auto i = delta, li = 0u; i < n;) {
            auto s = d.next();
            auto r = dadaist_vector_t{};
            try {
                if (t.transient)
                    t.vt.drop(delta);
                else
                    t.vp = t.vp.drop(delta);
                if (t.step()) {
                    li = i;
                }
                delta = deltas.next();
                i += delta;
            } catch (dada_error) {}
            if (t.transient) {
                CHECK_VECTOR_EQUALS(t.vt, boost::irange(i - delta, n));
                CHECK_VECTOR_EQUALS(t.vp, boost::irange(li, n));
            } else {
                CHECK_VECTOR_EQUALS(t.vp, boost::irange(i - delta, n));
                CHECK_VECTOR_EQUALS(t.vt, boost::irange(li, n));
            }
        }
        CHECK(d.happenings > 0);
        CHECK(t.d.happenings > 0);
    }

    SECTION("append")
    {
        auto make_ = [](auto i, auto delta) {
            auto d = dadaism::disable();
            return make_test_flex_vector<dadaist_vector_t>(i, i + delta);
        };
        auto t      = as_transient_tester(dadaist_vector_t{});
        auto d      = dadaism();
        auto deltas = magic_rotator();
        auto delta  = deltas.next();
        for (auto i = 0u, li = 0u; i < n;) {
            try {
                if (t.transient) {
                    auto data  = make_(i, delta);
                    auto datat = data.transient();
                    t.vt.append(datat);
                } else {
                    auto data = make_(i, delta);
                    t.vp      = t.vp + data;
                }
                i += delta;
                if (t.step()) {
                    li = i;
                }
                delta = deltas.next() * 3;
            } catch (dada_error) {}
            if (t.transient) {
                CHECK_VECTOR_EQUALS(t.vt, boost::irange(0u, i));
                CHECK_VECTOR_EQUALS(t.vp, boost::irange(0u, li));
            } else {
                CHECK_VECTOR_EQUALS(t.vp, boost::irange(0u, i));
                CHECK_VECTOR_EQUALS(t.vt, boost::irange(0u, li));
            }
        }
        CHECK(d.happenings == 0);
        CHECK(t.d.happenings > 0);
    }

    SECTION("append mut")
    {
        auto make_ = [](auto i, auto delta) {
            auto d = dadaism::disable();
            return make_test_flex_vector<dadaist_vector_t>(i, i + delta);
        };
        auto t      = as_transient_tester(dadaist_vector_t{});
        auto d      = dadaism();
        auto deltas = magic_rotator();
        auto delta  = deltas.next();
        for (auto i = 0u, li = 0u; i < n;) {
            try {
                if (t.transient) {
                    auto data  = make_(i, delta);
                    auto datat = data.transient();
                    t.vt.append(std::move(datat));
                } else {
                    auto data = make_(i, delta);
                    t.vp      = t.vp + data;
                }
                i += delta;
                if (t.step()) {
                    li = i;
                }
                delta = deltas.next() * 3;
            } catch (dada_error) {}
            if (t.transient) {
                CHECK_VECTOR_EQUALS(t.vt, boost::irange(0u, i));
                CHECK_VECTOR_EQUALS(t.vp, boost::irange(0u, li));
            } else {
                CHECK_VECTOR_EQUALS(t.vp, boost::irange(0u, i));
                CHECK_VECTOR_EQUALS(t.vt, boost::irange(0u, li));
            }
        }
        CHECK(d.happenings == 0);
        CHECK(t.d.happenings > 0);
    }

    SECTION("prepend")
    {
        auto make_ = [](auto i, auto delta) {
            auto d = dadaism::disable();
            return make_test_flex_vector<dadaist_vector_t>(i, i + delta);
        };
        auto t      = as_transient_tester(dadaist_vector_t{});
        auto d      = dadaism();
        auto deltas = magic_rotator();
        auto delta  = deltas.next();
        for (auto i = n, li = n; i > 0;) {
            delta = std::min(i, delta);
            try {
                if (t.transient) {
                    auto data  = make_(i - delta, delta);
                    auto datat = data.transient();
                    t.vt.prepend(datat);
                } else {
                    auto data = make_(i - delta, delta);
                    t.vp      = data + t.vp;
                }
                i -= delta;
                if (t.step()) {
                    li = i;
                }
                delta = deltas.next() * 3;
            } catch (dada_error) {}
            if (t.transient) {
                CHECK_VECTOR_EQUALS(t.vt, boost::irange(i, n));
                CHECK_VECTOR_EQUALS(t.vp, boost::irange(li, n));
            } else {
                CHECK_VECTOR_EQUALS(t.vp, boost::irange(i, n));
                CHECK_VECTOR_EQUALS(t.vt, boost::irange(li, n));
            }
        }
        CHECK(d.happenings == 0);
        CHECK(t.d.happenings > 0);
    }

    SECTION("prepend mut")
    {
        auto make_ = [](auto i, auto delta) {
            auto d = dadaism::disable();
            return make_test_flex_vector<dadaist_vector_t>(i, i + delta);
        };
        auto t      = as_transient_tester(dadaist_vector_t{});
        auto d      = dadaism();
        auto deltas = magic_rotator();
        auto delta  = deltas.next();
        for (auto i = n, li = n; i > 0;) {
            delta = std::min(i, delta);
            try {
                if (t.transient) {
                    auto data  = make_(i - delta, delta);
                    auto datat = data.transient();
                    t.vt.prepend(std::move(datat));
                } else {
                    auto data = make_(i - delta, delta);
                    t.vp      = data + t.vp;
                }
                i -= delta;
                if (t.step()) {
                    li = i;
                }
                delta = deltas.next() * 3;
            } catch (dada_error) {}
            if (t.transient) {
                CHECK_VECTOR_EQUALS(t.vt, boost::irange(i, n));
                CHECK_VECTOR_EQUALS(t.vp, boost::irange(li, n));
            } else {
                CHECK_VECTOR_EQUALS(t.vp, boost::irange(i, n));
                CHECK_VECTOR_EQUALS(t.vt, boost::irange(li, n));
            }
        }
        CHECK(d.happenings == 0);
        CHECK(t.d.happenings > 0);
    }
}
