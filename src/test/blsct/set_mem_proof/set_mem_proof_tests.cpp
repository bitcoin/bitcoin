// Copyright (c) 2023 The Navcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#define BOOST_UNIT_TEST

#include <blsct/arith/mcl/mcl.h>
#include <blsct/arith/elements.h>
#include <blsct/set_mem_proof/set_mem_proof.h>
#include <streams.h>
#include <boost/test/unit_test.hpp>
#include <test/util/setup_common.h>

using Scalar = Mcl::Scalar;
using Point = Mcl::Point;
using Points = Elements<Point>;

BOOST_FIXTURE_TEST_SUITE(set_mem_proof_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(test_equal)
{
    Point g = Point::GetBasePoint();

    Point phi = g;
    Point A1 = g * 2;
    Point A2 = g * 3;
    Point S1 = g * 4;
    Point S2 = g * 5;
    Point S3 = g * 6;
    Point T1 = g * 7;
    Point T2 = g * 8;
    Scalar tau_x(1);
    Scalar mu(2);
    Scalar z_alpha(3);
    Scalar z_tau(4);
    Scalar z_beta(5);
    Scalar t(6);
    Points Ls;
    Ls.Add(g * 100);
    Ls.Add(g * 101);
    Points Rs;
    Rs.Add(g * 200);
    Rs.Add(g * 201);
    Scalar a(7);
    Scalar b(8);
    Scalar omega(9);

    auto p = SetMemProof<Mcl>(
        phi,
        A1,
        A2,
        S1,
        S2,
        S3,
        T1,
        T2,
        tau_x,
        mu,
        z_alpha,
        z_tau,
        z_beta,
        t,
        Ls,
        Rs,
        a,
        b,
        omega
    );

    auto q = SetMemProof<Mcl>(
        phi,
        g,
        A2,
        S1,
        S2,
        S3,
        T1,
        T2,
        tau_x,
        mu,
        z_alpha,
        z_tau,
        z_beta,
        t,
        Ls,
        Rs,
        a,
        b,
        omega
    );

    BOOST_CHECK(p == p);
    BOOST_CHECK(p != q);
}

BOOST_AUTO_TEST_CASE(test_de_ser)
{
    Point g = Point::GetBasePoint();

    Point phi = g;
    Point A1 = g * 2;
    Point A2 = g * 3;
    Point S1 = g * 4;
    Point S2 = g * 5;
    Point S3 = g * 6;
    Point T1 = g * 7;
    Point T2 = g * 8;
    Scalar tau_x(1);
    Scalar mu(2);
    Scalar z_alpha(3);
    Scalar z_tau(4);
    Scalar z_beta(5);
    Scalar t(6);
    Points Ls;
    Ls.Add(g * 100);
    Ls.Add(g * 101);
    Points Rs;
    Rs.Add(g * 200);
    Rs.Add(g * 201);
    Scalar a(7);
    Scalar b(8);
    Scalar omega(9);

    auto p = SetMemProof<Mcl>(
        phi,
        A1,
        A2,
        S1,
        S2,
        S3,
        T1,
        T2,
        tau_x,
        mu,
        z_alpha,
        z_tau,
        z_beta,
        t,
        Ls,
        Rs,
        a,
        b,
        omega
    );

    DataStream st{};
    p.Serialize(st);

    SetMemProof<Mcl> q;
    q.Unserialize(st);

    BOOST_CHECK(p  == q);
}

BOOST_AUTO_TEST_SUITE_END()
