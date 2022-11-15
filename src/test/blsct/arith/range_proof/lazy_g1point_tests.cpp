// Copyright (c) 2011-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <blsct/arith/elements.h>
#include <blsct/arith/range_proof/lazy_g1point.h>
#include <test/util/setup_common.h>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(lazy_g1point_tests, MclTestingSetup)

BOOST_AUTO_TEST_CASE(test_lazy_g1point_mulvec_mcl)
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

BOOST_AUTO_TEST_SUITE_END()
