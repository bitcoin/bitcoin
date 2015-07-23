#include "omnicore/rules.h"

#include <boost/test/unit_test.hpp>

using namespace mastercore;

BOOST_AUTO_TEST_SUITE(omnicore_params_tests)

BOOST_AUTO_TEST_CASE(get_params)
{
    const CConsensusParams& params = ConsensusParams();
    BOOST_CHECK(params.exodusReward == 100);
}


BOOST_AUTO_TEST_SUITE_END()
