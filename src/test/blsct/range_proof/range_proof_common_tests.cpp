// Copyright (c) 2022 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <blsct/arith/mcl/mcl.h>
#include <blsct/range_proof/common.h>
#include <blsct/range_proof/bulletproofs/range_proof.h>
#include <test/util/setup_common.h>

#include <boost/test/unit_test.hpp>


BOOST_FIXTURE_TEST_SUITE(range_proof_common_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(test_get_num_rounds_excl_last)
{
    auto num_rounds = range_proof::Common<Mcl>::GetNumRoundsExclLast(64);
    BOOST_CHECK(num_rounds == 12);
}

BOOST_AUTO_TEST_CASE(test_range_proof_validate_proofs_by_sizes)
{
    using T = Mcl;

    auto gen_valid_proof_wo_value_commitments = [](size_t num_inputs) {
        bulletproofs::RangeProof<T> p;
        auto n = blsct::Common::GetFirstPowerOf2GreaterOrEqTo(num_inputs);
        for (size_t i=0; i<n; ++i) {
            p.Vs.Add(MclG1Point::GetBasePoint());
        }
        auto num_rounds = range_proof::Common<T>::GetNumRoundsExclLast(n);
        for (size_t i=0; i<num_rounds; ++i) {
            p.Ls.Add(MclG1Point::GetBasePoint());
            p.Rs.Add(MclG1Point::GetBasePoint());
        }
        return p;
    };

    bulletproofs::RangeProof<T> rp;
    {
        // no proof should validate fine
        std::vector<bulletproofs::RangeProof<T>> proofs;
        BOOST_CHECK_NO_THROW(range_proof::Common<T>::ValidateProofsBySizes(proofs));
    }
    {
        // no value commitment
        bulletproofs::RangeProof<T> p;
        std::vector<bulletproofs::RangeProof<T>> proofs { p };
        BOOST_CHECK_THROW(range_proof::Common<T>::ValidateProofsBySizes(proofs), std::runtime_error);
    }
    {
        // minimum number of value commitments
        auto p = gen_valid_proof_wo_value_commitments(1);
        std::vector<bulletproofs::RangeProof<T>> proofs { p };
        BOOST_CHECK_NO_THROW(range_proof::Common<T>::ValidateProofsBySizes(proofs));
    }
    {
        // maximum number of value commitments
        auto p = gen_valid_proof_wo_value_commitments(range_proof::Setup::max_input_values);
        std::vector<bulletproofs::RangeProof<T>> proofs { p };
        BOOST_CHECK_NO_THROW(range_proof::Common<T>::ValidateProofsBySizes(proofs));
    }
    {
        // number of value commitments exceeding maximum
        auto p = gen_valid_proof_wo_value_commitments(range_proof::Setup::max_input_values + 1);
        std::vector<bulletproofs::RangeProof<T>> proofs { p };
        BOOST_CHECK_THROW(range_proof::Common<T>::ValidateProofsBySizes(proofs), std::runtime_error);
    }
}

BOOST_AUTO_TEST_SUITE_END()
