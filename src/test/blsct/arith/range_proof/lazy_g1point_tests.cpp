// Copyright (c) 2011-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <blsct/arith/elements.h>
#include <blsct/arith/range_proof/lazy_g1point.h>
#include <test/util/setup_common.h>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(lazy_g1point_tests, MclTestingSetup)

BOOST_AUTO_TEST_CASE(test_lazy_g1points_ctor)
{
    auto g = G1Point::GetBasePoint();
    auto g2 = g + g;

    LazyG1Points points1(
        std::vector<G1Point> { g, g2 },
        std::vector<Scalar> { 1, 2 }
    );

    LazyG1Points points2;
    points2.Add(LazyG1Point(g, 1));
    points2.Add(LazyG1Point(g, 2));

    BOOST_CHECK(points1.Sum() == points1.Sum());
}

BOOST_AUTO_TEST_CASE(test_lazy_g1points_sum)
{
    auto lazy_g = LazyG1Point(G1Point::GetBasePoint(), 1);
    auto lazy_g2 = LazyG1Point(G1Point::GetBasePoint(), 2);
    LazyG1Points lazy_points;
    lazy_points.Add(lazy_g);
    lazy_points.Add(lazy_g2);
    auto lazy_sum = lazy_points.Sum();

    auto g = G1Point::GetBasePoint();
    auto g2 = g + g;
    G1Points points;
    points.Add(g);
    points.Add(g2);
    auto sum = points.Sum();

    BOOST_CHECK(sum == lazy_sum);
}

BOOST_AUTO_TEST_CASE(test_lazy_g1points_add_lazy_g1points_to_lazy_g1points)
{
    auto g = LazyG1Point(G1Point::GetBasePoint(), 1);
    auto g2 = LazyG1Point(G1Point::GetBasePoint(), 2);

    LazyG1Points ps1, ps2, ps3;
    ps1.Add(g);
    ps2.Add(g2);

    ps3.Add(g);
    ps3.Add(g2);

    BOOST_CHECK(ps3.Sum() == (ps1 + ps2).Sum());
}

BOOST_AUTO_TEST_CASE(test_lazy_g1points_add_lazy_g1points_to_lazy_g1point)
{
    auto g = LazyG1Point(G1Point::GetBasePoint(), 1);
    auto g2 = LazyG1Point(G1Point::GetBasePoint(), 2);

    LazyG1Points ps1;
    ps1.Add(g);
    auto ps2 = ps1 + g2;

    LazyG1Points ps3;
    ps3.Add(g);
    ps3.Add(g2);

    BOOST_CHECK(ps3.Sum() == ps2.Sum());
}

BOOST_AUTO_TEST_SUITE_END()
