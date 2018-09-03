// Copyright (c) 2014-2018 The Crown developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/test/unit_test.hpp>
#include "base58.h"

BOOST_AUTO_TEST_SUITE(Prefix_tests)

    BOOST_AUTO_TEST_CASE(OldAddressCheck)
    {
        SelectParams(CBaseChainParams::MAIN);
        std::string oldAddress = "1HAsA9SSDXRBt33ofkmzXL9N52we815XK4";
        CKeyID key;
        // Check that the address is invalid for new prefixes
        BOOST_CHECK(!CBitcoinAddress(oldAddress).GetKeyID(key));
        // Check that the address is valid for old prefixes
        BOOST_CHECK(CBitcoinAddressOld(oldAddress).GetKeyID(key));
    }

    BOOST_AUTO_TEST_CASE(NewAddressCheck)
    {
        SelectParams(CBaseChainParams::MAIN);
        std::string newAddress = "CRWU8rUhhtL86yQEQRtLMmzjRaNjZJs3NwWj";
        CKeyID key;
        // Check that the address is invalid for new prefixes
        BOOST_CHECK(CBitcoinAddress(newAddress).GetKeyID(key));
        // Check that the address is valid for old prefixes
        BOOST_CHECK(!CBitcoinAddressOld(newAddress).GetKeyID(key));
    }

    BOOST_AUTO_TEST_CASE(KeyIdCheck)
    {
        SelectParams(CBaseChainParams::MAIN);
        std::string oldAddress = "1HAsA9SSDXRBt33ofkmzXL9N52we815XK4";

        CKeyID key;
        BOOST_CHECK(CBitcoinAddressOld(oldAddress).GetKeyID(key));
        std::string newAddress = CBitcoinAddress(key).ToString();

        CKeyID newKey;
        CBitcoinAddress(newAddress).GetKeyID(newKey);
        // Check if the KeyId is the same for both old and new addresses
        BOOST_CHECK(key == newKey);
    }

BOOST_AUTO_TEST_SUITE_END()
