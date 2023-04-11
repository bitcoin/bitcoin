// Copyright (c) 2023 The Navcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#define BOOST_UNIT_TEST
#define BLS_ETH 1

#include <boost/test/unit_test.hpp>
#include <test/util/setup_common.h>
#include <cstdio>
#include <sstream>
#include <blsct/arith/mcl/mcl.h>
#include <blsct/arith/elements.h>
#include <blsct/set_mem_proof/set_mem_proof_prover.h>
#include <blsct/set_mem_proof/set_mem_proof_setup.h>
#include <blsct/set_mem_proof/set_mem_proof.h>

BOOST_FIXTURE_TEST_SUITE(set_mem_proof_prover_tests, BasicTestingSetup)

using Point = Mcl::Point;
using Scalar = Mcl::Scalar;
using Points = Elements<Point>;

BOOST_AUTO_TEST_CASE(test_extend_ys)
{
    {
        SetMemProofProver prover;
        Points ys;
        auto ys2 = prover.ExtendYs(ys, 1);
        BOOST_CHECK_EQUAL(ys2.Size(), 1);
    }
    {
        SetMemProofProver prover;
        Points ys;
        auto ys2 = prover.ExtendYs(ys, 2);
        BOOST_CHECK_EQUAL(ys2.Size(), 2);
    }
    {
        SetMemProofProver prover;
        Points ys;
        ys.Add(Point::GetBasePoint());
        auto ys2 = prover.ExtendYs(ys, 1);
        BOOST_CHECK_EQUAL(ys2.Size(), 1);

        BOOST_CHECK(ys2[0] == ys[0]);
    }
    {
        SetMemProofProver prover;
        Points ys;
        ys.Add(Point::GetBasePoint());
        auto ys2 = prover.ExtendYs(ys, 2);
        BOOST_CHECK_EQUAL(ys2.Size(), 2);

        BOOST_CHECK(ys2[0] == ys[0]);
        BOOST_CHECK(ys2[0] != ys2[1]);
    }
    {
        SetMemProofProver prover;
        Points ys;
        ys.Add(Point::GetBasePoint());
        size_t new_size = 64;
        auto ys2 = prover.ExtendYs(ys, new_size);
        BOOST_CHECK_EQUAL(ys2.Size(), new_size);

        BOOST_CHECK(ys2[0] == ys[0]);

        for (size_t i=0; i<ys2.Size()-1; ++i) {
            for (size_t j=i+1; j<ys2.Size(); ++j) {
                if (i == j) continue;
                BOOST_CHECK(ys2[i] != ys2[j]);
            }
        }
    }
    {
        SetMemProofProver prover;
        Points ys;
        ys.Add(Point::GetBasePoint());
        BOOST_CHECK_THROW(prover.ExtendYs(ys, 0), std::runtime_error);
    }
}

BOOST_AUTO_TEST_CASE(test_prove_verify_small_size_good_input)
{
    auto y1 = Point::MapToG1("y1", Endianness::Little);
    auto y2 = Point::MapToG1("y2", Endianness::Little);
    auto y4 = Point::MapToG1("y4", Endianness::Little);

    SetMemProofSetup setup;
    Scalar m = Scalar::Rand();
    Scalar f = Scalar::Rand();
    auto sigma = setup.PedersenCommitment(m, f);

    Points Ys;
    Ys.Add(y1);
    Ys.Add(y2);
    Ys.Add(sigma);
    Ys.Add(y4);

    Scalar eta = Scalar::Rand();
    SetMemProofProver prover;
    auto proof = prover.Prove(Ys, sigma, f, m, eta);
    auto res = prover.Verify(Ys, eta, proof);

    BOOST_CHECK_EQUAL(res, true);
}

BOOST_AUTO_TEST_CASE(test_prove_verify_small_size_bad_input)
{
    auto y1 = Point::MapToG1("y1", Endianness::Little);
    auto y2 = Point::MapToG1("y2", Endianness::Little);
    auto y4 = Point::MapToG1("y4", Endianness::Little);

    SetMemProofSetup setup;
    Scalar m = Scalar::Rand();
    Scalar f = Scalar::Rand();
    auto sigma = setup.PedersenCommitment(m, f);

    Points prove_Ys;
    prove_Ys.Add(y1);
    prove_Ys.Add(y2);
    prove_Ys.Add(sigma);
    prove_Ys.Add(y4);

    auto y3 = Point::MapToG1("y3", Endianness::Little);
    Points verify_Ys;
    verify_Ys.Add(y1);
    verify_Ys.Add(y2);
    verify_Ys.Add(y3);
    verify_Ys.Add(y4);

    Scalar eta = Scalar::Rand();
    SetMemProofProver prover;
    auto proof = prover.Prove(prove_Ys, sigma, f, m, eta);
    auto res = prover.Verify(verify_Ys, eta, proof);

    BOOST_CHECK_EQUAL(res, false);
}

BOOST_AUTO_TEST_CASE(test_prove_verify_large_size_input)
{
    SetMemProofSetup setup;
    const size_t NUM_INPUTS = setup.N;
    Points Ys;
    Scalar m = Scalar::Rand();
    Scalar f = Scalar::Rand();
    Point sigma = setup.PedersenCommitment(m, f);

    for (size_t i=0; i<NUM_INPUTS; ++i) {
        if (i == NUM_INPUTS / 2) {
            Ys.Add(sigma);
        } else {
            std::ostringstream ss;
            ss << "y" << i;
            auto y = Point::MapToG1(ss.str(), Endianness::Little);
            Ys.Add(y);
        }
    }

    Scalar eta = Scalar::Rand();
    SetMemProofProver prover;
    auto proof = prover.Prove(Ys, sigma, f, m, eta);
    auto res = prover.Verify(Ys, eta, proof);

    BOOST_CHECK_EQUAL(res, true);
}

BOOST_AUTO_TEST_SUITE_END()
