// Copyright (c) 2022 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <blsct/arith/mcl/mcl.h>
#include <blsct/building_block/imp_inner_prod_arg.h>
#include <blsct/range_proof/common.h>
#include <blsct/range_proof/bulletproofs/range_proof_with_transcript.h>

#include <test/util/setup_common.h>
#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(bulletproofs_range_proof_with_transcript_tests, BasicTestingSetup)

using T = Mcl;

BOOST_AUTO_TEST_CASE(test_range_proof_with_transcript_recover_num_rounds)
{
    BOOST_CHECK((range_proof::Common<T>::GetNumRoundsExclLast(3ul)) == 8ul);
    BOOST_CHECK((range_proof::Common<T>::GetNumRoundsExclLast(2ul)) == 7ul);
    BOOST_CHECK((range_proof::Common<T>::GetNumRoundsExclLast(4ul)) == 8ul);
    BOOST_CHECK((range_proof::Common<T>::GetNumRoundsExclLast(5ul)) == 9ul);
    BOOST_CHECK((range_proof::Common<T>::GetNumRoundsExclLast(1ul)) == 6ul);
    BOOST_CHECK((range_proof::Common<T>::GetNumRoundsExclLast(6ul)) == 9ul);
    BOOST_CHECK((range_proof::Common<T>::GetNumRoundsExclLast(7ul)) == 9ul);
    BOOST_CHECK((range_proof::Common<T>::GetNumRoundsExclLast(8ul)) == 9ul);
    BOOST_CHECK((range_proof::Common<T>::GetNumRoundsExclLast(9ul)) == 10ul);
    BOOST_CHECK((range_proof::Common<T>::GetNumRoundsExclLast(10ul)) == 10ul);
    BOOST_CHECK((range_proof::Common<T>::GetNumRoundsExclLast(11ul)) == 10ul);
    BOOST_CHECK((range_proof::Common<T>::GetNumRoundsExclLast(12ul)) == 10ul);
    BOOST_CHECK((range_proof::Common<T>::GetNumRoundsExclLast(13ul)) == 10ul);
    BOOST_CHECK((range_proof::Common<T>::GetNumRoundsExclLast(14ul)) == 10ul);
    BOOST_CHECK((range_proof::Common<T>::GetNumRoundsExclLast(15ul)) == 10ul);
    BOOST_CHECK((range_proof::Common<T>::GetNumRoundsExclLast(16ul)) == 10ul);
}

BOOST_AUTO_TEST_SUITE_END()
