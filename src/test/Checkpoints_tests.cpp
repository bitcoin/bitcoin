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
    uint256 p107996 = uint256("0x00000000000a23840ac16115407488267aa3da2b9bc843e301185b7d17e4dc40");
    BOOST_CHECK(Checkpoints::CheckBlock(88805, p88805));
    BOOST_CHECK(Checkpoints::CheckBlock(107996, p107996));

    
    // Wrong hashes at checkpoints should fail:
    BOOST_CHECK(!Checkpoints::CheckBlock(88805, p107996));
    BOOST_CHECK(!Checkpoints::CheckBlock(107996, p88805));

    // ... but any hash not at a checkpoint should succeed:
    BOOST_CHECK(Checkpoints::CheckBlock(88805+1, p107996));
    BOOST_CHECK(Checkpoints::CheckBlock(107996+1, p88805));

    BOOST_CHECK(Checkpoints::GetTotalBlocksEstimate() >= 107996);
}    

BOOST_AUTO_TEST_SUITE_END()
