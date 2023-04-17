// Copyright (c) 2022 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <blsct/arith/mcl/mcl.h>
#include <blsct/range_proof/range_proof_with_transcript.h>

#include <test/util/setup_common.h>
#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(range_proof_with_transcript_tests, BasicTestingSetup)

using T = Mcl;

BOOST_AUTO_TEST_CASE(test_range_proof_with_transcript_recover_num_rounds)
{
    BOOST_CHECK((RangeProofWithTranscript<T>::RecoverNumRounds(1ul)) == 6ul);
    BOOST_CHECK((RangeProofWithTranscript<T>::RecoverNumRounds(2ul)) == 7ul);
    BOOST_CHECK((RangeProofWithTranscript<T>::RecoverNumRounds(3ul)) == 8ul);
    BOOST_CHECK((RangeProofWithTranscript<T>::RecoverNumRounds(4ul)) == 8ul);
    BOOST_CHECK((RangeProofWithTranscript<T>::RecoverNumRounds(5ul)) == 9ul);
    BOOST_CHECK((RangeProofWithTranscript<T>::RecoverNumRounds(6ul)) == 9ul);
    BOOST_CHECK((RangeProofWithTranscript<T>::RecoverNumRounds(7ul)) == 9ul);
    BOOST_CHECK((RangeProofWithTranscript<T>::RecoverNumRounds(8ul)) == 9ul);
    BOOST_CHECK((RangeProofWithTranscript<T>::RecoverNumRounds(9ul)) == 10ul);
    BOOST_CHECK((RangeProofWithTranscript<T>::RecoverNumRounds(10ul)) == 10ul);
    BOOST_CHECK((RangeProofWithTranscript<T>::RecoverNumRounds(11ul)) == 10ul);
    BOOST_CHECK((RangeProofWithTranscript<T>::RecoverNumRounds(12ul)) == 10ul);
    BOOST_CHECK((RangeProofWithTranscript<T>::RecoverNumRounds(13ul)) == 10ul);
    BOOST_CHECK((RangeProofWithTranscript<T>::RecoverNumRounds(14ul)) == 10ul);
    BOOST_CHECK((RangeProofWithTranscript<T>::RecoverNumRounds(15ul)) == 10ul);
    BOOST_CHECK((RangeProofWithTranscript<T>::RecoverNumRounds(16ul)) == 10ul);
}

BOOST_AUTO_TEST_SUITE_END()
