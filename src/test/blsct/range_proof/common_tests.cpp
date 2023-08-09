// Copyright (c) 2022 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <blsct/arith/mcl/mcl.h>
#include <blsct/range_proof/common.h>
#include <test/util/setup_common.h>

#include <boost/test/unit_test.hpp>


BOOST_FIXTURE_TEST_SUITE(range_proof_common_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(test_get_num_rounds_excl_last)
{
    auto num_rounds = range_proof::Common<Mcl>::GetNumRoundsExclLast(64);
    BOOST_CHECK(num_rounds == 6);
}

BOOST_AUTO_TEST_SUITE_END()