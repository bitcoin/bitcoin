//
// Unit tests for block-chain checkpoints
//
#include <boost/assign/list_of.hpp> // for 'map_list_of()'
#include <boost/test/unit_test.hpp>
#include <boost/foreach.hpp>

#include "../checkpoints.h"
#include "../util.h"

using namespace std;

BOOST_AUTO_TEST_SUITE(Checkpoints_tests)

BOOST_AUTO_TEST_CASE(sanity)
{
    uint256 p19080 = uint256("0x000000000000bca54d9ac17881f94193fd6a270c1bb21c3bf0b37f588a40dbd7");
    uint256 p99999 = uint256("0x27fd5e1de16a4270eb8c68dee2754a64da6312c7c3a0e99a7e6776246be1ee3f");
    BOOST_CHECK(Checkpoints::CheckHardened(19080, p19080));
    BOOST_CHECK(Checkpoints::CheckHardened(99999, p99999));

    
    // Wrong hashes at checkpoints should fail:
    BOOST_CHECK(!Checkpoints::CheckHardened(19080, p99999));
    BOOST_CHECK(!Checkpoints::CheckHardened(99999, p19080));

    // ... but any hash not at a checkpoint should succeed:
    BOOST_CHECK(Checkpoints::CheckHardened(19080+1, p99999));
    BOOST_CHECK(Checkpoints::CheckHardened(99999+1, p19080));

    BOOST_CHECK(Checkpoints::GetTotalBlocksEstimate() >= 99999);
}

BOOST_AUTO_TEST_SUITE_END()
