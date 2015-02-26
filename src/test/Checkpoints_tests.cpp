// Copyright (c) 2011-2013 The Bitcoin Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

//
// Unit tests for block-chain checkpoints
//

#include "checkpoints.h"

#include "uint256.h"

#include <boost/test/unit_test.hpp>

using namespace std;

BOOST_AUTO_TEST_SUITE(Checkpoints_tests)

BOOST_AUTO_TEST_CASE(sanity)
{
    uint256 p88805 = uint256("0x00000000001392f1652e9bf45cd8bc79dc60fe935277cd11538565b4a94fa85f");
    uint256 p217752 = uint256("0x00000000000a7baeb2148272a7e14edf5af99a64af456c0afc23d15a0918b704");
    BOOST_CHECK(Checkpoints::CheckBlock(88805, p88805));
    BOOST_CHECK(Checkpoints::CheckBlock(217752, p217752));

    
    // Wrong hashes at checkpoints should fail:
    BOOST_CHECK(!Checkpoints::CheckBlock(88805, p217752));
    BOOST_CHECK(!Checkpoints::CheckBlock(217752, p88805));

    // ... but any hash not at a checkpoint should succeed:
    BOOST_CHECK(Checkpoints::CheckBlock(88805+1, p217752));
    BOOST_CHECK(Checkpoints::CheckBlock(217752+1, p88805));

    BOOST_CHECK(Checkpoints::GetTotalBlocksEstimate() >= 217752);
}    

BOOST_AUTO_TEST_SUITE_END()
