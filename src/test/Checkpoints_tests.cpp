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
    uint256 p11111 = uint256("0x0000000069e244f73d78e8fd29ba2fd2ed618bd6fa2ee92559f542fdb26e7c1d");
    uint256 p140700 = uint256("0x000000000000033b512028abb90e1626d8b346fd0ed598ac0a3c371138dce2bd");
    BOOST_CHECK(Checkpoints::CheckBlock(11111, p11111));
    BOOST_CHECK(Checkpoints::CheckBlock(140700, p140700));

    
    // Wrong hashes at checkpoints should fail:
    BOOST_CHECK(!Checkpoints::CheckBlock(11111, p140700));
    BOOST_CHECK(!Checkpoints::CheckBlock(140700, p11111));

    // ... but any hash not at a checkpoint should succeed:
    BOOST_CHECK(Checkpoints::CheckBlock(11111+1, p140700));
    BOOST_CHECK(Checkpoints::CheckBlock(140700+1, p11111));

    BOOST_CHECK(Checkpoints::GetTotalBlocksEstimate() >= 140700);
}    

BOOST_AUTO_TEST_SUITE_END()
