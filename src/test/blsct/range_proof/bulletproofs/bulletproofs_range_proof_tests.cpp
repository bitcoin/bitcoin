// Copyright (c) 2022 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <blsct/range_proof/bulletproofs/range_proof.h>
#include <blsct/arith/mcl/mcl.h>
#include <blsct/common.h>
#include <test/util/setup_common.h>
#include <boost/test/unit_test.hpp>
#include <streams.h>
#include <ctokens/tokenid.h>
#include <uint256.h>

BOOST_FIXTURE_TEST_SUITE(bulletproofs_range_proof_tests, BasicTestingSetup)

using T = Mcl;
using Point = T::Point;
using Scalar = T::Scalar;
using Points = Elements<Point>;

BOOST_AUTO_TEST_CASE(test_different_seed_type)
{
    Point g = Point::GetBasePoint();

    Points Vs;
    Vs.Add(g * 100);
    Vs.Add(g * 101);
    Point A = g * 2;
    Point S = g * 3;
    Point T1 = g * 4;
    Point T2 = g * 5;
    Scalar mu(2);
    Scalar tau_x(3);
    Points Ls;
    Ls.Add(g * 200);
    Ls.Add(g * 201);
    Points Rs;
    Rs.Add(g * 300);
    Rs.Add(g * 301);
    Scalar a(4);
    Scalar b(5);
    Scalar t_hat(6);
    TokenId token_id(uint256(1));
    blsct::Message msg { 2 };

    bulletproofs::RangeProof<T> p;
    p.seed = token_id;
    p.Vs = Vs;
    p.A = A;
    p.S = S;
    p.T1 = T1;
    p.T2 = T2;
    p.mu = mu;
    p.tau_x = tau_x;
    p.Ls = Ls;
    p.Rs = Rs;
    p.a = a;
    p.b = b;
    p.t_hat = t_hat;

    bulletproofs::RangeProof<T> q;
    p.seed = msg;
    q.Vs = Vs;
    q.A = A;
    q.S = S;
    q.T1 = T1;
    q.T2 = T2;
    q.mu = mu;
    q.tau_x = tau_x;
    q.Ls = Ls;
    q.Rs = Rs;
    q.a = a;
    q.b = b;
    q.t_hat = t_hat;

    BOOST_CHECK(p == p);
    BOOST_CHECK(p != q);
}

BOOST_AUTO_TEST_CASE(test_message_equal)
{
    Point g = Point::GetBasePoint();

    Points Vs;
    Vs.Add(g * 100);
    Vs.Add(g * 101);
    Point A = g * 2;
    Point S = g * 3;
    Point T1 = g * 4;
    Point T2 = g * 5;
    Scalar mu(2);
    Scalar tau_x(3);
    Points Ls;
    Ls.Add(g * 200);
    Ls.Add(g * 201);
    Points Rs;
    Rs.Add(g * 300);
    Rs.Add(g * 301);
    Scalar a(4);
    Scalar b(5);
    Scalar t_hat(6);
    blsct::Message msg_1 { 1 };
    blsct::Message msg_2 { 2 };

    bulletproofs::RangeProof<T> p;
    p.seed = msg_1;
    p.Vs = Vs;
    p.A = A;
    p.S = S;
    p.T1 = T1;
    p.T2 = T2;
    p.mu = mu;
    p.tau_x = tau_x;
    p.Ls = Ls;
    p.Rs = Rs;
    p.a = a;
    p.b = b;
    p.t_hat = t_hat;

    bulletproofs::RangeProof<T> q;
    p.seed = msg_2;
    q.Vs = Vs;
    q.A = A;
    q.S = S;
    q.T1 = T1;
    q.T2 = T2;
    q.mu = mu;
    q.tau_x = tau_x;
    q.Ls = Ls;
    q.Rs = Rs;
    q.a = a;
    q.b = b;
    q.t_hat = t_hat;

    BOOST_CHECK(p == p);
    BOOST_CHECK(p != q);
}

