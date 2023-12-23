// Copyright (c) 2022 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <blsct/range_proof/bulletproofs_plus/range_proof.h>
#include <blsct/arith/mcl/mcl.h>
#include <test/util/setup_common.h>
#include <boost/test/unit_test.hpp>
#include <streams.h>

BOOST_FIXTURE_TEST_SUITE(bulletproofs_plus_range_proof_tests, BasicTestingSetup)

using T = Mcl;
using Point = T::Point;
using Scalar = T::Scalar;
using Points = Elements<Point>;

bulletproofs_plus::RangeProof<T> GenProof() {
    Point g = Point::GetBasePoint();

    Points Vs;
    Vs.Add(g * 100);
    Vs.Add(g * 101);

    Points Ls;
    Ls.Add(g * 200);
    Ls.Add(g * 201);

    Points Rs;
    Rs.Add(g * 300);
    Rs.Add(g * 301);

    Point A = g * 2;
    Point A_wip = g * 3;
    Point B = g * 4;

    Scalar r_prime(2);
    Scalar s_prime(3);
    Scalar delta_prime(4);
    Scalar alpha_hat(5);
    Scalar tau_x(6);

    bulletproofs_plus::RangeProof<T> p;
    {
        p.Vs = Vs;
        p.Ls = Ls;
        p.Rs = Rs;

        p.A = A;
        p.A_wip = A_wip;
        p.B = B;

        p.r_prime = r_prime;
        p.s_prime = s_prime;
        p.delta_prime = delta_prime;
        p.alpha_hat = alpha_hat;
        p.tau_x = tau_x;
    }

    return p;
}

BOOST_AUTO_TEST_CASE(test_equal)
{
    auto p = GenProof();
    auto q = GenProof();
    BOOST_CHECK(p == p);

    q.A = q.A + q.A;

    BOOST_CHECK(p != q);
}

BOOST_AUTO_TEST_CASE(test_de_ser)
{
    auto p = GenProof();

    DataStream st{};
    p.Serialize(st);

    bulletproofs_plus::RangeProof<T> q;
    q.Unserialize(st);

    BOOST_CHECK(p == q);
}

BOOST_AUTO_TEST_SUITE_END()
