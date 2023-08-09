// Copyright (c) 2023 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <blsct/arith/elements.h>
#include <blsct/arith/mcl/mcl.h>
#include <blsct/building_block/lazy_points.h>
#include <test/util/setup_common.h>
#include <boost/test/unit_test.hpp>

template <typename T>
struct Tester {
    using Point = typename T::Point;
    using Scalar = typename T::Scalar;

    static void TestCtor() {
        auto g = Point::GetBasePoint();
        auto g2 = g + g;

        LazyPoints<T> points1(
            std::vector<Point> { g, g2 },
            std::vector<Scalar> { 1, 2 }
        );

        LazyPoints<T> points2;
        points2.Add(LazyPoint<T>(g, 1));
        points2.Add(LazyPoint<T>(g, 2));

        BOOST_CHECK((points1.Sum()) == (points1.Sum()));
    }

    static void TestSum() {
        auto lazy_g = LazyPoint<T>(Point::GetBasePoint(), 1);
        auto lazy_g2 = LazyPoint<T>(Point::GetBasePoint(), 2);
        LazyPoints<T> lazy_points;
        lazy_points.Add(lazy_g);
        lazy_points.Add(lazy_g2);
        auto lazy_sum = lazy_points.Sum();

        auto g = Point::GetBasePoint();
        auto g2 = g + g;
        Elements<Point> points;
        points.Add(g);
        points.Add(g2);
        auto sum = points.Sum();

        BOOST_CHECK(sum == lazy_sum);
    }

    static void TestAddLazyPointsToLazyPoints() {
        auto g = LazyPoint<T>(Point::GetBasePoint(), 1);
        auto g2 = LazyPoint<T>(Point::GetBasePoint(), 2);

        LazyPoints<T> ps1, ps2, ps3;
        ps1.Add(g);
        ps2.Add(g2);

        ps3.Add(g);
        ps3.Add(g2);

        BOOST_CHECK((ps3.Sum()) == ((ps1 + ps2).Sum()));
    }

    static void TestAddLazyPointsToLazyPoint() {
        auto g = LazyPoint<T>(Point::GetBasePoint(), 1);
        auto g2 = LazyPoint<T>(Point::GetBasePoint(), 2);

        LazyPoints<T> ps1;
        ps1.Add(g);
        auto ps2 = ps1 + g2;

        LazyPoints<T> ps3;
        ps3.Add(g);
        ps3.Add(g2);

        BOOST_CHECK((ps3.Sum()) == (ps2.Sum()));
    }
};

BOOST_FIXTURE_TEST_SUITE(lazy_points_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(test_lazy_points_ctor)
{
    Tester<Mcl>::TestCtor();
}

BOOST_AUTO_TEST_CASE(test_lazy_points_sum)
{
    Tester<Mcl>::TestSum();
}

BOOST_AUTO_TEST_CASE(test_lazy_points_add_lazy_points_to_lazy_points)
{
    Tester<Mcl>::TestAddLazyPointsToLazyPoints();
}

BOOST_AUTO_TEST_CASE(test_lazy_points_add_lazy_points_to_lazy_point)
{
    Tester<Mcl>::TestAddLazyPointsToLazyPoint();
}

BOOST_AUTO_TEST_SUITE_END()