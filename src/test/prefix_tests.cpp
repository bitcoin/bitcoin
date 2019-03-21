// Copyright (c) 2014-2018 The Crown developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/test/unit_test.hpp>
#include "base58.h"

namespace
{
    struct PrefixFixture
    {
        PrefixFixture()
            : oldAddress("1HAsA9SSDXRBt33ofkmzXL9N52we815XK4")
            , newAddress("CRWU8rUhhtL86yQEQRtLMmzjRaNjZJs3NwWj")
            , oldAddressMultisig("CJz5RtLWq6WWvS8289r3yq4ZWS3qBXLR7D")
            , newAddressMultisig("CRMP3uX1hZSg2VYxWK7kmuCxjSLunLNyphFW")
            , testnetOldAddress("mrhiugdyVuAFMiNSc3pxJZ2dPCd7R3Lzy7")
            , testnetNewAddress("tCRWZTKkdwMnhsvSYjVZayVSFBnHpcrKijiRo")
            , testnetOldAddressMultisig("2NG7oeDyqZYxN2JjYkjsYpTQefsWRDQRhpC")
            , testnetNewAddressMultisig("tCRMd28eueqQYz41Bq44CPV7sfkNCHttHVHce")
        {
            SelectParams(CBaseChainParams::MAIN);
        }

        const std::string oldAddress;
        const std::string newAddress;
        const std::string oldAddressMultisig;
        const std::string newAddressMultisig;
        const std::string testnetOldAddress;
        const std::string testnetNewAddress;
        const std::string testnetOldAddressMultisig;
        const std::string testnetNewAddressMultisig;
    };

}

BOOST_FIXTURE_TEST_SUITE(Prefix_tests, PrefixFixture)

    BOOST_AUTO_TEST_CASE(OldAddressCheck)
    {
        CKeyID key;
        // Check that the address is invalid for new prefixes
        BOOST_CHECK(!CBitcoinAddress(oldAddress).GetKeyID(key));
        // Check that the address is valid for old prefixes
        BOOST_CHECK(CBitcoinAddress(oldAddress, CChainParams::DEPRECATED_ADDRESS_TYPE).GetKeyID(key));
    }

    BOOST_AUTO_TEST_CASE(NewAddressCheck)
    {
        CKeyID key;
        // Check that the address is valid for new prefixes
        BOOST_CHECK(CBitcoinAddress(newAddress).GetKeyID(key));
        // Check that the address is invalid for old prefixes
        BOOST_CHECK(!CBitcoinAddress(newAddress, CChainParams::DEPRECATED_ADDRESS_TYPE).GetKeyID(key));
    }

    BOOST_AUTO_TEST_CASE(KeyIdCheck)
    {
        CKeyID key;
        BOOST_CHECK(CBitcoinAddress(oldAddress, CChainParams::DEPRECATED_ADDRESS_TYPE).GetKeyID(key));
        std::string newAddress = CBitcoinAddress(key).ToString();

        CKeyID newKey;
        CBitcoinAddress(newAddress).GetKeyID(newKey);
        // Check if the KeyId is the same for both old and new addresses
        BOOST_CHECK(key == newKey);
    }

    BOOST_AUTO_TEST_CASE(OldAddressCheckMultisig)
    {
        // Check that the old address is deprecated
        BOOST_CHECK(CBitcoinAddress::IsDeprecated(oldAddressMultisig));
        // Get new address from the old one
        std::string newAddress = CBitcoinAddress::ConvertToNew(oldAddressMultisig);
        // Check that the new address is not deprecated
        BOOST_CHECK(!CBitcoinAddress::IsDeprecated(newAddress));
    }

    BOOST_AUTO_TEST_CASE(NewAddressCheckMultisig)
    {
        // Check that the new address is not deprecated
        BOOST_CHECK(!CBitcoinAddress::IsDeprecated(newAddressMultisig));
        // Get old address from the new one 
        std::string oldAddress = CBitcoinAddress::ConvertToOld(newAddressMultisig);
        // Check that the old address deprecated
        BOOST_CHECK(CBitcoinAddress::IsDeprecated(oldAddress));
    }

    // TESTNET check
    BOOST_AUTO_TEST_CASE(TestnetOldAddressCheck)
    {
        SelectParams(CBaseChainParams::TESTNET);
        CKeyID key;
        // Check that the address is invalid for new prefixes
        BOOST_CHECK(!CBitcoinAddress(testnetOldAddress).GetKeyID(key));
        // Check that the address is valid for old prefixes
        BOOST_CHECK(CBitcoinAddress(testnetOldAddress, CChainParams::DEPRECATED_ADDRESS_TYPE).GetKeyID(key));
    }

    BOOST_AUTO_TEST_CASE(TestnetNewAddressCheck)
    {
        SelectParams(CBaseChainParams::TESTNET);
        CKeyID key;
        // Check that the address is valid for new prefixes
        BOOST_CHECK(CBitcoinAddress(testnetNewAddress).GetKeyID(key));
        // Check that the address is invalid for old prefixes
        BOOST_CHECK(!CBitcoinAddress(testnetNewAddress, CChainParams::DEPRECATED_ADDRESS_TYPE).GetKeyID(key));
    }

    BOOST_AUTO_TEST_CASE(TestnetKeyIdCheck)
    {
        SelectParams(CBaseChainParams::TESTNET);
        CKeyID key;
        BOOST_CHECK(CBitcoinAddress(testnetOldAddress, CChainParams::DEPRECATED_ADDRESS_TYPE).GetKeyID(key));
        std::string newAddress = CBitcoinAddress(key).ToString();

        CKeyID newKey;
        CBitcoinAddress(newAddress).GetKeyID(newKey);
        // Check if the KeyId is the same for both old and new addresses
        BOOST_CHECK(key == newKey);
    }

    BOOST_AUTO_TEST_CASE(TestnetOldAddressCheckMultisig)
    {
        SelectParams(CBaseChainParams::TESTNET);
        // Check that the old address is deprecated
        BOOST_CHECK(CBitcoinAddress::IsDeprecated(testnetOldAddressMultisig));
        // Get new address from the old one
        std::string newAddress = CBitcoinAddress::ConvertToNew(testnetOldAddressMultisig);
        // Check that the new address is not deprecated
        BOOST_CHECK(!CBitcoinAddress::IsDeprecated(newAddress));
    }

    BOOST_AUTO_TEST_CASE(TestnetNewAddressCheckMultisig)
    {
        SelectParams(CBaseChainParams::TESTNET);
        // Check that the new address is not deprecated
        BOOST_CHECK(!CBitcoinAddress::IsDeprecated(testnetNewAddressMultisig));
        // Get old address from the new one 
        std::string oldAddress = CBitcoinAddress::ConvertToOld(testnetNewAddressMultisig);
        // Check that the old address deprecated
        BOOST_CHECK(CBitcoinAddress::IsDeprecated(oldAddress));
    }

BOOST_AUTO_TEST_SUITE_END()
