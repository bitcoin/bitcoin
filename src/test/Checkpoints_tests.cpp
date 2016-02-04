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
    uint256 p15000 = uint256("0x00000000024472d3430e135eccc90852469b0803a05eb54dd9d7d249b8903e11");
    uint256 p130000 = uint256("0xa51baaa8f0b5f35217479b4defd61f3ae18b5792033a723dd9bd802a78b48803");
    BOOST_CHECK(Checkpoints::CheckHardened(15000, p15000));
    BOOST_CHECK(Checkpoints::CheckHardened(130000, p130000));

    
    // Wrong hashes at checkpoints should fail:
    BOOST_CHECK(!Checkpoints::CheckHardened(15000, p130000));
    BOOST_CHECK(!Checkpoints::CheckHardened(130000, p15000));

    // ... but any hash not at a checkpoint should succeed:
    BOOST_CHECK(Checkpoints::CheckHardened(15000+1, p130000));
    BOOST_CHECK(Checkpoints::CheckHardened(130000+1, p15000));

    BOOST_CHECK(Checkpoints::GetTotalBlocksEstimate() >= 130000);
}    

BOOST_AUTO_TEST_SUITE_END()
