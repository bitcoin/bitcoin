// Copyright (c) 2023 The Navcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#define BOOST_UNIT_TEST

#include <boost/test/unit_test.hpp>
#include <test/util/setup_common.h>
#include <cstdio>
#include <sstream>
#include <blsct/arith/mcl/mcl.h>
#include <blsct/arith/elements.h>
#include <blsct/building_block/pedersen_commitment.h>
#include <blsct/set_mem_proof/set_mem_proof_prover.h>
#include <blsct/set_mem_proof/set_mem_proof_setup.h>
#include <blsct/set_mem_proof/set_mem_proof.h>

BOOST_FIXTURE_TEST_SUITE(set_mem_proof_prover_tests, BasicTestingSetup)

using Arith = Mcl;
using Point = Arith::Point;
using Scalar = Arith::Scalar;
using Points = Elements<Point>;
using Prover = SetMemProofProver<Arith>;

BOOST_AUTO_TEST_CASE(test_extend_ys)
{
    auto setup = SetMemProofSetup<Arith>::Get();
    {
        Points ys;
        auto ys2 = Prover::ExtendYs(setup, ys, 1);
        BOOST_CHECK_EQUAL(ys2.Size(), 1);
    }
    {
        Points ys;
        auto ys2 = Prover::ExtendYs(setup, ys, 2);
        BOOST_CHECK_EQUAL(ys2.Size(), 2);
    }
    {
        Points ys;
        ys.Add(Point::GetBasePoint());
        auto ys2 = Prover::ExtendYs(setup, ys, 1);
        BOOST_CHECK_EQUAL(ys2.Size(), 1);

        BOOST_CHECK(ys2[0] == ys[0]);
    }
    {
        Points ys;
        ys.Add(Point::GetBasePoint());
        auto ys2 = Prover::ExtendYs(setup, ys, 2);
        BOOST_CHECK_EQUAL(ys2.Size(), 2);

        BOOST_CHECK(ys2[0] == ys[0]);
        BOOST_CHECK(ys2[0] != ys2[1]);
    }
    {
        Points ys;
        ys.Add(Point::GetBasePoint());
        size_t new_size = 64;
        auto ys2 = Prover::ExtendYs(setup, ys, new_size);
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
        Points ys;
        ys.Add(Point::GetBasePoint());
        BOOST_CHECK_THROW(Prover::ExtendYs(setup, ys, 0), std::runtime_error);
    }
}

BOOST_AUTO_TEST_CASE(test_prove_verify_small_size_good_inputs_of_power_of_2)
{
    auto y1 = Point::MapToPoint("y1", Endianness::Little);
    auto y2 = Point::MapToPoint("y2", Endianness::Little);
    auto y4 = Point::MapToPoint("y4", Endianness::Little);

    auto setup = SetMemProofSetup<Arith>::Get();

    Scalar m = Scalar::Rand();
    Scalar f = Scalar::Rand();
    auto sigma = setup.pedersen.Commit(m, f);

    Points Ys;
    Ys.Add(y1);
    Ys.Add(y2);
    Ys.Add(sigma);
    Ys.Add(y4);

    Scalar eta_fiat_shamir = Scalar::Rand();
    blsct::Message eta_phi { 1, 2, 3 };
    auto proof = Prover::Prove(
        setup, Ys, sigma, m, f, eta_fiat_shamir, eta_phi
    );
    auto res = Prover::Verify(
        setup, Ys, eta_fiat_shamir, eta_phi, proof
    );

    BOOST_CHECK_EQUAL(res, true);
}

BOOST_AUTO_TEST_CASE(test_prove_verify_small_size_good_inputs_of_non_power_of_2)
{
    auto y1 = Point::MapToPoint("y1", Endianness::Little);
    auto y2 = Point::MapToPoint("y2", Endianness::Little);

    auto setup = SetMemProofSetup<Arith>::Get();

    Scalar m = Scalar::Rand();
    Scalar f = Scalar::Rand();
    auto sigma = setup.pedersen.Commit(m, f);

    Points Ys;
    Ys.Add(y1);
    Ys.Add(y2);
    Ys.Add(sigma);

    Scalar eta_fiat_shamir = Scalar::Rand();
    blsct::Message eta_phi { 1, 2, 3 };
    auto proof = Prover::Prove(
        setup, Ys, sigma, m, f, eta_fiat_shamir, eta_phi
    );
    auto res = Prover::Verify(
        setup, Ys, eta_fiat_shamir, eta_phi, proof
    );

    BOOST_CHECK_EQUAL(res, true);
}

