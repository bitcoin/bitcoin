// Copyright (c) 2023 The Navcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#define BOOST_UNIT_TEST

#include <boost/test/unit_test.hpp>
#include <test/util/setup_common.h>
#include <blsct/arith/mcl/mcl.h>
#include <blsct/set_mem_proof/set_mem_proof_setup.h>

BOOST_FIXTURE_TEST_SUITE(set_mem_proof_setup_tests, BasicTestingSetup)

using Scalar = Mcl::Scalar;
using Point = Mcl::Point;

BOOST_AUTO_TEST_CASE(test_size_of_hs)
{
    auto setup = SetMemProofSetup<Mcl>::Get();
    BOOST_CHECK(setup.N == setup.hs.Size());
}

BOOST_AUTO_TEST_CASE(test_g)
{
    auto setup = SetMemProofSetup<Mcl>::Get();
    BOOST_CHECK(setup.g == Point::GetBasePoint());
}

BOOST_AUTO_TEST_CASE(test_all_generators_differ)
{
    auto setup = SetMemProofSetup<Mcl>::Get();
    BOOST_CHECK(setup.g != setup.h);

    Point prev_p = setup.h;
    for (size_t i=0; i<setup.hs.Size(); i++) {
        BOOST_CHECK(setup.hs[i] != prev_p);
        prev_p = setup.hs[i];
    }
}

BOOST_AUTO_TEST_CASE(test_h1_to_h7)
{
    auto setup = SetMemProofSetup<Mcl>::Get();

    std::vector<uint8_t> msg {1,2,3};

    // the same message should be hashed to different scalar values
    {
        Scalar h1 = setup.H1(msg);
        Scalar h2 = setup.H2(msg);
        Scalar h3 = setup.H3(msg);
        Scalar h4 = setup.H4(msg);

        std::vector<Scalar> vec {h1, h2, h3, h4};
        for (size_t i=0; i<vec.size()-1; ++i) {
            for (size_t j=i+1; j<vec.size(); ++j) {
                BOOST_CHECK(vec[i] != vec[j]);
            }
        }
    }

    // the same message should be hashed to different points
    {
        Point p5 = setup.H5(msg);
        Point p6 = setup.H6(msg);
        Point p7 = setup.H7(msg);

        std::vector<Point> vec {p5, p6, p7};
        for (size_t i=0; i<vec.size()-1; ++i) {
            for (size_t j=i+1; j<vec.size(); ++j) {
                BOOST_CHECK(vec[i] != vec[j]);
            }
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()
