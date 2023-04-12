// Copyright (c) 2023 The Navcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#define BOOST_UNIT_TEST
#define BLS_ETH 1

#include <blsct/arith/mcl/mcl.h>
#include <blsct/arith/elements.h>
#include <blsct/set_mem_proof/set_mem_proof.h>
#include <streams.h>
#include <boost/test/unit_test.hpp>
#include <test/util/setup_common.h>

using Scalar = Mcl::Scalar;
using Point = Mcl::Point;
using Points = Elements<Mcl::Point>;

BOOST_FIXTURE_TEST_SUITE(set_mem_proof_tests, BasicTestingSetup)

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
    Ls.Add(g * 10);
    Ls.Add(g * 11);
    Points Rs;
    Rs.Add(g * 20);
    Rs.Add(g * 21);
    Scalar a(7);
    Scalar b(8);
    Scalar omega(9);
    Scalar c_factor(10);

    auto p = SetMemProof(
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
        omega,
        c_factor
    );

    CDataStream st(SER_DISK, PROTOCOL_VERSION);
    p.Serialize(st);

    SetMemProof q;
    q.Unserialize(st);

    BOOST_CHECK(p.phi == q.phi);
    BOOST_CHECK(p.A1 == q.A1);
    BOOST_CHECK(p.A2 == q.A2);
    BOOST_CHECK(p.S1 == q.S1);
    BOOST_CHECK(p.S2 == q.S2);
    BOOST_CHECK(p.S3 == q.S3);
    BOOST_CHECK(p.T1 == q.T1);
    BOOST_CHECK(p.T2 == q.T2);
    BOOST_CHECK(p.tau_x == q.tau_x);
    BOOST_CHECK(p.mu == q.mu);
    BOOST_CHECK(p.z_alpha == q.z_alpha);
    BOOST_CHECK(p.z_tau == q.z_tau);
    BOOST_CHECK(p.z_beta == q.z_beta);
    BOOST_CHECK(p.t == q.t);
    BOOST_CHECK(p.Ls == q.Ls);
    BOOST_CHECK(p.Rs == q.Rs);
    BOOST_CHECK(p.a == q.a);
    BOOST_CHECK(p.b == q.b);
    BOOST_CHECK(p.omega == q.omega);
    BOOST_CHECK(p.c_factor == q.c_factor);
}

BOOST_AUTO_TEST_SUITE_END()