BOOST_AUTO_TEST_CASE(test_prove_verify_small_size_sigma_not_included)
{
    auto y1 = Point::MapToPoint("y1", Endianness::Little);
    auto y2 = Point::MapToPoint("y2", Endianness::Little);
    auto y4 = Point::MapToPoint("y4", Endianness::Little);

    auto setup = SetMemProofSetup<Arith>::Get();

    Scalar m = Scalar::Rand();
    Scalar f = Scalar::Rand();
    auto sigma = setup.pedersen.Commit(m, f);

    Points prove_Ys;
    prove_Ys.Add(y1);
    prove_Ys.Add(y2);
    prove_Ys.Add(sigma);
    prove_Ys.Add(y4);

    auto y3 = Point::MapToPoint("y3", Endianness::Little);
    Points verify_Ys;
    verify_Ys.Add(y1);
    verify_Ys.Add(y2);
    verify_Ys.Add(y3);
    verify_Ys.Add(y4);

    Scalar eta_fiat_shamir = Scalar::Rand();
    blsct::Message eta_phi { 1, 2, 3 };
    auto proof = Prover::Prove(
        setup, prove_Ys, sigma, m, f, eta_fiat_shamir, eta_phi
    );
    auto res = Prover::Verify(
        setup, verify_Ys, eta_fiat_shamir, eta_phi, proof
    );

    BOOST_CHECK_EQUAL(res, false);
}

BOOST_AUTO_TEST_CASE(test_prove_verify_small_size_sigma_generated_from_other_inputs)
{
    auto setup = SetMemProofSetup<Arith>::Get();

    // Commitment set includes A=g*f_a+h*m_a, B=g*f_b+h*m_b, and C=g*f_c+h*m_c
    Scalar m_a = Scalar::Rand();
    Scalar m_b = Scalar::Rand();
    Scalar m_c = Scalar::Rand();
    Scalar m_e = Scalar::Rand();
    Scalar f_a = Scalar::Rand();
    Scalar f_b = Scalar::Rand();
    Scalar f_c = Scalar::Rand();
    Scalar f_e = Scalar::Rand();

    auto A = setup.pedersen.Commit(m_a, f_a);
    auto B = setup.pedersen.Commit(m_b, f_b);
    auto C = setup.pedersen.Commit(m_c, f_c);
    auto E = setup.pedersen.Commit(m_e, f_e);

    Points ys;
    ys.Add(A);
    ys.Add(B);
    ys.Add(C);
    ys.Add(E);

    Scalar eta_fiat_shamir = Scalar::Rand();
    blsct::Message eta_phi { 1, 2, 3 };

    // A proof over the membership of D=A+B=g*(f_a+f_b)+h*(m_a+m_b) should be deemed as invalid
    auto m_d = m_a + m_b;
    auto f_d = f_a + f_b;
    auto D = setup.pedersen.Commit(m_d, f_d);

    auto proof = Prover::Prove(
        setup, ys, D, m_d, f_d, eta_fiat_shamir, eta_phi
    );
    auto res = Prover::Verify(
        setup, ys, eta_fiat_shamir, eta_phi, proof
    );

    BOOST_CHECK_EQUAL(res, false);
}

BOOST_AUTO_TEST_CASE(test_prove_verify_small_size_sigma_in_different_pos)
{
    auto y1 = Point::MapToPoint("y1", Endianness::Little);
    auto y2 = Point::MapToPoint("y2", Endianness::Little);
    auto y3 = Point::MapToPoint("y4", Endianness::Little);

    auto setup = SetMemProofSetup<Arith>::Get();

    Scalar m = Scalar::Rand();
    Scalar f = Scalar::Rand();
    auto sigma = setup.pedersen.Commit(m, f);

    Points prove_Ys;
    prove_Ys.Add(y1);
    prove_Ys.Add(y2);
    prove_Ys.Add(sigma);
    prove_Ys.Add(y3);

    Points verify_Ys;
    verify_Ys.Add(y1);
    verify_Ys.Add(y2);
    verify_Ys.Add(y3);
    verify_Ys.Add(sigma);

    Scalar eta_fiat_shamir = Scalar::Rand();
    blsct::Message eta_phi { 1, 2, 3 };

    auto proof = Prover::Prove(
        setup, prove_Ys, sigma, m, f, eta_fiat_shamir, eta_phi
    );
    auto res = Prover::Verify(
        setup, verify_Ys, eta_fiat_shamir, eta_phi, proof
    );

    BOOST_CHECK_EQUAL(res, false);
}