BOOST_AUTO_TEST_CASE(test_token_id_equal)
{
    // different A,S
    {
        Point g = Point::GetBasePoint();

        Points Vs;
        Vs.Add(g * 100);
        Vs.Add(g * 101);
        Point A = g * 2;
        Point S = g * 3;
        Point T1 = g * 4;
        Point T2 = g * 5;
        Scalar mu(2);
        Scalar tau_x(3);
        Points Ls;
        Ls.Add(g * 200);
        Ls.Add(g * 201);
        Points Rs;
        Rs.Add(g * 300);
        Rs.Add(g * 301);
        Scalar a(4);
        Scalar b(5);
        Scalar t_hat(6);
        TokenId token_id(uint256(6));

        bulletproofs::RangeProof<T> p;
        p.seed = token_id;
        p.Vs = Vs;
        p.A = A;
        p.S = S;
        p.T1 = T1;
        p.T2 = T2;
        p.mu = mu;
        p.tau_x = tau_x;
        p.Ls = Ls;
        p.Rs = Rs;
        p.a = a;
        p.b = b;
        p.t_hat = t_hat;

        bulletproofs::RangeProof<T> q;
        p.seed = token_id;
        q.Vs = Vs;
        q.A = g;
        q.S = S;
        q.T1 = T1;
        q.T2 = T2;
        q.mu = mu;
        q.tau_x = tau_x;
        q.Ls = Ls;
        q.Rs = Rs;
        q.a = a;
        q.b = b;
        q.t_hat = t_hat;

        BOOST_CHECK(p == p);
        BOOST_CHECK(p != q);
    }

    // different token_id
    {
        Point g = Point::GetBasePoint();

        Points Vs;
        Vs.Add(g * 100);
        Vs.Add(g * 101);
        Point A = g * 2;
        Point S = g * 3;
        Point T1 = g * 4;
        Point T2 = g * 5;
        Scalar mu(2);
        Scalar tau_x(3);
        Points Ls;
        Ls.Add(g * 200);
        Ls.Add(g * 201);
        Points Rs;
        Rs.Add(g * 300);
        Rs.Add(g * 301);
        Scalar a(4);
        Scalar b(5);
        Scalar t_hat(6);
        TokenId token_id_1(uint256(1));
        TokenId token_id_2(uint256(2));

        bulletproofs::RangeProof<T> p;
        p.seed = token_id_1;
        p.Vs = Vs;
        p.A = A;
        p.S = S;
        p.T1 = T1;
        p.T2 = T2;
        p.mu = mu;
        p.tau_x = tau_x;
        p.Ls = Ls;
        p.Rs = Rs;
        p.a = a;
        p.b = b;
        p.t_hat = t_hat;

        bulletproofs::RangeProof<T> q;
        p.seed = token_id_2;
        q.Vs = Vs;
        q.A = A;
        q.S = S;
        q.T1 = T1;
        q.T2 = T2;
        q.mu = mu;
        q.tau_x = tau_x;
        q.Ls = Ls;
        q.Rs = Rs;
        q.a = a;
        q.b = b;
        q.t_hat = t_hat;

        BOOST_CHECK(p == p);
        BOOST_CHECK(p != q);
    }
}

BOOST_AUTO_TEST_CASE(test_de_ser_token_id)
{
    Point g = Point::GetBasePoint();

    Points Vs;
    Vs.Add(g * 100);
    Vs.Add(g * 101);
    Point A = g * 2;
    Point S = g * 3;
    Point T1 = g * 4;
    Point T2 = g * 5;
    Scalar mu(2);
    Scalar tau_x(3);
    Points Ls;
    Ls.Add(g * 200);
    Ls.Add(g * 201);
    Points Rs;
    Rs.Add(g * 300);
    Rs.Add(g * 301);
    Scalar a(4);
    Scalar b(5);
    Scalar t_hat(6);
    TokenId token_id(uint256(123));

    bulletproofs::RangeProof<T> p;
    p.seed = token_id;
    p.Vs = Vs;
    p.A = A;
    p.S = S;
    p.T1 = T1;
    p.T2 = T2;
    p.mu = mu;
    p.tau_x = tau_x;
    p.Ls = Ls;
    p.Rs = Rs;
    p.a = a;
    p.b = b;
    p.t_hat = t_hat;

    DataStream st{};
    p.Serialize(st);

    bulletproofs::RangeProof<T> q;
    q.Unserialize(st);

    BOOST_CHECK(p == q);
}

BOOST_AUTO_TEST_SUITE_END()