BOOST_AUTO_TEST_CASE(test_prove_verify_small_size_different_eta)
{
    auto y1 = Point::MapToPoint("y1", Endianness::Little);
    auto y2 = Point::MapToPoint("y2", Endianness::Little);
    auto y4 = Point::MapToPoint("y4", Endianness::Little);

    auto setup = SetMemProofSetup<Arith>::Get();

    Scalar m = Scalar::Rand();
    Scalar f = Scalar::Rand();
    auto sigma = setup.pedersen.Commit(m, f);

    Points ys;
    ys.Add(y1);
    ys.Add(y2);
    ys.Add(sigma);
    ys.Add(y4);

    Scalar eta_fiat_shamir_123(123);
    Scalar eta_fiat_shamir_456(456);
    blsct::Message eta_phi { 1, 2, 3 };

    auto proof = Prover::Prove(
        setup, ys, sigma, m, f, eta_fiat_shamir_123, eta_phi
    );
    auto res = Prover::Verify(
        setup, ys, eta_fiat_shamir_456, eta_phi, proof
    );

    BOOST_CHECK_EQUAL(res, false);
}

BOOST_AUTO_TEST_CASE(test_prove_verify_small_size_same_sigma_different_ys)
{
    auto y1_1 = Point::MapToPoint("y1_1", Endianness::Little);
    auto y2_1 = Point::MapToPoint("y2_1", Endianness::Little);
    auto y4_1 = Point::MapToPoint("y4_1", Endianness::Little);

    auto y1_2 = Point::MapToPoint("y1_2", Endianness::Little);
    auto y2_2 = Point::MapToPoint("y2_2", Endianness::Little);
    auto y4_2 = Point::MapToPoint("y4_2", Endianness::Little);

    auto setup = SetMemProofSetup<Arith>::Get();

    Scalar m = Scalar::Rand();
    Scalar f = Scalar::Rand();
    auto sigma = setup.pedersen.Commit(m, f);

    Points prove_Ys;
    prove_Ys.Add(y1_1);
    prove_Ys.Add(y2_1);
    prove_Ys.Add(sigma);
    prove_Ys.Add(y4_1);

    Points verify_Ys;
    verify_Ys.Add(y1_2);
    verify_Ys.Add(y2_2);
    verify_Ys.Add(sigma);
    verify_Ys.Add(y4_2);

    Scalar eta_fiat_shamir = Scalar::Rand();
    blsct::Message eta_phi { 1, 2, 3 };

    auto proof = Prover::Prove(
        setup, prove_Ys, sigma, m, f, eta_fiat_shamir, eta_phi
    );
    auto res = Prover::Verify(
        setup, verify_Ys, eta_fiat_shamir, eta_phi, proof
    );

    BOOST_CHECK_EQUAL(res, false);
}


BOOST_AUTO_TEST_CASE(test_prove_verify_large_size_input)
{
    auto setup = SetMemProofSetup<Arith>::Get();

    const size_t NUM_INPUTS = setup.N;
    Points Ys;
    Scalar m = Scalar::Rand();
    Scalar f = Scalar::Rand();
    Point sigma = setup.pedersen.Commit(m, f);

    for (size_t i=0; i<NUM_INPUTS; ++i) {
        if (i == NUM_INPUTS / 2) {
            Ys.Add(sigma);
        } else {
            std::ostringstream ss;
            ss << "y" << i;
            auto y = Point::MapToPoint(ss.str(), Endianness::Little);
            Ys.Add(y);
        }
    }

    Scalar eta_fiat_shamir = Scalar::Rand();
    blsct::Message eta_phi { 1, 2, 3 };

    auto proof = Prover::Prove(
        setup, Ys, sigma, m, f, eta_fiat_shamir, eta_phi
    );
    auto res = Prover::Verify(
        setup, Ys, eta_fiat_shamir, eta_phi, proof
    );

    BOOST_CHECK_EQUAL(res, true);
}

BOOST_AUTO_TEST_SUITE_END()
